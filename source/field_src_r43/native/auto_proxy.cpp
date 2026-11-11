#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "proxy_exports.h"
#include "install_files.h"
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static FARPROC addresses[FL_PROXY_EXPORT_COUNT];
static HMODULE self_module;
static BOOL CALLBACK initialize(PINIT_ONCE,void *,void **) {
    wchar_t system[32768]; UINT n=GetSystemDirectoryW(system,32768);
    if(!n || n+32>=32768) return FALSE;
    wcscat(system,L"\\");wcscat(system,FL_PROXY_SYSTEM_NAME);
    HMODULE module=LoadLibraryExW(system,NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module) return FALSE;
    for(unsigned i=0;i<FL_PROXY_EXPORT_COUNT;++i) addresses[i]=GetProcAddress(module,fl_proxy_export_keys[i]);
    return TRUE;
}
extern "C" FARPROC fl_proxy_address(unsigned index) {
    DWORD error=GetLastError();
    BOOL ok=InitOnceExecuteOnce(&once,initialize,NULL,NULL);
    FARPROC p=ok && index<FL_PROXY_EXPORT_COUNT?addresses[index]:NULL;
    SetLastError(error);
    if(!p) { RaiseException(0xc0000139,EXCEPTION_NONCONTINUABLE,0,NULL);TerminateProcess(GetCurrentProcess(),0xc0000139); }
    return p;
}
static DWORD WINAPI start_runtime(void *) {
    wchar_t host[32768],proxy[32768],path[32768],command[32768];
    if(!GetModuleFileNameW(NULL,host,32768) || !GetModuleFileNameW(self_module,proxy,32768)) return 0;
    wchar_t *h=wcsrchr(host,L'\\'),*p=wcsrchr(proxy,L'\\');
    if(!h || !p || _wcsicmp(h+1,L"SAN14CE_KO.exe")) return 0;
    *h=*p=0; if(_wcsicmp(host,proxy) || wcslen(host)>32000) return 0;
    // Locate only this slot's owned installation, including legacy R23 installs.
    // Foreign folders and files are neither loaded nor renamed.
    bool found=false;
    for(unsigned i=0;i<fl_install::directory_count;++i) {
        std::wstring directory=std::wstring(host)+L"\\"+fl_install::directory_names[i];
        fl_install::Manifest manifest={};
        if(!fl_install::read_manifest(directory,manifest)||manifest.slot!=FL_PROXY_SLOT)continue;
        std::wstring runtime=directory+L"\\FieldLeader3D_Runtime.exe";
        if(!fl_install::matches(runtime,manifest.runtime))continue;
        swprintf(path,32768,L"%ls",runtime.c_str());found=true;break;
    }
    if(!found) {
        MessageBoxW(NULL,L"FieldLeader3D 적용 파일을 확인하지 못했습니다. 설치 프로그램에서 설치 상태를 확인해 주세요.",L"FieldLeader3D",MB_OK|MB_ICONINFORMATION);
        return 0;
    }
    swprintf(command,32768,L"\"%ls\" --automatic %lu",path,(unsigned long)GetCurrentProcessId());
    STARTUPINFOW startup={};startup.cb=sizeof startup;PROCESS_INFORMATION process={};
    if(CreateProcessW(path,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,host,&startup,&process)) {
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
    } else MessageBoxW(NULL,L"FieldLeader3D 자동 적용을 시작하지 못했습니다. 설치 프로그램을 열어 설치 상태를 확인해 주세요.",L"FieldLeader3D",MB_OK|MB_ICONINFORMATION);
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void *) {
    if(reason==DLL_PROCESS_ATTACH) {
        self_module=module;DisableThreadLibraryCalls(module);
        // The worker runs only after the process loader releases its lock.
        HANDLE thread=CreateThread(NULL,0,start_runtime,NULL,0,NULL);
        if(thread) CloseHandle(thread);
    }
    return TRUE;
}
