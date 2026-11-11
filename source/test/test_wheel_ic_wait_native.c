// P202(2026-11-09): 30번 휠클릭이 고치는 자리(exe+0x232F60)가 통합편의 시작 확인 구간(전역 확인 115번) 안 → 통합편의 확인이 끝난 뒤에 고치게 함.
//  kits/WheelRegion/dll/wr_ic_wait.h 의 실제 코드를 가짜 윈도 함수로 돌려 본다.
//  빌드/실행: clang -O1 -w -I kits/WheelRegion/dll test/test_wheel_ic_wait_native.c -o /tmp/t && /tmp/t
#include <stdio.h>
#include <string.h>
#include <wchar.h>
static int ic_file, ic_ini, ic_enabled, has_export, ready_after, wcalls, sleeps;
static long long load_at;
static unsigned long long now_ms;
static void path_of(wchar_t *out, const wchar_t *name) { wcscpy(out, L"C:\\G\\"); wcscat(out, name); }
static unsigned long GetFileAttributesW(const wchar_t *p) {
    if (wcsstr(p, L"SAN14_IntegratedConvenience.dll")) return ic_file ? 0x20 : 0xfffffffful;
    if (wcsstr(p, L"IntegratedConvenience_GreenlandCompat.ini")) return ic_ini ? 0x20 : 0xfffffffful;
    return 0xfffffffful;
}
static unsigned int GetPrivateProfileIntW(const wchar_t *a, const wchar_t *b, int d, const wchar_t *f) { return ic_enabled; }
static char icmod[8];
static void *GetModuleHandleW(const wchar_t *n) { return (load_at >= 0 && (long long)now_ms >= load_at) ? icmod : 0; }
static unsigned int fake_wait(void) { int k = wcalls++; if (ready_after >= 0 && k >= ready_after) return 1; now_ms += 15000; return 0; }   // 통합편의: 최대 15초 기다리다 0
static void *GetProcAddress(void *m, const char *n) { return has_export && !strcmp(n, "SAN14_WaitConvenienceV1") ? (void *)fake_wait : 0; }
static unsigned long long GetTickCount64(void) { return now_ms; }
static void Sleep(unsigned long ms) { now_ms += ms; sleeps++; }
#include "wr_ic_wait.h"
static int fails;
static void check(int c, const char *m) { printf("%s %s\n", c ? "  ok  " : "  FAIL", m); if (!c) fails++; }
static void reset(void) { ic_file = ic_ini = ic_enabled = has_export = 1; ready_after = 0; wcalls = sleeps = 0; load_at = 0; now_ms = 1000; }
int main(void) {
    const char *r;
    reset(); ic_file = 0; r = wait_ic_restore(); check(!strcmp(r, "ic-not-installed") && !sleeps && !wcalls, "통합편의 파일 없음 → 바로 고침(기대지 않음)");
    reset(); ic_ini = 0; r = wait_ic_restore(); check(!strcmp(r, "ic-not-registered") && !sleeps, "통합편의 등록 파일 없음 → 바로");
    reset(); ic_enabled = 0; r = wait_ic_restore(); check(!strcmp(r, "ic-disabled") && !sleeps, "통합편의 꺼 둠 → 바로");
    reset(); r = wait_ic_restore(); check(!strcmp(r, "ic-ready") && wcalls == 1, "통합편의 확인이 이미 끝남 → 한 번 물어보고 바로");
    reset(); load_at = 6000; ready_after = 2; r = wait_ic_restore();
    check(!strcmp(r, "ic-ready") && wcalls == 3 && now_ms >= 6000 + 30000, "제보 상황: 통합편의가 늦게 올라오고 확인도 늦음 → 끝날 때까지 기다린 뒤 고침");
    reset(); load_at = -1; r = wait_ic_restore(); check(!strcmp(r, "ic-absent") && now_ms > 61000 && now_ms < 63000 && !wcalls, "60초 안에 안 올라옴 → 그냥 고침");
    reset(); has_export = 0; r = wait_ic_restore(); check(!strcmp(r, "ic-no-wait-export"), "기다리기 함수가 없는 옛 통합편의 → 그냥 고침");
    reset(); ready_after = -1; r = wait_ic_restore(); check(!strcmp(r, "ic-timeout") && now_ms >= 601000 && now_ms < 640000, "10분이 지나도 안 끝남 → 그냥 고침(영영 안 켜지지는 않음)");
    reset(); ready_after = 30; r = wait_ic_restore(); check(!strcmp(r, "ic-ready") && wcalls == 31, "느린 PC(확인에 7분 넘게) → 끝난 즉시 고침");
    printf(fails ? "실패 %d\n" : "전부 통과\n", fails);
    return fails ? 1 : 0;
}
