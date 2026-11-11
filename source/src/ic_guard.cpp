// R40 통합편의 장기 안정(ICGuard) — 1번 통합편의(IC) 본체 파일은 그대로 두고, 게임이 켜진 뒤 IC 코드에만 작게 덧댄다.
//  원칙(사용자 2026-10-28): 오래 해도 모드가 계속 돌아가야 함. 검사는 그대로, 걸리면 그 하나만 건너뛰고 나머지는 계속 + 사유 기록.
//  묶음 A (2번 자동인사, A11)
//   A-1 워커 0x18001591d: 투자·논공 단계(0x180006840)가 실패해도 탐색은 진행(je 6바이트 → nop). 탐색은 자기 검사(명령서 감소 확인)가 따로 있음.
//   A-2 논공 실행 0x180056b20: 결과 검사 실패면 목록에서 랭크·보상수·행동표시가 예측과 다른 무장을 찾아 6턴 동안 자동 논공 후보에서 뺌.
//   A-3 후보 검사 0x18004c840: 빠진 무장은 후보 아님(0 반환). 0x18002d750(후보 목록 만들기) 부를 때마다 1턴으로 셈.
//  묶음 B (7번 인재·포로 편의 + 1·2번 입력창, A13)
//   창 열기 함수 5개: '열고 있음' 표시를 먼저 켜고, 창이 실제로 만들어졌을 때만 포인터를 저장 → 실패하면 표시가 남아 재시작 전까지 버튼 무반응.
//   감싸서: 부르기 전 표시 0 이었고, 부른 뒤 표시는 켜졌는데 포인터가 0 이면(=창 못 만듦) 표시를 되돌림.
//   포로 목록 512명: 0x18002bb93 'je 실패' → 'je 다음 무장'(512명까지만 담고 계속).
//  두 묶음은 따로 설치(한쪽 실패해도 다른 쪽은 동작). 원래 바이트가 다르면 그 묶음은 손대지 않고 기록만 남김.
#include "module_hooks.h"
#include "r39_record.h"
using U64=uint64_t;
static const S14_CoreAPI*core;static U8*ic;static bool attached,attempted;static SRWLOCK lock=SRWLOCK_INIT;
static const char*MOD="SAN14_ICGuard.dll";
static void logf(const char*s){if(core&&core->Log)core->Log(s);}
static void hex16(char*o,unsigned v){static const char h[]="0123456789abcdef";for(int i=0;i<4;i++)o[i]=h[(v>>(12-4*i))&15];o[4]=0;}
// ---------- 묶음 A ----------
struct Excl{U16 id;U16 tries;U32 turn;};static Excl excl[64];static volatile LONG turn_no;static SRWLOCK xlock=SRWLOCK_INIT;
static unsigned excluded_total,skipped_total;
static bool is_excluded(U16 id){
 if(!id)return false;bool r=false;AcquireSRWLockShared(&xlock);
 for(auto&e:excl)if(e.id==id&&(U32)turn_no-e.turn<6){r=true;break;}
 ReleaseSRWLockShared(&xlock);return r;
}
static void exclude(U16 id){
 if(!id)return;AcquireSRWLockExclusive(&xlock);
 Excl*slot=nullptr;for(auto&e:excl)if(e.id==id){slot=&e;break;}
 if(!slot){for(auto&e:excl)if(!e.id||(U32)turn_no-e.turn>=6){slot=&e;break;}}
 if(!slot){slot=&excl[0];for(auto&e:excl)if(e.turn<slot->turn)slot=&e;}
 bool same=slot->id==id;slot->id=id;slot->turn=(U32)turn_no;slot->tries=same?(U16)(slot->tries+1):(U16)1;U16 tries=slot->tries;
 ReleaseSRWLockExclusive(&xlock);excluded_total++;
 if(tries<=3){char m[160]="ICGuard R40: auto merit result differs for officer #";char h[5];hex16(h,id);r39::scat(m,sizeof m,h);r39::scat(m,sizeof m," -> skipped from auto merit for 6 turns (others continue).");logf(m);}
 r39::counter(2,MOD,"merit_officer_skipped",excluded_total);
}
using Fn2=U64(*)(U64,U64,U64,U64);
static Fn2 o_check,o_exec,o_build;
static bool readable(const void*p,size_t n){return p&&core->Readable((void*)p,n);}
static U64 w_check(U64 ctx,U64 person,U64 out,U64 a4){
 U8*p=(U8*)person;if(readable(p,0x20)&&is_excluded(at<U16>(p,0x10))){skipped_total++;return 0;}
 return o_check(ctx,person,out,a4);
}
static U64 w_build(U64 ctx,U64 out,U64 a3,U64 a4){InterlockedIncrement(&turn_no);return o_build(ctx,out,a3,a4);}
// R42i: 2번 자동논공 새 엔진(SAN14_AutoMerit.dll)이 있으면 그쪽이 IC 계획을 게임 논공 확정 함수로 넣음(못 쓰는 경우엔 그쪽이 IC 원래 처리를 부름).
//  없으면 예전처럼 IC 원래 처리. 설치 순서와 상관없이 부를 때마다 찾음.
using AmExec=U64(*)(U64,U64,U64,U64,Fn2);
// 20차(P227): 13번(논공 가챠)이 깔려 있으면 자동논공은 꺼진 것으로 한다(사용자 규칙). 예전엔 우리 설정 창만 잠그고,
//  통합편의 자기 설정 창·세이브의 정책 비트 4 로 다시 켜지면 통합편의 원래 처리(불러오면 내정 보상이 사라지는 방식)로 돌았음.
//  → 실행 자리에서 건너뛰고, 켬 칸(0x10500d)·정책 비트 4 를 끈다(통합편의 정책 저장 함수 0x65580).
static volatile LONG merit13_skips;
static bool merit13(){return GetModuleHandleW(L"SAN14_MeritTraits.dll")!=nullptr;}
static void merit13_off(){
 if(at<U8>(ic,0x10500d))__atomic_store_n(ic+0x10500d,(U8)0,__ATOMIC_SEQ_CST);
 int pol=at<int>(ic,0x105010);if(pol&4){uint64_t th=(U32)at<int>(ic,0x105014);fn<U8(*)(uint64_t)>(ic,0x65580)((uint64_t)(U32)(pol&~4)|(th<<32));}
}
static U64 w_exec(U64 ctx,U64 vec,U64 a3,U64 a4){
 if(merit13()){merit13_off();if(InterlockedIncrement(&merit13_skips)==1&&core->Log)core->Log("ICGuard R43 K2: item 13 (merit gacha) installed -> IC auto merit skipped and switched off (it was turned on by the IC window or a loaded save).");return 0;}
 HMODULE am=GetModuleHandleW(L"SAN14_AutoMerit.dll");AmExec ax=am?(AmExec)GetProcAddress(am,"SAN14_AutoMeritExec"):nullptr;
 U64 r=ax?ax(ctx,vec,a3,a4,o_exec):o_exec(ctx,vec,a3,a4);
 if((U8)r)return r;
 // 실패: 게임이 논공을 이미 처리했을 수 있음. 무장별로 결과를 다시 보고 다른 무장만 뺀다(명령서·금 불일치 등 무장 탓이 아닌 실패는 빼지 않음).
 U8**v=(U8**)vec;if(!readable(v,16))return r;U8*b=v[0],*e=v[1];
 if(!b||e<b||(size_t)(e-b)%0x28||(size_t)(e-b)>0x28*200)return r;
 unsigned found=0;
 for(U8*it=b;it<e;it+=0x28){
  if(!readable(it,0x28))break;U8*p=at<U8*>(it,0);if(!readable(p,0x1c4))continue;
  bool bad=at<U8>(p,0x1bc)!=(U8)at<U32>(it,0x10)||at<U8>(p,0x1c2)!=(U8)at<U32>(it,0x1c)||at<U8>(p,0x1c3)!=(U8)at<U32>(it,0x20)||!(at<U16>(p,0x15e)&0x8000);
  if(bad){exclude(at<U16>(p,0x10));found++;}
 }
 if(!found){static unsigned n;if(++n<=3)logf("ICGuard R40: auto merit check failed without an officer-level difference (orders/gold); search still runs.");}
 return r;
}
// R43: 묶음별 실패 자리(IC/DomesticCE/AISearch 안 오프셋·기대/실제 바이트) 보관 → 창
static ModuleTransaction::Bad ic_bad[8]; // 0 A 1 B 2 E 3 F 4 C(DomesticCE) 5 D(AISearch) 6 G 7 H
static bool fin(ModuleTransaction&tx,int g){bool ok=tx.install();if(!ok&&g>=0&&g<8)ic_bad[g]=tx.badinfo;return ok;}
static void pre_bad(int g,U8*at,const U8*exp,U32 n,const char*why){if(g<0||g>7)return;ModuleTransaction::Bad&b=ic_bad[g];b.set=true;b.at=at;b.n=n>16?16:n;b.why=why;memcpy(b.ex,exp,b.n);if(at&&core&&core->Readable(at,b.n))memcpy(b.ac,at,b.n);else memset(b.ac,0,16);}
static void report(int g,const char*what,const U8*base,const char*base_name,int feature,const wchar_t*effect){r39::fail_tx(feature,MOD,ic_bad[g],base,base_name,what,effect);}
static bool install_a(){
 ModuleTransaction tx;
 static const U8 je[]={0x0f,0x84,0xb2,0x03,0x00,0x00},nop6[]={0x66,0x0f,0x1f,0x44,0x00,0x00};
 tx.bytes(ic+0x1591d,je,nop6,6);
 static const U8 e_check[]={0x40,0x53,0x55,0x56,0x57},e_exec[]={0x48,0x89,0x5c,0x24,0x18},e_build[]={0x48,0x89,0x5c,0x24,0x08};
 o_check=(Fn2)tx.detour(ic+0x4c840,e_check,5,(void*)w_check);
 o_exec=(Fn2)tx.detour(ic+0x56b20,e_exec,5,(void*)w_exec);
 o_build=(Fn2)tx.detour(ic+0x2d750,e_build,5,(void*)w_build);
 return fin(tx,0);
}
// ---------- 묶음 B ----------
struct Dlg{U32 fn;U8 e[11];U8 n;U32 flag;U8 flag_size;U32 ptr;const char*name;Fn2 orig;unsigned fixed;};
static Dlg dlg[5]={
 {0x68430,{0x4c,0x8b,0xdc,0x55,0x41,0x57},6,0x19e99e,1,0x1a2c48,"captive_dialog",nullptr,0},
 {0x47420,{0x48,0x89,0x5c,0x24,0x10},5,0x1c07fc,1,0x1c0fd0,"troops_input",nullptr,0},
 {0x47870,{0x48,0x89,0x5c,0x24,0x20},5,0x105058,4,0x105048,"captive_list",nullptr,0},
 {0x47f90,{0x40,0x55,0x57,0x48,0x8d,0xac,0x24,0x78,0x24,0xff,0xff},11,0x1c076d,1,0x1a4ef0,"recruit_gate",nullptr,0},
 {0x48480,{0x40,0x55,0x53,0x56,0x57},5,0x1c07fe,1,0x106a50,"reward_input",nullptr,0},
};
static U32 rdflag(const Dlg&d){return d.flag_size==4?at<volatile U32>(ic,d.flag):at<volatile U8>(ic,d.flag);}
static void clrflag(const Dlg&d){if(d.flag_size==4)InterlockedExchange((volatile LONG*)(ic+d.flag),0);else __atomic_store_n(ic+d.flag,(U8)0,__ATOMIC_SEQ_CST);}
// ---------- 묶음 K (P225 11-10): [포로 회유]·[등용문] 버튼의 게임 코드 대조 — 다른 모드가 알려 준 자리만큼 맞춰 줌 (규칙·형식: ic_mapsites.h) ----------
//  통합편의 대조 표(주소·길이·기대 바이트 포인터, 0x18 간격): 포로 = IC+0xc3560(21줄)·0xc58a0(6줄)·0xc5c60(6줄, 0x18003a390)
//   / 등용문 = IC+0xc6f50(14줄, 0x18003a470)·0xc5c60. 네 표는 이 두 기능만 씀(통합편의 안 참조 확인, 기대 바이트 포인터를 다른 표와 안 나눔).
//  버튼을 누를 때마다(=B 의 captive_list·recruit_gate 감싸기 입구) SAN14Mods\sites\*.txt 를 새로 읽음 — 버튼 한 번에 파일 몇 개, 주기적으로 읽지 않음.
#include "ic_mapsites.h"
static U8*game_img(){return core&&core->resolved?core->resolved->image:nullptr;}
static void k_cat(wchar_t*o,size_t cap,const wchar_t*a,const wchar_t*b=nullptr,const wchar_t*c=nullptr){o[0]=0;r39::wcat(o,cap,a);if(b)r39::wcat(o,cap,b);if(c)r39::wcat(o,cap,c);}
static bool k_prefix_ci(const wchar_t*s,const wchar_t*p){auto lc=[](wchar_t c){return (c>=L'A'&&c<=L'Z')?(wchar_t)(c+32):(c==L'/'?L'\\':c);};size_t i=0;for(;p[i];i++)if(lc(s[i])!=lc(p[i]))return false;return true;}
struct KTab{U32 a,b;};
static const KTab k_cap[]={{0xc3560,0xc3758},{0xc58a0,0xc5930},{0xc5c60,0xc5cf0}},k_rec[]={{0xc6f50,0xc70a0},{0xc5c60,0xc5cf0}};
static kms::File k_file;static volatile LONG k_lock,k_clicks,k_applied,k_diff;static bool k_shown[2];static char k_mods[160];
static U32 k_ic_size(){U32 pe=at<U32>(ic,0x3c);return at<U32>(ic,pe+0x18+56);}
static unsigned k_entries(const KTab*t,unsigned nt,kms::Entry*out,unsigned cap){
 unsigned n=0;U32 isz=k_ic_size();UPTR lo=(UPTR)ic,hi=lo+isz;
 for(unsigned i=0;i<nt;i++)for(U32 x=t[i].a;x+0x18<=t[i].b&&n<cap;x+=0x18){
  U64 rva=at<U64>(ic,x),len=at<U64>(ic,x+8),ptr=at<U64>(ic,x+0x10);
  if(rva<0x1000||len<1||len>0x2000||rva+len>S14_GAME_IMAGE||ptr<lo||ptr+len>hi)continue;
  out[n].rva=(uint32_t)rva;out[n].len=(uint32_t)len;out[n].exp=(uint8_t*)ptr;n++;}
 return n;
}
static void k_write(uint8_t*dst,const uint8_t*src,uint32_t n){DWORD old=0;if(VirtualProtect(dst,n,PAGE_READWRITE,&old)){memcpy(dst,src,n);VirtualProtect(dst,n,old,&old);}}
// 한 파일 읽어 자리 적용(맞으면 바뀐 바이트 수 더함). 이름은 k_mods 에 모음.
static unsigned k_one(const wchar_t*path,const kms::Entry*e,unsigned ne){
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(h==INVALID_HANDLE_VALUE)return 0;
 static char buf[1<<20];DWORD got=0;BOOL ok=ReadFile(h,buf,sizeof buf,&got,nullptr);CloseHandle(h);if(!ok||!got)return 0;
 kms::parse(buf,got,S14_GAME_STAMP,GetCurrentProcessId(),S14_GAME_IMAGE,k_file);if(!k_file.ok)return 0;
 if(k_file.mod[0]&&r39::slen(k_mods)+r39::slen(k_file.mod)+3<sizeof k_mods){if(k_mods[0])r39::scat(k_mods,sizeof k_mods,", ");r39::scat(k_mods,sizeof k_mods,k_file.mod);}
 return kms::apply(game_img(),e,ne,k_file,k_write);
}
static bool k_mapexpand_loaded(){HMODULE v=GetModuleHandleW(L"version.dll");if(!v)return false;wchar_t mp[1024];if(!GetModuleFileNameW(v,mp,1024))return false;wchar_t rt[1024];if(!r39::game_root(rt,1024))return false;return k_prefix_ci(mp,rt);}
static void k_prep(int which){   // 0 포로 회유(captive_list) 1 등용문(recruit_gate)
 if(InterlockedExchange(&k_lock,1))return;
 static kms::Entry e[64];unsigned ne=which==0?k_entries(k_cap,3,e,64):k_entries(k_rec,2,e,64);
 k_mods[0]=0;unsigned changed=0;wchar_t dir[1024];
 if(r39::game_root(dir,1024)){wchar_t pat[1100];k_cat(pat,1100,dir,L"\\SAN14Mods\\sites\\*.txt");WIN32_FIND_DATAW fd;HANDLE f=FindFirstFileW(pat,&fd);
  if(f!=INVALID_HANDLE_VALUE){int cnt=0;do{if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&cnt++<16){wchar_t fp[1400];k_cat(fp,1400,dir,L"\\SAN14Mods\\sites\\",fd.cFileName);changed+=k_one(fp,e,ne);}}while(FindNextFileW(f,&fd));FindClose(f);}}
 long c=InterlockedIncrement(&k_clicks);if(changed){long a=InterlockedIncrement(&k_applied);if(a<=5)r39::counter(2,MOD,which?"k_recruit_sites_matched":"k_captive_sites_matched",changed);}
 unsigned ei=0;uint32_t off=0;
 if(kms::first_diff(game_img(),e,ne,&ei,&off)&&!k_shown[which]){
  k_shown[which]=true;InterlockedIncrement(&k_diff);
  U32 at_rva=e[ei].rva+off;size_t n=e[ei].len-off;if(n>16)n=16;
  const U8*ex=e[ei].exp+off;const U8*ac=game_img()+at_rva;
  char where[200]="IC ";r39::scat(where,sizeof where,which?"recruit-gate (등용문)":"captive persuasion (포로 회유)");r39::scat(where,sizeof where," verify table @ exe+0x");r39::shex(where,sizeof where,e[ei].rva);
  r39::scat(where,sizeof where,"+");r39::snum(where,sizeof where,off);r39::scat(where,sizeof where," = exe+0x");r39::shex(where,sizeof where,at_rva);
  r39::scat(where,sizeof where,k_mods[0]?" (site lists: ":" (site lists: none");if(k_mods[0]){r39::scat(where,sizeof where,k_mods);}r39::scat(where,sizeof where,")");
  char exs[64],acs[64];exs[0]=acs[0]=0;r39::sbytes(exs,sizeof exs,ex,n);r39::sbytes(acs,sizeof acs,ac,n);
  r43c::Owner own=r39::site_owner(at_rva,ex,ac,n);
  const wchar_t*eff=which?L"통합편의 [등용문] 버튼이 열리지 않음(게임 상태는 안 바뀜). 통합편의가 부르는 게임 함수를 다른 모드가 고쳐 둠. 다른 기능은 그대로."
                         :L"통합편의 [포로 회유] 버튼이 열리지 않음(게임 상태는 안 바뀜). 통합편의가 부르는 게임 함수를 다른 모드가 고쳐 둠. 다른 기능은 그대로.";
  wchar_t effb[700];effb[0]=0;r39::wcat(effb,700,eff);
  if(!k_mods[0]&&k_mapexpand_loaded())r39::wcat(effb,700,L"\r\n맵 확장(version.dll)이 켜져 있음: 맵 확장 137 이상은 '바꾼 자리 목록'(SAN14Mods\\sites\\MapExpand.txt)을 써서 이 버튼이 열림. 맵 확장을 137 이상으로 올려 주세요.");
  else if(k_mods[0])r39::wcat(effb,700,L"\r\n자리 목록을 준 모드가 있지만, 이 자리는 그 목록에 없음 — 목록에 빠진 자리이거나 또 다른 모드가 고친 자리.");
  r39::fail_ex(which?7:7,MOD,which?"verify_recruit":"verify_captive",where,exs,acs,effb,&own);
 }
 (void)c;InterlockedExchange(&k_lock,0);
}
template<int K> static U64 w_dlg(U64 a1,U64 a2,U64 a3,U64 a4){
 Dlg&d=dlg[K];U32 f0=rdflag(d);
 if(K==2)k_prep(0);else if(K==3)k_prep(1);
 U64 r=d.orig(a1,a2,a3,a4);
 if(!f0&&rdflag(d)&&!at<volatile U64>(ic,d.ptr)){
  clrflag(d);d.fixed++;
  if(d.fixed<=3){char m[160]="ICGuard R40: window open failed (";r39::scat(m,sizeof m,d.name);r39::scat(m,sizeof m,"); busy flag released so the button keeps working.");logf(m);}
  char c[48]="released_";r39::scat(c,sizeof c,d.name);r39::counter(7,MOD,c,d.fixed);
 }
 return r;
}
static bool install_b(){
 ModuleTransaction tx;void*w[5]={(void*)w_dlg<0>,(void*)w_dlg<1>,(void*)w_dlg<2>,(void*)w_dlg<3>,(void*)w_dlg<4>};
 for(int k=0;k<5;k++)dlg[k].orig=(Fn2)tx.detour(ic+dlg[k].fn,dlg[k].e,dlg[k].n,w[k]);
 static const U8 je_fail[]={0x74,0x55},je_next[]={0x74,0x41}; // 0x18002bb93: je 0x18002bbea(실패) → je 0x18002bbd6(다음)
 tx.bytes(ic+0x2bb93,je_fail,je_next,2);
 return fin(tx,1);
}
// ---------- 묶음 E (3번 문전박대, A1): 군주가 관문·항구에 있으면 '군주 도시' 판정(0x1800399b0)이 실패해 문전박대가 통째로 안 됨 ----------
//  A2(R39) HOMEFIX 와 같은 규칙: 원래 판정이 실패하면 세력 본거지 도시(force+0x160)로 대체. 도시가 아니거나 유효하지 않으면 원래대로 실패.
static Fn2 o_rulercity;static U8*game;static unsigned homefix_used;
static U64 w_rulercity(U64 ctx,U64 force,U64 out,U64 a4){
 U64 r=o_rulercity(ctx,force,out,a4);U8*f=(U8*)force;U8*c=(U8*)ctx;
 if((U8)r){return r;}
 if(!readable(f,0x168)||!readable(c,0x10)||!readable((void*)out,8)){return r;}
 if(at<U8*>(f,0)!=game+0x13a9880){return r;}
 U8*db=at<U8*>(c,8);if(!readable(db,8)){return r;}
 // 시험17(17:01 기록): 관문에 있으면 원래 판정이 CGateData(게임+0x13a9c30)를 돌려주고 실패. 세력+0x160 은 0(두 세력 모두) → 본거지 대체도 실패했음.
 //  게임 자신의 '거점 → 소속 도시'(0x22c1f0: 거점 지역 +0x12 → 지역 +0x35 도시; 통합편의 포로 거점 찾기도 관문에 같은 함수 사용)로 관문이 속한 도시를 씀.
 U8*city=nullptr;U8*got=*(U8**)out;
 if(readable(got,0x20)&&at<U8*>(got,0)==game+0x13a9c30)city=fn<U8*(*)(void*)>(game,0x22c1f0)(got);
 bool ok=readable(city,0x80)&&at<U8*>(city,0)==game+0x13a96d0&&fn<int(*)(void*)>(game,0x331780)(city);
 if(!ok){U8 home=at<U8>(f,0x160);if(home){city=fn<U8*(*)(void*,unsigned)>(game,0x32e3b0)(db,home);ok=readable(city,0x80)&&at<U8*>(city,0)==game+0x13a96d0&&fn<int(*)(void*)>(game,0x331780)(city);}}
 if(!ok){return r;}
 *(U8**)out=city;if(++homefix_used<=3)logf("ICGuard R43 E: ruler is at a gate/port; envoy refusal uses the city that owns it.");
 return 1;
}
static bool install_e(){
 ModuleTransaction tx;static const U8 e[]={0x40,0x53,0x55,0x56,0x57};
 o_rulercity=(Fn2)tx.detour(ic+0x399b0,e,5,(void*)w_rulercity);return fin(tx,2);
}
// ---------- 묶음 F (26번 처단불가 + 7번 포로 일괄 처우, A4): 통합편의가 게임 처단 함수(exe+0x2fc7f0)를 직접 불러 처단불가를 우회 ----------
//  IC 0x180023f04: 무장 +0x15e 비트 9/12(게임의 처단불가 표시)가 없으면 처단. 처단불가(Mode=1)가 켜져 있으면 '표시 있음'과 같은 길로 보냄.
//  켜짐 판단: SAN14_NoExecute.dll 이 올라와 있고 ExtBridge.ini [Ext] NoExecute≠0, SAN14_NoExecute.ini Enabled=1·Mode=1 (2초마다 다시 읽음 → 설정 창에서 바로 반영).
static volatile U8 noexec_on;static bool f_done;static wchar_t root_dir[1024];
static U8*near_block(U8*target){
 UPTR start=(UPTR)target&~UPTR(0xffff);
 for(UPTR off=0x10000;off<0x40000000;off+=0x10000){
  U8*b=(U8*)VirtualAlloc((void*)(start+off),0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(b)return b;
  if(start>off){b=(U8*)VirtualAlloc((void*)(start-off),0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(b)return b;}
 }
 return nullptr;
}
static void refresh_noexec(){
 if(!GetModuleHandleW(L"SAN14_NoExecute.dll")){noexec_on=0;return;}
 wchar_t a[1100],b[1100];r39::wlen(root_dir);a[0]=b[0]=0;r39::wcat(a,1100,root_dir);r39::wcat(a,1100,L"SAN14Mods\\modules\\ExtBridge.ini");
 r39::wcat(b,1100,root_dir);r39::wcat(b,1100,L"SAN14Mods\\Ext\\SAN14_NoExecute.ini");
 bool on=GetPrivateProfileIntW(L"Ext",L"NoExecute",1,a)!=0&&GetPrivateProfileIntW(L"NoExecute",L"Enabled",0,b)==1&&GetPrivateProfileIntW(L"NoExecute",L"Mode",0,b)==1;
 noexec_on=on?1:0;
}
static bool install_f(){
 U8*site=ic+0x23f04;U8*stub=near_block(site);if(!stub){static const U8 b6[]={0x0f,0x84,0xc6,0x00,0x00,0x00};pre_bad(3,site,b6,6,"near block alloc failed");return false;}
 U8 code[64];int k=0;U64 flag=(U64)&noexec_on,exec=(U64)(ic+0x23fd0),skip=(U64)(ic+0x23f0a);
 code[k++]=0x50;code[k++]=0x48;code[k++]=0xb8;memcpy(code+k,&flag,8);k+=8;code[k++]=0x80;code[k++]=0x38;code[k++]=0x00;code[k++]=0x58;code[k++]=0x75;code[k++]=0x0e;
 code[k++]=0xff;code[k++]=0x25;memset(code+k,0,4);k+=4;memcpy(code+k,&exec,8);k+=8;
 code[k++]=0xff;code[k++]=0x25;memset(code+k,0,4);k+=4;memcpy(code+k,&skip,8);k+=8;
 memcpy(stub,code,k);FlushInstructionCache(GetCurrentProcess(),stub,k);
 static const U8 before[]={0x0f,0x84,0xc6,0x00,0x00,0x00};
 intptr_t rel=stub-(site+6);if(rel<INT32_MIN||rel>INT32_MAX){VirtualFree(stub,0,MEM_RELEASE);pre_bad(3,site,before,6,"near block too far");return false;}
 U8 after[6]={0x0f,0x84};I32 r32=(I32)rel;memcpy(after+2,&r32,4);
 ModuleTransaction tx;tx.bytes(site,before,after,6);bool ok=fin(tx,3);if(!ok)VirtualFree(stub,0,MEM_RELEASE);return ok;
}
// ---------- 묶음 C (22번 DomesticCE, A14) / D (26번 AISearch, A17): 남의 DLL 파일은 그대로, 메모리에서만 2바이트씩 ----------
//  C: 3초 타이머(0x1800013a0)가 지역 338곳 값 검사에서 한 곳이라도 범위 밖이면 그 틱 전체(내정 자동·AI 명령서 99·최소 인구)를 건너뜀.
//     → 값 범위 밖인 지역이 있어도 계속(읽기 실패=게임 상태 아님 은 그대로 중단). 둘째 고리는 지역마다 따로 읽고 따로 처리함.
//  D: 행동트리가 아직 안 읽혔을 때 300초가 지나면 감시 스레드가 영구 종료 → 계속 기다림(0.5초마다 포인터 하나 읽는 가벼운 대기).
static bool c_done,d_done;
static bool patch_module(const wchar_t*name,U32 rva1,const U8*b1,const U8*a1,U32 n1,U32 rva2,const U8*b2,const U8*a2,U32 n2){
 U8*m=(U8*)GetModuleHandleW(name);if(!m)return false;ModuleTransaction tx;tx.bytes(m+rva1,b1,a1,n1);if(n2)tx.bytes(m+rva2,b2,a2,n2);return fin(tx,5);
}
// R42 22번 C 보강(A14 나머지): R40 은 첫 고리(값 검사)만 '끝까지 돌기'로 바꿔서, 범위 밖 지역도 둘째 고리(중점 개발 칸 +0x72 쓰기·인구 하한 +0x52 쓰기)에서
//  그대로 처리됐음. 또 DomesticCE 의 대상 주소(P = [[game+0x27d4140]+0x1e0] 의 첫 qword, 지역 i = P-0x51510+i*0x98)가 진짜 지역 표인지 확인하던
//  유일한 장치(338곳 모두 범위 안)를 없앤 셈이라, 대상이 엉뚱한 메모리여도 쓰기가 일어날 수 있었음.
//  고침: (가) 둘째 고리 앞(0x180001617)에서 대상 확인 — 지역 338곳 주소가 게임 지역 표(db+0x6caf0, CAreaData 포인터 501칸)의 연속 항목과 정확히 같은지.
//         같으면 '지역별 모드': 범위 밖 지역은 그 지역만 건너뜀. 다르면 '원래 모드': DomesticCE 원래 규칙(하나라도 범위 밖·읽기 실패면 그 틱 전체 건너뜀).
//        (나) 둘째 고리 머리(0x18000163b)에서 지역별 모드면 그 지역 값(+0x3c/+0x3e/+0x40 개발 ≤1000, +0x72 중점 <4)을 보고 밖이면 다음 지역(0x180001627)으로.
//  지역 구조(게임 코드 근거): +0x3c/+0x3e/+0x40 개발 3종(0x22e2b0/0x22c760/0x231170 등급 계산), +0x52 인구(DomesticCE 하한 1만), +0x72 중점 개발(1 농·2 상·3 병, 0x0b1aa0),
//   +0x78 BYTE = 세력 번호(0x2b08d1: 세력 관계표 +0xea 인덱스로 씀), +0x79 = 생성자(0x220740)가 안 채우는 빈칸. DomesticCE 는 +0x78 을 WORD 로 읽어 <256 을 봄 = 사실상 빈칸 +0x79==0 검사.
//   → 지역별 모드에선 +0x78/+0x79 는 보지 않음(처리에 안 쓰는 칸이고 빈칸 쓰레기값으로 멀쩡한 지역이 빠지던 원인 후보).
//  AI 명령서: 셋째 고리 P+0x4858 부터 0x30 간격 50칸(+0 세력, +4 명령서). 세력 표는 52칸(db+0xdf30, 0..0x33)인데 이 표가 52칸인지는 정적 근거 없음
//   → 바꾸지 않고, 처음 한 번 46~55칸 값과 세력 수를 기록(R42D 22 orders ...)해 실게임 기록으로 확정.
static U8*dce;static volatile LONG dce_mode;static int dce_j0=-1;static unsigned dce_ticks,dce_skipped,dce_strict_skip,dce_logged;
extern "C"{void*DcePreTramp;void*DceAbort;void*DceRegTramp;void*DceNext;volatile int DceResult,DceRegResult;void DcePreStub();void DceRegStub();}
static bool rpm(const void*src,void*dst,size_t n){if(!src||!core||!core->Readable((void*)src,n))return false;memcpy(dst,src,n);return true;} // ModCore 읽기 가능 검사 뒤 복사
static bool dce_in_range_full(const U8*r){ // DomesticCE 원래 첫 고리 규칙(지역 구조 기준 오프셋)
 return at<U16>((void*)r,0x3c)<=1000&&at<U16>((void*)r,0x3e)<=1000&&at<U16>((void*)r,0x40)<=1000&&r[0x72]<4&&at<U16>((void*)r,0x78)<256;
}
static bool dce_in_range_used(const U8*r){return at<U16>((void*)r,0x3c)<=1000&&at<U16>((void*)r,0x3e)<=1000&&at<U16>((void*)r,0x40)<=1000&&r[0x72]<4;}
static void dce_log_orders(U8*P,U8*d){
 if(!r39::s14_diag())return;
 char m[400]="R42D\t22\tSAN14_ICGuard.dll\torders";U8 e[0x30];
 for(int k=46;k<56;k++){r39::scat(m,sizeof m,"\t");r39::snum(m,sizeof m,(unsigned)k);r39::scat(m,sizeof m,":");
  if(rpm(P+0x4858+(size_t)k*0x30,e,0x30)){r39::snum(m,sizeof m,e[0]);r39::scat(m,sizeof m,"/");r39::snum(m,sizeof m,at<U16>(e,4));}else r39::scat(m,sizeof m,"x");}
 U8*fp[0x34];unsigned nf=0,maxid=0;if(rpm(d+0xdf30,fp,sizeof fp))for(unsigned k=1;k<0x34;k++)if(fp[k]&&fp[k]!=fp[0]){nf++;maxid=k;}
 r39::scat(m,sizeof m,"\tforces=");r39::snum(m,sizeof m,nf);r39::scat(m,sizeof m,"\tmaxforce=");r39::snum(m,sizeof m,maxid);
 r39::scat(m,sizeof m,"\tj0=");r39::snum(m,sizeof m,(unsigned)dce_j0);
 logf(m);
}
// 0 = 이 틱 건너뜀(원래 0x180001bcc), 1 = 진행
extern "C" int DcePrescanC(U8*P){
 dce_ticks++;U8*base=P-0x51510;U8*d=game?*(U8**)(game+0x2780f60):nullptr;static U8*tab[501];bool ident=false;
 if(d&&rpm(d+0x6caf0,tab,sizeof tab)){
  int j0=-1;for(int k=0;k<501;k++)if(tab[k]==base){j0=k;break;}
  if(j0>=0&&j0+338<=501){ident=true;for(int i=0;i<338;i++)if(tab[j0+i]!=base+(size_t)i*0x98){ident=false;break;}if(ident)dce_j0=j0;}
 }
 if(ident){
  LONG was=InterlockedExchange(&dce_mode,1);
  if(was!=1&&dce_logged<6){dce_logged++;logf("ICGuard R42 C: DomesticCE target = game area table (338 areas verified); an out-of-range area is skipped alone.");}
  static bool orders_done;if(!orders_done){orders_done=true;dce_log_orders(P,d);}
  return 1;
 }
 LONG was=InterlockedExchange(&dce_mode,2);
 if(was!=2&&dce_logged<6){dce_logged++;logf("ICGuard R42 C: DomesticCE target not matched to the game area table; original all-or-nothing range check kept.");}
 U8 r[0x98];
 for(int i=0;i<338;i++){if(!rpm(base+(size_t)i*0x98,r,sizeof r)||!dce_in_range_full(r)){dce_strict_skip++;if(dce_strict_skip<=3)logf("ICGuard R42 C: (original mode) a region is out of range -> this 3 s tick skipped.");return 0;}}
 return 1;
}
// 0 = 이 지역 건너뜀(0x180001627), 1 = 처리. rsi = 지역 i 의 +0x3c 칸 - P
extern "C" int DceRegionC(U8*P,long long rsi){
 if(dce_mode!=1)return 1;
 U8*r=P+rsi-0x3c;U8 b[0x98];
 if(!rpm(r,b,sizeof b)){dce_skipped++;return 0;}
 if(dce_in_range_used(b))return 1;
 dce_skipped++;
 if(dce_skipped<=5){
  char m[200]="ICGuard R42 C: area #";unsigned idx=(unsigned)(dce_j0+(int)((rsi+0x514d4)/0x98));r39::snum(m,sizeof m,idx);
  r39::scat(m,sizeof m," out of range (dev ");r39::snum(m,sizeof m,at<U16>(b,0x3c));r39::scat(m,sizeof m,"/");r39::snum(m,sizeof m,at<U16>(b,0x3e));r39::scat(m,sizeof m,"/");
  r39::snum(m,sizeof m,at<U16>(b,0x40));r39::scat(m,sizeof m,", focus ");r39::snum(m,sizeof m,b[0x72]);r39::scat(m,sizeof m,") -> skipped only this area; others processed.");logf(m);
 }
 if(dce_skipped==1||dce_skipped%50==0)r39::counter(22,MOD,"domestic_area_skipped",dce_skipped);
 return 0;
}
// 둘 다 DomesticCE 3초 타이머 함수 몸통(rbp = 그 함수 틀, [rbp+0x68] = P)에서 jmp 로 들어옴.
asm(R"(
.text
.globl DcePreStub
DcePreStub:
 pushfq
 push %rax
 push %rcx
 push %rdx
 push %r8
 push %r9
 push %r10
 push %r11
 sub $0x80,%rsp
 movdqu %xmm0,0x20(%rsp)
 movdqu %xmm1,0x30(%rsp)
 movdqu %xmm2,0x40(%rsp)
 movdqu %xmm3,0x50(%rsp)
 movdqu %xmm4,0x60(%rsp)
 movdqu %xmm5,0x70(%rsp)
 mov 0x68(%rbp),%rcx
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call DcePrescanC
 mov %rbp,%rsp
 pop %rbp
 mov %eax,DceResult(%rip)
 movdqu 0x20(%rsp),%xmm0
 movdqu 0x30(%rsp),%xmm1
 movdqu 0x40(%rsp),%xmm2
 movdqu 0x50(%rsp),%xmm3
 movdqu 0x60(%rsp),%xmm4
 movdqu 0x70(%rsp),%xmm5
 add $0x80,%rsp
 pop %r11
 pop %r10
 pop %r9
 pop %r8
 pop %rdx
 pop %rcx
 pop %rax
 popfq
 cmpl $0,DceResult(%rip)
 je 1f
 jmp *DcePreTramp(%rip)
1:
 jmp *DceAbort(%rip)
.globl DceRegStub
DceRegStub:
 pushfq
 push %rax
 push %rcx
 push %rdx
 push %r8
 push %r9
 push %r10
 push %r11
 sub $0x80,%rsp
 movdqu %xmm0,0x20(%rsp)
 movdqu %xmm1,0x30(%rsp)
 movdqu %xmm2,0x40(%rsp)
 movdqu %xmm3,0x50(%rsp)
 movdqu %xmm4,0x60(%rsp)
 movdqu %xmm5,0x70(%rsp)
 mov 0x68(%rbp),%rcx
 mov %rsi,%rdx
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call DceRegionC
 mov %rbp,%rsp
 pop %rbp
 mov %eax,DceRegResult(%rip)
 movdqu 0x20(%rsp),%xmm0
 movdqu 0x30(%rsp),%xmm1
 movdqu 0x40(%rsp),%xmm2
 movdqu 0x50(%rsp),%xmm3
 movdqu 0x60(%rsp),%xmm4
 movdqu 0x70(%rsp),%xmm5
 add $0x80,%rsp
 pop %r11
 pop %r10
 pop %r9
 pop %r8
 pop %rdx
 pop %rcx
 pop %rax
 popfq
 cmpl $0,DceRegResult(%rip)
 je 2f
 jmp *DceRegTramp(%rip)
2:
 jmp *DceNext(%rip)
)");
static bool install_c(U8*m){
 ModuleTransaction tx;
 static const U8 b1[]={0x3c,0x01,0x75,0x17},a1[]={0x3c,0x01,0x90,0x90},b2[]={0x84,0xc0,0x0f,0x84,0xb5,0x05,0x00,0x00},a2[]={0x84,0xc0,0x66,0x0f,0x1f,0x44,0x00,0x00};
 static const U8 e3[]={0xc7,0x45,0x74,0x00,0x00,0x00,0x00},e4[]={0x48,0x8b,0x5d,0x68,0x4c,0x8d,0x2c,0x1e};
 // 둘째 고리 앞뒤 지문: 0x180001625 jmp 0x18000163b / 0x180001627 add rsi,0x98(다음 지역) / 0x180001bcc 틱 끝(건너뜀 길)
 static const U8 f1[]={0xeb,0x14,0x48,0x81,0xc6,0x98,0x00,0x00,0x00},f2[]={0x48,0x8b,0x5d,0x68};
 static const U8 f3[]={0x31,0xc0,0x87,0x05};
 if(memcmp(m+0x1625,f1,9)){pre_bad(4,m+0x1625,f1,9,"bytes differ");return false;}
 if(memcmp(m+0x163b,f2,4)){pre_bad(4,m+0x163b,f2,4,"bytes differ");return false;}
 if(memcmp(m+0x1bcc,f3,4)){pre_bad(4,m+0x1bcc,f3,4,"bytes differ");return false;}
 tx.bytes(m+0x15f4,b1,a1,4);tx.bytes(m+0x160f,b2,a2,8);
 void*t3=tx.detour(m+0x1617,e3,7,(void*)DcePreStub);void*t4=tx.detour(m+0x163b,e4,8,(void*)DceRegStub);
 if(!t3||!t4){ic_bad[4]=tx.badinfo;return false;}
 DcePreTramp=t3;DceRegTramp=t4;DceAbort=m+0x1bcc;DceNext=m+0x1627;
 return fin(tx,4);
}
static void try_c(){
 if(c_done)return;U8*m=(U8*)GetModuleHandleW(L"SAN14_DomesticCE.dll");if(!m)return;c_done=true;dce=m;
 if(!game&&core)game=core->resolved->image;
 bool ok=install_c(m);
 logf(ok?"ICGuard R42 C: DomesticCE 3 s tick checks each area separately (out-of-range area skipped alone; target verified against the game area table).":"ICGuard R42 C: DomesticCE bytes differ (other build?); left unchanged.");
 r39::start(22,MOD,ok?"OK":"SKIP",ok?1:0,1,"-",ok?"-":"DomesticCE bytes differ");
 if(!ok)report(4,"DomesticCE 3s tick area loop (C)",m,"DomesticCE",22,L"22번 내정 자동(DomesticCE)의 지역별 안전 처리(ICGuard C)가 안 붙음 → DomesticCE 원래 규칙(범위 밖 지역 하나면 그 틱 전체 건너뜀). 다른 기능은 그대로");
}
static void try_d(){
 if(d_done||!GetModuleHandleW(L"SAN14_AISearch.dll"))return;d_done=true;
 static const U8 b1[]={0x7f,0x59,0xb9,0xf4},a1[]={0x90,0x90,0xb9,0xf4};
 bool ok=patch_module(L"SAN14_AISearch.dll",0x1e46,b1,a1,4,0,nullptr,nullptr,0);
 logf(ok?"ICGuard R40 D: AISearch keeps waiting for the behaviour tree instead of stopping after 300 s.":"ICGuard R40 D: AISearch bytes differ (other build?); left unchanged.");
 r39::start(26,MOD,ok?"OK":"SKIP",ok?1:0,1,"-",ok?"-":"AISearch bytes differ");
 if(!ok)report(5,"AISearch 300s watchdog je (D)",(U8*)GetModuleHandleW(L"SAN14_AISearch.dll"),"AISearch",26,L"26번 AISearch 300초 뒤 감시 종료 방지(ICGuard D)가 안 붙음 → 행동트리가 늦게 읽히면 26번이 꺼진 채로 갈 수 있음. 다른 기능은 그대로");
}

// ---------- R42d 묶음 G (10번 고유무장 등록 '신무장' 탭) ----------
//  IC 는 신무장 원본을 '게임 폴더\SteamData\editdataexceKO.s14' 에서만 찾음 → 스팀판은 이 파일이 Steam\userdata\<계정>\872410\remote 에 있어 늘 0명.
//  IC 가 이 파일을 열 때(CreateFileW, IC 가져오기 칸 하나만 바꿈 — IC 코드 바이트는 그대로)만 2D 모드 v25 와 같은 순서로 실제 파일을 찾아 연다:
//   ① ICGuard.ini [NewOfficer] File= 직접 지정 ② 원래 경로에 파일 있으면 그대로 ③ 같은 폴더 다른 언어판(editdataexce*.s14 최신)
//   ④ 스팀 폴더(게임이 깔린 라이브러리 → HKCU SteamPath → HKLM InstallPath) 의 userdata\<모든 계정>\872410\remote — 로그인 계정 우선, 그다음 최신
//  읽기 전용 열기만 바꿈(쓰기·다른 파일은 그대로 통과).
typedef HANDLE(WINAPI*CFW)(LPCWSTR,DWORD,DWORD,void*,DWORD,DWORD,HANDLE);
static CFW o_cfw;static volatile LONG no_calls,no_found,no_miss;static U32 g_state; // 0 아직 1 원래 경로 2 직접 지정 3 같은 폴더 4 스팀 5 못 찾음
static wchar_t lowc(wchar_t c){return (c>=L'A'&&c<=L'Z')?(wchar_t)(c+32):(c==L'/'?L'\\':c);}
static bool ends_ci(const wchar_t*s,const wchar_t*suf){size_t a=r39::wlen(s),b=r39::wlen(suf);if(a<b)return false;for(size_t i=0;i<b;i++)if(lowc(s[a-b+i])!=lowc(suf[i]))return false;return true;}
static int find_ci(const wchar_t*s,const wchar_t*k){size_t a=r39::wlen(s),b=r39::wlen(k);for(size_t i=0;i+b<=a;i++){size_t j=0;while(j<b&&lowc(s[i+j])==lowc(k[j]))j++;if(j==b)return (int)i;}return -1;}
static bool same_ci(const wchar_t*a,const wchar_t*b){size_t i=0;for(;a[i]&&b[i];i++)if(lowc(a[i])!=lowc(b[i]))return false;while(a[i]==L'\\')i++;size_t j=i;while(b[j]==L'\\')j++;return !a[i]&&!b[j];}
static U64 ft(const FILETIME&f){return ((U64)f.dwHighDateTime<<32)|f.dwLowDateTime;}
static bool finfo(const wchar_t*p,U64*mt){WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(p,&fd);if(h==INVALID_HANDLE_VALUE||!h)return false;FindClose(h);if(fd.dwFileAttributes&0x10)return false;if(!fd.nFileSizeLow&&!fd.nFileSizeHigh)return false;*mt=ft(fd.ftLastWriteTime);return true;}
static void wset(wchar_t*o,size_t cap,const wchar_t*a,const wchar_t*b=nullptr,const wchar_t*c=nullptr,const wchar_t*d=nullptr){o[0]=0;r39::wcat(o,cap,a);if(b)r39::wcat(o,cap,b);if(c)r39::wcat(o,cap,c);if(d)r39::wcat(o,cap,d);}
// dir\editdataexceKO.s14 → 없으면 dir\editdataexce*.s14 중 최신
static bool pick_in(const wchar_t*dir,wchar_t*out,U64*omt){
 wchar_t p[600];wset(p,600,dir,L"\\editdataexceKO.s14");if(finfo(p,omt)){wset(out,600,p);return true;}
 wset(p,600,dir,L"\\editdataexce*.s14");WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(p,&fd);if(h==INVALID_HANDLE_VALUE||!h)return false;
 bool got=false;U64 best=0;
 do{if(fd.dwFileAttributes&0x10)continue;if(!fd.nFileSizeLow&&!fd.nFileSizeHigh)continue;U64 m=ft(fd.ftLastWriteTime);if(got&&m<best)continue;best=m;wset(out,600,dir,L"\\",fd.cFileName);*omt=m;got=true;}while(FindNextFileW(h,&fd));
 FindClose(h);return got;
}
struct NoBest{wchar_t path[600];U64 mt;bool active;};
static void scan_root(const wchar_t*root,U32 active,bool any_app,NoBest&bb,U32*naccts){
 wchar_t pat[600];wset(pat,600,root,L"\\userdata\\*");WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE||!h)return;
 do{
  if(!(fd.dwFileAttributes&0x10)||fd.cFileName[0]==L'.')continue;
  U32 acct=0;for(int i=0;fd.cFileName[i]>=L'0'&&fd.cFileName[i]<=L'9';i++)acct=acct*10+(U32)(fd.cFileName[i]-L'0');
  bool isact=active&&acct==active;wchar_t dir[600],cand[600];U64 mt=0;
  if(!any_app){(*naccts)++;wset(dir,600,root,L"\\userdata\\",fd.cFileName,L"\\872410\\remote");
   if(pick_in(dir,cand,&mt)&&(!bb.path[0]||(isact&&!bb.active)||(isact==bb.active&&mt>bb.mt))){wset(bb.path,600,cand);bb.mt=mt;bb.active=isact;}}
  else{wchar_t p2[600];wset(p2,600,root,L"\\userdata\\",fd.cFileName,L"\\*");WIN32_FIND_DATAW f2;HANDLE h2=FindFirstFileW(p2,&f2);if(h2==INVALID_HANDLE_VALUE||!h2)continue;
   do{if(!(f2.dwFileAttributes&0x10)||f2.cFileName[0]==L'.')continue;wset(dir,600,root,L"\\userdata\\",fd.cFileName,L"\\");r39::wcat(dir,600,f2.cFileName);r39::wcat(dir,600,L"\\remote");
    if(pick_in(dir,cand,&mt)&&(!bb.path[0]||(isact&&!bb.active)||(isact==bb.active&&mt>bb.mt))){wset(bb.path,600,cand);bb.mt=mt;bb.active=isact;}}while(FindNextFileW(h2,&f2));
   FindClose(h2);}
 }while(FindNextFileW(h,&fd));
 FindClose(h);
}
typedef LONG(WINAPI*RGV)(void*,LPCWSTR,LPCWSTR,DWORD,DWORD*,void*,DWORD*);
static RGV rgv(){static RGV f;static bool tried;if(!tried){tried=true;HMODULE a=LoadLibraryExW(L"advapi32.dll",nullptr,0x800);if(a)f=(RGV)(void*)GetProcAddress(a,"RegGetValueW");}return f;}
static bool reg_sz(U64 hive,const wchar_t*key,const wchar_t*val,wchar_t*out,DWORD cap){
 RGV f=rgv();out[0]=0;if(!f)return false;DWORD sz=cap*2;if(f((void*)hive,key,val,2,nullptr,out,&sz)!=0){out[0]=0;return false;}
 for(int i=0;out[i];i++)if(out[i]==L'/')out[i]=L'\\';size_t n=r39::wlen(out);while(n>0&&out[n-1]==L'\\')out[--n]=0;return out[0]!=0;
}
static void w2u(char*o,size_t cap,const wchar_t*w){size_t n=r39::slen(o);for(;*w&&n+4<cap;w++){unsigned c=*w;if(c<0x80)o[n++]=(char)c;else if(c<0x800){o[n++]=(char)(0xc0|c>>6);o[n++]=(char)(0x80|(c&63));}else{o[n++]=(char)(0xe0|c>>12);o[n++]=(char)(0x80|((c>>6)&63));o[n++]=(char)(0x80|(c&63));}}o[n]=0;}
static void no_log(const char*what,const wchar_t*path,U32 roots,U32 accts){
 static LONG n;if(InterlockedIncrement(&n)>4)return;
 char l[1400];l[0]=0;r39::scat(l,sizeof l,"ICGuard R42 G: new-officer file ");r39::scat(l,sizeof l,what);r39::scat(l,sizeof l," = ");w2u(l,sizeof l,path);
 r39::scat(l,sizeof l," (steam roots ");r39::snum(l,sizeof l,roots);r39::scat(l,sizeof l,", accounts ");r39::snum(l,sizeof l,accts);r39::scat(l,sizeof l,")");logf(l);
}
// 찾은 경로를 out 에. 원래 경로가 있으면 false(그대로 열기)
static bool newoff_resolve(const wchar_t*gp,wchar_t*out){
 U64 mt=0;
 {wchar_t ini[1100],f[600];wset(ini,1100,root_dir,L"SAN14Mods\\modules\\ICGuard.ini");f[0]=0;GetPrivateProfileStringW(L"NewOfficer",L"File",L"",f,599,ini);
  size_t n=r39::wlen(f);while(n>0&&(f[n-1]==L' '||f[n-1]==L'"'))f[--n]=0;const wchar_t*q=f;if(*q==L'"')q++;
  if(*q&&finfo(q,&mt)){wset(out,600,q);g_state=2;no_log("(ini File=)",out,0,0);return true;}}
 if(finfo(gp,&mt)){g_state=1;return false;}
 wchar_t gdir[600];wset(gdir,600,gp);{int cut=-1;for(int i=0;gdir[i];i++)if(gdir[i]==L'\\'||gdir[i]==L'/')cut=i;if(cut>0){gdir[cut]=0;if(pick_in(gdir,out,&mt)){g_state=3;no_log("(same folder)",out,0,0);return true;}}}
 static wchar_t roots[4][600];U32 nr=0;
 {int k=find_ci(gp,L"\\steamapps\\");if(k<0)k=find_ci(gp,L"/steamapps/");if(k>0&&k<590){for(int i=0;i<k;i++)roots[nr][i]=gp[i];roots[nr][k]=0;nr++;}
  wchar_t t[600];
  for(int r=0;r<3&&nr<4;r++){
   bool got=r==0?reg_sz(0x80000001ull,L"Software\\Valve\\Steam",L"SteamPath",t,599):r==1?reg_sz(0x80000002ull,L"SOFTWARE\\WOW6432Node\\Valve\\Steam",L"InstallPath",t,599):reg_sz(0x80000002ull,L"SOFTWARE\\Valve\\Steam",L"InstallPath",t,599);
   if(!got)continue;bool dup=false;for(U32 q=0;q<nr;q++)if(same_ci(roots[q],t))dup=true;if(!dup){wset(roots[nr],600,t);nr++;}}}
 U32 active=0;{RGV f=rgv();if(f){DWORD v=0,sz=4;if(f((void*)0x80000001ull,L"Software\\Valve\\Steam\\ActiveProcess",L"ActiveUser",0x10,nullptr,&v,&sz)==0)active=v;}}
 static NoBest bb;memset(&bb,0,sizeof bb);U32 naccts=0;
 for(U32 q=0;q<nr;q++)scan_root(roots[q],active,false,bb,&naccts);
 if(!bb.path[0]){U32 d=0;for(U32 q=0;q<nr;q++)scan_root(roots[q],active,true,bb,&d);}
 if(!bb.path[0]){g_state=5;InterlockedIncrement(&no_miss);no_log("NOT FOUND, game path",gp,nr,naccts);return false;}
 wset(out,600,bb.path);g_state=4;no_log(bb.active?"(steam, signed-in account)":"(steam)",out,nr,naccts);return true;
}
static HANDLE WINAPI w_cfw(LPCWSTR name,DWORD acc,DWORD share,void*sa,DWORD disp,DWORD flags,HANDLE tmpl){
 if(name&&acc==0x80000000u&&disp==3&&ends_ci(name,L"\\SteamData\\editdataexceKO.s14")){
  InterlockedIncrement(&no_calls);wchar_t alt[600];
  if(newoff_resolve(name,alt)){HANDLE h=o_cfw(alt,acc,share|1,sa,disp,flags,tmpl);if(h!=INVALID_HANDLE_VALUE){InterlockedIncrement(&no_found);return h;}}
 }
 return o_cfw(name,acc,share,sa,disp,flags,tmpl);
}
// 신무장 파일 해석(IC 0x18002e410)이 칸마다 [+2]u16==4 를 요구 → 실제 게임 저장(예: 0x4003)은 하나라도 다르면 전체 실패 → 2D 모드 v25 와 같은 2곳 수정:
//  0x18002e4f1 jne 실패 → nop6 (+2 값 검사 안 함) / 0x18002e4d9 번호 불일치 jne 실패 → jne 빈 칸 건너뛰기(0x18002e9f6, 그 칸만 빼고 계속)
static bool g_parse;
static bool install_g_parse(){
 static const U8 b1[]={0x0f,0x85,0x97,0x05,0x00,0x00},a1[]={0x66,0x0f,0x1f,0x44,0x00,0x00};
 static const U8 b2[]={0x0f,0x85,0xaf,0x05,0x00,0x00},a2[]={0x0f,0x85,0x17,0x05,0x00,0x00};
 if(!readable(ic+0x2e4f1,6))return false;
 if(!memcmp(ic+0x2e4f1,a1,6)&&!memcmp(ic+0x2e4d9,a2,6))return true; // 이미 됨
 return patch_module(L"SAN14_IntegratedConvenience.dll",0x2e4f1,b1,a1,6,0x2e4d9,b2,a2,6);
}
static bool install_g(){
 g_parse=install_g_parse();
 // IC 가져오기 칸(CreateFileW) +0x9a060, 신무장 파일 이름 글자 +0xe91b8 가 알려진 판과 같을 때만
 static const wchar_t nm[]=L"editdataexceKO.s14";if(!readable(ic+0xe91b8,sizeof nm)||memcmp(ic+0xe91b8,nm,sizeof nm)){pre_bad(6,ic+0xe91b8,(const U8*)nm,16,"name string differs");return false;}
 void*real=(void*)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"CreateFileW");void**slot=(void**)(ic+0x9a060);
 if(!real||!readable(slot,8)||*slot!=real){U8 ex[8];memcpy(ex,&real,8);pre_bad(6,(U8*)slot,ex,8,"import slot points elsewhere (another mod hooked CreateFileW?)");return false;}
 o_cfw=(CFW)real;DWORD old;if(!VirtualProtect(slot,8,PAGE_READWRITE,&old))return false;
 __atomic_store_n(slot,(void*)w_cfw,__ATOMIC_SEQ_CST);VirtualProtect(slot,8,old,&old);return true;
}
// ---------- R42e 묶음 H (1번 IC 길찾기 보호, 카페 제보 11-02 이그마·아나키: 출진·수송 부대 병참 단절·혼란/저지, 제자리에서 사기 0) ----------
//  IC 0x180001806: 길찾기 한 칸 비용이 16384(0x4000) 넘으면 길찾기 전체를 실패시킴(0x180001818 실패 길) → 원래 게임에선 지나가는 비싼 칸에서 '길 없음'.
//  고침: 0x4000 이하는 원래대로 / 0x4000~0x10000000 은 원래 게임처럼 통과(횟수만 셈, old_rule_hits) / 0x10000000 넘는 망가진 값만 원래 실패 길(blocked).
//  원래 13바이트(mov ecx,eax; mov eax,0x4000; cmp ecx,eax; ja +7; mov eax,ecx) → jmp 동굴 + nop8. 뒤 7바이트(add rsp,48; ret; mov edx,..)가 알려진 판일 때만.
//  5분마다 횟수가 바뀌었으면 적용확인.log 에 R42H 줄(old_rule_hits=·blocked=).
static volatile LONG h_old_hits,h_blocks;static bool h_done;
// 동굴 코드(0x42바이트): 시험에서 바이트를 그대로 비교할 수 있게 따로 만듦
static U32 h_cave(U8*c,U64 old_hits,U64 blocks,U64 back){
 static const U8 head[]={0x3d,0x00,0x40,0x00,0x00, 0x76,0x14, 0x48,0xb9};          // cmp eax,0x4000 ; jbe 원래 반환 ; mov rcx,&old_hits
 static const U8 mid[]={0xf0,0xff,0x01, 0x3d,0x00,0x00,0x00,0x10, 0x77,0x05,      // lock inc [rcx] ; cmp eax,0x10000000 ; ja 막기
                        0x48,0x83,0xc4,0x48,0xc3, 0x48,0xb9};                      // add rsp,0x48 ; ret (원래 반환) ; mov rcx,&blocks
 static const U8 tail[]={0xf0,0xff,0x01, 0x8b,0xc8,0xb8,0x00,0x40,0x00,0x00, 0xff,0x25,0x00,0x00,0x00,0x00}; // lock inc ; 원래 값 맞춤 ; jmp [rip] 실패 길
 U32 k=0;memcpy(c+k,head,sizeof head);k+=sizeof head;memcpy(c+k,&old_hits,8);k+=8;memcpy(c+k,mid,sizeof mid);k+=sizeof mid;
 memcpy(c+k,&blocks,8);k+=8;memcpy(c+k,tail,sizeof tail);k+=sizeof tail;memcpy(c+k,&back,8);k+=8;return k;
}
static bool install_h(){
 U8*site=ic+0x1806;if(!readable(site,0x14)){pre_bad(7,site,(const U8*)"\x8b\xc8\xb8\x00\x40\x00\x00\x3b\xc8\x77\x07\x8b\xc1",13,"unreadable");return false;}
 static const U8 tail[]={0x48,0x83,0xc4,0x48,0xc3,0x8b,0x15};if(memcmp(ic+0x1813,tail,7)){pre_bad(7,ic+0x1813,tail,7,"bytes differ");return false;}
 static const U8 before[]={0x8b,0xc8,0xb8,0x00,0x40,0x00,0x00,0x3b,0xc8,0x77,0x07,0x8b,0xc1};
 U8*stub=near_block(site);if(!stub){pre_bad(7,site,before,13,"near block alloc failed");return false;}
 U8 code[0x42];U32 n=h_cave(code,(U64)&h_old_hits,(U64)&h_blocks,(U64)(ic+0x1818));
 memcpy(stub,code,n);FlushInstructionCache(GetCurrentProcess(),stub,n);
 intptr_t rel=stub-(site+5);if(rel<INT32_MIN||rel>INT32_MAX){VirtualFree(stub,0,MEM_RELEASE);pre_bad(7,site,before,13,"near block too far");return false;}
 U8 after[13];after[0]=0xe9;I32 r32=(I32)rel;memcpy(after+1,&r32,4);memset(after+5,0x90,8);
 ModuleTransaction tx;tx.bytes(site,before,after,13);bool ok=fin(tx,7);if(!ok)VirtualFree(stub,0,MEM_RELEASE);return ok;
}
static void h_report(){
 static U32 last_old=0xffffffffu,last_blk=0xffffffffu;U32 o=(U32)h_old_hits,k=(U32)h_blocks;if(o==last_old&&k==last_blk)return;last_old=o;last_blk=k;
 char l[200]="R42H\t1\tSAN14_ICGuard.dll\tpath\told_rule_hits=";r39::snum(l,sizeof l,o);r39::scat(l,sizeof l,"\tblocked=");r39::snum(l,sizeof l,k);r39::scat(l,sizeof l,"\t");
 r39::stamp(l,sizeof l);r39::append_line(l);
}
// ---------- R43 11-10 묶음 I (2번 턴 종료 명승 자동 투자 — P205, 교훈 L096) ----------
// 통합편의의 투자 한 건(IC+0x40480): 비용(게임 exe+0x22c5f0) < 1 · 기간(게임 exe+0x22c810)이 0 · 날짜 이상이면 -1 을 돌려준다(IC+0x4067c).
// 턴 끝 고리(IC+0x6933)는 -1 을 보면 '구조/비용 확인 실패'로 치고 남은 명승을 전부 건너뛴다.
// 게임은 '지금은 발전시킬 수 없는 명승'에 비용·기간 0 을 돌려준다 → 그런 명승이 하나라도 후보 앞쪽에 있으면 그 턴 투자가 통째로 안 된다.
//  사용자 기록: 10월 자동인사 229턴 중 실패 1·투자 21턴 / 새 판 88턴 중 실패 15·투자 0턴. 통합편의 코드·게임 함수는 옛 판과 같음(대조함) → 세이브 상태에 따라 나는 통합편의 버그.
// 고침: 값이 범위 밖인 경우(위 3가지)의 -1 을 0 으로(그 명승만 건너뛰고 다음 명승으로). 실제로 쓴 뒤 확인이 어긋난 경우의 -1(IC+0x40673)은 그대로 둔다.
// 자가진단: 건너뛴 명승의 단계·비용·기간을 실행당 8줄까지 적용확인.log 에 남김(R43I). 읽기만 함.
static int(*o_invest)(U8*,U8*);static volatile LONG inv_ok,inv_skip,inv_fail;static ModuleTransaction::Bad i_bad;static bool i_done;
static int w_invest(U8*ctx,U8*cand){
 int r=o_invest(ctx,cand);
 if(r==1){long k=InterlockedIncrement(&inv_ok);if(k==1||k%20==0)r39::counter(2,MOD,"invest_done",(unsigned)k);}
 else if(r<0)InterlockedIncrement(&inv_fail);
 else{long k=InterlockedIncrement(&inv_skip);
  if(k<=8&&cand&&core->Readable(cand,0x18)){U8*sc=at<U8*>(cand,0);
   if(sc&&core->Readable(sc,0x70)){
    int cost=reinterpret_cast<int(*)(U8*)>(game+0x22c5f0)(sc),days=reinterpret_cast<int(*)(U8*)>(game+0x22c810)(sc);
    char l[220]="R43I\t2\tSAN14_ICGuard.dll\tinvest_skip\tidx=";r39::snum(l,sizeof l,(unsigned)at<int>(cand,0x10));
    r39::scat(l,sizeof l,"\tlevel=");r39::snum(l,sizeof l,at<U8>(sc,0x6c));
    r39::scat(l,sizeof l,"\tcost=");r39::snum(l,sizeof l,(unsigned)(cost<0?0:cost));r39::scat(l,sizeof l,"\tdays=");r39::snum(l,sizeof l,(unsigned)(days<0?0:days));
    r39::scat(l,sizeof l,cost<1||days<1||days>0xffff?"\twhy=not-developable-now\t":"\twhy=gold-short-or-changed\t");r39::stamp(l,sizeof l);r39::append_line(l);}}}
 return r;
}
static bool install_i(){
 ModuleTransaction tx;
 static const U8 before[]={0xb8,0xff,0xff,0xff,0xff},after[]={0x31,0xc0,0x90,0x90,0x90};   // mov eax,-1 → xor eax,eax ; nop3 (뒤의 jmp +2 는 그대로)
 tx.bytes(ic+0x4067c,before,after,5);
 static const U8 e[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10};                                  // mov rax,rsp ; mov [rax+0x10],rbx
 o_invest=reinterpret_cast<int(*)(U8*,U8*)>(tx.detour(ic+0x40480,e,7,(void*)w_invest));
 bool ok=tx.install();if(!ok)i_bad=tx.badinfo;return ok;
}
static void attach(){
 if(attached||attempted)return;if(!GetModuleHandleW(L"SAN14_IntegratedConvenience.dll"))return;
 attempted=true;ic=known_common();
 if(!ic){logf("ICGuard R40: unknown integrated module; nothing changed.");r39::start(1,MOD,"SKIP",0,2,"-","unknown integrated module");
  r39::fail(1,MOD,"ic_build","attach() -> known_common() SHA-256 of SAN14_IntegratedConvenience.dll","known IC build (R40 ae365eab… / R39 / ce101 / R11 / P1)","IC loaded but hash unknown",L"통합편의(1~7번) 보강(ICGuard A~H: 자동인사 실패 복구·창 잠김 해제·사신 거절·포로 처리 처단 금지·신무장 저장 읽기·경로 보호)이 전부 안 붙음. 통합편의 파일이 통합패치 판이 아님");return;}
 game=core->resolved->image;r39::game_root(root_dir,1024);refresh_noexec();bool a=install_a(),b=install_b(),e=install_e();f_done=install_f();bool g=install_g();h_done=install_h();i_done=install_i();
 { // J (P205 확정 11-10): 설정 창에는 '턴 종료 명승 자동 투자 = 켬'(ic+0x10500c)인데 자동인사 정책의 투자 비트(ic+0x105010 & 2)가 꺼져 있으면
   //  통합편의가 투자 단계를 통째로 건너뛴다(IC+0x686d). 옛 설정 창이 켬 칸만 쓰고 비트를 안 켠 채 저장된 상태를 한 번 바로잡는다. 자동 논공(0x10500d, 비트 4)도 같음.
   int pol=at<int>(ic,0x105010),want=pol;if(at<U8>(ic,0x10500c))want|=2;if(at<U8>(ic,0x10500d))want|=4;if(merit13()){want&=~4;if(at<U8>(ic,0x10500d))__atomic_store_n(ic+0x10500d,(U8)0,__ATOMIC_SEQ_CST);}   // 20차: 13번이면 자동논공 끔
   if(want!=pol){uint64_t th=(U32)at<int>(ic,0x105014);fn<U8(*)(uint64_t)>(ic,0x65580)((uint64_t)(U32)want|(th<<32));
    r39::counter(2,MOD,"auto_policy_bits_fixed",(unsigned)(want^pol));
    if(core->Log)core->Log("ICGuard R43 J: auto-personnel policy bits now match the on/off switches (scenic invest / auto credit were on in settings but off in the saved policy).");} }attached=a||b||e||f_done;
 logf(i_done?"ICGuard R43 I: auto scenic invest skips a scenic spot that cannot be developed now instead of aborting the whole invest step.":"ICGuard R43 I: scenic-invest bytes differ; left unchanged.");
 logf(h_done?"ICGuard R42 H: path guard fails a search only for broken step costs (>0x10000000); expensive steps (>16384) pass as in the original game (counted).":"ICGuard R42 H: path-guard bytes differ; left unchanged.");
 logf(g?"ICGuard R42 G: new-officer list reads the real Steam save (userdata\\<account>\\872410\\remote) when the game-folder SteamData copy is missing.":"ICGuard R42 G: integrated file-open slot differs; new-officer path left unchanged.");
 logf(g_parse?"ICGuard R42 G: new-officer parser accepts real save records (slot check like 2D v25).":"ICGuard R42 G: new-officer parser bytes differ; left unchanged.");
 logf(f_done?"ICGuard R40 F: integrated captive processing respects No-Execute (26) when it is on.":"ICGuard R40 F: captive-processing bytes differ; left unchanged.");
 logf(e?"ICGuard R43 E: envoy refusal works while the ruler is at a gate/port (city that owns the gate/port).":"ICGuard R40 E: envoy-refusal function differs; left unchanged.");
 logf(a?"ICGuard R40 A: search runs even if invest/merit step fails; officer-level merit mismatch skips only that officer.":"ICGuard R40 A: auto-personnel bytes differ (another mod?); left unchanged.");
 logf(b?"ICGuard R40 B: 5 window busy flags released on open failure; captive list keeps first 512.":"ICGuard R40 B: window functions differ (another mod?); left unchanged.");
 r39::start(1,MOD,a&&b&&e?"OK":(a||b||e?"PARTIAL":"FAIL"),(a?1:0)+(b?1:0)+(e?1:0),3,"-",a&&b&&e?"-":(!a?"group A bytes differ":(!b?"group B bytes differ":"group E bytes differ")));
 if(!a)report(0,"IC auto-personnel (A: IC+0x1591d je, 0x4c840 check, 0x56b20 exec, 0x2d750 build)",ic,"IC",1,L"2번 자동인사 보강(ICGuard A: 논공 실패해도 탐색 계속·자동논공 새 엔진 연결)이 안 붙음 → 자동논공 새 엔진(2번)도 안 씀. 다른 기능은 그대로");
 if(!b)report(1,"IC window busy flags (B: 5 dialog fns + IC+0x2bb93)",ic,"IC",1,L"통합편의 창 잠김 해제 보강(ICGuard B)이 안 붙음 → 창 열기 실패 뒤 버튼이 안 눌리는 증상이 다시 날 수 있음");
 if(!e)report(2,"IC envoy refusal (E: IC+0x399b0)",ic,"IC",1,L"군주가 관문/항구에 있을 때 사신 거절(ICGuard E)이 안 됨");
 if(!f_done)report(3,"IC captive processing no-execute (F: IC+0x23f04)",ic,"IC",26,L"통합편의 포로 처리에서 처단 금지(26번) 적용(ICGuard F)이 안 됨 → 통합편의 자동 포로 처리가 처단할 수 있음");
 if(!g&&ic_bad[6].set)report(6,"IC new-officer file open (G: IC+0xe91b8 name / import slot IC+0x9a060)",ic,"IC",10,L"고유무장 등록 '신무장' 탭이 스팀 저장 위치(userdata)의 신무장 파일을 못 읽음 → 게임 폴더 SteamData 에 파일이 없으면 0명");
 if(!i_done)r39::fail_tx(2,MOD,i_bad,ic,"IC","IC scenic invest (I: IC+0x4067c result / IC+0x40480 entry)",L"2번 '턴 종료 명승 자동 투자' 보강(ICGuard I)이 안 붙음 → 발전시킬 수 없는 명승이 하나라도 있으면 그 턴 투자가 전부 안 되는 증상이 남음");
 if(!h_done)report(7,"IC path guard (H: IC+0x1806)",ic,"IC",1,L"경로 탐색 보호(ICGuard H)가 안 붙음 → 깨진 경로 비용으로 탐색이 멈추는 드문 튕김 보호가 빠짐");
}
static DWORD WINAPI attach_worker(LPVOID){
 uint64_t until=GetTickCount64()+600000;
 while(!attached&&!attempted&&GetTickCount64()<until){
  HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
  auto wait=common?reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1")):nullptr;
  if(wait&&wait()){AcquireSRWLockExclusive(&lock);attach();ReleaseSRWLockExclusive(&lock);break;}
  Sleep(1000);
 }
 for(int i=0;;i++){ // 남의 DLL 은 늦게 올라올 수 있음(최대 2분), 처단불가 켜짐은 계속 2초마다, 길찾기 횟수 기록은 5분마다
  if(i<60){try_c();try_d();}
  if(f_done)refresh_noexec();
  if(i%150==149&&h_done)h_report();
  if(!f_done&&!h_done&&i>=60)break;
  Sleep(2000);
 }
 return 0;
}
// 시험 장치 전용(게임에서는 안 부름): 내부 상태를 가짜 메모리로 돌려 감싸기·동굴 동작을 확인
extern "C" __declspec(dllexport) U64 SAN14_ICGuardTest(int op,U64 a,U64 b){
 switch(op){
  case 1:ic=(U8*)a;core=(const S14_CoreAPI*)b;return 1;
  case 2:game=(U8*)a;return 1;
  case 3:if(a<5)dlg[a].orig=(Fn2)b;return 1;
  case 4:{void*w[5]={(void*)w_dlg<0>,(void*)w_dlg<1>,(void*)w_dlg<2>,(void*)w_dlg<3>,(void*)w_dlg<4>};return a<5?(U64)w[a]:0;}
  case 5:return install_f()?1:0;
  case 6:return (U64)&noexec_on;
  case 7:o_rulercity=(Fn2)a;return (U64)(void*)w_rulercity;
  case 8:o_check=(Fn2)a;return (U64)(void*)w_check;
  case 9:o_exec=(Fn2)a;return (U64)(void*)w_exec;
  case 10:return is_excluded((U16)a)?1:0;
  case 11:exclude((U16)a);return 1;
  case 12:InterlockedIncrement(&turn_no);return (U64)turn_no;
  case 13:return (U64)DcePrescanC((U8*)a);
  case 14:return (U64)DceRegionC((U8*)a,(long long)b);
  case 15:return install_c((U8*)a)?1:0;
  case 16:return (U64)dce_mode|((U64)(U32)dce_j0<<8)|((U64)dce_skipped<<32);
  case 17:return b==0?(U64)(void*)DcePreStub:(U64)(void*)DceRegStub;
  case 18:return a==0?(U64)DcePreTramp:a==1?(U64)DceRegTramp:a==2?(U64)DceAbort:(U64)DceNext;
  case 19:return install_h()?1:0;
  case 20:return (U64)(U32)h_old_hits|((U64)(U32)h_blocks<<32);
  case 21:h_report();return 1;
 }
 return 0;
}
extern "C" __declspec(dllexport) U64 SAN14_ICGuardNewOff(int op,U64 a,U64 b){
 switch(op){
  case 0:return (U64)no_calls|((U64)no_found<<20)|((U64)no_miss<<40);
  case 1:return (U64)g_state;
  case 2:return newoff_resolve((const wchar_t*)a,(wchar_t*)b)?1:0;
  case 3:ic=(U8*)a;core=(const S14_CoreAPI*)b;return install_g()?1:0;
  case 4:return (U64)(void*)w_cfw;
  case 5:{wset(root_dir,1024,(const wchar_t*)a);return 1;}
  case 6:return g_parse?1:0;
 }
 return 0;
}
extern "C" __declspec(dllexport) unsigned SAN14_ICGuardState(){return (attached?1u:0u)|((unsigned)turn_no<<8);}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*a){if(!r39::check_api(a,1,MOD))return 0;core=a;HANDLE t=CreateThread(nullptr,0,attach_worker,nullptr,0,nullptr);if(t)CloseHandle(t);return 1;}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
