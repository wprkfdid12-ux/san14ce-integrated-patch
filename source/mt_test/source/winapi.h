#ifndef MERIT_WINAPI_H
#define MERIT_WINAPI_H
#include "merit_core.h"
typedef void* HANDLE;
typedef int BOOL;
typedef struct {void*p;} SRWLOCK;
/* Win64 MEMORY_BASIC_INFORMATION, including padding/alignment. */
typedef struct {void*base;void*allocation;u32 allocation_protect;u32 pad1;usize region_size;u32 state;u32 protect;u32 type;u32 pad2;} MemoryRegion;
#define WINAPI __declspec(dllimport)
WINAPI HANDLE GetModuleHandleW(const u16*);
WINAPI u32 GetModuleFileNameW(HANDLE,u16*,u32);
WINAPI void* GetProcAddress(HANDLE,const char*);
WINAPI HANDLE LoadLibraryExW(const u16*,HANDLE,u32);
WINAPI HANDLE GetCurrentProcess(void);
WINAPI u32 GetCurrentThreadId(void);
WINAPI u64 GetTickCount64(void);
WINAPI BOOL QueryPerformanceCounter(long long*);
WINAPI BOOL QueryPerformanceFrequency(long long*);
WINAPI u32 GetLastError(void);
WINAPI void SetLastError(u32);
WINAPI BOOL VirtualProtect(void*,usize,u32,u32*);
WINAPI BOOL FlushInstructionCache(HANDLE,const void*,usize);
WINAPI void* VirtualAlloc(void*,usize,u32,u32);
WINAPI BOOL VirtualFree(void*,usize,u32);
WINAPI usize VirtualQuery(const void*,MemoryRegion*,usize);
WINAPI short GetAsyncKeyState(int);
WINAPI HANDLE GetForegroundWindow(void);
WINAPI u32 GetWindowThreadProcessId(HANDLE,u32*);
WINAPI u32 GetCurrentProcessId(void);
WINAPI HANDLE CreateFileW(const u16*,u32,u32,void*,u32,u32,HANDLE);
WINAPI BOOL WriteFile(HANDLE,const void*,u32,u32*,void*);
WINAPI BOOL ReadFile(HANDLE,void*,u32,u32*,void*);
WINAPI BOOL CloseHandle(HANDLE);
WINAPI BOOL FlushFileBuffers(HANDLE);
WINAPI BOOL MoveFileExW(const u16*,const u16*,u32);
WINAPI BOOL DeleteFileW(const u16*);
WINAPI BOOL CreateDirectoryW(const u16*,void*);
WINAPI u32 GetEnvironmentVariableW(const u16*,u16*,u32);
WINAPI BOOL GetFileSizeEx(HANDLE,long long*);
WINAPI u32 GetFileAttributesW(const u16*);
WINAPI void AcquireSRWLockExclusive(SRWLOCK*);
WINAPI void ReleaseSRWLockExclusive(SRWLOCK*);
WINAPI int MessageBoxW(HANDLE,const u16*,const u16*,u32);
WINAPI long BCryptGenRandom(HANDLE,u8*,u32,u32);
WINAPI HANDLE CreateEventW(void*,int,int,const u16*);
WINAPI HANDLE CreateThread(void*,usize,u32(*)(void*),void*,u32,u32*);
WINAPI u32 WaitForSingleObject(HANDLE,u32);
WINAPI int SetEvent(HANDLE);
WINAPI void GetSystemTimeAsFileTime(u64*);
typedef struct {u32 attributes;u32 creation[2],access[2],write[2];u32 size_high,size_low,reserved0,reserved1;u16 name[260],alternate[14];} WinFindDataW;
WINAPI HANDLE FindFirstFileW(const u16*,WinFindDataW*);
WINAPI BOOL FindNextFileW(HANDLE,WinFindDataW*);
WINAPI BOOL FindClose(HANDLE);
#define INVALID_HANDLE ((HANDLE)(iptr)-1)
#endif
