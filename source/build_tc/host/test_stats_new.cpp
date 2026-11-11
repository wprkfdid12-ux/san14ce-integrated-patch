// 17번 Stats200New 검사(리눅스): 실제 새 exe 이미지(/tmp/new_img.bin, upd_tools newmap 으로 평탄화)에 모듈을 돌려
//  15자리 전부 바뀌는지, 릴레이가 5바이트만 쓰는지, 릴레이 판정(XpBridge)이 세 경우에 맞게 가는지 확인.
// clang++ -std=c++17 -fshort-wchar -fdeclspec -Wno-unknown-attributes -Wno-ignored-attributes -DS14_TEST_SHIM -I build_tc/host -I src -O1 -g build_tc/host/test_stats_new.cpp src/stats200_new.cpp src/stats200_new_bridge.S -o /tmp/tstats_new -lpthread
#include "winstubs.h"
extern "C"{int SAN14_ModInitV1(const S14_CoreAPI*);long SAN14_Stats200NewXpHits();void XpBridge();extern void*XpCont,*XpCap;}
static U8*game;static std::vector<std::string> logs;static std::vector<std::vector<U8>> relays;static int installs=0,refuse=0;
static I32 Readable(const void*p,UPTR n){return p&&(U8*)p>=game&&(U8*)p+n<=game+0x2a02000;}
static I32 InstallHooks(const char*id,const S14_Hook*h,U32 n){
 if(refuse){return 0;}
 for(U32 i=0;i<n;i++)if(memcmp(h[i].target,h[i].expected,h[i].length)){printf("  InstallHooks(%s): 기대 불일치 #%u\n",id,i);return 0;}
 for(U32 i=0;i<n;i++)memcpy(h[i].target,h[i].replacement,h[i].length);installs++;return 1;
}
static U8*MakeRelay(const void*,const void*dest){U8*r=(U8*)mmap(nullptr,0x1000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);r[0]=0xff;r[1]=0x25;memset(r+2,0,4);memcpy(r+6,&dest,8);return r;}
static void FreeRelay(void*p){munmap(p,0x1000);}
static void Log(const char*s){logs.push_back(s);printf("    [기록] %s\n",s);}
#define fails failures
static bool has(const char*n){for(auto&l:logs)if(l.find(n)!=std::string::npos)return true;return false;}
// XpBridge 를 직접 불러 판정 경로 확인: cont/cap 을 우리 표식 함수로 바꿔 두고 cmp ecx,0x7b 플래그를 만든 뒤 점프
static volatile int landed=0;
extern "C" void land_cont(){landed=1;}
extern "C" void land_cap(){landed=2;}
static int drive(unsigned ecx,unsigned esi){
 landed=0;XpCont=(void*)&land_cont;XpCap=(void*)&land_cap;
 // XpBridge 는 jmp 로 끝나므로 call 로 들어가면 land_* 가 우리 대신 ret 해 준다
 asm volatile("mov %0,%%ecx\n\tmov %1,%%esi\n\tcmp $0x7b,%%ecx\n\tcall *%2\n"::"r"(ecx),"r"(esi),"r"((void*)&XpBridge):"rcx","rsi","rax","rdx","r8","r9","r10","r11","memory","cc");
 return landed;
}
int main(){
 FILE*f=fopen("/tmp/new_img.bin","rb");if(!f){printf("이미지 없음\n");return 2;}
 game=(U8*)mmap(nullptr,0x2a02000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);size_t n=fread(game,1,0x2a02000,f);fclose(f);printf("이미지 %zu\n",n);exeHandle=game;
 S14_Resolved res{};res.size=sizeof res;res.abi=S14_ABI;res.layout_abi=1;res.image_size=0x2a02000;res.image=game;
 S14_CoreAPI api{};api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=Readable;api.InstallHooks=InstallHooks;api.MakeRelay=MakeRelay;api.FreeRelay=FreeRelay;api.Log=Log;
 printf("[17번 능력치 200 새 모듈 검사 — 실제 새 exe 이미지]\n");
 std::vector<U8> before(game+0x231af8,game+0x231b08);
 int r=SAN14_ModInitV1(&api);
 check(r==1,"초기화 성공");
 check(has("caps 14 sites OK (applied 14, already 0)"),"상수 14곳 전부 새로 적용(이미 적용 0)");
 check(has("xp-growth relay installed"),"경험치 성장 릴레이 설치");
 check(installs==2,"ModCore 묶음 2개(Caps, XP)로 등록");
 // 자리 확인
 static const unsigned sites3[]={0x226e2a,0x226e7f,0x22ae26,0x22ae99,0x23c458,0x23c4b6};bool ok3=true;for(unsigned s:sites3)ok3&=game[s]==0xb2&&game[s+1]==0xc8&&game[s+2]==0x90;check(ok3,"편집 입력 상한 6곳 = mov dl,0xc8");
 check(game[0x20b7c6]==0xc8&&game[0x20bfdb]==0xc8&&game[0x211f0e]==0xc8,"읽기 상한 3곳 = 0xc8");
 check(game[0x5dfd1b]==0xc8&&game[0x5dfb86]==0xc8,"표시 상한 2곳 = 0xc8");
 check(*(U32*)(game+0x142fc70)==0xc8&&*(U32*)(game+0x142fc7c)==0xc8,"상한 표 4칸 = 0xc8");
 check(game[0x321f3e]==0x80&&game[0x321f40]==0xc8&&game[0x2bc14e]==0x72,"비교 2곳 바뀜");
 check(game[0x231afb]==0xe9&&!memcmp(game+0x231b00,&before[8],8),"성장 분기: 5바이트 점프만 쓰고 0x231b00 이후는 그대로");
 // 두 번째 초기화(다른 모듈이 먼저 깔린 상황 흉내): 이미 적용으로 처리, 점프는 안 건드림
 logs.clear();installs=0;r=SAN14_ModInitV1(&api);
 check(r==1&&has("already 14")&&has("already hooked")&&installs==0,"다시 돌려도 이미 적용으로 보고 손대지 않음");
 // 릴레이 판정
 check(drive(5,150)==1,"ecx!=0x7b → 계속(0x231b00)");
 check(drive(0x7b,50)==2,"ecx==0x7b, esi<100 → 원래대로 esi=100 경로(0x231b42)");
 check(drive(0x7b,150)==1&&SAN14_Stats200NewXpHits()==1,"ecx==0x7b, esi>=100 → 상한 안 내리고 계속 + 횟수 1");
 check(drive(0x7b,100)==1,"esi==100 도 계속(내리지 않음)");
 // 바이트가 다른 판: 묶음 A 전부 보류, 창 기록
 game[0x226e2a]=0x11;logs.clear();
 // (다시 원본 상태로 만들기 위해 이미지 재로드)
 f=fopen("/tmp/new_img.bin","rb");n=fread(game,1,0x2a02000,f);fclose(f);game[0x22ae26]=0x11;
 r=SAN14_ModInitV1(&api);
 check(has("caps group NOT applied (site #2 bytes differ)")&&game[0x226e2a]==0x0f&&game[0x231afb]==0xe9,"상수 1곳이 다르면 14곳 전부 보류(다른 자리 안 건드림), 성장 릴레이는 따로 적용");
 // ModCore 거절 → 직접 쓰기
 f=fopen("/tmp/new_img.bin","rb");n=fread(game,1,0x2a02000,f);fclose(f);refuse=1;logs.clear();r=SAN14_ModInitV1(&api);refuse=0;
 check(has("written directly")&&game[0x226e2b]==0xc8&&has("xp-growth relay NOT installed"),"ModCore 가 거절하면 상수는 직접 쓰고, 릴레이는 보류 보고");
 printf("통과 %d 실패 %d\n",passes,fails);return fails?1:0;
}
