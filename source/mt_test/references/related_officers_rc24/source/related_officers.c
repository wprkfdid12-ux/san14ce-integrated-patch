/*
 * SAN14CE Related Officers Tab RC2.4 all-officer-list coverage.
 *
 * Scope is deliberately narrow: attach the game's native relation metadata to
 * common officer-table windows. Relation data, commands, selection limits,
 * native trait/hover code and the original sortie relation UI are not replaced.
 */
#include "runtime_common.h"
#include "relation_api.h"
static const S14_CoreAPI *core;
static const RAPI *r;
static struct {
 HANDLE (*GetProcessHeap)(void);
 void *(*HeapAlloc)(HANDLE,U32,SIZE);
 BOOL (*HeapFree)(HANDLE,U32,void *);
} heap;
static HANDLE heap_handle;
static U32 enabled,serial,officer_cap;
static U32 skip_counts[16];

typedef struct {
 U32 live,generation,tab_count,relation_index,column_count,syncing,dirty,overflow_noted,closing;
 void *parent,*list,*pinned,*preview,*suppress_hover; U32 readonly_noted; RTab *original_tabs,*tabs; RColumn *columns;
 I32 relation_indices[REL_SLOTS+2];
 void *compare[REL_SLOTS];
 void **selected; U32 selected_count;
 U16 labels[REL_SLOTS][256];
} Context;
static Context contexts[REL_WINDOWS];
static I32 rd(const void *p,SIZE n){return core && core->Readable && core->Readable(p,n);}
static void *ptr(const void *p,U32 off){return *(void *const *)((const U8 *)p+off);}
static const char *rtti_name(void *dlg){
 U8 *base,*vt,*col,*td;SIZE image_size;U32 rv,i;const char *name;
 if(!core || !core->resolved || !dlg || !rd(dlg,8))return 0;
 base=core->resolved->image;image_size=core->resolved->image_size;vt=ptr(dlg,0);
 if(!vt || !rd(vt-8,8))return 0;col=*(U8 **)(vt-8);
 if(!col || !rd(col,24) || *(U32 *)col!=1)return 0;rv=*(U32 *)(col+12);
 if(rv>=image_size || image_size-rv<24)return 0;td=base+rv;if(!rd(td,128))return 0;name=(const char *)td+16;
 for(i=0;i<104;++i){if(!name[i])return name;}return 0;
}
static I32 integer(const void *p,U32 off){return *(const I32 *)((const U8 *)p+off);}
static void *alloc(SIZE n){return heap.HeapAlloc(heap_handle,8,n);}
static void release(void *p){if(p)heap.HeapFree(heap_handle,0,p);}
static const U16 *empty(void){return r && r->empty_slot && rd(r->empty_slot,8)?*r->empty_slot:L"";}
static I32 copy_text(U16 *out,U32 cap,const U16 *s){
 U32 i;out[0]=0;if(!s)return 0;
 for(i=0;i<cap;++i){if(!rd(s+i,2)){out[0]=0;return 0;}out[i]=s[i];if(!s[i])return 1;}
 out[0]=0;return 0;
}
static I32 same_text(const U16 *a,const U16 *b){
 U32 i;if(a==b)return 1;if(!a || !b)return 0;
 for(i=0;i<64;++i){if(!rd(a+i,2) || !rd(b+i,2) || a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;
}
static Context *by_parent(void *p){U32 i;for(i=0;i<REL_WINDOWS;++i)if(contexts[i].live && contexts[i].parent==p)return &contexts[i];return 0;}
static Context *by_list_direct(void *p){U32 i;for(i=0;i<REL_WINDOWS;++i)if(contexts[i].live && contexts[i].list==p)return &contexts[i];return 0;}
static I32 bind_list(Context *c,void *list){
 if(!c || !c->live || !list || !rd(list,r->l.list_count+4))return 0;
 if(ptr(list,r->l.list_tabs)!=c->tabs || ptr(list,r->l.list_columns)!=c->columns)return 0;
 c->list=list;return 1;
}
static I32 ensure_list(Context *c){
 void *list;if(!c || !c->live)return 0;if(c->list)return 1;
 if(!c->parent || !rd(c->parent,r->l.dlg_list+8))return 0;list=ptr(c->parent,r->l.dlg_list);
 return bind_list(c,list);
}
static Context *by_list(void *list){
 Context *c;U32 i;if((c=by_list_direct(list)))return c;
 for(i=0;i<REL_WINDOWS;++i)if(contexts[i].live && contexts[i].parent && rd(contexts[i].parent,r->l.dlg_list+8) && ptr(contexts[i].parent,r->l.dlg_list)==list && bind_list(&contexts[i],list))return &contexts[i];
 return 0;
}
static Context *by_body(void *body){
 U32 i;void *tabs;if(!rd(body,r->l.body_tabs+8))return 0;tabs=ptr(body,r->l.body_tabs);
 for(i=0;i<REL_WINDOWS;++i)if(contexts[i].live && contexts[i].tabs==tabs)return &contexts[i];
 return 0;
}
static void *active_body(Context *c){
 I32 k;void **v;if(!ensure_list(c) || !rd(c->list,r->l.list_count+4))return 0;
 k=integer(c->list,r->l.list_index);if(k<0 || (U32)k>=c->tab_count)return 0;
 v=ptr(c->list,r->l.list_bodies);if(!rd(v,c->tab_count*8))return 0;return v[k];
}
static I32 contains(void **a,U32 n,void *p){U32 i;for(i=0;i<n;++i)if(a[i]==p)return 1;return 0;}
static I32 snapshot(Context *c,void *body){
 void **rows,**heads;U8 *node;void *handle;U32 h,n,steps=0,capacity;I32 k;
 c->selected_count=0;
 if(!rd(body,r->l.body_tabs+8) || ptr(body,r->l.body_tabs)!=c->tabs)return 0;
 k=integer(body,r->l.body_count);if(k<0 || (U32)k>officer_cap)return 0;n=(U32)k;
 rows=ptr(body,r->l.body_rows);if(n && !rd(rows,n*8))return 0;
 handle=ptr(body,r->l.body_checked);if(!handle)return 1;if(!rd(handle,4))return 0;h=*(U32 *)handle;
 if(!rd(r->node_capacity_slot,4) || !rd(r->node_table_slot,8))return 0;
 capacity=*r->node_capacity_slot;heads=*r->node_table_slot;
 if(h>=capacity || !heads || !rd(heads+h,8))return 1;
 node=heads[h];
 while(node){
  if(++steps>officer_cap+1 || !rd(node,16))return 0;k=*(I32 *)node;
  if(k<0 || (U32)k>=n)return 0;
  if(rows[k] && !contains(c->selected,c->selected_count,rows[k])){
   if(c->selected_count>=officer_cap)return 0;c->selected[c->selected_count++]=rows[k];
  }
  node=*(U8 **)(node+8);
 }
 return 1;
}
static I32 readonly_context(Context *c,void *body){
 return c && c->live && ensure_list(c) && body && r->ReadOnly && r->ReadOnly(c->parent,c->list,body);
}
static I32 relation_active(Context *c,void *body){
 return c && c->live && ensure_list(c) && active_body(c)==body && integer(c->list,r->l.list_index)==(I32)c->relation_index;
}
static I32 valid_person(void *p){return p && rd(p,8) && rd(ptr(p,0),32) && r->Valid(p);}
/* Window-local pointer identity (native CPersonData objects) is independent of
   visible row indices. The window owns no game object and writes no check set.
   A fixed person therefore survives scrolling, sorting and filtering. */
static I32 sync_data(Context *c,void *body,void *preferred){
 void *next[REL_SLOTS]={0};U32 i,n=0;I32 ok,changed;
 if(c->syncing || !c->live)return 0;c->syncing=1;
 if(readonly_context(c,body)){
  if(c->pinned && !valid_person(c->pinned))c->pinned=0;
  if(c->preview && !valid_person(c->preview))c->preview=0;
  next[0]=c->pinned?c->pinned:c->preview;ok=1;
 }else{
 ok=snapshot(c,body);
 if(ok){
  for(i=0;i<REL_SLOTS;++i)if(c->compare[i] && contains(c->selected,c->selected_count,c->compare[i]))next[n++]=c->compare[i];
  if(n<REL_SLOTS && preferred && contains(c->selected,c->selected_count,preferred) && !contains(next,n,preferred))next[n++]=preferred;
  for(i=0;i<c->selected_count && n<REL_SLOTS;++i)if(!contains(next,n,c->selected[i]))next[n++]=c->selected[i];
  if(c->selected_count>REL_SLOTS && !c->overflow_noted){c->overflow_noted=1;core->Log("RelatedOfficersTab: native nine comparison columns; command selection beyond nine is preserved.\r\n");}
 }
 }
 changed=ok && !memeq(next,c->compare,sizeof(next));
 if(changed){
  memcopy(c->compare,next,sizeof(next));c->dirty=1;
  for(i=0;i<REL_SLOTS;++i){c->labels[i][0]=0;if(next[i] && r->Valid(next[i]))copy_text(c->labels[i],256,r->Name(next[i]));c->columns[c->column_count-REL_SLOTS+i].header=c->labels[i];}
 }
 c->syncing=0;return changed;
}
static void sync_and_refresh(Context *c,void *body,void *preferred){
 I32 active;if(!c || !c->live || !ensure_list(c) || active_body(c)!=body)return;
 sync_data(c,body,preferred);active=integer(c->list,r->l.list_index)==(I32)c->relation_index;
 if(active && c->dirty){c->dirty=0;r->Headers(c->list,(I32)c->relation_index);if(c->live && active_body(c)==body)r->Rows(body);}
}
static const U16 *cell(U32 slot,U32 col,void *person){
 Context *c=&contexts[slot];U32 result[4]={0};void *other;
 if(!c->live || col>=REL_SLOTS)return empty();other=c->compare[col];
 if(!other || !person || !r->Valid(other) || !r->Valid(person))return empty();
 r->Pair(result,person,other);return r->RelationText((I32)result[2]);
}
static void *key(U32 slot,U32 col,void *out,void *person){
 Context *c=&contexts[slot];memzero(out,16);return r->Pair(out,person,c->live && col<REL_SLOTS?c->compare[col]:0);
}
#include "relation_callbacks.h"
/* Scope: every common CPersonData list/select dialog that exposes the native tab bar.
   Do not classify by InfoDetail RTTI name: that family also backs legitimate multi-officer
   information routes such as force/all-force/city officer lists. Existing native relation
   pages are detected from metadata and never duplicated. */
static void discard(Context *c){release(c->tabs);release(c->columns);release(c->selected);memzero(c,sizeof(*c));}
/* Only log the first few occurrences per reason to avoid frame/log floods. */
static void log_skip(U32 code,const char *why){
 if(code<16 && skip_counts[code]++<4){core->Log("RelatedOfficersTab RC2.4: ATTACH_SKIPPED: ");core->Log(why);core->Log("\r\n");}
}
static Context *prepare(void *dlg,void *config){
 RTab *oldtabs;RColumn *oldcols;Context *c=0;U32 i,j,count=0,ncols,slot=0;I32 max=-1,index;U16 relation_label[32];
 if(!rd(config,r->l.cfg_size)){log_skip(0,"unreadable config");return 0;}
 if(!integer(config,r->l.cfg_bar)){log_skip(1,"native tab bar disabled");return 0;}
 if(by_parent(dlg))return 0;oldtabs=ptr(config,r->l.cfg_tabs);oldcols=ptr(config,r->l.cfg_columns);
 if(!oldtabs || !oldcols || !copy_text(relation_label,32,r->Resource(r->tab_resource))){log_skip(3,"metadata/resource unavailable");return 0;}
 for(count=0;count<64;++count){
  RTab *t=oldtabs+count;if(!rd(t,sizeof(*t))){log_skip(4,"unreadable tab metadata");return 0;}if(!t->columns)break;
  if(t->resource==(I32)r->tab_resource || (t->label && same_text(t->label,relation_label)))return 0;
  for(j=0;j<256;++j){if(!rd(t->columns+j,4))return 0;index=t->columns[j];if(index==-1)break;if(index<0 || index>=2048){log_skip(5,"invalid column index");return 0;}if(index>max)max=index;}
  if(j==256)return 0;
 }
 if(count<1 || count>=64 || max<0){log_skip(6,"unsupported native tab count");return 0;}
 ncols=r->base_column_count;if((U32)(max+1)>ncols)ncols=(U32)max+1;
 if(ncols>2048 || !rd(oldcols,(SIZE)ncols*sizeof(*oldcols))){log_skip(7,"unreadable column span");return 0;}
 /* Native descriptor id is a semantic/sort identifier, NOT its array slot.
    The supplied game's slot 13 has id 8 (and slots 14..17 reuse 9..12).
    Keep all original descriptor bytes; validate actual array references instead. */
 for(i=0;i<REL_WINDOWS;++i)if(!contexts[i].live && !contexts[i].closing){c=&contexts[i];slot=i;break;}
 if(!c){core->Log("RelatedOfficersTab: context capacity reached; original window unchanged.\r\n");return 0;}
 memzero(c,sizeof(*c));c->tabs=alloc((count+2)*sizeof(RTab));c->columns=alloc((ncols+REL_SLOTS)*sizeof(RColumn));c->selected=alloc(officer_cap*8);
 if(!c->tabs || !c->columns || !c->selected){discard(c);return 0;}
 memcopy(c->tabs,oldtabs,count*sizeof(RTab));memcopy(c->columns,oldcols,ncols*sizeof(RColumn));
 c->relation_indices[0]=0;
 for(i=0;i<REL_SLOTS;++i){RColumn *d=&c->columns[ncols+i];*d=r->native_column;d->id=ncols+i;d->header=c->labels[i];get_callbacks(slot,i,&d->Text,&d->Key);c->relation_indices[i+1]=(I32)(ncols+i);}
 c->relation_indices[REL_SLOTS+1]=-1;
 /* Use the original native resource id instead of manufacturing a new label. */
 c->tabs[count].columns=c->relation_indices;c->tabs[count].label=0;c->tabs[count].resource=(I32)r->tab_resource;c->tabs[count].pad=0;
 c->parent=dlg;c->original_tabs=oldtabs;c->tab_count=count+1;c->relation_index=count;c->column_count=ncols+REL_SLOTS;c->dirty=1;c->generation=++serial;c->live=1;
 return c;
}
/* Called only after the game's config-copy/composition path has run. This lets
   Traits9/UI_Hover expand its native tables first; we then copy that result and
   append relation metadata without touching any of its hooks or descriptors. */
static void attach_after_copy(void *dlg,void *config){
 Context *c;if(!enabled || !(c=prepare(dlg,config)))return;
 *(RTab **)((U8 *)config+r->l.cfg_tabs)=c->tabs;*(RColumn **)((U8 *)config+r->l.cfg_columns)=c->columns;
 {const char *name=rtti_name(dlg);core->Log("RelatedOfficersTab RC2.4: ATTACHED class=");core->Log(name?name:"<unknown>");core->Log("\r\n");}
}
static I32 pin_event(void *dlg,void *event){
 Context *c=enabled?by_parent(dlg):0;void *body,*person;void **rows;I32 index,count;
 if(!c || !ensure_list(c) || !(body=active_body(c)) || !relation_active(c,body) ||
    !readonly_context(c,body) || !rd(event,r->l.event_index+4) || !rd(body,r->l.body_count+4))return 0;
 index=integer(event,r->l.event_index);count=integer(body,r->l.body_count);rows=ptr(body,r->l.body_rows);
 if(index<0 || index>=count || count<1 || (U32)count>officer_cap || !rd(rows,(SIZE)count*8))return 0;
 person=rows[index];if(!valid_person(person))return 0;
 if(c->pinned==person){c->pinned=0;c->preview=0;c->suppress_hover=person;core->Log("RelatedOfficersTab RC2.4: VIRTUAL_UNPIN\r\n");}
 else{c->pinned=person;c->preview=0;c->suppress_hover=0;core->Log("RelatedOfficersTab RC2.4: VIRTUAL_PIN\r\n");}
 /* The native input callback may still be walking row widgets. Change only
    our reference data here; repaint is deferred until the post-input observer. */
 sync_data(c,body,0);return 1;
}
static SIZE selection(void *dlg,void *event){
 Context *c=enabled?by_parent(dlg):0;U32 generation=c?c->generation:0;void *body=0,*preferred=0;I32 index,count;void **rows;SIZE result;
 if(c && ensure_list(c) && (body=active_body(c)) && rd(event,r->l.event_index+4) && rd(body,r->l.body_count+4)){
  index=integer(event,r->l.event_index);count=integer(body,r->l.body_count);rows=ptr(body,r->l.body_rows);
  if(index>=0 && index<count && (U32)count<=officer_cap && rd(rows,(SIZE)count*8))preferred=rows[index];
 }
 if(pin_event(dlg,event))return 0;
 result=r->Native.Select(dlg,event);
 if(c && c->live && c->generation==generation && ensure_list(c) && (body=active_body(c)))sync_and_refresh(c,body,preferred);
 return result;
}
/* This callback runs before the game's original body-activation virtual call.
   The resolver bridge then invokes that exact original virtual target. */
static void activation(void *body){
 Context *c=enabled?by_body(body):0;
 if(c && ensure_list(c) && active_body(c)==body){c->preview=0;c->suppress_hover=0;sync_data(c,body,0);}
}
/* No polling thread, Win32 mouse hook, vtable replacement or synthesized clicks.
   Called on the existing game's UI thread, after its normal row/input update. */
static void tick(void *body){
 Context *c=enabled?by_body(body):0;void *hover;
 if(!c || !ensure_list(c) || active_body(c)!=body || !readonly_context(c,body))return;
 if(!relation_active(c,body)){c->preview=0;c->suppress_hover=0;return;}
 if(!c->readonly_noted){c->readonly_noted=1;core->Log("RelatedOfficersTab RC2.4: READONLY_RELATION_INPUT_READY\r\n");}
 hover=c->pinned?0:r->HoverPerson(body,c->list);
 if(c->suppress_hover && hover!=c->suppress_hover)c->suppress_hover=0;
 c->preview=(!c->pinned && hover!=c->suppress_hover)?hover:0;
 sync_and_refresh(c,body,0);
}
static SIZE destruction(void *list){
 Context *c=by_list(list);SIZE result;if(c){c->closing=1;c->live=0;}result=r->Native.Destroy(list);if(c)discard(c);return result;
}
U32 SAN14_ModInitV1(const S14_CoreAPI *h){
 HANDLE k,m;RResolve resolve;RCallbacks cb;S14_Limits limits;
 if(enabled)return 1;
 if(!h || h->size<sizeof(*h) || h->abi!=S14_ABI || !h->resolved || !h->Readable || !h->Log || !h->resolved->ReadLimits)return 0;
 core=h;core->Log("RelatedOfficersTab RC2.4: MODULE_INIT_ENTERED\r\n");k=win.GetModuleHandleW(L"kernel32.dll");if(!k)return 0;
 heap.GetProcessHeap=(__typeof__(heap.GetProcessHeap))win.GetProcAddress(k,"GetProcessHeap");
 heap.HeapAlloc=(__typeof__(heap.HeapAlloc))win.GetProcAddress(k,"HeapAlloc");heap.HeapFree=(__typeof__(heap.HeapFree))win.GetProcAddress(k,"HeapFree");
 if(!heap.GetProcessHeap || !heap.HeapAlloc || !heap.HeapFree)return 0;heap_handle=heap.GetProcessHeap();if(!heap_handle)return 0;
 if(!h->resolved->ReadLimits(&limits) || !limits.officer_max || limits.officer_max>6000)return 0;officer_cap=limits.officer_max;
 m=load_relative_game(L"SAN14Mods\\resolvers\\SAN14_RelatedOfficersResolver.dll");
 if(!m){core->Log("RelatedOfficersTab: private resolver DLL could not be loaded.\r\n");return 0;}
 resolve=(RResolve)win.GetProcAddress(m,"SAN14_RelatedResolverV1");
 if(!resolve || !(r=resolve(h)) || r->size<sizeof(*r) || r->abi!=REL_ABI || !r->ReadOnly || !r->HoverPerson){core->Log("RelatedOfficersTab: resolver ABI/build/signature/layout validation failed; feature not installed.\r\n");return 0;}
 cb.Attach=attach_after_copy;cb.Select=selection;cb.Activate=activation;cb.Destroy=destruction;cb.Tick=tick;cb.Pin=pin_event;
 if(!r->Install(&cb)){core->Log("RelatedOfficersTab: hook conflict detected; feature not installed and no partial hooks were committed.\r\n");return 0;}
 enabled=1;core->Log("RelatedOfficersTab RC2.4: HOOKS_INSTALLED / 4Mods-safe sites / native checks + readonly hover/pin.\r\n");return 1;
}
BOOL DllMain(HANDLE module,U32 reason,void *reserved){(void)module;(void)reason;(void)reserved;return 1;}
