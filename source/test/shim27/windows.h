#pragma once
// 25번 시작 시험용: 공통 흉내(shim) + 스레드·TLS·시간
#include "../shim/windows.h"
typedef unsigned long long ULONGLONG;typedef size_t SIZE_T;typedef DWORD*LPDWORD;typedef DWORD(*LPTHREAD_START_ROUTINE)(LPVOID);
#define TLS_OUT_OF_INDEXES 0xFFFFFFFFu
extern "C" ULONGLONG GetTickCount64(void);extern "C" void Sleep(DWORD);
extern "C" HANDLE CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
extern "C" DWORD TlsAlloc(void);extern "C" BOOL TlsFree(DWORD);extern "C" LPVOID TlsGetValue(DWORD);extern "C" BOOL TlsSetValue(DWORD,LPVOID);
