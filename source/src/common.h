#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <bcrypt.h>
using U8=uint8_t;using U16=uint16_t;using U32=uint32_t;using I32=int32_t;using UPTR=uintptr_t;
#include "mod_api.h"
#include "version.h"
template<class T> T& at(void*p,size_t n){return *reinterpret_cast<T*>(static_cast<U8*>(p)+n);}
template<class T> T fn(U8*g,U32 r){return reinterpret_cast<T>(g+r);}
// 공통 부품(ModCore) 연결 확인. 실패 사유까지 필요하면 r39::check_api 를 쓴다(창+기록).
inline bool valid_api(const S14_CoreAPI*a){return a&&a->size>=sizeof(*a)&&a->abi==S14_ABI&&a->resolved&&a->resolved->image_size==S14_GAME_IMAGE&&a->Readable&&a->InstallHooks&&a->MakeRelay&&a->FreeRelay;}
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
 static const U8 ce101[]={0x13,0xd1,0x6c,0x04,0xb6,0x46,0x19,0x97,0x30,0x7d,0x80,0x32,0xa7,0x10,0xc5,0x9e,0xe0,0xb0,0xd1,0x6e,0xb2,0x73,0xb2,0x8a,0x8c,0x20,0x87,0x6a,0x86,0x85,0xcd,0x42};
 static const U8 r39[]={0xbb,0x68,0xbe,0x42,0xa4,0xdb,0x9b,0x37,0x33,0xe5,0xbc,0x98,0xba,0x2f,0xa8,0x1e,0x62,0x49,0x64,0x41,0x58,0x2c,0x92,0x16,0x7a,0x2c,0x59,0xd1,0x58,0x7f,0xaa,0xde}; // R39: ce101 + 0x2ca11 관문/항구 본거지 대체 동굴(0x208470)
 static const U8 r40[]={0xae,0x36,0x5e,0xab,0x73,0x19,0x5e,0x23,0xcb,0x0e,0x3c,0xc3,0x86,0x1c,0x2b,0x8b,0x9c,0x90,0x97,0x50,0x5c,0xbf,0xb8,0x46,0x88,0x1e,0xc6,0xfb,0xbb,0x49,0x83,0xb7}; // R40: r39 + 전역 호환 검사에서 다른 모드의 함수 입구 우회 허용(0x2084e0)
 static const U8 r43a[]={0x9e,0x76,0xd6,0xb0,0x56,0x67,0xbd,0x53,0x9a,0x5b,0x7c,0x39,0x37,0x66,0xa9,0x94,0xd9,0xc6,0x55,0x7e,0x62,0x7a,0x0a,0x18,0x83,0x60,0x45,0x99,0x8a,0x81,0x80,0xb5}; // R43 정식판~5차 IC(6차에서 포로 표 주소 2곳 되돌리기 전) — 옛 IC 가 남은 PC 도 알아봄
 static const U8 r43c[]={0x47,0xa6,0x2d,0x84,0xa8,0x1a,0x66,0xb7,0x41,0xd9,0xfc,0x94,0xef,0x25,0xfe,0x45,0xd5,0x91,0x80,0xd8,0x28,0xb9,0xb6,0x6d,0x8c,0x32,0xde,0xc3,0x79,0xb5,0xb8,0x92}; // R43 9차: 시작 대기에서 2,346곳 전부 원본 조건 뺌(0x4b5c1)
 static const U8 r43[]={0xdf,0xa4,0xca,0x9f,0x6f,0x99,0xd7,0x61,0xbf,0xcf,0xb5,0x78,0x20,0xfc,0x0d,0xeb,0x7c,0xa9,0x6d,0xe8,0x1a,0xa6,0x76,0x0c,0xe6,0xf7,0xd5,0xb4,0xcc,0xd1,0xaf,0x11}; // R43: r40 + 새 게임 판(2026-11-05) 주소 바이트 교체(upd_tools/apply_new.py)
 HMODULE m=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");return (module_sha(m,p1)||module_sha(m,r11)||module_sha(m,ce101)||module_sha(m,r39)||module_sha(m,r40)||module_sha(m,r43)||module_sha(m,r43c)||module_sha(m,r43a))?reinterpret_cast<U8*>(m):nullptr;
}
// Game-only transactions remain owned by ModCore. Failed groups publish nothing.
struct HookPlan{
 const S14_CoreAPI* api;U8*game;S14_Hook hooks[32]{};U8*relays[32]{};U32 count=0;bool bad=false,committed=false;
 // R43: 처음 실패한 자리를 남긴다(오류창에 기대/실제 바이트를 적기 위해). why: 실패 종류(ASCII)
 U32 bad_rva=0,bad_len=0;U8 bad_expected[16]{},bad_actual[16]{};const char*bad_why="";
 HookPlan(const S14_CoreAPI*a):api(a),game(a->resolved->image){}
 ~HookPlan(){if(!committed)for(U32 i=0;i<count;i++)if(relays[i])api->FreeRelay(relays[i]);}
 void mark(U32 rva,const U8*expected,U32 length,const char*why,bool readable){
  if(bad)return;bad=true;bad_rva=rva;bad_len=length>16?16:length;bad_why=why;
  if(expected)memcpy(bad_expected,expected,bad_len);
  if(readable)memcpy(bad_actual,game+rva,bad_len);else memset(bad_actual,0,16);
 }
 void jump(U32 rva,const U8*expected,U32 length,void*dest,U8 opcode=0xe9){
  if(bad)return;
  if(count>=32){mark(rva,expected,length,"too many hooks(32)",false);return;}
  if(length<5||length>16){mark(rva,expected,length,"bad hook length",false);return;}
  auto&h=hooks[count];h.target=game+rva;h.length=length;memcpy(h.expected,expected,length);
  if(!api->Readable(h.target,length)){mark(rva,expected,length,"unreadable",false);return;}
  if(memcmp(h.target,expected,length)){mark(rva,expected,length,"bytes differ",true);return;}
  U8*relay=api->MakeRelay(h.target,dest);if(!relay){mark(rva,expected,length,"relay alloc failed",true);return;}
  relays[count]=relay;intptr_t delta=relay-(h.target+5);
  if(delta<INT32_MIN||delta>INT32_MAX){api->FreeRelay(relay);relays[count]=nullptr;mark(rva,expected,length,"relay too far",true);return;}
  memset(h.replacement,0x90,length);h.replacement[0]=opcode;I32 d=(I32)delta;memcpy(h.replacement+1,&d,4);count++;
 }
 void call(U32 at_rva,U32 original,void*dest){U8 e[5]={0xe8};I32 d=I32(original-at_rva-5);memcpy(e+1,&d,4);jump(at_rva,e,5,dest,0xe8);}
 bool install(const char*id){
  if(bad)return false;
  if(!count){bad=true;bad_why="no hooks planned";return false;}
  committed=api->InstallHooks(id,hooks,count)!=0;
  if(!committed){bad=true;bad_why="InstallHooks refused(ModCore)";bad_rva=(U32)(hooks[0].target-game);bad_len=hooks[0].length;memcpy(bad_expected,hooks[0].expected,bad_len);memcpy(bad_actual,hooks[0].target,bad_len);}
  return committed;
 }
};
