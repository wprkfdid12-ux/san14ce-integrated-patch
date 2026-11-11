// R41e A8 명품 목록 그림: 명품 창(수여·몰수 등)의 명품 선택 목록 왼쪽 그림 칸이 명품 그림 대신 보유 무장(군주) 얼굴로 나오던 것.
//  원인(게임 코드): 명품 선택 목록(0x686100 → CDataSelectState<CItemData>)은 설정 구조 +0x114 가 켜져 있어야
//   그림 칸을 "명품 그림"(람다 0x561f40: 명품+0x51(+0xc9))으로 만들고, 꺼져 있으면 "보유 무장 얼굴"(람다 0x65eee0 → 0x5a2ff0: 명품+0x4a 무장 얼굴)로 만듦.
//   (목록 설정 0x4d8fa3: 목록+0x26c=설정+0x114, 그림 칸 만들기 0x55c8f0, 그림 얻기 선택 0x4ce8f1)
//   게임의 다른 명품 목록(0x556624·0x612c4d 등)은 +0x114=1 을 넣어 명품 그림을 쓰는데, 명품 창의 목록 3곳(0x747ca6·0x7486db·0x750e2e)만 안 넣음.
//  고침: 0x686100 입구(9바이트)에서, 부른 쪽이 그림 방식(+0x114/+0x118/+0x11c)을 하나도 안 정했으면 +0x114=1 로 정함. 그 밖엔 손대지 않음.
//   30번 다중선택(체크)·한 개 고르기 둘 다 같은 함수라 둘 다 명품 그림이 나옴.
//  설정: SAN14Mods\modules\ItemPicture.ini [ItemPicture] Enabled=1
#include "common.h"
#include "r39_record.h"
static const S14_CoreAPI*core;static U8*game;static volatile long enabled=1;static volatile long uses,set_count;
extern "C"{void ItBridge();void*ItRet;}
extern "C" void ItFix(U8*cfg){
 if(!cfg||!enabled)return;
 long n=InterlockedIncrement(&uses);
 if(at<U32>(cfg,0x114)||at<U32>(cfg,0x118)||at<U32>(cfg,0x11c))return; // 부른 쪽이 정한 그림 방식은 그대로
 at<U32>(cfg,0x114)=1;long s=InterlockedIncrement(&set_count);
 if(s==1||s%20==0)r39::counter(30,"SAN14_ItemPicture.dll","item_picture_set",(unsigned)s);
 (void)n;
}
extern "C" __declspec(dllexport) void*SAN14_ItemPictureTestTable(int i){void*t[]={(void*)&ItBridge,(void*)&ItFix,(void*)&ItRet,(void*)&uses,(void*)&set_count,(void*)&enabled};return i>=0&&i<6?t[i]:nullptr;}
extern "C" __declspec(dllexport) void SAN14_ItemPictureSetEnabled(int on){InterlockedExchange(&enabled,on?1:0);}
extern "C" __declspec(dllexport) int SAN14_ItemPictureState(){return enabled;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,30,"SAN14_ItemPicture.dll"))return 0;core=api;game=api->resolved->image;
 wchar_t ini[1024];if(r39::game_root(ini,1024)){r39::wcat(ini,1024,L"SAN14Mods\\modules\\ItemPicture.ini");enabled=GetPrivateProfileIntW(L"ItemPicture",L"Enabled",1,ini)?1:0;}
 HookPlan plan(api);
 static const U8 e1[]={0x40,0x57,0x48,0x81,0xec,0x80,0x04,0x00,0x00};ItRet=game+0x686100+9;plan.jump(0x686100,e1,9,(void*)&ItBridge);
 bool ok=plan.install("SAN14CE.ItemPicture");
 if(core->Log)core->Log(ok?"ItemPicture R41e: item select list (0x686100) shows item pictures instead of the holder's face.":"ItemPicture R41e: hook NOT installed (bytes differ / conflict). Item list unchanged.");
 r39::counter(30,"SAN14_ItemPicture.dll",ok?"item_picture_hooked":"item_picture_hook_failed",ok?1:0);
 // 22차(P230): 시작 줄은 셈 줄과 따로 — 20차에 셈 줄을 본판에서 빼자 진단(Diag)이 "시작 기록 없음"으로 봤음
 r39::start(30,"SAN14_ItemPicture.dll",ok?"OK":"FAIL",ok?1:0,1,enabled?"Enabled=1":"Enabled=0",ok?"-":"item select list hook not installed");
 if(!ok)r39::fail_plan(30,"SAN14_ItemPicture.dll",plan,"item select list fn entry exe+0x686100",L"보물 선택 목록의 보물 그림(30번 ItemPicture)이 안 나옴 → 게임 기본(소유자 얼굴). 다른 기능은 그대로");
 return ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
