/* Only kernel32 imports; every other system entry is resolved explicitly. */
#include "win_types.h"
#include "mod_api.h"
typedef struct {
    void *base,*allocation; U32 allocation_protect,padding; SIZE size;
    U32 state,protect,type,padding2;
} S14_MemoryInfo;
static void memzero(void *p,SIZE n) { U8 *d=p;while(n--)*d++=0; }
static void memcopy(void *p,const void *q,SIZE n) { U8 *d=p;const U8 *s=q;while(n--)*d++=*s++; }
static I32 memeq(const void *p,const void *q,SIZE n) { const U8 *a=p,*b=q;while(n--)if(*a++!=*b++)return 0;return 1; }
static U32 wappend(U16 *p,U32 n,U32 cap,const U16 *s) {
    if(n>=cap)return 0;
    while(*s){if(n+1>=cap){p[0]=0;return 0;}p[n++]=*s++;}p[n]=0;return n;
}
static I32 ascii_equal(const char *a,const char *b) { while(*a && *a==*b){++a;++b;}return *a==*b; }
static I32 wide_equal(const U16 *a,const U16 *b) {
    U16 x,y;do {x=*a++;y=*b++;if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y)return 0;}while(x);return 1;
}
static U32 module_dir(HANDLE m,U16 *path,U32 cap) {
    U32 n=win.GetModuleFileNameW(m,path,cap);if(!n || n>=cap)return 0;
    while(n && path[n-1]!='\\' && path[n-1]!='/')--n;
    if(n)path[n]=0;
    return n;
}
static U32 game_dir(U16 *path,U32 cap) {
    U32 n=win.GetModuleFileNameW(0,path,cap),i;
    if(!n || n>=cap)return 0;
    i=n;
    while(i && path[i-1]!='\\' && path[i-1]!='/')--i;
    if(!i || !wide_equal(path+i,L"SAN14CE_KO.exe"))return 0;
    path[i]=0;return i;
}
static HANDLE load_relative_game(const U16 *tail) {
    U16 path[1024];U32 n=game_dir(path,1024);
    if(!n || !wappend(path,n,1024,tail))return 0;
    return win.LoadLibraryExW(path,0,0x900); /* absolute DLL dir + System32 only */
}
