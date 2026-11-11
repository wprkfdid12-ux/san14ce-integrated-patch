#pragma once
/* Host-only Win32 shim. Never on the Windows DLL build's include path. */
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <mutex>
#include <chrono>
#define __declspec(x)
#define WINAPI
#define TRUE 1
#define GENERIC_WRITE 1
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define CREATE_ALWAYS 1
#define FILE_BEGIN 0
#define FILE_ATTRIBUTE_NORMAL 0
typedef void *HANDLE;
typedef void *HINSTANCE;
typedef void *LPVOID;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int BOOL;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
struct SRWLOCK { std::mutex mutex; inline static thread_local int depth=0; };
#define SRWLOCK_INIT {}
static thread_local DWORD fl_test_thread=77;
static uint64_t fl_test_now_ms=1000;
static uint64_t GetTickCount64() { return fl_test_now_ms; }
struct LARGE_INTEGER { int64_t QuadPart; };
static BOOL SetFilePointerEx(HANDLE,LARGE_INTEGER,LARGE_INTEGER *,DWORD){return TRUE;}
static BOOL SetEndOfFile(HANDLE){return TRUE;}
static BOOL QueryPerformanceFrequency(LARGE_INTEGER *v) { v->QuadPart=1000000000;return TRUE; }
static uint64_t fl_test_qpc_override;
static BOOL QueryPerformanceCounter(LARGE_INTEGER *v) {
    if(fl_test_qpc_override){v->QuadPart=fl_test_qpc_override;return TRUE;}
    v->QuadPart=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();return TRUE;
}
static DWORD GetCurrentThreadId() { return fl_test_thread; }
static void AcquireSRWLockExclusive(SRWLOCK *s) { assert(s->depth==0); s->mutex.lock(); ++s->depth; }
static void ReleaseSRWLockExclusive(SRWLOCK *s) { assert(s->depth==1); --s->depth; s->mutex.unlock(); }
static BOOL WriteFile(HANDLE,const void *,DWORD n,DWORD *done,void *) { *done=n; return TRUE; }
static BOOL FlushFileBuffers(HANDLE) { return TRUE; }
static LONG InterlockedIncrement(volatile LONG *p) { return __atomic_add_fetch(p,1,__ATOMIC_SEQ_CST); }
static LONG InterlockedExchange(volatile LONG *p,LONG v) { return __atomic_exchange_n(p,v,__ATOMIC_SEQ_CST); }
static void *GetModuleHandleW(const wchar_t *) { return NULL; }
static HANDLE CreateFileW(const wchar_t *,DWORD,DWORD,void *,DWORD,DWORD,void *) { return INVALID_HANDLE_VALUE; }
