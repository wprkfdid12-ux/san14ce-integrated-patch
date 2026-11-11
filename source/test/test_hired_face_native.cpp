// 32번 고용무장 얼굴 R42e 최적화 — 리눅스 네이티브 비교 시험(에뮬 없이).
//  같은 가짜 게임 상태·같은 호출 순서로 '최적화 전 소스'와 '지금 소스'를 각각 돌려, 모든 무장의 얼굴이 매 단계 똑같은지 비교한다.
//  빌드: clang++ -std=c++17 -O1 -w -I test/shim -I src -DHF_SRC='"<소스 경로>"' test/test_hired_face_native.cpp -o /tmp/hf && /tmp/hf
//  (test/run_hired_face_compare.sh 가 두 판을 빌드해 결과를 비교)
#include HF_SRC
#include <sys/mman.h>
#include <string>
#include <map>
#include <sys/stat.h>
extern "C"{void FaceBridge1(){}void FaceBridge2(){}void FaceBridge3(){}void FaceBridge4(){}}
static long long qpc;extern "C" BOOL QueryPerformanceCounter(LARGE_INTEGER*q){q->QuadPart=qpc;return 1;}
extern "C" BOOL QueryPerformanceFrequency(LARGE_INTEGER*q){q->QuadPart=10000000;return 1;}
static DWORD tid=7;extern "C" DWORD GetCurrentThreadId(){return tid;}
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p="/tmp/hf_root\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){mkdir(narrow(p).c_str(),0755);return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=3;}
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int d,const wchar_t*){return (unsigned)d;}
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){return 1;}
extern "C" HMODULE GetModuleHandleW(const wchar_t*){return nullptr;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
static unsigned rnd_s=12345;static unsigned rnd(){rnd_s=rnd_s*1103515245u+12345u;return (rnd_s>>8)&0xffffff;}
static U8*mk(unsigned id,unsigned face,unsigned st,unsigned gender){U8*o=(U8*)aligned_alloc(16,0x200);memset(o,0,0x200);at<U16>(o,0x10)=(U16)id;at<U16>(o,0xa6)=(U16)face;at<U8>(o,0xe6)=(U8)st;at<U8>(o,0xa9)=(U8)gender;return o;}
int main(){
 mkdir("/tmp/hf_root",0755);
 game=(U8*)mmap(0,0x2800000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE,-1,0);
 U8*db=(U8*)calloc(1,0x861a0+0x208*8+0x100);*(U8**)(game+0x2780f60)=db;U8**tab=(U8**)(db+0x158);U8**pool=(U8**)(db+0x861a0);
 // 무장 표: 일반 무장(원본 얼굴 일부 사용), 고용무장(4201~4400, 임시 얼굴 4201+), 죽은 무장
 for(unsigned id=1;id<=0x1770;id++){unsigned r=rnd()%100;if(r<40)continue;
  unsigned g=rnd()%3==0;unsigned st=r<85?1+rnd()%6:(rnd()%2?0:7);
  unsigned face=id>=4201&&id<=4400?id:(rnd()%5==0?2000+rnd()%167:100+rnd()%3000);
  tab[id]=mk(id,face,st,g);}
 for(unsigned i=0;i<0x208;i+=3){U8*o=mk(0,4201+rnd()%200,1,rnd()%2);at<U16>(o,0x15e)=0x4000;at<U8>(o,0x1c5)=1;pool[i]=o;}
 unsigned long long sig=1469598103934665603ull;
 auto dump=[&](int step){unsigned long long h=1469598103934665603ull;for(unsigned id=0;id<=0x1770;id++)if(tab[id]){h^=at<U16>(tab[id],0xa6)+id*65536ull;h*=1099511628211ull;}
  for(unsigned i=0;i<0x208;i++)if(pool[i]){h^=at<U16>(pool[i],0xa6)+i*7ull;h*=1099511628211ull;}printf("%d %016llx\n",step,h);sig^=h;};
 for(int step=0;step<400;step++){
  unsigned op=rnd()%10;
  if(op<6){ // 고용 후보 만들기: 한 번에 여러 명(3ms 안) 또는 띄엄띄엄
   int n=1+rnd()%6;for(int k=0;k<n;k++){
    unsigned id=4201+rnd()%200;U8*o;bool in=rnd()%3!=0;
    if(in&&tab[id]){o=tab[id];at<U8>(o,0xe6)=(U8)(1+rnd()%6);}else{o=mk(id,id,1,rnd()%2);if(in)tab[id]=o;}
    FaceAssign(o,rnd()%4==0?7800+rnd()%100:at<U16>(o,0xa6));
    qpc+=rnd()%4==0?100000:(rnd()%20000);}
  }else if(op<8){ // 다른 무장 상태 바뀜(죽음/등장/얼굴 바뀜)
   for(int k=0;k<30;k++){unsigned id=1+rnd()%0x1770;if(!tab[id])continue;unsigned r=rnd()%3;if(r==0)at<U8>(tab[id],0xe6)=0;else if(r==1)at<U8>(tab[id],0xe6)=1;else at<U16>(tab[id],0xa6)=(U16)(2000+rnd()%167);}
   qpc+=rnd()%2?5000:200000;
  }else if(op==8){FaceRescan();qpc+=100000;}
  else{tid=tid==7?8:7;qpc+=1000;}
  dump(step);
 }
 printf("SIG %016llx\n",sig);
#ifdef HF_NEW
 fprintf(stderr,"표 새로 만듦 %u, 다시 씀 %u, 배정 %u\n",SAN14_HiredFaceStats(0),SAN14_HiredFaceStats(1),SAN14_HiredFaceStats(2));
#endif
 return 0;
}
