// 22번 플레이어 내정 자동(DomesticPlayer R42h) — 리눅스 네이티브 시험(에뮬 없이)
//  가짜 게임 메모리(지역 포인터 표를 일부러 연속 아니게 = 맵 확장 흉내)로 tick() 을 그대로 돌려,
//  ① 플레이어 지역만 손대고 ② 중점은 가장 낮은 단계로(R43: 1=상 2=농 3=병, 플레이어 선택 유지) ③ 인구<1만이면 1만 ④ 바뀔 때만 씀(멱등) ⑤ 개발값 이상한 지역은 건너뜀 을 확인.
//  빌드·실행: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_domestic_player_native.cpp -o /tmp/dp && /tmp/dp
#include "common.h"
#include <sys/mman.h>
#include <vector>
#include <string>
#include <cassert>
static DWORD WINAPI _ignore(void*){return 0;}
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*)(void*),void*,DWORD,DWORD*){return (HANDLE)1;} // 스레드 안 띄움(테스트가 tick 직접 호출)
extern "C" void Sleep(DWORD){}
#include "../src/domestic_player.cpp"
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*o,DWORD cap){std::string p="/tmp/dp_root\\x.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)o[n]=(wchar_t)p[n];o[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string c="mkdir -p '"+narrow(p)+"'";if(system(c.c_str())){}return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE&&h!=(HANDLE)1)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=4;}
static std::vector<std::string> ini_w;
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*sec,const wchar_t*k,const wchar_t*v,const wchar_t*f){ini_w.push_back(narrow(f)+":"+narrow(k)+"="+narrow(v));return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int d,const wchar_t*){return (unsigned)d;}
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){return 1;}
static U8 fake_ce[0x30000];static bool ce_loaded;
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){if(n&&ce_loaded&&narrow(n)=="SAN14_DomesticCE.dll")return (HMODULE)fake_ce;return nullptr;}
extern "C" DWORD GetFileAttributesW(const wchar_t*p){return narrow(p).find("SAN14_DomesticCE.ini")!=std::string::npos?0x20:0xffffffffu;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
static I32 rdbl(const void*p,UPTR n){return p!=nullptr;}
static S14_CoreAPI api;
static U8*mkarea(unsigned owner,unsigned com,unsigned ag,unsigned bar,unsigned foc,int pop){
 U8*a=(U8*)calloc(1,0x98);at<U8>(a,0x78)=(U8)owner;at<U16>(a,0x3c)=(U16)com;at<U16>(a,0x3e)=(U16)ag;at<U16>(a,0x40)=(U16)bar;at<U8>(a,0x72)=(U8)foc;at<U16>(a,0x52)=(U16)pop;at<U16>(a,0x54)=70;   /* +0x54 = 옆 칸(다른 값) — 인구를 고쳐도 그대로여야 함(P213) */at<U16>(a,0x36)=1;return a;
}
int main(){setvbuf(stdout,nullptr,_IONBF,0);
 if(system("mkdir -p /tmp/dp_root")){}
 // 순수 규칙(R43 11-08 바로잡음): 게임 판정 함수 0xb2af0 그대로 1=상업 2=농업 3=병영, 가장 낮은 단계(같으면 상>농>병), ≥1000 제외
 assert(SAN14_DomesticPlayerFocus(50,50,50)==1);    // 다 같은 단계 → 상업(코드1)  ← 옛 판은 2(=게임에선 농업)를 써서 제보 증상
 assert(SAN14_DomesticPlayerFocus(400,50,50)==2);   // 상 높음 → 농업(코드2)
 assert(SAN14_DomesticPlayerFocus(400,400,50)==3);  // 병 낮음 → 병영(코드3)
 assert(SAN14_DomesticPlayerFocus(1000,1000,1000)==0); // 다 만렙 → 없음(0)
 assert(SAN14_DomesticPlayerFocus(1000,50,700)==2);  // 상 만렙 제외, 농 낮음 → 농업
 // 가짜 게임: game, db, world, 지역 포인터 표(일부러 거꾸로 꽂아 비연속 = 맵 확장 흉내)
 game=(U8*)mmap(0,0x2a00000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE,-1,0);
 U8*d=(U8*)calloc(1,0x880000);*(U8**)(game+0x2780f60)=d;
 U8*w=(U8*)calloc(1,0x40);*(U8**)(d+0x871e8)=w;at<U8>(w,0x3a)=2;at<short>(w,0x34)=200;at<U8>(w,0x36)=5; // 플레이어=세력2, 200년 5월
 U8**tab=(U8**)(d+0x6caf0);
 // 지역 10개: 2,5,7,9,10 = 플레이어(세력2). 3 = 개발값 이상(건너뛰어야). 나머지 = 다른 세력.
 //  R43: 처음 볼 때는 중점이 쓸모없을 때(0·모르는 값·만렙 항목)만 씀 — 불러온 세이브의 플레이어 선택을 덮어쓰지 않음
 U8*ar[11]={0};
 ar[1]=mkarea(1, 50, 50, 50, 1, 5000);
 ar[2]=mkarea(2, 50, 50, 50, 0, 3000);   // 플레이어: 중점 없음 → 상업(1), 인구 3000→10000
 ar[3]=mkarea(2, 9999,50,50, 1, 3000);   // 플레이어지만 개발값 이상(9999) → 건너뜀
 ar[4]=mkarea(9, 50, 50, 50, 2, 3000);
 ar[5]=mkarea(2, 400,50,50, 3, 20000);   // 플레이어: 병영(3) 고른 상태, 병영 안 만렙 → 그대로(답은 농업이어도 안 바꿈), 인구 2만 그대로
 ar[6]=mkarea(1, 50, 50, 50, 1, 500);
 ar[7]=mkarea(2, 1000,400,50,1, 8000);   // 플레이어: 상업(1)인데 상업 만렙 → 병영(3), 인구 8000→10000
 ar[8]=mkarea(3, 50, 50, 50, 1, 100);
 ar[9]=mkarea(2, 1000,1000,1000,2,50000); // 플레이어지만 다 만렙 → 답 0(안 바꿈), 인구 그대로
 ar[10]=mkarea(2,50,50,50,1,12000);      // 플레이어: 상업(1) → 그대로(제보: 상업 고른 지역)
 for(unsigned i=1;i<=10;i++)tab[i]=ar[i]?(U8*)ar[i]:nullptr; // 각 칸은 포인터(비연속 — 객체는 calloc 이라 흩어져 있음)
 api.Readable=rdbl;core=&api;en_dom=1;en_pop=1;
 tick();
 auto foc=[&](int i){return (int)at<U8>(ar[i],0x72);};auto pop=[&](int i){return at<U16>(ar[i],0x52);};
 printf("1차: 2=%d pop=%d / 5=%d / 7=%d pop=%d / 3=%d / 9=%d / 10=%d\n",foc(2),pop(2),foc(5),foc(7),pop(7),foc(3),foc(9),foc(10));
 assert(foc(2)==1&&pop(2)==10000);
 assert(foc(5)==3&&pop(5)==20000);
 assert(foc(7)==3&&pop(7)==10000);
 assert(foc(3)==1&&pop(3)==3000);   // 개발값 이상 지역은 안 건드림
 assert(foc(9)==2);                  // 만렙 → 중점 안 바꿈
 assert(foc(10)==1&&pop(10)==12000); // 상업 고른 지역 그대로
 assert(foc(1)==1&&foc(4)==2&&foc(8)==1); // 남의 세력 지역 = 손 안 댐
 assert(pop(1)==5000&&pop(6)==500&&pop(8)==100);
 for(int i=1;i<=10;i++)assert(at<U16>(ar[i],0x54)==70);   // P213: 인구를 1만으로 올려도 옆 칸(+0x54)은 그대로
 { U8*q=mkarea(2,50,50,50,1,9812);at<U16>(q,0x54)=70;tab[11]=q;long pb9=t_pop;tick();assert(at<U16>(q,0x52)==10000&&at<U16>(q,0x54)==70&&t_pop==pb9+1);   // 제보 기록의 실제 값(9812, 옆 칸 70) = 옛 코드는 32비트 4597332 로 읽어 안 올림
   at<U16>(q,0x78)=0;tab[11]=nullptr;printf("통과: P213 인구 16비트 — 9812 → 10000, 옆 칸 그대로\n"); }
 unsigned f1=t_focus,p1=t_pop;
 tick();assert(t_focus==f1&&t_pop==p1); printf("2차: 멱등\n");
 // 제보 상황: 플레이어가 지역 2 를 상업→농업... 아니 직접 '병영'으로 바꿈 → 이후 틱·턴이 지나도 그대로
 at<U8>(ar[2],0x72)=3;for(int k=0;k<5;k++)tick();assert(foc(2)==3&&t_kept>=1);printf("플레이어가 바꾼 지역(2→병영) 유지, kept=%ld\n",t_kept);
 // 같은 지역이 다른 항목 개발로 단계가 바뀌어도(답 변화) 플레이어 선택 유지
 at<U16>(ar[2],0x3c)=150;tick();assert(foc(2)==3);
 // 고른 항목(병영)이 만렙이 되면 그때만 다음 것으로
 at<U16>(ar[2],0x40)=1000;tick();assert(foc(2)==2);printf("고른 병영 만렙 → 농업(2)으로\n");
 // 플레이어가 안 건드린 지역: 개발 단계가 바뀌어 답이 바뀌면 따라감(골고루)
 // 지역 10: 상업(1) 출발, 답=상업. 상업이 100 넘어 단계↑ → 답=농업(2) → 바꿈
 at<U16>(ar[10],0x3c)=120;tick();assert(foc(10)==2);printf("안 건드린 지역은 단계 바뀌면 골고루(상업→농업)\n");
 // 같은 단계 안에서는 그대로(턴마다 덮어쓰기 없음)
 at<U16>(ar[10],0x3e)=60;tick();assert(foc(10)==2);
 // 세이브 불러오기 흉내: 연월이 뒤로 → 기억 지움, 불러온 값(플레이어 선택) 그대로 출발
 at<U8>(ar[10],0x72)=1;at<U8>(w,0x36)=3;tick();assert(foc(10)==1);printf("불러오기 뒤 불러온 중점(상업) 유지\n");
 // 끔 시험
 en_dom=0;en_pop=0;at<U8>(ar[2],0x72)=0;tick();assert(foc(2)==0); // 꺼졌으면 안 건드림
 // R42i3: 플레이어 없음(세력 0, 데모·타이틀) → 아무것도 안 씀 + idle 기록 한 번
 en_dom=1;en_pop=1;for(int i=1;i<=10;i++){at<U8>(ar[i],0x78)=0;at<U8>(ar[i],0x72)=0;at<U16>(ar[i],0x52)=3000;}
 at<U8>(w,0x3a)=0;long fb=t_focus,pb=t_pop;tick();tick();
 bool none=true;for(int i=1;i<=10;i++)none=none&&at<U8>(ar[i],0x72)==0&&at<U16>(ar[i],0x52)==3000;
 assert(none&&t_focus==fb&&t_pop==pb&&t_idle>=2);
 // 게임 안 불러옴(연도 0) → 플레이어 번호가 있어도 아무것도 안 씀
 for(int i=1;i<=10;i++)at<U8>(ar[i],0x78)=2;at<U8>(w,0x3a)=2;at<short>(w,0x34)=0;tick();
 none=true;for(int i=1;i<=10;i++)none=none&&at<U8>(ar[i],0x72)==0&&at<U16>(ar[i],0x52)==3000;assert(none);
 // 값이 전부 0 인 빈 칸은 플레이어 소유여도 건너뜀
 at<short>(w,0x34)=200;at<U16>(ar[4],0x3c)=0;at<U16>(ar[4],0x3e)=0;at<U16>(ar[4],0x40)=0;at<U16>(ar[4],0x52)=0;at<U8>(ar[4],0x72)=0;tick();
 assert(at<U8>(ar[4],0x72)==0&&at<U16>(ar[4],0x52)==0);   // 빈 칸: 중점·인구 안 건드림
 assert(at<U8>(ar[2],0x72)==2&&at<U16>(ar[2],0x52)==10000); // 진짜 지역(중점 없음)은 다시 정상 처리(상업 150·농 50·병 1000 → 농업)
 // R43 11-08: 옛 DomesticCE 의 모든 내정(칸1)·최소 인구(칸2) 끄기 — 알려진 판일 때만, AI 명령서(칸3) 그대로
 { ce_loaded=true;volatile U32*f=(volatile U32*)(fake_ce+0x2d580);f[1]=1;f[2]=1;f[3]=1;
   ce_known=0;ce_quiet();assert(f[1]==1&&f[2]==1);              // 모르는 판 → 안 건드림
   ce_known=1;ce_quiet();assert(f[1]==0&&f[2]==0&&f[3]==1);     // 알려진 판 → 칸1·2 끔, 칸3 그대로
   f[1]=1;ce_quiet();assert(f[1]==0&&t_ce_off==3);              // 다시 켜지면 또 끔
   ini_w.clear();ce_ini_off();bool a1=false,p1=false,o1=false;for(auto&w:ini_w){if(w.find("DomesticCE.ini:AllDomestic=0")!=std::string::npos)a1=true;if(w.find("DomesticCE.ini:PopulationFloor=0")!=std::string::npos)p1=true;if(w.find("AiOrders")!=std::string::npos)o1=true;}
   assert(a1&&p1&&!o1);printf("통과: 옛 DomesticCE 플레이어 내정 끄기(칸1·2, ini), AI 명령서 그대로\n");ce_loaded=false; }
 // P206: 달 요약 한 줄(world) — 달이 바뀔 때 한 번, 읽기만, 22번 설정을 꺼도 남음
 { auto last_world=[&](int*count){std::string out;*count=0;FILE*f=popen("cat /tmp/dp_root/*/* 2>/dev/null","r");char buf[600];while(f&&fgets(buf,sizeof buf,f)){if(strstr(buf,"\tworld\t")){out=buf;(*count)++;}}if(f)pclose(f);return out;};
   en_dom=0;en_pop=0;at<U8>(w,0x3a)=2;at<short>(w,0x34)=201;at<U8>(w,0x36)=4;
   struct{unsigned o,com;int pop;}v[11]={{0,0,0},{1,50,5000},{2,50,10000},{2,9999,3000},{9,50,3000},{2,50,20000},{1,50,500},{2,50,8000},{3,50,100},{2,50,50000},{7,50,12000}};
   for(int i=1;i<=10;i++){at<U8>(ar[i],0x78)=(U8)v[i].o;at<U16>(ar[i],0x3c)=(U16)v[i].com;at<U16>(ar[i],0x3e)=50;at<U16>(ar[i],0x40)=50;at<U16>(ar[i],0x52)=v[i].pop;}
   // 세력 객체: 1·9 = 명령서 99, 3 = 7장, 7 = vtable 이 달라 안 셈, 5 = 지역이 없어 안 셈, 2 = 플레이어라 안 셈
   // 16차: 명령서는 군단(+0x10 세력, +0x14 명령서, vtable 0x13a98f0). 세력 객체(+0x14=77)는 읽지 않아야 한다.
   for(unsigned f=1;f<=9;f++){U8*o=(U8*)calloc(1,0x40);*(U8**)o=game+0x13a9880;at<U8>(o,0x14)=77;*(U8**)(d+0xdf30+8*f)=o;}
   auto mkdist=[&](unsigned k,unsigned f,unsigned cmd,bool good){U8*o=(U8*)calloc(1,0x40);*(U8**)o=good?game+0x13a98f0:game+0x13a96d0;at<U8>(o,0x10)=(U8)f;at<U8>(o,0x14)=(U8)cmd;*(U8**)(d+0xe0d0+8*k)=o;};
   mkdist(1,1,99,true);mkdist(2,9,99,true);mkdist(3,3,7,true);mkdist(4,7,99,false);mkdist(5,5,99,true);mkdist(6,2,3,true);mkdist(7,2,50,true);mkdist(8,0,99,true);
   int n0,n1,n2;last_world(&n0);long fb2=t_focus,pb2=t_pop;
   world_tick();std::string l=last_world(&n1);assert(n1==n0+1);
   printf("world: %s",l.c_str());
   // 지역 3(개발값 이상)은 빠짐 → 9곳. 인구 합 = 5000+10000+3000+20000+500+8000+100+50000+12000 = 108600, 플레이어(2) = 10000+20000+8000+50000 = 88000
   // 1만 미만 = 지역 1·4·6·7·8 = 5곳, 그중 플레이어 = 지역 7 = 1곳. AI 세력 = 1·9·3 (7은 객체가 달라 빠짐) → 3, 그중 99장 = 2, 최소 = 7
   for(const char*k:{"\tworld\t201/4\t","\tareas=9\t","\tpop_all=108600\t","\tpop_player=88000\t","\tunder10k=5\t","\tunder10k_player=1\t","\tai_corps=3\t","\tai_cmd99=2\t","\tai_cmd_min=7\t","\tplayer=2\t","\tcmd_player=3\t"})
    if(l.find(k)==std::string::npos){printf("FAIL world 줄에 %s 없음\n",k);return 1;}
   for(int i=1;i<=10;i++)assert(at<U16>(ar[i],0x52)==v[i].pop&&at<U8>(ar[i],0x78)==v[i].o);assert(t_focus==fb2&&t_pop==pb2);   // 읽기만
   world_tick();world_tick();last_world(&n2);assert(n2==n1);                       // 같은 달 = 다시 안 씀
   at<U8>(w,0x36)=5;world_tick();last_world(&n2);assert(n2==n1+1);                 // 달이 바뀌면 한 줄
   at<U8>(w,0x3a)=0;at<U8>(w,0x36)=6;world_tick();int n3;last_world(&n3);assert(n3==n2+1);n2=n3;   // 데모(플레이어 0)에서도 씀 — 11차
   at<U8>(w,0x3a)=2;at<short>(w,0x34)=0;world_tick();last_world(&n3);assert(n3==n2);       // 불러오기 전 = 안 씀
   *(U8**)(game+0x2780f60)=nullptr;world_tick();last_world(&n3);assert(n3==n2);*(U8**)(game+0x2780f60)=d;  // 게임 표 없음 = 안 씀(튕기지 않음)
   { // 11차: 인물 집계(census) — 소속 3(그중 플레이어 2)·재야 2·포로 1·그 밖 1
     static U8 pp[8][0x100];unsigned st[8]={0,1,3,4,5,5,6,0},fo[8]={0,2,2,7,0,0,2,0};
     for(int i=1;i<=7;i++){at<U8>(pp[i],0xe6)=(U8)st[i];at<U8>(pp[i],0xe0)=(U8)fo[i];*(U8**)(d+0x158+8*i)=pp[i];}
     at<U8>(w,0x3a)=2;at<short>(w,0x34)=202;at<U8>(w,0x36)=1;world_tick();
     std::string c;FILE*f=popen("cat /tmp/dp_root/*/* 2>/dev/null","r");char buf[600];while(f&&fgets(buf,sizeof buf,f))if(strstr(buf,"\tcensus\t"))c=buf;if(f)pclose(f);
     for(const char*k:{"\tcensus\t202/1\t","\tofficers=3\t","\tofficers_player=2\t","\tronin=2\t","\tcaptive=1\t","\tcaptive_of_player_force=1\t","\tgone=1\t"})
      if(c.find(k)==std::string::npos){printf("FAIL census 줄에 %s 없음: %s\n",k,c.c_str());return 1;}
     for(int i=1;i<=7;i++)assert(at<U8>(pp[i],0xe6)==st[i]);for(int i=1;i<=7;i++)*(U8**)(d+0x158+8*i)=nullptr;
     printf("통과: 인물 집계(census) 값 맞음·읽기만\n"); }
   printf("통과: P206 달 요약(world) — 값 맞음·읽기만·같은 달 한 번·타이틀/불러오기 전 안 씀\n"); }
 printf("통과: R42i3 플레이어 없음·불러오기 전·빈 칸이면 안 씀\n");
 printf("통과: DomesticPlayer R42h 네이티브 시험 (포인터 표 비연속·플레이어만·골고루·인구하한·멱등·끔)\n");
 return 0;
}
