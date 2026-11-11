#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <stdio.h>
#include "install_transaction.h"
#include "install_files.h"
#include "installer_payload.h"
#include "zip_store.h"
using std::wstring;
using fl_install::Manifest;
using fl_install::manifest_magic;
using fl_install::slot_names;
using fl_install::hash_file;
using fl_install::matches;
static HWND window,path_label,status_label;
static wstring selected,self_path,last_report;
static wstring parent(wstring p) {size_t i=p.find_last_of(L"\\/");return i==wstring::npos?L"":p.substr(0,i);}
static wstring normal_path(wstring p) {
 for(auto &c:p)if(c==L'/')c=L'\\';
 wchar_t full[32768];DWORD n=GetFullPathNameW(p.c_str(),32768,full,NULL);if(n&&n<32768)p=full;
 while(p.size()>3&&p.back()==L'\\')p.pop_back();return p;
}
static bool exists(const wstring &p) {return fl_install::path_exists(p);}
static bool regular(const wstring &p) {return fl_install::regular_file(p);}
static bool write_new(const wstring &p,const unsigned char *bytes,size_t size) {
 HANDLE f=CreateFileW(p.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);if(f==INVALID_HANDLE_VALUE)return false;
 DWORD n=0;bool ok=size<=MAXDWORD && WriteFile(f,bytes,DWORD(size),&n,NULL)&&n==size&&FlushFileBuffers(f);CloseHandle(f);
 if(!ok)DeleteFileW(p.c_str());return ok;
}
static DWORD running_game(const wstring &dir) {
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snap==INVALID_HANDLE_VALUE)return MAXDWORD;
 PROCESSENTRY32W e={};e.dwSize=sizeof e;DWORD pid=0;
 if(Process32FirstW(snap,&e)) {do{if(_wcsicmp(e.szExeFile,L"SAN14CE_KO.exe"))continue;
  HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,e.th32ProcessID);
  wchar_t path[32768];DWORD n=32768;
  if(!p){pid=MAXDWORD;break;}bool queried=QueryFullProcessImageNameW(p,0,path,&n)!=0;CloseHandle(p);
  if(!queried){pid=MAXDWORD;break;}bool match=!_wcsicmp(normal_path(parent(path)).c_str(),normal_path(dir).c_str());
  if(match){pid=e.th32ProcessID;break;}
 }while(Process32NextW(snap,&e));}
 CloseHandle(snap);return pid;
}
struct Disk {
 wstring game,dir;Manifest manifest={};int directory_index=-1;
 Disk(const wstring &p):game(normal_path(p)){directory_index=fl_install::choose_directory(*this);if(directory_index>=0)dir=folder(unsigned(directory_index));}
 wstring folder(unsigned i){return game+L"\\"+fl_install::directory_names[i];}
 bool owned_at(unsigned i){Manifest m={};return fl_install::read_manifest(folder(i),m);}
 bool exists_at(unsigned i){return exists(folder(i));}
 wstring path(unsigned i,int slot) {if(i==3)return game+L"\\"+slot_names[slot];return dir+L"\\"+(i==0?L"FieldLeader3D_Runtime.exe":i==1?L"FieldLeader3D_Setup.exe":L"install.dat");}
 bool running(){return running_game(game)!=0;}
 bool directory_exists(){return directory_index<0||exists(dir);}
 bool owned(){return directory_index>=0&&fl_install::read_manifest(dir,manifest);}
 int owned_slot(){return int(manifest.slot);}
 bool complete(){for(unsigned i:{0u,1u,3u})if(!exists(path(i,manifest.slot)))return false;return true;}
 bool repair(){
  if(memcmp(manifest.runtime,fl_runtime_sha256,32)||memcmp(manifest.proxy,fl_proxy_payloads[manifest.slot].sha256,32))return false;
  unsigned char h[32];if(!hash_file(self_path,h)||memcmp(h,manifest.manager,32))return false;
  for(unsigned i:{0u,1u,3u})if(!exists(path(i,manifest.slot))&&((i==3&&running())||!create(i,manifest.slot)))return false;
  return true;
 }
 bool verify(){
  // Missing owned files are accepted only for resuming removal. A changed or
  // replacement file, junction or foreign proxy is never removed.
  if(!fl_install::plain_directory(dir))return false;
  for(unsigned i:{0u,1u,3u}){const unsigned char *h=i==0?manifest.runtime:i==1?manifest.manager:manifest.proxy;
   if(exists(path(i,manifest.slot))&&!matches(path(i,manifest.slot),h))return false;}
  return true;
 }
 int free_slot(){for(unsigned i=0;i<fl_install::slot_count;++i)if(!exists(path(3,int(i))))return int(i);return -1;}
 bool make_directory(){return directory_index>=0&&CreateDirectoryW(dir.c_str(),NULL)!=0;}
 bool create(unsigned i,int slot){
  if(i==0)return write_new(path(i,slot),fl_runtime,sizeof fl_runtime);
  if(i==1)return CopyFileW(self_path.c_str(),path(i,slot).c_str(),TRUE)!=0;
  if(i==2){memcpy(manifest.magic,manifest_magic,16);manifest.revision=23;manifest.slot=unsigned(slot);
   memcpy(manifest.runtime,fl_runtime_sha256,32);memcpy(manifest.proxy,fl_proxy_payloads[slot].sha256,32);
   if(!hash_file(self_path,manifest.manager))return false;
   return write_new(path(i,slot),(unsigned char *)&manifest,sizeof manifest);}
  const auto &payload=fl_proxy_payloads[slot];return write_new(path(i,slot),payload.bytes,payload.size);
 }
 bool erase(unsigned i,int slot){wstring p=path(i,slot);return !exists(p)||DeleteFileW(p.c_str());}
 void remove_directory(){RemoveDirectoryW(dir.c_str());}
};
#include "install_upgrade.h"
#include "install_upgrade_disk.h"
static void status(const wchar_t *s){SetWindowTextW(status_label,s);}
static wstring read_text(const wstring &p){
 HANDLE f=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return L"";
 LARGE_INTEGER z={};if(!GetFileSizeEx(f,&z)||z.QuadPart<0||z.QuadPart>2*1024*1024){CloseHandle(f);return L"";}
 std::vector<char>b(size_t(z.QuadPart)+1);DWORD n=0;bool ok=ReadFile(f,b.data(),DWORD(z.QuadPart),&n,NULL)!=0;CloseHandle(f);if(!ok)return L"";
 int len=MultiByteToWideChar(CP_UTF8,0,b.data(),int(n),NULL,0);wstring out(size_t(len),L'\0');if(len)MultiByteToWideChar(CP_UTF8,0,b.data(),int(n),out.data(),len);return out;
}
static wstring value(const wstring &text,const wchar_t *key,size_t &from){
 wstring token=L"\""+wstring(key)+L"\"";size_t i=text.find(token,from);if(i==wstring::npos){from=i;return L"";}
 i=text.find(L'"',i+token.size());if(i==wstring::npos){from=i;return L"";}
 wstring out;for(++i;i<text.size();++i){if(text[i]==L'"'){from=i+1;return out;}if(text[i]==L'\\'&&i+1<text.size()&&(text[i+1]==L'\\'||text[i+1]==L'"'))++i;out+=text[i];}
 from=wstring::npos;return L"";
}
static void consider(const wstring &dir){if(selected.empty()&&regular(dir+L"\\SAN14CE_KO.exe"))selected=normal_path(dir);}
static void steam_library(const wstring &root){size_t pos=0;wstring name=value(read_text(root+L"\\steamapps\\appmanifest_872410.acf"),L"installdir",pos);
 if(!name.empty()&&name.find_first_of(L"\\/:")==wstring::npos)consider(root+L"\\steamapps\\common\\"+name);
}
static void locate(){
 consider(parent(self_path));consider(parent(parent(self_path)));
 wchar_t steam[32768]={};DWORD size=sizeof steam;
 if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath",RRF_RT_REG_SZ,NULL,steam,&size)!=ERROR_SUCCESS){size=sizeof steam;RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\WOW6432Node\\Valve\\Steam",L"InstallPath",RRF_RT_REG_SZ,NULL,steam,&size);}
 if(steam[0]){steam_library(steam);wstring vdf=read_text(wstring(steam)+L"\\steamapps\\libraryfolders.vdf");size_t pos=0;for(unsigned i=0;i<64&&pos!=wstring::npos;++i){wstring dir=value(vdf,L"path",pos);if(!dir.empty())steam_library(dir);}}
}
static void choose(){wchar_t path[32768]={};OPENFILENAMEW dialog={};dialog.lStructSize=sizeof dialog;dialog.hwndOwner=window;
 dialog.lpstrFile=path;dialog.nMaxFile=32768;dialog.lpstrFilter=L"삼국지14 실행 파일 (SAN14CE_KO.exe)\0SAN14CE_KO.exe\0\0";
 dialog.lpstrTitle=L"삼국지14가 설치된 폴더의 SAN14CE_KO.exe를 선택해 주세요 (처음 한 번만)";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(GetOpenFileNameW(&dialog)){selected=normal_path(parent(path));SetWindowTextW(path_label,selected.c_str());status(L"게임을 종료한 뒤 ‘설치’를 누르세요.");}}
static void results(){wchar_t local[32768]={};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768))return;
 wstring folder=wstring(local)+L"\\FieldLeader3D\\Results";CreateDirectoryW((wstring(local)+L"\\FieldLeader3D").c_str(),NULL);CreateDirectoryW(folder.c_str(),NULL);ShellExecuteW(window,L"open",folder.c_str(),NULL,NULL,SW_SHOWNORMAL);}
static void select_report(){if(last_report.empty()){results();return;}wstring args=L"/select,\""+last_report+L"\"";ShellExecuteW(window,L"open",L"explorer.exe",args.c_str(),NULL,SW_SHOWNORMAL);}
static bool install_report(const wchar_t *reason){
 if(selected.empty())return false;
 wstring text=L"FieldLeader3D R24 installer report\r\nGame: "+selected+L"\r\nResult: "+reason+L"\r\n";
 Disk disk(selected);
 text+=L"Private folder: "+disk.dir+L"\r\n";
 for(unsigned i=0;i<fl_install::slot_count;++i){wstring path=selected+L"\\"+slot_names[i];text+=wstring(slot_names[i])+L": ";
  if(!exists(path)){text+=L"vacant\r\n";continue;}
  text+=L"occupied (preserved)";unsigned char hash[32];
  if(regular(path)&&hash_file(path,hash)){text+=L" SHA256=";wchar_t hex[3];for(unsigned char byte:hash){swprintf(hex,3,L"%02x",unsigned(byte));text+=hex;}}
  text+=L"\r\n";
 }
 for(unsigned i=0;i<fl_install::directory_count;++i)text+=wstring(fl_install::directory_names[i])+L": "+(disk.owned_at(i)?L"owned":disk.exists_at(i)?L"foreign/preserved":L"vacant")+L"\r\n";
 WIN32_FIND_DATAW data={};HANDLE search=FindFirstFileW((selected+L"\\*.dll").c_str(),&data);
 if(search!=INVALID_HANDLE_VALUE){text+=L"Root DLL inventory (names only):\r\n";unsigned count=0;do{if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))text+=wstring(data.cFileName)+L"\r\n";}while(++count<512&&FindNextFileW(search,&data));FindClose(search);}
 int size=WideCharToMultiByte(CP_UTF8,0,text.c_str(),int(text.size()),NULL,0,NULL,NULL);if(size<=0)return false;
 std::string utf8(size_t(size),'\0');WideCharToMultiByte(CP_UTF8,0,text.c_str(),int(text.size()),utf8.data(),size,NULL,NULL);
 FLZip zip;zip.add("install.txt",reinterpret_cast<const unsigned char *>(utf8.data()),utf8.size());zip.finish();
 wchar_t local[32768];DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);if(!n||n>32000)return false;
 wstring folder=wstring(local)+L"\\FieldLeader3D";CreateDirectoryW(folder.c_str(),NULL);folder+=L"\\Results";CreateDirectoryW(folder.c_str(),NULL);
 SYSTEMTIME now;GetLocalTime(&now);wchar_t name[160];swprintf(name,160,L"\\FieldLeader3D_Install_%04u%02u%02u_%02u%02u%02u_%lu_%llu.zip",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,(unsigned long)GetCurrentProcessId(),(unsigned long long)GetTickCount64());
 wstring path=folder+name;if(!write_new(path,zip.bytes.data(),zip.bytes.size()))return false;last_report=path;return true;
}
static void collect(){if(selected.empty()){choose();if(selected.empty())return;}DWORD pid=running_game(selected);
 if(!pid||pid==MAXDWORD){if(install_report(L"User requested installation diagnostics"))select_report();else results();return;}
 Disk disk(selected);if(!disk.owned()||!matches(disk.path(0,0),disk.manifest.runtime)){if(install_report(L"Installation missing or changed"))select_report();else status(L"결과 파일을 저장하지 못했습니다. 저장 공간을 확인해 주세요.");return;}
 wchar_t args[128];swprintf(args,128,L"--collect %lu",(unsigned long)pid);
 ShellExecuteW(window,L"open",disk.path(0,0).c_str(),args,selected.c_str(),SW_HIDE);
 status(L"게임을 계속 실행한 채 결과를 모읍니다. 잠시 뒤 결과 파일이 선택된 폴더가 열립니다.");}
static void action(bool remove){
 if(selected.empty()){choose();if(selected.empty())return;}
 if(!regular(selected+L"\\SAN14CE_KO.exe")){status(L"게임 위치를 다시 선택해 주세요.");return;}
 if(!remove&&!matches(selected+L"\\SAN14CE_KO.exe",fl_game_sha256)){status(L"지원하는 한국어판 게임 실행 파일과 다릅니다. 게임 파일을 변경하지 않았습니다.");return;}
 Disk disk(selected);
 if(remove && !_wcsicmp(self_path.c_str(),disk.path(1,0).c_str())) {
  // An executable cannot delete itself while Windows has it mapped. Continue
  // the same requested removal from a temporary copy after this window exits.
  wchar_t temp[32768];DWORD n=GetTempPathW(32768,temp);if(!n||n>32000)return;
  wstring copy=wstring(temp)+L"FieldLeader3D_Remove_"+std::to_wstring(GetCurrentProcessId())+L".exe";
  if(!CopyFileW(self_path.c_str(),copy.c_str(),TRUE)){status(L"제거 준비를 하지 못했습니다. 내려받은 설치 프로그램에서 제거를 눌러 주세요.");return;}
  wstring args=L"\""+copy+L"\" --remove \""+selected+L"\"";STARTUPINFOW si={};si.cb=sizeof si;PROCESS_INFORMATION pi={};
  if(CreateProcessW(copy.c_str(),args.data(),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){CloseHandle(pi.hThread);CloseHandle(pi.hProcess);DestroyWindow(window);}
  else {DeleteFileW(copy.c_str());status(L"제거 준비를 하지 못했습니다. 내려받은 설치 프로그램에서 제거를 눌러 주세요.");}
  return;
 }
 bool pending=exists(disk.dir+L"\\upgrade.dat");
 bool outdated=disk.owned()&&(memcmp(disk.manifest.runtime,fl_runtime_sha256,32)||memcmp(disk.manifest.proxy,fl_proxy_payloads[disk.manifest.slot].sha256,32));
 fl_install::Result result;
 if(pending||(!remove&&outdated)) {
  UpgradeDisk update(disk);result=fl_install::upgrade(update);
  if(remove&&result==fl_install::installed)result=fl_install::uninstall(disk);
 } else result=remove?fl_install::uninstall(disk):fl_install::install(disk);
 switch(result){
 case fl_install::installed:status(L"설치 완료. 이제 평소처럼 게임을 실행하면 자동 적용됩니다. 이 창은 닫아도 됩니다.");break;
 case fl_install::already_installed:status(L"이미 설치되어 있습니다. 평소처럼 게임을 실행해 주세요.");break;
 case fl_install::removed:status(L"제거 완료. 다음 게임 실행부터 원래 상태로 돌아갑니다.");break;
 case fl_install::running:status(L"게임을 완전히 종료한 뒤 다시 눌러 주세요.");break;
 case fl_install::conflict:
  status(disk.directory_index<0?L"전용 설치 폴더 이름이 모두 사용 중입니다. 기존 폴더를 보존했습니다. ‘결과 저장’을 누르면 설치 기록이 열립니다.":L"자동 연결 파일 4개가 모두 사용 중입니다. 기존 패치를 보존했습니다. ‘결과 저장’을 눌러 나온 ZIP을 보내 주세요.");break;
 case fl_install::changed:status(L"설치 후 파일이 바뀌었습니다. 다른 패치를 보호하기 위해 덮어쓰거나 삭제하지 않았습니다.");break;
 case fl_install::absent:status(L"이 위치에는 FieldLeader3D가 설치되어 있지 않습니다.");break;
 default:status(L"작업을 완료하지 못했습니다. 게임 폴더의 쓰기 권한과 저장 공간을 확인해 주세요. ‘결과 저장’으로 설치 기록을 모을 수 있습니다.");break;}
 if(result==fl_install::conflict||result==fl_install::failed||result==fl_install::changed)install_report((L"Installer result="+std::to_wstring(int(result))).c_str());
}
static LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_COMMAND){switch(LOWORD(wp)){case 1:choose();break;case 2:action(false);break;case 3:action(true);break;case 4:collect();break;case 5:results();break;}return 0;}
 if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(w,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,wchar_t *,int show){
 int argc=0;wchar_t **argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 bool removing=argc==3&&!wcscmp(argv[1],L"--remove");wstring remove_dir=removing?argv[2]:L"";if(argv)LocalFree(argv);
 HANDLE mutex=NULL;
 for(unsigned attempt=0;attempt<50;++attempt){mutex=CreateMutexW(NULL,FALSE,L"Local\\FieldLeader3D_Setup");if(!mutex)return 2;if(GetLastError()!=ERROR_ALREADY_EXISTS)break;CloseHandle(mutex);mutex=NULL;if(!removing)return 0;Sleep(100);}
 if(!mutex)return 2;
 wchar_t path[32768];if(!GetModuleFileNameW(NULL,path,32768)){CloseHandle(mutex);return 2;}self_path=normal_path(path);if(removing)selected=normal_path(remove_dir);else locate();SetProcessDPIAware();
 WNDCLASSW wc={};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"FieldLeader3D_Setup";wc.hCursor=LoadCursorW(NULL,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);
 window=CreateWindowExW(0,wc.lpszClassName,L"FieldLeader3D R24 조형 연동 수정본 · 설치 / 제거",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,720,420,NULL,NULL,instance,NULL);
 HFONT font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
 auto control=[&](const wchar_t *cls,const wchar_t *text,int x,int y,int width,int height,int id,DWORD style=0)->HWND{HWND c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,window,(HMENU)(uintptr_t)id,instance,NULL);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;};
 control(L"STATIC",L"한 번 설치하면 게임 실행 때마다 자동으로 적용됩니다.\r\n공성병기는 원본 그대로, 부대정보 정리만 함께 적용합니다.",22,20,665,60,0);
 path_label=control(L"EDIT",selected.empty()?L"게임 위치를 자동으로 찾지 못했습니다.":selected.c_str(),22,94,525,32,0,ES_READONLY|ES_AUTOHSCROLL|WS_BORDER);
 control(L"BUTTON",L"게임 찾기",555,92,125,36,1,WS_TABSTOP);
 control(L"BUTTON",L"설치 / 업데이트",22,150,150,42,2,WS_TABSTOP|BS_DEFPUSHBUTTON);control(L"BUTTON",L"제거",184,150,150,42,3,WS_TABSTOP);
 control(L"BUTTON",L"결과 저장",346,150,160,42,4,WS_TABSTOP);control(L"BUTTON",L"결과 폴더",518,150,162,42,5,WS_TABSTOP);
 status_label=control(L"STATIC",L"게임을 종료한 뒤 ‘설치’를 누르세요. 설치 후에는 별도 실행기를 켤 필요가 없습니다.",22,218,660,88,0);
 control(L"STATIC",L"제거는 이 프로그램에서 가능합니다. 게임 저장 파일은 변경하지 않습니다.",22,328,660,35,0);
 ShowWindow(window,show);UpdateWindow(window);if(removing)action(true);MSG msg;
 while(GetMessageW(&msg,NULL,0,0)>0){if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 DeleteObject(font);CloseHandle(mutex);return 0;
}
