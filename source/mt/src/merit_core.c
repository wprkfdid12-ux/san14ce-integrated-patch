#include "merit_core.h"
#include "catalog.h"
void *mc_copy(void *d,const void*s,usize n){u8*a=d;const u8*b=s;while(n--)*a++=*b++;return d;}
void *mc_zero(void*d,usize n){u8*a=d;while(n--)*a++=0;return d;}
int mc_equal(const void*a,const void*b,usize n){const u8*x=a,*y=b;while(n--)if(*x++!=*y++)return 0;return 1;}
u8 mc_type(const u8 s[5]){u32 i;u8 best=0,m=0;for(i=0;i<5;i++)if(s[i]>best)best=s[i];for(i=0;i<5;i++)if(s[i]==best)m|=(u8)(1u<<i);return m;}
int mc_has(const u16 a[9],u16 id){u32 i;for(i=0;i<9;i++)if(a[i]==id)return 1;return 0;}
int mc_slot(const u16 a[9]){u32 i;for(i=0;i<9;i++)if(!a[i]||a[i]==NO_TRAIT)return (int)i;return -1;}
int mc_blocks(u16 held,u16 candidate){u32 i;if(held==candidate)return 1;for(i=0;i<sizeof(blocked_pairs)/sizeof(blocked_pairs[0]);i++)if(blocked_pairs[i].held==held&&blocked_pairs[i].candidate==candidate)return 1;return 0;}
u32 mc_pool(u8 mask,u8 stage,const u16 a[9],u16 out[355]){u32 i,j,n=0;u8 tier=stage==2?2:1;if(stage>2||!mask)return 0;for(i=0;i<sizeof(catalog)/sizeof(catalog[0]);i++){const TraitDef*t=&catalog[i];if(t->tier!=tier||!(mask&t->mask))continue;for(j=0;j<9;j++)if(a[j]&&a[j]!=NO_TRAIT&&mc_blocks(a[j],t->id))break;if(j==9)out[n++]=t->id;}return n;}
/* Rejection sampling: neither modulo bias nor time re-seeding; no outcome history. */
int mc_uniform(Random32 r,void*c,u32 n,u32*out){u32 x,limit,attempt;if(!r||!out||!n)return 0;limit=(0u-n)%n;for(attempt=0;attempt<128;attempt++){if(!r(c,&x))return 0;if(x>=limit){*out=x%n;return 1;}}return 0;}
int mc_draw(Random32 r,void*c,u8 mask,u8 stage,const u16 owned[9],u16*out){u16 p[355];u32 n,k;if(!out||mc_slot(owned)<0)return 0;n=mc_pool(mask,stage,owned,p);if(!mc_uniform(r,c,n,&k))return 0;*out=p[k];return 1;}
int mc_insert(u16 a[9],u16 id){int s;if(!id||id>=MAX_TRAITS||mc_has(a,id)||(s=mc_slot(a))<0)return 0;a[s]=id;return 1;}
void mc_profile(Profile*p,u16 id,const u8 s[5],u8 origin){mc_zero(p,sizeof(*p));p->id=id;p->mask=mc_type(s);p->origin=origin;p->valid=1;mc_copy(p->initial,s,5);p->awards[0]=p->awards[1]=p->awards[2]=NO_TRAIT;}
u32 mc_roll_index(u32 t,u32 n){u32 k;if(!n)return 0;/* Visual-only deterministic schedule, never consumes reward RNG. */k=t/65;return k%n;}
/* SHA-256, standard algorithm; save contents are hashed, never rewritten. */
static const u32 K[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static u32 rr(u32 x,u32 n){return(x>>n)|(x<<(32-n));}
static void block(u32 h[8],const u8 p[64]){u32 w[64],a,b,c,d,e,f,g,z,i,x,y;for(i=0;i<16;i++)w[i]=((u32)p[i*4]<<24)|((u32)p[i*4+1]<<16)|((u32)p[i*4+2]<<8)|p[i*4+3];for(i=16;i<64;i++){x=w[i-15];y=w[i-2];w[i]=w[i-16]+(rr(x,7)^rr(x,18)^(x>>3))+w[i-7]+(rr(y,17)^rr(y,19)^(y>>10));}a=h[0];b=h[1];c=h[2];d=h[3];e=h[4];f=h[5];g=h[6];z=h[7];for(i=0;i<64;i++){x=z+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+K[i]+w[i];y=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c));z=g;g=f;f=e;e=d+x;d=c;c=b;b=a;a=x+y;}h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;}
void mc_sha256(const void*data,usize n,u8 out[32]){const u8*p=data;u32 h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};u8 last[128];u64 bits=(u64)n*8;u32 i,k;while(n>=64){block(h,p);p+=64;n-=64;}mc_zero(last,128);mc_copy(last,p,n);last[n]=128;k=n<56?64:128;for(i=0;i<8;i++)last[k-1-i]=(u8)(bits>>(i*8));block(h,last);if(k==128)block(h,last+64);for(i=0;i<32;i++)out[i]=(u8)(h[i/4]>>(24-8*(i%4)));}
/* Freestanding Win64 compiler support; no CRT runtime dependency. */
int _fltused=0;
void*memcpy(void*d,const void*s,usize n){return mc_copy(d,s,n);}
void*memset(void*d,int c,usize n){u8*p=d;while(n--)*p++=(u8)c;return d;}
