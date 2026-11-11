// R39 33번: 안행·누선 / 투석 사거리 설정 — 아군·적·AI 모두 같은 값.
//  공식 1.0.1 (S14_GAME_STAMP) 전용. ModCore 묶음 "SAN14CE.Range" (14곳, 전부 아니면 전무).
//  게임 원리(코드·자료로 확인):
//   - 진형 객체 +0xa6 bit0 = 원거리(fixdataexce.s14: 안행(3)·누선(13)=1, 정란·충차·투석(8~10)=2(공성 bit1), 나머지 0)
//   - 0x2c9080(세력, 진형) = 원거리 여부(0/1). 끝에서 세력+0x193 bit7 이면 투석(10)도 원거리.
//   - 사용처 7곳이 그 값을 "사거리 = 1 + 원거리" 로 바꿔 씀(0x1f8bc, 0x16dced, 0x16dd14, 0x170336, 0x2cb976, 0x2ccad1, 0x88cff9).
//   - 명령 판정 0x2c6f80 은 고정 칸 목록(반경 1/2/3)으로 판정 → 거리 판정으로 바꿔 줌(P1~P4), 원거리 표식은 r8 에 실어 나름(P0).
//   - 적 찾기 0x2b5330 반경 3 → 사거리가 4 이상이면 그만큼(P5).
//  표준: 안행·누선 2칸, 투석 1칸(원거리 아님; 세력 해금 bit7 이면 2칸).
//  설정: SAN14Mods\modules\Range.ini [Range] BowNaval=2..4, Catapult=1..4(1=표준), Enabled=1.
#include "common.h"
#include "r39_record.h"
static const S14_CoreAPI*core;static U8*game;
static volatile long enabled=1,bow=2,cat=1;
static const int kBowMin=2,kBowMax=4,kCatMin=1,kCatMax=4;
using FnI_P=int(*)(void*);using FnP_P=void*(*)(void*);using FnI_PP=int(*)(void*,void*);using FnI_II=int(*)(int,int);
static U8*db(){U8*d=*(U8**)(game+0x2780f60);return d;}
static int form_id(U8*f){
 U8*d=db();if(!d||!f)return -1;U8*base=*(U8**)(d+0x77428);if(!base||f<base)return -1;
 UPTR off=(UPTR)(f-base);if(off%0xc8||off/0xc8>20)return -1;return (int)(off/0xc8);
}
static bool army_ok(U8*u){
 U8*d=db();if(!d||!u)return false;U8*base=*(U8**)(d+0x7e838);if(!base||u<base)return false;
 UPTR off=(UPTR)(u-base);return off%0x200==0&&off/0x200<=500; // 게임 부대 배열 안 칸이면 유효. VirtualQuery 안 씀(AI 판정마다 불려 매우 잦음 → 턴 진행이 크게 느려지던 원인)
}
static int range_for(int id,int flag){
 int native=flag?2:1;if(!enabled)return native;
 if((id==3||id==13)&&flag)return bow;
 if(id==10&&cat>native)return cat;
 return native;
}
static int dist(int a,int b){return fn<FnI_II>(game,0x2acb80)(a,b);}
static int ring_member(int center,int cell,int r){
 if((unsigned)center>0xbd0f||(unsigned)cell>0xbd0f)return 0;
 int d=dist(center,cell);return (d>=1&&d<=r)?1:2;
}
static volatile long hits[8];
extern "C"{
// 0x2c9109: 0x2c9080 끝. 투석 사거리를 바꿨으면 투석도 원거리로 본다.
int RangeTail(U8*f,int flag){if(enabled&&cat>1&&form_id(f)==10){InterlockedIncrement(&hits[0]);return 1;}return flag;}
// C1·C2·C3·C4·C6·C7: 사거리 값 그대로
int RangeFor(U8*f,int flag){int id=form_id(f);int r=range_for(id,flag!=0);if(r!=(flag?2:1))InterlockedIncrement(&hits[1]);return r;}
// C5: 원거리면 edi += 사거리-1
int RangeC5(U8*f,int flag){if(!flag)return 0;return range_for(form_id(f),1)-1;}
// P0: 0x2c9424. 원거리이고 사거리가 2가 아니면 r8 = 0x100|사거리 (0이 아니므로 게임의 test r8d 판정은 그대로)
unsigned long long RangeP0(U8*unit,int flag){
 if(!flag||!enabled||!army_ok(unit))return (unsigned)flag;
 int id=fn<FnI_P>(game,0x22cb80)(unit);int r=range_for(id,1);
 return r==2?(unsigned)flag:(0x100u|(unsigned)r);
}
// P1·P2: r8 에 실린 사거리로 거리 판정. 0=원래 코드, 1=범위 안, 2=범위 밖. base 2 = 반경 (1+사거리), base 1 = 반경 사거리
int RangeP12(unsigned long long r8,int center,int cell,int base){
 if((r8>>3)!=0x20)return 0;int r=(int)(r8&7);InterlockedIncrement(&hits[2]);return ring_member(center,cell,base-1+r);
}
// P3·P4: 부대(rbx)에서 사거리. packed = base | 대상종류<<8. P4 에서 대상이 도시(5)면 원거리 아닌 길이라 원래 코드
int RangeP34(U8*unit,int center,int cell,int packed){
 int base=packed&0xff,kind=(packed>>8)&0xff;if(!enabled)return 0;
 if(base==1&&kind==5)return 0;
 if(!army_ok(unit))return 0;
 int r=range_for(fn<FnI_P>(game,0x22cb80)(unit),1);if(r==2)return 0;
 InterlockedIncrement(&hits[3]);return ring_member(center,cell,base-1+r);
}
// P5: 0x2b76e2 적 찾기 반경. 3 이고 이 부대 사거리가 4 이상이면 사거리로
int RangeP5(U8*unit,int radius){
 if(radius!=3||!enabled||!army_ok(unit))return radius;
 if(bow<4&&cat<4)return radius;
 U8*f=(U8*)fn<FnP_P>(game,0x22cad0)(unit);U8*force=(U8*)fn<FnP_P>(game,0x22e2a0)(unit);
 if(!f||!force||!core->Readable(force,0x200)||!core->Readable(f,0xc8))return radius;
 int flag=fn<FnI_PP>(game,0x2c9080)(force,f);int r=range_for(form_id(f),flag!=0);
 if(r>3){InterlockedIncrement(&hits[4]);return r;}return radius;
}
void*RangeRetT,*RangeRetC1,*RangeRetC2,*RangeRetC3,*RangeRetC4,*RangeRetC5,*RangeRetC6,*RangeRetC7,*RangeRetP0,*RangeRetP5;
void*RangeRetP1,*RangeRetP2,*RangeRetP3,*RangeRetP4,*RangeInP1,*RangeInP3,*RangeOut;
void RangeBridgeT();void RangeBridgeC1();void RangeBridgeC2();void RangeBridgeC3();void RangeBridgeC4();void RangeBridgeC5();void RangeBridgeC6();void RangeBridgeC7();
void RangeBridgeP0();void RangeBridgeP1();void RangeBridgeP2();void RangeBridgeP3();void RangeBridgeP4();void RangeBridgeP5();
}
// ---- 설정 파일
static bool ini_path(wchar_t*p){if(!r39::game_root(p,1024))return false;r39::wcat(p,1024,L"SAN14Mods\\modules\\Range.ini");return true;}
static void wnum(wchar_t*o,int v){wchar_t b[8];int k=0;if(v<0)v=0;do{b[k++]=(wchar_t)(L'0'+v%10);v/=10;}while(v&&k<7);int j=0;while(k)o[j++]=b[--k];o[j]=0;}
static void save_ini(){
 wchar_t p[1024],v[8];if(!ini_path(p))return;
 wnum(v,bow);WritePrivateProfileStringW(L"Range",L"BowNaval",v,p);
 wnum(v,cat);WritePrivateProfileStringW(L"Range",L"Catapult",v,p);
 wnum(v,enabled?1:0);WritePrivateProfileStringW(L"Range",L"Enabled",v,p);
}
static void load_ini(){
 wchar_t p[1024];if(!ini_path(p))return;
 int b=(int)GetPrivateProfileIntW(L"Range",L"BowNaval",2,p),c=(int)GetPrivateProfileIntW(L"Range",L"Catapult",1,p);
 bow=(b>=kBowMin&&b<=kBowMax)?b:2;cat=(c>=kCatMin&&c<=kCatMax)?c:1;
 enabled=GetPrivateProfileIntW(L"Range",L"Enabled",1,p)?1:0;
}
static void settings_text(char*o,size_t cap){o[0]=0;r39::scat(o,cap,"BowNaval=");r39::snum(o,cap,(unsigned)bow);r39::scat(o,cap,";Catapult=");r39::snum(o,cap,(unsigned)cat);if(!enabled)r39::scat(o,cap,";Enabled=0");}
// ---- 내보내기
extern "C" __declspec(dllexport) int SAN14_RangeSet(int which,int cells){
 if(which==0){if(cells<kBowMin||cells>kBowMax)return 0;InterlockedExchange(&bow,cells);}
 else if(which==1){if(cells<kCatMin||cells>kCatMax)return 0;InterlockedExchange(&cat,cells);}
 else return 0;
 save_ini();return 1;
}
extern "C" __declspec(dllexport) int SAN14_RangeGet(int which){
 if(which==0)return bow;if(which==1)return cat;
 if(which==2)return kBowMin|(kBowMax<<8);if(which==3)return kCatMin|(kCatMax<<8);  // 범위(설정창용)
 if(which==4)return 2;if(which==5)return 1;                                        // 표준값
 return -1;
}
extern "C" __declspec(dllexport) void SAN14_RangeSetEnabled(int on){InterlockedExchange(&enabled,on?1:0);save_ini();}
extern "C" __declspec(dllexport) int SAN14_RangeState(){return enabled;}
extern "C" __declspec(dllexport) long SAN14_RangeHits(int i){return i>=0&&i<8?hits[i]:-1;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,33,"SAN14_Range.dll"))return 0;core=api;game=api->resolved->image;
 load_ini();
 HookPlan plan(api);
 static const U8 eT[]={0x8b,0xc3,0x48,0x8b,0x5c,0x24,0x40};                         RangeRetT=game+0x2c9109 +7;plan.jump(0x2c9109,eT,7,(void*)&RangeBridgeT);
 static const U8 e1[]={0xbf,0x01,0x00,0x00,0x00,0xb9,0x02,0x00,0x00,0x00,0x85,0xc0,0x0f,0x45,0xf9};RangeRetC1=game+0x1f8bc +15;plan.jump(0x1f8bc,e1,15,(void*)&RangeBridgeC1);
 static const U8 e2[]={0x85,0xc0,0x0f,0x95,0xc3,0xff,0xc3};                         RangeRetC2=game+0x16dced +7;plan.jump(0x16dced,e2,7,(void*)&RangeBridgeC2);
 static const U8 e3[]={0x85,0xc0,0x0f,0x95,0xc3,0xff,0xc3};                         RangeRetC3=game+0x16dd14 +7;plan.jump(0x16dd14,e3,7,(void*)&RangeBridgeC3);
 static const U8 e4[]={0x33,0xdb,0x85,0xc0,0x0f,0x95,0xc3,0xff,0xc3};               RangeRetC4=game+0x170336 +9;plan.jump(0x170336,e4,9,(void*)&RangeBridgeC4);
 static const U8 e5[]={0x85,0xc0,0x74,0x02,0xff,0xc7};                              RangeRetC5=game+0x2cb976 +6;plan.jump(0x2cb976,e5,6,(void*)&RangeBridgeC5);
 static const U8 e6[]={0x85,0xc0,0xb9,0x01,0x00,0x00,0x00,0xbd,0x02,0x00,0x00,0x00}; RangeRetC6=game+0x2ccad1 +12;plan.jump(0x2ccad1,e6,12,(void*)&RangeBridgeC6);
 static const U8 e7[]={0x33,0xc9,0x85,0xc0,0x0f,0x95,0xc1,0xff,0xc1};               RangeRetC7=game+0x88cff9 +9;plan.jump(0x88cff9,e7,9,(void*)&RangeBridgeC7);
 static const U8 p0[]={0x44,0x0f,0xb6,0x73,0x3c,0x44,0x8b,0xc0};                    RangeRetP0=game+0x2c9424 +8;plan.jump(0x2c9424,p0,8,(void*)&RangeBridgeP0);
 RangeInP1=game+0x2c9607;RangeInP3=game+0x2c9772;RangeOut=game+0x2c97ae;
 static const U8 p1[]={0x41,0xb8,0x03,0x00,0x00,0x00,0x48,0x8d,0x4c,0x24,0x40};      RangeRetP1=game+0x2c94aa +11;plan.jump(0x2c94aa,p1,11,(void*)&RangeBridgeP1);
 static const U8 p2[]={0x41,0xb8,0x02,0x00,0x00,0x00,0x48,0x8d,0x4c,0x24,0x40};      RangeRetP2=game+0x2c95e1 +11;plan.jump(0x2c95e1,p2,11,(void*)&RangeBridgeP2);
 static const U8 p3[]={0x44,0x8d,0x46,0xfe,0x48,0x8d,0x4c,0x24,0x40};                RangeRetP3=game+0x2c96dc +9;plan.jump(0x2c96dc,p3,9,(void*)&RangeBridgeP3);
 static const U8 p4[]={0x48,0x8d,0x4c,0x24,0x40,0x41,0xb8,0x02,0x00,0x00,0x00};      RangeRetP4=game+0x2c974f +11;plan.jump(0x2c974f,p4,11,(void*)&RangeBridgeP4);
 static const U8 p5[]={0x41,0xb9,0x10,0xbd,0x00,0x00,0x44,0x8b,0xc3};                RangeRetP5=game+0x2b76e2 +9;plan.jump(0x2b76e2,p5,9,(void*)&RangeBridgeP5);
 bool ok=plan.install("SAN14CE.Range");
 if(core->Log)core->Log(ok?"Range R39: 14 sites hooked (bow/naval & catapult attack range).":"Range R39: hook group NOT installed (bytes differ / conflict). Nothing changed.");
 char st[96];settings_text(st,sizeof st);
 r39::start(33,"SAN14_Range.dll",ok?(enabled?"OK":"OFF"):"FAIL",ok?14:0,14,st,ok?"-":"hook bytes differ or another mod patched the site");
 if(!ok)r39::fail_plan(33,"SAN14_Range.dll",plan,"range sites(14) first failing",L"안행·누선/투석 사거리(33번)가 적용 안 됨 → 게임 기본 사거리. 다른 기능은 그대로");
 return ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
