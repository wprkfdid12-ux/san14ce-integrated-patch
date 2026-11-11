// 시험 전용 윈도 API 흉내(리눅스에서 모듈 소스를 그대로 컴파일해 돌리기 위한 최소 선언). 본판 빌드에는 쓰지 않음.
#pragma once
#define S14_TEST_SHIM 1
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef long LONG;typedef long long LONG64;typedef uint32_t DWORD;typedef int BOOL;typedef void*HANDLE;typedef void*HMODULE;typedef void*HINSTANCE;typedef void*LPVOID;typedef unsigned short WORD;
typedef struct{WORD wYear,wMonth,wDayOfWeek,wDay,wHour,wMinute,wSecond,wMilliseconds;}SYSTEMTIME;
typedef union{struct{DWORD LowPart;LONG HighPart;};long long QuadPart;}LARGE_INTEGER;
typedef void*BCRYPT_ALG_HANDLE;typedef void*BCRYPT_HASH_HANDLE;
#define WINAPI
#define __declspec(x)
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define GENERIC_READ 1
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_APPEND_DATA 4
#define TRUE 1
#define FALSE 0
#define BCRYPT_SHA256_ALGORITHM L"SHA256"
#define INT32_MIN_ (-2147483647-1)
static inline LONG InterlockedExchange(volatile LONG*p,LONG v){return __atomic_exchange_n(p,v,__ATOMIC_SEQ_CST);}
static inline LONG InterlockedIncrement(volatile LONG*p){return __atomic_add_fetch(p,1,__ATOMIC_SEQ_CST);}
// 아래는 시험기(test_domestic_ai_native.cpp)가 구현
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*,DWORD);
extern "C" BOOL CreateDirectoryW(const wchar_t*,void*);
extern "C" HANDLE CreateFileW(const wchar_t*,DWORD,DWORD,void*,DWORD,DWORD,void*);
extern "C" BOOL WriteFile(HANDLE,const void*,DWORD,DWORD*,void*);
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*);
extern "C" BOOL CloseHandle(HANDLE);
extern "C" void GetLocalTime(SYSTEMTIME*);
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int,const wchar_t*);
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned);
extern "C" HMODULE GetModuleHandleW(const wchar_t*);
extern "C" void*GetProcAddress(HMODULE,const char*);
extern "C" HMODULE LoadLibraryExW(const wchar_t*,HANDLE,DWORD);   // 정식판 뒤: 9번 확인 모듈이 화면 부품을 미리 켤 때
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned);
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned);
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned);
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned);
extern "C" LONG BCryptDestroyHash(void*);
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned);
// HiredFace 시험기가 구현(다른 시험기는 안 씀)
extern "C" BOOL QueryPerformanceCounter(LARGE_INTEGER*);
extern "C" BOOL QueryPerformanceFrequency(LARGE_INTEGER*);
extern "C" DWORD GetCurrentThreadId(void);
// 22차 SceneSkip 넓은 창 시험기가 구현
typedef unsigned long long ULONGLONG;
extern "C" ULONGLONG GetTickCount64(void);
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*)(void*),void*,DWORD,DWORD*);
// R43 공통 실패 보고가 씀
static inline DWORD GetLastError(void){return 0;}

// R43 Diag 시험기가 구현(다른 시험기는 안 씀). 선언만.
typedef struct{DWORD dwLowDateTime,dwHighDateTime;}FILETIME;
typedef struct{DWORD dwFileAttributes;FILETIME ftCreationTime,ftLastAccessTime,ftLastWriteTime;DWORD nFileSizeHigh,nFileSizeLow,dwReserved0,dwReserved1;wchar_t cFileName[260];wchar_t cAlternateFileName[14];}WIN32_FIND_DATAW;
#define FILE_SHARE_DELETE 4
#define CP_UTF8 65001
extern "C" HANDLE FindFirstFileW(const wchar_t*,WIN32_FIND_DATAW*);
extern "C" BOOL FindNextFileW(HANDLE,WIN32_FIND_DATAW*);
extern "C" BOOL FindClose(HANDLE);
extern "C" DWORD GetPrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,wchar_t*,DWORD,const wchar_t*);
extern "C" DWORD GetEnvironmentVariableW(const wchar_t*,wchar_t*,DWORD);
extern "C" BOOL GetFileSizeEx(HANDLE,LARGE_INTEGER*);
extern "C" BOOL SetFilePointerEx(HANDLE,LARGE_INTEGER,LARGE_INTEGER*,DWORD);
extern "C" void GetSystemTimeAsFileTime(FILETIME*);
extern "C" DWORD GetFileAttributesW(const wchar_t*);
