// R40 A22 → R43: 게임 안 안내창(공통 부품이 게임을 못 알아봤을 때).
// ModCore 가 게임 확인(리졸버) 단계에서 멈추면 모듈이 하나도 안 켜져서 "모드 설정이 안 뜸 / 전부 시작 기록 없음"이 된다.
// 그때만 ModCore 의 실패 길에서 이 DLL 을 불러온다(정상 실행에서는 불리지 않음; 정상 실행의 종합 진단은 SAN14_Diag.dll).
// R43: 조용히 쉬지 않는다 — 기대/실제 판(빌드 시각·크기), 꺼진 모듈 목록(기능 번호), 진단 코드까지 창에 적고 R43E 줄을 남긴다. 게임 동작은 건드리지 않는다.
#include "r39_record.h"
#include "feature_table.h"

namespace {
const U8 kResolver[32]={0xc8,0x3c,0xe6,0xfc,0x07,0x9d,0x4e,0x76,0x33,0x93,0x80,0x59,0xd8,0x05,0x4b,0x97,0x8c,0x5a,0x25,0xac,0xe2,0xb2,0xfb,0x72,0x84,0xcb,0x1f,0x17,0x94,0xd9,0xd8,0x13};
const U32 kStamp101=S14_GAME_STAMP,kSize101=S14_GAME_IMAGE;   // 기준판(version.h)
const U32 kStampOld[]={0x6a6d9f72,0x6a983c25};                   // 옛 판: 1.0.0(2026-08-01), 1.0.1(2026-09-02)

bool file_sha(const wchar_t*path,U8 out[32]){
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE a=0;BCRYPT_HASH_HANDLE h=0;U8 buf[8192];DWORD n=0;bool ok=false;
 if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,0,0)>=0&&BCryptCreateHash(a,&h,0,0,0,0,0)>=0){
  ok=true;for(;;){if(!ReadFile(f,buf,sizeof buf,&n,0)){ok=false;break;}if(!n)break;if(BCryptHashData(h,buf,n,0)<0){ok=false;break;}}
  if(BCryptFinishHash(h,out,32,0)<0)ok=false;
 }
 if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);CloseHandle(f);return ok;
}
void whex(wchar_t*o,size_t cap,U32 v){char t[12]={};r39::shex(t,sizeof t,v);r39::wcat_a(o,cap,t);}
void wnum(wchar_t*o,size_t cap,unsigned v){char t[12]={};r39::snum(t,sizeof t,v);r39::wcat_a(o,cap,t);}
// 등록된(Enabled=1) 모듈 목록 → "번호 이름(파일)" 줄들. 돌아온 값 = 개수
unsigned enabled_modules(const wchar_t*root,wchar_t*out,size_t cap,char*log,size_t logcap){
 wchar_t pat[1100];pat[0]=0;r39::wcat(pat,1100,root);r39::wcat(pat,1100,L"SAN14Mods\\modules\\*.ini");
 WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE)return 0;unsigned n=0;
 do{
  if(fd.dwFileAttributes&0x10)continue;
  wchar_t p[1100];p[0]=0;r39::wcat(p,1100,root);r39::wcat(p,1100,L"SAN14Mods\\modules\\");r39::wcat(p,1100,fd.cFileName);
  if(GetPrivateProfileIntW(L"Mod",L"Enabled",0,p)!=1)continue;
  wchar_t file[128];file[0]=0;GetPrivateProfileStringW(L"Mod",L"File",L"",file,128,p);
  const FeatIni*f=feat_of_ini(fd.cFileName);n++;
  r39::wcat(out,cap,L"\r\n  ");if(f&&f->feature){wnum(out,cap,(unsigned)f->feature);r39::wcat(out,cap,L"번 ");}else r39::wcat(out,cap,L"공통 ");
  r39::wcat(out,cap,f?f->name:fd.cFileName);r39::wcat(out,cap,L" (");r39::wcat(out,cap,file[0]?file:fd.cFileName);r39::wcat(out,cap,L")");
  if(log){if(log[0])r39::scat(log,logcap,",");char a[64];a[0]=0;for(int i=0;file[i]&&i<60;i++)a[i]=(char)(file[i]<128?file[i]:'?'),a[i+1]=0;r39::scat(log,logcap,a[0]?a:"?");}
 }while(FindNextFileW(h,&fd));
 FindClose(h);return n;
}
}

// 실행 파일 머리(PE)로 종류를 가린다. 시험용으로도 내보냄.
// 0 = 지원판 그대로, 2 = 같은 판을 통째로 고친 실행 파일(신무장 1000명 등 exe 교체형), 3 = 지원판 이전, 4 = 다른 판(업데이트 등), 9 = 머리 못 읽음
extern "C" __declspec(dllexport) int SAN14_NoticeClassify(const U8*img,U32*stamp,U32*size){
 if(!img||img[0]!='M'||img[1]!='Z')return 9;
 U32 pe=at<U32>((void*)img,0x3c);if(pe<0x40||pe>0x1000||at<U32>((void*)img,pe)!=0x4550)return 9;
 U32 ts=at<U32>((void*)img,pe+8),sz=at<U32>((void*)img,pe+0x18+56);U16 nsec=at<U16>((void*)img,pe+6),opt=at<U16>((void*)img,pe+20);
 if(stamp)*stamp=ts;if(size)*size=sz;
 bool extra=false;const U8*sec=img+pe+24+opt;
 for(U16 i=0;i<nsec&&i<32;i++){const U8*s=sec+i*40;if(s[0]=='.'&&s[1]=='c'&&s[2]=='e'&&s[3]=='c'&&s[4]=='o'&&s[5]=='d'&&s[6]=='e')extra=true;}
 if(ts==kStamp101)return (sz==kSize101&&!extra)?0:2;
 for(U32 o:kStampOld)if(ts==o)return 3;
 return 4;
}

static DWORD WINAPI notice_main(void*){
 U32 ts=0,sz=0;int kind=SAN14_NoticeClassify((const U8*)GetModuleHandleW(0),&ts,&sz);
 wchar_t root[1024];if(!r39::game_root(root,1024))root[0]=0;
 wchar_t res[1100];res[0]=0;r39::wcat(res,1100,root);r39::wcat(res,1100,L"SAN14Mods\\resolvers\\SAN14_Resolver_IP01.dll");
 U8 d[32];bool have=GetFileAttributesW(res)!=0xffffffffu,ours=have&&file_sha(res,d)&&!memcmp(d,kResolver,32);
 // 게임 폴더의 다른 모드 로더(같이 깔린 모드 이름을 안내에 적음)
 static const wchar_t*proxies[]={L"dinput8.dll",L"version.dll",L"winmm.dll",L"d3d11.dll",L"dsound.dll",L"xinput1_3.dll",L"d3d9.dll",L"dxgi.dll"};
 wchar_t others[256];others[0]=0;
 for(const wchar_t*p:proxies){wchar_t f[1100];f[0]=0;r39::wcat(f,1100,root);r39::wcat(f,1100,p);if(GetFileAttributesW(f)!=0xffffffffu){if(others[0])r39::wcat(others,256,L", ");r39::wcat(others,256,p);}}
 // 코드: R43-MC<종류>-<타임스탬프>-<크기>
 int mc=kind==0?(!have?5:!ours?6:1):kind==9?4:kind;
 char code[64];code[0]=0;r39::scat(code,sizeof code,S14_PATCH_VER "-MC");r39::snum(code,sizeof code,(unsigned)mc);r39::scat(code,sizeof code,"-");r39::shex(code,sizeof code,ts);r39::scat(code,sizeof code,"-");r39::shex(code,sizeof code,sz);
 const wchar_t*why=L"",*fix=L"";
 switch(mc){
  case 1:why=L"다른 모드가 게임 코드를 먼저 바꿔서 통합패치 공통 부품(리졸버)이 게임 함수 지문을 못 찾았습니다.";fix=L"같이 깐 다른 모드(아래 로더 파일)를 잠시 빼고 실행해 보세요. 그래도 같으면 이 창 글자를 그대로 알려 주세요.";break;
  case 2:why=L"게임 실행 파일(SAN14CE_KO.exe)이 다른 패치로 통째로 바뀌어 있습니다(빌드 시각은 같은데 크기/구역이 다름: 신무장 1000명처럼 exe 를 바꾸는 패치).";fix=L"통합패치는 스팀 원본 실행 파일에서만 켜집니다. 스팀 [속성 → 설치된 파일 → 게임 파일 무결성 확인]으로 원본을 되돌리면 켜집니다(그 패치는 빠집니다).";break;
  case 3:why=L"게임 실행 파일이 통합패치 기준판보다 옛 판(1.0.1 또는 그 이전·롤백판)입니다.";fix=L"스팀에서 게임 파일 무결성 확인(최신판)을 한 뒤 실행해 주세요. 옛 판을 그대로 쓰려면 그 판에 맞는 옛 통합패치(R42i4 이하)를 쓰세요.";break;
  case 4:why=L"게임 실행 파일이 통합패치가 아는 판이 아닙니다(게임 업데이트 등). 통합패치는 기준판의 함수 주소·바이트를 쓰므로 다른 판에서는 스스로 전부 꺼집니다(튕김 방지).";fix=L"새 판에 맞춘 통합패치가 나올 때까지 기다려 주세요. 이 창 글자(기대/실제 판·코드)를 그대로 알려 주시면 그 판에 맞춰 고칩니다.";break;
  case 5:why=L"통합패치 공통 파일(SAN14Mods\\resolvers\\SAN14_Resolver_IP01.dll)이 없습니다.";fix=L"통합패치 설치기에서 [선택 설치]를 다시 눌러 주세요.";break;
  case 6:why=L"통합패치 공통 파일(SAN14_Resolver_IP01.dll)이 다른 패치의 것으로 바뀌어 있습니다(SHA-256 다름).";fix=L"다른 모드를 깐 뒤라면 통합패치 설치기에서 [선택 설치]를 다시 눌러 주세요.";break;
 }
 static wchar_t msg[6144];msg[0]=0;const size_t W=6144;
 r39::wcat(msg,W,L"[통합패치 " S14_PATCH_VERW L"] 이번 실행에서는 통합패치 모드가 하나도 켜지지 않았습니다.\r\n(모드 설정 창도 정보 메뉴에 나오지 않습니다. 게임은 모드 없이 그대로 진행됩니다.)\r\n\r\n이유: ");
 r39::wcat(msg,W,why);r39::wcat(msg,W,L"\r\n해결: ");r39::wcat(msg,W,fix);
 r39::wcat(msg,W,L"\r\n\r\n단계: SAN14_ModCore.dll → 리졸버(SAN14_Resolver_IP01.dll) 게임 판 확인");
 r39::wcat(msg,W,L"\r\n기대: 빌드 시각 0x");whex(msg,W,kStamp101);r39::wcat(msg,W,L" / 이미지 크기 0x");whex(msg,W,kSize101);r39::wcat(msg,W,L" (" S14_GAME_NAME L")");
 r39::wcat(msg,W,L"\r\n실제: 빌드 시각 0x");whex(msg,W,ts);r39::wcat(msg,W,L" / 이미지 크기 0x");whex(msg,W,sz);
 if(kind==9)r39::wcat(msg,W,L" (PE 머리를 못 읽음)");else if(kind==2)r39::wcat(msg,W,L" (빌드 시각 같음, 크기/구역 다름)");else if(kind==3)r39::wcat(msg,W,ts==0x6a983c25?L" (옛 판 1.0.1)":L" (옛 판 1.0.0)");
 r39::wcat(msg,W,L"\r\n리졸버 파일: ");r39::wcat(msg,W,!have?L"없음":ours?L"통합패치 것(해시 일치)":L"다른 것(해시 불일치)");
 if(others[0]){r39::wcat(msg,W,L"\r\n게임 폴더의 다른 로더 파일: ");r39::wcat(msg,W,others);}
 // 꺼진 모듈 목록
 char mods[1024];mods[0]=0;
 {size_t at0=r39::wlen(msg);r39::wcat(msg,W,L"\r\n\r\n이번 실행에서 꺼진 통합패치 모듈(등록 Enabled=1):");unsigned n=enabled_modules(root,msg,W,mods,sizeof mods);if(!n){msg[at0]=0;r39::wcat(msg,W,L"\r\n\r\n등록된 모듈 파일(SAN14Mods\\modules\\*.ini)을 못 찾음");}}
 r39::wcat(msg,W,L"\r\n\r\n진단 코드: ");r39::wcat_a(msg,W,code);
 r39::wcat(msg,W,L"\r\n(이 창에서 Ctrl+C 를 누르면 글자가 복사됩니다. 그대로 붙여 알려 주세요.)");
 // 기록: R43E\tMC\t시각\t코드\t기대stamp/size\t실제stamp/size\t로더파일\t꺼진모듈
 char line[2048];line[0]=0;const size_t C=sizeof line;
 r39::scat(line,C,S14_PATCH_VER "E\tMC\t");r39::stamp(line,C);r39::scat(line,C,"\t");r39::scat(line,C,code);
 r39::scat(line,C,"\texpect=0x");r39::shex(line,C,kStamp101);r39::scat(line,C,"/0x");r39::shex(line,C,kSize101);
 r39::scat(line,C,"\tactual=0x");r39::shex(line,C,ts);r39::scat(line,C,"/0x");r39::shex(line,C,sz);r39::scat(line,C,"\t");
 {char o[256];int i=0;for(;others[i]&&i<255;i++)o[i]=(char)others[i];o[i]=0;r39::scat(line,C,o[0]?o:"-");}
 r39::scat(line,C,"\t");r39::scat(line,C,mods[0]?mods:"-");
 r39::append_line(line);
 MessageBoxW(0,msg,L"통합패치 " S14_PATCH_VERW L" 안내",0x30|0x40000|0x10000);
 return 0;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE,DWORD reason,void*){
 if(reason==DLL_PROCESS_ATTACH){HANDLE t=CreateThread(0,0,notice_main,0,0,0);if(t)CloseHandle(t);}
 return TRUE;
}
