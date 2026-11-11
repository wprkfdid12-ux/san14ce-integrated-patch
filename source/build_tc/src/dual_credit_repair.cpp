#include "common.h"
#include "dual_credit_codec.h"
extern "C" {
 UPTR MeritReadReturn,MeritWriteReturn;
 void MeritReadBridge();void MeritWriteBridge();
 __declspec(dllexport) void MeritEncode(U8*out,const U8*in){merit_encode(out,in);}
 __declspec(dllexport) void MeritDecode(U8*out,const U8*in){merit_decode(out,in);}
 __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*a){
  if(!valid_api(a))return 0;U8*g=a->resolved->image;
  MeritReadReturn=(UPTR)(g+0x2399d7);MeritWriteReturn=(UPTR)(g+0x23998f);
  const U8 rd[]={0x66,0x44,0x89,0x7c,0x24,0x20,0x41,0x0f,0xb7,0xc7};
  const U8 wr[]={0x45,0x8b,0xc7,0x41,0x8b,0xcf,0x41,0x0f,0xb6,0xbc,0x24,0xc2,0x01,0x00,0x00};
  HookPlan p(a);p.jump(0x2398e3,rd,sizeof(rd),(void*)MeritReadBridge);p.jump(0x239934,wr,sizeof(wr),(void*)MeritWriteBridge);
  bool ok=p.install("SAN14CE.DualCreditSaveRepair");
  if(a->Log)a->Log(ok?"DualCredit: 3-byte compatible codec installed; saved pairs remain readable with feature OFF.":"DualCredit: signature/conflict failure; no serializer hooks changed.");return ok;
 }
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
