// 2번 자동논공 새 엔진 — R42i2 (2026-11-04) · R42i3(11-05) 판 확인 고침 · R42i4(11-05) 1.0.1 주소로 바로잡음 SAN14_AutoMerit.dll (설정 파일 SAN14Mods\modules\AutoPersonnelRepairMerit.ini)
//  왜: 통합편의(소스 없음)의 자동논공은 게임 논공 확정 함수(손 논공·AI 논공이 함께 쓰는 1.0.1 0x1ed8f0)를 거치지 않고
//      자기 함수(IC+0x56b20)로 무장 칸 4개(계급·상위 보상 수·내정 보상 수·행동 표시)만 맞춘다.
//      제보(11-04 삼갤러, 13번 안 씀): 자동논공으로 받은 내정갯수+2·시정레벨+1 이 세이브 불러오면 사라지고 병사+2000 만 남음.
//  어떻게: ICGuard 가 IC 논공 실행을 감쌀 때 SAN14_AutoMeritExec 를 부른다. IC 가 고른 계획(무장·목표 상위/내정 수)을
//      손 논공 화면과 같은 형식(std::map<무장번호, 상위|내정<<4> + 논공 주는 사람)으로 만들어 게임 논공 확정 함수에 넘긴다.
//      무엇을 줄지(2번 설정)는 IC 가 고른 그대로, 적용·저장은 게임이 손 논공과 똑같이 한다.
//  안전:
//   (1) 실행 파일 지문(빌드 시각·크기·함수 앞 16바이트)이 다르면 손 안 댐 → 옛 방식(IC 원래 처리)
//   (2) 표의 '논공 주는 사람'(+0x10): 게임 논공 확정은 이 칸으로 '어느 세력인지'만 얻음(13번 분석, F_VALID → 가상함수 +0x80 = 세력).
//       그래서 계획 무장들과 같은 세력의 군주를 넣음(넣기 전에 군주·무장 세력이 같은지 다시 확인).
//       R42i2: R42i 의 '게임 논공을 지켜보는 끼어들기'(AI 0x2ca66·손 0x6cba7c)를 없앰 — 통합편의도 0x2ca66 을 알고 있어
//       원본 검사에 걸리면 자동논공·투자를 스스로 끌 수 있었음. 이제 이 모듈은 게임 코드를 한 바이트도 안 바꿈.
//   (3) 게임 처리 스레드에서만(게임 창 스레드)
//   (4) 확정 직전 Pending=1, 끝나면 0 → 그 사이 튕기면 다음 실행에서 새 엔진 끔 + 오류창(글자 복사해 댓글)
//   (5) 게임이 거절하고 아무것도 안 바뀌었으면 그 턴은 옛 방식으로
//   (6) 13번(논공 가챠)이 깔려 있으면 아무것도 안 함(그때는 자동논공 자체가 꺼지고 잠김)
//   표 칸은 게임 할당기로 만들고 해제하지 않음(게임이 해제해도 짝이 맞고, 안 해도 턴당 수십 바이트).
//  기록(SAN14_기록\적용확인.log): R42D 2 SAN14_AutoMerit.dll start / exec / fallback / drop(불러온 뒤 보상 수가 줄면)
#include "common.h"
#include "r39_record.h"
using U64=uint64_t;
static const S14_CoreAPI*core;static U8*game;
namespace am{
// R42i4: 삼국지14 CE 한국어 1.0.1 기준값(1.0.1 해제본·mt/src 리졸버로 확인). R42i~i3 은 1.0.0 값(0x6a6d9f72·0x1ebca0·0x32e4c0)을 잘못 써서 늘 판 다름이었음
const U32 STAMP=S14_GAME_STAMP,IMAGE=S14_GAME_IMAGE;   // version.h 한 곳에서 관리
const U32 RVA_COMMIT=0x1ed8f0,RVA_VALID=0x331780,RVA_ALLOC=0x3f90e0,RVA_DB=0x2780f60;
const U32 DB_PERSONS=0x158,DB_WORLD=0x871e8;
const U32 P_ID=0x10,P_RULER=0xe6,P_FLAGS=0x15e,P_RANK=0x1bc,P_UPPER=0x1c2,P_LOWER=0x1c3,VT_FORCE=0x80;
const U8 G_COMMIT[16]={0x41,0x56,0x48,0x83,0xec,0x30,0x48,0x83,0x79,0x08,0x00,0x4c,0x8b,0xf1,0x77,0x09};
const U8 G_VALID[16]={0x48,0x83,0xec,0x28,0x48,0x85,0xc9,0x74,0x14,0x48,0x8b,0x01,0xff,0x50,0x18,0x85};
const unsigned MAX_PLAN=200,MAX_TRACK=256;
}
using namespace am;
// ---------- 상태
static volatile LONG ver_ok,merit13,engine_on=1,disabled;
static volatile LONG fn_commit_state,fn_valid_state;   // 0 원본 1 다른 모드 우회 점프 2 알 수 없게 바뀜
static char fn_hex[2][40];
static volatile LONG t_calls,t_new,t_old,t_mismatch;
static volatile LONG why_count[32];
static U32 person_cap=6000,person_off=DB_PERSONS;
static const char*MOD="SAN14_AutoMerit.dll";
// ---------- 작은 도구
static bool rd(const void*p,size_t n){return p&&core&&core->Readable((void*)p,n);}
template<class T> static T R(const void*p,size_t o){return *reinterpret_cast<const T*>(static_cast<const U8*>(p)+o);}
static U8*db(){U8**s=(U8**)(game+RVA_DB);return rd(s,8)?*s:nullptr;}
static U8*world(){U8*d=db();return d&&rd(d+DB_WORLD,8)?R<U8*>(d,DB_WORLD):nullptr;}
static long cur_ym(U8*w){if(!w||!rd(w,0x40))return -1;int y=R<short>(w,0x34),m=R<U8>(w,0x36);if(m<1||m>12)return -1;return (long)y*12+(m-1);}
static bool valid(void*p){return p&&fn<int(*)(void*)>(game,RVA_VALID)(p)!=0;}
static bool in_image(const void*p){return (const U8*)p>=game&&(const U8*)p<game+IMAGE;}
static U8*person(U32 id){U8*d=db();if(!d||id>=person_cap||!rd(d+person_off+(size_t)id*8,8))return nullptr;U8*p=R<U8*>(d,person_off+(size_t)id*8);return rd(p,0x1c4)?p:nullptr;}
static bool person_ok(U8*p){return p&&rd(p,0x1c4)&&person(R<U16>(p,P_ID))==p;}
static U8*force_of(U8*p){
 if(!p||!rd(p,8))return nullptr;U8*vt=R<U8*>(p,0);if(!in_image(vt)||!rd(vt+VT_FORCE,8))return nullptr;
 void*f=R<void*>(vt,VT_FORCE);if(!in_image(f)||!valid(p))return nullptr;return reinterpret_cast<U8*(*)(U8*)>(f)(p);
}
static bool ini_path(wchar_t*p){if(!r39::game_root(p,1024))return false;r39::wcat(p,1024,L"SAN14Mods\\modules\\AutoPersonnelRepairMerit.ini");return true;}
static int ini_get(const wchar_t*k,int d){wchar_t p[1024];return ini_path(p)?(int)GetPrivateProfileIntW(L"AutoMerit",k,d,p):d;}
static void ini_set(const wchar_t*k,unsigned v){wchar_t p[1024],b[16];if(!ini_path(p))return;int n=0;wchar_t t[16];do{t[n++]=(wchar_t)(L'0'+v%10);v/=10;}while(v);for(int i=0;i<n;i++)b[i]=t[n-1-i];b[n]=0;WritePrivateProfileStringW(L"AutoMerit",k,b,p);}
static void line(const char*kind,const char*text){
 char m[900];m[0]=0;const size_t C=sizeof m;
 r39::scat(m,C,"R42D\t2\t");r39::scat(m,C,MOD);r39::scat(m,C,"\t");r39::scat(m,C,kind);r39::scat(m,C,"\t");r39::scat(m,C,text);r39::scat(m,C,"\t");r39::stamp(m,C);r39::append_line(m);
}
static DWORD WINAPI box_thread(void*t){r39::error_box((const wchar_t*)t);return 0;}
static void box(const wchar_t*t){HANDLE h=CreateThread(nullptr,0,box_thread,(void*)t,0,nullptr);if(h)CloseHandle(h);}
// ---------- 게임 창 스레드 (게임 처리 기준)
// user32 는 게임이 이미 올려 둔 것을 이름으로 찾아 씀(도구 묶음 가져오기 표에 MessageBoxW 뿐)
static volatile LONG win_thread;
static struct{BOOL(*enumw)(BOOL(*)(HWND,UPTR),UPTR);DWORD(*tpid)(HWND,DWORD*);BOOL(*visible)(HWND);HWND(*getw)(HWND,unsigned);}u32f;
static BOOL enum_win(HWND h,UPTR){
 DWORD pid=0,t=u32f.tpid(h,&pid);
 if(pid==GetCurrentProcessId()&&u32f.visible(h)&&!u32f.getw(h,4/*GW_OWNER*/)){InterlockedExchange(&win_thread,(LONG)t);return FALSE;}
 return TRUE;
}
static void find_window_thread(){
 HMODULE m=GetModuleHandleW(L"user32.dll");if(!m)return;
 u32f.enumw=(BOOL(*)(BOOL(*)(HWND,UPTR),UPTR))GetProcAddress(m,"EnumWindows");u32f.tpid=(DWORD(*)(HWND,DWORD*))GetProcAddress(m,"GetWindowThreadProcessId");
 u32f.visible=(BOOL(*)(HWND))GetProcAddress(m,"IsWindowVisible");u32f.getw=(HWND(*)(HWND,unsigned))GetProcAddress(m,"GetWindow");
 if(u32f.enumw&&u32f.tpid&&u32f.visible&&u32f.getw)u32f.enumw(enum_win,0);
}
static bool thread_ok(){
 DWORD t=GetCurrentThreadId();
 if(!win_thread)find_window_thread();
 return win_thread&&(DWORD)win_thread==t;
}
// ---------- 이유 (기록·시험에서 같은 번호)
enum{W_READY=0,W_ENGINE_OFF=1,W_VERSION=2,W_MERIT13=3,W_CRASHED=4,W_NOT_LEARNED=5,W_LEARN_BAD=6,W_THREAD=7,
     W_VEC=10,W_PERSON=11,W_TARGET=12,W_FORCE=13,W_RULER=14,W_ALLOC=15,W_FUNC_CHANGED=16,W_REJECTED=20};
static const char*why_text(int w){
 switch(w){case W_ENGINE_OFF:return "engine_off(ini Engine=0)";case W_VERSION:return "game_version_differs";case W_MERIT13:return "merit13_installed";
 case W_CRASHED:return "disabled_after_stop_during_commit";
 case W_THREAD:return "not_game_thread";case W_VEC:return "ic_plan_invalid";case W_PERSON:return "ic_plan_officer_invalid";case W_TARGET:return "ic_plan_target_invalid";
 case W_FORCE:return "ic_plan_mixed_forces";case W_RULER:return "ruler_not_found";case W_ALLOC:return "alloc_failed";case W_REJECTED:return "game_rejected_nothing_changed";case W_FUNC_CHANGED:return "game_merit_function_changed_by_other_mod(unknown_form)";}
 return "?";
}
static bool merit13_loaded(){return merit13||GetModuleHandleW(L"SAN14_MeritTraits.dll")!=nullptr;}
static int ready_reason(){
 if(!ver_ok)return W_VERSION;if(merit13_loaded())return W_MERIT13;if(fn_commit_state==2||fn_valid_state==2)return W_FUNC_CHANGED;if(!engine_on)return W_ENGINE_OFF;if(disabled)return W_CRASHED;
 if(!thread_ok())return W_THREAD;return W_READY;
}
static volatile LONG version_boxed;
static void fallback(int w){
 if(w<0||w>=32)w=31;LONG n=InterlockedIncrement(&why_count[w]);InterlockedIncrement(&t_old);
 if(n<=3){char t[200];t[0]=0;r39::scat(t,sizeof t,"reason=");r39::snum(t,sizeof t,(unsigned)w);r39::scat(t,sizeof t," ");r39::scat(t,sizeof t,why_text(w));
  r39::scat(t,sizeof t," -> old IC merit this turn (count=");r39::snum(t,sizeof t,(unsigned)n);r39::scat(t,sizeof t,")");line("fallback",t);}
 // 20차(P227): 새 엔진을 못 써 옛 방식(불러오면 내정 보상이 사라지는 통합편의 처리)으로 갈 때 조용히 가지 않는다 — 이유를 적은 창 1번(실행마다).
 //  13번이 깔려 있으면 ICGuard 가 자동논공 자체를 끄므로 여기 안 옴(옛 ICGuard 와 섞인 경우만 → 창 없음). Engine=0 은 사용자가 끈 것 → 창 없음.
 if(w!=W_VERSION&&w!=W_MERIT13&&w!=W_ENGINE_OFF){char ac[96];ac[0]=0;r39::scat(ac,sizeof ac,"reason=");r39::snum(ac,sizeof ac,(unsigned)w);r39::scat(ac,sizeof ac," ");r39::scat(ac,sizeof ac,why_text(w));
  r39::fail(2,MOD,"fallback","SAN14_AutoMeritExec (IC auto merit -> game merit commit 0x1ed8f0)","new engine used",ac,
   L"이번 자동논공은 예전 방식(통합편의 처리)으로 됨 → 세이브를 불러오면 내정 보상(담당)이 사라지고 병력만 남을 수 있음. 손 논공은 그대로 정상. 이 창의 글자를 댓글로 알려 주세요.");}
 if(w==W_VERSION&&!InterlockedExchange(&version_boxed,1)){U32 ts=0,sz=0;r39::exe_info(&ts,&sz);char ex[64],ac[64];ex[0]=ac[0]=0;
  r39::scat(ex,sizeof ex,"stamp=0x");r39::shex(ex,sizeof ex,STAMP);r39::scat(ex,sizeof ex," size=0x");r39::shex(ex,sizeof ex,IMAGE);
  r39::scat(ac,sizeof ac,"stamp=0x");r39::shex(ac,sizeof ac,ts);r39::scat(ac,sizeof ac," size=0x");r39::shex(ac,sizeof ac,sz);
  r39::fail(2,MOD,"version","fallback() at IC auto-merit exec",ex,ac,L"자동논공 새 엔진 꺼짐 → 예전 방식(통합편의)으로만 동작");}
}
// ---------- 게임 형식 표(std::map<u32,u8>) 만들기
struct Node{U8*left,*parent,*right;U8 color,isnil,pad[2];U32 key;U8 val,pad2[7];};
static_assert(sizeof(Node)==0x28,"map node");
struct Batch{U8*head;U64 size;U8*giver;};
static U8*galloc(size_t n){return fn<U8*(*)(U64)>(game,RVA_ALLOC)(n);}
// ids 는 오름차순·중복 없음. 오른쪽으로만 이어진 사슬 = 올바른 이진 탐색 나무(순회·찾기만 쓰면 충분)
static bool build_map(Batch*b,const U32*ids,const U8*vals,unsigned n){
 Node*h=(Node*)galloc(sizeof(Node));if(!h)return false;memset(h,0,sizeof*h);h->isnil=1;h->color=1;
 h->left=h->right=h->parent=(U8*)h;Node*prev=nullptr,*first=nullptr;
 for(unsigned i=0;i<n;i++){
  Node*x=(Node*)galloc(sizeof(Node));if(!x)return false;memset(x,0,sizeof*x);
  x->key=ids[i];x->val=vals[i];x->left=x->right=(U8*)h;x->color=1;
  if(!prev){x->parent=(U8*)h;first=x;}else{x->parent=(U8*)prev;prev->right=(U8*)x;}
  prev=x;
 }
 if(first){h->parent=(U8*)first;h->left=(U8*)first;h->right=(U8*)prev;}
 b->head=(U8*)h;b->size=n;return true;
}
// ---------- IC 계획 읽기 (항목 0x28: +0 무장, +0x10 예상 계급, +0x1c 예상 상위 수, +0x20 예상 내정 수)
struct Item{U8*o;U32 id;U32 rank,upper,lower;U8 b_rank,b_upper,b_lower;U16 b_flags;};
static int prepare(U64 vec,Item*it,unsigned*n,U8**giver){
 *n=0;*giver=nullptr;U8**v=(U8**)vec;if(!rd(v,16))return W_VEC;U8*b=v[0],*e=v[1];
 if(!b||e<=b||(size_t)(e-b)%0x28||(size_t)(e-b)/0x28>MAX_PLAN||!rd(b,(size_t)(e-b)))return W_VEC;
 unsigned c=(unsigned)((e-b)/0x28);U8*force=nullptr;
 for(unsigned i=0;i<c;i++){
  U8*en=b+(size_t)i*0x28;Item&x=it[i];x.o=R<U8*>(en,0);
  if(!person_ok(x.o)||!valid(x.o))return W_PERSON;
  x.id=R<U16>(x.o,P_ID);x.rank=R<U32>(en,0x10);x.upper=R<U32>(en,0x1c);x.lower=R<U32>(en,0x20);
  x.b_rank=R<U8>(x.o,P_RANK);x.b_upper=R<U8>(x.o,P_UPPER);x.b_lower=R<U8>(x.o,P_LOWER);x.b_flags=R<U16>(x.o,P_FLAGS);
  if(x.upper>3||x.lower>3||x.upper+x.lower>3||x.upper<x.b_upper||x.lower<x.b_lower||x.rank>15)return W_TARGET;
  for(unsigned j=0;j<i;j++)if(it[j].id==x.id)return W_VEC;
  U8*f=force_of(x.o);if(!f)return W_FORCE;if(!force)force=f;else if(f!=force)return W_FORCE;
 }
 // 논공 주는 사람 = 그 세력 군주 (게임 논공을 지켜봐서 확인한 형식)
 for(U32 id=0;id<person_cap;id++){U8*p=person(id);if(!p||R<U8>(p,P_RULER)!=1||!valid(p))continue;if(force_of(p)==force){*giver=p;break;}}
 if(!*giver)return W_RULER;
 // 번호 오름차순으로
 for(unsigned i=1;i<c;i++){Item k=it[i];unsigned j=i;while(j&&it[j-1].id>k.id){it[j]=it[j-1];j--;}it[j]=k;}
 *n=c;return W_READY;
}
static bool unchanged(const Item*it,unsigned n){
 for(unsigned i=0;i<n;i++){const Item&x=it[i];if(R<U8>(x.o,P_RANK)!=x.b_rank||R<U8>(x.o,P_UPPER)!=x.b_upper||R<U8>(x.o,P_LOWER)!=x.b_lower||R<U16>(x.o,P_FLAGS)!=x.b_flags)return false;}
 return true;
}
// ---------- 불러온 뒤 보상 수가 줄었는지 (자동논공으로 준 무장만, 5초마다, 바이트 몇 개만 읽음)
struct Track{U16 id;U8 upper,lower,path;long ym;};
static Track trk[MAX_TRACK];static unsigned trk_n;static SRWLOCK trk_lock=SRWLOCK_INIT;static volatile LONG t_drop;
static void track(U8*o,int path){
 if(!o)return;long ym=cur_ym(world());U16 id=R<U16>(o,P_ID);AcquireSRWLockExclusive(&trk_lock);
 unsigned k=trk_n;for(unsigned i=0;i<trk_n;i++)if(trk[i].id==id){k=i;break;}
 if(k==trk_n){if(trk_n<MAX_TRACK)trk_n++;else k=0;}
 trk[k]={id,R<U8>(o,P_UPPER),R<U8>(o,P_LOWER),(U8)path,ym};ReleaseSRWLockExclusive(&trk_lock);
}
static U8*last_db,*last_world;static long last_ym=-1;
static void drop_check(){
 U8*d=db(),*w=world();long ym=cur_ym(w);if(!d||!w||ym<0)return;
 bool loaded=last_world&&(w!=last_world||d!=last_db||ym<last_ym||ym>last_ym+2);
 last_db=d;last_world=w;last_ym=ym;if(!loaded)return;
 AcquireSRWLockExclusive(&trk_lock);
 for(unsigned i=0;i<trk_n;i++){const Track&t=trk[i];if(ym<=t.ym)continue;U8*o=person(t.id);if(!o)continue;
  U8 u=R<U8>(o,P_UPPER),l=R<U8>(o,P_LOWER);if(u>=t.upper&&l>=t.lower)continue;
  if(InterlockedIncrement(&t_drop)<=20){char m[200];m[0]=0;r39::scat(m,sizeof m,"after load officer #");r39::snum(m,sizeof m,t.id);
   r39::scat(m,sizeof m," upper ");r39::snum(m,sizeof m,t.upper);r39::scat(m,sizeof m,"->");r39::snum(m,sizeof m,u);r39::scat(m,sizeof m," lower ");r39::snum(m,sizeof m,t.lower);
   r39::scat(m,sizeof m,"->");r39::snum(m,sizeof m,l);r39::scat(m,sizeof m,t.path?" (given by NEW engine)":" (given by OLD IC merit)");line("drop",m);}
 }
 trk_n=0;ReleaseSRWLockExclusive(&trk_lock);
}
static DWORD WINAPI watcher(void*){for(;;){Sleep(5000);drop_check();}}
// ---------- ICGuard 가 IC 논공 실행(IC+0x56b20) 자리에서 부름
using IcExec=U64(*)(U64,U64,U64,U64);
static void track_vec(U64 vec,int path){U8**v=(U8**)vec;if(!rd(v,16))return;U8*b=v[0],*e=v[1];if(!b||e<=b||(size_t)(e-b)%0x28||(size_t)(e-b)/0x28>MAX_PLAN)return;
 for(U8*en=b;en<e;en+=0x28){if(!rd(en,8))return;U8*o=R<U8*>(en,0);if(person_ok(o))track(o,path);}}
extern "C" __declspec(dllexport) U64 SAN14_AutoMeritExec(U64 ctx,U64 vec,U64 a3,U64 a4,IcExec orig){
 if(!orig)return 0;InterlockedIncrement(&t_calls);
 static Item it[MAX_PLAN];unsigned n=0;U8*giver=nullptr;
 int w=ready_reason();if(!w)w=prepare(vec,it,&n,&giver);
 if(w){fallback(w);U64 r=orig(ctx,vec,a3,a4);if((U8)r)track_vec(vec,0);return r;}
 U32 ids[MAX_PLAN];U8 vals[MAX_PLAN];for(unsigned i=0;i<n;i++){ids[i]=it[i].id;vals[i]=(U8)((it[i].upper&15)|((it[i].lower&15)<<4));}
 Batch bt{};if(!build_map(&bt,ids,vals,n)){fallback(W_ALLOC);return orig(ctx,vec,a3,a4);}
 bt.giver=giver;
 ini_set(L"Pending",1);
 int r=fn<int(*)(void*)>(game,RVA_COMMIT)(&bt);
 ini_set(L"Pending",0);
 if(!r&&unchanged(it,n)){fallback(W_REJECTED);U64 o=orig(ctx,vec,a3,a4);if((U8)o)track_vec(vec,0);return o;}
 unsigned mism=0;char det[300];det[0]=0;
 for(unsigned i=0;i<n;i++){const Item&x=it[i];U8 rk=R<U8>(x.o,P_RANK),u=R<U8>(x.o,P_UPPER),l=R<U8>(x.o,P_LOWER);bool acted=(R<U16>(x.o,P_FLAGS)&0x8000)!=0;
  if(rk!=(U8)x.rank||u!=(U8)x.upper||l!=(U8)x.lower||!acted){mism++;if(mism<=4){r39::scat(det,sizeof det," #");r39::snum(det,sizeof det,x.id);r39::scat(det,sizeof det,":want r");r39::snum(det,sizeof det,x.rank);
   r39::scat(det,sizeof det,"/u");r39::snum(det,sizeof det,x.upper);r39::scat(det,sizeof det,"/l");r39::snum(det,sizeof det,x.lower);r39::scat(det,sizeof det," got r");r39::snum(det,sizeof det,rk);
   r39::scat(det,sizeof det,"/u");r39::snum(det,sizeof det,u);r39::scat(det,sizeof det,"/l");r39::snum(det,sizeof det,l);r39::scat(det,sizeof det,acted?"":" not_acted");}}
  track(x.o,1);
 }
 InterlockedIncrement(&t_new);if(mism)InterlockedExchange(&t_mismatch,t_mismatch+(LONG)mism);
 if((r39::s14_diag()&&t_new<=5)||mism){char m[500];m[0]=0;r39::scat(m,sizeof m,"native commit officers=");r39::snum(m,sizeof m,n);r39::scat(m,sizeof m," result=");r39::snum(m,sizeof m,(unsigned)(r!=0));
  r39::scat(m,sizeof m," differs_from_ic_plan=");r39::snum(m,sizeof m,mism);r39::scat(m,sizeof m,det);line("exec",m);}
 return (r&&!mism)?1:0;   // 다르면 0 → ICGuard 가 그 무장만 6턴 빼고 기록(예전 IC 검사와 같은 뜻)
}
// ---------- 상태(설정 창·시험용)
extern "C" __declspec(dllexport) int SAN14_AutoMeritState(int which){
 switch(which){case 0:return ready_reason();case 1:return (int)t_calls;case 2:return (int)t_new;case 3:return (int)t_old;
 case 6:return (int)t_mismatch;case 7:return (int)t_drop;}return -1;
}
// ---------- 시작
// R42i3: 게임 판 = 빌드 시각·이미지 크기(13번 리졸버와 같은 기준). 함수 앞 16바이트는 따로 봄:
//  0 원본 · 1 다른 모드의 우회 점프(뒤쪽은 원본) → 그대로 부름(게임이 부르는 것과 같음) · 2 알 수 없게 바뀜 → 새 엔진 안 씀(옛 방식), 바이트를 기록
//  (R42i2 데모에서 version=DIFFERS — 13번 깔린 PC. 앞부분을 누가 바꿨는지 기록으로 보려고 hex 를 남김)
static int jump_kind(const U8*c,const U8*orig){
 if(!memcmp(c,orig,16))return 0;
 if(c[0]==0xe9&&!memcmp(c+6,orig+6,10))return 1;                               // jmp rel32 (+nop)
 if(c[0]==0xff&&c[1]==0x25&&!memcmp(c+14,orig+14,2))return 1;                  // jmp [rip+0] + 8바이트 주소
 if(c[0]==0x48&&c[1]==0xb8&&c[10]==0xff&&c[11]==0xe0&&!memcmp(c+12,orig+12,4))return 1; // mov rax,imm64; jmp rax
 if(c[0]==0x49&&c[1]==0xbb&&c[10]==0x41&&c[11]==0xff&&c[12]==0xe3&&!memcmp(c+13,orig+13,3))return 1; // mov r11,imm64; jmp r11
 return 2;
}
static void hex16(const U8*c,char*o){static const char*h="0123456789abcdef";for(int i=0;i<16;i++){o[i*2]=h[c[i]>>4];o[i*2+1]=h[c[i]&15];}o[32]=0;}
static bool version_ok(){
 if(!rd(game,0x400))return false;U32 nt=R<U32>(game,0x3c);if(nt<64||nt>4096||!rd(game+nt,0x58))return false;
 if(R<U32>(game,nt)!=0x4550||R<U32>(game,nt+8)!=STAMP||R<U32>(game,nt+0x50)!=IMAGE)return false;
 if(!rd(game+RVA_COMMIT,16)||!rd(game+RVA_VALID,16))return false;
 InterlockedExchange(&fn_commit_state,jump_kind(game+RVA_COMMIT,G_COMMIT));InterlockedExchange(&fn_valid_state,jump_kind(game+RVA_VALID,G_VALID));
 hex16(game+RVA_COMMIT,fn_hex[0]);hex16(game+RVA_VALID,fn_hex[1]);
 return true;
}
static bool file_in_root(const wchar_t*name){wchar_t p[1024];if(!r39::game_root(p,1024))return false;r39::wcat(p,1024,name);return GetFileAttributesW(p)!=INVALID_FILE_ATTRIBUTES;}
static void read_limits(){
 if(!core->resolved->ReadLimits)return;S14_Limits l{};if(!core->resolved->ReadLimits(&l))return;
 if(l.person_table==DB_PERSONS&&l.officer_max>=6000&&l.officer_max<=20000)person_cap=l.officer_max;
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,2,MOD))return 0;core=api;game=api->resolved->image;read_limits();
 InterlockedExchange(&ver_ok,version_ok()?1:0);
 if(!ver_ok){ // R43: 조용히 쉬지 않는다 — 어디가 어떻게 다른지 바로 창
  U32 ts=0,sz=0;r39::exe_info(&ts,&sz);char ex[96],ac[96];ex[0]=ac[0]=0;
  r39::scat(ex,sizeof ex,"stamp=0x");r39::shex(ex,sizeof ex,STAMP);r39::scat(ex,sizeof ex," size=0x");r39::shex(ex,sizeof ex,IMAGE);r39::scat(ex,sizeof ex," F_COMMIT/F_VALID readable");
  r39::scat(ac,sizeof ac,"stamp=0x");r39::shex(ac,sizeof ac,ts);r39::scat(ac,sizeof ac," size=0x");r39::shex(ac,sizeof ac,sz);
  if(ts==STAMP&&sz==IMAGE)r39::scat(ac,sizeof ac," (header same; F_COMMIT exe+0x1ed8f0 or F_VALID exe+0x331780 unreadable)");
  InterlockedExchange(&version_boxed,1);
  r39::fail(2,MOD,"version","version_ok() <- SAN14_ModInitV1; game F_COMMIT exe+0x1ed8f0, F_VALID exe+0x331780",ex,ac,L"자동논공 새 엔진 꺼짐 → 예전 방식(통합편의)으로만 동작. 세이브 뒤 내정 보상 수가 사라지는 문제(2번)가 다시 생길 수 있음");
 }else if(fn_commit_state==2||fn_valid_state==2){
  char ex[80],ac[80];ex[0]=ac[0]=0;bool c=fn_commit_state==2;
  r39::sbytes(ex,sizeof ex,c?G_COMMIT:G_VALID,16);r39::scat(ac,sizeof ac,fn_hex[c?0:1]);
  r39::fail(2,MOD,"funcbytes",c?"game merit commit F_COMMIT exe+0x1ed8f0 first 16 bytes":"game merit valid F_VALID exe+0x331780 first 16 bytes",ex,ac,L"다른 모드가 게임 논공 함수를 알 수 없는 모양으로 바꿈 → 자동논공 새 엔진 꺼짐(예전 방식으로만 동작)");
 }
 InterlockedExchange(&merit13,(file_in_root(L"SAN14_MeritTraits.dll")||GetModuleHandleW(L"SAN14_MeritTraits.dll"))?1:0);
 InterlockedExchange(&engine_on,ini_get(L"Engine",1)?1:0);
 if(ini_get(L"Pending",0)){ini_set(L"Pending",0);ini_set(L"Disabled",1);
  r39::fail(2,MOD,"crash_guard","previous run: Pending=1 left in AutoPersonnelRepairMerit.ini [AutoMerit]","Pending=0 after each commit","Pending=1 (game stopped during new-engine commit)",L"자동논공 새 엔진을 껐음(Disabled=1). 자동논공은 예전 방식으로 계속 동작. 다시 켜려면 ini 의 Disabled=0");
  line("start","previous run stopped during new-engine commit -> Disabled=1");}
 InterlockedExchange(&disabled,ini_get(L"Disabled",0)?1:0);
 HANDLE h=CreateThread(nullptr,0,watcher,nullptr,0,nullptr);if(h)CloseHandle(h);
 char t[700];t[0]=0;const size_t C=sizeof t;
 r39::scat(t,C,ver_ok?"version=OK":"version=DIFFERS");
 {static const char*st[3]={"original","other_mod_jump","CHANGED"};
  if(ver_ok&&(fn_commit_state||fn_valid_state)){r39::scat(t,C," commit=");r39::scat(t,C,st[fn_commit_state%3]);if(fn_commit_state){r39::scat(t,C,"(");r39::scat(t,C,fn_hex[0]);r39::scat(t,C,")");}
   r39::scat(t,C," valid=");r39::scat(t,C,st[fn_valid_state%3]);if(fn_valid_state){r39::scat(t,C,"(");r39::scat(t,C,fn_hex[1]);r39::scat(t,C,")");}}}r39::scat(t,C,merit13?" merit13=installed(idle)":" merit13=no");
 r39::scat(t,C," code_patch=none");
 r39::scat(t,C,engine_on?" engine=1":" engine=0");r39::scat(t,C,disabled?" disabled=1":" disabled=0");
 line("start",t);
 if(core->Log)core->Log("AutoMerit R42i4: IC auto-merit plan is committed through the game's own merit commit (same path as manual/AI merit).");
 return 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
