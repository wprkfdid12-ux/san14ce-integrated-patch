#pragma once
// R43 공통: 모듈 등록 파일(SAN14Mods\modules\*.ini) ↔ 통합패치 기능 번호·짧은 이름. 안내창(Notice)·종합 진단(Diag)이 같이 쓴다.
// 번호 0 = 공통 부품(번호 없음). 이름은 설치기 목록(Spec.Names)을 줄인 것.
#include "common.h"
struct FeatIni{const wchar_t*ini;int feature;const wchar_t*name;};
static const FeatIni kFeatIni[]={
 {L"IntegratedConvenience_GreenlandCompat.ini",1,L"통합편의(1~7번 공통 부품)"},
 {L"ModSettings.ini",1,L"모드 설정 창"},
 {L"ICGuard.ini",1,L"통합편의 보강(ICGuard)"},
 {L"SettingsHelpScenic.ini",0,L"설정 도움말·명승 연출 생략"},
 {L"CrashLog.ini",0,L"튕김 기록"},
 {L"ExtBridge.ini",30,L"편의성 묶음 연결(ExtBridge)"},
 {L"Diag.ini",0,L"종합 진단"},
 {L"AutoPersonnelRepair.ini",2,L"자동인사 복구"},
 {L"AutoPersonnelRepairMerit.ini",2,L"자동논공 새 엔진"},
 {L"SceneSkip.ini",4,L"연출 생략(재해·등용·보물·턴 요약·카메라)"},
 {L"Traits9Check.ini",9,L"개성 9칸 확인"},
 {L"Mod09_Traits9Check.ini",9,L"개성 9칸 확인"},
 {L"MeritTraits.ini",13,L"논공 개성 가챠"},
 {L"RelatedOfficersTab.ini",15,L"연계(관련 장수) 탭"},
 {L"SortiePresets.ini",16,L"출진 프리셋"},
 {L"Stats200New.ini",17,L"능력치 상한 200"},
 {L"FPS120.ini",19,L"120프레임"},
 {L"PrisonerReturn.ini",21,L"포로 관직 유지·원래 도시 귀환"},
 {L"DomesticCE.ini",22,L"내정 자동화(DomesticCE)"},
 {L"DomesticTalent.ini",22,L"내정 인재 배치"},
 {L"DomesticAI.ini",22,L"AI 내정 순환(군주 반영)"},
 {L"DomesticPlayer.ini",22,L"플레이어 내정 자동"},
 {L"CityCaps.ini",23,L"도시 자원·병력 상한"},
 {L"Patch27.ini",25,L"타 세력 재야 등용·행동 중 관직·전사 알림·이름 4자"},
 {L"ExecuteLock.ini",26,L"처단 버튼 잠금"},
 {L"StatusColor.ini",29,L"항장·부상 색 표시"},
 {L"DragGive.ini",30,L"명품 다중 수여"},
 {L"DragLearn.ini",30,L"전법서 다중 습득"},
 {L"ItemPicture.ini",30,L"보물 그림"},
 {L"WheelRegion.ini",30,L"휠클릭 지역내정"},
 {L"HiredFace.ini",32,L"고용무장 원본 얼굴"},
 {L"Range.ini",33,L"안행·누선/투석 사거리"},
 {L"RegionOfficer.ini",34,L"지역담당관 공석 경고"},
};
inline const FeatIni*feat_of_ini(const wchar_t*name){
 for(const FeatIni&f:kFeatIni){const wchar_t*a=f.ini,*b=name;while(*a&&*b){wchar_t x=*a,y=*b;if(x>=L'A'&&x<=L'Z')x+=32;if(y>=L'A'&&y<=L'Z')y+=32;if(x!=y)break;a++;b++;}if(!*a&&!*b)return &f;}
 return nullptr;
}
