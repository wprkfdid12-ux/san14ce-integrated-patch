// 정식판 뒤(11-08) '다른 모드와 충돌' 판정 시험(리눅스 네이티브, 흉내 메모리 지도).
//  빌드/실행: clang++ -std=c++17 -O1 -DS14_TEST_SHIM -Wno-everything -I test/shim -I src test/test_r43_conflict_native.cpp -o /tmp/t_conf && /tmp/t_conf
//  확인: ① 다른 모드 DLL 로 바로 jmp ② 1번 설정 창 제보 모양(jcc 목표만 바뀌어 게임 바깥 다리 코드 → FF25 → 다른 모드) ③ 통합패치 파일로 call
//        ④ 바이트만 바뀜(갈림 없음) ⑤ 게임 안으로만 가는 갈림 ⑥ 바뀐 범위 밖(뒤쪽 원래 바이트)에 우연히 E9 가 있어도 안 짚음
//        ⑦ 함수표 칸 주인(14번 TraitHover = 통합패치끼리) ⑧ 다리 코드가 빌린 명령 뒤 게임으로 복귀 + 모듈로 call ⑨ mov rax,imm64 / push-ret
//        ⑩ 판정 문장(큰 제목·원인 단락·기록 칸) ⑪ r39::fail_ex 가 큰 제목 창으로 띄우고 R43F 줄 끝에 owner= 를 남김
#include "common.h"
#include "r39_record.h"
#include <sys/mman.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
static U8*M;static const size_t MB=1u<<20;
static U8*EXE,*TR,*FOR,*OUR,*ICM;
static bool in(const void*p,const U8*b,size_t n){return (const U8*)p>=b&&(const U8*)p<b+n;}
static bool fake_readable(const void*p,size_t n){const U8*a=(const U8*)p;return a>=M&&a+n<=M+64*MB;}
static bool fake_module(const void*p,wchar_t*name,size_t cap,const U8**base){
 const wchar_t*nm=nullptr;const U8*b=nullptr;
 if(in(p,EXE,16*MB)){nm=L"SAN14CE_KO.exe";b=EXE;}else if(in(p,FOR,MB)){nm=L"AI_TacticsMod.dll";b=FOR;}else if(in(p,OUR,MB)){nm=L"SAN14_TraitHover.dll";b=OUR;}else if(in(p,ICM,MB)){nm=L"SAN14_IntegratedConvenience.dll";b=ICM;}
 if(!nm)return false;size_t k=0;for(;nm[k]&&k+1<cap;k++)name[k]=nm[k];name[k]=0;if(base)*base=b;return true;}
static std::vector<std::wstring> shown_main,shown_title;static std::vector<std::wstring> shown_body;
static void fake_show(const wchar_t*t,const wchar_t*m,const wchar_t*c){shown_title.push_back(t);shown_main.push_back(m?m:L"");shown_body.push_back(c?c:L"");}
static void put_rel(U8*at,U8 op,const U8*target){at[0]=op;int32_t r=(int32_t)(target-(at+5));memcpy(at+1,&r,4);}
static int fails=0;static void check(bool c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
static std::string nar(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');return s;}
// r39 기록·창 흉내
static std::string root_dir="/tmp/t_conf_root";static std::vector<std::wstring> boxes;
static std::string narrow_path(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?(*w=='\\'?'/':*w):'_');return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p=root_dir+"\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string s="mkdir -p '"+narrow_path(p)+"'";(void)!system(s.c_str());return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow_path(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=(HANDLE)1)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=8;}
extern "C" int MessageBoxW(void*,const wchar_t*t,const wchar_t*,unsigned){boxes.push_back(t);return 1;}
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){return n?nullptr:(HMODULE)EXE;}
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*)(void*),void*,DWORD,DWORD*){return nullptr;}
int main(){
 M=(U8*)mmap(nullptr,64*MB,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 EXE=M;TR=M+16*MB;FOR=M+20*MB;OUR=M+24*MB;ICM=M+28*MB;memset(M,0x90,64*MB);
 auto&h=r43c::th();h.readable=fake_readable;h.module_of=fake_module;h.show=fake_show;h.exe_lo=EXE;h.exe_hi=EXE+16*MB;
 printf("[1] 다른 모드 DLL 로 바로 jmp\n");
 {U8*s=EXE+0x184ff9;put_rel(s,0xE9,FOR+0x1234);r43c::Owner o=r43c::owner_of_site(s,5,0,5);check(o.kind==r43c::K_FOREIGN&&nar(o.name)=="AI_TacticsMod.dll"&&o.off==0x1234,"FOREIGN AI_TacticsMod.dll+0x1234");}
 printf("[2] 1번 설정 창 제보 모양: cmp 뒤 ja 목표만 게임 바깥 다리 코드로 → FF25 → 다른 모드\n");
 {U8*s=EXE+0x3ec920;U8 orig[9]={0x83,0xfa,0x3c,0x0f,0x87,0xb6,0x02,0x00,0x00};memcpy(s,orig,9);
  U8*t=TR+0x100;int32_t r=(int32_t)(t-(s+9));memcpy(s+5,&r,4);              // ja → 다리
  t[0]=0xFF;t[1]=0x25;int32_t z=0;memcpy(t+2,&z,4);uint64_t dst=(uint64_t)(FOR+0x777);memcpy(t+6,&dst,8);   // jmp [rip+0] → 다른 모드
  U8 now[9];memcpy(now,s,9);size_t d0=0,d1=9;while(d0<9&&orig[d0]==now[d0])d0++;while(d1>d0&&orig[d1-1]==now[d1-1])d1--;
  r43c::Owner o=r43c::owner_of_site(s,9,d0,d1);check(d0==5&&o.kind==r43c::K_FOREIGN&&nar(o.name)=="AI_TacticsMod.dll","jcc 목표 → 다리 → FOREIGN");}
 printf("[3] 통합패치 파일로 call\n");
 {U8*s=EXE+0x500000;put_rel(s,0xE8,OUR+0x50);r43c::Owner o=r43c::owner_of_site(s,5,0,5);check(o.kind==r43c::K_OURS&&nar(o.name)=="SAN14_TraitHover.dll","OURS SAN14_TraitHover.dll");}
 printf("[4] 바이트만 바뀜(갈림 없음)\n");
 {U8*s=EXE+0x510000;U8 b[6]={0x48,0x31,0xc0,0x90,0x90,0xc3};memcpy(s,b,6);r43c::Owner o=r43c::owner_of_site(s,6,0,6);check(o.kind==r43c::K_CHANGED,"CHANGED");}
 printf("[5] 게임 안으로만 가는 갈림\n");
 {U8*s=EXE+0x520000;put_rel(s,0xE9,EXE+0x600000);r43c::Owner o=r43c::owner_of_site(s,5,0,5);check(o.kind==r43c::K_CHANGED,"게임 안 → CHANGED");}
 printf("[6] 바뀐 범위 밖 뒤쪽 원래 바이트에 E9(다른 모드 쪽)가 있어도 안 짚음\n");
 {U8*s=EXE+0x530000;U8 b[5]={0x48,0x31,0xc0,0x90,0x90};memcpy(s,b,5);put_rel(s+5,0xE9,FOR+0x10);r43c::Owner o=r43c::owner_of_site(s,5,0,3);check(o.kind==r43c::K_CHANGED,"바뀐 3바이트만 봄 → CHANGED");}
 printf("[7] 함수표 칸 주인\n");
 {r43c::Owner o=r43c::owner_of_ptr(OUR+0x2dd0);check(o.kind==r43c::K_OURS&&nar(o.name)=="SAN14_TraitHover.dll","칸 → TraitHover(통합패치끼리)");
  r43c::Owner o2=r43c::owner_of_ptr(EXE+0x8ca1c0);check(o2.kind==r43c::K_CHANGED,"게임 안 다른 함수 → CHANGED");
  r43c::Owner o3=r43c::owner_of_ptr(FOR+0x40);check(o3.kind==r43c::K_FOREIGN,"다른 모드 함수 → FOREIGN");}
 printf("[8] 다리 코드: 빌린 명령 뒤 게임 복귀 jmp 가 먼저, 그 뒤 다른 모드로 call\n");
 {U8*s=EXE+0x540000;U8*t=TR+0x400;put_rel(s,0xE9,t);put_rel(t,0xE9,EXE+0x540005);put_rel(t+5,0xE8,FOR+0x99);
  r43c::Owner o=r43c::owner_of_site(s,5,0,5);check(o.kind==r43c::K_FOREIGN&&o.off==0x99,"복귀 갈래는 건너뛰고 FOREIGN");}
 printf("[9] mov rax,imm64 + jmp rax / push-ret\n");
 {U8*s=EXE+0x550000;s[0]=0x48;s[1]=0xB8;uint64_t v=(uint64_t)(FOR+0x300);memcpy(s+2,&v,8);s[10]=0xFF;s[11]=0xE0;r43c::Owner o=r43c::owner_of_site(s,12,0,12);check(o.kind==r43c::K_FOREIGN,"mov rax,imm64");
  U8*q=EXE+0x560000;uint64_t a=(uint64_t)(OUR+0x10);q[0]=0x68;U32 lo=(U32)a,hi=(U32)(a>>32);memcpy(q+1,&lo,4);q[5]=0xC7;q[6]=0x44;q[7]=0x24;q[8]=0x04;memcpy(q+9,&hi,4);q[13]=0xC3;
  r43c::Owner o2=r43c::owner_of_site(q,14,0,14);check(o2.kind==r43c::K_OURS,"push-ret → OURS");}
 printf("[10] 판정 문장\n");
 {r43c::Owner o{};o.kind=r43c::K_FOREIGN;const wchar_t*n=L"AI_TacticsMod.dll";for(int i=0;n[i];i++){o.name[i]=n[i];o.name[i+1]=0;}o.off=0x1234;
  wchar_t hd[200],ex[600];char tg[120];r43c::headline(o,hd,200);r43c::explain(o,ex,600);r43c::tag(o,tg,sizeof tg);
  check(std::wstring(hd)==L"다른 모드와 충돌 — AI_TacticsMod.dll","큰 제목: 다른 모드와 충돌 — 이름");
  check(std::wstring(ex).find(L"통합패치 고장이 아닙니다")!=std::wstring::npos,"원인 단락: 통합패치 고장 아님");
  check(std::string(tg)=="owner=foreign:AI_TacticsMod.dll+0x1234","기록 칸 owner=foreign:...");
  o.kind=r43c::K_OURS;r43c::headline(o,hd,200);check(std::wstring(hd).find(L"통합패치 기능끼리 겹침")==0,"OURS 제목");
  o.kind=r43c::K_CHANGED;r43c::headline(o,hd,200);check(std::wstring(hd).find(L"다른 모드와 충돌 가능성 높음")==0,"CHANGED 제목");}
 printf("[11] r39::fail_ex → 큰 제목 창 + R43F 줄 owner=\n");
 {(void)!system(("rm -rf "+root_dir+" && mkdir -p "+root_dir).c_str());
  r43c::Owner o{};o.kind=r43c::K_FOREIGN;const wchar_t*n=L"Other.dll";for(int i=0;n[i];i++){o.name[i]=n[i];o.name[i+1]=0;}o.off=0x10;
  shown_main.clear();boxes.clear();r39::fail_ex(1,"SAN14_ModSettings.dll","hook","sites @ exe+0x3ec920","83 fa","83 fb",L"새 설정 창 안 열림",&o);
  check(shown_main.size()==1&&shown_main[0]==L"다른 모드와 충돌 — Other.dll"&&boxes.empty(),"큰 제목 창 1번(지금 창 아님)");
  check(shown_title.size()==1&&shown_title[0].find(L"다른 모드와 충돌")!=std::wstring::npos,"창 제목: 다른 모드와 충돌");
  check(!shown_body.empty()&&shown_body[0].find(L"■ 원인: 다른 모드와 충돌")==0&&shown_body[0].find(L"단계: hook")!=std::wstring::npos,"본문: 원인 단락 + 원래 자세한 글자");
  std::string log;FILE*f=fopen((root_dir+"/SAN14___/____.log").c_str(),"rb");if(f){char b[4096];size_t k=fread(b,1,sizeof b-1,f);b[k]=0;log=b;fclose(f);}
  check(log.find("\towner=foreign:Other.dll+0x10")!=std::string::npos,"R43F 줄 끝 owner=foreign:Other.dll+0x10");
  shown_main.clear();boxes.clear();r39::fail(2,"X.dll","api","w","e","a",L"x");check(shown_main.empty()&&boxes.size()==1,"주인 없는 실패는 예전 창 그대로");}
 printf(fails?"\n실패 %d\n":"\n전부 통과\n",fails);return fails?1:0;
}
