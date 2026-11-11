/* Minimal Win64 declarations for a CRT-free clang build (only what these mods use). */
#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WINAPI
#define WINBASEAPI __declspec(dllimport)
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef long long LONG64;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef uintptr_t ULONG_PTR, SIZE_T, DWORD_PTR;
typedef void *HANDLE, *LPVOID, *PVOID;
typedef const void *LPCVOID;
typedef struct HINSTANCE__ *HINSTANCE;
typedef HINSTANCE HMODULE;
typedef struct HWND__ *HWND;
typedef wchar_t WCHAR;
typedef const wchar_t *LPCWSTR;
typedef wchar_t *LPWSTR;
typedef const char *LPCSTR;
typedef DWORD *LPDWORD;
typedef long NTSTATUS;
typedef union _LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; } u; LONGLONG QuadPart; } LARGE_INTEGER, *PLARGE_INTEGER;
typedef struct _RTL_SRWLOCK { PVOID Ptr; } SRWLOCK, *PSRWLOCK;
#define SRWLOCK_INIT {0}
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
typedef void *FARPROC_;
#define TRUE 1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define PAGE_READWRITE 0x04
#define PAGE_EXECUTE_READWRITE 0x40
#define GENERIC_READ 0x80000000UL
#define GENERIC_WRITE 0x40000000UL
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_BEGIN 0
#define FILE_END 2
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define MB_OK 0
#define MB_ICONERROR 0x10
#define DLL_PROCESS_ATTACH 1
WINBASEAPI HMODULE WINAPI GetModuleHandleW(LPCWSTR);
WINBASEAPI void *WINAPI GetProcAddress(HMODULE, LPCSTR);
WINBASEAPI DWORD WINAPI GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
WINBASEAPI HANDLE WINAPI CreateFileW(LPCWSTR, DWORD, DWORD, void *, DWORD, DWORD, HANDLE);
WINBASEAPI BOOL WINAPI ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, void *);
WINBASEAPI BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, void *);
WINBASEAPI BOOL WINAPI CloseHandle(HANDLE);
WINBASEAPI BOOL WINAPI GetFileSizeEx(HANDLE, PLARGE_INTEGER);
WINBASEAPI BOOL WINAPI SetFilePointerEx(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD);
WINBASEAPI BOOL WINAPI SetEndOfFile(HANDLE);
WINBASEAPI BOOL WINAPI FlushFileBuffers(HANDLE);
WINBASEAPI BOOL WINAPI CreateDirectoryW(LPCWSTR, void *);
WINBASEAPI DWORD WINAPI GetFileAttributesW(LPCWSTR);
WINBASEAPI BOOL WINAPI CopyFileW(LPCWSTR, LPCWSTR, BOOL);
WINBASEAPI unsigned WINAPI GetPrivateProfileIntW(LPCWSTR, LPCWSTR, int, LPCWSTR);
WINBASEAPI DWORD WINAPI GetPrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
WINBASEAPI BOOL WINAPI WritePrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
WINBASEAPI LPVOID WINAPI VirtualAlloc(LPVOID, SIZE_T, DWORD, DWORD);
WINBASEAPI BOOL WINAPI VirtualFree(LPVOID, SIZE_T, DWORD);
WINBASEAPI BOOL WINAPI VirtualProtect(LPVOID, SIZE_T, DWORD, DWORD *);
WINBASEAPI BOOL WINAPI FlushInstructionCache(HANDLE, LPCVOID, SIZE_T);
WINBASEAPI HANDLE WINAPI GetCurrentProcess(void);
WINBASEAPI DWORD WINAPI TlsAlloc(void);
WINBASEAPI BOOL WINAPI TlsFree(DWORD);
WINBASEAPI LPVOID WINAPI TlsGetValue(DWORD);
WINBASEAPI BOOL WINAPI TlsSetValue(DWORD, LPVOID);
WINBASEAPI ULONGLONG WINAPI GetTickCount64(void);
WINBASEAPI BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER*);
WINBASEAPI BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER*);
WINBASEAPI void WINAPI AcquireSRWLockExclusive(PSRWLOCK);
WINBASEAPI void WINAPI ReleaseSRWLockExclusive(PSRWLOCK);
WINBASEAPI void WINAPI AcquireSRWLockShared(PSRWLOCK);
WINBASEAPI void WINAPI ReleaseSRWLockShared(PSRWLOCK);
WINBASEAPI HANDLE WINAPI CreateThread(void *, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
WINBASEAPI void WINAPI Sleep(DWORD);
WINBASEAPI DWORD WINAPI GetLastError(void);
WINBASEAPI void WINAPI SetLastError(DWORD);
WINBASEAPI BOOL WINAPI DisableThreadLibraryCalls(HMODULE);
typedef struct _FILETIME { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
typedef struct _WIN32_FIND_DATAW { DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime; DWORD nFileSizeHigh, nFileSizeLow, dwReserved0, dwReserved1; WCHAR cFileName[260]; WCHAR cAlternateFileName[14]; DWORD dwFileType, dwCreatorType; WORD wFinderFlags; } WIN32_FIND_DATAW;
typedef struct _SYSTEMTIME { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME;
#define FILE_SHARE_WRITE 2
#define FILE_SHARE_DELETE 4
#define CREATE_ALWAYS 2
#define FILE_APPEND_DATA 4
#define CP_UTF8 65001
#define MOVEFILE_REPLACE_EXISTING 1
WINBASEAPI HANDLE WINAPI FindFirstFileW(LPCWSTR, WIN32_FIND_DATAW *);
WINBASEAPI BOOL WINAPI FindNextFileW(HANDLE, WIN32_FIND_DATAW *);
WINBASEAPI BOOL WINAPI FindClose(HANDLE);
WINBASEAPI int WINAPI WideCharToMultiByte(unsigned, DWORD, LPCWSTR, int, char *, int, LPCSTR, BOOL *);
WINBASEAPI int WINAPI MultiByteToWideChar(unsigned, DWORD, LPCSTR, int, LPWSTR, int);
WINBASEAPI DWORD WINAPI GetEnvironmentVariableW(LPCWSTR, LPWSTR, DWORD);
WINBASEAPI void WINAPI GetSystemTimeAsFileTime(FILETIME *);
WINBASEAPI BOOL WINAPI DeleteFileW(LPCWSTR);
WINBASEAPI DWORD WINAPI GetCurrentProcessId(void);
WINBASEAPI DWORD WINAPI GetCurrentThreadId(void);
WINBASEAPI void WINAPI GetLocalTime(SYSTEMTIME *);
WINBASEAPI BOOL WINAPI MoveFileExW(LPCWSTR, LPCWSTR, DWORD);
WINBASEAPI HMODULE WINAPI LoadLibraryExW(LPCWSTR, HANDLE, DWORD);
#define LOAD_WITH_ALTERED_SEARCH_PATH 8
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
__declspec(dllimport) int WINAPI MessageBoxW(HWND, LPCWSTR, LPCWSTR, unsigned);
static inline LONG InterlockedExchange(volatile LONG *p, LONG v) { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }
static inline LONG InterlockedIncrement(volatile LONG *p) { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
static inline LONG64 InterlockedExchange64(volatile LONG64 *p, LONG64 v) { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }
static inline LONG64 InterlockedCompareExchange64(volatile LONG64 *p, LONG64 v, LONG64 cmp) { __atomic_compare_exchange_n(p, &cmp, v, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return cmp; }
#ifdef __cplusplus
}
#endif
