// 18번 능력치 상한(Stats200 R4) 검사: 새로 붙인 1곳이 안 맞아도 기존 14곳은 적용되는지
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
static unsigned char*fake_core;static int install_ok=1;
extern "C" void*GetModuleHandleW(const wchar_t*n){return n&&n[0]=='S'&&n[6]=='M'?fake_core:nullptr;}
extern "C" int VirtualProtect(void*,unsigned long long,unsigned long,unsigned long*o){*o=0x40;return 1;}
extern "C" int FlushInstructionCache(void*,const void*,unsigned long long){return 1;}
extern "C" void*GetCurrentProcess(){return (void*)-1;}
#include "../src/stats200_direct.cpp"
static U8*img;static std::string logs;static int fails=0,passes=0;
static void check(bool ok,const char*w){printf("  %s: %s\n",ok?"통과":"실패",w);ok?++passes:++fails;}
static I32 R(const void*,UPTR){return 1;}
static I32 IH(const char*,const S14_Hook*h,U32 n){if(!install_ok)return 0;for(U32 i=0;i<n;i++)if(memcmp(h[i].target,h[i].expected,h[i].length))return 0;for(U32 i=0;i<n;i++)memcpy(h[i].target,h[i].replacement,h[i].length);return 1;}
static void L(const char*s){logs+=s;logs+="\n";printf("    [기록] %s\n",s);}
static void fresh(bool bad15,bool pre14){
 memset(img,0,0x29ff000);img[0]='M';img[1]='Z';*(U32*)(img+0x3c)=0x80;U8*nt=img+0x80;*(U32*)nt=0x4550;*(U16*)(nt+4)=0x8664;*(U16*)(nt+24)=0x20b;*(U32*)(nt+8)=0x6a6d9f72;*(U32*)(nt+0x50)=0x029ff000;
 for(int i=0;i<15;i++){const PatchSpec*s=&specs[i];memcpy(img+s->context,s->context_bytes,s->context_len);memcpy(img+s->site,(pre14&&i<14)?s->replacement:s->expected,s->len);}
 if(bad15)img[specs[14].context]^=0xff;
}
static bool applied(int i){return !memcmp(img+specs[i].site,specs[i].replacement,specs[i].len);}
int main(){
 img=(U8*)malloc(0x29ff000);S14_Resolved res{};res.size=sizeof res;res.abi=S14_ABI;res.layout_abi=1;res.image_size=0x29ff000;res.image=img;
 S14_CoreAPI api{};api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=R;api.InstallHooks=IH;api.Log=L;
 printf("[18번 능력치 상한 검사]\n");
 fresh(true,false);U32 r=SAN14_ModInitV1(&api);bool all14=true;for(int i=0;i<14;i++)all14&=applied(i);
 check(r==1&&all14&&!applied(14),"새로 붙인 1곳이 안 맞아도 HF5에서 되던 14곳은 적용 (R3는 이 경우 15곳 전부 포기)");
 fresh(false,false);r=SAN14_ModInitV1(&api);bool all=true;for(int i=0;i<15;i++)all&=applied(i);
 check(r==1&&all,"모두 맞으면 15곳 전부 적용 (편집 자료 읽기 상한 포함)");
 fresh(false,true);r=SAN14_ModInitV1(&api);all=true;for(int i=0;i<15;i++)all&=applied(i);
 check(r==1&&all,"예전 판이 14곳을 이미 바꿔 놓았어도 나머지 1곳만 추가 적용");
 // ModCore 등록부가 꽉 찬 상황: 로더가 거부해도 직접 적용
 fake_core=(unsigned char*)calloc(1,0x6000);const unsigned char sig[]={0x8b,0x05,0x7f,0x3e,0,0,0x44,0x01,0xc0,0x3d,0x80,0,0,0};memcpy(fake_core+0x117b,sig,14);*(U32*)(fake_core+0x5000)=120;
 install_ok=0;fresh(false,false);logs.clear();r=SAN14_ModInitV1(&api);all=true;for(int i=0;i<15;i++)all&=applied(i);
 check(r==1&&all,"로더 등록부가 꽉 차 등록을 거부해도 15곳을 직접 적용");
 check(logs.find("registry used before this module: 120/128")!=std::string::npos,"로더 등록부 사용량(120/128)을 기록에 남김");
 install_ok=1;
 // 확장 로더(1024)도 인식
 const unsigned char sig2[]={0x8b,0x05,0x7f,0x3e,0,0,0x44,0x01,0xc0,0x3d,0x00,0x04,0,0};memcpy(fake_core+0x117b,sig2,14);*(U32*)(fake_core+0x5000)=130;
 fresh(false,false);logs.clear();r=SAN14_ModInitV1(&api);all=true;for(int i=0;i<15;i++)all&=applied(i);
 check(r==1&&all&&logs.find("registry used before this module: 130/1024")!=std::string::npos,"확장 로더(1024칸)에서는 130/1024 로 기록하고 정상 적용");
 printf("결과: 통과 %d / 실패 %d\n",passes,fails);return fails?1:0;
}
