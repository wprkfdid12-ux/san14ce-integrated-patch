#pragma once
// R39 공통 적용 확인 기록: 게임 폴더\SAN14_기록\적용확인.log (형식: 기록형식.md). CRT 없이 동작.
// R43: 실패 보고 공통 부품(r39::fail / check_api / fail_plan) — 모듈이 못 돌면 반드시 자세한 창 + R43F 줄을 남긴다(조용히 쉬지 않는다).
#include "common.h"
#include "r43_conflict.h"
namespace r39{
inline size_t slen(const char*s){size_t n=0;while(s[n])n++;return n;}
inline size_t wlen(const wchar_t*s){size_t n=0;while(s[n])n++;return n;}
inline void scat(char*o,size_t cap,const char*s){size_t n=slen(o);while(*s&&n+1<cap)o[n++]=*s++;o[n]=0;}
inline void wcat(wchar_t*o,size_t cap,const wchar_t*s){size_t n=wlen(o);while(*s&&n+1<cap)o[n++]=*s++;o[n]=0;}
inline void snum(char*o,size_t cap,unsigned v,int w=0){char b[12];int k=0;do{b[k++]=(char)('0'+v%10);v/=10;}while(v);while(k<w)b[k++]='0';char t[13];int j=0;while(k)t[j++]=b[--k];t[j]=0;scat(o,cap,t);}
inline void shex(char*o,size_t cap,unsigned long long v,int w=0){static const char*d="0123456789abcdef";char b[17];int k=0;do{b[k++]=d[v&15];v>>=4;}while(v);while(k<w)b[k++]='0';char t[18];int j=0;while(k)t[j++]=b[--k];t[j]=0;scat(o,cap,t);}
inline void sbytes(char*o,size_t cap,const U8*p,size_t n){for(size_t i=0;i<n;i++){if(i)scat(o,cap," ");shex(o,cap,p[i],2);}}
// ASCII 글자를 넓은 글자 뒤에 덧붙임(기록용 좁은 글자 → 창용)
inline void wcat_a(wchar_t*o,size_t cap,const char*s){size_t n=wlen(o);while(*s&&n+1<cap){unsigned char c=(unsigned char)*s++;o[n++]=(wchar_t)(c<128?c:'?');}o[n]=0;}
inline bool game_root(wchar_t*out,size_t cap){
 DWORD n=GetModuleFileNameW(0,out,(DWORD)cap);if(!n||n>=cap-40)return false;
 while(n&&out[n-1]!=L'\\'&&out[n-1]!=L'/')n--;out[n]=0;return n>0;
}
inline void stamp(char*o,size_t cap){
 SYSTEMTIME t;GetLocalTime(&t);
 snum(o,cap,t.wYear,4);scat(o,cap,"-");snum(o,cap,t.wMonth,2);scat(o,cap,"-");snum(o,cap,t.wDay,2);scat(o,cap," ");
 snum(o,cap,t.wHour,2);scat(o,cap,":");snum(o,cap,t.wMinute,2);scat(o,cap,":");snum(o,cap,t.wSecond,2);
}
inline void append_line(const char*line){
 wchar_t path[1024];if(!game_root(path,1024))return;
 wcat(path,1024,L"SAN14_\xAE30\xB85D");CreateDirectoryW(path,0);
 wcat(path,1024,L"\\\xC801\xC6A9\xD655\xC778.log");
 HANDLE f=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_ALWAYS,0x80,0);if(f==INVALID_HANDLE_VALUE)return;
 DWORD w;WriteFile(f,line,(DWORD)slen(line),&w,0);WriteFile(f,"\n",1,&w,0);CloseHandle(f);
}
// 시작 줄: R39\t번호\t모듈\t시각\t상태\t자리 n/m\t설정\t이유
inline void start(int num,const char*module,const char*state,unsigned ok,unsigned total,const char*settings,const char*reason){
 char line[1024];line[0]=0;const size_t C=sizeof line;
 scat(line,C,"R39\t");snum(line,C,(unsigned)num);scat(line,C,"\t");scat(line,C,module);scat(line,C,"\t");stamp(line,C);scat(line,C,"\t");scat(line,C,state);scat(line,C,"\t");
 snum(line,C,ok);scat(line,C,"/");snum(line,C,total);scat(line,C,"\t");scat(line,C,settings?settings:"-");scat(line,C,"\t");scat(line,C,reason?reason:"-");
 append_line(line);
}
// 카운터 줄: R39C\t번호\t모듈\t이름\t값\t시각
// 20차(L132, 사용자 11-11): 확인용 기록(R39C 셈 줄·달마다 쓰는 R42D 요약)은 본판에 넣지 않는다. 시험·확인판만 -DS14_DIAG_RECORDS=1 로 켬.
//  꺼져 있으면 아래 counter 와 s14_diag() 로 막은 기록은 빌드 결과에서 글자째 빠짐(관문 c12 가 본판 DLL 에서 확인).
//  남는 것: 모듈 시작 줄(R39 OK/FAIL), 오류 창, 실패·되돌림 같은 사건 기록.
#ifndef S14_DIAG_RECORDS
#define S14_DIAG_RECORDS 0
#endif
constexpr bool s14_diag(){return S14_DIAG_RECORDS!=0;}
#if S14_DIAG_RECORDS
__attribute__((used)) static const char s14_diag_on_marker[]="S14_DIAG_RECORDS_ON";   // 확인판 표시 — 본판 DLL 에 이 글자가 있으면 관문 c12 실패
#endif
inline void counter(int num,const char*module,const char*name,unsigned value){
#if S14_DIAG_RECORDS
 char line[512];line[0]=0;const size_t C=sizeof line;
 scat(line,C,"R39C\t");snum(line,C,(unsigned)num);scat(line,C,"\t");scat(line,C,module);scat(line,C,"\t");scat(line,C,name);scat(line,C,"\t");snum(line,C,value);scat(line,C,"\t");stamp(line,C);
 append_line(line);
#else
 (void)num;(void)module;(void)name;(void)value;
#endif
}
// 시작 실패 오류창(글자 Ctrl+C 복사 가능). 게임 창보다 위에 뜨도록 TOPMOST. (막힘: 누를 때까지 부른 스레드가 기다림)
inline void error_box(const wchar_t*text){MessageBoxW(0,text,L"통합패치 " S14_PATCH_VERW,0x30|0x40000);}

// ---- R43 실패 보고 ----
// 지금 실행 파일(SAN14CE_KO.exe) PE 머리: 빌드 시각·이미지 크기. 못 읽으면 false.
inline bool exe_info(U32*stamp,U32*size){
 const U8*img=(const U8*)GetModuleHandleW(0);if(!img||img[0]!='M'||img[1]!='Z')return false;
 U32 pe=at<U32>((void*)img,0x3c);if(pe<0x40||pe>0x1000||at<U32>((void*)img,pe)!=0x4550)return false;
 if(stamp)*stamp=at<U32>((void*)img,pe+8);if(size)*size=at<U32>((void*)img,pe+0x18+56);return true;
}
#ifndef S14_TEST_SHIM
inline DWORD WINAPI fail_box_thread(void*t){MessageBoxW(0,(const wchar_t*)t,L"통합패치 " S14_PATCH_VERW L" 오류",0x10|0x40000|0x10000);return 0;}
#endif
// 실패 보고: 창(막지 않음, TOPMOST, Ctrl+C 복사) + 기록 줄.
//  feature: 기능 번호(0=공통)  module: DLL 파일 이름  stage: 단계(ASCII 짧게: api/hook/thread/file/ic/version/data …)
//  where: 어느 소스 함수·게임 함수·주소(ASCII)  expected/actual: 기대·실제(ASCII)  effect: 사용자에게 보이는 영향(한글)
//  기록 줄: R43F\t번호\t모듈\t시각\t단계\t어디\t기대\t실제\t영향(UTF-8)\t진단코드\t게임(stamp/size)
//  진단 코드: R43F-<번호>-<단계>  (같은 모듈에서 같은 번호·단계는 창을 한 번만 띄움)
// 정식판 뒤(11-08): own = 실패 자리 주인 판정(r43_conflict.h). 다른 모드·통합패치끼리·바뀜이면 창 맨 위 큰 제목 + 원인 단락, 기록 줄 끝에 owner= 칸.
inline void fail_ex(int feature,const char*module,const char*stage,const char*where,const char*expected,const char*actual,const wchar_t*effect,const r43c::Owner*own){
 if(!module)module="?";if(!stage)stage="?";if(!where)where="-";if(!expected)expected="-";if(!actual)actual="-";if(!effect)effect=L"-";
 U32 ts=0,sz=0;bool have=exe_info(&ts,&sz);
 char code[64];code[0]=0;scat(code,sizeof code,S14_PATCH_VER "F-");snum(code,sizeof code,(unsigned)feature);scat(code,sizeof code,"-");scat(code,sizeof code,stage);
 char game[96];game[0]=0;
 if(have){scat(game,sizeof game,"stamp=0x");shex(game,sizeof game,ts);scat(game,sizeof game," size=0x");shex(game,sizeof game,sz);scat(game,sizeof game,(ts==S14_GAME_STAMP&&sz==S14_GAME_IMAGE)?" (=" S14_PATCH_VER " base)":" (!= base 0x");if(!(ts==S14_GAME_STAMP&&sz==S14_GAME_IMAGE)){shex(game,sizeof game,S14_GAME_STAMP);scat(game,sizeof game,"/0x");shex(game,sizeof game,S14_GAME_IMAGE);scat(game,sizeof game,")");}}
 else scat(game,sizeof game,"PE header unreadable");
 // 기록
 {char line[2048];line[0]=0;const size_t C=sizeof line;
  scat(line,C,S14_PATCH_VER "F\t");snum(line,C,(unsigned)feature);scat(line,C,"\t");scat(line,C,module);scat(line,C,"\t");stamp(line,C);scat(line,C,"\t");
  scat(line,C,stage);scat(line,C,"\t");scat(line,C,where);scat(line,C,"\t");scat(line,C,expected);scat(line,C,"\t");scat(line,C,actual);scat(line,C,"\t");
  {size_t n=slen(line);
#ifdef S14_TEST_SHIM
   for(const wchar_t*p=effect;*p&&n+1<C;p++)line[n++]=(char)(*p<128?*p:'_');line[n]=0;
#else
   int k=WideCharToMultiByte(CP_UTF8,0,effect,-1,line+n,(int)(C-n-1),0,0);if(k>0)line[n+k-1]=0;else line[n]=0;
#endif
  }
  scat(line,C,"\t");scat(line,C,code);scat(line,C,"\t");scat(line,C,game);
  if(own&&own->kind){char t[200];r43c::tag(*own,t,sizeof t);scat(line,C,"\t");scat(line,C,t);}
  append_line(line);}
 // 같은 (번호,단계)는 한 번만 창
 {static unsigned seen[32];static unsigned nseen;unsigned h=(unsigned)feature*131u;for(const char*p=stage;*p;p++)h=h*31u+(unsigned char)*p;
  for(unsigned i=0;i<nseen;i++)if(seen[i]==h)return;if(nseen<32)seen[nseen++]=h;}
 // 창 글자(한글은 소스 그대로 UTF-8 → clang 이 L"" 로 넓힘)
 static wchar_t bufs[4][3072];static volatile LONG next;LONG slot=(InterlockedIncrement(&next)-1)&3;wchar_t*m=bufs[slot];m[0]=0;const size_t W=3072;
 static wchar_t heads[4][200];wchar_t*hd=heads[slot];hd[0]=0;
 if(own&&own->kind){r43c::headline(*own,hd,200);r43c::explain(*own,m,W);}
 char fnum[12];fnum[0]=0;snum(fnum,sizeof fnum,(unsigned)feature);
 wcat(m,W,L"[통합패치 " S14_PATCH_VERW L"] 기능 ");wcat_a(m,W,fnum);wcat(m,W,L"번 적용 실패 — ");wcat_a(m,W,module);
 wcat(m,W,L"\r\n\r\n단계: ");wcat_a(m,W,stage);
 wcat(m,W,L"\r\n어디서: ");wcat_a(m,W,where);
 wcat(m,W,L"\r\n기대: ");wcat_a(m,W,expected);
 wcat(m,W,L"\r\n실제: ");wcat_a(m,W,actual);
 wcat(m,W,L"\r\n영향: ");wcat(m,W,effect);
 wcat(m,W,L"\r\n게임 실행 파일: ");wcat_a(m,W,game);
 wcat(m,W,L"\r\n\r\n진단 코드: ");wcat_a(m,W,code);
 wcat(m,W,L"\r\n\r\n이 창에서 Ctrl+C 를 누르면 글자가 복사됩니다. 그대로 붙여 알려 주세요.\r\n다른 기능은 그대로이고 게임은 계속 진행됩니다.");
#ifdef S14_TEST_SHIM
 if(hd[0])r43c::show(own->kind==r43c::K_OURS?L"통합패치 " S14_PATCH_VERW L" 오류":L"통합패치 " S14_PATCH_VERW L" — 다른 모드와 충돌",hd,m,0);else MessageBoxW(0,m,L"test",0);
#else
 if(hd[0]){r43c::show(own->kind==r43c::K_OURS?L"통합패치 " S14_PATCH_VERW L" 오류":L"통합패치 " S14_PATCH_VERW L" — 다른 모드와 충돌",hd,m,0x30|0x40000|0x10000);return;}
 HANDLE t=CreateThread(0,0,fail_box_thread,m,0,0);if(t)CloseHandle(t);else MessageBoxW(0,m,L"통합패치 " S14_PATCH_VERW L" 오류",0x10|0x40000|0x10000);
#endif
}
inline void fail(int feature,const char*module,const char*stage,const char*where,const char*expected,const char*actual,const wchar_t*effect){fail_ex(feature,module,stage,where,expected,actual,effect,nullptr);}
// 게임 실행 파일 자리(rva)가 기대(ex)와 다를 때 주인 판정. 게임 판이 기준과 다르면 판정 안 함(그건 게임 판 문제).
inline r43c::Owner site_owner(U32 rva,const U8*ex,const U8*ac,size_t n){
 r43c::Owner o{};o.kind=r43c::K_NONE;if(!rva||!n||!ac)return o;if(ex&&!memcmp(ex,ac,n))return o;
 U32 ts=0,sz=0;if(!exe_info(&ts,&sz)||ts!=S14_GAME_STAMP||sz!=S14_GAME_IMAGE)return o;
 size_t d0=0,d1=n;if(ex){while(d0<n&&ex[d0]==ac[d0])d0++;while(d1>d0&&ex[d1-1]==ac[d1-1])d1--;}
 const U8*img=(const U8*)GetModuleHandleW(nullptr);if(!img)return o;return r43c::owner_of_site(img+rva,n,d0,d1);
}
// 공통 부품(ModCore API) 확인 + 실패 보고. SAN14_ModInitV1 첫 줄에서 쓴다.
inline bool check_api(const S14_CoreAPI*a,int feature,const char*module){
 if(valid_api(a))return true;
 char ex[160],ac[224];ex[0]=ac[0]=0;
 scat(ex,sizeof ex,"size>=");snum(ex,sizeof ex,(unsigned)sizeof(S14_CoreAPI));scat(ex,sizeof ex," abi=0x");shex(ex,sizeof ex,S14_ABI);scat(ex,sizeof ex," image_size=0x");shex(ex,sizeof ex,S14_GAME_IMAGE);scat(ex,sizeof ex," Readable/InstallHooks/MakeRelay/FreeRelay set");
 if(!a)scat(ac,sizeof ac,"api=null (ModCore passed nothing)");
 else{
  scat(ac,sizeof ac,"size=");snum(ac,sizeof ac,a->size);scat(ac,sizeof ac," abi=0x");shex(ac,sizeof ac,a->abi);
  if(!a->resolved)scat(ac,sizeof ac," resolved=null");else{scat(ac,sizeof ac," image_size=0x");shex(ac,sizeof ac,a->resolved->image_size);if(a->resolved->profile){scat(ac,sizeof ac," profile=");scat(ac,sizeof ac,a->resolved->profile);}}
  if(!a->Readable)scat(ac,sizeof ac," Readable=null");if(!a->InstallHooks)scat(ac,sizeof ac," InstallHooks=null");if(!a->MakeRelay)scat(ac,sizeof ac," MakeRelay=null");if(!a->FreeRelay)scat(ac,sizeof ac," FreeRelay=null");
 }
 fail(feature,module,"api","SAN14_ModInitV1 <- SAN14_ModCore.dll",ex,ac,L"이 모듈은 이번 실행에서 꺼짐(게임 판이 통합패치 기준과 다르거나, 공통 부품 ModCore 연결 규격이 맞지 않음)");
 return false;
}
// 훅 계획 실패 보고: 어느 자리(RVA)·기대/실제 바이트까지 적는다.
inline void fail_plan(int feature,const char*module,const HookPlan&p,const char*what,const wchar_t*effect){
 char where[128],ex[96],ac[96];where[0]=ex[0]=ac[0]=0;
 scat(where,sizeof where,what?what:"hook");scat(where,sizeof where," @ exe+0x");shex(where,sizeof where,p.bad_rva);scat(where,sizeof where," (");scat(where,sizeof where,p.bad_why);scat(where,sizeof where,") planned=");snum(where,sizeof where,p.count);
 sbytes(ex,sizeof ex,p.bad_expected,p.bad_len);sbytes(ac,sizeof ac,p.bad_actual,p.bad_len);
 if(!p.bad_len){scat(ex,sizeof ex,"-");scat(ac,sizeof ac,"-");}
 r43c::Owner own=p.bad_len?site_owner(p.bad_rva,p.bad_expected,p.bad_actual,p.bad_len):r43c::Owner{};
 fail_ex(feature,module,"hook",where,ex,ac,effect,&own);
}
// 단순 바이트 대조 실패(HookPlan 없이 직접 읽은 자리)
inline void fail_bytes(int feature,const char*module,const char*what,U32 rva,const U8*expected,const U8*actual,size_t n,const wchar_t*effect){
 char where[128],ex[96],ac[96];where[0]=ex[0]=ac[0]=0;if(n>16)n=16;
 scat(where,sizeof where,what?what:"site");scat(where,sizeof where," @ exe+0x");shex(where,sizeof where,rva);
 sbytes(ex,sizeof ex,expected,n);if(actual)sbytes(ac,sizeof ac,actual,n);else scat(ac,sizeof ac,"unreadable");
 // 통합편의(IC) 안 자리(what 이 "IC" 로 시작)는 게임 실행 파일 자리가 아니라 판정 안 함
 r43c::Owner own=(what&&what[0]=='I'&&what[1]=='C')?r43c::Owner{}:site_owner(rva,expected,actual,n);
 fail_ex(feature,module,"bytes",where,ex,ac,effect,&own);
}
}
