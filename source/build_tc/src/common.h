#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <bcrypt.h>
using U8=uint8_t;using U16=uint16_t;using U32=uint32_t;using I32=int32_t;using UPTR=uintptr_t;
#include "mod_api.h"
template<class T> T& at(void*p,size_t n){return *reinterpret_cast<T*>(static_cast<U8*>(p)+n);}
template<class T> T fn(U8*g,U32 r){return reinterpret_cast<T>(g+r);}
inline bool valid_api(const S14_CoreAPI*a){return a&&a->size>=sizeof(*a)&&a->abi==S14_ABI&&a->resolved&&a->resolved->image_size==0x29ff000&&a->Readable&&a->InstallHooks&&a->MakeRelay&&a->FreeRelay;}
inline bool module_sha(HMODULE m,const U8 expected[32]){
 wchar_t path[1024];if(!m||!GetModuleFileNameW(m,path,1024))return false;
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE a=0;BCRYPT_HASH_HANDLE h=0;U8 digest[32]={},buf[8192];DWORD n=0;bool ok=false;
 if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,0,0)>=0&&BCryptCreateHash(a,&h,0,0,0,0,0)>=0){
  ok=true;for(;;){if(!ReadFile(f,buf,sizeof(buf),&n,0)){ok=false;break;}if(!n)break;if(BCryptHashData(h,buf,n,0)<0){ok=false;break;}}
  if(BCryptFinishHash(h,digest,32,0)<0)ok=false;
 }
 if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);CloseHandle(f);
 return ok&&!memcmp(digest,expected,32);
}
// Two accepted common-module builds: P1 (R2 baseline) and R11 (P1 + 11-byte menu-help
// compatibility patch; identical code layout, so every offset below applies to both).
inline U8* known_common(){
 static const U8 p1[]={0xfa,0x07,0xcd,0x5f,0xdf,0x5c,0x26,0xab,0x6d,0xec,0xce,0xb9,0x33,0xf6,0x25,0x6a,0x73,0xd2,0xd0,0xfc,0xf0,0x5e,0x95,0x10,0x2f,0x82,0x81,0x2f,0x34,0x15,0xda,0x0d};
 static const U8 r11[]={0x9c,0x50,0xb7,0x3b,0xdf,0xe1,0x3d,0xbf,0xd9,0xbf,0xea,0x0f,0x3d,0xeb,0x56,0x08,0x82,0xd6,0x89,0x2a,0xea,0x45,0xdf,0x78,0xb1,0x3c,0x57,0xf4,0xa8,0xd7,0xc2,0x82};
 HMODULE m=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");return (module_sha(m,p1)||module_sha(m,r11))?reinterpret_cast<U8*>(m):nullptr;
}
// Game-only transactions remain owned by ModCore. Failed groups publish nothing.
struct HookPlan{
 const S14_CoreAPI* api;U8*game;S14_Hook hooks[32]{};U8*relays[32]{};U32 count=0;bool bad=false,committed=false;
 HookPlan(const S14_CoreAPI*a):api(a),game(a->resolved->image){}
 ~HookPlan(){if(!committed)for(U32 i=0;i<count;i++)if(relays[i])api->FreeRelay(relays[i]);}
 void jump(U32 rva,const U8*expected,U32 length,void*dest,U8 opcode=0xe9){
  if(bad||count>=32||length<5||length>16){bad=true;return;}
  auto&h=hooks[count];h.target=game+rva;h.length=length;memcpy(h.expected,expected,length);
  if(!api->Readable(h.target,length)||memcmp(h.target,expected,length)){bad=true;return;}
  U8*relay=api->MakeRelay(h.target,dest);if(!relay){bad=true;return;}
  relays[count]=relay;intptr_t delta=relay-(h.target+5);
  if(delta<INT32_MIN||delta>INT32_MAX){api->FreeRelay(relay);relays[count]=nullptr;bad=true;return;}
  memset(h.replacement,0x90,length);h.replacement[0]=opcode;I32 d=(I32)delta;memcpy(h.replacement+1,&d,4);count++;
 }
 void call(U32 at_rva,U32 original,void*dest){U8 e[5]={0xe8};I32 d=I32(original-at_rva-5);memcpy(e+1,&d,4);jump(at_rva,e,5,dest,0xe8);}
 bool install(const char*id){if(bad||!count)return false;committed=api->InstallHooks(id,hooks,count)!=0;return committed;}
};
