// 9칸 화면 부품 R42e(T9AllCreate 너그러운 다시 재기 + 실패 시 오류창 예약) — 리눅스 네이티브 시험(에뮬 없이)
//  게임 함수 4곳(표지 칸 할당·생성·위치·만들기)과 화면 부품 함수 2곳을 이 시험기의 함수로 이어 놓고(가짜 메모리에 jmp) 그대로 부른다.
//  빌드·실행: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_traits9_all_native.cpp -o /tmp/t9 && /tmp/t9
#include "common.h"
#include <sys/mman.h>
#include <string>
#include <vector>
#include <cassert>
#define INVALID_FILE_ATTRIBUTES 0xffffffffu
#define PAGE_EXECUTE_READWRITE 0x40
#define FILE_SHARE_DELETE 4
extern "C" DWORD GetFileAttributesW(const wchar_t*){return INVALID_FILE_ATTRIBUTES;}
extern "C" BOOL VirtualProtect(void*,size_t,DWORD,DWORD*){return 0;}
extern "C" BOOL FlushInstructionCache(HANDLE,const void*,size_t){return 1;}
extern "C" HANDLE GetCurrentProcess(){return 0;}
extern "C" void Sleep(DWORD){}
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*)(void*),void*,DWORD,DWORD*){return 0;}
#include "../src/traits9_check.cpp"
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p="/tmp/t9_root\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string s=narrow(p);std::string c="mkdir -p '"+s+"'";if(system(c.c_str())){}return 1;}
static std::vector<std::string> lines;
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){lines.push_back(std::string((const char*)b,n));*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=3;}
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int d,const wchar_t*){return (unsigned)d;}
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){return 1;}
static U8*fake_ui;extern "C" HMODULE GetModuleHandleW(const wchar_t*){return (HMODULE)fake_ui;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
extern "C" HMODULE LoadLibraryExW(const wchar_t*,HANDLE,DWORD){return nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
// ---- 가짜 게임·화면 부품 함수
struct Lab{int x,y,w,h;U8*parent;};static std::vector<Lab> made;static bool fail_alloc,fail_create;static int grid_logs;
static void*vt_lab[0x30];
static U8*g_alloc(U64 n){if(fail_alloc)return nullptr;U8*o=(U8*)calloc(1,n);*(void***)o=vt_lab;return o;}
static void g_ctor(U8*){}
static void g_place(U8*o,int x,int y,int w,int h,U8*p,int){at<int>(o,0x20)=x;at<int>(o,0x24)=y;at<int>(o,0x28)=w;at<int>(o,0x2c)=h;at<U8*>(o,8)=p;made.push_back({x,y,w,h,p});}
static void g_make(U8*o,int){if(!fail_create)at<U8*>(o,0x178)=o;}
static void g_attach(void*,U8*){}
static void u_refresh(U8*,int){}
static void u_log(const char*){grid_logs++;}
static void jmp_to(U8*at_,void*f){U8 c[12]={0x48,0xb8};memcpy(c+2,&f,8);c[10]=0xff;c[11]=0xe0;memcpy(at_,c,12);}
static I32 readable(const void*p,UPTR){return p!=nullptr;}
static S14_CoreAPI api;
static U8*nat_of(int x,int y,int w,int h,U8*parent){U8*n=(U8*)calloc(1,0x200);at<int>(n,0x20)=x;at<int>(n,0x24)=y;at<int>(n,0x28)=w;at<int>(n,0x2c)=h;at<U8*>(n,8)=parent;return n;}
static int run(const char*name,std::vector<Lab> boxes,U8*tparent,int expect_ret,int expect_rows){
 made.clear();grid_logs=0;pend_on=0;pend[0]=0;
 U8*tbl=(U8*)calloc(1,0x1000);*(U8**)tbl=tparent;U8*nat[5];for(int i=0;i<5;i++)nat[i]=nat_of(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h,boxes[i].parent);
 int r=T9AllCreate(tbl,1,nat,(void*)0x1);
 int rows=0;{std::vector<int> ys;for(auto&m:made){bool f=false;for(int y:ys)if(y==m.y)f=true;if(!f)ys.push_back(m.y);}rows=(int)ys.size();}
 printf("%-34s ret=%d 만든칸=%zu 줄=%d 기록=%d 오류창예약=%d diag=[%s]\n",name,r,made.size(),rows,grid_logs,(int)pend_on,all_diag);
 if(pend_on)printf("   예약 글: %s\n",pend);
 if(r)for(auto&m:made)assert(m.w>=16&&m.h>0);
 assert(r==expect_ret);if(expect_rows>=0)assert(rows==expect_rows);
 if(!r)assert(pend_on==1&&strstr(pend,"ALLSCREEN create failed - "));
 return r;
}
int main(){
 if(system("mkdir -p /tmp/t9_root")){}
 game=(U8*)mmap(0,0x2a00000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE,-1,0);
 fake_ui=(U8*)mmap(0,0x10000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 jmp_to(game+0x3f90e0,(void*)g_alloc);jmp_to(game+0x84d4b0,(void*)g_ctor);jmp_to(game+0x8534e0,(void*)g_place);jmp_to(game+0x89d1b0,(void*)g_attach);
 jmp_to(fake_ui+0x1e50,(void*)u_refresh);jmp_to(fake_ui+0x1060,(void*)u_log);
 vt_lab[0xc8/8]=(void*)g_make;
 api.Readable=readable;core=&api;
 U8*P=(U8*)calloc(1,64),*Q=(U8*)calloc(1,64);
 // 1) 보통: 한 줄 5칸, 간격 6 → 원래 계산 그대로(why=-)
 run("보통 한 줄",{{0,0,60,20,P},{66,0,60,20,P},{132,0,60,20,P},{198,0,60,20,P},{264,0,60,20,P}},P,1,1);
 assert(all_diag[0]==0||strstr(all_diag,"why=-"));
 // 2) 일기토 무장 정보(제보 why=N): 102x42 칸이 100 간격(겹침) → 원래는 N, 이제 너그럽게 만듦
 run("일기토형 겹친 칸(why=N)",{{0,0,102,42,P},{100,0,102,42,P},{200,0,102,42,P},{300,0,102,42,P},{400,0,30,42,P}},P,1,-1);
 assert(strstr(all_diag,"relaxed=ok(+)"));
 // 3) 두 줄인데 줄 간격 < 높이(제보 why=G) → 너그럽게
 run("줄 간격 좁음(why=G)",{{0,0,80,30,P},{90,0,80,30,P},{180,0,80,30,P},{0,20,80,30,P},{90,20,80,30,P}},P,1,-1);
 // 4) 높이 다름(why=E)
 run("높이 다름(why=E)",{{0,0,80,30,P},{90,0,80,28,P},{180,0,80,30,P},{270,0,80,26,P},{360,0,80,30,P}},P,1,1);
 // 5) 표 부모와 순정 칸 부모가 다름(why=P), 순정 칸끼리는 같음 → 순정 칸 부모로 만듦
 run("표 부모 다름(why=P)",{{0,0,60,20,Q},{66,0,60,20,Q},{132,0,60,20,Q},{198,0,60,20,Q},{264,0,60,20,Q}},P,1,1);
 for(auto&m:made)assert(m.parent==Q);
 // 6) 순정 칸끼리 부모가 다름 → 실패 + 오류창 예약
 run("순정 칸 부모 섞임",{{0,0,60,20,P},{66,0,60,20,Q},{132,0,60,20,P},{198,0,60,20,P},{264,0,60,20,P}},P,0,-1);
 assert(strstr(pend,"labels on different parents"));
 // 7) 크기 범위 밖 → 실패
 run("크기 범위 밖",{{0,0,60,700,P},{66,0,60,20,P},{132,0,60,20,P},{198,0,60,20,P},{264,0,60,20,P}},P,0,-1);
 assert(strstr(pend,"label size out of range"));
 // 8) 칸 할당 실패 / 만들기 실패
 fail_alloc=true;run("칸 할당 실패",{{0,0,60,20,P},{66,0,60,20,P},{132,0,60,20,P},{198,0,60,20,P},{264,0,60,20,P}},P,0,-1);fail_alloc=false;
 assert(strstr(pend,"label alloc failed"));
 fail_create=true;run("칸 만들기 실패",{{0,0,60,20,P},{66,0,60,20,P},{132,0,60,20,P},{198,0,60,20,P},{264,0,60,20,P}},P,0,-1);fail_create=false;
 assert(strstr(pend,"native label create failed"));
 // 9) 순정 칸 하나 못 읽음
 {made.clear();pend_on=0;U8*tbl=(U8*)calloc(1,0x1000);*(U8**)tbl=P;U8*nat[5]={nat_of(0,0,60,20,P),nullptr,nat_of(0,0,60,20,P),nat_of(0,0,60,20,P),nat_of(0,0,60,20,P)};
  int r=T9AllCreate(tbl,2,nat,0);printf("%-34s ret=%d 예약=%d 글=%s\n","순정 칸 못 읽음",r,(int)pend_on,pend);assert(!r&&strstr(pend,"native label unreadable")&&strstr(pend,"kind=2"));}
 // 10) 예약 중이면 새 실패는 글을 덮지 않음(감시 스레드가 먼저 것을 띄움)
 {char keep[700];memcpy(keep,pend,sizeof keep);U8*tbl=(U8*)calloc(1,0x1000);*(U8**)tbl=P;U8*nat[5]={nullptr,nullptr,nullptr,nullptr,nullptr};T9AllCreate(tbl,3,nat,0);assert(!strcmp(keep,pend));}
 printf("통과: 9칸 R42e 시험 10가지\n");
 return 0;
}
