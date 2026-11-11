// ICGuard K (P225, 2026-11-10) — 통합편의 [포로 회유]·[등용문] 버튼의 게임 코드 대조를 다른 모드가 알려 준 자리만큼 맞춰 주기.
//  왜: 통합편의는 두 버튼을 누를 때마다 자기가 부를 게임 함수 구간(표: 주소·길이·기대 바이트)을 memcmp 로 대조하고,
//      한 바이트라도 다르면 "열기 실패". 맵 확장이 새 도시 때문에 그 구간 안 21곳(도시 상한·표 변위·jmp)을 바꿔 늘 실패했음.
//  어떻게: 다른 모드가 게임 폴더 SAN14Mods\sites\<이름>.txt 에 "내가 바꾼 자리(주소·길이)" 를 써 두면,
//      버튼을 누르는 순간(통합편의 함수 입구) 그 파일을 새로 읽어, 대조 구간과 겹치는 자리만 통합편의의 기대 바이트를
//      '지금 게임에 있는 바이트' 로 맞춘다. 파일에 없는 자리가 다르면 그대로 실패(그때는 우리 오류 창).
//  파일 형식(맵 확장과 합의, 137 부터):
//      stamp=0x6ad06f53      ← 게임 판. 다르면 파일 전체 무시
//      mod=맵 확장 137        ← 오류 창·기록에 보일 이름
//      pid=12345             ← (있으면) 이 프로세스 번호와 다르면 무시(손으로 DLL 만 지워 파일이 남은 경우)
//      22c250 1              ← 주소(RVA, 16진) 길이(10진). 넷째 줄부터, 한 줄에 하나. # 으로 시작하는 줄·빈 줄은 건너뜀
//  게임을 안 부르고 메모리만 비교하는 순수 계산 → 시험(test/test_ic_mapsites_native.cpp)이 이 파일을 그대로 씀.
#pragma once
#include <stdint.h>
#include <string.h>
#include <stddef.h>
namespace kms{
static const unsigned kMaxSites=8192,kMaxLen=256;
struct Site{uint32_t rva,len;};
struct File{
 bool ok;          // stamp 맞고(그리고 pid 가 있으면 맞고) 자리를 1개 이상 읽음
 bool stamp_seen,stamp_ok,pid_seen,pid_ok;
 uint32_t stamp,pid;
 char mod[96];     // UTF-8 그대로
 unsigned n,bad_lines;
 Site s[kMaxSites];
};
static inline int hexv(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static inline bool rd_hex(const char*&p,const char*e,uint32_t&v){
 if(p+1<e&&p[0]=='0'&&(p[1]=='x'||p[1]=='X'))p+=2;
 uint64_t x=0;int k=0;while(p<e&&hexv(*p)>=0){x=x*16+(unsigned)hexv(*p++);if(++k>8)return false;}
 if(!k)return false;v=(uint32_t)x;return true;}
static inline bool rd_dec(const char*&p,const char*e,uint32_t&v){
 uint64_t x=0;int k=0;while(p<e&&*p>='0'&&*p<='9'){x=x*10+(unsigned)(*p++-'0');if(++k>10||x>0xffffffffull)return false;}
 if(!k)return false;v=(uint32_t)x;return true;}
static inline bool key(const char*p,const char*e,const char*k){size_t n=0;while(k[n])n++;return (size_t)(e-p)>=n&&!memcmp(p,k,n);}
// image_size: 게임 이미지 크기(주소+길이가 이보다 크면 그 줄은 버림)
static inline void parse(const char*b,size_t n,uint32_t want_stamp,uint32_t my_pid,uint32_t image_size,File&f){
 memset(&f,0,sizeof f);
 const char*p=b,*end=b+n;
 if(n>=3&&(uint8_t)b[0]==0xef&&(uint8_t)b[1]==0xbb&&(uint8_t)b[2]==0xbf)p+=3;   // UTF-8 BOM
 while(p<end){
  const char*q=p;while(q<end&&*q!='\n')q++;
  const char*e=q;if(e>p&&e[-1]=='\r')e--;
  while(p<e&&(*p==' '||*p=='\t'))p++;
  const char*le=e;while(le>p&&(le[-1]==' '||le[-1]=='\t'))le--;
  if(p<le&&*p!='#'){
   if(key(p,le,"stamp=")){const char*r=p+6;uint32_t v;f.stamp_seen=true;if(rd_hex(r,le,v)&&r==le){f.stamp=v;f.stamp_ok=(v==want_stamp);}}
   else if(key(p,le,"pid=")){const char*r=p+4;uint32_t v;f.pid_seen=true;if(rd_dec(r,le,v)&&r==le){f.pid=v;f.pid_ok=(v==my_pid);}}
   else if(key(p,le,"mod=")){size_t k=(size_t)(le-(p+4));if(k>sizeof f.mod-1)k=sizeof f.mod-1;memcpy(f.mod,p+4,k);f.mod[k]=0;}
   else{
    const char*r=p;uint32_t a,l;bool good=rd_hex(r,le,a);
    if(good){while(r<le&&(*r==' '||*r=='\t'))r++;good=rd_dec(r,le,l)&&r==le;}
    if(good&&l>=1&&l<=kMaxLen&&a>=0x1000&&(uint64_t)a+l<=image_size&&f.n<kMaxSites){f.s[f.n].rva=a;f.s[f.n].len=l;f.n++;}
    else f.bad_lines++;
   }
  }
  p=q<end?q+1:end;
 }
 f.ok=f.stamp_ok&&(!f.pid_seen||f.pid_ok)&&f.n>0;
}
// 통합편의 대조 표 한 줄: 게임 rva·길이·기대 바이트(통합편의 메모리 안)
struct Entry{uint32_t rva,len;uint8_t*exp;};
// 겹치는 자리만 기대 바이트 ← 지금 게임 바이트. write(dst,src,n) 은 쓰기 권한까지 맡음(시험에선 memcpy). 바뀐 바이트 수를 돌려줌.
template<class W> static inline unsigned apply(const uint8_t*game,const Entry*e,unsigned ne,const File&f,W write){
 unsigned changed=0;if(!f.ok)return 0;
 for(unsigned i=0;i<ne;i++){if(!e[i].exp||!e[i].len)continue;
  uint32_t lo0=e[i].rva,hi0=e[i].rva+e[i].len;
  for(unsigned k=0;k<f.n;k++){
   uint32_t lo=f.s[k].rva>lo0?f.s[k].rva:lo0,hi=f.s[k].rva+f.s[k].len<hi0?f.s[k].rva+f.s[k].len:hi0;
   if(lo>=hi)continue;
   const uint8_t*src=game+lo;uint8_t*dst=e[i].exp+(lo-lo0);uint32_t n=hi-lo;
   if(memcmp(dst,src,n)){unsigned d=0;for(uint32_t j=0;j<n;j++)if(dst[j]!=src[j])d++;write(dst,src,n);changed+=d;}
  }
 }
 return changed;
}
// 남은 차이: 첫 번째로 다른 표 줄 번호와 그 줄 안 위치. 없으면 false.
static inline bool first_diff(const uint8_t*game,const Entry*e,unsigned ne,unsigned*ei,uint32_t*off){
 for(unsigned i=0;i<ne;i++){if(!e[i].exp)continue;const uint8_t*g=game+e[i].rva;
  for(uint32_t j=0;j<e[i].len;j++)if(g[j]!=e[i].exp[j]){*ei=i;*off=j;return true;}}
 return false;
}
}
