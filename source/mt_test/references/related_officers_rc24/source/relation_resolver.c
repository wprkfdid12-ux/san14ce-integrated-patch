/*
 * Feature-specific resolver for RelatedOfficersTab RC2.4.
 * Shared RC23 Loader/Core/Resolver remain byte-identical.
 *
 * RC2 deliberately avoids the PersonListBody tick entry (0x50e1a0 on the
 * supported reference) because TraitHover verifies that native prologue.
 */
#include "runtime_common.h"
#include "relation_api.h"
#include "relation_profile.h"
#include "input_profile.h"
static const S14_CoreAPI *host;
static RAPI api;
static U8 *found[RP_COUNT],*hook_site[6];
static U8 hook_len[6]={7,5,6,5,5,7};
SIZE relation_select_continuation;
SIZE relation_late_callback;
SIZE relation_activate_callback;
SIZE relation_input_callback;
SIZE relation_input_getter;
SIZE relation_dispatch_continuation;
static I32 (*relation_pin_callback)(void *,void *);
static U8 resolved_expected[6][16];
extern __attribute__((visibility("hidden"))) SIZE rel_original_select(void *,void *);
extern __attribute__((visibility("hidden"))) SIZE rel_late_bridge(void);
extern __attribute__((visibility("hidden"))) SIZE rel_activate_bridge(void);
/*
 * Selection entry trampoline replays only the verified relocation-free first
 * instruction. The activation bridge replaces a native CALL. The attach bridge replaces
 * a MOV at an unconditional join and explicitly saves all volatile registers.
 */
__asm__(".text\n.globl rel_original_select\n.type rel_original_select,@function\nrel_original_select:\n"
        "movq %rbx,8(%rsp)\njmp *relation_select_continuation(%rip)\n"
        ".size rel_original_select,.-rel_original_select\n"
        ".globl rel_late_bridge\n.type rel_late_bridge,@function\nrel_late_bridge:\n"
        "subq $184,%rsp\n"
        "movq %rax,32(%rsp)\n"
        "movq %rdx,40(%rsp)\n"
        "movq %r8,48(%rsp)\n"
        "movq %r9,56(%rsp)\n"
        "movq %r10,64(%rsp)\n"
        "movq %r11,72(%rsp)\n"
        "movdqu %xmm0,80(%rsp)\n"
        "movdqu %xmm1,96(%rsp)\n"
        "movdqu %xmm2,112(%rsp)\n"
        "movdqu %xmm3,128(%rsp)\n"
        "movdqu %xmm4,144(%rsp)\n"
        "movdqu %xmm5,160(%rsp)\n"
        "movq %rdi,%rcx\n"
        "movq 0x1a8(%rdi),%rdx\n"
        "call *relation_late_callback(%rip)\n"
        "movdqu 80(%rsp),%xmm0\n"
        "movdqu 96(%rsp),%xmm1\n"
        "movdqu 112(%rsp),%xmm2\n"
        "movdqu 128(%rsp),%xmm3\n"
        "movdqu 144(%rsp),%xmm4\n"
        "movdqu 160(%rsp),%xmm5\n"
        "movq 32(%rsp),%rax\n"
        "movq 40(%rsp),%rdx\n"
        "movq 48(%rsp),%r8\n"
        "movq 56(%rsp),%r9\n"
        "movq 64(%rsp),%r10\n"
        "movq 72(%rsp),%r11\n"
        "movq 0x1a8(%rdi),%rcx\n"
        "addq $184,%rsp\n"
        "ret\n"
        ".size rel_late_bridge,.-rel_late_bridge\n"
        ".globl rel_activate_bridge\n.type rel_activate_bridge,@function\nrel_activate_bridge:\n"
        "subq $56,%rsp\n"
        "movq %rax,32(%rsp)\n"
        "movq %rcx,40(%rsp)\n"
        "call *relation_activate_callback(%rip)\n"
        "movq 40(%rsp),%rcx\n"
        "movq 32(%rsp),%rax\n"
        "call *0x180(%rax)\n"
        "addq $56,%rsp\nret\n"
        ".size rel_activate_bridge,.-rel_activate_bridge\n");
static U8 *unique(U8 *text,U32 size,const RPattern *p) {
 U32 skip[256],i,j,at,last=p->alen-1;U8 *one=0,*q;
 if(!p->n || p->n>size || p->alen<4 || p->anchor+p->alen>p->n)return 0;
 for(i=0;i<256;++i)skip[i]=p->alen;
 for(i=0;i<last;++i)skip[p->bytes[p->anchor+i]]=last-i;
 at=p->anchor;
 while(at<=size-p->alen) {
  if(memeq(text+at,p->bytes+p->anchor,p->alen)) {
   if(at>=p->anchor && at-p->anchor<=size-p->n) {
    q=text+at-p->anchor;
    for(j=0;j<p->n;++j)if(p->mask[j] && q[j]!=p->bytes[j])break;
    if(j==p->n){if(one)return 0;one=q;}
   } ++at;
  } else at+=skip[text[at+last]];
 }return one;
}
extern __attribute__((visibility("hidden"))) SIZE rel_input_bridge(void);
/* Original CALL had no arguments; its result is still consumed by the native
   tick. Preserve the returned RAX across the observer callback. RBX is the
   current body per the validated surrounding native tick instructions. */
__asm__(".text\n.globl rel_input_bridge\n.type rel_input_bridge,@function\nrel_input_bridge:\n"
        "subq $56,%rsp\n"
        "call *relation_input_getter(%rip)\n"
        "movq %rax,32(%rsp)\n"
        "movq %rbx,%rcx\n"
        "call *relation_input_callback(%rip)\n"
        "movq 32(%rsp),%rax\n"
        "addq $56,%rsp\nret\n"
        ".size rel_input_bridge,.-rel_input_bridge\n");
#include "relation_input_resolver.h"
extern __attribute__((visibility("hidden"))) SIZE rel_original_dispatch(void *,void *);
__asm__(".text\n.globl rel_original_dispatch\n.type rel_original_dispatch,@function\nrel_original_dispatch:\n"
        "movq 8(%rcx),%rcx\nmovq (%rdx),%rdx\n"
        "jmp *relation_dispatch_continuation(%rip)\n"
        ".size rel_original_dispatch,.-rel_original_dispatch\n");
/* Native std::function row delegate. InfoList does NOT use the checkbox
   selection override; it dispatches its own virtual handler via this thunk.
   Preserve that dispatch for every non-owned window/other tab/native check. */
static SIZE rel_dispatch_bridge(void *delegate,void *event_ref){
 if(ri_read(delegate,16) && ri_read(event_ref,8) &&
    relation_pin_callback(ri_ptr(delegate,8),ri_ptr(event_ref,0)))return 0;
 return rel_original_dispatch(delegate,event_ref);
}

static I32 in_image(const void *p,SIZE n) {
 SIZE v=(SIZE)p-(SIZE)host->resolved->image;
 return v<host->resolved->image_size && n<=host->resolved->image_size-v && host->Readable(p,n);
}
static U8 *rip(U8 *instruction,U32 displacement,U32 end) {
 U8 *p=instruction+end+*(I32 *)(instruction+displacement);return in_image(p,8)?p:0;
}
/* Find the common tab-switch call to the already-resolved native Headers
   function. There are multiple xrefs; require the exact common-list register
   setup used by this window family, then derive/validate the preceding body
   activation virtual call. No fixed RVA fallback is used. */
static U8 *header_switch_call(U8 *text,U32 size,U8 *headers){
 U32 i;U8 *one=0,*p;I32 d;
 for(i=5;i+5<=size;++i){p=text+i;if(*p!=0xe8)continue;d=*(I32 *)(p+1);if(p+5+d!=headers)continue;
  if(!memeq(p-5,"\x8b\xd5\x48\x8b\xcb",5))continue;
  if(one)return 0;one=p;
 }
 return one;
}
static I32 install(const RCallbacks *cb) {
 S14_Hook h[6];U8 *relay[6]={0};U32 i,j;SIZE dest[6];I32 ok=0;static U32 installed;
 if(installed || !cb || !cb->Attach || !cb->Select || !cb->Activate || !cb->Destroy || !cb->Tick || !cb->Pin)return 0;
 relation_late_callback=(SIZE)cb->Attach;relation_activate_callback=(SIZE)cb->Activate;relation_input_callback=(SIZE)cb->Tick;relation_pin_callback=cb->Pin;
 dest[0]=(SIZE)rel_late_bridge;dest[1]=(SIZE)cb->Select;dest[2]=(SIZE)rel_activate_bridge;dest[3]=(SIZE)cb->Destroy;dest[4]=(SIZE)rel_input_bridge;dest[5]=(SIZE)rel_dispatch_bridge;
 memzero(h,sizeof(h));
 for(i=0;i<6;++i){
  h[i].target=hook_site[i];h[i].length=hook_len[i];memcopy(h[i].expected,resolved_expected[i],hook_len[i]);
  relay[i]=host->MakeRelay(hook_site[i],(const void *)dest[i]);if(!relay[i])goto done;
  h[i].replacement[0]=(i==1 || i==5)?0xe9:0xe8;
  if((long long)(relay[i]-hook_site[i]-5)<-2147483648LL || (long long)(relay[i]-hook_site[i]-5)>2147483647LL)goto done;
  *(I32 *)(h[i].replacement+1)=(I32)(relay[i]-hook_site[i]-5);for(j=5;j<hook_len[i];++j)h[i].replacement[j]=0x90;
 }
 ok=host->InstallHooks("RelatedOfficersTab.RC2_4.HoverPin",h,6);if(ok)installed=1;
 done:if(!ok)for(i=0;i<6;++i)if(relay[i])host->FreeRelay(relay[i]);return ok;
}
const RAPI *SAN14_RelatedResolverV1(const S14_CoreAPI *h) {
 U8 *base,*nt,*sections,*text=0,*p,*q,*header_call,*native_input,*fn;U32 pe,n,i,tsize=0;RLayout l;
 static const U8 leaf_head[5]={0x48,0x89,0x5c,0x24,0x08};
 if(!h || h->size<sizeof(*h) || h->abi!=S14_ABI || !h->resolved || !h->Readable || !h->InstallHooks || !h->MakeRelay || !h->FreeRelay)return 0;
 if(h->resolved->layout_abi!=1 || h->resolved->image_size!=0x029ff000)return 0;
 host=h;base=h->resolved->image;if(!base || !h->Readable(base,4096))return 0;pe=*(U32 *)(base+0x3c);if(pe<64 || pe>3000)return 0;nt=base+pe;
 if(*(U32 *)nt!=0x4550 || *(U16 *)(nt+4)!=0x8664 || *(U16 *)(nt+24)!=0x20b || *(U32 *)(nt+8)!=0x6a6d9f72)return 0;
 n=*(U16 *)(nt+6);if(!n || n>32)return 0;sections=nt+24+*(U16 *)(nt+20);if(!h->Readable(sections,n*40))return 0;
 for(i=0;i<n;++i)if(memeq(sections+i*40,".text\0",6)){if(text)return 0;text=base+*(U32 *)(sections+i*40+12);tsize=*(U32 *)(sections+i*40+8);if(!in_image(text,tsize))return 0;}
 if(!text || !tsize)return 0;
 for(i=0;i<RP_COUNT;++i){
  /* RC2 neither hooks nor depends on PersonListBody_Tick. In particular this
     leaves TraitHover's 0x50e1a0 signature ownership completely untouched. */
  if(i==RP_TICK){found[i]=0;continue;}
  found[i]=unique(text,tsize,&rp[i]);
  if(!found[i]){const char *names[RP_COUNT]={"INIT","TICK","SELECT","DTOR_WRAPPER","VALID","PAIR","REL_TEXT","NAME","RESOURCE","HEADERS","ROWS","CHECKED","SORTIE_INIT","SORTIE_TEXT"};if(h->Log){h->Log("RelatedOfficersTab: missing/ambiguous signature ");h->Log(names[i]);h->Log("\r\n");}return 0;}
 }
 /* Common join AFTER optional title/caption setup and BEFORE tab enumeration.
    RC2's caption CALL at 0x2c00c8 was conditional; the join is not.
    Do not move attachment earlier: native title setup compares the borrowed
    tab pointer by identity and must see the original pointer.
    The 7-byte MOV is replaced with CALL + 2 NOPs. The bridge preserves every
    volatile GP/XMM register, replays that exact MOV, and returns to the native
    CMP. Flags from the callback cannot escape: CMP immediately replaces them.
    A local instruction signature finds the join; there is no RVA fallback. */
 hook_site[0]=0;
 for(i=0;i+33<0x600;++i){
  p=found[RP_INIT]+i;
  if(in_image(p,33) && memeq(p,"\x48\x8b\x8f\xa8\x01\x00\x00\x83\xb9\x9c\x00\x00\x00\x00\x0f\x84",16) &&
     memeq(p+20,"\x48\x8b\x49\x08\x48\x83\x39\x00",8)){
   if(hook_site[0])return 0;hook_site[0]=p;
  }
 }
 if(!hook_site[0]){if(h->Log)h->Log("RelatedOfficersTab RC2.4: unconditional attach boundary missing.\r\n");return 0;}
 hook_site[1]=found[RP_SELECT];if(!memeq(hook_site[1],leaf_head,5))return 0;relation_select_continuation=(SIZE)hook_site[1]+5;
 header_call=header_switch_call(text,tsize,found[RP_HEADERS]);if(!header_call || header_call<text+0x22)return 0;
 hook_site[2]=header_call-0x22;
 if(!memeq(hook_site[2],"\xff\x90\x80\x01\x00\x00",6))return 0;
 /* Validate the whole local transition so another build/layout cannot be
    mistaken for the supported common-list tab switch. */
 if(!memeq(header_call-0x30,"\x48\x8b\x83\x30\x01\x00\x00",7) || !memeq(header_call-5,"\x8b\xd5\x48\x8b\xcb",5))return 0;
 hook_site[3]=found[RP_DTOR_WRAPPER]+15;if(*hook_site[3]!=0xe8)return 0;
 /* Fifth hook is the interior CALL after normal input, NOT the protected
    first 16 bytes or any TraitHover-owned vtable slot. */
 hook_site[4]=exact_unique(text,tsize,ip_POST_INPUT,sizeof(ip_POST_INPUT));
 p=exact_unique(text,tsize,ip_ROW_INDEX,sizeof(ip_ROW_INDEX));
 q=exact_unique(text,tsize,ip_ROW_WIDGET,sizeof(ip_ROW_WIDGET));
 fn=exact_unique(text,tsize,ip_HOVER_TARGET,sizeof(ip_HOVER_TARGET));
 if(!hook_site[4] || !p || !q || !fn){if(h->Log)h->Log("RelatedOfficersTab RC2.4: INPUT_SIGNATURE_UNAVAILABLE; nothing installed.\r\n");return 0;}
 rel_row_index=(__typeof__(rel_row_index))p;rel_row_widget=(__typeof__(rel_row_widget))q;
 rel_hover_target=(__typeof__(rel_hover_target))fn;
 if(!in_image(hook_site[4]-5,5) || hook_site[4][-5]!=0xe8)return 0;
 native_input=rip(hook_site[4]-5,1,5);p=rip(hook_site[4],1,5);
 if(!native_input || !p || !in_image(native_input,0x4b))return 0;
 relation_input_getter=(SIZE)p;
 if(native_input[0x26]!=0xe8 || native_input[0x46]!=0xe8 ||
    !memeq(native_input+0x23,"\x48\x8b\xf1",3))return 0;
 p=rip(native_input+0x26,1,5);q=rip(native_input+0x46,1,5);
 if(!p || !q)return 0;
 rel_get_app=(__typeof__(rel_get_app))p;rel_get_input=(__typeof__(rel_get_input))q;
 hook_site[5]=exact_unique(text,tsize,ip_ROW_DISPATCH,sizeof(ip_ROW_DISPATCH));
 if(!hook_site[5]){if(h->Log)h->Log("RelatedOfficersTab RC2.4: ROW_DISPATCH_SIGNATURE_UNAVAILABLE\r\n");return 0;}
 relation_dispatch_continuation=(SIZE)hook_site[5]+7;
 /* Confirm this is the row-selection delegate actually bound by the same
    common officer initializer, rather than assuming all SELECT overrides match. */
 q=0;
 for(i=0;i<0xc00-64;++i){
  p=found[RP_INIT]+i;
  if(in_image(p,64) && memeq(p,"\x48\x8d\x05",3)){
   fn=rip(p,3,7);
   if(fn && in_image(fn,24) && *(U8 **)(fn+16)==hook_site[5] &&
      memeq(p+27,"\xba\x14\x05\x00\x00",5)){if(q)return 0;q=p;}
  }
 }
 if(!q){if(h->Log)h->Log("RelatedOfficersTab RC2.4: native row delegate binding not verified.\r\n");return 0;}
 memzero(&api,sizeof(api));api.size=sizeof(api);api.abi=REL_ABI;
 l=(RLayout){0x400,8,16,0x9c,0x18,0x180,0x130,0x150,0x158,0x208,0x20c,0x198,0x1a0,0x1a8,0x1b0,0x1b8,0x158,0x80};api.l=l;
 api.Native.Select=rel_original_select;p=rip(hook_site[3],1,5);if(!p)return 0;api.Native.Destroy=(__typeof__(api.Native.Destroy))p;
 api.Valid=(__typeof__(api.Valid))found[RP_VALID];api.Pair=(__typeof__(api.Pair))found[RP_PAIR];api.RelationText=(__typeof__(api.RelationText))found[RP_REL_TEXT];api.Name=(__typeof__(api.Name))found[RP_NAME];
 api.Resource=(__typeof__(api.Resource))found[RP_RESOURCE];api.Headers=(__typeof__(api.Headers))found[RP_HEADERS];api.Rows=(__typeof__(api.Rows))found[RP_ROWS];
 p=found[RP_CHECKED];api.node_table_slot=(void ***)rip(p+0x15,3,7);api.node_capacity_slot=(U32 *)rip(p+0x23,2,6);api.empty_slot=(const U16 **)rip(found[RP_SORTIE_TEXT]+0x4f,3,7);
 if(!api.node_table_slot || !api.node_capacity_slot || !api.empty_slot)return 0;
 p=found[RP_SORTIE_INIT];if(!in_image(p,0x500))return 0;
 if(!memeq(p+0x5c,"\x41\xbe",2) || !memeq(p+0x163,"\x48\x83\xfe",3) || p[0x205]!=0xb9)return 0;
 api.base_column_count=*(U32 *)(p+0x5e);api.compare_count=p[0x166];api.tab_resource=*(U32 *)(p+0x206);
 if(!api.base_column_count || api.base_column_count>2048 || api.compare_count!=REL_SLOTS || api.tab_resource!=0x235)return 0;
 api.native_column.width=*(U32 *)(p+0x3d0);api.native_column.flags=*(U32 *)(p+0x3d7);api.native_column.align=*(U32 *)(p+0x3e9);
 q=rip(p+0x404,4,8);if(!q || !in_image(q,16))return 0;memcopy(api.native_column.native_tail,q,16);api.native_column.visible=*(U32 *)(p+0x418);
 if(api.native_column.width<32 || api.native_column.width>512 || !api.native_column.visible)return 0;
 for(i=0;i<6;++i)memcopy(resolved_expected[i],hook_site[i],hook_len[i]);api.Install=install;api.ReadOnly=read_only_mode;api.HoverPerson=hover_person;return &api;
}
BOOL DllMain(HANDLE module,U32 reason,void *reserved){(void)module;(void)reason;(void)reserved;return 1;}
