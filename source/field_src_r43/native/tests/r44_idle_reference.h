#pragma once
// Restored R4.4 logic. No allocator, I/O, native clock writes, or extra animation evaluation.
namespace IdleReference {
using U32=__UINT32_TYPE__; using U64=__UINT64_TYPE__; using UP=__UINTPTR_TYPE__;
struct alignas(16) Pose { float scale[3]; int parent; float rotation[4]; float position[3]; U32 flags; };
static_assert(sizeof(Pose)==48,"Native local SRT stride");
struct Key {UP actor,setting,model,bones; U64 serial; int action;};
struct alignas(16) State { Key key; U32 count,tick; float frame,age; bool active,blending; Pose previous[256],boundary[256]; };
inline bool finite(float x){return __builtin_isfinite(x);}
inline bool same(const Key&a,const Key&b){return a.actor==b.actor&&a.setting==b.setting&&a.model==b.model&&a.bones==b.bones&&a.serial==b.serial&&a.action==b.action;}
inline void reset(State&s){s.active=false;s.blending=false;s.count=0;}
inline void copy(Pose* to,const Pose* from,unsigned n){
 typedef float V4 __attribute__((vector_size(16)));
 auto*d=reinterpret_cast<volatile V4*>(to);auto*s=reinterpret_cast<const volatile V4*>(from);
 for(unsigned i=0;i<n*3;++i)d[i]=s[i];
}
inline bool valid(const Pose* p,unsigned count){
 for(unsigned i=0;i<count;++i){
  if(p[i].parent< -1||p[i].parent>=int(count))return false;
  for(unsigned k=0;k<3;++k)if(!finite(p[i].scale[k])||!finite(p[i].position[k]))return false;
  float norm=0;for(unsigned k=0;k<4;++k){if(!finite(p[i].rotation[k]))return false;norm+=p[i].rotation[k]*p[i].rotation[k];}
  if(!finite(norm)||norm<0.000001f)return false;
 }
 return true;
}
// Called ONLY in noncombat POSE_IDLE; never POSE_BATTLE_IDLE.
inline void apply(State&s,const Key&key,Pose*bones,unsigned count,float frame,U32 tick){
 if(!bones||!count||count>256||!finite(frame)||frame<0){reset(s);return;}
 bool continuity=s.active&&s.count==count&&same(s.key,key)&&U32(tick-s.tick)<=1;
 if(continuity&&tick==s.tick)return;
 if(!continuity){s.key=key;s.count=count;s.frame=frame;s.tick=tick;s.blending=false;s.active=true;copy(s.previous,bones,count);return;}
 bool wrapped=frame+0.001f<s.frame;
 if(wrapped){
  if(!valid(s.previous,count)||!valid(bones,count)){reset(s);return;}
  for(unsigned i=0;i<count;++i)if(s.previous[i].parent!=bones[i].parent){reset(s);return;}
  copy(s.boundary,s.previous,count);s.age=0;s.blending=true;
 }else if(s.blending){float step=frame-s.frame;if(step>15.0f){s.blending=false;}else s.age+=step;}
 if(s.blending&&s.age>=12.0f)s.blending=false;
 if(s.blending){
  if(!valid(bones,count)){reset(s);return;}
  for(unsigned i=0;i<count;++i)if(bones[i].parent!=s.boundary[i].parent){reset(s);return;}
  float t=s.age*(1.0f/12.0f);t=t*t*(3.0f-2.0f*t);
  for(unsigned i=0;i<count;++i){
   Pose&to=bones[i];const Pose&from=s.boundary[i];
   if(to.parent!=from.parent){reset(s);return;}
   float dot=0,norm=0,q[4];
   for(unsigned k=0;k<4;++k)dot+=from.rotation[k]*to.rotation[k];
   for(unsigned k=0;k<4;++k){q[k]=from.rotation[k]*(1-t)+(dot<0?-1.0f:1.0f)*to.rotation[k]*t;norm+=q[k]*q[k];}
   if(finite(norm)&&norm>0.000001f){float inv=1.0f/__builtin_sqrtf(norm);for(unsigned k=0;k<4;++k)to.rotation[k]=q[k]*inv;}
   for(unsigned k=0;k<3;++k){to.scale[k]=from.scale[k]*(1-t)+to.scale[k]*t;to.position[k]=from.position[k]*(1-t)+to.position[k]*t;}
  }
 }
 copy(s.previous,bones,count);s.frame=frame;s.tick=tick;
}
}
