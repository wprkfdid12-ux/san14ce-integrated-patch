// R26: 도시 증축 이벤트 생략 (1번 설정 창 1쪽 '증축 이벤트 생략' = 통합편의 증축 스위치)
//  - 게임 주소는 S14_GAME_STAMP 판 기준(R43: 2026-11-05 새 판으로 옮김; 연출 명령 vtable 38개는 시험9 에 RTTI 로 다시 찾음).
//  - 증축 이벤트(일반 0x3f~0xc8, 이민족 0x258~0x266)를 재생하는 동안 화면 연출 명령(SAct* : 그림·대사·나레이션·
//    입력 대기·카메라·소리·암전)을 실행하지 않고 '끝남(1)'으로 돌려준다. 규모 변경(SExecChangeCityLevel) 같은
//    게임 데이터 명령은 그대로 실행된다.
//  - 나레이션(SActShowNarration) 문장은 화면에 띄우는 대신 게임 기록(이력)에 한 줄로 넣는다(중요 표시).
//  - 스위치(통합편의 .data +0x105004)가 꺼져 있으면 원래대로 모두 실행.
#include "patch27.h"

static U8 *G27;      // 게임 이미지
static U8 *IC27;     // 통합편의 모듈
static U32 skipped_events, logged_lines, skipped_cmds;

// 연출 명령 vtable (RTTI .?AUSAct*@S14Event@@ 에서 뽑음; 메시지 인자 설정·세력 변경 계열은 제외)
static const U32 kSkipVt[] = {   // 시험9: 새 판 RTTI 이름으로 다시 찾음(upd_tools/rtti_r43.py). 이전엔 1.0.1 값 그대로라 증축 연출 생략이 안 걸렸음
  0x13bf5f8, 0x13bf718, 0x13bf988, 0x13bfa30, 0x13bfb68, 0x13bfc70, 0x13bfeb0, 0x13bfee0, 0x13bff28, 0x13c01f8,
  0x13c0210, 0x13c0360, 0x13c03d8, 0x13c04e0, 0x13c0660, 0x13c0738, 0x13c0798, 0x13c08e8, 0x13c0900, 0x13c0918,
  0x13c0ab0, 0x13c0be8, 0x13c0e10, 0x13c0ea0, 0x13c1080, 0x13c11d0, 0x13c1218, 0x13c1320, 0x13c1440, 0x13c1458,
  0x13c15c0, 0x13c16f8, 0x13c17e8, 0x13c1938, 0x13c1a70, 0x13c1ab8, 0x13c1c20, 0x13bd688,
};
static const U32 kNarrationVt = 0x13c0ea0;   // SActShowNarration (새 판)

static bool switch_on() {
  if (!IC27) IC27 = (U8 *)GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
  return IC27 && IC27[0x105004] != 0;
}
// 지금 재생 중인 이벤트가 증축 이벤트인가 (통합편의 증축 판정과 같은 조건)
static bool expansion_event() {
  U8 *ev = *(U8 **)(G27 + 0x27d6c88);
  if (!ev || *(U8 **)ev != G27 + 0x1467118) return false;          // CEventPlaybackState
  if (*(int *)(ev + 0x484) != 0 || *(int *)(ev + 0x488) != 0) return false;
  int id = *(int *)(ev + 0x4b0);
  return (id >= 0x3f && id <= 0xc8) || (id >= 0x258 && id <= 0x266);
}
static bool skip_vt(U32 r) {
  for (U32 v : kSkipVt) if (v == r) return true;
  return false;
}
static int last_event_seen = -1;

static void log_narration(U8 *desc) {
  U8 *ins = *(U8 **)desc;
  if (!ins) return;
  int id = *(int *)(ins + 0x10);                                        // 나레이션 문장 번호(+0xc 는 표시 방식)
  void *msg = G27 + 0x1a26440;                                          // 게임 메시지 표
  using GetText = const wchar_t *(*)(void *, int);
  const wchar_t *tmpl = ((GetText)((*(void ***)msg)[1]))(msg, id);
  if (!tmpl || !*tmpl) return;
  static wchar_t buf[0x1000];
  memset(buf, 0, sizeof(buf));
  using Fmt = void *(*)();
  using Expand = void (*)(void *, wchar_t *, U32, const wchar_t *);
  void *f = ((Fmt)(G27 + 0x1970e0))();
  ((Expand)(G27 + 0x1d3110))(f, buf, 0x1000, tmpl);                    // 메시지 인자(도시 이름·규모) 채우기
  if (!buf[0]) return;
  U8 *root = *(U8 **)(G27 + 0x2780f60);
  if (!root) return;
  U8 *sc = *(U8 **)(root + 0x871e8);
  int date = sc ? *(int *)(sc + 0x450) : 0;
  using Mgr = void *(*)();
  using AddHistory = U8 *(*)(void *, int, int, int, int, int, const wchar_t *, const wchar_t *, int, int, int, int, int);
  void *mgr = ((Mgr)(G27 + 0x16ff70))();
  const wchar_t *empty = *(const wchar_t **)(G27 + 0x1a4ab88);
  // 종류 1(이벤트 기록과 같은 칸), 중요 표시 1, 화자 없음(0x1394)
  ((AddHistory)(G27 + 0x936980))(mgr, 1, 0, 0, 1, date, buf, empty ? empty : L"", 0x1394, 0, 0, 1, 0);
  logged_lines++;
}

extern "C" int CityEventCmd(void **cmd, U8 *desc) {
  using Exec = int (*)(void *, U8 *);
  void **vt = (void **)*cmd;
  if (switch_on() && expansion_event()) {
    U32 r = (U32)((U8 *)vt - G27);
    if (skip_vt(r)) {
      int id = *(int *)(*(U8 **)(G27 + 0x27d6c88) + 0x4b0);
      if (id != last_event_seen) { last_event_seen = id; skipped_events++; }
      skipped_cmds++;
      if (r == kNarrationVt) log_narration(desc);
      return 1;
    }
  }
  return ((Exec)vt[0])(cmd, desc);
}

// 명령 실행 자리(0x3d8101: mov rax,[rcx] / mov rdx,r14 / call [rax])를 call 로 바꿔 여기로: rcx=명령, r14=실행 정보
asm(".text\n.globl CityEventBridge\nCityEventBridge:\n mov %r14,%rdx\n jmp CityEventCmd\n");
extern "C" void CityEventBridge();

extern "C" __declspec(dllexport) U32 CityEventStats(U32 *events, U32 *lines) {
  if (events) *events = skipped_events;
  if (lines) *lines = logged_lines;
  return skipped_cmds;
}

extern "C" bool CityEventSkipInstall(const S14_CoreAPI *a) {
  G27 = a->resolved->image;
  HookPlan p(a);
  const U8 e[] = {0x48, 0x8b, 0x01, 0x49, 0x8b, 0xd6, 0xff, 0x10};
  p.jump(0x3d8101, e, sizeof(e), (void *)CityEventBridge, 0xe8);
  bool ok = p.install("SAN14CE.Patch27.CityEventSkip");
  if (a->Log) a->Log(ok ? "Patch27 R26: city expansion event skip installed (presentation commands skipped while the convenience 'city' switch is on; result line goes to history)."
                        : "Patch27 R26: city expansion event skip not installed (site mismatch); original event kept.");
  return ok;
}
