#pragma once
// R43 시험11: 프레임 1~2 진단용 표본기(Diag 안). 게임 폴더 SAN14_Minimap.ini 에 "Profile=1" 이 있을 때만 켠다(평소 꺼짐).
//  왜: 미니맵 표본기(FogTrial)는 "메인 스레드가 ntdll 안 99%"까지만 알려 줌 → 누가 그 시스템 함수를 부르는지 모름.
//  어떻게: 게임 창의 스레드(메인)를 약 1ms 마다 잠깐 멈추고(SuspendThread) RIP·RSP·인자 레지스터와 스택 32KB 를 복사한 뒤 바로 풀어 준다.
//   멈춘 동안은 잠금을 잡는 함수(힙·로더)를 절대 부르지 않는다(GetThreadContext·VirtualQuery·memcpy 만) → 교착 없음.
//   풀어 준 뒤 복사본에서 '돌아갈 주소'(앞 바이트가 call 명령인 것)를 찾아 첫 비시스템 호출자(게임/모듈)를 센다.
//   30초씩 3번(창 뜬 뒤 5초·95초·185초) 재고 SAN14_기록\Diag_Perf.log 에 적은 뒤 끝난다(계속 돌지 않음).
//   스레드별 CPU 시간도 같이 적어 '기다리는지(CPU 적음)·돌고 있는지(CPU 많음)'를 가른다.
#include <stddef.h>
#include <string.h>
namespace perf{
typedef unsigned long long u64;typedef unsigned int u32;typedef unsigned char u8;
// ---- 순수 로직(호스트 시험 가능)
// 돌아갈 주소 r 앞 바이트가 call 인가. p = r 의 메모리(앞쪽 7바이트 읽기 가능해야 함)
inline bool is_call_before(const u8*p){
 if(p[-5]==0xE8)return true;                                   // call rel32
 if(p[-6]==0xFF&&p[-5]==0x15)return true;                       // call [rip+d32]
 if(p[-2]==0xFF&&(p[-1]&0xF8)==0xD0)return true;                // call reg
 if(p[-3]==0x41&&p[-2]==0xFF&&(p[-1]&0xF8)==0xD0)return true;   // call r8~r15
 if(p[-3]==0xFF&&(p[-2]&0xF8)==0x50)return true;                // call [reg+d8]
 if(p[-4]==0x41&&p[-3]==0xFF&&(p[-2]&0xF8)==0x50)return true;
 if(p[-6]==0xFF&&(p[-5]&0xF8)==0x90)return true;                // call [reg+d32]
 if(p[-7]==0x41&&p[-6]==0xFF&&(p[-5]&0xF8)==0x90)return true;
 if(p[-2]==0xFF&&(p[-1]&0xF8)==0x10&&(p[-1]&7)!=4&&(p[-1]&7)!=5)return true;   // call [reg]
 if(p[-3]==0xFF&&p[-2]==0x14)return true;                       // call [sib]
 if(p[-4]==0xFF&&p[-3]==0x54)return true;                       // call [sib+d8]
 return false;}
// 키→개수 표(선형 탐사). key 0 은 빈칸.
template<unsigned N>struct Tab{u64 k[N];u32 c[N];u64 aux[N];u64 aux2[N];unsigned used;
 void clear(){memset(k,0,sizeof k);memset(c,0,sizeof c);memset(aux,0,sizeof aux);memset(aux2,0,sizeof aux2);used=0;}
 void add(u64 key,u64 a=0,u64 a2=0){if(!key)return;u64 h=key*0x9E3779B97F4A7C15ull;unsigned i=(unsigned)(h>>40)%N;
  for(unsigned n=0;n<N;n++,i=(i+1)%N){if(k[i]==key){c[i]++;aux[i]=a;aux2[i]=a2;return;}if(!k[i]){if(used*4>=N*3)return;k[i]=key;c[i]=1;aux[i]=a;aux2[i]=a2;used++;return;}}}
 // 많은 순 상위 m 개의 칸 번호
 unsigned top(unsigned*out,unsigned m)const{unsigned n=0;for(unsigned i=0;i<N;i++){if(!k[i])continue;unsigned j;
   if(n<m){j=n++;out[j]=i;}else if(c[i]>c[out[m-1]]){j=m-1;out[j]=i;}else continue;
   while(j>0&&c[out[j-1]]<c[out[j]]){unsigned t=out[j-1];out[j-1]=out[j];out[j]=t;j--;}}return n;}
};
struct Mod{u64 base,end;bool sys;bool game;char name[64];};
// 정렬된 모듈 표에서 주소 찾기
inline int find_mod(const Mod*m,int n,u64 a){int lo=0,hi=n-1;while(lo<=hi){int mid=(lo+hi)/2;if(a<m[mid].base)hi=mid-1;else if(a>=m[mid].end)lo=mid+1;else return mid;}return -1;}
inline void sort_mods(Mod*m,int n){for(int i=1;i<n;i++){Mod t=m[i];int j=i;while(j>0&&m[j-1].base>t.base){m[j]=m[j-1];j--;}m[j]=t;}}
// 메모리에 올라온 PE 의 export 중 a 바로 아래 이름(없으면 nullptr)
inline const char*near_export(const u8*base,u64 size,u64 a,u64*off){
 if(size<0x400||base[0]!='M'||base[1]!='Z')return nullptr;u32 pe=*(const u32*)(base+0x3c);if(pe>size-0x100||memcmp(base+pe,"PE\0\0",4))return nullptr;
 u32 erva=*(const u32*)(base+pe+0x88),esz=*(const u32*)(base+pe+0x8c);if(!erva||!esz||erva+0x28>size)return nullptr;
 const u8*ed=base+erva;u32 nfun=*(const u32*)(ed+0x14),nnam=*(const u32*)(ed+0x18),afun=*(const u32*)(ed+0x1c),anam=*(const u32*)(ed+0x20),aord=*(const u32*)(ed+0x24);
 if(afun+4ull*nfun>size||anam+4ull*nnam>size||aord+2ull*nnam>size)return nullptr;
 u64 rva=a-(u64)base;const char*best=nullptr;u64 bestrva=0;
 for(u32 i=0;i<nnam;i++){u32 nr=((const u32*)(base+anam))[i];unsigned short o=((const unsigned short*)(base+aord))[i];if(o>=nfun||nr>=size)continue;u32 fr=((const u32*)(base+afun))[o];
  if(fr>=erva&&fr<erva+esz)continue;if(fr<=rva&&fr>=bestrva){bestrva=fr;best=(const char*)(base+nr);}}
 if(best)*off=rva-bestrva;return best;}
// 복사한 스택(stk, 바이트 n, 원래 주소 rsp)에서 비시스템 호출자 차례로 최대 want 개
// readable(a): a-7..a 를 읽어도 되는가(모듈 안이면 됨)
typedef bool(*Readable)(u64 lo,u64 hi);
inline int callers(const u8*stk,size_t n,const Mod*m,int nm,u64*out,int want,bool skip_game,int*first_game_idx,Readable rd=nullptr){
 int k=0;if(first_game_idx)*first_game_idx=-1;
 for(size_t i=0;i+8<=n&&k<want;i+=8){u64 v;memcpy(&v,stk+i,8);int mi=find_mod(m,nm,v);if(mi<0||m[mi].sys)continue;if(v<m[mi].base+0x1000)continue;if(rd&&!rd(v-8,v))continue;
  if(!is_call_before((const u8*)v))continue;if(skip_game&&m[mi].game)continue;
  if(first_game_idx&&*first_game_idx<0&&m[mi].game)*first_game_idx=k;out[k++]=v;}
 return k;}
}
