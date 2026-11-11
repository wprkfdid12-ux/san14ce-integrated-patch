#pragma once
#include "common.h"
#include "r39_record.h"
// Called only by the settings UI thread, after the known module has initialized.
struct ModuleTransaction{
 struct Row{U8*at;U32 size;U8 before[16],after[16];DWORD protect;bool writable;};
 // R43: 처음 실패한 자리(오류창에 기대/실제 바이트)
 struct Bad{U8*at;U32 n;U8 ex[16],ac[16];const char*why;bool set;};
 Row rows[8]{};void*blocks[8]{};unsigned count=0,allocated=0;bool bad=false,committed=false;Bad badinfo{};
 ~ModuleTransaction(){if(!committed)for(unsigned i=0;i<allocated;i++)VirtualFree(blocks[i],0,MEM_RELEASE);}
 void mark(void*target,const void*expected,U32 n,const char*why,bool readable=true){
  if(bad)return;bad=true;badinfo.set=true;badinfo.at=(U8*)target;badinfo.n=n>16?16:n;badinfo.why=why;
  if(expected)memcpy(badinfo.ex,expected,badinfo.n);else memset(badinfo.ex,0,16);
  if(readable&&target)memcpy(badinfo.ac,target,badinfo.n);else memset(badinfo.ac,0,16);
 }
 void bytes(void*target,const void*before,const void*after,U32 n){
  if(bad)return;
  if(count==8){mark(target,before,n,"too many rows(8)");return;}if(n>16||!n){mark(target,before,n,"bad row length",false);return;}
  auto&r=rows[count++];r.at=(U8*)target;r.size=n;memcpy(r.before,before,n);memcpy(r.after,after,n);
  if(memcmp(r.at,before,n))mark(target,before,n,"bytes differ");
 }
 void pointer(void*target,void*old,void*value){bytes(target,&old,&value,8);}
 static void abs_jump(U8*at,void*dest){const U8 ins[]={0xff,0x25,0,0,0,0};memcpy(at,ins,6);memcpy(at+6,&dest,8);}
 void* detour(U8*target,const U8*expected,U32 n,void*dest){
  if(bad)return nullptr;
  if(allocated==8){mark(target,expected,n,"too many detours(8)");return nullptr;}
  if(n<5||n>16){mark(target,expected,n,"bad detour length",false);return nullptr;}
  if(memcmp(target,expected,n)){mark(target,expected,n,"bytes differ");return nullptr;}
  U8*block=nullptr;UPTR start=(UPTR)target&~UPTR(0xffff);
  for(UPTR offset=0x10000;!block&&offset<0x40000000;offset+=0x10000){
   block=(U8*)VirtualAlloc((void*)(start+offset),0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
   if(!block&&start>offset)block=(U8*)VirtualAlloc((void*)(start-offset),0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
  }
  if(!block){mark(target,expected,n,"near block alloc failed");return nullptr;}blocks[allocated++]=block;
  intptr_t delta=block-(target+5);if(delta<INT32_MIN||delta>INT32_MAX){mark(target,expected,n,"near block too far");return nullptr;}
  abs_jump(block,dest);
  if(expected[0]==0xe9){I32 old;memcpy(&old,expected+1,4);abs_jump(block+16,target+5+old);}
  else {memcpy(block+16,expected,n);abs_jump(block+16+n,target+n);}
  if(!FlushInstructionCache(GetCurrentProcess(),block,16+n+14)){mark(target,expected,n,"FlushInstructionCache failed");return nullptr;}
  U8 patch[16];memset(patch,0x90,n);patch[0]=0xe9;I32 d=(I32)delta;memcpy(patch+1,&d,4);bytes(target,expected,patch,n);return block+16;
 }
 // R3: code bytes may be executed by another thread while they are rewritten
 // (the start-up workers patch from their own thread). Rows that fit in one aligned
 // qword are swapped with one atomic exchange; longer rows starting on a qword write the
 // first qword atomically (it holds the new jump) and only then the dead tail bytes.
 // Restoring writes the tail first and the head last, for the same reason.
 static void put(U8*at,const U8*bytes,U32 n,bool restoring){
  UPTR a=(UPTR)at,q=a&~UPTR(7);
  if(((a+n-1)&~UPTR(7))==q){
   volatile long long*p=(volatile long long*)q;long long old=__atomic_load_n(p,__ATOMIC_ACQUIRE),next;
   do{next=old;memcpy((U8*)&next+(a-q),bytes,n);}while(!__atomic_compare_exchange_n(p,&old,next,false,__ATOMIC_SEQ_CST,__ATOMIC_ACQUIRE));
   return;
  }
  if(!(a&7)&&n>8&&n<=16){
   long long head;memcpy(&head,bytes,8);
   if(restoring)memcpy(at+8,bytes+8,n-8);
   __atomic_store_n((volatile long long*)at,head,__ATOMIC_SEQ_CST);
   if(!restoring)memcpy(at+8,bytes+8,n-8);
   return;
  }
  memcpy(at,bytes,n);
 }
 bool install(){
  if(bad)return false;if(!count){mark(nullptr,nullptr,0,"no rows planned",false);return false;}
  for(unsigned i=0;i<count;i++){
   auto&r=rows[i];if(memcmp(r.at,r.before,r.size)){mark(r.at,r.before,r.size,"bytes changed before install");break;}
   if(!VirtualProtect(r.at,r.size,PAGE_EXECUTE_READWRITE,&r.protect)){mark(r.at,r.before,r.size,"VirtualProtect failed");break;}r.writable=true;
  }
  if(!bad)for(unsigned i=0;i<count;i++)if(memcmp(rows[i].at,rows[i].before,rows[i].size)){mark(rows[i].at,rows[i].before,rows[i].size,"bytes changed during install");break;}
  if(!bad){for(unsigned i=0;i<count;i++){auto&r=rows[i];put(r.at,r.after,r.size,false);}committed=true;
   for(unsigned i=0;i<count;i++){auto&r=rows[i];if(!FlushInstructionCache(GetCurrentProcess(),r.at,r.size)){bad=true;badinfo.set=true;badinfo.at=r.at;badinfo.n=r.size;memcpy(badinfo.ex,r.before,r.size);memcpy(badinfo.ac,r.after,r.size);badinfo.why="FlushInstructionCache failed";}}
   if(bad){for(unsigned i=0;i<count;i++){auto&r=rows[i];put(r.at,r.before,r.size,true);FlushInstructionCache(GetCurrentProcess(),r.at,r.size);}committed=false;}
  }
  for(unsigned i=0;i<count;i++){auto&r=rows[i];if(r.writable){DWORD unused;VirtualProtect(r.at,r.size,r.protect,&unused);}}
  return committed;
 }
};
// R43: 거래 실패 보고(어느 모듈 안 자리인지 base 기준 오프셋으로)
namespace r39{
inline void fail_tx(int feature,const char*module,const ModuleTransaction::Bad&b,const U8*base,const char*base_name,const char*what,const wchar_t*effect){
 char where[160],ex[96],ac[96];where[0]=ex[0]=ac[0]=0;
 scat(where,sizeof where,what?what:"site");
 if(b.set&&b.at){scat(where,sizeof where," @ ");scat(where,sizeof where,base_name?base_name:"mod");scat(where,sizeof where,"+0x");shex(where,sizeof where,(unsigned long long)(b.at-base));}
 scat(where,sizeof where," (");scat(where,sizeof where,b.set&&b.why?b.why:"install returned 0");scat(where,sizeof where,")");
 if(b.set&&b.n){sbytes(ex,sizeof ex,b.ex,b.n);sbytes(ac,sizeof ac,b.ac,b.n);}else{scat(ex,sizeof ex,"-");scat(ac,sizeof ac,"-");}
 fail(feature,module,"bytes",where,ex,ac,effect);
}
}
