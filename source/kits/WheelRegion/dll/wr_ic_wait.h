/* 통합편의(SAN14_IntegratedConvenience.dll) 시작 확인이 끝날 때까지 기다리기 — wr_dll.c 의 core_patch 가 씀.
 * 시험(통합패치 test/test_wheel_ic_wait_native.c)에서는 아래 윈도 함수 이름을 가짜로 정의한 뒤 이 파일을 넣는다.
 * 쓰는 것: path_of(out,name) · GetFileAttributesW · GetPrivateProfileIntW · GetModuleHandleW · GetProcAddress · GetTickCount64 · Sleep */
static const char *wait_ic_restore(void) {
    wchar_t p[600];
    void *ic;
    unsigned long long t0;
    unsigned int (*wait)(void);
    path_of(p, L"SAN14_IntegratedConvenience.dll");
    if (GetFileAttributesW(p) == 0xfffffffful) return "ic-not-installed";
    path_of(p, L"SAN14Mods\\modules\\IntegratedConvenience_GreenlandCompat.ini");
    if (GetFileAttributesW(p) == 0xfffffffful) return "ic-not-registered";
    if (!GetPrivateProfileIntW(L"Mod", L"Enabled", 1, p)) return "ic-disabled";
    t0 = GetTickCount64();
    while (!(ic = GetModuleHandleW(L"SAN14_IntegratedConvenience.dll"))) {
        if (GetTickCount64() - t0 > 60000) return "ic-absent";
        Sleep(200);
    }
    wait = (unsigned int (*)(void))GetProcAddress(ic, "SAN14_WaitConvenienceV1");
    if (!wait) return "ic-no-wait-export";
    t0 = GetTickCount64();
    while (GetTickCount64() - t0 < 600000) {
        if (wait()) return "ic-ready";
        Sleep(500);
    }
    return "ic-timeout";
}
