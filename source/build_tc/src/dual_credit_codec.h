#pragma once
#include <stdint.h>
struct MeritCode{uint8_t upper,lower,wire[3];};
static const MeritCode merit_codes[]={
 {1,3,{4,5,1}},{2,2,{2,1,4}},{2,3,{4,2,5}},
 {3,1,{3,1,4}},{3,2,{3,2,4}},{3,3,{3,2,1}}
};
inline void merit_encode(uint8_t*out,const uint8_t*in){
 unsigned upper=in[0]>3?3:in[0],lower=in[1]>3?3:in[1];
 for(const auto&r:merit_codes)if(r.upper==upper&&r.lower==lower){for(int i=0;i<3;i++)out[i]=r.wire[i];return;}
 for(int i=0;i<3;i++)out[i]=0;unsigned n=0;
 for(unsigned i=0;i<upper&&n<3;i++)out[n++]=uint8_t(i+1);
 for(unsigned i=0;i<lower&&n<3;i++)out[n++]=uint8_t(i+4);
}
inline void merit_decode(uint8_t*out,const uint8_t*in){
 for(const auto&r:merit_codes)if(in[0]==r.wire[0]&&in[1]==r.wire[1]&&in[2]==r.wire[2]){out[0]=r.upper;out[1]=r.lower;return;}
 out[0]=out[1]=0;for(int i=0;i<3;i++){if(in[i]>=1&&in[i]<=3)out[0]++;else if(in[i]>=4&&in[i]<=6)out[1]++;}
}
