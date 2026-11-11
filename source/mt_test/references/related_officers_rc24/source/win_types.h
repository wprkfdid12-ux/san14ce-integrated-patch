/* Shared freestanding Windows x64 import declarations for SortiePresets / ModCore. */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef __SIZE_TYPE__ SIZE;
typedef void *HANDLE;
typedef int BOOL;
typedef struct { void *state; } ONCE;
typedef BOOL (*ONCE_FN)(ONCE *, void *, void **);
struct WIN_API {
    U32 (*GetSystemDirectoryW)(U16 *, U32);
    HANDLE (*LoadLibraryExW)(const U16 *, HANDLE, U32);
    void *(*GetProcAddress)(HANDLE, const char *);
    BOOL (*InitOnceExecuteOnce)(ONCE *, ONCE_FN, void *, void **);
    HANDLE (*GetModuleHandleW)(const U16 *);
    U32 (*GetModuleFileNameW)(HANDLE, U16 *, U32);
    BOOL (*VirtualProtect)(void *, SIZE, U32, U32 *);
    BOOL (*FlushInstructionCache)(HANDLE, const void *, SIZE);
    HANDLE (*GetCurrentProcess)(void);
    HANDLE (*CreateFileW)(const U16 *, U32, U32, void *, U32, U32, HANDLE);
    BOOL (*WriteFile)(HANDLE, const void *, U32, U32 *, void *);
    BOOL (*CloseHandle)(HANDLE);
    U32 (*GetLastError)(void);
    void (*SetLastError)(U32);
    void *terminator;
};
struct WIN_API win = {0};

typedef int I32;
