#pragma once
static int automatic_runtime(DWORD pid,bool collect) {
    wchar_t mutex_name[128];swprintf(mutex_name,128,L"Local\\FieldLeader3D_%ls_%lu",collect?L"Collect":L"Auto",(unsigned long)pid);
    HANDLE single=CreateMutexW(NULL,FALSE,mutex_name);
    if(!single) return 2;
    if(GetLastError()==ERROR_ALREADY_EXISTS) {CloseHandle(single);return 0;}
    game_pid=pid;
    if(!prepare_files()) {CloseHandle(single);return 2;}
    DWORD access=PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|SYNCHRONIZE;
    if(!collect) access|=PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_CREATE_THREAD;
    game=OpenProcess(access,FALSE,pid);
    bool ok=false,closed_before_field=false;
    if(game) {
        wchar_t host[1024]={},own[1024]={};
        // Toolhelp may transiently fail while Steam/the loader is still adding
        // modules. A temporary startup snapshot failure is not a version error.
        for(unsigned attempt=0;attempt<50;++attempt) {
            game_base=module_named(L"SAN14CE_KO.exe",host);
            if(game_base||WaitForSingleObject(game,100)==WAIT_OBJECT_0)break;
        }
        GetModuleFileNameW(NULL,own,1024);
        wchar_t *slash=wcsrchr(own,L'\\');if(slash)*slash=0;
        slash=wcsrchr(own,L'\\');if(slash)*slash=0;
        wchar_t expected[1024];swprintf(expected,1024,L"%ls\\SAN14CE_KO.exe",own);
        unsigned char hash[32];
        if(game_base && !_wcsicmp(host,expected) && hash_file(host,hash) && !memcmp(hash,fl_expected_exe_sha256,32)) {
            if(collect) {
                check_existing_session();
                dll_base=module_named(L"FieldLeader3D_PoC.dll");
                if(dll_base && existing_runtime_path[0]) {capture_live_evidence();capture_game();}
                ok=true;
            } else if(check_existing_session()) {
                bool ready=wait_for_field();
                closed_before_field=!ready && WaitForSingleObject(game,0)==WAIT_OBJECT_0;
                ok=ready && check_build() && load_dll() && install_calls();
                if(ok) ok=monitor();
            } else {
                FLStatus status={};uintptr_t loaded=module_named(L"FieldLeader3D_PoC.dll");
                ok=existing_runtime_path[0] && loaded &&
                    fl_read_status(read_exact,loaded+FL_EXPORT_STATUS,loaded+FL_EXPORT_STATUS_SEQUENCE,status) &&
                    !status.stopped && status.phase!=FL_BLOCKED;
                log_text(ok?"DUPLICATE_AUTO_ATTACH_SKIPPED":"OLDER_OR_STOPPED_SESSION close old test launcher and restart game");
            }
        } else {
            strncpy(failure_code,"INSTALL_LOCATION_OR_GAME_VERSION",sizeof failure_code-1);
            char actual[3072],wanted[3072],line[6400];
            module_name_utf8(host,actual,sizeof actual);module_name_utf8(expected,wanted,sizeof wanted);
            snprintf(line,sizeof line,"INSTALL_LOCATION_OR_GAME_VERSION_MISMATCH actual=%s expected=%s no game write",actual,wanted);log_text(line);
            if(game_base&&hash_file(host,hash))log_bytes("DISK_GAME_SHA256",0,hash,32);
        }
        CloseHandle(game);game=INVALID_HANDLE_VALUE;
    } else {strncpy(failure_code,"AUTO_OPEN_GAME",sizeof failure_code-1);log_text("AUTO_OPEN_GAME_FAILED no game write");}
    if(session_log!=INVALID_HANDLE_VALUE) {CloseHandle(session_log);session_log=INVALID_HANDLE_VALUE;}
    if(collect || (!ok && !closed_before_field)) {
        if(collect_result()) {
            if(collect) open_result();
            else {
                wchar_t reason[96]={},message[700];MultiByteToWideChar(CP_UTF8,0,failure_code,-1,reason,96);
                swprintf(message,700,L"FieldLeader3D 자동 적용을 완료하지 못했습니다.\r\n확인을 누르면 이번 실패 기록 ZIP이 선택된 폴더가 열립니다.\r\n선택된 ZIP 하나만 채팅에 올려 주세요.\r\n\r\n진단 코드: %ls",reason);
                MessageBoxW(NULL,message,L"FieldLeader3D R24",MB_OK|MB_ICONINFORMATION);open_result();
            }
        }
    }
    CloseHandle(single);return ok?0:1;
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,wchar_t *,int) {
    int count=0;wchar_t **args=CommandLineToArgvW(GetCommandLineW(),&count);
    bool valid=count==3 && (!wcscmp(args[1],L"--automatic") || !wcscmp(args[1],L"--collect"));
    wchar_t *end=NULL;unsigned long pid=valid?wcstoul(args[2],&end,10):0;
    if(!valid || !pid || !end || *end) {if(args)LocalFree(args);return 2;}
    bool collect=!wcscmp(args[1],L"--collect");LocalFree(args);
    return automatic_runtime(DWORD(pid),collect);
}
