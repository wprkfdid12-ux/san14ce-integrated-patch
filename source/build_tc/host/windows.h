/* Host (Linux) stand-in for windows.h used only by the logic tests. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WINAPI
typedef int BOOL; typedef unsigned char BYTE; typedef uint16_t WORD; typedef uint32_t DWORD; typedef int32_t LONG;
typedef int64_t LONG64, LONGLONG; typedef uint64_t ULONGLONG; typedef uintptr_t ULONG_PTR, SIZE_T, DWORD_PTR;
typedef void *HANDLE,*LPVOID,*PVOID; typedef const void *LPCVOID; typedef struct HINSTANCE__ *HINSTANCE; typedef HINSTANCE HMODULE;
typedef struct HWND__ *HWND; typedef wchar_t WCHAR; typedef const wchar_t *LPCWSTR; typedef wchar_t *LPWSTR; typedef const char *LPCSTR; typedef DWORD *LPDWORD; typedef int32_t NTSTATUS;
typedef union _LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; } u; LONGLONG QuadPart; } LARGE_INTEGER,*PLARGE_INTEGER;
typedef struct _RTL_SRWLOCK { PVOID Ptr; } SRWLOCK,*PSRWLOCK;
#define SRWLOCK_INIT {0}
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);
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
#define MB_OK 0
#define MB_ICONERROR 0x10
HMODULE GetModuleHandleW(LPCWSTR); void *GetProcAddress(HMODULE,LPCSTR); DWORD GetModuleFileNameW(HMODULE,LPWSTR,DWORD);
HANDLE CreateFileW(LPCWSTR,DWORD,DWORD,void*,DWORD,DWORD,HANDLE); BOOL ReadFile(HANDLE,LPVOID,DWORD,LPDWORD,void*); BOOL WriteFile(HANDLE,LPCVOID,DWORD,LPDWORD,void*);
BOOL CloseHandle(HANDLE); BOOL GetFileSizeEx(HANDLE,PLARGE_INTEGER); BOOL SetFilePointerEx(HANDLE,LARGE_INTEGER,PLARGE_INTEGER,DWORD); BOOL FlushFileBuffers(HANDLE);
HMODULE LoadLibraryExW(LPCWSTR,HANDLE,DWORD);
#define LOAD_WITH_ALTERED_SEARCH_PATH 8
BOOL CreateDirectoryW(LPCWSTR,void*); DWORD GetFileAttributesW(LPCWSTR); unsigned GetPrivateProfileIntW(LPCWSTR,LPCWSTR,int,LPCWSTR);
BOOL WritePrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR); LPVOID VirtualAlloc(LPVOID,SIZE_T,DWORD,DWORD); BOOL VirtualFree(LPVOID,SIZE_T,DWORD);
BOOL VirtualProtect(LPVOID,SIZE_T,DWORD,DWORD*); BOOL FlushInstructionCache(HANDLE,LPCVOID,SIZE_T); HANDLE GetCurrentProcess(void);
DWORD TlsAlloc(void); BOOL TlsFree(DWORD); LPVOID TlsGetValue(DWORD); BOOL TlsSetValue(DWORD,LPVOID); ULONGLONG GetTickCount64(void);
void AcquireSRWLockExclusive(PSRWLOCK); void ReleaseSRWLockExclusive(PSRWLOCK); void AcquireSRWLockShared(PSRWLOCK); void ReleaseSRWLockShared(PSRWLOCK);
HANDLE CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD); void Sleep(DWORD); DWORD GetLastError(void); void SetLastError(DWORD);
int MessageBoxW(HWND,LPCWSTR,LPCWSTR,unsigned);

typedef struct _FILETIME { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
typedef struct _WIN32_FIND_DATAW { DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime; DWORD nFileSizeHigh, nFileSizeLow, dwReserved0, dwReserved1; WCHAR cFileName[260]; WCHAR cAlternateFileName[14]; DWORD dwFileType, dwCreatorType; WORD wFinderFlags; } WIN32_FIND_DATAW;
typedef struct _SYSTEMTIME { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME;
#define FILE_SHARE_WRITE 2
#define FILE_SHARE_DELETE 4
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_APPEND_DATA 4
#define CP_UTF8 65001
#define MOVEFILE_REPLACE_EXISTING 1
HANDLE FindFirstFileW(LPCWSTR, WIN32_FIND_DATAW *); BOOL FindNextFileW(HANDLE, WIN32_FIND_DATAW *); BOOL FindClose(HANDLE);
int WideCharToMultiByte(unsigned, DWORD, LPCWSTR, int, char *, int, LPCSTR, BOOL *); int MultiByteToWideChar(unsigned, DWORD, LPCSTR, int, LPWSTR, int);
DWORD GetEnvironmentVariableW(LPCWSTR, LPWSTR, DWORD); BOOL DeleteFileW(LPCWSTR); DWORD GetCurrentProcessId(void); DWORD GetCurrentThreadId(void); void GetLocalTime(SYSTEMTIME *); BOOL MoveFileExW(LPCWSTR, LPCWSTR, DWORD);
DWORD GetPrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);

static inline LONG InterlockedExchange(volatile LONG *p, LONG v) { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }
static inline LONG InterlockedIncrement(volatile LONG *p) { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
BOOL QueryPerformanceCounter(LARGE_INTEGER*); BOOL QueryPerformanceFrequency(LARGE_INTEGER*);
static inline LONG64 InterlockedExchange64(volatile LONG64 *p, LONG64 v) { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }
static inline LONG64 InterlockedCompareExchange64(volatile LONG64 *p, LONG64 v, LONG64 cmp) { __atomic_compare_exchange_n(p, &cmp, v, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return cmp; }

#ifdef __cplusplus
}
#endif
