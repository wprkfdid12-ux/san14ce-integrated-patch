#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <conio.h>
#include "poc_contract.h"
#include "status_snapshot.h"
#include "game_guards.h"
#include "dll_build.h"
#include "zip_store.h"
#include "remote_probe.h"
#include "embedded_runtime.h"
#include "registry_bridge.h"

/* Guarded runtime for this exact game build. The separately installed forwarding
   DLL starts --automatic for its own game PID; --collect only reads that session.
   Native engine functions are never called from the loader's remote threads. */
static HANDLE game=INVALID_HANDLE_VALUE, session_log=INVALID_HANDLE_VALUE;
static DWORD game_pid;
static uintptr_t game_base, dll_base;
static uintptr_t registry_module,registry_table;
static uintptr_t info_destroy_next;
static bool registry_hash_known;
static bool runtime_configured;
static bool existing_session_only;
static char failure_code[96]="AUTO_START";
static FLZip live_evidence;
static bool live_evidence_captured;
static wchar_t directory[1024], dll_path[1024], dll_log_path[1024];
static wchar_t existing_runtime_path[1024], existing_loader_path[1024];
static void print_state(const FLStatus &s);
#include "launcher_ui.h"
#define puts ui_puts
#define printf ui_printf

static void log_text(const char *s) {
    if (session_log==INVALID_HANDLE_VALUE) return;
    static unsigned written=0;size_t length=strlen(s);if(written+length+2>2*1024*1024)return;
    written+=unsigned(length)+2;DWORD count; WriteFile(session_log,s,(DWORD)length,&count,NULL);
    WriteFile(session_log,"\r\n",2,&count,NULL);
}
static bool fail(const char *code, const char *message) {
    strncpy(failure_code,code,sizeof failure_code-1);
    char text[512]; snprintf(text,sizeof text,"ERROR %s Win32=%lu",code,(unsigned long)GetLastError());
    log_text(text); puts(message); puts("진단 결과를 자동으로 저장합니다."); return false;
}
static bool read_exact(uintptr_t address,void *buffer,SIZE_T size) {
    SIZE_T done=0;
    return ReadProcessMemory(game,(LPCVOID)address,buffer,size,&done) && done==size;
}
static bool write_exact(uintptr_t address,const void *buffer,SIZE_T size) {
    SIZE_T done=0;
    return WriteProcessMemory(game,(LPVOID)address,buffer,size,&done) && done==size;
}
static bool hash_file(const wchar_t *path,unsigned char out[32]) {
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,0,NULL);
    if (f==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_HASH_HANDLE hash=NULL;
    bool ok=false;
    if (BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)>=0 &&
        BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)>=0) {
        unsigned char b[65536]; DWORD n;
        ok=true;
        for (;;) {
            if (!ReadFile(f,b,sizeof b,&n,NULL)) { ok=false; break; }
            if (!n) break;
            if (BCryptHashData(hash,b,n,0)<0) { ok=false; break; }
        }
        if (ok && BCryptFinishHash(hash,out,32,0)<0) ok=false;
    }
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg,0);
    CloseHandle(f); return ok;
}
static uintptr_t module_named(const wchar_t *name,wchar_t *path=NULL) {
    HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,game_pid);
    if (s==INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W m={}; m.dwSize=sizeof m; uintptr_t found=0;
    if (Module32FirstW(s,&m)) do {
        if (_wcsicmp(m.szModule,name)==0) {
            found=(uintptr_t)m.modBaseAddr;
            if (path) wcsncpy(path,m.szExePath,1023);
            break;
        }
    } while (Module32NextW(s,&m));
    CloseHandle(s); return found;
}
static void module_name_utf8(const wchar_t *name,char *out,int capacity) {
    if (!WideCharToMultiByte(CP_UTF8,0,name,-1,out,capacity,NULL,NULL)) {
        strncpy(out,"<name unavailable>",(size_t)capacity-1); out[capacity-1]=0;
    }
}
static void log_modules() {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,game_pid);
    if (snapshot==INVALID_HANDLE_VALUE) { log_text("MODULE_SNAPSHOT_FAILED"); return; }
    MODULEENTRY32W m={}; m.dwSize=sizeof m; unsigned count=0;
    if (Module32FirstW(snapshot,&m)) do {
        char name[1024],row[1200]; module_name_utf8(m.szModule,name,sizeof name);
        snprintf(row,sizeof row,"MODULE base=%llx size=%lx name=%s",
                 (unsigned long long)(uintptr_t)m.modBaseAddr,(unsigned long)m.modBaseSize,name);
        log_text(row);
        if (++count==256) { log_text("MODULE_LIMIT 256"); break; }
    } while (Module32NextW(snapshot,&m));
    CloseHandle(snapshot);
}
static void log_address_owner(uintptr_t address) {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,game_pid);
    if (snapshot==INVALID_HANDLE_VALUE) { log_text("OWNER_SNAPSHOT_FAILED"); return; }
    MODULEENTRY32W m={}; m.dwSize=sizeof m; bool found=false;
    if (Module32FirstW(snapshot,&m)) do {
        uintptr_t start=(uintptr_t)m.modBaseAddr;
        if (address>=start && address-start<m.modBaseSize) {
            char name[1024],row[1200]; module_name_utf8(m.szModule,name,sizeof name);
            snprintf(row,sizeof row,"OWNER va=%llx module=%s rva=%llx",
                     (unsigned long long)address,name,(unsigned long long)(address-start));
            log_text(row); found=true; break;
        }
    } while (Module32NextW(snapshot,&m));
    CloseHandle(snapshot);
    if (!found) log_text("OWNER not in the loaded module list");
}
static void log_bytes(const char *label,uintptr_t address,const unsigned char *bytes,size_t length) {
    char hex[257],row[400];
    if (length>128) length=128;
    for (size_t i=0;i<length;++i) snprintf(hex+2*i,3,"%02x",bytes[i]);
    hex[2*length]=0;
    snprintf(row,sizeof row,"%s va=%llx length=%u bytes=%s",label,(unsigned long long)address,(unsigned)length,hex);
    log_text(row);
}
static void log_code_chain(uintptr_t address) {
    uintptr_t visited[4]={};
    for (unsigned depth=0;depth<4;++depth) {
        for (unsigned i=0;i<depth;++i) if (visited[i]==address) { log_text("BRANCH_CYCLE"); return; }
        visited[depth]=address; log_address_owner(address);
        unsigned char bytes[64]; size_t size=sizeof bytes;
        while (size>=8 && !read_exact(address,bytes,size)) size/=2;
        if (size<8) { log_text("BRANCH_BYTES_UNREADABLE"); return; }
        log_bytes("BRANCH_CODE",address,bytes,size);
        FLBranch branch=fl_probe_branch(address,bytes,size,read_exact);
        if (!branch.kind) { log_text("BRANCH_END unrecognized; bytes retained for review"); return; }
        char row[200]; snprintf(row,sizeof row,"BRANCH kind=%s resolved=%u slot=%llx target=%llx",branch.kind,
            branch.resolved?1:0,(unsigned long long)branch.slot,(unsigned long long)branch.target); log_text(row);
        if (!branch.resolved || !branch.target) return;
        address=branch.target;
    }
    log_address_owner(address); log_text("BRANCH_DEPTH_LIMIT 4");
}
static bool check_existing_session() {
    wchar_t path[1024]={};
    uintptr_t loaded=module_named(L"FieldLeader3D_PoC.dll",path);
    if (!loaded) return true;
    existing_session_only=true;
    char row[256]; snprintf(row,sizeof row,"ALREADY_LOADED base=%llx read-only inspection; no guard check or patch attempted",
                           (unsigned long long)loaded); log_text(row);
    unsigned char hash[32];
    if (hash_file(path,hash) && !memcmp(hash,fl_dll_sha256,32)) {
        log_text("EXISTING_DLL_FILE_HASH matched R24; reading known status layout only");
        FLStatus status={};
        if (fl_read_status(read_exact,loaded+FL_EXPORT_STATUS,loaded+FL_EXPORT_STATUS_SEQUENCE,status)) {
            log_text("EXISTING_STATUS stable sequence-checked snapshot");
            snprintf(row,sizeof row,"EXISTING_STATUS phase=%u reason=%u frame=%u draws=%u ready=%u",status.phase,status.reason,status.frame,status.draw_frames,status.ready_count);
            log_text(row);
            for (unsigned i=0;i<FL_SNAPSHOT_SLOTS;++i) {
                const uint64_t handles[]={status.body_handle[i],status.horse_handle[i],status.weapon_handle[i]};
                for (unsigned model=0;model<3;++model) {
                    uintptr_t setting=0,child=0,payload=0,vt=0;
                    bool readable=handles[model] && read_exact(handles[model]+0x20,&setting,8) && setting &&
                        read_exact(setting+0x30,&child,8) && child && read_exact(child+0x20,&payload,8) && payload &&
                        read_exact(payload,&vt,8);
                    snprintf(row,sizeof row,"EXISTING_MODEL slot=%u model=%u handle=%llx setting=%llx child=%llx payload=%llx vtable=%llx readable=%u",
                        i,model,(unsigned long long)handles[model],(unsigned long long)setting,
                        (unsigned long long)child,(unsigned long long)payload,(unsigned long long)vt,readable?1:0);
                    log_text(row);
                    if (readable) log_address_owner(vt);
                }
            }
        } else log_text("EXISTING_STATUS_UNAVAILABLE_OR_UNCONFIGURED");
        wchar_t *slash=wcsrchr(path,L'\\');
        if (slash) {
            *slash=0;
            if (wcslen(path)<850) {
                swprintf(existing_runtime_path,1024,L"%ls\\FieldLeader3D_Runtime.log",path);
                swprintf(existing_loader_path,1024,L"%ls\\FieldLeader3D_Loader.log",path);
                log_text("EXISTING_LOG_SOURCE loaded DLL directory; packaged separately from this attempt");
            }
        }
    } else log_text("EXISTING_DLL_FILE_HASH_UNVERIFIED status layout and previous logs not assumed");
    for (unsigned i=0;i<FL_PATCH_COUNT;++i) log_code_chain(game_base+fl_patch_specs[i].site);
    puts("이 게임에는 이전 시험 코드가 남아 있습니다. 게임을 완전히 종료한 뒤 다시 켜 주세요.\n이 창은 그대로 두시면 자동으로 이어서 진행합니다.");
    return false;
}
static bool find_game() {
    unsigned last_count=UINT32_MAX;
    for (;;) {
    if (result_requested()) { log_text("USER_ENDED_WAIT"); return false; }
    HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (s==INVALID_HANDLE_VALUE) return fail("PROCESS_SNAPSHOT","게임 목록을 읽지 못했습니다.");
    PROCESSENTRY32W p={}; p.dwSize=sizeof p; unsigned n=0;
    if (Process32FirstW(s,&p)) do {
        if (_wcsicmp(p.szExeFile,L"SAN14CE_KO.exe")==0) { game_pid=p.th32ProcessID; ++n; }
    } while(Process32NextW(s,&p));
    CloseHandle(s);
    if (n==1) break;
    if (n!=last_count) puts(n==0?"게임을 기다리고 있습니다. 삼국지14를 평소처럼 켜 주세요.":"삼국지14가 여러 개 실행 중입니다. 사용할 게임 하나만 남겨 주세요.");
    last_count=n; Sleep(400);
    }
    game=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|
                     PROCESS_CREATE_THREAD|SYNCHRONIZE,FALSE,game_pid);
    if (!game) return fail("OPEN_GAME","실행 중인 게임에 연결하지 못했습니다.");
    wchar_t path[1024]={}; game_base=module_named(L"SAN14CE_KO.exe",path);
    if (!game_base) return fail("GAME_MODULE","게임 실행 파일 위치를 읽지 못했습니다.");
    unsigned char hash[32];
    if (!hash_file(path,hash) || memcmp(hash,fl_expected_exe_sha256,32))
        return fail("GAME_BUILD_MISMATCH","분석했던 게임 버전과 다릅니다. 패치 적용을 중단했습니다.");
    char row[180]; snprintf(row,sizeof row,"GAME pid=%lu base=%llx disk_sha256=verified",
                           (unsigned long)game_pid,(unsigned long long)game_base); log_text(row);
    return true;
}
static bool wait_for_field() {
    puts("필드를 기다리고 있습니다. 평소 사용하시는 저장 게임을 불러와 주세요.");
    while (!result_requested()) {
        if (WaitForSingleObject(game,0)==WAIT_OBJECT_0) return false;
        uintptr_t manager=0,army=0,vt=0,context=0,list_handle=0,list=0,list_vt=0;
        if (read_exact(game_base+FL_UNIT_MANAGER,&manager,8) && manager &&
            read_exact(manager+0x18,&army,8) && army && read_exact(army,&vt,8) && vt==game_base+FL_ARMY_VTABLE &&
            read_exact(game_base+FL_RESOURCE_CONTEXT,&context,8) && context &&
            read_exact(context+0x98,&list_handle,8) && list_handle &&
            read_exact(list_handle+0x20,&list,8) && list && read_exact(list,&list_vt,8) && list_vt==game_base+FL_ARRAY_VTABLE) {
            log_text("FIELD_READY original active army and Array resource observed"); return true;
        }
        Sleep(300);
    }
    return false;
}
static void registry_detect() {
    static uint64_t last_scan;
    uint64_t now=GetTickCount64();
    if(!registry_hash_known && (!last_scan || now-last_scan>=2000)) {
        last_scan=now;wchar_t path[1024]={};
        uintptr_t m=module_named(L"SAN14_IntegratedConvenience.dll",path);
        if(m) {
            unsigned char h[32];registry_hash_known=true;
            if(hash_file(path,h)&&fl_registry::module_known(h)) {
                registry_module=m;log_text("R24_REGISTRY exact R2/R8/P1 verified; read-only bridge enabled");
            } else log_text("R24_REGISTRY unknown module build; no guard exemption");
        }
    }
    uintptr_t table=0;
    registry_table=fl_registry::installed(read_exact,game_base,registry_module,table)?table:0;
}
static void registry_publish() {
    static uint64_t last_poll;
    static uint32_t sequence;
    static bool incomplete;
    static fl_registry::Snapshot previous={},next={};
    uint64_t now=GetTickCount64();if(last_poll&&now-last_poll<500)return;last_poll=now;
    registry_detect();
    if(registry_table) {
        if(!fl_registry::collect(read_exact,game_base,registry_module,registry_table,FL_DATA_MANAGER,FL_PERSON_VTABLE,next))return;
    } else memset(&next,0,sizeof next);
    if(!incomplete&&!memcmp(&next,&previous,sizeof next))return;
    // Publish only into our own DLL, never into the game's registry. The field
    // thread reads one row with sequence checks and keeps the old model on a tear.
    uintptr_t box=dll_base+FL_EXPORT_REGISTRY;
    sequence+=2;uint32_t odd=sequence-1;incomplete=true;
    if(!write_exact(box,&odd,4)||!write_exact(box+offsetof(fl_registry::Mailbox,snapshot),&next,sizeof next)||
       !write_exact(box,&sequence,4)) {log_text("R24_REGISTRY snapshot transfer deferred");return;}
    previous=next;incomplete=false;
    unsigned count=0;for(const auto &row:next.rows)if(row.person)++count;
    char line[160];snprintf(line,sizeof line,"R24_REGISTRY snapshot active=%u mapped_officers=%u original_ids=0..24 extended_ids=25..173 catalog_only=1",next.active,count);log_text(line);
}
static bool check_build() {
    registry_detect();
    info_destroy_next=fl_registry::info_cleanup(read_exact,game_base,registry_module);
    if(info_destroy_next)log_text("R24_COMPAT integrated unit-info cleanup verified; chain preserved with option12 on or off");
    unsigned mismatches=0;
    for (unsigned i=0;i<FL_GUARD_COUNT;++i) {
        unsigned char b[64]; const FLGuard &g=fl_guards[i];
        bool readable=g.length<=sizeof b && read_exact(game_base+g.rva,b,g.length);
        DWORD error=readable?0:GetLastError();
        // R35: 맵 확장(dinput8)이 넓히는 칸 상한 즉치(0x408e6a, 원래 0xbd0f)는 값만 다르면 허용 — 3D 는 이 값을 그대로 읽어 씀
        if (readable && g.rva==0x408e54 && g.length==58 && !memcmp(b,g.bytes,22) && !memcmp(b+26,g.bytes+26,32)) {
            uint32_t v; memcpy(&v,b+22,4);
            if (v>=0xbd0f && v<0x80000000u) {
                if (v!=0xbd0f) { char row[120]; snprintf(row,sizeof row,"GUARD_COMPAT rva=00408e6a cell limit %#x (map expansion) accepted",(unsigned)v); log_text(row); }
                continue;
            }
        }
        if (!readable || !fl_registry::code_matches(game_base,registry_table,g.rva,g.bytes,b,g.length)) {
            char row[160]; snprintf(row,sizeof row,"GUARD_MISMATCH rva=%08lx readable=%u Win32=%lu",
                (unsigned long)g.rva,readable?1:0,(unsigned long)error); log_text(row);
            log_bytes("EXPECTED",game_base+g.rva,g.bytes,g.length);
            if (readable) log_bytes("ACTUAL",game_base+g.rva,b,g.length);
            if (mismatches<8) {
                unsigned char context[112];
                uintptr_t start=game_base+g.rva-24;
                if (read_exact(start,context,sizeof context)) log_bytes("CONTEXT",start,context,sizeof context);
                log_code_chain(game_base+g.rva);
            }
            ++mismatches;
        }
    }
    unsigned vtable_mismatches=0;
    for (unsigned i=0;i<FL_VTABLE_GUARD_COUNT;++i) {
        uintptr_t p=0;
        bool readable=read_exact(game_base+fl_vtable_guards[i].rva,&p,8);
        uintptr_t expected=game_base+fl_vtable_guards[i].target;
        if(registry_table && fl_vtable_guards[i].rva==fl_registry::update_slot)expected=registry_module+fl_registry::update_hook;
        if(registry_table && fl_vtable_guards[i].rva==fl_registry::create_slot)expected=registry_module+fl_registry::create_hook;
        if(info_destroy_next && fl_vtable_guards[i].rva==fl_registry::info_slot)expected=info_destroy_next;
        if (!readable || p!=expected) {
            char row[200]; snprintf(row,sizeof row,"VTABLE_MISMATCH rva=%08lx readable=%u expected=%llx actual=%llx",
                (unsigned long)fl_vtable_guards[i].rva,readable?1:0,
                (unsigned long long)(game_base+fl_vtable_guards[i].target),(unsigned long long)p); log_text(row);
            if (readable && p && vtable_mismatches<4) log_code_chain(p);
            ++vtable_mismatches;
        }
    }
    if (mismatches || vtable_mismatches) {
        char row[160]; snprintf(row,sizeof row,"GUARD_SUMMARY code=%u/%u vtable=%u/%u no DLL load or code write",
            mismatches,(unsigned)FL_GUARD_COUNT,vtable_mismatches,(unsigned)FL_VTABLE_GUARD_COUNT); log_text(row);
        log_modules();
        return fail("RUNTIME_GUARD","연결 지점의 코드가 예상과 다릅니다. 변경 내용은 결과 ZIP에 기록했습니다. 기존 패치를 지우거나 다시 설치할 필요는 없습니다.");
    }
    uintptr_t manager=0,data=0,head=0;
    if (!read_exact(game_base+FL_UNIT_MANAGER,&manager,8) || !manager ||
        !read_exact(game_base+FL_DATA_MANAGER,&data,8) || !data ||
        !read_exact(manager+0x18,&head,8) || !head)
        return fail("FIELD_NOT_READY","연결 준비 중 필드 상태가 바뀌어 패치 연결을 중단했습니다.");
    return true;
}

/* GetProcAddress may resolve a forwarder into KernelBase. Relocate from the
   module that actually owns the returned address, never assume equal ASLR. */
static uintptr_t remote_load_library() {
    FARPROC address=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HMODULE owner=NULL;
    if (!address || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                       (LPCWSTR)address,&owner)) return 0;
    wchar_t path[1024]={}; if (!GetModuleFileNameW(owner,path,1024)) return 0;
    const wchar_t *name=wcsrchr(path,L'\\'); name=name?name+1:path;
    uintptr_t remote=module_named(name);
    return remote ? remote + (uintptr_t)address-(uintptr_t)owner : 0;
}
static bool call_remote(uintptr_t entry,uintptr_t parameter,DWORD *exit_code) {
    HANDLE thread=CreateRemoteThread(game,NULL,0,(LPTHREAD_START_ROUTINE)entry,(LPVOID)parameter,0,NULL);
    if (!thread) return false;
    DWORD wait=WaitForSingleObject(thread,15000);
    bool ok=wait==WAIT_OBJECT_0 && GetExitCodeThread(thread,exit_code);
    /* On a timeout the parameter memory must remain alive. We never terminate
       a loader thread or free parameters that may still be in use. */
    CloseHandle(thread); return ok;
}
static bool load_dll() {
    if (module_named(L"FieldLeader3D_PoC.dll"))
        return fail("ALREADY_LOADED","이 게임 세션에는 시험 DLL이 이미 들어 있습니다. 새로 시험하려면 게임을 껐다 켜 주세요.");
    unsigned char hash[32];
    if (!hash_file(dll_path,hash) || memcmp(hash,fl_dll_sha256,32))
        return fail("DLL_HASH","내장된 패치 파일을 준비하지 못했습니다. 결과 파일을 보내 주세요.");
    uintptr_t loader=remote_load_library();
    SIZE_T bytes=(wcslen(dll_path)+1)*sizeof(wchar_t);
    uintptr_t remote=(uintptr_t)VirtualAllocEx(game,NULL,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if (!loader || !remote) return fail("LOAD_SETUP","시험 DLL 연결 준비를 하지 못했습니다.");
    if (!write_exact(remote,dll_path,bytes)) {
        VirtualFreeEx(game,(LPVOID)remote,0,MEM_RELEASE); return fail("LOAD_PATH","시험 파일 위치를 전달하지 못했습니다.");
    }
    DWORD result=0;
    if (!call_remote(loader,remote,&result)) return fail("LOAD_TIMEOUT","시험 연결 응답을 받지 못했습니다.");
    VirtualFreeEx(game,(LPVOID)remote,0,MEM_RELEASE);
    /* DWORD thread exit code truncates an HMODULE on x64: resolve the actual
       module from the target's module list instead of treating it as a pointer. */
    wchar_t actual[1024]={}; dll_base=module_named(L"FieldLeader3D_PoC.dll",actual);
    if (!dll_base || _wcsicmp(actual,dll_path)) return fail("DLL_NOT_LOADED","시험 DLL을 불러오지 못했습니다.");
    FLConfig config={}; config.protocol=FL_PROTOCOL; config.module_base=game_base;
    config.info_destroy_next=info_destroy_next;
    memcpy(config.log_path,dll_log_path,(wcslen(dll_log_path)+1)*sizeof(wchar_t));
    remote=(uintptr_t)VirtualAllocEx(game,NULL,sizeof config,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if (!remote) return fail("CONFIG_ALLOC","시험 설정 메모리를 준비하지 못했습니다.");
    if (!write_exact(remote,&config,sizeof config)) {
        VirtualFreeEx(game,(LPVOID)remote,0,MEM_RELEASE); return fail("CONFIG_WRITE","시험 설정을 전달하지 못했습니다.");
    }
    if (!call_remote(dll_base+FL_EXPORT_CONFIGURE,remote,&result))
        return fail("CONFIG_TIMEOUT","시험 설정 응답을 받지 못했습니다.");
    VirtualFreeEx(game,(LPVOID)remote,0,MEM_RELEASE);
    if (result!=1) return fail("CONFIG_FAILED","시험 로그를 열거나 설정을 적용하지 못했습니다.");
    runtime_configured=true;registry_publish();
    log_text("DLL_CONFIGURED no engine calls on the remote loader thread"); return true;
}

struct ThreadSet {
    HANDLE handles[2048]; DWORD ids[2048]; bool suspended[2048]; unsigned count;
    ThreadSet():count(0) { memset(suspended,0,sizeof suspended); }
    ~ThreadSet() { resume(); for (unsigned i=0;i<count;++i) CloseHandle(handles[i]); }
    void resume() { for (unsigned i=0;i<count;++i) if (suspended[i]) { ResumeThread(handles[i]); suspended[i]=false; } }
    bool gather() {
        HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
        if (s==INVALID_HANDLE_VALUE) return false;
        THREADENTRY32 t={}; t.dwSize=sizeof t; bool ok=true;
        if (Thread32First(s,&t)) do {
            if (t.th32OwnerProcessID!=game_pid) continue;
            bool exists=false; for (unsigned i=0;i<count;++i) if (ids[i]==t.th32ThreadID) exists=true;
            if (exists) continue;
            if (count==2048) { ok=false; break; }
            HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,t.th32ThreadID);
            if (!h) { ok=false; break; }
            handles[count]=h; ids[count]=t.th32ThreadID; suspended[count]=false; ++count;
        } while (Thread32Next(s,&t));
        CloseHandle(s); return ok;
    }
    bool freeze() {
        /* Re-enumerate after suspension so a thread created during the first
           pass cannot continue across the callsite commit. */
        for (unsigned pass=0;pass<4;++pass) {
            if (!gather() || !count) return false;
            for (unsigned i=0;i<count;++i) if (!suspended[i]) {
                DWORD prior=SuspendThread(handles[i]);
                if (prior==(DWORD)-1) {
                    DWORD code=0; if (GetExitCodeThread(handles[i],&code) && code!=STILL_ACTIVE) continue;
                    return false;
                }
                suspended[i]=true;
            }
            unsigned old=count; if (!gather()) return false;
            if (count==old) {
                for (unsigned i=0;i<count;++i) if (suspended[i]) {
                    CONTEXT c={}; c.ContextFlags=CONTEXT_CONTROL;
                    if (!GetThreadContext(handles[i],&c)) return false;
                    for (unsigned j=0;j<FL_PATCH_COUNT;++j) {
                        uintptr_t a=game_base+fl_patch_specs[j].site;
                        if (fl_patch_width(fl_patch_specs[j])==5 && c.Rip>a && c.Rip<a+5) return false;
                    }
                }
                return true;
            }
        }
        return false;
    }
};
static uintptr_t allocate_near() {
    SYSTEM_INFO info; GetSystemInfo(&info);
    uintptr_t gran=info.dwAllocationGranularity;
    uintptr_t centre=game_base & ~(gran-1);
    for (uintptr_t delta=gran;delta<0x70000000ULL;delta+=gran) {
        uintptr_t choices[2]={centre+delta,centre>delta?centre-delta:0};
        for (unsigned i=0;i<2;++i) if (choices[i]>=0x10000) {
            uintptr_t p=(uintptr_t)VirtualAllocEx(game,(LPVOID)choices[i],4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
            if (p) return p;
        }
    }
    return 0;
}
static bool install_calls() {
    uintptr_t stubs=allocate_near();
    if (!stubs) return fail("NEAR_ALLOC","필드 호출을 연결할 메모리를 확보하지 못했습니다.");
    unsigned char code[FL_PATCH_COUNT*16]={};
    const uint32_t exports[]={FL_EXPORT_UPDATE,FL_EXPORT_DRAW,FL_EXPORT_MANAGER_RELEASE,FL_EXPORT_RESOURCE_RELEASE,
        FL_EXPORT_FIELD_GROUP_DRAW,FL_EXPORT_FIELD_GROUP_DRAW,FL_EXPORT_FIELD_GROUP_DRAW,FL_EXPORT_FIELD_GROUP_DRAW,
        FL_EXPORT_FIELD_ACTION_DRAW,FL_EXPORT_FIELD_GROUP_DRAW,FL_EXPORT_FIELD_GROUP_DRAW,
        FL_EXPORT_FIELD_GROUP_DRAW,FL_EXPORT_FIELD_GROUP_DRAW,
        FL_EXPORT_EVENT_SOUND_2D,FL_EXPORT_EVENT_SOUND_3D,
        FL_EXPORT_TACTIC_EFFECT,FL_EXPORT_TACTIC_DAMAGE,
        FL_EXPORT_AMBIENT_ENQUEUE,FL_EXPORT_AMBIENT_ENQUEUE,FL_EXPORT_AMBIENT_ENQUEUE,FL_EXPORT_AMBIENT_CLEANUP,
        FL_EXPORT_NORMAL_DAMAGE,FL_EXPORT_MOTION_EFFECT,FL_EXPORT_TACTIC_CHILD,
        FL_EXPORT_AMBIENT_START,FL_EXPORT_AMBIENT_VOLUME,FL_EXPORT_ARMY_UPDATE,
        FL_EXPORT_GROUND_EFFECT,FL_EXPORT_NORMAL_FIELD_EFFECT,FL_EXPORT_DEFEAT_COMMIT,FL_EXPORT_ARMY_DETACH,FL_EXPORT_INFO_ANCHOR,FL_EXPORT_INFO_DESTROY,FL_EXPORT_ANIMATION_RATE,FL_EXPORT_ANIMATION_RATE,FL_EXPORT_MOTION_EVALUATE,FL_EXPORT_MOTION_COMMIT};
    static_assert(sizeof(exports)/sizeof(exports[0])==FL_PATCH_COUNT,"Every patched call must have a paired export");
    unsigned char original[FL_PATCH_COUNT][8]={}, replacement[FL_PATCH_COUNT][8]={};
    for (unsigned i=0;i<FL_PATCH_COUNT;++i) {
        /* jmp qword ptr [rip+0], followed by the full 64-bit destination.
           Tail jump leaves the original call's return address and ABI intact. */
        code[i*16]=0xff; code[i*16+1]=0x25;
        uint64_t destination=dll_base+exports[i]; memcpy(code+i*16+6,&destination,8);
        if (!fl_encode_patch(fl_patch_specs[i],game_base,stubs+i*16,original[i],replacement[i])) {
            VirtualFreeEx(game,(LPVOID)stubs,0,MEM_RELEASE); return false;
        }
        if(info_destroy_next && fl_patch_specs[i].site==fl_registry::info_slot)
            memcpy(original[i],&info_destroy_next,8);
    }
    DWORD old=0;
    if (!write_exact(stubs,code,sizeof code) || !VirtualProtectEx(game,(LPVOID)stubs,4096,PAGE_EXECUTE_READ,&old) ||
        !FlushInstructionCache(game,(LPCVOID)stubs,sizeof code)) {
        VirtualFreeEx(game,(LPVOID)stubs,0,MEM_RELEASE); return fail("STUB_WRITE","필드 호출 연결 준비에 실패했습니다.");
    }
    bool success=false, touched=false, rollback_ok=true;
    DWORD protections[FL_PATCH_COUNT]={}; bool changed[FL_PATCH_COUNT]={};
    {
        ThreadSet threads;
        if (threads.freeze()) {
            bool good=true;
            for (unsigned i=0;i<FL_PATCH_COUNT;++i) {
                unsigned char b[8];
                if (!read_exact(game_base+fl_patch_specs[i].site,b,fl_patch_width(fl_patch_specs[i])) || memcmp(b,original[i],fl_patch_width(fl_patch_specs[i]))) good=false;
            }
            for (unsigned i=0;good && i<FL_PATCH_COUNT;++i) {
                changed[i]=VirtualProtectEx(game,(LPVOID)(game_base+fl_patch_specs[i].site),fl_patch_width(fl_patch_specs[i]),
                                            PAGE_EXECUTE_READWRITE,&protections[i])!=0;
                if (!changed[i]) good=false;
            }
            if (good) for (unsigned i=0;i<FL_PATCH_COUNT;++i) {
                touched=true;
                if (!write_exact(game_base+fl_patch_specs[i].site,replacement[i],fl_patch_width(fl_patch_specs[i]))) { good=false; break; }
            }
            for (unsigned i=0;good && i<FL_PATCH_COUNT;++i) {
                unsigned char b[8];
                if (!read_exact(game_base+fl_patch_specs[i].site,b,fl_patch_width(fl_patch_specs[i])) || memcmp(b,replacement[i],fl_patch_width(fl_patch_specs[i]))) good=false;
            }
            if (good && !FlushInstructionCache(game,NULL,0)) good=false;
            if (!good && touched) {
                for (unsigned i=0;i<FL_PATCH_COUNT;++i) {
                    unsigned char b[8];
                    if (!write_exact(game_base+fl_patch_specs[i].site,original[i],fl_patch_width(fl_patch_specs[i])) ||
                        !read_exact(game_base+fl_patch_specs[i].site,b,fl_patch_width(fl_patch_specs[i])) || memcmp(b,original[i],fl_patch_width(fl_patch_specs[i]))) rollback_ok=false;
                }
                if (!FlushInstructionCache(game,NULL,0)) rollback_ok=false;
            }
            /* The first two sites share a page: restore in REVERSE order. */
            for (unsigned n=FL_PATCH_COUNT;n>0;--n) if (changed[n-1]) {
                DWORD discard;
                if (!VirtualProtectEx(game,(LPVOID)(game_base+fl_patch_specs[n-1].site),fl_patch_width(fl_patch_specs[n-1]),
                                      protections[n-1],&discard)) rollback_ok=false;
            }
            success=good && rollback_ok;
        }
        /* All previously suspended target threads resume on every exit path. */
    }
    if (!success) {
        if (touched) {
            DWORD result=0;
            /* A protection-restore failure may leave valid hooks installed.
               Ask them to stop instead of silently leaving a partial trial. */
            call_remote(dll_base+FL_EXPORT_STOP,0,&result);
        }
        if (!touched && rollback_ok) VirtualFreeEx(game,(LPVOID)stubs,0,MEM_RELEASE);
        /* Keep stubs/DLL allocated if any target reference may remain. */
        return fail(rollback_ok?"PATCH_NOT_COMMITTED":"PATCH_ROLLBACK_UNCERTAIN",
                    "패치 연결을 완료하지 못했습니다.");
    }
    log_text("HOOKS_COMMITTED 29 verified E8 calls and three guarded presentation/lifetime vtable pointers; no executable disk writes");
    return true;
}

static void print_state(const FLStatus &s) {
    char line[384]; snprintf(line,sizeof line,"STATUS phase=%u reason=%u frame=%u draws=%u ready=%u target=%u pending=%u water=%u excluded=%u creates=%u releases=%u callback_vt=%llx",
        s.phase,s.reason,s.frame,s.draw_frames,s.ready_count,s.target_count,s.pending_count,s.water_count,s.excluded_count,s.creates,s.releases,
        (unsigned long long)s.callback_vtable_rva); log_text(line);
    if (s.phase==FL_BLOCKED) puts("시험 표시를 멈췄습니다. 결과를 자동 저장합니다.");
    else if (s.phase==FL_STOPPED) puts("시험 표시를 종료했습니다. 원래 부대 모습으로 돌아갑니다.");
    else if (s.reason==FL_NO_MAP) puts("필드 화면을 기다리고 있습니다. 저장한 게임을 불러와 주세요.");
    else if (s.reason==FL_NO_LAND) puts("현재 표시할 육상 부대가 없습니다. 출진하면 자동으로 적용합니다. 수상 부대는 배 모습을 유지합니다.");
    else if (s.ready_count && s.draw_frames) {
        snprintf(line,sizeof line,"장수 표시 %u / 대상 %u부대 · 준비 중 %u부대 · 수상 %u부대는 원래 배 모습입니다. 이동음·통상공격·전법 동작을 확인한 뒤 ‘결과 저장’을 눌러 주세요.",
            s.ready_count,s.target_count,s.pending_count,s.water_count); puts(line);
    } else if (s.phase==FL_LOADING) puts("장수·말·무기를 차례로 불러오고 있습니다. 준비된 부대부터 자동으로 바뀝니다.");
}
static void capture_live_evidence() {
    if (live_evidence_captured) return;
    live_evidence_captured=true;
    FLStatus status={};
    if (!fl_read_status(read_exact,dll_base+FL_EXPORT_STATUS,dll_base+FL_EXPORT_STATUS_SEQUENCE,status)) return;
    live_evidence.add("FLStatus.bin",(const unsigned char *)&status,sizeof status);
    FLRosterAudit roster={};
    if (fl_read_audit(read_exact,dll_base+FL_EXPORT_ROSTER_AUDIT,dll_base+FL_EXPORT_STATUS_SEQUENCE,roster)) {
        live_evidence.add("Roster_audit.bin",(const unsigned char *)&roster,sizeof roster);
        std::string csv="officer_id,body,weapon,body_color,horse_color,selection_resolved,selection_unresolved,field_color_ready,field_submitted,pixels_verified\r\n";
        char row[240];
        for(unsigned id=0;id<=FL_MAX_OFFICER;++id) {
            const FLRosterRow &r=roster.rows[id]; if(!r.flags) continue;
            snprintf(row,sizeof row,"%u,%d,%d,%d,%d,%u,%u,%u,%u,0\r\n",id,r.body,r.weapon,r.body_color,r.horse_color,
                !!(r.flags&FL_ROSTER_RESOLVED),!!(r.flags&FL_ROSTER_UNRESOLVED),
                !!(r.flags&FL_ROSTER_COLOR_READY),!!(r.flags&FL_ROSTER_SUBMITTED));
            csv+=row;
        }
        live_evidence.add("Officer_appearance_audit.csv",(const unsigned char *)csv.data(),csv.size());
        snprintf(row,sizeof row,"ROSTER_CAPTURE generation=%u scanned=%u registered=%u resolved=%u unresolved=%u complete=%u pixels_verified=0",
            roster.generation,roster.scanned_slots,roster.registered,roster.resolved,roster.unresolved,roster.complete);
        log_text(row);
    } else log_text("ROSTER_CAPTURE_UNAVAILABLE no coherent snapshot; not assumed complete");
    /* ReadProcessMemory tolerates an owner disappearing between reads. No
       engine function, arbitrary process dump or external collector is used. */
    static const char *parts[]={"body","horse","weapon"};
    unsigned sampled=0;
    for(unsigned slot=0;slot<FL_PROXY_COUNT && sampled<FL_SNAPSHOT_SLOTS;++slot) {
        if (status.appearance[slot]<0 || !status.body_handle[slot]) continue;
        ++sampled;
        const uint64_t handles[]={status.body_handle[slot],status.horse_handle[slot],status.weapon_handle[slot]};
        for(unsigned k=0;k<3;++k) {
            uintptr_t setting=0,child=0,payload=0,vt=0;
            unsigned char bytes[0x370];
            if (handles[k] && read_exact(handles[k]+0x20,&setting,8) && setting &&
                read_exact(setting+0x30,&child,8) && child && read_exact(child+0x20,&payload,8) && payload &&
                read_exact(payload,&vt,8) && vt==game_base+FL_CHARACTER_VTABLE && read_exact(payload,bytes,sizeof bytes)) {
                char name[96]; snprintf(name,sizeof name,"Slot_%u_Officer_%u_%s.bin",slot,status.officer[slot],parts[k]);
                live_evidence.add(name,bytes,sizeof bytes);
            }
        }
    }
    live_evidence.finish();
    log_text("LIVE_EVIDENCE bounded read-only Character snapshots; best effort during live execution");
}
static bool monitor() {
    puts("게임에 연결했습니다. 공성병기를 제외한 육상 부대에 자동 적용합니다. 이동음·통상공격·전법 동작을 확인한 뒤 ‘결과 저장’을 눌러 주세요.");
    unsigned last_phase=0,last_reason=UINT32_MAX,last_ready=UINT32_MAX,last_draw=0,last_target=UINT32_MAX,last_water=UINT32_MAX;
    FLStatus s={};
    uint32_t last_roster_generation=0;
    for (;;) {
        if (WaitForSingleObject(game,0)==WAIT_OBJECT_0) { log_text("GAME_EXITED"); break; }
        registry_publish();
        LONG request=result_requested();
        if (request==1) break; /* Explicit close: preserve diagnostics even before ready. */
        if (request==2 && (LONG64)GetTickCount64()>=InterlockedCompareExchange64(&ui_capture_after,0,0)) {
            if (!capture_game()) puts("화면은 담지 못했지만 기록은 저장합니다. 다시 누를 필요 없이 잠시 기다려 주세요.");
            break;
        }
        if (fl_read_status(read_exact,dll_base+FL_EXPORT_STATUS,dll_base+FL_EXPORT_STATUS_SEQUENCE,s)) {
            ui_set_capture_ready(s.phase!=FL_BLOCKED && !s.stopped);
            if (s.phase!=last_phase || s.reason!=last_reason || s.ready_count!=last_ready || s.target_count!=last_target || s.water_count!=last_water || (!last_draw && s.draw_frames)) {
                print_state(s); last_phase=s.phase; last_reason=s.reason; last_ready=s.ready_count; last_draw=s.draw_frames; last_target=s.target_count; last_water=s.water_count;
            }
            if (s.phase==FL_BLOCKED) break;
            struct AuditHeader { uint32_t protocol,generation,scanned,registered,resolved,unresolved,complete,reserved; } audit={};
            if (fl_read_audit(read_exact,dll_base+FL_EXPORT_ROSTER_AUDIT,dll_base+FL_EXPORT_STATUS_SEQUENCE,audit) &&
                audit.complete && audit.generation!=last_roster_generation) {
                last_roster_generation=audit.generation;
                printf("인물 외형 정보 검사 완료: %u명 확인 · 추가 확인 %u명. 결과 파일에 자동으로 포함합니다. 화면에서 장수와 말의 모습도 확인해 주세요.",
                       audit.resolved,audit.unresolved);
            }
        }
        Sleep(200);
    }
    ui_set_capture_ready(false);
    if (WaitForSingleObject(game,0)!=WAIT_OBJECT_0) {
        capture_live_evidence();
        DWORD result;
        if (call_remote(dll_base+FL_EXPORT_STOP,0,&result)) {
            for (unsigned i=0;i<25;++i) {
                if (fl_read_status(read_exact,dll_base+FL_EXPORT_STATUS,dll_base+FL_EXPORT_STATUS_SEQUENCE,s) && s.stopped) break;
                Sleep(100);
            }
            print_state(s);
            log_text(s.stopped?"STOPPED original field calls continue":"STOP_REQUESTED cleanup waits for the next field frame");
        }
    }
    return WaitForSingleObject(game,0)==WAIT_OBJECT_0 || s.phase!=FL_BLOCKED;
}
static bool collect_result() {
    FLZip zip;
    const wchar_t *sources[]={L"FieldLeader3D_Loader.log",L"FieldLeader3D_Runtime.log"};
    const char *names[]={"FieldLeader3D_Loader.log","FieldLeader3D_Runtime.log"};
    for (unsigned i=0;i<2;++i) {
        if (i==1 && !runtime_configured) continue;
        wchar_t path[1024]; swprintf(path,1024,L"%ls\\%ls",directory,sources[i]);
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,0,NULL);
        if (f==INVALID_HANDLE_VALUE) continue;
        LARGE_INTEGER n; bool ok=GetFileSizeEx(f,&n)!=0 && n.QuadPart>=0 && n.QuadPart<=8*1024*1024;
        if (ok) {
            std::vector<unsigned char> b((size_t)n.QuadPart); DWORD got=0;
            ok=b.empty() || (ReadFile(f,b.data(),(DWORD)b.size(),&got,NULL) && got==b.size());
            if (ok) zip.add(names[i],b.data(),b.size());
        }
        CloseHandle(f);
        if (!ok) {
            const char *message="This log could not be read completely; other diagnostics remain available.\n";
            zip.add(i==0?"Loader_log_unavailable.txt":"Runtime_log_unavailable.txt",
                (const unsigned char *)message,strlen(message));
        }
    }
    const wchar_t *previous_paths[]={existing_runtime_path,existing_loader_path};
    const char *previous_names[]={"ExistingSession_Runtime.log","ExistingSession_Loader.log"};
    for (unsigned i=0;i<2;++i) {
        if (!previous_paths[i][0]) continue;
        HANDLE f=CreateFileW(previous_paths[i],GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             NULL,OPEN_EXISTING,0,NULL);
        if (f==INVALID_HANDLE_VALUE) continue;
        LARGE_INTEGER n; bool ok=GetFileSizeEx(f,&n)!=0 && n.QuadPart>=0 && n.QuadPart<=8*1024*1024;
        if (ok) {
            std::vector<unsigned char> b((size_t)n.QuadPart); DWORD got=0;
            ok=b.empty() || (ReadFile(f,b.data(),(DWORD)b.size(),&got,NULL) && got==b.size());
            if (ok) zip.add(previous_names[i],b.data(),b.size());
        }
        CloseHandle(f);
        if (!ok) {
            const char *message="Existing-session log could not be read completely; inspect the current loader snapshot.\n";
            zip.add(i==0?"ExistingSession_Runtime_unavailable.txt":"ExistingSession_Loader_unavailable.txt",
                    (const unsigned char*)message,strlen(message));
        }
    }
    char dll_hash[65]; for (unsigned i=0;i<32;++i) snprintf(dll_hash+2*i,3,"%02x",fl_dll_sha256[i]);
    char meta[640]; snprintf(meta,sizeof meta,"{\"test\":\"FieldLeader3D_R24_registry_compat\",\"dll_sha256\":\"%s\",\"disk_game_sha256\":\"782cf229a56cdde2b95399e0e7536740b3f5e96bea702679365e4dad061abdce\",\"existing_session_read_only\":%s,\"runtime_configured_this_attempt\":%s,\"screenshot_included\":%s,\"visual_result\":\"NOT_AUTOMATICALLY_ASSERTED\"}\n",
        dll_hash,existing_session_only?"true":"false",runtime_configured?"true":"false",captured_image.empty()?"false":"true");
    zip.add("test_build.json",(const unsigned char *)meta,strlen(meta));
    if (!captured_image.empty()) zip.add("Game_screen.bmp",captured_image.data(),captured_image.size());
    if (!live_evidence.bytes.empty()) zip.add("Render_state.zip",live_evidence.bytes.data(),live_evidence.bytes.size());
    zip.finish();
    SYSTEMTIME now; GetLocalTime(&now);
    wchar_t result[1024];
    swprintf(result,1024,L"%ls\\FieldLeader3D_Result_%04u%02u%02u_%02u%02u%02u_%03u.zip",result_directory,
             now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,now.wMilliseconds);
    HANDLE f=CreateFileW(result,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,0,NULL);
    if (f==INVALID_HANDLE_VALUE) return false;
    DWORD done=0; bool ok=WriteFile(f,zip.bytes.data(),(DWORD)zip.bytes.size(),&done,NULL) && done==zip.bytes.size();
    if (ok) ok=FlushFileBuffers(f)!=0;
    CloseHandle(f); if (!ok) { DeleteFileW(result); return false; }
    wcscpy(result_path,result);
    puts("결과를 저장했습니다. 지금 열리는 폴더에서 선택된 ZIP 하나만 채팅에 올려 주세요.");
    return true;
}
static bool prepare_files() {
    wchar_t local[1024]={};
    DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,1024);
    if (!n || n>700) return false;
    wchar_t root[1024]; swprintf(root,1024,L"%ls\\FieldLeader3D",local);
    if (!CreateDirectoryW(root,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    swprintf(result_directory,1024,L"%ls\\Results",root);
    if (!CreateDirectoryW(result_directory,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    SYSTEMTIME t; GetLocalTime(&t);
    swprintf(directory,1024,L"%ls\\R48_%04u%02u%02u_%02u%02u%02u_%03u_%lu",root,
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,(unsigned long)GetCurrentProcessId());
    if (!CreateDirectoryW(directory,NULL)) return false;
    swprintf(dll_path,1024,L"%ls\\FieldLeader3D_PoC.dll",directory);
    swprintf(dll_log_path,1024,L"%ls\\FieldLeader3D_Runtime.log",directory);
    wchar_t log_path[1024]; swprintf(log_path,1024,L"%ls\\FieldLeader3D_Loader.log",directory);
    session_log=CreateFileW(log_path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_NEW,0,NULL);
    if (session_log==INVALID_HANDLE_VALUE) return false;
    HANDLE f=CreateFileW(dll_path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,0,NULL);
    if (f==INVALID_HANDLE_VALUE) return false;
    DWORD done=0; bool ok=WriteFile(f,fl_embedded_dll,sizeof fl_embedded_dll,&done,NULL) && done==sizeof fl_embedded_dll;
    if (ok) ok=FlushFileBuffers(f)!=0;
    CloseHandle(f); return ok;
}
static DWORD WINAPI run_trial(void *) {
    if (!prepare_files()) {
        puts("패치 파일을 자동으로 준비하지 못했습니다. 저장 공간을 확인한 뒤 다시 실행해 주세요.");
        if (session_log!=INVALID_HANDLE_VALUE) CloseHandle(session_log);
        PostMessageW(ui_window,UI_FINISHED,0,0); return 2;
    }
    log_text("FIELD_MOD R4.8 float-clock and pose-transition executable; land/elephant units; native siege; original water boats; bounded loading; capture failure never blocks result ZIP");
    bool ok=false;
    for (;;) {
        if (!find_game()) break;
        if (check_existing_session()) {
            existing_session_only=false;
            if (!result_requested()) ok=wait_for_field() && check_build() && load_dll() && install_calls();
            break;
        }
        while (!result_requested() && WaitForSingleObject(game,400)!=WAIT_OBJECT_0) {}
        CloseHandle(game); game=INVALID_HANDLE_VALUE; game_base=dll_base=0;
        if (result_requested()) break;
        puts("게임 종료를 확인했습니다. 게임을 다시 켜고 필드를 불러오면 자동으로 연결합니다.");
    }
    if (ok) monitor();
    if (game && game!=INVALID_HANDLE_VALUE) CloseHandle(game);
    if (session_log!=INVALID_HANDLE_VALUE) { CloseHandle(session_log); session_log=INVALID_HANDLE_VALUE; }
    if (!collect_result()) {
        ui_printf("결과 ZIP을 저장하지 못했습니다. 아래 폴더를 그대로 압축해 보내 주세요.\n");
        wchar_t *path=(wchar_t *)malloc((wcslen(directory)+1)*sizeof(wchar_t));
        if (path) { wcscpy(path,directory); if(!PostMessageW(ui_window,UI_TEXT,0,(LPARAM)path))free(path); }
        ShellExecuteW(ui_window,L"open",directory,NULL,NULL,SW_SHOWNORMAL);
    }
    PostMessageW(ui_window,UI_FINISHED,0,0); return ok?0:1;
}

#include "automatic_runtime.h"
