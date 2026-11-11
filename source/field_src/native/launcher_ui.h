#pragma once
#include <shellapi.h>
#include <stdarg.h>
#include <stdlib.h>
#include "capture_bounds.h"

/* One GUI, one worker, one result. F8 or the capture button captures the foreground
   game's client area; never capture another app or a desktop-sized region. */
static HWND ui_window,ui_messages,ui_save,ui_open;
static HFONT ui_font;
static HANDLE ui_worker;
static volatile LONG ui_result_request,ui_finished;
static volatile LONG ui_capture_ready;
alignas(8) static volatile LONG64 ui_capture_after;
static wchar_t result_path[1024],result_directory[1024];
static std::vector<unsigned char> captured_image;
static const UINT UI_TEXT=WM_APP+1, UI_FINISHED=WM_APP+2, UI_READY=WM_APP+3;
static DWORD WINAPI run_trial(void *);
static void log_text(const char *);
static LONG result_requested() { return InterlockedCompareExchange(&ui_result_request,0,0); }
static void ui_set_capture_ready(bool ready) {
    if (InterlockedExchange(&ui_capture_ready,ready?1:0)!=(ready?1:0))
        PostMessageW(ui_window,UI_READY,0,0);
}
static void ui_puts(const char *s) {
    if(!ui_window) {log_text(s);return;}
    int n=MultiByteToWideChar(CP_UTF8,0,s,-1,NULL,0);
    if (n<=0) return;
    wchar_t *text=(wchar_t *)malloc((size_t)n*sizeof(wchar_t));
    if (!text) return;
    MultiByteToWideChar(CP_UTF8,0,s,-1,text,n);
    if (!PostMessageW(ui_window,UI_TEXT,0,(LPARAM)text)) free(text);
}
static void ui_printf(const char *fmt,...) {
    char text[2048]; va_list args; va_start(args,fmt);
    vsnprintf(text,sizeof text,fmt,args); va_end(args); ui_puts(text);
}
static void open_result() {
    if (!result_path[0]) return;
    wchar_t args[1100]; swprintf(args,1100,L"/select,\"%ls\"",result_path);
    ShellExecuteW(ui_window,L"open",L"explorer.exe",args,NULL,SW_SHOWNORMAL);
}
static BOOL CALLBACK find_game_window(HWND window,LPARAM value) {
    DWORD pid=0; GetWindowThreadProcessId(window,&pid);
    if (pid==game_pid && IsWindowVisible(window) && !GetWindow(window,GW_OWNER)) {
        *reinterpret_cast<HWND *>(value)=window; return FALSE;
    }
    return TRUE;
}
static void ui_request_capture() {
    if (InterlockedCompareExchange(&ui_finished,0,0) || result_requested()) return;
    if (!InterlockedCompareExchange(&ui_capture_ready,0,0)) {
        ui_puts("아직 시험 표시를 준비하고 있습니다. 준비가 끝나면 ‘결과 저장’ 버튼이 켜집니다.");
        return;
    }
    HWND target=GetForegroundWindow(); DWORD pid=0;
    GetWindowThreadProcessId(target,&pid);
    if (pid!=game_pid) { target=NULL; EnumWindows(find_game_window,(LPARAM)&target); }
    /* Run foreground activation in the UI thread handling the user's action.
       The worker still verifies the foreground PID before and after capture. */
    if (target && IsIconic(target)) ShowWindow(target,SW_RESTORE);
    if (target) SetForegroundWindow(target);
    DWORD active_pid=0; GetWindowThreadProcessId(GetForegroundWindow(),&active_pid);
    if (active_pid==game_pid) ShowWindow(ui_window,SW_MINIMIZE);
    InterlockedExchange64(&ui_capture_after,(LONG64)GetTickCount64()+600);
    InterlockedCompareExchange(&ui_result_request,2,0);
    EnableWindow(ui_save,FALSE);
}
static bool capture_game() {
    HWND window=GetForegroundWindow(); DWORD pid=0;
    if (!window || !GetWindowThreadProcessId(window,&pid) || pid!=game_pid || IsIconic(window)) {
        log_text("SCREENSHOT_SKIPPED foreground window is not the target game"); return false;
    }
    RECT client; POINT origin={};
    if (!GetClientRect(window,&client) || !ClientToScreen(window,&origin)) return false;
    FLCaptureRect bounds;
    int vx=GetSystemMetrics(SM_XVIRTUALSCREEN),vy=GetSystemMetrics(SM_YVIRTUALSCREEN);
    if (!fl_capture_rect(origin.x,origin.y,client.right-client.left,client.bottom-client.top,
            vx,vy,GetSystemMetrics(SM_CXVIRTUALSCREEN),GetSystemMetrics(SM_CYVIRTUALSCREEN),bounds)) {
        log_text("SCREENSHOT_SKIPPED no supported visible game client rectangle; diagnostics will still save"); return false;
    }
    int w=bounds.width,h=bounds.height;
    HDC screen=GetDC(NULL),memory=screen?CreateCompatibleDC(screen):NULL;
    if (!screen || !memory) { if(screen)ReleaseDC(NULL,screen); return false; }
    BITMAPINFO info={}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=w; info.bmiHeader.biHeight=-h;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void *pixels=NULL; HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,NULL,0);
    HGDIOBJ old=bitmap?SelectObject(memory,bitmap):NULL;
    bool ok=bitmap && old && BitBlt(memory,0,0,w,h,screen,bounds.x,bounds.y,SRCCOPY)!=0;
    GdiFlush();
    DWORD after_pid=0; HWND after=GetForegroundWindow(); GetWindowThreadProcessId(after,&after_pid);
    ok=ok && after==window && after_pid==game_pid;
    if (ok) {
        BITMAPFILEHEADER header={}; header.bfType=0x4d42;
        header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);
        header.bfSize=header.bfOffBits+(DWORD)((size_t)w*h*4);
        captured_image.resize(header.bfSize);
        memcpy(captured_image.data(),&header,sizeof header);
        memcpy(captured_image.data()+sizeof header,&info.bmiHeader,sizeof info.bmiHeader);
        memcpy(captured_image.data()+header.bfOffBits,pixels,(size_t)w*h*4);
        char row[180]; snprintf(row,sizeof row,"SCREENSHOT_CAPTURED visible game client %dx%d cropped=%u; pixel visibility not asserted",w,h,bounds.cropped?1:0);
        log_text(row);
    } else log_text("SCREENSHOT_UNAVAILABLE capture failed or foreground changed");
    if (old) SelectObject(memory,old);
    if (bitmap) DeleteObject(bitmap);
    DeleteDC(memory); ReleaseDC(NULL,screen); return ok;
}
static HWND ui_control(const wchar_t *type,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id) {
    HWND control=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,ui_window,
        (HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    SendMessageW(control,WM_SETFONT,(WPARAM)ui_font,TRUE); return control;
}
static LRESULT CALLBACK ui_proc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case UI_TEXT: {
        wchar_t *text=(wchar_t *)lp;
        int length=GetWindowTextLengthW(ui_messages);
        SendMessageW(ui_messages,EM_SETSEL,length,length);
        SendMessageW(ui_messages,EM_REPLACESEL,FALSE,(LPARAM)text);
        SendMessageW(ui_messages,EM_REPLACESEL,FALSE,(LPARAM)L"\r\n\r\n");
        free(text); return 0;
    }
    case WM_HOTKEY:
        if (wp==1) ui_request_capture();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp)==101) ui_request_capture();
        else if (LOWORD(wp)==102) open_result();
        else if (LOWORD(wp)==103) SendMessageW(window,WM_CLOSE,0,0);
        return 0;
    case UI_FINISHED:
        InterlockedExchange(&ui_finished,1); UnregisterHotKey(window,1);
        EnableWindow(ui_save,FALSE); EnableWindow(ui_open,result_path[0]!=0);
        ShowWindow(window,SW_RESTORE);
        if (result_path[0]) open_result();
        return 0;
    case UI_READY:
        EnableWindow(ui_save,InterlockedCompareExchange(&ui_capture_ready,0,0) && !result_requested() &&
            !InterlockedCompareExchange(&ui_finished,0,0));
        return 0;
    case WM_CLOSE:
        if (!InterlockedCompareExchange(&ui_finished,0,0)) {
            InterlockedCompareExchange(&ui_result_request,1,0);
            EnableWindow(ui_save,FALSE);
            ui_puts("결과를 저장하고 있습니다. 잠시만 기다려 주세요.");
        } else DestroyWindow(window);
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window,message,wp,lp);
}
int WINAPI legacy_ui_main(HINSTANCE instance,HINSTANCE,wchar_t *,int show) {
    HANDLE single=CreateMutexW(NULL,FALSE,L"Local\\FieldLeader3D_IntegratedLauncher");
    if (!single) return 2;
    if (GetLastError()==ERROR_ALREADY_EXISTS) {
        HWND existing=FindWindowW(L"FieldLeader3D_Integrated",NULL);
        if (existing) { ShowWindow(existing,SW_RESTORE); SetForegroundWindow(existing); }
        CloseHandle(single); return 0;
    }
    SetProcessDPIAware();
    WNDCLASSW wc={}; wc.hInstance=instance; wc.lpfnWndProc=ui_proc;
    wc.lpszClassName=L"FieldLeader3D_Integrated"; wc.hCursor=LoadCursorW(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    if (!RegisterClassW(&wc)) { CloseHandle(single); return 2; }
    ui_window=CreateWindowExW(0,wc.lpszClassName,L"FieldLeader3D · R23",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,710,485,NULL,NULL,instance,NULL);
    if (!ui_window) { CloseHandle(single); return 2; }
    ui_font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
    ui_control(L"STATIC",L"R23 · 반구형 효과 제거 / 정지 흙먼지 정리 / 포획별 전멸 연출\r\n말 다리 사이 부대정보 / 품질 유지 최적화 · 확인 후 ‘결과 저장’을 누르세요.",0,22,18,660,60,0);
    ui_messages=ui_control(L"EDIT",L"",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_BORDER,22,90,650,245,0);
    ui_save=ui_control(L"BUTTON",L"결과 저장",WS_TABSTOP|BS_PUSHBUTTON,22,352,165,40,101);
    ui_open=ui_control(L"BUTTON",L"결과 파일 보기",WS_TABSTOP|BS_PUSHBUTTON,197,352,175,40,102);
    ui_control(L"BUTTON",L"닫기",WS_TABSTOP|BS_PUSHBUTTON,562,352,110,40,103);
    ui_control(L"STATIC",L"실제 표시를 확인하는 시험판입니다. 게임을 미리 저장해 주세요.",0,22,407,650,25,0);
    EnableWindow(ui_open,FALSE); EnableWindow(ui_save,FALSE);
    bool hotkey=RegisterHotKey(ui_window,1,MOD_NOREPEAT,VK_F8)!=0;
    ShowWindow(ui_window,show); UpdateWindow(ui_window);
    if (!hotkey) ui_puts("F8을 다른 프로그램이 사용 중입니다. 이 창의 ‘결과 저장’ 버튼을 사용하세요.");
    ui_worker=CreateThread(NULL,0,run_trial,NULL,0,NULL);
    if (!ui_worker) { ui_puts("시작하지 못했습니다. 창을 닫고 다시 실행해 주세요."); PostMessageW(ui_window,UI_FINISHED,0,0); }
    MSG msg;
    while (GetMessageW(&msg,NULL,0,0)>0) {
        if (!IsDialogMessageW(ui_window,&msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    if (ui_worker) { WaitForSingleObject(ui_worker,INFINITE); CloseHandle(ui_worker); }
    if(ui_font)DeleteObject(ui_font); CloseHandle(single); return 0;
}
