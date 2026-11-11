// R42 B2·B3 (4번 연출 생략): 등용 결과 대사 장면 / 탐색 명품 발견 창을 띄우지 않고 기록만 남김.
//  게임 흐름(1.0.1 = S14_GAME_STAMP, 근거는 notes_b23.md):
//   B2 등용 명령 실행 CCmdActionEmploy 슬롯20 0x1dabf0 / CCmdActionEmployWait 슬롯20 0x1db8e0
//     → 성공 판정 0x2784b0 → (실행 세력이 플레이어이고 대상이 아직 우리 세력이 아닐 때만) 0x6c6b10(&{실행자*,대상*,성공}) 이
//       대사 장면 CStrategyEmployTalkState(vt 0x1440ff8)를 쌓음(0x6873c0) → 그 뒤에 같은 함수가 결과 처리(0x2717d0 소속 변경·
//       0x2ede90·0x2b9440 등)와 게임 자체 이력(0x936980, 제목 0xc3f1 성공/0xc3f2 실패)을 이어서 함. 장면 상태 안에서는 화면만 그림.
//     → 0x6c6b10 입구에서 바로 돌아오면 장면만 빠지고 결과는 원작 그대로(게임 자체 '건너뛰기'(0x335110)도 같은 식으로 그냥 돌아옴).
//   B3 탐색 실행 CCmdActionSearch 슬롯20 0x1dc320 → 명품 발견(0x27f6e0) → 소유 변경 0x323ed0 → (플레이어만) 0x6c6dd0(&{종류0,실행자*,곳,명품*,...})
//       이 발견 창 CSearchDiscoveryState(vt 0x14471e8, 안의 CDiscoveryItemDialog 0x1447030)를 쌓음 → 이어서 게임 자체 이력(0xc39d/0x51bd).
//     → 0x6c6dd0 입구에서 종류 0(명품)일 때만 바로 돌아옴. 종류 1(탐색 인재 발견·등용)·3·4·5(명승 영상 뒤)는 원래대로.
//  두 자리 모두 입구 6바이트(40 57 48 83 ec 70 = push rdi; sub rsp,0x70). AI 모드 v13.3 편의 DLL 해시 48곳과 안 겹침.
//  기록: 행동보고(이력) 한 줄은 본체 scene_skip.cpp 의 SsAddHistory, 상태 기록은 적용확인.log (R39C 4 / R42B 4 줄).
#include "common.h"
#include "r39_record.h"

extern "C" void SsAddHistory(const wchar_t*line);   // 본체(scene_skip.cpp)가 구현: 행동보고 중요 표시 한 줄
extern "C" void SsCollect(int cat,const wchar_t*line,int sub); // 본체: 턴 끝 결과 창 모음(1 등용 결과, 2 보물 발견)

static const S14_CoreAPI*B23api;static U8*B23g;
static volatile long*emp_on;static volatile long*tre_on;
static volatile long emp_skips,tre_skips,b23_text_fail;
extern "C"{void SsEmpBridge();void SsTreBridge();void*SsEmpRet;void*SsTreRet;}

static const U8 kEntry[6]={0x40,0x57,0x48,0x83,0xec,0x70};
static bool ok_ptr(const void*p,UPTR n){return p&&B23api&&B23api->Readable(p,n);}

// ── 게임 메시지 표(0x1a26440)로 문장 만들기 ─────────────────────────────
//  인자 비우기 0x30c380, 무장 0x30fc40(id,칸), 명품 0x30fcb0(명품+0x10,칸), 지역 0x30fa60(id,칸), 도시 0x30f950(id,칸),
//  틀 vt[1](id), 채우기 0x197e50(0x1970e0(), 버퍼, 크기, L"%s", 틀). 게임 자신의 이력 문장 만들기와 같은 순서.
//  (불러 준 쪽 게임 코드는 다음 문장을 만들기 전에 늘 0x30c380 으로 인자를 비우므로 여기서 인자를 바꿔도 영향 없음)
static U8*msg(){return B23g+0x1a26440;}
static void a_reset(){fn<void(*)(void*)>(B23g,0x30c380)(msg());}
static void a_person(U8*p,int slot){if(ok_ptr(p,0x12))fn<void(*)(void*,U32,U32)>(B23g,0x30fc40)(msg(),at<U16>(p,0x10),(U32)slot);}
static void a_item(U8*it,int slot){if(ok_ptr(it,0x20))fn<void(*)(void*,void*,U32)>(B23g,0x30fcb0)(msg(),it+0x10,(U32)slot);}
static bool render(U32 id,wchar_t*out,U32 cap){
 out[0]=0;
 const wchar_t*t=((const wchar_t*(*)(void*,U32))((*(void***)msg())[1]))(msg(),id);
 if(!t||!*t)return false;
 void*f=fn<void*(*)()>(B23g,0x1970e0)();
 fn<void(*)(void*,wchar_t*,U32,const wchar_t*,const wchar_t*)>(B23g,0x197e50)(f,out,cap,(const wchar_t*)(B23g+0x139f770),t);
 out[cap-1]=0;
 for(U32 i=0;out[i];i++)if(out[i]==L'\r'||out[i]==L'\n')out[i]=L' ';
 return out[0]!=0;
}
static U8*force_of(U8*person){return ok_ptr(person,0xe2)?fn<U8*(*)(U8*)>(B23g,0x22e450)(person):nullptr;}

static void state_line(const char*what,U8*a,U8*b,int v){
 char l[160];l[0]=0;r39::scat(l,160,"R42B\t4\tSAN14_SceneSkip.dll\t");r39::scat(l,160,what);
 r39::scat(l,160,"\ta=");r39::snum(l,160,ok_ptr(a,0x12)?at<U16>(a,0x10):0);
 r39::scat(l,160,"\tb=");r39::snum(l,160,ok_ptr(b,0x12)?at<U16>(b,0x10):0);
 r39::scat(l,160,"\tv=");r39::snum(l,160,(unsigned)v);r39::scat(l,160,"\t");r39::stamp(l,160);r39::append_line(l);
}

// B2: rcx = 0x6c6b10 인자 {+0 실행자*, +8 대상*, +0x10 성공(1)/실패(0)}. 1 = 장면 건너뜀.
extern "C" int SsEmpFilter(U8*p){
 if(!emp_on||!*emp_on||!ok_ptr(p,0x14))return 0;
 U8*actor=at<U8*>(p,0);U8*target=at<U8*>(p,8);int success=at<int>(p,0x10)!=0;
 if(!ok_ptr(actor,0x12)||!ok_ptr(target,0x12))return 0;          // 모르는 모양이면 원래대로
 static wchar_t line[0x800];
 a_reset();a_person(actor,0);a_person(target,1);
 if(!render(success?0xc3f1:0xc3f2,line,0x800)){InterlockedIncrement(&b23_text_fail);line[0]=0;}
 if(line[0])SsAddHistory(line);
 SsCollect(1,line,success);
 long n=InterlockedIncrement(&emp_skips);
 if(n<=20)state_line(success?"employ_skip_ok":"employ_skip_fail",actor,target,line[0]?1:0);
 if(n==1||n%20==0)r39::counter(4,"SAN14_SceneSkip.dll","employ_scene_skipped",(unsigned)n);
 return 1;
}
// B3: rcx = 0x6c6dd0 인자 {+0 종류, +8 실행자*, +0x10 곳, +0x18 명품*, ...}. 종류 0(탐색 명품)만. 1 = 창 건너뜀.
extern "C" int SsTreFilter(U8*p){
 if(!tre_on||!*tre_on||!ok_ptr(p,0x3c))return 0;
 if(at<int>(p,0)!=0)return 0;
 U8*actor=at<U8*>(p,8);U8*place=at<U8*>(p,0x10);U8*item=at<U8*>(p,0x18);
 if(!ok_ptr(actor,0x12)||!ok_ptr(item,0x60))return 0;
 static wchar_t line[0x800];
 // 문장: 게임 자체 이력 본문(0x51bd: 누가·어디서·무슨 명품)과 같은 인자 → 안 되면 제목(0xc39d: 누가·무슨 명품)
 bool ok=false;
 U8*force=force_of(actor);
 if(ok_ptr(place,0x10)&&force){
  a_reset();a_person(actor,0);
  U32 lord=fn<U32(*)(U8*)>(B23g,0x230400)(force);fn<void(*)(void*,U32,U32)>(B23g,0x30fc40)(msg(),lord,1);
  U32 area=fn<U32(*)(U8*)>(B23g,0x22f470)(place);fn<void(*)(void*,U32,U32)>(B23g,0x30fa60)(msg(),area,0);
  a_item(item,0);
  U8*city=fn<U8*(*)(U8*)>(B23g,0x40060)(place);
  if(ok_ptr(city,0x50))fn<void(*)(void*,U32,U32)>(B23g,0x30f950)(msg(),at<U16>(city,0x4e),0);
  ok=render(0x51bd,line,0x800);
 }
 if(!ok){a_reset();a_person(actor,0);a_item(item,0);ok=render(0xc39d,line,0x800);}
 if(!ok){InterlockedIncrement(&b23_text_fail);line[0]=0;}
 if(line[0])SsAddHistory(line);
 SsCollect(2,line,0);
 long n=InterlockedIncrement(&tre_skips);
 if(n<=20)state_line("treasure_skip",actor,nullptr,at<U16>(item,0x4a)); // v = 명품 보유자 번호(실행자와 같아야 정상)
 if(n==1||n%20==0)r39::counter(4,"SAN14_SceneSkip.dll","treasure_window_skipped",(unsigned)n);
 return 1;
}

static bool site_ok(U32 rva){return B23api->Readable(B23g+rva,6)&&!memcmp(B23g+rva,kEntry,6);}
extern "C" bool SsEmployInstall(const S14_CoreAPI*api,HookPlan&plan,volatile long*enabled){
 B23api=api;B23g=api->resolved->image;emp_on=enabled;
 if(!site_ok(0x6c6b10)){if(api->Log)api->Log("SceneSkip B2: employ talk site 0x6c6b10 differs; employ scene kept.");plan.mark(0x6c6b10,kEntry,6,"bytes differ",api->Readable(B23g+0x6c6b10,6)!=0);return false;}
 SsEmpRet=B23g+0x6c6b10+6;plan.jump(0x6c6b10,kEntry,6,(void*)&SsEmpBridge);return !plan.bad;
}
extern "C" bool SsTreasureInstall(const S14_CoreAPI*api,HookPlan&plan,volatile long*enabled){
 B23api=api;B23g=api->resolved->image;tre_on=enabled;
 if(!site_ok(0x6c6dd0)){if(api->Log)api->Log("SceneSkip B3: discovery site 0x6c6dd0 differs; treasure window kept.");plan.mark(0x6c6dd0,kEntry,6,"bytes differ",api->Readable(B23g+0x6c6dd0,6)!=0);return false;}
 SsTreRet=B23g+0x6c6dd0+6;plan.jump(0x6c6dd0,kEntry,6,(void*)&SsTreBridge);return !plan.bad;
}
// 시험용: 0 SsEmpBridge 1 SsTreBridge 2 SsEmpFilter 3 SsTreFilter 4 &emp_skips 5 &tre_skips 6 &SsEmpRet 7 &SsTreRet 8 &b23_text_fail
extern "C" __declspec(dllexport) void*SsB23Table(int i){void*t[]={(void*)&SsEmpBridge,(void*)&SsTreBridge,(void*)&SsEmpFilter,(void*)&SsTreFilter,(void*)&emp_skips,(void*)&tre_skips,(void*)&SsEmpRet,(void*)&SsTreRet,(void*)&b23_text_fail};return i>=0&&i<9?t[i]:nullptr;}
