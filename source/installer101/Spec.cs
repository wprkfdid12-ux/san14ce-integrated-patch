using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Linq;

namespace San14Patch101 {
// 삼국지14 CE 통합패치 (공식 2026-11-05 업데이트판 EXE 기준; R42i4 까지는 1.0.1) — 기능 목록·분류·충돌 규칙·파일 목록(manifest)·설정 파일 생성 규칙
public static class Spec {
 public const string Version="R43.30";   // R43.28(11-11)부터 '큰 판.차수'. 깃허브 배포 판 번호(태그)와 같아야 함(Update.cs)
 public const int Count=34;
 public const int FirstNew=26; // (옛 표시 기준 — R43.28 부터는 아래 NewIds·Changed 를 씀)
 // R43.28: 이번 판에서 새로 생긴 번호([새로])·바뀐 번호([바뀜] + 무엇이 바뀌었나). 판마다 고친다. 이번 판은 설치기만 바뀜.
 public static readonly int[] NewIds={};
 public static readonly Dictionary<int,string> Changed=new Dictionary<int,string>();
 public static string Mark(int id){return Array.IndexOf(NewIds,id)>=0?"[새로]":Changed.ContainsKey(id)?"[바뀜]":"";}
 // 지원 게임 EXE (디스크 파일, Steam 보호 포함) : CE 한국어 2026-11-05 업데이트판(PE 빌드 시각 0x6ad06f53, 이미지 0x2a02000)
 public const string GameLabel="2026-11-05 업데이트판";   // 사용자에게 보이는 지원 판 이름(옛 판 이름은 OldGameLabels)
 public const string ExpectedGame="782cf229a56cdde2b95399e0e7536740b3f5e96bea702679365e4dad061abdce";
 // 옛 판(설치 거부): 1.0.1(2026-09-02 빌드, R42i4 까지의 기준판) / 1.0.0(2026-08-01)
 public static readonly string[] OldGames={"87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a","07a047826cfc316dc9e3db6bfddc72b85e8d7de99743cc73a2d7199a1ad3aa40"};
 public static readonly string[] OldGameLabels={"1.0.1(2026-09-02) — R42i4 이하 통합패치용","1.0.1 이전(2026-08-01)"};
 public const string OldGame="87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a";   // 호환용(가장 가까운 옛 판)
 public static int OldGameIndex(string sha){for(int i=0;i<OldGames.Length;i++)if(OldGames[i]==sha)return i;return -1;}
 public static readonly string[] Names={
  "모드 설정 창","턴 종료 시 자동 포상·명승 투자·논공·자동인사","사신 자동 거절(문전박대)","카메라 이동 끄기·반복 보고 넘김·명승/증축/재해/등용 결과/보물 발견 연출 생략·턴 끝 결과 창(버튼 8개)","여러 부대 함께 이동·아군 부대 따라가기(수행)",
  "진행 배속","인재·포로 편의","BGM 추가·플레이어","개성 9개·콜라보 해금","고유 무장 등록·추가 3D 모델 149개",
  "필드 부대 이미지 3D 조형으로(최적화 포함)","필드 안개(전장의 안개 1.6·미니맵)","논공 개성 가챠·군주도 적용","개성·전법 마우스 설명","모든 정보 탭에 연계 탭 추가",
  "출진 프리셋","능력치 상한 200","경험치로 능력치 엄청 안 오름","120프레임","글씨체 변경·고운 바탕체",
  "포로 관직 유지·원래 소속 도시 귀환","내정 자동화(모든 내정·AI 명령서 99장 유지·최소 인구 1만)·저돌 수정·AI 내정 순환(군주 반영)·AI 명승 투자 금 제한 — 스위치 6개","도시 자원·병력 상한 확장","수여로 오르는 능력치 미적용","타 세력 재야 등용·행동 중 관직 수여·전사 알림·이름 4자",
  "기능확장 v1.5 — 처단불가·재야 갇힘 해소·2차 탐색 제한","독립 미니맵(안개 없이)","인구 증가 보너스·전투 인구 감소 50%","항장·부상 색 표시","편의성 v11 — 메뉴 빠르게·휠클릭 지역내정·전법서 다중 습득·명품 다중 수여",
  "주메뉴·버튼 그래픽 v30","고용무장 원본 얼굴(167장)","안행·누선/투석 사거리 설정","지역담당관 공석 경고"};
 // 설명칸 4항목: [0]무엇을 하나 [1]켜면 이렇게 됩니다 [2]게임 안에서 켜고 끄는 곳 [3]같이 못 쓰는 것·주의
 public static readonly string[][] Details={
  new[]{"게임 안 정보창에 '모드' 버튼으로 여는 모드 설정 창입니다. 이번 판부터 큰 창(분류 탭 6개)으로 바뀌었습니다.","정보창 → 모드 버튼을 누르면 편의/밸런스/정보/그래픽/무장/사운드 탭으로 나뉜 설정 창이 뜨고, 각 번호의 스위치·선택지를 바로 바꿀 수 있습니다. 아래 설명 칸에 선택한 줄의 설명이 나옵니다.","20·31번 말고 어떤 번호를 고르든 자동으로 같이 깔리고 뺄 수 없습니다(다른 번호의 게임 안 스위치가 모두 여기 있음).","거의 모든 번호가 이 창에 스위치를 둡니다. 옛 설정 창은 더 이상 쓰지 않습니다. 다른 모드가 같은 자리를 먼저 고쳐 이 창을 못 붙이면 '모드' 버튼에서 충돌 안내 창이 뜹니다."},
  new[]{"턴을 끝낼 때 포상·명승지 투자·논공·자동인사(남은 무장 배치)를 자동으로 해 줍니다.","턴 종료 때 충성 낮은 무장 포상, 명승지 투자, 논공, 남은 명령서로 무장 탐색이 자동으로 처리됩니다(군주가 도시 밖 관문·항구에 있어도 본거지 기준으로 처리 — R39 수정). 인사 메뉴에 '자동인사' 버튼도 생깁니다.","모드 설정 창 → 편의 탭 → '자동 포상'·'자동 투자'·'자동 논공'·'자동인사' 각각 켬/끔. '자동인사'를 끄면 인사 메뉴의 자동인사 버튼이 사라지고 자동 포상도 동작하지 않습니다(자동 포상은 자동인사에 딸린 기능).","13번 논공 개성 가챠와 같이 설치하면 자동 논공만 강제로 꺼집니다(논공 기록이 어긋나는 충돌 방지). 설치기에서 둘을 같이 고르면 알림창으로 안내합니다."},
  new[]{"찾아온 사신을 자동으로 거절합니다.","외교 메뉴의 문전박대 버튼으로 OFF·ON·항상을 고릅니다. ON 이면 선택적으로, 항상이면 모든 사신을 돌려보냅니다.","모드 설정 창 → 편의 탭 → '문전박대 버튼 보이기'. 끄면 외교 메뉴 버튼이 사라지고 기능도 꺼집니다. 설치기에서 끄면 처음부터 숨김.","없음."},
  new[]{"카메라 자동 이동 끄기 / 반복 보고 넘기기 / 명승 연출 생략(암전 제거 포함) / 증축 이벤트 생략 / 재해 연출 생략 / 등용 결과 장면 생략 / 보물 발견 창 생략 / 턴 끝 결과 창 — 버튼 8개입니다.","카메라 자동 이동을 끄면 사건·재해(발생 중 알림 포함)·명승·컷신·이벤트 때 카메라가 그 장소로 날아가거나 확대되는 것을 모두 막습니다(기록 창 '위치로 가기'처럼 직접 누른 이동은 그대로). 반복 보고 화면이 자동으로 넘어가고, 명승·증축 연출이 생략돼 턴이 빨리 갑니다. 역병·메뚜기·풍작은 알림 띠·재해 음악·창 없이 넘어가고, 등용 결과 대사 장면과 보물(명품) 발견 창도 안 뜹니다. 생략된 결과는 화면 밑 로그창에 한 줄씩 남고, 턴 처리가 끝날 때 재해·등용 결과·보물 발견·명승·증축으로 묶은 결과 창이 한 번 뜹니다. 결과·보상·피해·기록은 원작 그대로입니다.","모드 설정 창 → 편의 탭 → 8개 줄 각각 켬/끔(카메라 자동 이동은 기본 끔, 나머지 기본 켬).","없음."},
  new[]{"여러 부대를 함께 움직이고 아군 부대를 따라가게(수행) 합니다.","부대 여러 개를 골라 한 번에 이동 명령, 아군 부대를 지정하면 따라갑니다. 불러오기 후에는 수행을 다시 지정해야 합니다.","모드 설정 창 → 편의 탭 → '여러 부대 이동'·'수행' 켬/끔.","없음."},
  new[]{"게임 진행 속도 단계를 늘립니다.","원본 화살표를 다시 누르면 2배속 터보가 됩니다.","모드 설정 창 → 편의 탭 → '진행 배속' 켬/끔.","없음."},
  new[]{"포로 회유·등용문·인사 말풍선 설명·포로 초상 등 인재·포로 편의 기능입니다.","붙잡은 포로를 회유하는 추가 선택지, 재야가 임관을 청하는 등용문, 인재 화면 추가 설명, 포로 화면에 포박 일러스트가 나옵니다.","모드 설정 창 → 편의 탭 → '포로 회유'·'등용문'·'인사 설명'·'포로 초상' 켬/끔.","없음."},
  new[]{"BGM 을 추가하고 게임 안에서 골라 재생하는 플레이어입니다.","San14BgmExpansion 폴더에 넣은 음원을 게임 안에서 선택해 재생합니다. 사용자 음원과 설정은 보존됩니다.","모드 설정 창 → 사운드 탭 → 'BGM 플레이어' 켬/끔과 곡 선택.","없음."},
  new[]{"콜라보 개성·전법 해금(모음팩 v1.7 Collab)과 개성 9칸입니다. 개성 9칸은 9·13번 공용 부품이 맡습니다(9칸을 못 붙이면 게임 안에 '개성 9칸 - 오류 코드' 창이 뜹니다).","무장 개성 칸이 9개가 되고 콜라보 개성·전법을 쓸 수 있습니다. 게임 데이터 파일은 바꾸지 않습니다.","설치만 하면 켜집니다(게임 안 스위치 없음).","13번과 같이 켜도 됩니다. 개성 9칸은 공용 부품 하나가 맡습니다."},
  new[]{"고유 무장 등록·모델 ID 확장(0~173)과 옛 통합본의 추가 3D 모델 149개(ID 25~173), 무장 연결 115명입니다.","무장 정보창의 조형 설정에서 무장마다 3D 모델을 고를 수 있고, 추가 모델 149개가 더 보입니다. 게임 0002 폴더 색인 3개는 1.0.1 원본일 때만 바꾸고 제거하면 원본으로 되돌립니다.","무장 정보창 → 조형 설정. 추가 모델 자료는 별도 ZIP 에 있습니다(없으면 기본 모델 0~24만).","11번 3D 조형과 연결됩니다. 다른 모드가 같은 색인 레코드를 바꿔 두었으면 추가 모델만 건너뜁니다."},
  new[]{"필드 부대 이미지를 일기토 3D 조형으로 바꿉니다(화면 밖 동작 갱신 중단 등 최적화 포함).","지도 위 부대가 2D 그림 대신 3D 조형으로 나옵니다. 1.0.1 EXE 기준으로 다시 빌드했습니다.","설치만 하면 켜집니다. 모드 설정 창 → 그래픽 탭에 최적화 스위치.","게임 폴더에 2D 모드(d3d11.dll)가 있으면 필드 그림을 둘이 바꾸므로 같이 못 씁니다(설치기가 막음). 맵 확장(dinput8)과는 같이 쓸 수 있습니다."},
  new[]{"전장의 안개 1.6(1.0.1 대응판, 미니맵 포함)입니다.","안 보이는 지역은 안개로 가려지고, 시설은 영향 범위(공격 시설은 최대 사거리)만큼 시야를 가집니다. 미니맵이 함께 뜨며 압정 버튼으로 고정/이동(끌어서 옮김)합니다.","모드 설정 창 → 그래픽 탭 → '미니맵 표시'·'날씨' 켬/끔(바로 적용).","27번 독립 미니맵과 같이 설치하면 미니맵이 둘이라 설치기가 막습니다. 11번과 함께 쓰면 안개 연동 3D 최적화가 같이 적용됩니다."},
  new[]{"논공 개성 가챠 R17(군주에게도 적용)입니다. 개성 9칸(9·13번 공용 부품)이 함께 깔립니다.","논공 때 개성을 뽑는 가챠가 생기고, 군주도 대상입니다. 항장(공로 마이너스)이 되면 이전 소속에서 가챠로 받은 개성은 사라지고 랭크는 게임 값 그대로입니다(R39 수정, 순정과 다른 점).","설치만 하면 켜집니다. 가챠는 논공 화면에서.","2번의 자동 논공은 이 번호가 있으면 강제로 꺼집니다. 개성 9칸은 9번과 같은 공용 부품입니다(9번 없이도 9칸)."},
  new[]{"개성·전법에 마우스를 올리면 설명이 뜹니다(모음팩 v1.7 TraitHover, 전법 설명 TXT 포함).","정보 표의 개성·전법 위에 마우스를 올리면 효과 설명창이 나옵니다.","설치만 하면 켜집니다.","9·13번과 함께면 개성 설명은 개성 9칸 화면이, 전법 설명은 이 번호가 띄웁니다(9 단독·13 단독·둘 다 같음)."},
  new[]{"모든 정보 탭에 연계(관련 장수) 탭을 추가합니다.","무장·도시 정보창 맨 오른쪽에 연계 탭이 생겨 관련 무장을 바로 볼 수 있습니다.","무장 정보창 맨 오른쪽 '연계' 탭. 게임 안 스위치 없음.","없음."},
  new[]{"출진 프리셋(그룹·자동 저장)입니다.","출진 화면에서 부대 구성을 그룹으로 저장하고 불러옵니다. 현재 그룹은 자동 저장됩니다.","출진 화면의 프리셋 버튼. 게임 안 스위치 없음.","없음."},
  new[]{"능력치 상한 200 해금과 100 이후 자연 성장(모음팩 v1.7 Stats200)입니다.","능력치가 100을 넘어 200까지 오르고, 편집 무장 파일 읽기 상한이 100→200 이 됩니다.","설치만 하면 켜집니다.","능력치 200 관련 처리는 이 번호에만 있습니다. 32번 고용무장 얼굴은 신무장 칸 200 기준으로 중복을 검사합니다."},
  new[]{"경험치로 능력치가 거의 오르지 않게 하는 데이터 파일입니다.","전투·행동 경험치로 능력치가 오르는 폭이 크게 줄어듭니다.","모드 설정 창 → 밸런스 탭 → '경험치 성장 억제' 켬/끔(게임 재시작 후 적용).","게임 데이터 파일(0000/experience_data_exce.bin)을 바꿉니다. 제거하면 1.0.1 원본으로 되돌립니다."},
  new[]{"최대 120프레임(모음팩 v1.7 FPS120, 1.0.1 대응판)입니다.","지도·전투 화면이 120프레임까지 나옵니다. 일기토는 원본 60프레임 타이밍을 유지합니다.","설치만 하면 켜집니다.","없음."},
  new[]{"고운 바탕체 글꼴 자료를 적용합니다.","게임 글씨가 고운 바탕체로 바뀝니다. 기존 백업 자료가 있으면 재사용합니다.","설치만 하면 켜집니다(파일 교체).","0002/RRPreview.rdb 를 바꿉니다. 다른 글꼴 패치가 깔려 있으면 설치기가 막습니다(원본으로 되돌린 뒤 설치)."},
  new[]{"포로가 석방·귀환될 때 관직을 유지하고 원래 소속 도시로 돌아갑니다.","붙잡혔다 풀려난 무장이 관직을 잃지 않고 원래 도시로 갑니다. 저장 슬롯별로 기록합니다.","설치만 하면 켜집니다.","없음."},
  new[]{"모든 내정 자동 / AI 명령서 99장 유지 / 최소 인구 1만 유지 / 저돌 수정 / AI 내정 순환(군주 반영) / AI 명승 투자 금 제한 — 스위치 6개입니다.","① 내 지역 중점 개발을 골고루(가장 덜 큰 항목부터) 자동으로 맞춤 — 직접 고른 중점은 그대로 둠(고른 항목이 다 크면 다음 항목으로) ② AI 세력 명령서 99장 유지 ③ 도시 인구가 1만 밑으로 안 내려감 ④ 개성 '저돌' 효과를 고친 값으로 적용. ⑤ AI 세력이 지역을 상업→농업→병영 차례로 골고루 키움 — 군주 정치 70 이상이면 전 지역, 40~69 절반, 40 미만 3분의 1만 골고루, 나머지 지역은 군주가 중시하는 물자(금→상업, 군량→농업, 병사→병영)만. 어느 지역이 골고루인지는 달마다 바뀜. 플레이어 지역은 그대로. 지역 값이 이상한 지역이 있어도 그 지역만 건너뛰고 나머지 내정 자동·최소 인구·AI 명령서는 계속 처리합니다. ⑥ AI 세력은 명승에 투자한 뒤 남는 금이 비용의 2배 이상·5000 이상일 때만 투자(가난한 도시가 명승에 금을 다 쓰지 않게, 플레이어는 그대로).","모드 설정 창 → 편의·밸런스 탭 → 6개 줄 각각 켬/끔. 설정은 SAN14_DomesticCE.ini(앞 4개·명승 금 제한 AiScenicGold)·SAN14Mods\\modules\\DomesticAI.ini(AI 순환)에 보존됩니다.","28번 인구 보너스와 같이 써도 되지만, 인구 1만 아래에서는 ③이 먼저 올려 버려 28번 보너스가 표가 안 납니다."},
  new[]{"도시 자원·병력 상한의 기준값을 늘립니다.","금 = 도시 레벨 × 15만(원래 5만), 병량 = 도시 레벨 × 20만(원래 10만), 병력 = 10만(원래 5만) + 레벨별 가산. 게임이 따로 고정해 둔 특수 도시 상한은 그대로.","설치만 하면 켜집니다.","능력치 200 부분은 17번으로 분리돼 있습니다."},
  new[]{"관직·작위·방책 수여로 더해지는 능력치 보너스를 적용하지 않습니다(1.0.1 공용 데이터 파일 299개 항목 편집).","관직을 줘도 능력치가 안 오릅니다.","모드 설정 창 → 밸런스 탭 → '수여 능력치 미적용' 켬/끔(게임 재시작 후 적용).","게임 데이터 파일(0010_KO/fixdataexce.s14)을 바꿉니다. 제거하면 1.0.1 원본으로 되돌립니다."},
  new[]{"타 세력 도시 재야 등용, 행동 중 무장 관직 수여, 전사 알림(일러스트·메시지·기록), 신무장 이름 4자 확장입니다.","타 세력 도시의 재야도 등용 후보에 나오고, 출진 중 지휘관에게 관직을 줄 수 있고, AI끼리의 전투를 포함한 전사가 알림으로 뜹니다.","모드 설정 창 → 밸런스 탭 → '타 세력 등용'·'행동 중 관직'·'전사 알림' 켬/끔. 이름 4자는 항상.","없음."},
  new[]{"기능확장 v1.5 묶음: 처단불가 / 재야 갇힘 해소 / 2차 탐색 제한 — 셋을 따로 켜고 끕니다.","① 처단불가: 플레이어·AI 모두 포로를 처단할 수 없습니다(혐오 무장 처단도 막힘) 켜져 있으면 포로 처우 창·7번 포로 회유 창의 처단 버튼이 회색이 되어 누를 수 없습니다(군주 포로 창은 원래도 꺼짐). ② 재야 갇힘 해소: 혐오 군주 도시의 재야는 매 턴 떠나고, 재해로 갇힌 재야는 6개월 뒤 걸어 나갑니다. ③ 2차 탐색 제한: AI 의 2차 탐색을 제한합니다.","모드 설정 창 → 밸런스 탭 → 3개 줄 각각 켬/끔(게임 안 끄고 바로 적용). 설치하면 셋 다 켜진 상태로 시작합니다.","없음."},
  new[]{"안개 없이 미니맵만 씁니다.","지도 왼쪽 아래에 미니맵이 뜹니다(안개 기능 없음). 압정 버튼으로 고정/이동.","모드 설정 창 → 그래픽 탭 → '미니맵 표시' 켬/끔, 크기 선택.","없음. (R41: 12번 안개는 통합패치에서 빠졌습니다. 미니맵은 이 번호로 씁니다.)"},
  new[]{"인구 증가 보너스와 전투 인구 감소 50%(모음팩 v1.9)입니다.","인구 증가 보너스: 1·4·7·10월 순정 증가에 더합니다(표준 / 최대 +3,000 / 최대 +5,000). 인구 5,000명 이하는 보너스 전액, 20,000명까지 비례 감소, 20,000명 이상은 없음. 전투 인구 감소 50%: 전투로 줄어드는 인구가 절반이 됩니다.","모드 설정 창 → 밸런스 탭 → '인구 증가 보너스'(표준/+3,000/+5,000)·'전투시 인구 감소'(표준/50%). 설치 직후 기본값은 표준(=꺼짐)입니다.","22번 최소 인구 1만과 같이 써도 됩니다(1만 아래에서는 22번이 먼저 올려 보너스가 표가 안 남). AI 도시에도 똑같이 적용됩니다."},
  new[]{"항장(공로 마이너스)과 부상 무장의 이름·능력치를 색으로 표시합니다.","항장은 이름과 능력치가 회색, 부상은 빨강(순정은 숫자만 빨강), 둘 다면 빨강입니다. 무장 목록·무장 정보·관직 수여·출진·인사 선택 등 이름과 능력치가 나오는 창 전부.","모드 설정 창 → 정보 탭 → '항장·부상 색 표시' 켬/끔.","없음."},
  new[]{"편의성 모드 v11 묶음: 메뉴 빠르게 / 휠클릭 지역내정 / 전법서 다중 습득 / 명품 다중 수여 — 넷을 따로 켜고 끕니다.","① 메뉴 빠르게: 도시·부대를 클릭하면 메뉴가 바로 뜹니다(원래 약 0.2초 대기). ② 휠클릭 지역내정: 지도 빈 칸에서 휠 클릭 → 그 지역의 마을·항구·도시가 바로 선택됩니다. ③ 전법서 다중 습득: 전법서를 최대 10권, 무장 최대 10명까지 한 번에 고릅니다(명령서만큼). ④ 명품 다중 수여: 명품 여러 개를 한 번에 주고 창이 안 닫혀 연속으로 줄 수 있습니다. 명품 고르는 목록 왼쪽 그림 칸에는 보유 무장 얼굴 대신 마우스를 올린 명품 그림이 나옵니다.","모드 설정 창 → 편의 탭 → 4개 줄 각각 켬/끔(게임 안 끄고 바로 적용). 설치하면 넷 다 켜진 상태로 시작합니다.","다른 편의성 모드 로더(version.dll + SAN14Conv 폴더)를 따로 깔았다면 지우세요(두 번 적용 방지)."},
  new[]{"CE 주메뉴·버튼 그래픽 v30 입니다(글꼴은 포함하지 않음).","지도 왼쪽 주메뉴와 버튼 그래픽이 바뀝니다. 화면 색인(ScreenLayout.rdb)의 레코드 9개만 바꾸고 그림은 새 컨테이너 파일에 넣습니다.","설치만 하면 켜집니다(파일 교체). 게임 안 스위치 없음.","0002/ScreenLayout.rdb 색인이 1.0.1 원본일 때만 바꾸고(다른 모드가 같은 레코드를 바꿨으면 건너뜀), 제거하면 원본으로 되돌립니다. 20번 글꼴과는 다른 파일이라 같이 써도 됩니다."},
  new[]{"고용무장에게 범용 얼굴 대신 게임 내장 오리지널 얼굴 167장(남 101·여 66)을 배정합니다.","고용무장이 생길 때 무장 번호 기준으로 오리지널 얼굴을 고르고, 살아 있는 무장·신무장이 쓰는 얼굴은 건너뜁니다. 167장을 다 쓰면 기본 얼굴로 돌아갑니다. 불러오기 때도 한 번 훑어 재배정합니다. 그림 파일 추가 없음.","모드 설정 창 → 무장 탭 → '고용무장 원본 얼굴' 켬/끔.","끄거나 제거해도 세이브는 그대로입니다(얼굴 번호만 남음)."},
  new[]{"안행·누선 진형과 투석 진형의 공격 사거리를 정합니다.","안행·누선 사거리: 표준 2칸 ~ 4칸, 투석 사거리: 표준 1칸(게임 그대로) ~ 4칸(2칸 이상이면 투석도 원거리 부대로 취급). 아군·적군·AI 모두 같은 값입니다. 세이브에는 저장되지 않습니다.","모드 설정 창 → 밸런스 탭 → '안행·누선 사거리'·'투석 사거리' ◀ ▶ 로 조절, [표준]으로 되돌림. 바로 적용.","없음."},
  new[]{"지역담당관이 없는 지역을 지도에서 바로 알 수 있게 경고를 붙입니다.","담당관이 없는 내 지역은 지도 거점 이름이 빨간 글씨로 보입니다(이름 글자·칸은 그대로라 잘리지 않음). 담당관을 임명하면 바로 원래 색으로 돌아갑니다.","모드 설정 창 → 정보 탭 → '지역담당관 공석 경고' 켬/끔(바로 적용).","없음."}};
 // 설명칸에서 쓰는 한 줄 요약(기존 호환: 마우스오버·옛 호출)
 public static string Describe(int id){var d=Details[id-1];return d[0]+"\r\n\r\n■ 켜면 이렇게 됩니다\r\n"+d[1]+"\r\n\r\n■ 게임 안에서 켜고 끄는 곳\r\n"+d[2]+"\r\n\r\n■ 같이 못 쓰는 것·주의\r\n"+d[3];}
 public static readonly string[] Descriptions=Enumerable.Range(1,Count).Select(i=>Details[i-1][0]).ToArray();
 // ---------- 분류 탭 ----------
 public static readonly string[] CategoryNames={"편의","밸런스·규칙","정보·UI","그래픽·화면","무장·얼굴","사운드"};
 // 번호 → 분류 번호(0~5). 수정안 D절 표.
 public static readonly int[] CategoryOf={0,0,0,0,0, 0,0,5,2,4, 3,3,1,2,2, 2,1,1,3,3, 1,0,1,1,1, 1,3,1,2,0, 3,4,1,2};
 public static int Category(int id){return CategoryOf[id-1];}
 public static int[] InCategory(int cat){return Enumerable.Range(1,Count).Where(id=>CategoryOf[id-1]==cat&&!Retired(id)).ToArray();}
 // ---------- 충돌 규칙 (설치기 단계) ----------
 public enum ConflictKind{Block,Notice}
 public sealed class Conflict{
  public string Key;public ConflictKind Kind;public int A,B;            // B=0 이면 외부 조건(게임 폴더 파일)과의 충돌
  public string Title,Message;public string[] Choices;                 // Choices: 사용자가 고를 버튼 글. 결과는 Resolve 로 적용
  public Func<string,bool> External;                                   // 외부 조건 검사(게임 폴더 경로) — 참이면 충돌
  // 이번 설치와 관계있는가: A·B 중 하나라도 '새로' 설치되는 경우만(둘 다 이미 깔려 있으면 지난 설치에서 이미 처리됨 — 다른 번호를 깔 때 다시 묻지 않음)
  public bool Relevant(bool[] before,bool[] on){if(before==null)return true;bool newA=on[A-1]&&!before[A-1];bool newB=B!=0&&on[B-1]&&!before[B-1];return newA||newB;}
  public string Check(bool[] on,string root){if(!on[A-1])return null;if(B!=0){if(!on[B-1])return null;}else if(External==null||!External(root))return null;return Message;}
 }
 // 충돌 창에서 고른 답을 적용(창 없이 시험 가능). 반환: false = 취소. resolved 에는 "둘 다 설치"로 해결된 규칙 키가 들어감
 public static bool ApplyChoice(string key,int pick,bool[] wanted,HashSet<string> resolved){
  switch(key){
   case "13-2": if(pick==0){resolved.Add(key);return true;} if(pick==1){wanted[12]=false;return true;} return false; // 0 = 2번은 자동 논공만 끄고 둘 다 설치(설치기가 AutoCredit=0)
   case "11-2D": if(pick==0){wanted[10]=false;return true;} return false;
   case "20-font": if(pick==0){wanted[19]=false;return true;} if(pick==1){resolved.Add(key);return true;} return false;
   default: if(pick==0){resolved.Add(key);return true;} return false;
  }
 }
 // 답을 적용한 뒤에도 남은 차단 충돌(없으면 null)
 public static Conflict RemainingBlock(bool[] wanted,string root,HashSet<string> resolved){
  return RemainingBlock(wanted,root,resolved,null);
 }
 public static Conflict RemainingBlock(bool[] wanted,string root,HashSet<string> resolved,bool[] before){
  foreach(var c in Conflicts){if(c.Kind!=ConflictKind.Block||resolved.Contains(c.Key)||!c.Relevant(before,wanted))continue;if(c.Check(wanted,root)!=null)return c;}return null;
 }
 static bool FileThere(string root,string rel){try{return File.Exists(Path.Combine(root,rel.Replace('/',Path.DirectorySeparatorChar)));}catch{return false;}}
 public static Func<string,bool> ForeignFont; // 20번: 글꼴 파일이 원본도 우리 것도 아님 (Engine 이 설정)
 // 2D 모드 판별: 2D 자료(San14_2D\San14Officers.bin) 또는 2D 표식이 든 d3d11.dll / d3d11_chain.dll(계절 맵 아래로 옮겨진 2D).
 public static bool Has2D(string root){return FileThere(root,"San14_2D/San14Officers.bin")||Is2DDll(root,"d3d11_chain.dll")||Is2DDll(root,"d3d11.dll");}
 // d3d11.dll 이름은 계절 맵(3DMigoto)·ReShade 등도 씀 → 파일 안에 2D 모드 표식(San14_2D)이 있을 때만 2D 로 봄
 static bool Is2DDll(string root,string rel){try{string p=Path.Combine(root,rel);if(!File.Exists(p))return false;var b=File.ReadAllBytes(p);
  return Find(b,Encoding.ASCII.GetBytes("San14_2D"))||Find(b,Encoding.Unicode.GetBytes("San14_2D"));}catch{return false;}}
 static bool Find(byte[] h,byte[] n){for(int i=0;i+n.Length<=h.Length;i++){int k=0;while(k<n.Length&&h[i+k]==n[k])k++;if(k==n.Length)return true;}return false;}
 public static readonly Conflict[] Conflicts={
  new Conflict{Key="13-2",Kind=ConflictKind.Block,A=13,B=2,Title="13번과 2번의 자동 논공은 같이 못 씁니다",
   Message="13번 '논공 개성 가챠'를 쓰면 2번 '턴 종료 자동 처리' 안의 자동 논공은 같이 못 씁니다(논공 기록이 어긋나 저장이 막히는 충돌). 2번의 자동 포상·투자·탐색은 그대로 쓸 수 있습니다.\r\n\r\n어떻게 할까요?",
   Choices=new[]{"2번은 자동 논공만 끄고 둘 다 설치","13번 빼기","취소"}},
  new Conflict{Key="11-2D",Kind=ConflictKind.Block,A=11,B=0,External=root=>Has2D(root),Title="11번은 2D 모드와 같이 못 씁니다",
   Message="게임 폴더에 2D 모드(d3d11.dll)가 있습니다. 11번 3D 조형과 2D 모드는 필드 부대 그림을 둘 다 바꾸므로 같이 못 씁니다.\r\n\r\n어떻게 할까요?",
   Choices=new[]{"11번 빼기","취소"}},
  new Conflict{Key="20-font",Kind=ConflictKind.Block,A=20,B=0,External=root=>ForeignFont!=null&&ForeignFont(root),Title="20번: 다른 글꼴 패치가 있습니다",
   Message="게임 0002/RRPreview.rdb 가 원본도 이 패치의 글꼴판도 아닙니다(다른 글꼴 패치 또는 BGM 적용). 그대로 두면 글꼴 색인이 충돌합니다.\r\n\r\n어떻게 할까요?",
   Choices=new[]{"20번 빼기","백업 후 그래도 설치","취소"}},
  new Conflict{Key="22-28",Kind=ConflictKind.Notice,A=22,B=28,Title="22번 + 28번",Message="22번 최소 인구 1만과 28번 인구 보너스를 같이 켜면, 인구 1만 아래에서는 22번이 먼저 올려 28번 보너스가 표가 안 납니다. 같이 써도 됩니다."},
  new Conflict{Key="11-map",Kind=ConflictKind.Notice,A=11,B=0,External=root=>FileThere(root,"dinput8.dll")&&FileThere(root,"gate1_backup"),Title="11번 + 맵 확장",Message="게임 폴더에 맵 확장(dinput8)이 있습니다. 11번 3D 조형은 R36 부터 맵 확장과 같이 쓸 수 있습니다."},
 };
 // 제거 때 "같이 바뀌는 것" 안내
 public static string RemovalNote(bool[] current,bool[] removing){
  var b=new StringBuilder();
  if(removing[12]&&current[1])b.AppendLine("· 13번을 빼면 2번의 자동 논공 잠금이 풀려 다시 켤 수 있습니다.");
  if(removing[0])b.AppendLine("· 1번을 빼면 다른 번호의 게임 안 스위치(모드 설정 창)가 사라집니다.");
  if(removing[9]&&current[10])b.AppendLine("· 10번을 빼면 11번 3D 조형은 기본 모델만 씁니다.");
  if(removing[8]&&!current[12])b.AppendLine("· 9번을 빼면 개성 9개 칸이 5개로 돌아갑니다(13번 없음).");
  return b.ToString();
 }
 // 옛 설치기(내부 번호 1~28) → 새 번호. 없어진 항목(옛 3 논공 동시, 10 이민족 성, 26 군단 명령)은 0.
 public static readonly int[] FromLegacy={7,2,0,3,4,5,6,1,8,0,9,10,11,13,14,15,16,17,18,19,20,12,21,22,23,0,24,25};
 public static int Legacy(int oldId){return oldId>=1&&oldId<=28?FromLegacy[oldId-1]:0;}
 // 통합편의(IC) 설정 파일 키: 번호별 (섹션\t키 = ON값). OFF 는 0 (AutoFocus 는 1).
 public static readonly Dictionary<int,string[]> IniKeys=new Dictionary<int,string[]>{
  {1,new[]{"Modules\tModSettingsUI=1"}},
  {2,new[]{"Convenience\tAutoPersonnelModule=1","Convenience\tAutoPersonnel=1","Convenience\tAutoScenery=1","Convenience\tAutoCredit=1"}},
  {3,new[]{"Convenience\tAudienceGate=2","Convenience\tAudienceButton=1"}},
  {4,new[]{"Camera\tAutoFocus=0","Convenience\tSkipReports=1","Convenience\tScenicRewardOnly=1","Convenience\tScenicNoFade=1","Convenience\tCityNoFade=1"}},
  {5,new[]{"Convenience\tFormationMoveModule=1","UnitFollow\tEnabled=1"}},
  {6,new[]{"Convenience\tProgressSpeedModule=1"}},
  {7,new[]{"Convenience\tPrisonerPersuasion=1","Convenience\tRecruitmentGate=1","Convenience\tPersonnelTooltips=1","Convenience\tTraitTooltips=1","Convenience\tCaptivePortrait=1","Convenience\tNoPositionTroops=5000","Army\tNoPositionTroops=5000"}},
  {8,new[]{"Modules\tBgmExpansionModule=1"}},
  {9,new[]{"Modules\tTraits9Module=1"}},
  {10,new[]{"Modules\tModelRegistryModule=1"}}};
 // 공통 런타임(IC·ModCore·로더)이 필요한 번호: 데이터/글꼴/화면 파일만 바꾸는 18·20·24·31 을 뺀 전부
 public static bool NeedsRuntime(int id){return id!=18&&id!=20&&id!=24&&id!=31;}
 public static bool AnyRuntime(bool[] on){for(int i=0;i<Count;i++)if(on[i]&&NeedsRuntime(i+1))return true;return false;}
 // R43 11-08: 1번(모드 설정 창) 잠금 — 다른 번호의 게임 안 스위치가 모두 1번 창에 있으므로, 20·31(게임 안 스위치 없음) 말고 다른 번호가 하나라도 깔리면 1번도 반드시 깔림.
 //  다른 번호가 없으면 1번은 사용자가 고른 대로(전부 제거 때 같이 빠짐, 20·31 만이면 안 깔림).
 public static bool UsesSettingsWindow(int id){return id!=1&&id!=20&&id!=31&&!Retired(id);}
 public static bool[] WithSettingsWindow(bool[] on){var c=(bool[])on.Clone();for(int i=0;i<Count;i++)if(c[i]&&UsesSettingsWindow(i+1)){c[0]=true;break;}return c;}
 public static string Num(int id){return id.ToString();}
 public static string Tag(int id){return id>=1&&id<=Count?Num(id)+"번 "+Names[id-1]:"공통 런타임";} // R29: 공통 파일(번호 0)도 안전하게
 // R29: 설치 자료를 찾을 폴더들 — 설치 자료 폴더 자신, 그 안 하위 폴더, 바로 옆 폴더(ZIP 을 각각 다른 폴더에 푼 경우). 파일은 항상 SHA-256 으로 확인하고 씀
 public static List<string> PayloadRoots(string pkg){
  var list=new List<string>();string root=Path.GetFullPath(pkg).TrimEnd(Path.DirectorySeparatorChar,Path.AltDirectorySeparatorChar);list.Add(root);
  Func<string,IEnumerable<string>> dirs=d=>{try{return Directory.GetDirectories(d).Take(200).ToArray();}catch{return new string[0];}};
  Func<string,bool> has=d=>{try{return Directory.Exists(Path.Combine(Path.Combine(d,"support"),"payload"))||Directory.Exists(Path.Combine(d,"data"));}catch{return false;}};
  var cand=new List<string>();
  foreach(string c in dirs(root))cand.Add(c);
  string parent=null;try{parent=Path.GetDirectoryName(root);}catch{}
  if(!String.IsNullOrEmpty(parent))foreach(string d in dirs(parent)){if(String.Equals(d.TrimEnd(Path.DirectorySeparatorChar),root,StringComparison.OrdinalIgnoreCase))continue;cand.Add(d);foreach(string c in dirs(d).Take(20))cand.Add(c);}
  foreach(string c in cand)if(has(c)&&!list.Any(x=>String.Equals(x,c,StringComparison.OrdinalIgnoreCase)))list.Add(c);
  return list;
 }
 public static string MissingText(IList<string> rels){
  var b=new StringBuilder();b.Append("설치 자료 파일이 없습니다: "+rels[0]);if(rels.Count>1)b.Append(" 외 "+(rels.Count-1)+"개");
  b.Append("\r\n받은 ZIP(1of2, 2of2 …)을 모두 같은 새 폴더에 풀어 주세요. 2번째 ZIP 을 받지 않았거나 백신 프로그램이 파일을 지웠을 수도 있습니다. 게임 파일은 바꾸지 않았습니다.");
  return b.ToString();
 }
 public static readonly int[] Order=Enumerable.Range(1,Count).ToArray();
 public static int IdAt(int row){return Order[row];}
 public static int RowOf(int id){return Array.IndexOf(Order,id);}
 public const int Fog=12;
 // R41: 12번(필드 안개 1.6·미니맵)은 통합패치에서 삭제. 번호는 그대로(빈 번호). 옛 기록의 12번은 읽을 때 꺼짐으로 바꾸고, [선택 설치] 때 그 파일을 지움.
 public static bool Retired(int id){return id==Fog;}
 public static int Shown{get{return Count-1;}}
 // R24 의 12.5번(내부 26, 안개 날씨 제거판)은 R25 에서 12번으로 합침: 날씨는 게임 안 설정 창에서 켜고 끔
 public const int LegacyFogNoWeather=26;
 // 설치 자료 폴더(설치.cmd 가 있는 폴더) 찾기: EXE 위치(support\installer101)에서 위로 올라가며 support\manifest101.xml 이 있는 곳
 public static string FindPackageRoot(string start){
  string d=Path.GetFullPath(String.IsNullOrEmpty(start)?".":start);
  for(int i=0;i<6&&!String.IsNullOrEmpty(d);i++){if(File.Exists(Path.Combine(Path.Combine(d,"support"),"manifest101.xml")))return d;d=Path.GetDirectoryName(d.TrimEnd(Path.DirectorySeparatorChar,Path.AltDirectorySeparatorChar));}
  throw new IOException("설치 자료 목록(support\\manifest101.xml)을 찾지 못했습니다. 두 ZIP을 모두 같은 새 폴더에 풀었는지 확인해 주세요.\r\n찾기 시작한 위치: "+start);
 }
}

public sealed class ManifestFile {
 public string Target,Source,Sha,When; public int Id;
 public bool Applies(bool[] on){
  // When: "base" (런타임 필요 시), "N" (번호 N 켜짐), "N&M", "N&!M", "always"
  if(When=="always")return true;
  if(When=="base")return Spec.AnyRuntime(on);
  foreach(string term in When.Split(new[]{'|'},StringSplitOptions.RemoveEmptyEntries)){
   bool all=true;
   foreach(string f in term.Split(new[]{'&'},StringSplitOptions.RemoveEmptyEntries)){
    bool neg=f.StartsWith("!");string name=neg?f.Substring(1):f;bool v=name=="base"?Spec.AnyRuntime(on):on[Int32.Parse(name)-1];if(neg)v=!v;all&=v;
   }
   if(all)return true;
  }
  return false;
 }
}
public sealed class Manifest {
 public readonly List<ManifestFile> Entries=new List<ManifestFile>();
 public readonly Dictionary<string,string> Known=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase); // 과거 판 파일 해시 → 설명 (덮어써도 되는 '우리 파일')
 public readonly Dictionary<string,string> Values=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
 public static Manifest Load(string path,string expectedSha){
  if(Files.HashFile(path)!=expectedSha)throw new IOException("설치 자료 목록(manifest101.xml)이 손상되었습니다. 압축을 새 폴더에 다시 풀어 주세요.");
  var d=Files.Xml(path);var m=new Manifest();
  foreach(var e in d.Root.Elements("File"))m.Entries.Add(new ManifestFile{Target=Files.Relative(Files.Attr(e,"target")),Source=Files.Relative(Files.Attr(e,"source")),Sha=Files.Attr(e,"sha256"),When=Files.Attr(e,"when"),Id=Int32.Parse(Files.Attr(e,"id"))});
  foreach(var e in d.Root.Elements("Known"))m.Known[Files.Attr(e,"sha256")]=Files.Attr(e,"note");
  foreach(var e in d.Root.Elements("Value"))m.Values[Files.Attr(e,"name")]=Files.Attr(e,"value");
  return m;
 }
}
}
