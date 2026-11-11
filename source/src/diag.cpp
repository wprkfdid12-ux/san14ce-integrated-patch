// R43 종합 진단 SAN14_Diag.dll (등록: SAN14Mods\modules\Diag.ini) — 2026-11-05
//  왜: 사용자 규칙 "작동이 안 되면 조용히 쉬지 말고 왜·어디서·어떻게 안 되는지 자세한 창이 반드시 떠야 한다".
//      각 모듈은 자기 실패를 r39::fail 로 띄우지만, (가) 아예 로드가 안 된 모듈, (나) 시작 기록을 안 남기고 멈춘 모듈,
//      (다) 소스가 없는 묶음 모듈(통합편의·13·15·16·17·19·22 DomesticCE·23·ModPack·Ext)은 스스로 창을 못 띄운다 → 여기서 한 번에 본다.
//  어떻게: 게임 시작 30초 뒤 1차 확인, 문제가 있으면 90초에 다시 확인해서 그때도 남은 문제만 창 하나로(막지 않음, TOPMOST, Ctrl+C 복사).
//      근거는 ① 등록 파일(modules\*.ini Enabled=1)의 DLL 이 프로세스에 로드됐는지 ② SAN14_기록\적용확인.log 의 이번 실행 시작 줄
//      ③ %LOCALAPPDATA%\SAN14CE_ModCore\runtime.log(이번 실행 "SAN14 ModCore 1.0" 뒤)의 성공 문구(설치기 '게임 적용' 판정과 같은 표)
//      ④ 통합편의 기록 SAN14CE_11Patch.log(UTF-16) 의 "지원 실행 파일 확인 완료".
//      ⑤ (시험8) 자기 기록에만 실패를 적는 모듈: 통합편의 실패 줄(모드 설정 v2·BGM·진행속도 등)·TraitHover·Traits9·미니맵·편의성 묶음 기록 → diag_ownlogs.h
//  게임 코드는 한 바이트도 안 바꾸고, 읽기만 한다. 문제가 없으면 창 없이 R43D 줄만 남긴다.
#include "r39_record.h"
#include "feature_table.h"
#include "diag_ownlogs.h"
#ifndef S14_TEST_SHIM
#include "diag_perf_win.h"   // 시험11: 프레임 진단 표본기(SAN14_Minimap.ini Profile=1 일 때만)
#endif
static const char*MOD="SAN14_Diag.dll";
static const S14_CoreAPI*core;static U8*game;
static wchar_t root[1024];static char since[20];static FILETIME since_ft;static int delay1=30,delay2=90,enabled=1;

// ---- 작은 도구
static bool starts(const char*s,const char*p){while(*p){if(*s++!=*p++)return false;}return true;}
static const char*find(const char*hay,size_t n,const char*needle){size_t m=r39::slen(needle);if(!m||n<m)return nullptr;for(size_t i=0;i+m<=n;i++){if(hay[i]==needle[0]&&!memcmp(hay+i,needle,m))return hay+i;}return nullptr;}
static const char*rfind(const char*hay,size_t n,const char*needle){size_t m=r39::slen(needle);if(!m||n<m)return nullptr;for(size_t i=n-m+1;i-->0;){if(hay[i]==needle[0]&&!memcmp(hay+i,needle,m))return hay+i;}return nullptr;}
// UTF-16 파일 내용(U16 배열)에서 찾기(wchar_t 크기에 안 기댐)
static bool wfind(const U16*hay,size_t n,const wchar_t*needle){size_t m=r39::wlen(needle);if(!m||n<m)return false;for(size_t i=0;i+m<=n;i++){size_t j=0;while(j<m&&hay[i+j]==(U16)needle[j])j++;if(j==m)return true;}return false;}
static void wnum(wchar_t*o,size_t cap,unsigned v){char t[12]={};r39::snum(t,sizeof t,v);r39::wcat_a(o,cap,t);}
// 파일 꼬리 읽기(최대 cap 바이트). 돌아온 값 = 읽은 바이트 수
static size_t tail(const wchar_t*path,char*buf,size_t cap,FILETIME*mtime){
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return 0;
 LARGE_INTEGER sz;if(!GetFileSizeEx(f,&sz)){CloseHandle(f);return 0;}
 long long start=sz.QuadPart>(long long)cap?sz.QuadPart-(long long)cap:0;LARGE_INTEGER mv;mv.QuadPart=start;SetFilePointerEx(f,mv,0,0);
 DWORD n=0;ReadFile(f,buf,(DWORD)cap,&n,0);CloseHandle(f);
 if(mtime){WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(path,&fd);if(h!=INVALID_HANDLE_VALUE){*mtime=fd.ftLastWriteTime;FindClose(h);}else{mtime->dwLowDateTime=mtime->dwHighDateTime=0;}}
 return n;
}
static bool newer(const FILETIME&a,const FILETIME&b){return a.dwHighDateTime>b.dwHighDateTime||(a.dwHighDateTime==b.dwHighDateTime&&a.dwLowDateTime>=b.dwLowDateTime);}
static bool exists(const wchar_t*rel){wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,rel);return GetFileAttributesW(p)!=0xffffffffu;}
static bool loaded(const wchar_t*file){return GetModuleHandleW(file)!=nullptr;}

// ---- 진단 결과
struct Problem{int feature;wchar_t text[420];};
static Problem probs[48];static unsigned nprob;static unsigned n_reg,n_loaded,n_started;
static void add(int feature,const wchar_t*name,const wchar_t*what,const char*evidence){
 if(nprob>=48)return;Problem&p=probs[nprob++];p.feature=feature;p.text[0]=0;const size_t C=420;
 if(feature){wnum(p.text,C,(unsigned)feature);r39::wcat(p.text,C,L"번 ");}else r39::wcat(p.text,C,L"공통 ");
 r39::wcat(p.text,C,name);r39::wcat(p.text,C,L": ");r39::wcat(p.text,C,what);
 if(evidence&&evidence[0]){r39::wcat(p.text,C,L" [근거: ");r39::wcat_a(p.text,C,evidence);r39::wcat(p.text,C,L"]");}
}
// 근거가 UTF-8(한글) 인 문제(자기 기록 줄)
static void add_u8(int feature,const wchar_t*name,const wchar_t*what,const char*u8text,unsigned count){
 if(nprob>=48)return;Problem&p=probs[nprob++];p.feature=feature;p.text[0]=0;const size_t C=420;
 if(feature){wnum(p.text,C,(unsigned)feature);r39::wcat(p.text,C,L"번 ");}else r39::wcat(p.text,C,L"공통 ");
 r39::wcat(p.text,C,name);r39::wcat(p.text,C,L": ");r39::wcat(p.text,C,what);r39::wcat(p.text,C,L" [");
 size_t n=r39::wlen(p.text);
#ifdef S14_TEST_SHIM
 r39::wcat_a(p.text,C,u8text);
#else
 int k=MultiByteToWideChar(CP_UTF8,0,u8text,-1,p.text+n,(int)(C-n-1));if(k>0)p.text[n+k-1]=0;else p.text[n]=0;
#endif
 r39::wcat(p.text,C,L"]");if(count>1){r39::wcat(p.text,C,L" (같은 줄 ");wnum(p.text,C,count);r39::wcat(p.text,C,L"번)");}
}
// 적용확인.log 이번 실행 줄 모음(최대 256KB 꼬리)
static char aplog[262144];static size_t aplog_n;
static char rtlog[262144];static size_t rtlog_n;static const char*rt;static size_t rt_n;   // runtime.log 이번 실행 부분
static U16 iclog[65536];static size_t iclog_n;static bool ic_fresh;
static void load_logs(){
 wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,L"SAN14_\xAE30\xB85D\\\xC801\xC6A9\xD655\xC778.log");
 aplog_n=tail(p,aplog,sizeof aplog-1,nullptr);aplog[aplog_n]=0;
 wchar_t la[600];DWORD k=GetEnvironmentVariableW(L"LOCALAPPDATA",la,600);rt=nullptr;rt_n=0;
 if(k&&k<590){wchar_t q[700];q[0]=0;r39::wcat(q,700,la);r39::wcat(q,700,L"\\SAN14CE_ModCore\\runtime.log");rtlog_n=tail(q,rtlog,sizeof rtlog-1,nullptr);rtlog[rtlog_n]=0;
  const char*m=rfind(rtlog,rtlog_n,"SAN14 ModCore 1.0");if(m){rt=m;rt_n=rtlog_n-(size_t)(m-rtlog);}else if(rtlog_n){rt=rtlog;rt_n=rtlog_n;}}
 wchar_t ip[1100];ip[0]=0;r39::wcat(ip,1100,root);r39::wcat(ip,1100,L"SAN14CE_11Patch.log");FILETIME mt{};
 size_t b=tail(ip,(char*)iclog,sizeof iclog-2,&mt);iclog_n=b/2;iclog[iclog_n]=0;ic_fresh=b&&newer(mt,since_ft);
}
static bool R(const char*phrase){return rt&&find(rt,rt_n,phrase)!=nullptr;}
// ---- 정식판 뒤(11-08): '다른 모드와 충돌' 판정 — 확인 때 실패가 보일 때만 한 번(평소 비용 없음)
#include "r43_t9slots.h"
struct Conf{r43c::Owner o;wchar_t what[60];U32 rva;};
static Conf confs[8];static unsigned nconf;
static void add_conf(const r43c::Owner&o,const wchar_t*what,U32 rva){
 if(o.kind==r43c::K_NONE)return;
 for(unsigned i=0;i<nconf;i++)if(confs[i].o.kind==o.kind&&r43c::ieq(confs[i].o.name,o.name))return;
 if(nconf>=8)return;Conf&c=confs[nconf++];c.o=o;c.what[0]=0;r39::wcat(c.what,60,what);c.rva=rva;
}
static void note_owner(unsigned pi,const r43c::Owner&o,U32 rva){   // 문제 줄 끝에 판정
 if(pi>=nprob||o.kind==r43c::K_NONE)return;wchar_t*t=probs[pi].text;const size_t C=420;
 r39::wcat(t,C,o.kind==r43c::K_FOREIGN?L" → 다른 모드와 충돌: ":o.kind==r43c::K_OURS?L" → 통합패치 기능끼리 겹침: ":L" → 다른 모드와 충돌 가능성(바꾼 파일 못 찾음)");
 if(o.kind!=r43c::K_CHANGED)r39::wcat(t,C,o.name);
 r39::wcat(t,C,L" (exe+0x");char h[16]={};r39::shex(h,sizeof h,rva);r39::wcat_a(t,C,h);r39::wcat(t,C,L")");
}
// 통합편의(R43 판) 안 표 위치 확인: 기다림 함수 코드·원본 확인 표 첫 칸이 맞을 때만 그 표를 읽음
static const U8*ic_known(){
 const U8*ic=(const U8*)GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");if(!ic)return nullptr;
 static const U8 wc[12]={0x48,0x83,0xec,0x28,0xc7,0x44,0x24,0x20,0xdc,0x05,0x00,0x00};
 if(!r43c::readable(ic+0x1f8120,12)||memcmp(ic+0x1f8120,wc,12))return nullptr;
 if(!r43c::readable(ic+0xb7310,16)||r43c::rd64(ic+0xb7310)!=0x1f473||r43c::rd32(ic+0xb7318)!=3)return nullptr;
 if(!r43c::readable(ic+0xdedd0,0x28*51)||r43c::rd64(ic+0xdedd0)!=0x174cd||r43c::rd64(ic+0xdedd0+0x18)!=5)return nullptr;
 if(!r43c::readable(ic+0x1fa000,4))return nullptr;
 return ic;
}
// 통합편의가 '게임 코드 원본 확인'(2,346곳, 최대 3분)을 아직 못 끝냈나(끝나면 0x1fa000 = 1)
static bool ic_waiting(){const U8*ic=ic_known();return ic&&*(volatile const U32*)(ic+0x1fa000)==0;}
// 원본 확인 표에서 바뀐 자리 → 주인. 반환: 첫 판정(문제 줄용)
static r43c::Owner scan_ic_restore(U32*first_rva){
 r43c::Owner first{};first.kind=r43c::K_NONE;*first_rva=0;const U8*ic=ic_known();if(!ic||!game)return first;
 const U8*tbl=ic+0xb7310,*exp=ic+0xb5710;unsigned changed=0;
 for(unsigned i=0;i<0x92a;i++){U32 rva=(U32)r43c::rd64(tbl+16*i);U32 len=r43c::rd32(tbl+16*i+8),eo=r43c::rd32(tbl+16*i+12);
  if(!len||len>32||rva>=S14_GAME_IMAGE)continue;const U8*cur=game+rva,*e=exp+eo;if(!memcmp(cur,e,len))continue;changed++;
  size_t d0=0,d1=len;while(d0<len&&cur[d0]==e[d0])d0++;while(d1>d0&&cur[d1-1]==e[d1-1])d1--;
  r43c::Owner o=r43c::owner_of_site(cur,len,d0,d1);add_conf(o,L"통합편의 원본 확인 자리",rva);if(first.kind==r43c::K_NONE||(first.kind==r43c::K_CHANGED&&o.kind!=r43c::K_CHANGED)){first=o;*first_rva=rva;}
  if(nconf>=8)break;
 }
 (void)changed;return first;
}
// 5번 수행·대열 자리(통합편의 표 51곳): 원래 call/jmp 도 아니고 통합편의 자기 연결도 아니면 → 주인
static r43c::Owner scan_ic_follow(U32*first_rva){
 r43c::Owner first{};first.kind=r43c::K_NONE;*first_rva=0;const U8*ic=ic_known();if(!ic||!game)return first;
 for(unsigned i=0;i<51;i++){const U8*r=ic+0xdedd0+0x28*i;U32 site=(U32)r43c::rd64(r),orig=(U32)r43c::rd64(r+8);U8 op=(U8)r43c::rd64(r+0x20);
  if(!site||site>=S14_GAME_IMAGE||r43c::rd64(r+0x18)!=5)continue;const U8*cur=game+site;
  if(cur[0]==op&&(U32)((int32_t)r43c::rd32(cur+1))==(U32)(orig-(site+5)))continue;   // 원래 모양
  r43c::Owner o=r43c::owner_of_site(cur,5,0,5);
  if(o.kind==r43c::K_OURS&&r43c::ieq(o.name,L"SAN14_IntegratedConvenience.dll"))continue;   // 통합편의 자기 연결
  add_conf(o,L"5번 수행·대열 이동 자리",site);if(first.kind==r43c::K_NONE){first=o;*first_rva=site;}
 }
 return first;
}
// 6번 진행 배속: 통합편의 '진행속도 확장 준비 검사'(IC+0x54050)가 고치려는 게임 자리 5곳(함수표 칸 3 + 코드 2). R43 11-08 제보 '진행속도 확장 준비 검사 실패'.
//  칸이 원래 함수도 통합편의도 아닌 곳을 가리키거나, 코드 자리가 다른 DLL 로 갈리면 → 주인
static r43c::Owner scan_ic_speed(U32*first_rva){
 r43c::Owner first{};first.kind=r43c::K_NONE;*first_rva=0;if(!game)return first;
 static const U32 vt[3][2]={{0x1476e58,0x880570},{0x1476e68,0x896620},{0x1476e80,0x895910}};
 for(const auto&e:vt){if(!r43c::readable(game+e[0],8))continue;const U8*v=(const U8*)r43c::rd64(game+e[0]);if(v==game+e[1])continue;
  r43c::Owner o=r43c::owner_of_ptr(v);if(o.kind!=r43c::K_FOREIGN&&o.kind!=r43c::K_OURS)continue;if(o.kind==r43c::K_OURS&&r43c::ieq(o.name,L"SAN14_IntegratedConvenience.dll"))continue;
  add_conf(o,L"6번 진행 배속 자리",e[0]);if(first.kind==r43c::K_NONE){first=o;*first_rva=e[0];}}
 static const U32 cs[2][2]={{0x89abf7,5},{0x44e2c1,0x2d}};
 for(const auto&e:cs){if(!r43c::readable(game+e[0],e[1]))continue;r43c::Owner o=r43c::owner_of_site(game+e[0],e[1],0,e[1]);
  if(o.kind!=r43c::K_FOREIGN&&o.kind!=r43c::K_OURS)continue;if(o.kind==r43c::K_OURS&&r43c::ieq(o.name,L"SAN14_IntegratedConvenience.dll"))continue;
  add_conf(o,L"6번 진행 배속 자리",e[0]);if(first.kind==r43c::K_NONE){first=o;*first_rva=e[0];}}
 return first;
}
// 9번 화면 부품의 개성 칸 처리 자리 146곳(함수표 칸): 원래 함수도 9번 화면 부품도 아니면 → 주인
static r43c::Owner scan_t9_slots(U32*first_rva){
 r43c::Owner first{};first.kind=r43c::K_NONE;*first_rva=0;if(!game)return first;
 for(const auto&e:kR43T9Slots){const U8*v=(const U8*)r43c::rd64(game+e[0]);if(v==game+e[1])continue;
  r43c::Owner o=r43c::owner_of_ptr(v);if(o.kind==r43c::K_OURS&&r43c::ieq(o.name,L"SAN14_Traits9_UI.dll"))continue;
  add_conf(o,L"9번 개성 칸 처리 자리",e[0]);if(first.kind==r43c::K_NONE){first=o;*first_rva=e[0];}
 }
 return first;
}
// 2026-11-10(P208, gggoeik R43D-5): 9번 화면·13번이 실패했을 때 창에 '그 자리를 가진 DLL 이름'이 없어 누구와 충돌인지 추정만 했음.
//  9번 개성 칸 146곳 중 통합패치 것이 아닌 주인을 찾음(다른 모드 우선, 없으면 '바꾼 파일 못 찾음'). 통합패치 14번이 가진 칸은 정상이라 뺌.
static r43c::Owner scan_t9_foreign(U32*first_rva){
 r43c::Owner first{};first.kind=r43c::K_NONE;*first_rva=0;if(!game)return first;
 for(const auto&e:kR43T9Slots){const U8*v=(const U8*)r43c::rd64(game+e[0]);if(v==game+e[1])continue;
  r43c::Owner o=r43c::owner_of_ptr(v);if(o.kind==r43c::K_OURS||o.kind==r43c::K_NONE)continue;
  add_conf(o,L"9번 개성 칸 처리 자리",e[0]);
  if(first.kind==r43c::K_NONE||(first.kind==r43c::K_CHANGED&&o.kind==r43c::K_FOREIGN)){first=o;*first_rva=e[0];}
  if(first.kind==r43c::K_FOREIGN)break;
 }
 return first;
}
// 적용확인.log: 이번 실행(시각 ≥ since) 에 module 의 시작 줄이 있나. kind: "R39" (col3 시각) / "R42D" (마지막 칸 시각, col4 == "start")
static bool start_line(const char*module,char*state_out,size_t state_cap){
 const char*s=aplog;const char*end=aplog+aplog_n;bool found=false;
 while(s<end){const char*e=s;while(e<end&&*e!='\n')e++;size_t len=(size_t)(e-s);
  // 칸 나누기
  const char*col[12];unsigned nc=0;const char*c=s;col[nc++]=c;for(const char*x=s;x<e&&nc<12;x++)if(*x=='\t')col[nc++]=x+1;
  auto cs=[&](unsigned i,const char*v){if(i>=nc)return false;const char*a=col[i];size_t m=r39::slen(v);if((size_t)(e-a)<m||memcmp(a,v,m))return false;return a[m]=='\t'||a+m==e||a[m]=='\r';};
  if(nc>=5&&(cs(0,"R39")||cs(0,"R39C")||cs(0,"R42D"))&&cs(2,module)){
   bool d=cs(0,"R42D");auto is_time=[&](const char*a){return (size_t)(e-a)>=19&&a[0]=='2'&&a[1]=='0'&&a[4]=='-'&&a[7]=='-'&&a[10]==' ';};
   bool d_mid=d&&nc>=6&&is_time(col[4]);   // (가) 형식
   const char*t=d?(d_mid?col[4]:col[nc-1]):cs(0,"R39C")?col[nc-1]:col[3];
   if(is_time(t)&&memcmp(t,since,19)>=0){
    if(d&&!cs(3,"start")){s=e+1;continue;}
    found=true;
    if(state_out){state_out[0]=0;const char*st=cs(0,"R39")?col[4]:d?(d_mid?col[5]:col[4]):col[3];
     if(d&&!d_mid){ // AutoMerit: "version=OK ..." 글 → OK / "version=DIFFERS" → FAIL
      const char*v="version=OK";size_t vm=r39::slen(v);bool ok=(size_t)(e-st)>=vm&&!memcmp(st,v,vm);r39::scat(state_out,state_cap,ok?"OK":"FAIL");}
     else{size_t m=0;while(st+m<e&&st[m]!='\t'&&st[m]!='\r'&&m+1<state_cap)m++;memcpy(state_out,st,m);state_out[m]=0;}}
   }
  }
  (void)len;s=e+1;
 }
 return found;
}
// 이번 실행 R43F 줄(각 모듈이 이미 창을 띄운 실패) 요약
static void collect_r43f(wchar_t*out,size_t cap,unsigned*count){
 const char*s=aplog;const char*end=aplog+aplog_n;*count=0;
 while(s<end){const char*e=s;while(e<end&&*e!='\n')e++;
  if((size_t)(e-s)>6&&starts(s,S14_PATCH_VER "F\t")){
   const char*col[12];unsigned nc=0;col[nc++]=s;for(const char*x=s;x<e&&nc<12;x++)if(*x=='\t')col[nc++]=x+1;
   if(nc>=10&&(size_t)(e-col[3])>=19&&memcmp(col[3],since,19)>=0){(*count)++;if(*count<=12){r39::wcat(out,cap,L"\r\n  ");
    auto piece=[&](unsigned ci,size_t lim){char t[64];size_t m=0;const char*a=col[ci];while(a+m<e&&a[m]!='\t'&&a[m]!='\r'&&m<lim&&m<63)m++;memcpy(t,a,m);t[m]=0;r39::wcat_a(out,cap,t);};
    piece(1,12);r39::wcat(out,cap,L"번 ");piece(2,40);r39::wcat(out,cap,L" 단계=");piece(4,24);r39::wcat(out,cap,L" 코드=");piece(9,32);}}
  }
  s=e+1;
 }
}
// ---- 모듈별 판정
//  우리 모듈: 시작 줄(적용확인.log) 또는 runtime.log 문구. 묶음 모듈: runtime.log/통합편의 기록 문구(설치기 Judge 표와 같음)
struct Rule{const wchar_t*ini;const char*start_module;const char*rt_ok;const char*rt_bad;const wchar_t*bad_text;};
static const Rule kRules[]={
 {L"IntegratedConvenience_GreenlandCompat.ini",nullptr,nullptr,nullptr,nullptr},
 {L"ModSettings.ini","SAN14_ModSettings.dll",nullptr,nullptr,nullptr},
 {L"ICGuard.ini","SAN14_ICGuard.dll",nullptr,nullptr,nullptr},
 {L"SettingsHelpScenic.ini",nullptr,"Scenery R4.3:","SettingsHelp: unknown integrated module",   /* P203(11-09): 통합편의 해시를 몰라 명승 연출 생략이 조용히 꺼져 있었음 → 창으로 */L"통합편의 파일을 못 알아봐 '명승 연출 생략'과 설정 창 설명이 꺼짐(통합패치 파일이 서로 다른 판으로 섞였을 수 있음 → 설치기로 다시 설치)"},
 {L"CrashLog.ini",nullptr,nullptr,nullptr,nullptr},
 {L"ExtBridge.ini",nullptr,"ExtBridge R39: dispatch finished",nullptr,nullptr},
 {L"Diag.ini",nullptr,nullptr,nullptr,nullptr},
 {L"AutoPersonnelRepair.ini",nullptr,"AutoPersonnel",nullptr,nullptr},
 {L"AutoPersonnelRepairMerit.ini","SAN14_AutoMerit.dll",nullptr,nullptr,nullptr},
 {L"SceneSkip.ini","SAN14_SceneSkip.dll",nullptr,nullptr,nullptr},
 {L"Traits9Check.ini","SAN14_Traits9Check.dll",nullptr,nullptr,nullptr},
 {L"Mod09_Traits9Check.ini","SAN14_Traits9Check.dll",nullptr,nullptr,nullptr},   // 정식판 뒤: 14번보다 먼저 올라오게 바꾼 이름
 {L"MeritTraits.ini",nullptr,"SAN14CE.MeritTraits.HoverX RVA=","merit awards not enabled",L"13번 시작 실패(runtime.log 'MeritTraits:' 줄 참조)"},
 {L"RelatedOfficersTab.ini",nullptr,"RelatedOfficersTab RC2.4: HOOKS_INSTALLED",nullptr,nullptr},
 {L"SortiePresets.ini",nullptr,"SortiePresets RVA=",nullptr,nullptr},
 {L"Stats200New.ini","SAN14_Stats200New.dll",nullptr,nullptr,nullptr},
 {L"FPS120.ini",nullptr,"FPS120 R8 ACTIVE",nullptr,nullptr},
 {L"PrisonerReturn.ini",nullptr,"PrisonerReturn R4 loaded",nullptr,nullptr},
 {L"DomesticCE.ini",nullptr,"DomesticCE loaded:","READ-ONLY",  /* 세이브 전에도 남는 시작 줄(ACTIVE 는 세이브를 불러와야 남아 타이틀에서 오래 있으면 잘못 잡혔음) */L"읽기 전용 모드로 돌고 있음(게임 자료 주소 확인 실패)"},
 {L"DomesticTalent.ini",nullptr,"SAN14CE.DomesticTalent RVA=",nullptr,nullptr},
 {L"DomesticAI.ini","SAN14_DomesticAI.dll",nullptr,nullptr,nullptr},
 {L"DomesticPlayer.ini","SAN14_DomesticPlayer.dll",nullptr,nullptr,nullptr},
 {L"CityCaps.ini",nullptr,"CityCaps: city limits extended",nullptr,nullptr},
 {L"Patch27.ini",nullptr,"Patch27: checked feature groups",nullptr,nullptr},
 {L"ExecuteLock.ini","SAN14_ExecuteLock.dll",nullptr,nullptr,nullptr},
 {L"StatusColor.ini","SAN14_StatusColor.dll",nullptr,nullptr,nullptr},
 {L"DragGive.ini",nullptr,nullptr,nullptr,nullptr},
 {L"DragLearn.ini",nullptr,nullptr,nullptr,nullptr},
 {L"ItemPicture.ini","SAN14_ItemPicture.dll",nullptr,nullptr,nullptr},
 {L"WheelRegion.ini",nullptr,nullptr,nullptr,nullptr},
 {L"HiredFace.ini","SAN14_HiredFace.dll",nullptr,nullptr,nullptr},
 {L"Range.ini","SAN14_Range.dll",nullptr,nullptr,nullptr},
 {L"RegionOfficer.ini","SAN14_RegionOfficer.dll",nullptr,nullptr,nullptr},
};
static const Rule*rule_of(const wchar_t*ini){for(const Rule&r:kRules){const wchar_t*a=r.ini,*b=ini;while(*a&&*b){wchar_t x=*a,y=*b;if(x>=L'A'&&x<=L'Z')x+=32;if(y>=L'A'&&y<=L'Z')y+=32;if(x!=y)break;a++;b++;}if(!*a&&!*b)return &r;}return nullptr;}
static bool bad_state(const char*st){return !st[0]||starts(st,"FAIL")||starts(st,"PART")||starts(st,"SKIP")||starts(st,"IDLE");}
// ---- 정식판 뒤(11-08 갤 제보 '9칸 안 깔았는데 9칸 오류'): 설치기 기록(.san14ce_compat_r3/state.xml)의 고른 항목.
//  통합편의는 게임 폴더에 SAN14_Traits9.dll 이 '있기만 하면' 불러서, 기록 없이 남은 옛 판 9번 파일도 9칸 기록을 남김 → 안 고른 항목의 기록은 문제로 안 셈.
//  기록을 못 읽으면(옛 설치기·없음) 예전처럼 다 봄.
static bool sel_known;static bool sel_on[64];
static char inst_ver[24];   // R43.29: 설치기 기록의 판(R43·R43.28 …) — 진단 창에 적어 제보 글자만으로 판을 알게
static void load_selection(){
 sel_known=false;for(bool&b:sel_on)b=false;inst_ver[0]=0;
 wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,L".san14ce_compat_r3\\state.xml");
 static char b[262144];size_t n=tail(p,b,sizeof b-1,nullptr);b[n]=0;
 if(n){const char*q=find(b,n,"installer=\"");if(q){q+=11;size_t k=0;while(k+1<sizeof inst_ver&&q+k<b+n&&q[k]!='"'&&q[k]>' ')k++;memcpy(inst_ver,q,k);inst_ver[k]=0;}}
 // 번호가 같은 이 판 설치기 기록만 믿음: R43 과 R43.28 처럼 'R43.차수'(R43.28 부터 설치기 판 = 깃허브 판 번호)
 if(!n||!find(b,n,"<Selected>")||!(find(b,n,"installer=\"" S14_PATCH_VER "\"")||find(b,n,"installer=\"" S14_PATCH_VER ".")))return;
 for(int id=1;id<64;id++){char pat[48];pat[0]=0;r39::scat(pat,sizeof pat,"<Feature id=\"");r39::snum(pat,sizeof pat,(unsigned)id);r39::scat(pat,sizeof pat,"\" enabled=\"true\"");if(find(b,n,pat))sel_on[id]=true;}
 sel_known=true;
}
// 26·27·28·30번은 남의 키트를 따로 깔아 쓰는 사용자도 있어(설치기도 그 자리는 안 지움) 늘 '고름'으로 봄
static bool selected(int id){
 if(!sel_known||id==26||id==27||id==28||id==30||(id>0&&id<64&&sel_on[id]))return true;
 if(id==1){for(int k=2;k<64;k++)if(sel_on[k]&&k!=20&&k!=31&&k!=12)return true;}   // R43 11-08: 1번은 20·31 말고 다른 번호가 있으면 설치기가 늘 같이 깖(옛 기록에 빠져 있어도)
 return false;
}
static unsigned n_leftover;   // 이번 확인: 남은 옛 파일 문제 수(창 맨 위 큰 제목용)
static bool icw;static unsigned ic_prob=99;   // 이번 확인: 통합편의가 원본 확인 중인가, 그 문제 번호
static const wchar_t*deps[8];static unsigned ndep;
static void dep_add(const wchar_t*n){if(ndep<8)deps[ndep++]=n;}
// 통합편의 시작이 끝나야 시작 줄을 남기는 모듈(ICGuard·ExecuteLock·AutoPersonnelRepair·Patch27 = SAN14_WaitConvenienceV1 사용)
static bool ic_dep(const Rule*r){
 if(r->start_module&&(starts(r->start_module,"SAN14_ICGuard.dll")||starts(r->start_module,"SAN14_ExecuteLock.dll")))return true;
 // 정식판 뒤(11-08): 29번·32번은 통합편의 원본 확인 자리를 고쳐서, 확인이 끝난 뒤에 고침(ic_after_restore.h)
 if(r->start_module&&(starts(r->start_module,"SAN14_HiredFace.dll")||starts(r->start_module,"SAN14_StatusColor.dll")))return true;
 if(r->rt_ok&&(starts(r->rt_ok,"AutoPersonnel")||starts(r->rt_ok,"Patch27:")))return true;
 return false;
}
static void check_registered(){
 n_reg=n_loaded=n_started=0;ndep=0;ic_prob=99;
 wchar_t pat[1100];pat[0]=0;r39::wcat(pat,1100,root);r39::wcat(pat,1100,L"SAN14Mods\\modules\\*.ini");
 WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE){add(0,L"모듈 등록 폴더",L"SAN14Mods\\modules\\*.ini 를 못 읽음","FindFirstFileW failed");return;}
 do{
  if(fd.dwFileAttributes&0x10)continue;
  wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,L"SAN14Mods\\modules\\");r39::wcat(p,1100,fd.cFileName);
  if(GetPrivateProfileIntW(L"Mod",L"Enabled",0,p)!=1)continue;
  wchar_t file[128];file[0]=0;GetPrivateProfileStringW(L"Mod",L"File",L"",file,128,p);if(!file[0])continue;
  n_reg++;const FeatIni*f=feat_of_ini(fd.cFileName);const Rule*r=rule_of(fd.cFileName);int feature=f?f->feature:0;const wchar_t*name=f?f->name:fd.cFileName;
  char ev[200];ev[0]=0;char fa[64];fa[0]=0;for(int i=0;file[i]&&i<60;i++){fa[i]=(char)(file[i]<128?file[i]:'?');fa[i+1]=0;}
  if(!loaded(file)){
   bool on_disk=exists(file);r39::scat(ev,sizeof ev,fa);r39::scat(ev,sizeof ev,on_disk?" not in process (ModCore did not load it or its DllMain/ModInit failed)":" file missing in game folder");
   add(feature,name,on_disk?L"DLL 이 프로세스에 로드되지 않음(SAN14_ModInitV1 이 0 을 돌려줬거나 ModCore 가 건너뜀)":L"DLL 파일이 게임 폴더에 없음(설치기 [선택 설치] 다시)",ev);continue;}
  n_loaded++;
  // 사용자 11-08: 안 고른 항목의 등록 파일이 남아 실행 중 → 옛 파일 남음(숨기지 않음, 설치기가 정리)
  {int n1=feature,n2=0;if(fd.cFileName[0]==L'M'&&fd.cFileName[1]==L'o'&&fd.cFileName[2]==L'd'&&fd.cFileName[3]==L'0'){n1=9;n2=13;}
   if(feature==9){n1=9;n2=13;}if(feature==25){n1=1;n2=25;}
   if(feature>1&&r!=nullptr&&rule_of(L"ExtBridge.ini")!=r&&!selected(n1)&&!(n2&&selected(n2))){r39::scat(ev,sizeof ev,fa);r39::scat(ev,sizeof ev," registered but item not selected in installer state");
    n_leftover++;add(feature,name,L"설치 안 한 기능의 옛 파일(등록 파일·DLL)이 남아 실행 중 → 설치기에서 '선택 설치'(또는 설치 갱신)를 한 번 누르면 정리됩니다",ev);continue;}}
  if(!r){n_started++;continue;}
  // 통합편의: 자기 기록으로
  if(f&&f->feature==1&&r->start_module==nullptr&&r->rt_ok==nullptr&&fd.cFileName[0]==L'I'){
   // 정식판 뒤(11-08 제보): 통합편의가 게임 코드 원본 확인(최대 3분)에서 막히면 1~7번 전부 안 켜지고, 기다리던 25·26번·1번 보강도 줄줄이 '시작 기록 없음'으로 보였음
   //  → 원인 한 줄로 묶고, 바뀐 자리의 주인(다른 모드)을 찾아 붙임
   bool gaveup=ic_fresh&&wfind(iclog,iclog_n,L"3분 안에 게임 코드 복원을 확인하지 못했습니다");
   if(icw||gaveup){add(1,name,gaveup?L"통합편의가 3분 동안 게임 코드 원본 확인을 못 해서 1~7번을 전부 안 켰음":L"통합편의가 게임 코드 원본 확인을 아직 못 끝냄(최대 3분 기다림) → 1~7번이 아직 안 켜짐",gaveup?"IC log: restore not confirmed within 3 min":"IC restore-wait flag still 0");
    ic_prob=nprob-1;U32 rv=0;r43c::Owner o=scan_ic_restore(&rv);note_owner(ic_prob,o,rv);continue;}
   if(ic_fresh&&wfind(iclog,iclog_n,L"지원 실행 파일 확인 완료"))n_started++;
   else add(1,name,ic_fresh?L"통합편의 기록에 '지원 실행 파일 확인 완료'가 없음(통합편의가 게임을 못 알아봄 → 1~7번 전부 꺼짐)":L"통합편의 기록(SAN14CE_11Patch.log)이 이번 실행에 안 써짐",ic_fresh?"SAN14CE_11Patch.log tail lacks success phrase":"SAN14CE_11Patch.log mtime < session start");
   continue;}
  if(r->start_module){char st[32];st[0]=0;bool sl=start_line(r->start_module,st,sizeof st);
   if(sl&&!bad_state(st))n_started++;
   else if(sl){r39::scat(ev,sizeof ev,"apply log start state=");r39::scat(ev,sizeof ev,st);add(feature,name,L"시작은 했지만 상태가 실패/일부(자세한 창은 그 모듈이 따로 띄움; 적용확인.log 참조)",ev);
    if(starts(r->start_module,"SAN14_Traits9Check.dll")){U32 rv=0;r43c::Owner o=scan_t9_foreign(&rv);note_owner(nprob-1,o,rv);}}   // P208: 9번 실패 → 개성 칸을 가진 DLL
   else if(icw&&ic_dep(r)){dep_add(name);}
   else{r39::scat(ev,sizeof ev,"no start line for ");r39::scat(ev,sizeof ev,r->start_module);r39::scat(ev,sizeof ev," since ");r39::scat(ev,sizeof ev,since);add(feature,name,L"로드는 됐는데 이번 실행 시작 기록이 없음(SAN14_ModInitV1 이 안 불렸거나 도중에 멈춤)",ev);}
   continue;}
  if(r->rt_ok){
   // 정식판 뒤: 25번을 안 깔고 1번만 깔면 Patch27.dll 은 설정 창(6쪽)용으로만 실리고 'features off' 를 남김 → 정상(오경보였음)
   if(rt&&starts(r->rt_ok,"Patch27:")&&R("Patch27 R22: features off")){n_started++;continue;}
   // 정식판 뒤(제보 ic_wait): 통합편의 시작이 15초를 넘기면 25번은 뒤에서 최대 10분 더 기다렸다가 켜짐(못 켜지면 25번이 직접 창) → 확인 시점엔 '기다리는 중'도 정상
   if(rt&&starts(r->rt_ok,"Patch27:")&&R("Patch27: common module startup slow")){n_started++;continue;}
   if(!rt){add(feature,name,L"runtime.log 를 못 읽음(%LOCALAPPDATA%\\SAN14CE_ModCore\\runtime.log)","runtime.log missing/unreadable");continue;}
   if(R(r->rt_ok)){if(r->rt_bad&&R(r->rt_bad)&&r->bad_text){r39::scat(ev,sizeof ev,"runtime.log has '");r39::scat(ev,sizeof ev,r->rt_bad);r39::scat(ev,sizeof ev,"'");add(feature,name,r->bad_text,ev);
     if(starts(r->rt_ok,"SAN14CE.MeritTraits")){U32 rv=0;r43c::Owner o=scan_t9_foreign(&rv);note_owner(nprob-1,o,rv);}   // P208: 13번은 9칸 화면이 붙어야 켜짐 → 개성 칸을 가진 DLL
    }else n_started++;continue;}
   if(r->rt_bad&&R(r->rt_bad)&&r->bad_text){const char*m=rfind(rt,rt_n,r->rt_bad);char t[120];t[0]=0;size_t k=0;while(m&&m[k]&&m[k]!='\r'&&m[k]!='\n'&&k<110){t[k]=m[k];k++;}t[k]=0;add(feature,name,r->bad_text,t);continue;}
   if(icw&&ic_dep(r)){dep_add(name);continue;}
   r39::scat(ev,sizeof ev,"runtime.log(this session) lacks '");r39::scat(ev,sizeof ev,r->rt_ok);r39::scat(ev,sizeof ev,"'");
   add(feature,name,L"로드는 됐는데 성공 기록이 없음(시작 도중 멈춤 또는 게임 판/다른 모드와 충돌)",ev);continue;}
  n_started++;   // 규칙 없음: 로드만 확인
 }while(FindNextFileW(h,&fd));
 FindClose(h);
 if(ndep){if(ic_prob<nprob){wchar_t*t=probs[ic_prob].text;r39::wcat(t,420,L" · 그래서 아직 시작 못 한 기능: ");for(unsigned i=0;i<ndep;i++){if(i)r39::wcat(t,420,L", ");r39::wcat(t,420,deps[i]);}}
  else for(unsigned i=0;i<ndep;i++)add(0,deps[i],L"통합편의 시작을 기다리는 중",nullptr);}
}
// 등록 파일 없이 따로 실리는 묶음(ModPack·Ext·안개·3D): 파일이 있으면 로드/문구 확인
static void check_others(){
 struct Extra{const wchar_t*rel;const wchar_t*dll;int feature;const wchar_t*name;const char*rt_ok;const char*rt_bad;const wchar_t*bad_text;const wchar_t*ini;const wchar_t*sec;const wchar_t*key;};
 // 시험14: 사용자가 모드 설정 창에서 끈 기능(ExtBridge.ini [Ext] / ModPackBridge.ini [ModPack] 값 0)은 안 실리는 게 정상 → 문제로 안 셈(16:21 'FogTrial 로드 안 됨' 거짓 경보)
 static const Extra ex[]={
  {L"SAN14_ModPackBridge.dll",L"SAN14_ModPackBridge.dll",9,L"모음팩 연결(ModPackBridge)",nullptr,nullptr,nullptr,nullptr,nullptr,nullptr},
  {L"SAN14Mods\\ModPack\\SAN14_Collab.dll",L"SAN14_Collab.dll",9,L"콜라보 해금(Collab)","Collab entry invoked",nullptr,nullptr,L"ModPackBridge.ini",L"ModPack",L"Collab"},
  {L"SAN14Mods\\ModPack\\SAN14_TraitHover.dll",L"SAN14_TraitHover.dll",14,L"개성·전법 마우스 설명(TraitHover)","TraitHover entry invoked",nullptr,nullptr,L"ModPackBridge.ini",L"ModPack",L"TraitHover"},
  {L"FieldLeader3D_Managed20\\FieldLeader3D_Runtime.exe",nullptr,11,L"필드 부대 3D 조형","FieldLeader3D automatic runtime started","automatic runtime launch FAILED",L"3D 런타임 시작 실패",L"ModPackBridge.ini",L"ModPack",L"Field3D"},
  {L"SAN14Mods\\Minimap\\SAN14_FogTrial.dll",L"SAN14_FogTrial.dll",27,L"독립 미니맵(FogTrial)",nullptr,nullptr,nullptr,L"ExtBridge.ini",L"Ext",L"Minimap"},
  {L"SAN14Mods\\Ext\\SAN14_AISearch.dll",L"SAN14_AISearch.dll",26,L"2차 탐색 제한(AISearch)",nullptr,nullptr,nullptr,L"ExtBridge.ini",L"Ext",L"AISearch"},
  {L"SAN14Mods\\Ext\\SAN14_NoExecute.dll",L"SAN14_NoExecute.dll",26,L"처단불가(NoExecute)",nullptr,nullptr,nullptr,L"ExtBridge.ini",L"Ext",L"NoExecute"},
  {L"SAN14Mods\\Ext\\SAN14_RoninEscape.dll",L"SAN14_RoninEscape.dll",26,L"재야 갇힘 해소(RoninEscape)",nullptr,nullptr,nullptr,L"ExtBridge.ini",L"Ext",L"RoninEscape"},
  {L"SAN14Mods\\Ext\\SAN14_Population.dll",L"SAN14_Population.dll",28,L"인구 증가 보너스(Population)",nullptr,nullptr,nullptr,L"ExtBridge.ini",L"Ext",L"Population"},
  {L"SAN14Mods\\Ext\\SAN14_FastMenu.dll",L"SAN14_FastMenu.dll",30,L"메뉴 빠르게(FastMenu)",nullptr,nullptr,nullptr,L"ExtBridge.ini",L"Ext",L"FastMenu"},
 };
 for(const Extra&e:ex){
  if(!exists(e.rel))continue;
  if(e.ini){wchar_t ip[1100];ip[0]=0;r39::wcat(ip,1100,root);r39::wcat(ip,1100,L"SAN14Mods\\modules\\");r39::wcat(ip,1100,e.ini);if(!GetPrivateProfileIntW(e.sec,e.key,1,ip)||!GetPrivateProfileIntW(L"Mod",L"Enabled",1,ip))continue;}   // 꺼 둔 기능(기능 값 0 또는 연결 모듈 자체 Enabled=0)
  char ev[160];ev[0]=0;
  if(e.dll&&!loaded(e.dll)){char fa[64];fa[0]=0;for(int i=0;e.dll[i]&&i<60;i++){fa[i]=(char)e.dll[i];fa[i+1]=0;}r39::scat(ev,sizeof ev,fa);r39::scat(ev,sizeof ev," present on disk, not in process");add(e.feature,e.name,L"파일은 있는데 프로세스에 로드되지 않음(연결 모듈이 안 실었거나 그 DLL 이 스스로 꺼짐)",ev);continue;}
  if(e.rt_ok){if(!rt){add(e.feature,e.name,L"runtime.log 를 못 읽음","runtime.log missing");continue;}
   if(R(e.rt_ok))continue;
   if(e.rt_bad&&R(e.rt_bad)){add(e.feature,e.name,e.bad_text,e.rt_bad);continue;}
   r39::scat(ev,sizeof ev,"runtime.log(this session) lacks '");r39::scat(ev,sizeof ev,e.rt_ok);r39::scat(ev,sizeof ev,"'");add(e.feature,e.name,L"성공 기록이 없음(시작 도중 멈춤 또는 게임 판/다른 모드와 충돌)",ev);}
 }
}
// ---- ⑤ 자기 기록 파일에만 실패를 적는 모듈(시험8). 확인 때 한 번씩만 읽음(계속 읽지 않음).
static FILETIME proc_ft;static unsigned long long th_base;static bool th_base_set;
static const wchar_t kTraitHoverLog[]=L"SAN14_TraitHover.log";
struct FreshLog{const wchar_t*file;int feature;const wchar_t*name;int need1;int need2;};   // need1/need2: 이 기록을 볼 설치 항목(둘 중 하나라도 깔려 있어야)
static const FreshLog kFresh[]={   // 실행마다 새로 쓰는 기록
 {L"SAN14_Traits9.log",9,L"개성 9칸",9,13},{L"SAN14_Traits9_UI.log",9,L"개성 9칸 화면",9,13},
 {L"SAN14_MinimapStartup.log",27,L"독립 미니맵",27,0},
 {L"SAN14_DragGive.log",30,L"명품 다중 수여",30,0},{L"SAN14_DragLearn.log",30,L"전법서 다중 습득",30,0},{L"SAN14_WheelRegion.log",30,L"휠클릭 지역내정",30,0},
};
static char olbuf[65536];
static bool file_info(const wchar_t*rel,unsigned long long*size,FILETIME*mt){wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,rel);
 WIN32_FIND_DATAW a;HANDLE h=FindFirstFileW(p,&a);if(h==INVALID_HANDLE_VALUE)return false;FindClose(h);*size=((unsigned long long)a.nFileSizeHigh<<32)|a.nFileSizeLow;*mt=a.ftLastWriteTime;return true;}
static size_t read_from(const wchar_t*rel,unsigned long long from,char*buf,size_t cap){wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,rel);
 HANDLE f=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return 0;
 LARGE_INTEGER sz;if(!GetFileSizeEx(f,&sz)){CloseHandle(f);return 0;}unsigned long long s=(unsigned long long)sz.QuadPart;if(from>s)from=0;if(s-from>cap)from=s-cap;
 LARGE_INTEGER mv;mv.QuadPart=(long long)from;SetFilePointerEx(f,mv,0,0);DWORD n=0;ReadFile(f,buf,(DWORD)cap,&n,0);CloseHandle(f);return n;}
static void state_path(wchar_t*p,size_t cap){p[0]=0;r39::wcat(p,cap,root);r39::wcat(p,cap,L"SAN14_\xAE30\xB85D\\Diag_state.ini");}
static void ownlog_init(){
 proc_ft=since_ft;   // 이번 실행 기준 = Diag 시작 − 60초(게임 시작 직후라 앞서 쓴 기록도 포함; 새 가져오기 함수 없이)
 unsigned long long s=0;FILETIME mt{};th_base_set=false;
 if(file_info(kTraitHoverLog,&s,&mt)){
  if(!newer(mt,proc_ft)){th_base=s;th_base_set=true;}   // 이번 실행에 아직 안 씀 → 지금 크기 뒤가 이번 실행
  else{wchar_t sp[1100];state_path(sp,1100);wchar_t v[32];v[0]=0;GetPrivateProfileStringW(L"OwnLog",L"TraitHover",L"",v,32,sp);
   unsigned long long b=0;bool any=false;for(int i=0;v[i]>=L'0'&&v[i]<=L'9';i++){b=b*10+(unsigned)(v[i]-L'0');any=true;}
   th_base=(any&&b<=s)?b:s;th_base_set=true;}
 }
}
static void ownlog_save(){unsigned long long s=0;FILETIME mt{};if(!file_info(kTraitHoverLog,&s,&mt))return;
 wchar_t sp[1100];state_path(sp,1100);wchar_t v[32];v[0]=0;char t[24]={};r39::snum(t,sizeof t,(unsigned)(s>0xffffffffull?0xffffffffull:s));r39::wcat_a(v,32,t);
 WritePrivateProfileStringW(L"OwnLog",L"TraitHover",v,sp);}
static unsigned n_ownlog;
static void check_ownlogs(){
 static ownlog::Hit h[40];unsigned k=0;n_ownlog=0;
 // 통합편의(이미 읽어 둔 꼬리)
 if(loaded(L"SAN14_IntegratedConvenience.dll")&&ic_fresh){// 시험14: 모드 설정 창(San14ModSettings.ini)에서 끈 통합편의 기능은 실패 줄이 있어도 문제로 안 셈
   wchar_t ms[1100];ms[0]=0;r39::wcat(ms,1100,root);r39::wcat(ms,1100,L"San14ModSettings.ini");
   struct OffKey{const wchar_t*sec;const wchar_t*key;const wchar_t*w1;const wchar_t*w2;const wchar_t*w3;};
   static const OffKey kOff[]={
    {L"Modules",L"Traits9Module",L"개성 9개",nullptr,nullptr},{L"Modules",L"ModelRegistryModule",L"일기토",L"고유무장",nullptr},
    {L"Modules",L"BgmExpansionModule",L"BGM",nullptr,nullptr},{L"BgmPlayer",L"Enabled",L"BGM 플레이어",nullptr,nullptr},
    {L"Convenience",L"ProgressSpeedModule",L"진행속도",nullptr,nullptr},{L"Convenience",L"FormationMoveModule",L"대열 이동",nullptr,nullptr},
    {L"Convenience",L"AutoPersonnelModule",L"자동인사",L"자동 투자",nullptr},{L"Convenience",L"AutoPersonnel",L"자동인사",nullptr,nullptr},
    {L"Convenience",L"DualCredit",L"논공 동시",nullptr,nullptr},{L"Camera",L"AutoFocus",L"자동 포커스",L"카메라",nullptr},
    {L"Convenience",L"SkipReports",L"행동보고",L"수지보고",nullptr},{L"Convenience",L"ScenicNoFade",L"명승",nullptr,nullptr},
    {L"Convenience",L"CityNoFade",L"도시 증축",nullptr,nullptr},{L"Convenience",L"RecruitmentGate",L"등용문",nullptr,nullptr},
    {L"Convenience",L"PrisonerPersuasion",L"포로",nullptr,nullptr},{L"Convenience",L"AudienceButton",L"문전박대",nullptr,nullptr},
    {L"UnitFollow",L"Enabled",L"수행",nullptr,nullptr}};
   const wchar_t*off[64];unsigned noff=0;
   for(const OffKey&o:kOff)if(GetPrivateProfileIntW(o.sec,o.key,1,ms)==0){const wchar_t*w[3]={o.w1,o.w2,o.w3};for(const wchar_t*x:w)if(x&&noff<64)off[noff++]=x;}
   k=ownlog::scan_ic(iclog,iclog_n,since,loaded(L"SAN14_Traits9_UI.dll"),h,40,k,exists(L"San14ModelRegistry.tsv"),off,noff);}   // 시험13: 10번 미설치면 일기토 연결 실패 줄 무시
 unsigned k_ic=k;
 for(const FreshLog&f:kFresh){unsigned long long s=0;FILETIME mt{};if(!file_info(f.file,&s,&mt)||!newer(mt,proc_ft))continue;
  // 사용자 11-08: 남은 옛 파일 때문이면 창을 숨기지 말고 원인(옛 파일)과 정리 방법을 알림 — 설치기 [선택 설치]가 기록 없이 남은 옛 판 파일을 지움
  if(!selected(f.need1)&&!(f.need2&&selected(f.need2))){size_t n=read_from(f.file,0,olbuf,sizeof olbuf-1);olbuf[n]=0;unsigned k0=k;k=ownlog::scan_ascii(olbuf,n,f.feature,f.name,false,nullptr,h,40,k);
   for(unsigned i=k0;i<k;i++){n_leftover++;add_u8(f.feature,f.name,L"설치 안 한 기능의 옛 판 파일이 게임 폴더에 남아 실행됨 → 설치기에서 '선택 설치'(또는 설치 갱신)를 한 번 누르면 정리됩니다. 기록",h[i].text,h[i].count);}
   k=k0;continue;}
  int feat=f.feature;const wchar_t*nm=f.name;if(feat==9&&sel_known&&!sel_on[9]&&sel_on[13]){feat=13;nm=f.file[11]==L'_'?L"개성 9칸 화면(13번 공용 부품)":L"개성 9칸(13번 공용 부품)";}
  size_t n=read_from(f.file,0,olbuf,sizeof olbuf-1);olbuf[n]=0;k=ownlog::scan_ascii(olbuf,n,feat,nm,false,nullptr,h,40,k);}
 unsigned k_fresh=k;
 if(th_base_set){unsigned long long s=0;FILETIME mt{};if(file_info(kTraitHoverLog,&s,&mt)&&s>th_base){size_t n=read_from(kTraitHoverLog,th_base,olbuf,sizeof olbuf-1);olbuf[n]=0;
   k=ownlog::scan_ascii(olbuf,n,14,L"개성·전법 마우스 설명",true,"duel skipped",h,40,k);}}
 bool f5=false,t9=false,f6=false;r43c::Owner o5{},o9{},o6{};U32 r5=0,r9=0,r6=0;
 for(unsigned i=0;i<k;i++){const wchar_t*what=i<k_ic?L"통합편의 기록(SAN14CE_11Patch.log)에 실패 줄":i<k_fresh?L"자기 기록(.log)에 실패 줄":L"자기 기록(SAN14_TraitHover.log)에 실패 줄";
  // 정식판 뒤(11-08 제보): 2번 자동인사 '상태 구조/비용 확인 실패' 는 그 턴만 건너뛰는 안전장치(같은 턴 두 번 불림·데모·빠른 턴 넘김 때 정상으로도 나옴)
  //  → 3번 이상이고 이번 실행에 성공한 턴('상태 완료')이 한 번도 없을 때만 문제
  if(i<k_ic&&find(h[i].text,r39::slen(h[i].text),"\xEA\xB5\xAC\xEC\xA1\xB0/\xEB\xB9\x84\xEC\x9A\xA9 \xED\x99\x95\xEC\x9D\xB8 \xEC\x8B\xA4\xED\x8C\xA8")&&(h[i].count<3||wfind(iclog,iclog_n,L"상태 완료")))continue;
  add_u8(h[i].feature,h[i].name,what,h[i].text,h[i].count);unsigned pi=nprob-1;
  // 정식판 뒤(11-08): 5번 수행·대열 실패 → 그 자리 주인 / 9번 화면 부품 HOVER_SKIPPED → 개성 칸 주인
  if(i<k_ic&&h[i].feature==5){if(!f5){f5=true;o5=scan_ic_follow(&r5);}note_owner(pi,o5,r5);}
  if(i<k_ic&&h[i].feature==6){if(!f6){f6=true;o6=scan_ic_speed(&r6);}note_owner(pi,o6,r6);}   // R43 11-08 진행 배속 제보
  if(i>=k_ic&&i<k_fresh&&find(h[i].text,r39::slen(h[i].text),"HOVER_SKIPPED")){if(!t9){t9=true;o9=scan_t9_slots(&r9);}note_owner(pi,o9,r9);}
 }
 n_ownlog=k;
}
static void run_checks(){nprob=0;nconf=0;load_logs();load_selection();n_leftover=0;icw=ic_waiting();check_registered();check_others();check_ownlogs();}
static void write_summary(const char*phase){
 char line[1536];line[0]=0;const size_t C=sizeof line;
 r39::scat(line,C,S14_PATCH_VER "D\t0\t");r39::scat(line,C,MOD);r39::scat(line,C,"\t");r39::stamp(line,C);r39::scat(line,C,"\t");r39::scat(line,C,phase);
 r39::scat(line,C,"\tregistered=");r39::snum(line,C,n_reg);r39::scat(line,C," loaded=");r39::snum(line,C,n_loaded);r39::scat(line,C," started=");r39::snum(line,C,n_started);r39::scat(line,C," problems=");r39::snum(line,C,nprob);
 for(unsigned i=0;i<nprob&&i<48;i++){r39::scat(line,C,"\t");size_t n=r39::slen(line);
#ifdef S14_TEST_SHIM
  for(const wchar_t*q=probs[i].text;*q&&n+1<C;q++)line[n++]=(char)(*q<128?*q:'_');line[n]=0;
#else
  int k=WideCharToMultiByte(CP_UTF8,0,probs[i].text,-1,line+n,(int)(C-n-1),0,0);if(k>0)line[n+k-1]=0;else line[n]=0;
#endif
 }
 r39::append_line(line);
}
// 큰 제목(hd)·원인 단락 + 본문(m) → full
static void conflict_text(wchar_t*hd,size_t hc,wchar_t*full,size_t fc,const wchar_t*m){
 hd[0]=0;full[0]=0;unsigned nf=0,no=0;for(unsigned i=0;i<nconf;i++){if(confs[i].o.kind==r43c::K_FOREIGN)nf++;else if(confs[i].o.kind==r43c::K_OURS)no++;}
 if(nf){r39::wcat(hd,hc,L"다른 모드와 충돌 — ");unsigned c=0;for(unsigned i=0;i<nconf&&c<3;i++)if(confs[i].o.kind==r43c::K_FOREIGN){if(c++)r39::wcat(hd,hc,L", ");r39::wcat(hd,hc,confs[i].o.name);}}
 else if(no){r39::wcat(hd,hc,L"통합패치 기능끼리 겹침 — ");for(unsigned i=0;i<nconf;i++)if(confs[i].o.kind==r43c::K_OURS){r39::wcat(hd,hc,confs[i].o.name);break;}r39::wcat(hd,hc,L" (통합패치 문제, 제보 부탁)");}
 else r39::wcat(hd,hc,L"다른 모드와 충돌 가능성 높음 (바꾼 파일은 못 찾음)");
 r39::wcat(full,fc,L"■ 원인\r\n");
 for(unsigned i=0;i<nconf;i++){const Conf&c=confs[i];r39::wcat(full,fc,L"  · ");r39::wcat(full,fc,c.what);r39::wcat(full,fc,L" exe+0x");{char h[16]={};r39::shex(h,sizeof h,c.rva);r39::wcat_a(full,fc,h);}r39::wcat(full,fc,L": ");
  if(c.o.kind==r43c::K_FOREIGN){r39::wcat(full,fc,c.o.name);r39::wcat(full,fc,L" (다른 모드) 이(가) 먼저 고침");}
  else if(c.o.kind==r43c::K_OURS){r39::wcat(full,fc,c.o.name);r39::wcat(full,fc,L" (통합패치 파일) 이(가) 먼저 고침 — 통합패치 문제");}
  else r39::wcat(full,fc,L"원래 모양과 다름(바꾼 파일 못 찾음)");
  r39::wcat(full,fc,L"\r\n");}
 if(nf)r39::wcat(full,fc,L"  → 통합패치 고장이 아닙니다. 그 모드를 빼거나 겹치는 기능을 끄면 됩니다(또는 그 모드 쪽에서 맞춰야 함).\r\n");
 r39::wcat(full,fc,L"\r\n");r39::wcat(full,fc,m);
}
static DWORD WINAPI box_thread(void*t){MessageBoxW(0,(const wchar_t*)t,L"통합패치 " S14_PATCH_VERW L" 종합 진단",0x30|0x40000|0x10000);return 0;}
static DWORD WINAPI diag_main(void*){
 Sleep((DWORD)delay1*1000);
 run_checks();write_summary("check30");
 if(!nprob){ownlog_save();return 0;}
 if(delay2>delay1){Sleep((DWORD)(delay2-delay1)*1000);run_checks();write_summary("check90");ownlog_save();if(!nprob)return 0;}
 // 창
 static wchar_t m[8192];m[0]=0;const size_t W=8192;U32 ts=0,sz=0;r39::exe_info(&ts,&sz);
 r39::wcat(m,W,L"[통합패치 " S14_PATCH_VERW L"] 시작 종합 진단 — 문제 ");wnum(m,W,nprob);r39::wcat(m,W,L"건 (게임 시작 뒤 ");wnum(m,W,(unsigned)delay2);r39::wcat(m,W,L"초 기준)\r\n");
 r39::wcat(m,W,L"게임 실행 파일: 빌드 시각 0x");{char t[16]={};r39::shex(t,sizeof t,ts);r39::wcat_a(m,W,t);}r39::wcat(m,W,L" 크기 0x");{char t[16]={};r39::shex(t,sizeof t,sz);r39::wcat_a(m,W,t);}
 r39::wcat(m,W,(ts==S14_GAME_STAMP&&sz==S14_GAME_IMAGE)?L" (= 통합패치 기준판)":L" (≠ 통합패치 기준판!)");
 r39::wcat(m,W,L"\r\n설치한 통합패치 판: ");if(inst_ver[0])r39::wcat_a(m,W,inst_ver);else r39::wcat(m,W,L"(설치 기록 없음)");
 r39::wcat(m,W,L"\r\n등록 모듈 ");wnum(m,W,n_reg);r39::wcat(m,W,L"개 중 로드 ");wnum(m,W,n_loaded);r39::wcat(m,W,L"개, 시작 확인 ");wnum(m,W,n_started);r39::wcat(m,W,L"개\r\n\r\n문제:");
 for(unsigned i=0;i<nprob;i++){r39::wcat(m,W,L"\r\n ");wnum(m,W,i+1);r39::wcat(m,W,L") ");r39::wcat(m,W,probs[i].text);}
 {unsigned nf=0;size_t at0=r39::wlen(m);r39::wcat(m,W,L"\r\n\r\n이번 실행에서 이미 따로 오류창을 띄운 실패(R43F):");collect_r43f(m,W,&nf);if(!nf)m[at0]=0;}
 r39::wcat(m,W,L"\r\n\r\n기록: 게임 폴더\\SAN14_기록\\적용확인.log (R43D·R43F 줄) · %LOCALAPPDATA%\\SAN14CE_ModCore\\runtime.log · 게임 폴더\\SAN14CE_11Patch.log(통합편의) · SAN14_TraitHover.log · SAN14_Traits9*.log · SAN14_MinimapStartup.log · SAN14_Drag*.log · SAN14_WheelRegion.log");
 r39::wcat(m,W,L"\r\n진단 코드: " S14_PATCH_VERW L"D-");wnum(m,W,nprob);
 r39::wcat(m,W,L"\r\n\r\n이 창에서 Ctrl+C 를 누르면 글자가 복사됩니다. 그대로 붙여 알려 주세요. 게임은 계속 진행됩니다.");
 // 정식판 뒤(11-08): 다른 모드와 충돌이 보이면 맨 위 큰 제목 + 원인 단락
 if(nconf){static wchar_t hd[300];static wchar_t full[9000];conflict_text(hd,300,full,9000,m);
  r43c::show(L"통합패치 " S14_PATCH_VERW L" 종합 진단 — 다른 모드와 충돌",hd,full,0x30|0x40000|0x10000);return 0;}
 if(n_leftover){r43c::show(L"통합패치 " S14_PATCH_VERW L" 종합 진단 — 옛 파일 남음",L"설치 안 한 기능의 옛 판 파일이 남아 있음 — 설치기에서 '선택 설치'(또는 설치 갱신)를 한 번 누르면 정리됩니다",m,0x30|0x40000|0x10000);return 0;}
 HANDLE t=CreateThread(0,0,box_thread,m,0,0);if(t)CloseHandle(t);
 return 0;
}
extern "C" __declspec(dllexport) int SAN14_DiagState(int which){if(which==0)return (int)nprob;if(which==1)return (int)n_reg;if(which==2)return (int)n_loaded;if(which==3)return (int)n_started;return -1;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,0,MOD))return 0;core=api;game=api->resolved->image;
 if(!r39::game_root(root,1024)){r39::fail(0,MOD,"root","SAN14_ModInitV1 -> r39::game_root","game folder path","failed",L"종합 진단이 이번 실행에서 안 돌아감");return 0;}
 {wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,L"SAN14Mods\\modules\\Diag.ini");enabled=GetPrivateProfileIntW(L"Diag",L"Enabled",1,p)?1:0;
  delay1=(int)GetPrivateProfileIntW(L"Diag",L"FirstCheckSec",30,p);delay2=(int)GetPrivateProfileIntW(L"Diag",L"FinalCheckSec",90,p);if(delay1<5)delay1=5;if(delay2<delay1)delay2=delay1;}
 // 이번 실행 기준 시각(모듈 시작 줄 비교용): 지금 − 60초(먼저 시작한 모듈의 줄도 포함)
 {SYSTEMTIME t;GetLocalTime(&t);
  // 60초 빼기는 FILETIME 으로
  GetSystemTimeAsFileTime(&since_ft);unsigned long long v=((unsigned long long)since_ft.dwHighDateTime<<32)|since_ft.dwLowDateTime;v-=60ull*10000000ull;since_ft.dwLowDateTime=(DWORD)v;since_ft.dwHighDateTime=(DWORD)(v>>32);
  // 문자열 시각(현지)은 분 단위로 1분 빼서 근사(초 자리는 00)
  int mi=(int)t.wMinute-1,hh=t.wHour,dd=t.wDay,mo=t.wMonth,yy=t.wYear;if(mi<0){mi=59;hh--;if(hh<0){hh=23;dd--;if(dd<1){mo--;if(mo<1){mo=12;yy--;}dd=28;}}}
  since[0]=0;r39::snum(since,20,(unsigned)yy,4);r39::scat(since,20,"-");r39::snum(since,20,(unsigned)mo,2);r39::scat(since,20,"-");r39::snum(since,20,(unsigned)dd,2);r39::scat(since,20," ");r39::snum(since,20,(unsigned)hh,2);r39::scat(since,20,":");r39::snum(since,20,(unsigned)mi,2);r39::scat(since,20,":00");}
 #ifndef S14_TEST_SHIM
 if(perfw::start(root)&&core->Log)core->Log("Diag R43: perf sampler ON (SAN14_Minimap.ini Profile=1) -> SAN14_기록\\Diag_Perf.log");
 #endif
 ownlog_init();   // 시험8: 이어 쓰는 기록(TraitHover)의 이번 실행 시작 자리
 if(!enabled){r39::start(0,MOD,"OFF",0,0,"Enabled=0","-");return 1;}
 HANDLE h=CreateThread(nullptr,0,diag_main,nullptr,0,nullptr);
 if(!h){r39::fail(0,MOD,"thread","SAN14_ModInitV1 -> CreateThread(diag_main)","thread handle","NULL",L"종합 진단이 이번 실행에서 안 돌아감");return 0;}
 CloseHandle(h);
 {char st[48];st[0]=0;r39::scat(st,sizeof st,"FirstCheckSec=");r39::snum(st,sizeof st,(unsigned)delay1);r39::scat(st,sizeof st," FinalCheckSec=");r39::snum(st,sizeof st,(unsigned)delay2);r39::start(0,MOD,"OK",1,1,st,"-");}
 if(core->Log)core->Log("Diag R43: start-up diagnosis scheduled (read-only; window only when a module is missing/failed).");
 return 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
