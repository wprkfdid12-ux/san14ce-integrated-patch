// R39 1번: 새 모드 설정 창 — 게임의 '숙련 설정' 화면(CSetUpScenarioSeniorState)을 그대로 빌려 쓴다.
//  위 탭 6개 = 편의 / 밸런스 / 정보 / 그래픽 / 무장 / 사운드. 켬끔 = [켬][끔], 고르기 = 버튼 나란히, 숫자 = 슬라이더+값+[표준], 아래 설명 칸.
//  그리기는 전부 게임 것(우리 그림 없음). 정보창 '모드' 버튼(통합편의 IC)을 누르면 이 창이 뜬다.
//  게임 코드 6곳(ModCore 묶음 "SAN14CE.ModSettingsUI", 전부 아니면 전무):
//   A 0x3ec920 (9)  설정 값 읽기 — id 0x100~0x1ff 는 우리 값
//   B 0x3f2200 (9)  설정 값 쓰기 — 우리 id 는 각 모듈에 바로 반영
//   C vtable 0x141ed58 (목록 줄 바인드 0x5bba10) — 뒤에서 우리 줄 슬라이더 범위·활성 보정
//   D 0x638b00 (12) [표준]/글상자 — 우리 id 면 우리 기본값
//   E vtable 0x13b1e28 (전역 글 관리자 파생 vtable 1번 칸 0x30af60→0x308e50) — 글 id 0x7ff00000~0x7ff000ff 는 우리 설명(게임은 2,000,000 이상 id 를 안 씀 → 겹침 없음)
//   F vtable 0x141f250 (숙련 설정 Enter 0x609100) — 우리가 연 창(mode 2)이면 줄 표·탭 수·탭 이름·제목을 우리 것으로
//  IC 쪽은 코드 안 고침: '모드' 버튼 릴레이 포인터([[IC+0x1c1358]+0x66], 값 IC+0x297f0) 하나만 우리 다리로 바꿈.
//  SAN14Mods\modules\ModSettings.ini [ModSettings] UseNewWindow=0 이면 C·D·F 와 릴레이 교체를 안 해서 옛 IC 창 그대로.
#include "common.h"
#include "r39_record.h"
static const S14_CoreAPI*core;static U8*game;static U8*ic;static bool use_new=true,hooks_ok=false;
static volatile LONG relay_done,pending_open,opened_count,changed_count;
// R43 11-08(사용자: 옛 설정 창은 아예 안 뜨게, 무조건 지금 설정 창): 새 창을 못 붙이면(다른 모드가 자리를 먼저 고침) 옛 창 대신
//  '모드' 버튼에서 충돌 안내 창을 띄움(blocked). 옛 창으로 가는 길은 설정(UseNewWindow=0)으로도 안 열림.
static volatile LONG blocked;static wchar_t block_main[240],block_text[900];
static void set_blocked(const r43c::Owner*own){
 block_main[0]=block_text[0]=0;
 if(own&&own->kind==r43c::K_FOREIGN){r43c::headline(*own,block_main,240);}
 else r39::wcat(block_main,240,L"모드 설정 창을 열 수 없습니다 — 다른 모드와 충돌");
 r39::wcat(block_text,900,L"다른 모드가 게임의 설정 창 자리를 먼저 고쳐 놓아서 통합패치 모드 설정 창(1번)을 붙이지 못했습니다.\n"
  L"통합패치 기능은 설치 때 정한 값으로 그대로 돌아갑니다. 게임 안에서 켜고 끄려면 충돌한 모드를 빼 주세요.");
 if(own&&own->kind!=r43c::K_NONE){wchar_t e[400];e[0]=0;r43c::explain(*own,e,400);if(e[0]){r39::wcat(block_text,900,L"\n\n");r39::wcat(block_text,900,e);}}
 InterlockedExchange(&blocked,1);
}
static wchar_t root[1024];static size_t root_n;
static const char*MOD="SAN14_ModSettings.dll";
static wchar_t*path(wchar_t*buf,const wchar_t*rel){memcpy(buf,root,root_n*sizeof(wchar_t));buf[root_n]=0;r39::wcat(buf,1200,rel);return buf;}
static int ini_get(const wchar_t*rel,const wchar_t*sec,const wchar_t*key,int def){wchar_t p[1200];return (int)GetPrivateProfileIntW(sec,key,def,path(p,rel));}
static bool ini_put(const wchar_t*rel,const wchar_t*sec,const wchar_t*key,int v){wchar_t p[1200],num[16];int k=0;unsigned u=(unsigned)(v<0?-v:v);wchar_t t[12];int n=0;do{t[n++]=(wchar_t)(L'0'+u%10);u/=10;}while(u);if(v<0)num[k++]=L'-';while(n)num[k++]=t[--n];num[k]=0;return WritePrivateProfileStringW(sec,key,num,path(p,rel))!=0;}
static bool file_exists(const wchar_t*rel){wchar_t p[1200];return GetFileAttributesW(path(p,rel))!=INVALID_FILE_ATTRIBUTES;}
static int ms_cmp(const wchar_t*a,const wchar_t*b){while(*a&&*a==*b){a++;b++;}return (int)*a-(int)*b;}
static void ms_cpy(wchar_t*o,const wchar_t*s){while((*o++=*s++));}
static void ms_cat(wchar_t*o,const wchar_t*s){while(*o)o++;ms_cpy(o,s);}
static U8 xchg8(volatile U8*p,char v){return __atomic_exchange_n(p,(U8)v,__ATOMIC_SEQ_CST);}
static HMODULE mod(const wchar_t*n){return GetModuleHandleW(n);}
template<class F> static F exp(const wchar_t*m,const char*n){HMODULE h=mod(m);return h?(F)GetProcAddress(h,n):nullptr;}

// ---------- 줄 정의 ----------
enum Kind{K_TOGGLE,K_CHOICE,K_SLIDER};
enum Src{S_NONE,S_IC,S_IC_REWARD_ON,S_IC_REWARD_TH,S_IC_TROOPS,S_IC_BGM,S_IC_FOLLOW,S_IC_AUD,S_DOM,S_DOM_RECK,S_P27F,S_P27D,S_EXT,S_POP_BONUS,S_POP_LOSS,S_KIT,S_FOG,S_HIRED,S_COLOR,S_OFFICER,S_RANGE,S_SCENE,S_DOM_AI};
struct Row{int tab;Kind kind;Src src;int arg;const wchar_t*name;const wchar_t*opt[4];int nopt,min,max,step,def;const wchar_t*key;const wchar_t*desc;};
#define TG(tab,src,arg,name,def,key,desc) {tab,K_TOGGLE,src,arg,name,{L"켬",L"끔"},2,0,1,1,def,key,desc}
static const Row rows[]={
 // 줄 순서(번호)는 바꾸지 않는다 — 번호가 설정 id(0x100+번호)와 설명 글 id 다. 탭만 맨 앞 숫자로 정한다(2026-11-10 P204 재분류:
 //  규칙 값(무관 병력·AI 명령서·최소 인구·AI 내정 순환·저돌)은 밸런스로, 말풍선 설명·전사 알림은 정보로, 포로 초상은 무장·얼굴로).
 // 0 편의 (1 2 3 4 5 6 7 22 30)
 TG(0,S_IC,0,L"카메라 자동 이동",1,nullptr,L"[4번] 켜면 턴 진행 중 사건·재해·명승·컷신이 난 곳으로 카메라가 저절로 움직이고 확대됩니다. 끄면 이런 카메라 이동·확대를 모두 막아 화면이 제자리에 있습니다(기록 창 '위치로 가기'처럼 직접 누른 이동은 그대로)."),
 TG(0,S_IC,1,L"반복 보고 넘김",1,nullptr,L"[4번] 매 턴 반복되는 보고 창을 넘깁니다."),
 TG(0,S_IC,2,L"명승 연출 생략",1,nullptr,L"[4번] 명승 발견 연출과 암전을 함께 생략하고 보상만 받습니다."),
 TG(0,S_IC,4,L"증축 이벤트 생략",1,nullptr,L"[4번] 도시 증축 이벤트 연출을 생략합니다."),
 TG(0,S_SCENE,0,L"재해 연출 생략",1,nullptr,L"[4번] 역병·메뚜기·풍작(발생·번짐·진행 중 알림 모두)이 생겨도 알림 띠·재해 음악·창 없이 넘어가고, 재해마다 화면 밑 로그창에 한 줄 + 턴 끝 결과 창에 묶여 나옵니다. 피해·효과는 원작 그대로."),
 TG(0,S_SCENE,1,L"등용 결과 장면 생략",1,nullptr,L"[4번] 등용 결과(성공·실패) 대사 장면 없이 넘어가고, 결과는 화면 밑 로그창에 한 줄 + 턴 끝 결과 창에 묶여 나옵니다. 결과는 원작 그대로."),
 TG(0,S_SCENE,2,L"보물 발견 창 생략",1,nullptr,L"[4번] 탐색으로 명품을 찾았을 때 뜨는 창 없이 넘어가고, 누가 무엇을 찾았는지 화면 밑 로그창에 한 줄 + 턴 끝 결과 창에 묶여 나옵니다. 명품은 원작대로 손에 들어옵니다."),
 TG(0,S_SCENE,3,L"턴 끝 결과 창",1,nullptr,L"[4번] 턴 처리가 끝날 때 이번에 생략된 재해·등용 결과·보물 발견과 우리 세력 명승·증축 소식을 종류별로 묶어 창 하나로 보여 줍니다. 로그창 한 줄은 그대로 남습니다."),
 TG(0,S_IC_REWARD_ON,0,L"턴 종료 자동 포상 (자동인사 켜야 동작)",1,nullptr,L"[2번] 턴이 끝날 때 충성도가 기준보다 낮은 무장에게 자동으로 포상합니다. 자동인사에 딸린 기능이라 아래 '자동인사'가 꺼져 있으면 동작하지 않습니다."),
 {0,K_SLIDER,S_IC_REWARD_TH,0,L"자동 포상 기준 충성도",{},0,0,100,1,100,nullptr,L"[2번] 이 값보다 충성도가 낮은 무장에게 자동 포상합니다. [표준]=100. 아래 '자동인사'가 켜져 있어야 동작합니다."},
 TG(0,S_IC,12,L"턴 종료 명승 자동 투자",1,nullptr,L"[2번] 턴이 끝날 때 명승에 자동으로 투자합니다."),
 TG(0,S_IC,13,L"턴 종료 자동 논공",1,nullptr,L"[2번] 턴이 끝날 때 논공을 자동으로 합니다. 13번(논공 가챠)이 설치돼 있으면 꺼지고 잠깁니다."),
 TG(0,S_IC,10,L"자동인사 (인사 메뉴 버튼·턴 종료 처리)",1,nullptr,L"[2번] 자동인사 기능 전체를 켜고 끕니다. 켜면 인사 메뉴에 '자동인사' 버튼이 나오고, 턴이 끝날 때 할 일이 남은 무장을 자동으로 배치합니다. 끄면 자동인사 버튼이 사라지고 '턴 종료 자동 포상'도 동작하지 않습니다."),
 TG(0,S_IC_AUD,0,L"문전박대 버튼 보이기",1,nullptr,L"[3번] 외교 메뉴의 문전박대(사신 자동 거절) 버튼을 보이거나 숨깁니다. 숨기면 기능도 꺼집니다. 단계(OFF/ON/항상)는 외교 메뉴 버튼으로 바꿉니다."),
 TG(0,S_IC,14,L"여러 부대 함께 이동",1,nullptr,L"[5번] 여러 부대를 한 번에 골라 함께 이동시킵니다."),
 TG(0,S_IC_FOLLOW,0,L"아군 부대 따라가기(수행)",1,nullptr,L"[5번] 고른 부대가 다른 아군 부대를 따라다닙니다."),
 TG(0,S_IC,15,L"진행 배속 확장",1,nullptr,L"[6번] 게임 진행 배속 단계를 늘립니다."),
 TG(0,S_IC,5,L"포로 회유",1,nullptr,L"[7번] 인재 메뉴에 '포로 회유'를 추가합니다. 우리 세력이 잡고 있는 포로를 한 화면에 모아, 한 명씩 등용·처단·해방하거나 계속 억류합니다."),
 TG(0,S_IC,6,L"등용문",1,nullptr,L"[7번] 인재 메뉴에 '등용문'을 추가합니다. 등용문을 선포하면 우리 영토의 재야가 한 명씩 임관을 신청하고, 수락·거절은 직접 고릅니다. 거절한 재야는 다음 선포 때 다시 신청합니다."),
 TG(2,S_IC,7,L"인사 말풍선 설명",1,nullptr,L"[7번] 인사 화면에 무장 설명 말풍선을 띄웁니다."),
 TG(4,S_IC,9,L"포로 초상(포박 일러스트)",1,nullptr,L"[7번] 포로로 잡을 때 일러스트를 보여 줍니다."),
 {1,K_SLIDER,S_IC_TROOPS,0,L"관직 없는 무장 병력 수",{},0,1000,20000,1000,7000,nullptr,L"[7번] 관직이 없는 무장이 이끄는 병력 수입니다. [표준]=7000."},
 TG(0,S_DOM,0,L"모든 내정 자동",1,nullptr,L"[22번] 군주 세력의 내정을 모두 자동으로 처리합니다. 중점 개발은 덜 큰 항목부터 골고루 맞추되, 직접 고른 중점은 그대로 둡니다."),
 TG(1,S_DOM,1,L"AI 명령서 99장 유지",1,nullptr,L"[22번] AI 세력의 명령서를 매번 99장으로 채워 둡니다(써도 다시 99장)."),
 TG(1,S_DOM,2,L"최소 인구 1만 유지",1,nullptr,L"[22번] 도시 인구가 1만 아래로 내려가지 않게 합니다."),
 TG(1,S_DOM_AI,0,L"AI 내정 순환(군주 반영)",1,nullptr,L"[22번] AI 세력도 상업·농업·병영을 골고루 키웁니다. 군주 정치 70↑ 전 지역, 40~69 절반, 40↓ 3분의 1만. 나머지 지역은 군주가 중시하는 물자(금·군량·병사)만. 플레이어는 그대로."),
 TG(1,S_DOM_AI,1,L"AI 명승 투자 금 제한",1,nullptr,L"[22번] AI 세력은 명승에 투자한 뒤 남는 금이 비용의 2배 이상이고 5000 이상일 때만 투자합니다(가난한 도시가 명승에 금을 다 쓰지 않게). 플레이어는 그대로. 바로 적용됩니다."),
 TG(1,S_DOM_RECK,0,L"저돌 수정",1,nullptr,L"[22번] 저돌 개성 처리를 고친 판으로 씁니다."),
 TG(0,S_EXT,0,L"메뉴 빠르게",1,L"FastMenu",L"[30번] 도시·부대 메뉴를 열고 닫을 때 기다림을 줄입니다. 바로 적용됩니다."),
 TG(0,S_KIT,0,L"지도 빈칸 휠클릭 지역내정",1,L"WheelRegion",L"[30번] 지도 빈칸을 휠클릭하면 그 지역 내정 메뉴가 열립니다."),
 TG(0,S_KIT,0,L"전법서 여러 권 한 번에 습득",1,L"DragLearn",L"[30번] 전법서 여러 권·여러 무장을 한 번에 습득시킵니다."),
 TG(0,S_KIT,0,L"명품 여러 개 한 번에 수여",1,L"DragGive",L"[30번] 명품 여러 개를 한 번에 수여합니다."),
 // 1 밸런스·규칙 (13 17 18 21 23 24 25 26 28 33)
 {1,K_CHOICE,S_P27D,0,L"경험치로 능력치 성장",{L"억제",L"원본"},2,0,1,1,1,nullptr,L"[18번] 경험치로 능력치가 오르지 않게 합니다(억제). 데이터 파일을 바꾸므로 게임을 다시 켠 뒤 적용됩니다."},
 {1,K_CHOICE,S_P27D,1,L"수여로 오르는 능력치",{L"미적용",L"원본"},2,0,1,1,1,nullptr,L"[24번] 명품·관직 수여로 능력치가 오르지 않게 합니다(미적용). 게임을 다시 켠 뒤 적용됩니다."},
 TG(1,S_P27F,0,L"타 세력 도시 재야 등용",1,nullptr,L"[25번] 다른 세력 도시에 있는 재야 무장도 등용할 수 있습니다."),
 TG(1,S_P27F,1,L"행동 중 무장 관직 수여",1,nullptr,L"[25번] 행동 중인 무장에게도 관직을 줄 수 있습니다."),
 TG(2,S_P27F,2,L"전사 일러스트·메시지·기록",1,nullptr,L"[25번] 무장이 전사하면 일러스트·메시지를 보여 주고 기록에 남깁니다."),
 TG(1,S_EXT,0,L"처단 불가",1,L"NoExecute",L"[26번] 포로를 처단할 수 없게 합니다. 켜져 있으면 포로 처우 창·포로 회유 창의 처단 버튼이 회색이 되어 누를 수 없습니다."),
 TG(1,S_EXT,0,L"재야 갇힘 해소",1,L"RoninEscape",L"[26번] 재야 무장이 한 도시에 갇혀 움직이지 못하는 문제를 풉니다."),
 TG(1,S_EXT,0,L"2차 탐색 제한",1,L"AISearch",L"[26번] 같은 턴 두 번째 탐색을 제한합니다."),
 {1,K_CHOICE,S_POP_BONUS,0,L"인구 증가 보너스",{L"표준",L"+3,000",L"+5,000"},3,0,2,1,1,nullptr,L"[28번] 분기마다 도시 인구에 더해 주는 보너스입니다."},
 {1,K_CHOICE,S_POP_LOSS,0,L"전투 인구 감소",{L"표준",L"50%"},2,0,1,1,1,nullptr,L"[28번] 전투로 줄어드는 인구를 표준 또는 절반(50%)으로 합니다."},
 {1,K_SLIDER,S_RANGE,0,L"안행·누선 사거리(칸)",{},0,2,4,1,2,nullptr,L"[33번] 안행·누선 부대의 공격 사거리입니다. [표준]=2칸."},
 {1,K_SLIDER,S_RANGE,1,L"투석 사거리(칸)",{},0,1,4,1,1,nullptr,L"[33번] 투석 부대의 공격 사거리입니다. [표준]=1칸(게임 그대로). 2칸 이상이면 투석도 원거리 부대로 취급됩니다."},
 // 2 정보·UI (9 14 15 16 29 34)
 TG(2,S_IC,8,L"개성·전법 마우스 설명",1,nullptr,L"[14번] 개성·전법에 마우스를 올리면 설명을 띄웁니다."),
 TG(2,S_COLOR,0,L"항장·부상 색 표시",1,nullptr,L"[29번] 항장(공로 마이너스)은 이름·능력치를 회색으로, 부상 무장은 빨강으로 보여 줍니다."),
 TG(2,S_OFFICER,1,L"지역담당관 공석 경고",1,nullptr,L"[34번] 지역담당관이 없는 내 지역은 지도 거점 이름이 빨간 글씨로 보여 임명을 놓치지 않게 합니다."),
 // 3 그래픽·화면 (11 12 19 20 27 31)
 TG(3,S_FOG,0,L"미니맵 표시(12번 안개)",1,nullptr,L"[12번] 안개 미니맵을 보이거나 숨깁니다. 바로 적용됩니다."),
 TG(3,S_FOG,1,L"날씨(12번 안개)",1,nullptr,L"[12번] 안개 날씨 효과를 켜고 끕니다. 바로 적용됩니다."),
 TG(3,S_EXT,0,L"독립 미니맵(안개 없이)",1,L"Minimap",L"[27번] 안개 없이 미니맵만 씁니다. 여기서 켜고 끄면 게임을 다시 켠 뒤 적용됩니다(게임 중 숨기기·보이기는 미니맵 옆 켜기/끄기 단추로 바로)."),
 // 4 무장·얼굴 (10 32)
 TG(4,S_HIRED,0,L"고용무장 원본 얼굴",1,nullptr,L"[32번] 고용무장이 범용 얼굴 대신 게임 내장 오리지널 얼굴 167장을 씁니다."),
 // 5 사운드 (8)
 TG(5,S_IC_BGM,0,L"BGM 플레이어",1,nullptr,L"[8번] 추가 BGM 플레이어를 씁니다."),
};
static const unsigned NROWS=sizeof rows/sizeof rows[0];
static const int PLACEHOLDER=0xff; // 빈 탭 안내 줄
static const int MSG_BASE=0x7ff00000; // 우리 설명 글 id 시작 (다리 .S 와 같은 값)
static const wchar_t*tab_names[6]={L"편의",L"밸런스·규칙",L"정보·UI",L"그래픽·화면",L"무장·얼굴",L"사운드"};
static wchar_t lockname[64][96];
// R39b: 우리 창이 열려 있는 동안 줄마다 '사용 가능/값'을 창 열 때 한 번만 읽어 둠(휠 스크롤마다 ini·파일 확인하던 것 → 프레임 저하 원인)
extern "C"{extern U8*volatile MsOurLayout;}
static int g_av[64],g_val[64];static volatile int g_cacheOn;static U8*volatile g_ourState;
extern "C" int RowMin[64],RowMax[64];int RowMin[64],RowMax[64];   // 숫자 칸 입력창 범위(게임 표 대신)
static const wchar_t*lock_reason;

// ---------- 값 읽기/쓰기 (모듈별) ----------
static U8*domestic(){
 static int ok=-1;HMODULE m=mod(L"SAN14_DomesticCE.dll");if(!m)return nullptr;
 if(ok<0){static const U8 sha[]={0xca,0xab,0x6a,0x7c,0xaf,0xc2,0x05,0x9e,0x72,0xb8,0x9b,0x86,0x7c,0x15,0x77,0xbc,0xeb,0x2f,0xc8,0x29,0xf8,0xb9,0x4d,0xc6,0xaf,0x77,0x40,0x79,0x01,0x06,0x64,0x24} /* R43 DomesticCE(새 판 바이트 교체) */;ok=module_sha(m,sha)?1:0;}
 return ok?(U8*)m:nullptr;
}
// R42h 22번: 모든 내정(arg0)·최소 인구(arg2)는 우리 소스 모듈(DomesticPlayer)이 처리(옛 DomesticCE 가 큰 맵에서 깨져서). AI 명령서(arg1)는 아직 DomesticCE.
static bool dplayer_arg(int arg){return arg==0||arg==2;}
static bool dplayer(){return exp<int(*)(int)>(L"SAN14_DomesticPlayer.dll","SAN14_DomesticPlayerGet")!=nullptr;}
static const wchar_t*dom_keys[]={L"AllDomestic",L"AiOrders",L"PopulationFloor"};static const unsigned dom_slot[]={1,3,2};
static const wchar_t*ext_file(const wchar_t*key,wchar_t*buf){
 if(!ms_cmp(key,L"Minimap")){ms_cpy(buf,L"SAN14Mods\\Minimap\\SAN14_FogTrial.dll");return buf;}
 ms_cpy(buf,L"SAN14Mods\\Ext\\SAN14_");ms_cat(buf,key);ms_cat(buf,L".dll");return buf;
}
static bool fog12(){return mod(L"SAN14_FogEngine121.dll")!=nullptr;}
static const wchar_t*ic_ini(){return ic?(const wchar_t*)(ic+0x1c1150):L"";}
static int avail(const Row&r){
 lock_reason=nullptr;
 switch(r.src){
 case S_IC:if(!ic)return -1;if(!at<U8>(ic,0x19e690+r.arg))return -1;
  if(r.arg==0&&!at<U8>(ic,0x19ebbb))return -1;
  if(r.arg==13&&mod(L"SAN14_MeritTraits.dll")){lock_reason=L"13번 논공 가챠가 맡음";return 0;}
  return 1;
 case S_IC_REWARD_ON:case S_IC_REWARD_TH:if(!ic||!at<U8>(ic,0x19e77d))return -1;if(at<U8>(ic,0x1c07fe)){lock_reason=L"잠김";return 0;}return 1;
 case S_IC_TROOPS:return ic?1:-1;
 case S_IC_BGM:if(!ic)return -1;return at<U8>(ic,0x1c07f9)||at<U8>(ic,0x1c07f8)?1:-1;
 case S_IC_FOLLOW:return ic&&at<U8>(ic,0x19ebbf)?1:-1;
 case S_IC_AUD:return ic&&at<U8>(ic,0x19e999)?1:-1;
 case S_DOM:return (dplayer_arg(r.arg)&&dplayer())||domestic()?1:-1;
 case S_DOM_RECK:{U8*dm=domestic();return dm&&GetProcAddress((HMODULE)dm,"RecklessGetEnabled")&&GetProcAddress((HMODULE)dm,"RecklessSetEnabled")?1:-1;}
 case S_P27F:{auto g=exp<int(*)(int)>(L"SAN14_Patch27.dll","Patch27GetFeature");return g&&g(r.arg)>=0?1:-1;}
 case S_P27D:{auto g=exp<int(*)(int)>(L"SAN14_Patch27.dll","Patch27DataToggleState");if(!g)return -1;int s=g(r.arg);if(s<0)return -1;if((s&3)==2){lock_reason=L"파일 확인 필요";return 0;}return 1;}
 case S_EXT:{wchar_t f[128];if(!mod(L"SAN14_ExtBridge.dll")||!file_exists(ext_file(r.key,f)))return -1;
  if(!ms_cmp(r.key,L"Minimap")&&fog12()){lock_reason=L"12번 안개에 포함됨";return 0;}return 1;}
 case S_POP_BONUS:case S_POP_LOSS:return mod(L"SAN14_ExtBridge.dll")&&file_exists(L"SAN14Mods\\Ext\\SAN14_Population.dll")?1:-1;
 case S_KIT:{wchar_t f[96];ms_cpy(f,L"SAN14_");ms_cat(f,r.key);ms_cat(f,L".dll");return file_exists(f)?1:-1;}
 case S_FOG:return fog12()?1:-1;
 case S_HIRED:return exp<int(*)()>(L"SAN14_HiredFace.dll","SAN14_HiredFaceState")?1:-1;
 case S_COLOR:return exp<int(*)()>(L"SAN14_StatusColor.dll","SAN14_StatusColorState")?1:-1;
 case S_OFFICER:return exp<int(*)(int)>(L"SAN14_RegionOfficer.dll","SAN14_RegionOfficerGet")?1:-1;
 case S_RANGE:return exp<int(*)(int)>(L"SAN14_Range.dll","SAN14_RangeGet")?1:-1;
 case S_DOM_AI:{auto g=exp<int(*)(int)>(L"SAN14_DomesticAI.dll","SAN14_DomesticAIGet");if(!g)return -1;return r.arg&&g(5)<0?-1:1;}   // arg 1 = 명승 금 제한(which 5, 자리 못 걸었으면 -1)
 case S_SCENE:{auto h=exp<int(*)(int)>(L"SAN14_SceneSkip.dll","SAN14_SceneSkipHooked");if(!h)return -1;if(!h(r.arg)){lock_reason=L"자리 안 맞음";return 0;}return 1;}
 default:return -1;
 }
}
// 30번 키트(휠클릭·전법서·명품): 키트는 자기 ini Enabled=0 이면 시작할 때 자리를 아예 안 걸어서 게임 중에 못 켬.
//  → 키트 ini 는 늘 Enabled=1(자리를 걸어 둠), 켬/끔은 우리 SAN14Mods\modules\ModSettings.ini [Kit] 에 두고 키트의 SetEnabled 로 바로 적용.
static const wchar_t*kit_keys[3]={L"WheelRegion",L"DragLearn",L"DragGive"};
static void kit_names(const wchar_t*key,wchar_t*ini,wchar_t*dll,char*fn){ms_cpy(ini,L"SAN14_");ms_cat(ini,key);ms_cpy(dll,ini);ms_cat(ini,L".ini");ms_cat(dll,L".dll");
 int i=0;const char*p="SAN14_";for(;*p;p++)fn[i++]=*p;for(int k=0;key[k]&&i<50;k++)fn[i++]=(char)key[k];p="SetEnabled";for(;*p;p++)fn[i++]=*p;fn[i]=0;}
static int kit_get(const wchar_t*key){wchar_t ini[96],dll[96];char fn[64];kit_names(key,ini,dll,fn);
 int own=ini_get(ini,key,L"Enabled",1);return ini_get(L"SAN14Mods\\modules\\ModSettings.ini",L"Kit",key,own?1:0)!=0;}
static bool kit_apply(const wchar_t*key,int v){wchar_t ini[96],dll[96];char fn[64];kit_names(key,ini,dll,fn);auto s=exp<void(*)(int)>(dll,fn);if(s)s(v?1:0);return s!=nullptr;}
static bool kit_set(const wchar_t*key,int v){wchar_t ini[96],dll[96];char fn[64];kit_names(key,ini,dll,fn);
 bool ok=ini_put(L"SAN14Mods\\modules\\ModSettings.ini",L"Kit",key,v?1:0);
 if(v&&ini_get(ini,key,L"Enabled",1)==0)ini_put(ini,key,L"Enabled",1); // 예전에 꺼 둔 키트: 다음 시작부터 자리를 걸도록(이번 판은 다시 켠 뒤 적용)
 kit_apply(key,v);return ok;}
static void kit_migrate(){ // 예전 방식으로 키트 ini 에 Enabled=0 을 써 둔 경우 → [Kit] 끔으로 옮기고 키트 ini 는 1 로(다음 시작부터 게임 중 켜고 끄기 가능)
 for(int i=0;i<3;i++){wchar_t ini[96],dll[96];char fn[64];kit_names(kit_keys[i],ini,dll,fn);wchar_t p[1200];if(!file_exists(dll))continue;
  if(ini_get(ini,kit_keys[i],L"Enabled",1)==0&&ini_get(L"SAN14Mods\\modules\\ModSettings.ini",L"Kit",kit_keys[i],-1)<0){ini_put(L"SAN14Mods\\modules\\ModSettings.ini",L"Kit",kit_keys[i],0);ini_put(ini,kit_keys[i],L"Enabled",1);}
  (void)p;}
}
static DWORD WINAPI kit_sync(void*){ // 키트는 늦게(3초 뒤·자리 건 뒤) 준비되므로 처음 10분은 3초마다 저장된 켬/끔을 다시 알려 줌(같은 값이면 키트가 그냥 넘김)
 for(int t=0;t<200;t++){Sleep(3000);for(int i=0;i<3;i++){int v=ini_get(L"SAN14Mods\\modules\\ModSettings.ini",L"Kit",kit_keys[i],-1);if(v>=0)kit_apply(kit_keys[i],v);}}
 return 0;
}
// 논리 값: 켬끔 줄은 1=켬 0=끔, 고르기 줄은 선택 번호, 슬라이더는 숫자
static int logical_get(const Row&r){
 switch(r.src){
 case S_IC:{bool on=at<U8>(ic,0x105000+r.arg)!=0;
  // P205(11-10 확정): 통합편의는 명승 투자(줄 12)·자동 논공(줄 13)을 켬 칸 + 자동인사 정책 비트(ic+0x105010 의 2·4) 둘 다 켜져야 돌린다(IC+0x686d·0x69ef).
  if(r.arg==12)on=on&&(at<int>(ic,0x105010)&2);else if(r.arg==13)on=on&&(at<int>(ic,0x105010)&4);return on;}
 case S_IC_REWARD_ON:return at<int>(ic,0x105010)&1;
 case S_IC_REWARD_TH:return at<int>(ic,0x105014);
 case S_IC_TROOPS:return at<int>(ic,0x10661c);
 case S_IC_BGM:return at<U8>(ic,0x1c07f8)!=0;
 case S_IC_FOLLOW:return at<U8>(ic,0x10501c)!=0;
 case S_IC_AUD:{U8 s=at<U8>(ic,0x1ee3a0);if(!s)s=GetPrivateProfileIntW(L"Convenience",L"AudienceButton",1,ic_ini())?2:1;return s==2;}
 case S_DOM:if(dplayer_arg(r.arg)&&dplayer())return exp<int(*)(int)>(L"SAN14_DomesticPlayer.dll","SAN14_DomesticPlayerGet")(r.arg)!=0;{U8*dm=domestic();return dm&&at<volatile LONG>(dm,0x2d580+4*dom_slot[r.arg])!=0;}
 case S_DOM_RECK:{auto g=(int(*)())GetProcAddress((HMODULE)domestic(),"RecklessGetEnabled");return g&&g()!=0;}
 case S_P27F:return exp<int(*)(int)>(L"SAN14_Patch27.dll","Patch27GetFeature")(r.arg)==1;
 case S_P27D:return (exp<int(*)(int)>(L"SAN14_Patch27.dll","Patch27DataToggleState")(r.arg)&3)==1?0:1; // 0=적용(억제/미적용) 1=원본
 case S_EXT:{auto st=exp<int(*)(const wchar_t*)>(L"SAN14_ExtBridge.dll","SAN14_ExtState");int s=st?st(r.key):-1;if(s>=0)return s;return ini_get(L"SAN14Mods\\modules\\ExtBridge.ini",L"Ext",r.key,1)!=0;}
 case S_POP_BONUS:{int v=ini_get(L"SAN14Mods\\modules\\ExtBridge.ini",L"Population",L"Bonus",1);return v<0?0:v>2?2:v;}
 case S_POP_LOSS:return ini_get(L"SAN14Mods\\modules\\ExtBridge.ini",L"Population",L"WarLoss",1)!=0;
 case S_KIT:return kit_get(r.key);
 case S_FOG:return r.arg==0?ini_get(L"SAN14_FogMinimapUI.ini",L"Minimap",L"Hidden",0)==0:ini_get(L"SAN14_FogMinimapUI.ini",L"Weather",L"Enabled",1)!=0;
 case S_HIRED:return exp<int(*)()>(L"SAN14_HiredFace.dll","SAN14_HiredFaceState")()!=0;
 case S_COLOR:return exp<int(*)()>(L"SAN14_StatusColor.dll","SAN14_StatusColorState")()!=0;
 case S_OFFICER:return exp<int(*)(int)>(L"SAN14_RegionOfficer.dll","SAN14_RegionOfficerGet")(r.arg)!=0;
 case S_RANGE:return exp<int(*)(int)>(L"SAN14_Range.dll","SAN14_RangeGet")(r.arg);
 case S_DOM_AI:return exp<int(*)(int)>(L"SAN14_DomesticAI.dll","SAN14_DomesticAIGet")(r.arg?5:0)>0;
 case S_SCENE:return exp<int(*)(int)>(L"SAN14_SceneSkip.dll","SAN14_SceneSkipGet")(r.arg)!=0;
 default:return 0;
 }
}
static void ic_line_set(int line,int v){
 U8 nv=v?1:0;
 if(line==0){if(!at<U8>(ic,0x19ebbb))return;xchg8((volatile U8*)(ic+0x19ebba),(char)nv);}
 if(line==8){auto p=fn<volatile LONG*(*)()>(ic,0x3afc0)();if(!p){at<U8>(ic,0x19e690+8)=0;return;}InterlockedExchange(p,nv);}
 U8 old=(U8)xchg8((volatile U8*)(ic+0x105000+line),(char)nv);
 if((old!=0)!=(nv!=0)){void(*cb)(U32)=at<void(*)(U32)>(ic,0x19f9a0+line*8);if(cb)cb(nv);}
 if(ic_ini()[0]){U8*e=ic+0xe4ee0+line*32;WritePrivateProfileStringW(at<const wchar_t*>(e,8),at<const wchar_t*>(e,0x10),nv?L"1":L"0",ic_ini());}
 if(line==2&&at<U8>(ic,0x19e690+3)&&at<U8>(ic,0x105003)!=nv){ // 명승: 암전 제거도 같이(Patch27 scenic_sync 와 같음)
  at<U8>(ic,0x105003)=nv;void(*cb)(U32)=at<void(*)(U32)>(ic,0x19f9a0+3*8);if(cb)cb(nv);
  if(ic_ini()[0])WritePrivateProfileStringW(L"Convenience",L"ScenicNoFade",nv?L"1":L"0",ic_ini());
 }
}
static bool logical_set(const Row&r,int v){
 switch(r.src){
 case S_IC:ic_line_set(r.arg,v);
  if(r.arg==12||r.arg==13){int bit=r.arg==12?2:4;uint64_t pol=(U32)at<int>(ic,0x105010),th=(U32)at<int>(ic,0x105014);   // 정책 비트도 같이(통합편의 정책 저장 함수 0x65580)
   if(((pol&bit)!=0)!=(v!=0)){pol=(pol&~(uint64_t)bit)|(v?bit:0);fn<U8(*)(uint64_t)>(ic,0x65580)(pol|(th<<32));}}
  if(r.arg==0){auto ss=exp<int(*)(int,int)>(L"SAN14_SceneSkip.dll","SAN14_SceneSkipSet");if(ss)ss(4,v?0:1);}return true; // R42c: 카메라 자동 이동 끔 = 4번 SceneSkip 카메라 막기 켬
 case S_IC_REWARD_ON:case S_IC_REWARD_TH:{
  uint64_t pol=(U32)at<int>(ic,0x105010),th=(U32)at<int>(ic,0x105014);
  if(r.src==S_IC_REWARD_ON)pol=(pol&~1ull)|(v?1:0);else th=(uint64_t)(v<0?0:v>100?100:v);
  return fn<U8(*)(uint64_t)>(ic,0x65580)(pol|(th<<32))!=0;}
 case S_IC_TROOPS:{
  if(v<1000)v=1000;if(v>20000)v=20000;int old=at<int>(ic,0x10661c);if(old==v)return true;
  if(!fn<U8(*)(int)>(ic,0x654f0)(v))return false;
  if(!fn<U8(*)(int)>(ic,0x23a90)(v)){fn<U8(*)(int)>(ic,0x654f0)(old);return false;}
  InterlockedExchange((volatile LONG*)(ic+0x10661c),v);return true;}
 case S_IC_BGM:fn<void(*)(U8)>(ic,0x67f10)((U8)(v?1:0));return true;
 case S_IC_FOLLOW:{U8 old=(U8)xchg8((volatile U8*)(ic+0x10501c),(char)(v?1:0));
  if((old!=0)!=(v!=0)){void(*cb)(U32)=at<void(*)(U32)>(ic,0x1c0b98);if(cb)cb(v?1:0);}
  const wchar_t*p=(const wchar_t*)(ic+0x1c0990);if(p[0])WritePrivateProfileStringW(L"UnitFollow",L"Enabled",v?L"1":L"0",p);return true;}
 case S_IC_AUD:{
  if(ic_ini()[0]&&!WritePrivateProfileStringW(L"Convenience",L"AudienceButton",v?L"1":L"0",ic_ini()))return false;
  at<U8>(ic,0x1ee3a0)=v?2:1;if(!v&&at<int>(ic,0x19e9a8)!=0)fn<void(*)(U32)>(ic,0x67f90)(0);return true;}
 case S_DOM:{U8*dm=domestic();wchar_t p[1200];
  // R43 11-08: 모든 내정(arg0)·최소 인구(arg2)는 DomesticPlayer 만 함 → 옛 DomesticCE 쪽은 늘 0(둘이 같이 돌며 중점을 서로 바꿔 쓰던 제보 '턴마다 상·농·군').
  bool ce_v=(dplayer_arg(r.arg)&&dplayer())?false:v;
  WritePrivateProfileStringW(L"Automation",dom_keys[r.arg],ce_v?L"1":L"0",path(p,L"SAN14_DomesticCE.ini"));
  if(dm)InterlockedExchange(&at<volatile LONG>(dm,0x2d580+4*dom_slot[r.arg]),ce_v?1:0);
  if(dplayer_arg(r.arg)&&dplayer()){auto s=exp<int(*)(int,int)>(L"SAN14_DomesticPlayer.dll","SAN14_DomesticPlayerSet");if(s)return s(r.arg,v)!=0;}
  return dm!=nullptr;}
 case S_DOM_RECK:{auto s=(int(*)(int))GetProcAddress((HMODULE)domestic(),"RecklessSetEnabled");if(s)s(v?1:0);return s!=nullptr;}
 case S_P27F:return exp<int(*)(int,int)>(L"SAN14_Patch27.dll","Patch27SetFeature")(r.arg,v)!=0;
 case S_P27D:{if(logical_get(r)==v)return true;return exp<int(*)(int)>(L"SAN14_Patch27.dll","Patch27DataToggleFlip")(r.arg)!=0;}
 case S_EXT:{auto s=exp<int(*)(const wchar_t*,int)>(L"SAN14_ExtBridge.dll","SAN14_ExtSetEnabled");if(!s)return false;s(r.key,v);return true;}
 case S_POP_BONUS:case S_POP_LOSS:{auto s=exp<int(*)(int,int)>(L"SAN14_ExtBridge.dll","SAN14_ExtPopulation");if(!s)return false;
  int b=ini_get(L"SAN14Mods\\modules\\ExtBridge.ini",L"Population",L"Bonus",1),l=ini_get(L"SAN14Mods\\modules\\ExtBridge.ini",L"Population",L"WarLoss",1);
  if(r.src==S_POP_BONUS)b=v;else l=v;s(b,l);return true;}
 case S_KIT:return kit_set(r.key,v);
 case S_FOG:return r.arg==0?ini_put(L"SAN14_FogMinimapUI.ini",L"Minimap",L"Hidden",v?0:1):ini_put(L"SAN14_FogMinimapUI.ini",L"Weather",L"Enabled",v?1:0);
 case S_HIRED:exp<void(*)(int)>(L"SAN14_HiredFace.dll","SAN14_HiredFaceSetEnabled")(v?1:0);return ini_put(L"SAN14Mods\\modules\\HiredFace.ini",L"HiredFace",L"Enabled",v?1:0);
 case S_COLOR:{auto s=exp<void(*)(int)>(L"SAN14_StatusColor.dll","SAN14_StatusColorSetEnabled");if(s)s(v?1:0);return ini_put(L"SAN14Mods\\modules\\StatusColor.ini",L"StatusColor",L"Enabled",v?1:0);}
 case S_OFFICER:{auto s=exp<int(*)(int,int)>(L"SAN14_RegionOfficer.dll","SAN14_RegionOfficerSet");if(s)s(r.arg,v?1:0);return ini_put(L"SAN14Mods\\modules\\RegionOfficer.ini",L"RegionOfficer",r.arg?L"Warning":L"Face",v?1:0);}
 case S_RANGE:{auto s=exp<int(*)(int,int)>(L"SAN14_Range.dll","SAN14_RangeSet");if(!s)return false;s(r.arg,v);return true;}
 case S_DOM_AI:{auto s=exp<int(*)(int,int)>(L"SAN14_DomesticAI.dll","SAN14_DomesticAISet");if(!s)return false;return s(r.arg?5:0,v?1:0)!=0;}
 case S_SCENE:{auto s=exp<int(*)(int,int)>(L"SAN14_SceneSkip.dll","SAN14_SceneSkipSet");if(!s)return false;return s(r.arg,v?1:0)!=0;}
 default:return false;
 }
}
// 게임 값(줄 표시) <-> 논리 값
static int to_game(const Row&r,int v){return r.kind==K_TOGGLE?(v?0:1):v;}
static int from_game(const Row&r,int v){return r.kind==K_TOGGLE?(v==0?1:0):v;}

// ---------- 훅 A/B/D/E 몸통 ----------
extern "C" int ModGet(int id){
 int i=id-0x100;if(i<0||i>=(int)NROWS)return 0;const Row&r=rows[i];
 if(g_cacheOn&&i<64){if(g_av[i]<0)return to_game(r,r.def);return to_game(r,g_val[i]);}
 if(avail(r)<0)return to_game(r,r.def);
 return to_game(r,logical_get(r));
}
extern "C" int ModSet(int id,int value){
 int i=id-0x100;if(i<0||i>=(int)NROWS)return 0;const Row&r=rows[i];if((g_cacheOn&&i<64?g_av[i]:avail(r))!=1)return 0;
 int v=from_game(r,value);if(r.kind!=K_SLIDER){if(v<0)v=0;if(v>r.nopt-1&&r.kind==K_CHOICE)v=r.nopt-1;}
 else{if(v<r.min)v=r.min;if(v>r.max)v=r.max;}
 if((g_cacheOn&&i<64?g_val[i]:logical_get(r))==v)return 1;
 bool ok=logical_set(r,v);long n=InterlockedIncrement(&changed_count);
 if(ok&&i<64)g_val[i]=g_cacheOn?logical_get(r):v;
 if(n<=3||n%10==0)r39::counter(1,MOD,"changed",(unsigned)n);
 if(!ok){if(core->Log)core->Log("ModSettings R39: a setting write failed (ini write denied or module refused); value unchanged.");
  // 20차(P228): 조용히 그대로 두지 않음 — 어느 줄인지 적은 창(같은 실행에 1번). 25번 줄은 Patch27 이 파일·오류 번호까지 따로 띄움.
  char wh[64]="ModSet row 0x";r39::shex(wh,sizeof wh,(unsigned)id);char ac[48]="write refused, Windows error ";r39::snum(ac,sizeof ac,(unsigned)GetLastError());
  if(r.src!=S_P27F)r39::fail(1,MOD,"set_failed",wh,"value saved",ac,L"이 설정이 바뀌지 않음 — 설정 파일을 쓰지 못했거나 그 기능 모듈이 거절함. 게임 폴더 쓰기 권한·읽기 전용 파일을 확인하고, 그래도 안 되면 이 창의 글자를 댓글로 알려 주세요.");}
 return ok?1:0;
}
extern "C" void ModStd(U8*layout,U8*ctl){
 int id=at<int>(ctl,0x88),v=at<int>(ctl,0x80),i=id-0x100;if(i<0||i>=(int)NROWS)return;const Row&r=rows[i];
 if(r.kind!=K_SLIDER||v!=-1000||(g_cacheOn&&i<64?g_av[i]:avail(r))!=1)return; // 글상자 숫자 입력은 안 씀(슬라이더로)
 if(!layout||!core->Readable(layout+0x8c0,8))return;U8*list=at<U8*>(layout,0x8c0);if(!list||!core->Readable(list,0x180))return;
 // 0x627310 과 같은 순회(줄 정의 사본 +0x220 의 id 로 줄 찾기 → 슬라이더 값 바꿈 → 게임이 set 으로 알림)지만 전역 표로 자르지 않음
 int n=at<int>(list,0x178);
 for(int k=0;k<n&&k<128;k++){U8*row=fn<U8*(*)(U8*,int)>(game,0x86daa0)(list,k);if(!row||!core->Readable(row,0x228))continue;
  if(at<int>(row,0x220)!=id)continue;U8*sl=at<U8*>(row,0x190);if(!sl||!core->Readable(sl,0x1c0))break;
  if(fn<int(*)(U8*)>(game,0x8510c0)(sl)!=r.def)fn<void(*)(U8*,int)>(game,0x85f190)(sl,r.def);
  break;}
 ModSet(id,r.def); // 슬라이더 알림이 안 와도 값은 확실히 표준으로
}
extern "C" const wchar_t*ModMsg(int id){
 unsigned i=(unsigned)(id-MSG_BASE);if(i<NROWS)return rows[i].desc;
 if(i==PLACEHOLDER)return L"이 분류에 설치된 항목이 없습니다. 설치기에서 고를 수 있습니다.";
 return L"";
}

// ---------- R39c 훅 W: 단추 켬/끔 함수(0x14140, CCmnButton/RadioButton vfunc 0xd0) ----------
// 우리 창의 '사용 가능' 줄 단추를 게임 쪽 어떤 코드가 끄면(on=0) 막고, 누가 껐는지(호출 자리) 기록한다.
struct Watch{U8*ctl;short row;};static Watch watch[512];static volatile LONG watch_on,blocked_n;static U8*mod_lo,*mod_hi;
static unsigned wh(U8*c){return (unsigned)(((UPTR)c>>4)*2654435761u)&511;}
static void watch_clear(){memset(watch,0,sizeof watch);InterlockedExchange(&watch_on,1);}
static void watch_add(U8*c,int row){if(!c)return;unsigned h=wh(c);for(unsigned k=0;k<512;k++){Watch&w=watch[(h+k)&511];if(!w.ctl||w.ctl==c){w.ctl=c;w.row=(short)row;return;}}}
static int watch_row(U8*c){unsigned h=wh(c);for(unsigned k=0;k<16;k++){Watch&w=watch[(h+k)&511];if(!w.ctl)return -1;if(w.ctl==c)return w.row;}return -1;}
static struct{U32 rva;LONG n;}callers[8];
extern "C" int ModEnWatch(U8*ctl,U8*ret){ // 1 = 끄기 막음
 if(!watch_on||!ctl)return 0;if(ret>=mod_lo&&ret<mod_hi)return 0; // 우리 코드가 끄는 건 그대로
 int i=watch_row(ctl);if(i<0||i>=64||g_av[i]!=1)return 0;
 U32 rva=(U32)(ret-game);bool newc=false;
 for(int k=0;k<8;k++){if(callers[k].rva==rva){InterlockedIncrement(&callers[k].n);break;}if(!callers[k].rva){callers[k].rva=rva;callers[k].n=1;newc=true;break;}}
 if(newc){char nm[48]="diag_disable_by_";const char*hx="0123456789abcdef";int p=16;for(int b=24;b>=0;b-=4)nm[p++]=hx[(rva>>b)&15];nm[p]=0;r39::counter(1,MOD,nm,(unsigned)i);}
 long n=InterlockedIncrement(&blocked_n);if(n==1||n==100)r39::counter(1,MOD,"blocked_disable",(unsigned)n);
 return 1;
}
// ---------- 훅 C: 줄 바인드 뒤 보정 ----------
using Bind=UPTR(*)(U8*,UPTR,UPTR,UPTR);static Bind bind_orig,bind_native; // R40: bind_orig 는 다른 모드(AI 숙련 공방 설정)가 먼저 건 함수일 수 있음
extern "C"{extern U8*volatile MsOurLayout;}
static void vcall_text(U8*ctl,const wchar_t*s){if(ctl&&core->Readable(ctl,8)){U8*vt=at<U8*>(ctl,0);at<void(*)(U8*,const wchar_t*)>(vt,0x1d8)(ctl,s);}}
static void enable_ctl(U8*ctl,int on){if(ctl&&core->Readable(ctl,8)){U8*vt=at<U8*>(ctl,0);at<void(*)(U8*,int)>(vt,0xd0)(ctl,on);}}
extern "C" UPTR ModBind(U8*list,UPTR a,UPTR idx,UPTR d){
 // R40: 우리 모드 창(같은 숙련 설정 화면 재사용)일 때는 게임 원래 바인드만 — 다른 모드의 숙련 줄 처리가 우리 줄에 끼지 않게
 Bind f=bind_orig;U8*ours=MsOurLayout;
 if(f!=bind_native&&ours&&core->Readable(list,0x1c8)&&at<U8*>(list,0x1c0)==ours)f=bind_native;
 UPTR ret=f(list,a,idx,d);
 // R39c: 게임 바인드 = (list, edx=자료 번호, r8=칸 번호). 0x5bba10 도 자료 표는 edx, 칸(위젯)은 r8 로 찾음.
 //  예전엔 둘 다 칸 번호로 찾아서, 스크롤해 내려간 줄에 위쪽 줄(0~8번)의 규칙을 적용 → 7번(자동 논공 잠김) 칸에 온 줄이 회색, 슬라이더 숫자 칸 회색.
 U8*def=fn<U8*(*)(U8*,int)>(game,0x86d620)(list,(int)a),*row=fn<U8*(*)(U8*,int)>(game,0x86daa0)(list,(int)idx);
 if(!def||!row||!core->Readable(def,0x60)||!core->Readable(row,0x1d0))return ret;
 int id=at<int>(def,0),i=id-0x100;
 if(id==0x100+PLACEHOLDER){for(int k=0;k<4;k++)enable_ctl(at<U8*>(row,0x1a8+8*k),0);return ret;}
 if(i<0||i>=(int)NROWS)return ret;
 const Row&r=rows[i];int av=g_cacheOn&&i<64?g_av[i]:avail(r);
 {static long once;if(!InterlockedExchange(&once,1))r39::counter(1,MOD,"rows_bound",(unsigned)i);}
 if(r.kind==K_SLIDER){watch_add(at<U8*>(row,0x190),i);watch_add(at<U8*>(row,0x198),i);watch_add(at<U8*>(row,0x1a0),i);}else for(int k=0;k<4;k++)watch_add(at<U8*>(row,0x1a8+8*k),i); // 자가진단: 우리 줄이 실제로 화면에 그려짐
 if(r.kind==K_SLIDER){
  U8*sl=at<U8*>(row,0x190);
  if(sl&&core->Readable(sl,0x1c0)){
   fn<void(*)(U8*,int)>(game,0x85d4e0)(sl,r.min);fn<void(*)(U8*,int)>(game,0x85d4d0)(sl,r.max);
   fn<void(*)(U8*,int)>(game,0x85ebf0)(sl,r.step<1?1:r.step);
   int v=av>=0?(g_cacheOn&&i<64?g_val[i]:logical_get(r)):r.def;fn<void(*)(U8*,int)>(game,0x85f190)(sl,v);fn<void(*)(U8*,int)>(game,0x85eb90)(sl,r.def);
   wchar_t num[16];int k=0;unsigned u=(unsigned)(v<0?0:v);wchar_t t[12];int n=0;do{t[n++]=(wchar_t)(L'0'+u%10);u/=10;}while(u);while(n)num[k++]=t[--n];num[k]=0;
   vcall_text(at<U8*>(row,0x198),num);
  }
  int on=av==1;enable_ctl(sl,on);enable_ctl(at<U8*>(row,0x198),on);enable_ctl(at<U8*>(row,0x1a0),on);   // R39b: 숫자 칸도 켬(입력창 범위는 RowMin/RowMax)
 }else{
  int on=av==1;for(int k=0;k<4;k++)enable_ctl(at<U8*>(row,0x1a8+8*k),on);
 }
 return ret;
}

// ---------- 훅 F: Enter 뒤 우리 표 ----------
using Enter=UPTR(*)(U8*,UPTR,UPTR,UPTR);static Enter enter_orig;
static unsigned fill_count[6];extern "C"{volatile int MsCnt[6];volatile LONG MsCntHits;void MsCntBridge();void*MsCntRet;}   // P204: 목록 만들기 안에서 다시 쓰는 줄 수(브리지 C)
extern "C"{volatile int MsEntering;extern void*MsMsgOrig;}
// 우리 창을 여는 동안 원래 글 함수 결과 중 화면 제목 '숙련 모드'만 '모드'로 바꿈(다른 글은 그대로)
static volatile uint64_t g_titleUntil;static volatile LONG suk_seen,title_fixed;
static bool is_suk_mode(const wchar_t*r){
 if(!r||r[0]!=L'\uC219'||r[1]!=L'\uB828')return false;unsigned n=0;bool mo=false;while(n<12&&r[n]){if(r[n]==L'\uBAA8'&&r[n+1]==L'\uB4DC')mo=true;n++;}return n<12&&mo;}
static inline bool filter_live(){if(!MsEntering)return false;if(GetTickCount64()>g_titleUntil){MsEntering=0;return false;}return true;}
extern "C" const wchar_t*ModMsgFilter(void*self,int id){
 const wchar_t*r=fn<const wchar_t*(*)(void*,int)>((U8*)MsMsgOrig,0)(self,id);
 if(!r||!filter_live())return r;
 if(r[0]==L'\uC219'){long k=InterlockedIncrement(&suk_seen);if(k<=4)r39::counter(1,MOD,"diag_suk_msg1_id",(unsigned)id);}
 static const wchar_t s1[]=L"\uC219\uB828"; // 숙련
 if(r[0]==s1[0]&&r[1]==s1[1]){unsigned n=0;bool mo=false;while(n<12&&r[n]){if(r[n]==L'\uBAA8'&&r[n+1]==L'\uB4DC')mo=true;n++;} // 모드
  if(n<12&&mo){long k=InterlockedIncrement(&title_fixed);if(k==1)r39::counter(1,MOD,"title_fixed_v1",(unsigned)id);return L"\uBAA8\uB4DC";}}
 return r;
}
// 글 관리자 vfunc2(0x30af70): 같은 글을 [this+0x1280] 버퍼에 만들고 길이 등을 돌려줌 → 화면 제목이 이 길로 올 수 있어 버퍼를 직접 '모드'로
using Msg2=UPTR(*)(U8*,UPTR,UPTR,UPTR);static Msg2 msg2_orig;
extern "C" UPTR ModMsg2(U8*self,UPTR id,UPTR c,UPTR d){
 UPTR ret=msg2_orig(self,id,c,d);
 if(filter_live()&&self){wchar_t*b=at<wchar_t*>(self,0x1280);
  if(b&&b[0]==L'\uC219'){long k=InterlockedIncrement(&suk_seen);if(k<=4)r39::counter(1,MOD,"diag_suk_msg2_id",(unsigned)id);}
  if(b&&is_suk_mode(b)){b[0]=L'\uBAA8';b[1]=L'\uB4DC';b[2]=0;long k=InterlockedIncrement(&title_fixed);if(k==1)r39::counter(1,MOD,"title_fixed_v2",(unsigned)id);}}
 return ret;
}
// 줄 표(61칸 × 0x60)와 탭별 줄 수(layout+0x17c, 6칸)를 우리 것으로 채움.
//  게임 목록 만들기(0x658df0): 탭 t 의 줄 = 표[앞 탭들 줄 수 합 … +줄 수[t]). 줄 수가 게임 기본값(9,8,7,8,10,19 — 생성자 0x59e150)으로
//  돌아가 있으면 우리 표가 그 숫자로 잘려, 앞 탭 끝줄이 뒤 탭에 나오고 사운드 탭에 빈 슬라이더 줄(100/표준)이 생긴다(P204, 2026-11-09 사용자 화면).
static void fill_table(U8*layout){
 U8*table=layout+0x8d0;memset(table,0,61*0x60);unsigned n=0;
 for(int tab=0;tab<6;tab++){
  unsigned cnt=0;
  for(unsigned i=0;i<NROWS&&n<61;i++){
   const Row&r=rows[i];if(r.tab!=tab)continue;int av=avail(r);if(av<0)continue;
   U8*e=table+n*0x60;at<int>(e,0)=0x100+(int)i;at<int>(e,4)=ModGet(0x100+(int)i);
   at<uint64_t>(e,8)=r.kind==K_SLIDER?0:(uint64_t)r.nopt;at<int>(e,0xc)=0;
   const wchar_t*name=r.name;
   if(av==0&&lock_reason&&i<64){lockname[i][0]=0;r39::wcat(lockname[i],96,r.name);r39::wcat(lockname[i],96,L" (");r39::wcat(lockname[i],96,lock_reason);r39::wcat(lockname[i],96,L")");name=lockname[i];}
   at<const wchar_t*>(e,0x18)=name;
   for(int k=0;k<4&&k<r.nopt;k++)at<const wchar_t*>(e,0x20+8*k)=r.opt[k];
   for(int k=0;k<5;k++)at<int>(e,0x48+4*k)=MSG_BASE+(int)i;
   n++;cnt++;
  }
  if(!cnt&&n<61){U8*e=table+n*0x60;at<int>(e,0)=0x100+PLACEHOLDER;at<uint64_t>(e,8)=2;at<const wchar_t*>(e,0x18)=L"설치된 항목 없음";
   at<const wchar_t*>(e,0x20)=L"-";at<const wchar_t*>(e,0x28)=L"-";for(int k=0;k<5;k++)at<int>(e,0x48+4*k)=MSG_BASE+PLACEHOLDER;n++;cnt=1;}
  at<int>(layout,0x17c+4*tab)=(int)cnt;fill_count[tab]=cnt;MsCnt[tab]=(int)cnt;
 }
}
// P204: 우리 창이 떠 있는 동안 줄 수·표가 우리 것 그대로인지 확인. 다르면 다시 채우고 목록을 다시 만든다(원인 기록 포함).
//  한 번 열 때 30번 넘게 되돌려지면 그만둠(게임이 매번 되돌리는 경우 깜빡임 방지) — 그때는 실패 기록.
static volatile LONG layout_fixed;static int fix_this_open;
static void keep_layout(U8*state){
 U8*layout=at<U8*>(state,0x470);if(!layout||!core->Readable(layout,0x1fb0))return;
 bool moved=layout!=MsOurLayout,cnt_bad=false;int first=at<int>(layout,0x8d0);
 for(int t=0;t<6;t++)if(at<int>(layout,0x17c+4*t)!=(int)fill_count[t])cnt_bad=true;
 bool tab_bad=first<0x100||first>0x1ff;
 if(!moved&&!cnt_bad&&!tab_bad)return;
 if(fix_this_open>=30){if(fix_this_open==30){fix_this_open++;r39::counter(1,MOD,"layout_fix_gave_up",30);}return;}
 fix_this_open++;
 long k=InterlockedIncrement(&layout_fixed);
 if(k<=5&&core->Log){char m[200];m[0]=0;r39::scat(m,sizeof m,"ModSettings R43: tab layout restored (moved=");r39::snum(m,sizeof m,moved?1u:0u);
  r39::scat(m,sizeof m," table=");r39::snum(m,sizeof m,tab_bad?1u:0u);r39::scat(m,sizeof m," counts=");
  for(int t=0;t<6;t++){if(t)r39::scat(m,sizeof m,",");r39::snum(m,sizeof m,(unsigned)at<int>(layout,0x17c+4*t));}
  r39::scat(m,sizeof m," ours=");for(int t=0;t<6;t++){if(t)r39::scat(m,sizeof m,",");r39::snum(m,sizeof m,fill_count[t]);}r39::scat(m,sizeof m,").");core->Log(m);}
 if(k<=3||k%50==0)r39::counter(1,MOD,"layout_fixed",(unsigned)((moved?1:0)|(cnt_bad?2:0)|(tab_bad?4:0)));
 MsOurLayout=layout;fill_table(layout);
 fn<void(*)(U8*)>(game,0x658df0)(layout);fn<void(*)(U8*)>(game,0x6594a0)(layout);
}
extern "C" UPTR ModEnter(U8*state,UPTR a,UPTR b,UPTR c){
 bool ours=pending_open&&state&&core->Readable(state,0x4a8)&&at<int>(state,0x490)==2;
 if(ours){for(unsigned i=0;i<NROWS&&i<64;i++){const Row&r=rows[i];g_av[i]=avail(r);g_val[i]=g_av[i]>=0?logical_get(r):r.def;}g_ourState=state;g_cacheOn=1;g_titleUntil=GetTickCount64()+5000;MsEntering=1;watch_clear();}
 UPTR ret=enter_orig(state,a,b,c);
 if(!state||!core->Readable(state,0x4a8)||at<int>(state,0x490)!=2||!InterlockedExchange(&pending_open,0))return ret;
 U8*layout=at<U8*>(state,0x470);if(!layout||!core->Readable(layout,0x1fb0))return ret;
 MsOurLayout=layout; // 훅 S: 우리 창 탭은 줄 수가 칸 수 이상이면 스크롤바 보임
 fix_this_open=0;fill_table(layout);
 U8*tp=at<U8*>(layout,0x8b8);
 if(tp&&core->Readable(tp,0x148)&&at<int>(tp,0x140)==6){U8**btn=at<U8**>(tp,0x130);
  if(btn&&core->Readable(btn,48))for(int t=0;t<6;t++)vcall_text(btn[t],tab_names[t]);}
 U8*desc=at<U8*>(layout,0x8c8);if(desc&&core->Readable(desc,0x98))fn<void(*)(U8*,const wchar_t*)>(game,0x8a35c0)(desc,L"항목에 마우스를 올리면 설명이 여기에 나옵니다. (통합패치 " S14_PATCH_VERW L" 모드 설정)");
 // 화면 제목: Enter(0x609100) 가 UI 글 0xbc('숙련 모드', 0x30cf80) 를 머리띠 객체 0x2b19d0() 에 0x856660(머리띠, 제목, 빈 글, 0) 로 넣음 → 같은 호출로 '모드'
 {U8*hdr=fn<U8*(*)()>(game,0x2b19d0)();if(hdr){fn<void(*)(U8*,const wchar_t*,const wchar_t*,int)>(game,0x856660)(hdr,L"\uBAA8\uB4DC",at<const wchar_t*>(game,0x1a4ab88),0);r39::counter(1,MOD,"title_set",1);}}
 fn<void(*)(U8*)>(game,0x658df0)(layout);fn<void(*)(U8*)>(game,0x6594a0)(layout);
 long k=InterlockedIncrement(&opened_count);if(k<=3||k%20==0)r39::counter(1,MOD,"opened",(unsigned)k);
 if(core->Log)core->Log("ModSettings R39: mod settings window opened (native senior-settings screen, 6 tabs).");
 return ret;
}

// ---------- R39b 훅 G·H: 창 Update 마다 우리 줄 활성 다시 확인 / 창 Exit 때 캐시 끝 ----------
// 컨트롤 활성 = [ctl+0x40] 비트 2 (CCmnButton/CCmnRadioButton vfunc 0xd0 = 0x14140 이 이 비트만 바꿈).
// 게임 쪽 어떤 갱신이 우리 줄 일부(켬 단추·[표준]·숫자 칸)를 꺼 버리는 일이 있어(원인: 게임 표를 우리 id 로 읽음), 매 프레임 비트만 보고 꺼졌으면 다시 켬.
using Upd=UPTR(*)(U8*,UPTR,UPTR,UPTR);static Upd upd_orig,exit_orig;static volatile LONG reenabled;
static inline void en_fast(U8*ctl){if(ctl&&!(at<U32>(ctl,0x40)&4)){U8*vt=at<U8*>(ctl,0);at<void(*)(U8*,int)>(vt,0xd0)(ctl,1);long n=InterlockedIncrement(&reenabled);if(n==1||n==100)r39::counter(1,MOD,"reenabled",(unsigned)n);}}
static void reassert(U8*state){
 U8*layout=at<U8*>(state,0x470);if(!layout)return;U8*list=at<U8*>(layout,0x8c0);if(!list)return;
 int n=at<int>(list,0x178);
 for(int k=0;k<n&&k<128;k++){
  U8*row=fn<U8*(*)(U8*,int)>(game,0x86daa0)(list,k);if(!row||!at<int>(row,0x84))continue;U8*def=fn<U8*(*)(U8*,int)>(game,0x86d620)(list,at<int>(row,0x80));if(!def)continue; // 칸의 자료 번호 = [칸+0x80]
  int i=at<int>(def,0)-0x100;if(i<0||i>=(int)NROWS||i>=64||g_av[i]!=1)continue;const Row&r=rows[i];
  if(r.kind==K_SLIDER){en_fast(at<U8*>(row,0x190));en_fast(at<U8*>(row,0x198));en_fast(at<U8*>(row,0x1a0));
   U8*sl=at<U8*>(row,0x190); // R39c: 슬라이더 범위가 게임 표(우리 id 로 읽은 쓰레기)로 바뀌어 있으면 다시 우리 범위·값·숫자로
   if(sl&&(at<int>(sl,0x17c)!=r.max||at<int>(sl,0x180)!=r.min)){
    fn<void(*)(U8*,int)>(game,0x85d4e0)(sl,r.min);fn<void(*)(U8*,int)>(game,0x85d4d0)(sl,r.max);fn<void(*)(U8*,int)>(game,0x85ebf0)(sl,r.step<1?1:r.step);
    int v=g_val[i];if(v<r.min)v=r.min;if(v>r.max)v=r.max;fn<void(*)(U8*,int)>(game,0x85f190)(sl,v);
    wchar_t num[16];int k2=0;unsigned u=(unsigned)v;wchar_t t[12];int nn=0;do{t[nn++]=(wchar_t)(L'0'+u%10);u/=10;}while(u);while(nn)num[k2++]=t[--nn];num[k2]=0;vcall_text(at<U8*>(row,0x198),num);
    static long sf;long c=InterlockedIncrement(&sf);if(c<=3)r39::counter(1,MOD,"slider_range_fixed",(unsigned)(0x100+i));}}
  else for(int j=0;j<r.nopt&&j<4;j++)en_fast(at<U8*>(row,0x1a8+8*j));
 }
}
// R39c 진단: 창 연 뒤 60프레임째에 보이는 줄의 단추 상태를 한 번 기록(회색 단추와 정상 단추 차이 찾기)
static void hx(char*o,int&p,U32 v,int nd){const char*h="0123456789abcdef";for(int b=(nd-1)*4;b>=0;b-=4)o[p++]=h[(v>>b)&15];}
static int seen_fr[64];static U8 dumped[64];
static void dump_slot(U8*list,int k){
 U8*def=fn<U8*(*)(U8*,int)>(game,0x86d620)(list,k),*row=fn<U8*(*)(U8*,int)>(game,0x86daa0)(list,k);if(!def||!row||!core->Readable(row,0x1d0)||!core->Readable(def,0x10))return;
 int id=at<int>(def,0);
 {char nm[48]="dump_row_k";int p=10;hx(nm,p,(U32)k,2);nm[p++]='_';hx(nm,p,(U32)(id&0xfff),3);nm[p++]=0;r39::counter(1,MOD,nm,(at<U32>(row,0x40)&0xffff)|((U32)(at<U8*>(row,0)-game)&0xffff)<<16);}
 for(int j=0;j<7;j++){U8*c=at<U8*>(row,j<3?0x190+8*j:0x1a8+8*(j-3));if(!c||!core->Readable(c,0x2a0))continue;
  char nm[64]="dump_k";int p=6;hx(nm,p,(U32)k,2);nm[p++]='_';nm[p++]='i';hx(nm,p,(U32)(id&0xfff),3);nm[p++]='_';nm[p++]='c';nm[p++]=(char)('0'+j);nm[p++]='_';
  hx(nm,p,(U32)(at<U8*>(c,0)-game),7);nm[p++]='_';hx(nm,p,at<U32>(c,8),8);nm[p++]=0;
  U32 v=(at<U32>(c,0x40)&0xffff)|((U32)at<U8>(c,0x44)<<16);U8*ch=at<U8*>(c,0x298);if(ch&&core->Readable(ch,0x48))v|=(U32)(at<U8>(ch,0x40)&0xff)<<24;
  r39::counter(1,MOD,nm,v);}
}
static void dump_new(U8*state,int fr){ // 줄 칸(slot)이 처음 보이고 30프레임 뒤 한 번씩 기록
 U8*layout=at<U8*>(state,0x470);if(!layout||!core->Readable(layout,0x8d0))return;U8*list=at<U8*>(layout,0x8c0);if(!list||!core->Readable(list,0x180))return;
 int n=at<int>(list,0x178);if(n>64)n=64;
 static int lastid[64];if(fr==0)memset(lastid,0,sizeof lastid);
 for(int k=0;k<n;k++){U8*df=fn<U8*(*)(U8*,int)>(game,0x86d620)(list,k);int id=df?at<int>(df,0):-1;if(id!=lastid[k]){lastid[k]=id;seen_fr[k]=0;dumped[k]=0;}
  if(dumped[k])continue;if(!seen_fr[k]){seen_fr[k]=fr+1;continue;}if(fr+1-seen_fr[k]>=30){dumped[k]=1;dump_slot(list,k);}}
 {static int lastn;if(n!=lastn){lastn=n;r39::counter(1,MOD,"diag_slots",(unsigned)n);}}
}
extern "C" UPTR ModUpdate(U8*state,UPTR a,UPTR b,UPTR c){
 UPTR ret=upd_orig(state,a,b,c);
 {static long n;if(InterlockedIncrement(&n)==1)r39::counter(1,MOD,"diag_update_called",1);}
 if(g_cacheOn&&state&&state==g_ourState){keep_layout(state);reassert(state);}
 return ret;
}
extern "C" UPTR ModExit(U8*state,UPTR a,UPTR b,UPTR c){
 {static long n;long k=InterlockedIncrement(&n);if(k<=2)r39::counter(1,MOD,"diag_exit_called",(unsigned)k);}
 if(state&&state==g_ourState){r39::counter(1,MOD,"counts_rewritten",(unsigned)InterlockedExchange(&MsCntHits,0));g_cacheOn=0;g_ourState=nullptr;MsEntering=0;InterlockedExchange(&watch_on,0);MsOurLayout=nullptr;}
 return exit_orig(state,a,b,c);
}

// ---------- IC '모드' 버튼 ----------
extern "C"{void*MsClickOrig;void MsClickBridge();}
static uint64_t last_click;
extern "C" int ModClick(U8*owner,U8*widget){
 if(!ic||!widget||!owner)return 0;
 if(!blocked&&(!hooks_ok||!use_new))return 0;
 if(!core->Readable(widget,0x90)||at<int>(widget,0x80)!=0x40)return 0;
 if(at<U8>(ic,0x1c1100))return 0; // 옛 창이 이미 열려 있으면 IC 가 닫게 둠
 if(!core->Readable(owner,0x148)||at<int>(owner,0x140)!=0x52||(at<U8>(widget,0x40)&7)!=7)return 0;
 uint64_t now=GetTickCount64();if(last_click&&now-last_click<350)return 1;last_click=now;
 if(blocked){r43c::show(L"삼국지14 통합패치 — 모드 설정 창",block_main,block_text,0x30);return 1;}   // 옛 창 대신 안내(막지 않음)
 static long long mode;mode=2;static U8 fnbuf[0x48];memset(fnbuf,0,sizeof fnbuf);
 InterlockedExchange(&pending_open,1);
 U8*mgr=fn<U8*(*)()>(game,0x11040)();if(!mgr){InterlockedExchange(&pending_open,0);return 0;}
 fn<void(*)(U8*,const char*,long long*,U8*)>(game,0x467c70)(mgr,(const char*)(game+0x13eabb8),&mode,fnbuf);
 return 1;
}
extern "C" __declspec(dllexport) int SAN14_ModSettingsOpen(){ // 시험용: UI 스레드에서만 부를 것
 if(!hooks_ok)return 0;static long long mode;mode=2;static U8 fnbuf[0x48];memset(fnbuf,0,sizeof fnbuf);InterlockedExchange(&pending_open,1);
 U8*mgr=fn<U8*(*)()>(game,0x11040)();if(!mgr)return 0;fn<void(*)(U8*,const char*,long long*,U8*)>(game,0x467c70)(mgr,(const char*)(game+0x13eabb8),&mode,fnbuf);return 1;
}
// R43: 공통 실패 보고(단계·자리·기대/실제). R43 11-08 부터 옛 창은 안 열림
// 26차 뒤(P239, 디시 11-11 "행동교정 모드만 깔았는데"): vtable 칸 충돌 창에 그 칸을 가진 DLL 이름이 안 나왔음(주인 판정을 해 놓고 창에 안 넘김) → own 을 창까지
static void fail_box(const char*stage,const char*where,const char*expected,const char*actual,const r43c::Owner*own=nullptr){if(blocked)return;/* 이미 충돌 창을 띄움 */r39::fail_ex(1,MOD,stage,where,expected,actual,L"모드 설정 창(1번)이 안 열림 → 게임 안에서 켜고 끄기 불가(기능은 설치 때 값으로 그대로)",own);}
static void hex8(char*o,size_t cap,const void*p){r39::sbytes(o,cap,(const U8*)p,8);}
static DWORD WINAPI relay_worker(void*){
 // IC 가 자기 릴레이를 만들 때까지 0.2초마다 기다림: 기본 120초. R43 11-08: IC 가 시작 확인(게임 코드 2,346곳)을 아직 하는 중이면
 //  그게 끝날 때까지 최대 10분 더 기다림(통합편의가 늦게 켜진 PC 에서 새 창 대신 옛 창이 뜨던 길 막음).
 unsigned long long t0=GetTickCount64(),deadline=t0+120000;bool ic_done=false;
 for(;GetTickCount64()<deadline;){
  HMODULE m=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
  if(m&&!ic_done){auto w=reinterpret_cast<U32(*)()>(GetProcAddress(m,"SAN14_WaitConvenienceV1"));
   if(!w)ic_done=true;else if(w())ic_done=true;else if(deadline<t0+600000)deadline=t0+600000;}   // w() 는 최대 15초 기다리고 돌아옴(이 뒤 작업 스레드라 괜찮음)
  if(m){
   if(!ic){ic=known_common();if(!ic){r39::start(1,MOD,"PART",6,7,"UseNewWindow=1","IC hash unknown; relay not swapped");fail_box("ic_build","relay_worker -> known_common() SHA-256 of SAN14_IntegratedConvenience.dll","known IC build (R40 ae365eab… / R39 / ce101 / R11 / P1)","IC loaded but hash unknown");return 0;}}
   U8*page=at<U8*>(ic,0x1c1358);
   if(page&&core->Readable(page+0x60,0x10)){
    U8**slot=(U8**)(page+0x66);U8*cur=*slot;
    if(cur==(U8*)&MsClickBridge){InterlockedExchange(&relay_done,1);return 0;}
    if(cur!=ic+0x297f0){r39::start(1,MOD,"PART",6,7,"UseNewWindow=1","IC relay points elsewhere (another mod?)");char ex[40],ac[40];ex[0]=ac[0]=0;r39::scat(ex,sizeof ex,"IC+0x297f0");r39::scat(ac,sizeof ac,cur>=ic&&cur<ic+0x200000?"IC+0x":"0x");r39::shex(ac,sizeof ac,cur>=ic&&cur<ic+0x200000?(unsigned long long)(cur-ic):(unsigned long long)(UPTR)cur);fail_box("ic_relay","relay_worker: IC mod-button relay slot [IC+0x1c1358]->page+0x66",ex,ac);return 0;}
    MsClickOrig=cur;DWORD old;
    if(!VirtualProtect(slot,8,PAGE_EXECUTE_READWRITE,&old)){r39::start(1,MOD,"PART",6,7,"UseNewWindow=1","VirtualProtect failed on IC relay");char ac[40];ac[0]=0;r39::scat(ac,sizeof ac,"VirtualProtect=0 GetLastError=");r39::snum(ac,sizeof ac,(unsigned)GetLastError());fail_box("ic_relay_protect","relay_worker: VirtualProtect(IC relay slot,8,RWX)","VirtualProtect!=0",ac);return 0;}
    InterlockedExchange64((volatile LONG64*)slot,(LONG64)(UPTR)&MsClickBridge);VirtualProtect(slot,8,old,&old);FlushInstructionCache(GetCurrentProcess(),slot,8);
    InterlockedExchange(&relay_done,1);r39::counter(1,MOD,"relay_swapped",1);
    if(core->Log)core->Log("ModSettings R39: info-window 'Mod' button now opens the new settings window.");
    return 0;
   }
  }
  Sleep(200);
 }
 // 기다림 끝(120초, IC 시작 확인 중이면 최대 10분): IC 가 없거나 IC 1번(모드 버튼)이 꺼진 구성이면 정상 → 오류창 없이 기록만
 HMODULE m=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
 if(!m){r39::start(1,MOD,"OFF",6,6,"UseNewWindow=1","IC not loaded (no info-window Mod button); nothing to swap");return 0;}
 if(ic&&core->Readable(ic+0x1c07ff,1)&&!at<U8>(ic,0x1c07ff)){r39::start(1,MOD,"OFF",6,6,"UseNewWindow=1","IC No.1 Mod button off; nothing to swap");return 0;}
 r39::start(1,MOD,"PART",6,7,"UseNewWindow=1","IC relay not found within 120s");
 {char ac[80];ac[0]=0;r39::scat(ac,sizeof ac,"page=");U8*pg=ic?at<U8*>(ic,0x1c1358):nullptr;r39::shex(ac,sizeof ac,(unsigned long long)(UPTR)pg);r39::scat(ac,sizeof ac,ic_done?" after wait (IC ready)":" after 10 min (IC startup check not finished)");
  fail_box("ic_relay_wait","relay_worker: IC settings page [IC+0x1c1358] readable within 120s (10 min while IC startup check runs)","page!=0 readable",ac);}
 return 0;
}

// ---------- 시작 ----------
extern "C"{void MsEnBridge();void MsSbBridge();void*MsSbRet;void MsClampBridge();void*MsClampRet;U8*volatile MsOurLayout;extern U8 __ImageBase;}
extern "C"{void*MsGetJa,*MsGetRet,*MsSetJa,*MsSetRet,*MsStdRet,*MsMsgOrig,*MsRngRet;void MsGetBridge();void MsSetBridge();void MsStdBridge();void MsMsgBridge();void MsRngBridge();}
extern "C" __declspec(dllexport) void*SAN14_ModSettingsTestTable(int i){
 void*t[]={(void*)&MsGetBridge,(void*)&MsSetBridge,(void*)&MsStdBridge,(void*)&MsMsgBridge,(void*)&MsClickBridge,(void*)&ModGet,(void*)&ModSet,&MsGetJa,&MsGetRet,&MsSetJa,&MsSetRet,&MsStdRet,&MsMsgOrig,(void*)&relay_done,(void*)&ModEnter,(void*)&ModBind,(void*)&enter_orig,(void*)&bind_orig,(void*)&pending_open,(void*)&MsClickOrig,(void*)&MsEntering,(void*)&MsRngBridge,(void*)&ModUpdate,(void*)&ModExit,&MsRngRet,(void*)&upd_orig,(void*)&exit_orig,(void*)RowMin,(void*)&g_cacheOn,(void*)&reenabled,(void*)&g_titleUntil,(void*)&blocked_n,(void*)&MsEnBridge,(void*)&MsSbBridge,(void*)&MsOurLayout,&MsSbRet,(void*)&MsClampBridge,&MsClampRet,(void*)&MsCntBridge,&MsCntRet,(void*)&MsCnt,(void*)&MsCntHits};
 return i>=0&&i<(int)(sizeof t/sizeof t[0])?t[i]:nullptr;
}
extern "C" __declspec(dllexport) int SAN14_ModSettingsState(){return (hooks_ok?1:0)|(use_new?2:0)|(relay_done?4:0);}
static bool chained_bind; // R40
static bool vt_hook(S14_Hook&h,U32 slot,U32 orig,void*dest){
 // R40: 줄 바인드 칸(0x141ed58)을 다른 모드가 먼저 바꿨으면(게임 밖 주소) 그 함수 위에 이어 붙임
 if(slot==0x141ed58&&core->Readable(game+slot,8)){U8*cur=*(U8**)(game+slot);
  if(cur!=game+orig&&(cur<game||cur>=game+0x2a00000)&&cur!=(U8*)dest&&core->Readable(cur,16)){
   bind_orig=(Bind)cur;chained_bind=true;h.target=game+slot;h.length=8;UPTR o=(UPTR)cur,dd=(UPTR)dest;memcpy(h.expected,&o,8);memcpy(h.replacement,&dd,8);return true;}}
 h.target=game+slot;h.length=8;UPTR o=(UPTR)(game+orig),d=(UPTR)dest;memcpy(h.expected,&o,8);memcpy(h.replacement,&d,8);
 return core->Readable(h.target,8)&&!memcmp(h.target,&o,8);
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,1,MOD))return 0;core=api;game=api->resolved->image;
 {U8*b=(U8*)&__ImageBase;mod_lo=b;mod_hi=b+at<U32>(b+at<U32>(b,0x3c),0x50);}
 if(r39::game_root(root,1024))root_n=r39::wlen(root);
 // R43 11-08: 옛 창(UseNewWindow=0) 선택 없앰 — 늘 새 창. (옛 설정 값은 읽지 않음)
 use_new=true;
 // 설치기에서 1번(모드 설정 창)을 끄면 IC 가 모드 버튼 자체를 안 만듦 → 새 창도 필요 없음(오류 아님)
 bool ui_on=ini_get(L"San14ModSettings.ini",L"Modules",L"ModSettingsUI",1)!=0;
 if(!ui_on)use_new=false;
 kit_migrate();{HANDLE kt=CreateThread(nullptr,0,kit_sync,nullptr,0,nullptr);if(kt)CloseHandle(kt);}
 HookPlan plan(api);
 static const U8 ea[]={0x83,0xfa,0x3c,0x0f,0x87,0xb6,0x02,0x00,0x00};MsGetJa=game+0x3ecbdf;MsGetRet=game+0x3ec920+9;plan.jump(0x3ec920,ea,9,(void*)&MsGetBridge);
 static const U8 eb[]={0x83,0xfa,0x3c,0x0f,0x87,0xf2,0x02,0x00,0x00};MsSetJa=game+0x3f24fb;MsSetRet=game+0x3f2200+9;plan.jump(0x3f2200,eb,9,(void*)&MsSetBridge);
 if(use_new){static const U8 ed[]={0x40,0x56,0x57,0x41,0x56,0x48,0x81,0xec,0x90,0x00,0x00,0x00};MsStdRet=game+0x638b00+12;plan.jump(0x638b00,ed,12,(void*)&MsStdBridge);}
 // vtable 칸(8바이트 포인터) 은 HookPlan 뒤에 이어 붙임 — 같은 묶음, 전부 아니면 전무
 bool vt_ok=true;U32 base=plan.count;
 // 글 관리자 전역 객체(0x1a26440)는 파생 클래스 vtable 0x13aedb0 을 씀(시작 코드 0x1460). 그 1번 칸 0x13b1e28 = 0x30af60(→0x308e50 점프).
 //  (기반 vtable 0x13aed78 칸은 실제로 안 불림 — R39 첫 판에서 설명 대신 게임 대사가 뜨던 원인)
 MsMsgOrig=game+0x30af60;vt_ok&=vt_hook(plan.hooks[plan.count++],0x13b1e28,0x30af60,(void*)&MsMsgBridge);
 if(use_new){bind_orig=bind_native=(Bind)(game+0x5bba10);vt_ok&=vt_hook(plan.hooks[plan.count++],0x141ed58,0x5bba10,(void*)&ModBind);
  enter_orig=(Enter)(game+0x609100);vt_ok&=vt_hook(plan.hooks[plan.count++],0x141f250,0x609100,(void*)&ModEnter);
  exit_orig=(Upd)(game+0x600ee0);vt_ok&=vt_hook(plan.hooks[plan.count++],0x141f258,0x600ee0,(void*)&ModExit);
  upd_orig=(Upd)(game+0x60ef50);vt_ok&=vt_hook(plan.hooks[plan.count++],0x141f270,0x60ef50,(void*)&ModUpdate);
  msg2_orig=(Msg2)(game+0x30af70);vt_ok&=vt_hook(plan.hooks[plan.count++],0x13b1e30,0x30af70,(void*)&ModMsg2);
  static const U8 es[]={0x83,0xfa,0x04,0x7d,0x04,0x33,0xd2,0xeb,0x05,0xba,0x01,0x00,0x00,0x00};MsSbRet=game+0x658e7b;plan.jump(0x658e6d,es,14,(void*)&MsSbBridge);
  // C(P204): 목록 만들기 둘째 명령에서 우리 창의 줄 수를 다시 씀 — 기색 모드가 들머리에서 게임 기본값으로 되돌려도 우리 창은 우리 줄 수로
  static const U8 ec[]={0x48,0xc7,0x44,0x24,0x20,0xfe,0xff,0xff,0xff};MsCntRet=game+0x658e03;plan.jump(0x658dfa,ec,9,(void*)&MsCntBridge);
  // K: 계산기(숫자 칸) 확인 콜백 0x65e8c0 → 0x627310(list,id,값) 이 전역 표로 자름 → 우리 id 는 표가 쓰레기(최소 26343292 등) → 그 값이 슬라이더·숫자 칸에. 범위를 우리 것으로
  static const U8 ek[]={0x42,0x8b,0x1c,0x21,0x42,0x8b,0x44,0x21,0x04};MsClampRet=game+0x629c49;plan.jump(0x629c40,ek,9,(void*)&MsClampBridge);
  // H: [표준]/숫자 칸 처리 0x638b00 안 입력창 범위 읽기(게임 표 0x1414005c0 + id*0x1c) — 우리 id 면 RowMin/RowMax
  static const U8 eh[]={0x42,0x8b,0x04,0x01,0x89,0x44,0x24,0x38,0x42,0x8b,0x44,0x01,0x04};MsRngRet=game+0x638c48+13;plan.jump(0x638c48,eh,13,(void*)&MsRngBridge);   // 뒤 mov [rsp+0x3c],eax 는 원래대로(eax=최대)
  for(unsigned i=0;i<NROWS&&i<64;i++){RowMin[i]=rows[i].kind==K_SLIDER?rows[i].min:0;RowMax[i]=rows[i].kind==K_SLIDER?rows[i].max:rows[i].nopt-1;}}
 (void)base;U32 total=plan.count;
 bool ok=vt_ok&&plan.install("SAN14CE.ModSettingsUI");hooks_ok=ok;
 if(ok&&chained_bind&&core->Log)core->Log("ModSettings R40: list-bind slot was already hooked by another mod (AI mod battle rules); chained, our window bypasses it.");
 const char*setting=use_new?"UseNewWindow=1":"UseNewWindow=0";
 if(core->Log)core->Log(ok?(use_new?"ModSettings R40: hook group installed (13 sites: get, set, std, msg, bind, enter, exit, update, msg2, scrollbar, range, clamp, counts).":"ModSettings R39: UseNewWindow=0 - only get/set/msg installed; old IC window kept."):"ModSettings R39: hook group NOT installed (bytes differ / conflict). Old IC window stays.");
 if(!ok){r39::start(1,MOD,"FAIL",0,total,setting,"hook bytes differ or another mod patched the site");
  r43c::Owner own{};
  if(plan.bad){own=plan.bad_len?r39::site_owner(plan.bad_rva,plan.bad_expected,plan.bad_actual,plan.bad_len):r43c::Owner{};
   r39::fail_plan(1,MOD,plan,"mod-settings sites(get 0x3ec920/set 0x3f2200/std 0x638b00/scrollbar 0x658e6d/clamp 0x629c40/range 0x638c48)",L"모드 설정 창(1번)이 안 열림(옛 창도 안 뜸) → 게임 안에서 켜고 끄기 불가. 기능은 설치 때 값으로 그대로");}
  else{ // vtable 칸이 다름: 어느 칸인지 찾아 적음
   char where[160],ex[64],ac[64];where[0]=ex[0]=ac[0]=0;r39::scat(where,sizeof where,"vtable slot differs: ");bool found=false;
   for(U32 i=base;i<total&&!found;i++){S14_Hook&h=plan.hooks[i];if(h.length!=8)continue;if(!core->Readable(h.target,8)||memcmp(h.target,h.expected,8)){found=true;if(core->Readable(h.target,8))own=r43c::owner_of_ptr(*(const U8**)h.target);r39::scat(where,sizeof where,"exe+0x");r39::shex(where,sizeof where,(unsigned long long)(h.target-game));hex8(ex,sizeof ex,h.expected);if(core->Readable(h.target,8))hex8(ac,sizeof ac,h.target);else r39::scat(ac,sizeof ac,"unreadable");}}
   if(!found){r39::scat(where,sizeof where,"(none found; InstallHooks refused)");r39::scat(ex,sizeof ex,"-");r39::scat(ac,sizeof ac,"-");}
   fail_box("vtable",where,ex,ac,&own);}
  // 옛 창 대신 '모드' 버튼에서 충돌 안내(DLL 은 남겨야 하므로 1 을 돌려줌 — 버튼 릴레이가 우리 함수를 가리킴)
  set_blocked(&own);{HANDLE th=CreateThread(nullptr,0,relay_worker,nullptr,0,nullptr);if(th)CloseHandle(th);}
  if(core->Log)core->Log("ModSettings R43: new window not installed (conflict); info-window 'Mod' button will show a conflict notice instead of the old window.");
  return 1;}
 if(!use_new){r39::start(1,MOD,"OFF",total,total,ui_on?setting:"ModSettingsUI=0",ui_on?"old IC window by user setting":"No.1 Mod settings off in installer");return 1;}
 r39::start(1,MOD,"OK",total,total,setting,"-");
 HANDLE th=CreateThread(nullptr,0,relay_worker,nullptr,0,nullptr);if(th)CloseHandle(th);
 return 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
