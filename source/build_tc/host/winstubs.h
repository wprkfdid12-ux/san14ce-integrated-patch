#include <atomic>
// 리눅스 검사용 Win32 흉내 (실제 임시 폴더에 파일을 쓴다). test_tester.cpp / test_prisoner.cpp 공용.
// 경로 매핑: C:\Game\ → <tmp>/game/, C:\Local\ → <tmp>/local/, 그 외 → <tmp>/other/
#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <fnmatch.h>
#include <time.h>
typedef uint8_t U8;typedef uint16_t U16;typedef uint32_t U32;typedef int32_t I32;typedef uintptr_t UPTR;
extern "C" {
#include "mod_api.h"
}
// ------------------------------------------------------------------ 검사 뼈대
static int passes,failures;
static void check(bool ok,const char*what){if(ok){passes++;printf("  통과: %s\n",what);}else{failures++;printf("  실패: %s\n",what);}}
static std::string u8(const wchar_t*s){std::string o;for(;*s;s++){unsigned c=*s;if(c<0x80)o+=char(c);else if(c<0x800){o+=char(0xc0|(c>>6));o+=char(0x80|(c&63));}else{o+=char(0xe0|(c>>12));o+=char(0x80|((c>>6)&63));o+=char(0x80|(c&63));}}return o;}
static std::u16string w16(const char*s){std::u16string o;const unsigned char*p=(const unsigned char*)s;while(*p){unsigned c=*p;if(c<0x80){o+=char16_t(c);p++;}else if((c&0xe0)==0xc0){o+=char16_t(((c&31)<<6)|(p[1]&63));p+=2;}else{o+=char16_t(((c&15)<<12)|((p[1]&63)<<6)|(p[2]&63));p+=3;}}return o;}
// ------------------------------------------------------------------ 경로 매핑: C:\Game\ → 임시 폴더, %LOCALAPPDATA% → 임시 폴더\local
static std::string tmpRoot;
static std::string mapPath(const wchar_t*p){std::string s=u8(p);std::string out;
 if(s.rfind("C:\\Game\\",0)==0)out=tmpRoot+"/game/"+s.substr(8);else if(s.rfind("C:\\Local\\",0)==0)out=tmpRoot+"/local/"+s.substr(9);else out=tmpRoot+"/other/"+s;
 for(auto&c:out)if(c=='\\')c='/';return out;}
static void mkdirs(const std::string&path){std::string cur;for(size_t i=0;i<path.size();i++){cur+=path[i];if(path[i]=='/'&&i)mkdir(cur.c_str(),0755);}mkdir(path.c_str(),0755);}
static void writeFile(const std::string&path,const std::string&data){auto slash=path.rfind('/');mkdirs(path.substr(0,slash));FILE*f=fopen(path.c_str(),"wb");fwrite(data.data(),1,data.size(),f);fclose(f);}
static std::string readFile(const std::string&path){FILE*f=fopen(path.c_str(),"rb");if(!f)return "";std::string s;char b[4096];size_t n;while((n=fread(b,1,sizeof b,f))>0)s.append(b,n);fclose(f);return s;}
// ------------------------------------------------------------------ Win32 흉내
static std::map<std::string,void*> modules;static std::map<std::string,void*> procs;static unsigned char icDigest[32];
static std::string lower(std::string s){for(auto&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
extern "C" {
wchar_t*wcscpy(wchar_t*d,const wchar_t*s){wchar_t*r=d;while((*d++=*s++)){}return r;}
static void*exeHandle=nullptr; // 검사에서 실제 exe 이미지를 올린 주소(r39::exe_info 용)
HMODULE GetModuleHandleW(LPCWSTR n){if(!n)return exeHandle?(HMODULE)exeHandle:(HMODULE)1;auto k=lower(u8(n));auto i=modules.find(k);return i==modules.end()?nullptr:(HMODULE)i->second;}
void*GetProcAddress(HMODULE,LPCSTR n){auto i=procs.find(n);return i==procs.end()?nullptr:i->second;}
static std::map<std::string,std::string> moduleNames;
DWORD GetModuleFileNameW(HMODULE m,LPWSTR out,DWORD cap){std::u16string p=u"C:\\Game\\SAN14CE_KO.exe";for(auto&e:modules)if(e.second==(void*)m&&m!=(HMODULE)1){p=u"C:\\Game\\"+w16(moduleNames[e.first].c_str());}if(p.size()+1>cap)return 0;memcpy(out,p.c_str(),(p.size()+1)*2);return (DWORD)p.size();}
struct File{int fd;};
static std::atomic<unsigned> createfile_calls{0},flush_calls{0};   // R20: 파일 열기·디스크 동기화 횟수
HANDLE CreateFileW(LPCWSTR n,DWORD access,DWORD,void*,DWORD disp,DWORD,HANDLE){++createfile_calls;std::string p=mapPath(n);int flags=0;
 if(access&GENERIC_WRITE)flags=O_RDWR;else if(access&FILE_APPEND_DATA)flags=O_WRONLY|O_APPEND;else flags=O_RDONLY;
 if(disp==OPEN_ALWAYS)flags|=O_CREAT;else if(disp==CREATE_ALWAYS)flags|=O_CREAT|O_TRUNC;
 int fd=open(p.c_str(),flags,0644);if(fd<0)return INVALID_HANDLE_VALUE;return new File{fd};}
BOOL ReadFile(HANDLE h,LPVOID b,DWORD n,LPDWORD got,void*){ssize_t r=read(((File*)h)->fd,b,n);if(r<0){perror("read");return 0;}*got=(DWORD)r;return 1;}
BOOL WriteFile(HANDLE h,LPCVOID b,DWORD n,LPDWORD got,void*){ssize_t r=write(((File*)h)->fd,b,n);if(r<0)return 0;*got=(DWORD)r;return 1;}
BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE&&(UPTR)h>0x1000){File*f=(File*)h;if(f->fd>=0)close(f->fd);delete f;}return 1;}
BOOL GetFileSizeEx(HANDLE h,PLARGE_INTEGER s){struct stat st;if(fstat(((File*)h)->fd,&st))return 0;s->QuadPart=st.st_size;return 1;}
BOOL CreateDirectoryW(LPCWSTR n,void*){mkdirs(mapPath(n));return 1;}
DWORD GetFileAttributesW(LPCWSTR n){struct stat st;if(stat(mapPath(n).c_str(),&st))return INVALID_FILE_ATTRIBUTES;return S_ISDIR(st.st_mode)?FILE_ATTRIBUTE_DIRECTORY:0x20;}
BOOL DeleteFileW(LPCWSTR n){return unlink(mapPath(n).c_str())==0;}
BOOL MoveFileExW(LPCWSTR a,LPCWSTR b,DWORD){return rename(mapPath(a).c_str(),mapPath(b).c_str())==0;}
struct Find{DIR*d;std::string pat;};
static bool fillFind(Find*f,WIN32_FIND_DATAW*fd){while(dirent*e=readdir(f->d)){if(e->d_name[0]=='.')continue;if(fnmatch(f->pat.c_str(),e->d_name,0))continue;memset(fd,0,sizeof *fd);std::u16string n=w16(e->d_name);memcpy(fd->cFileName,n.c_str(),(n.size()+1)*2);fd->dwFileAttributes=e->d_type==DT_DIR?FILE_ATTRIBUTE_DIRECTORY:0x20;return true;}return false;}
HANDLE FindFirstFileW(LPCWSTR n,WIN32_FIND_DATAW*fd){std::string p=mapPath(n);auto slash=p.rfind('/');Find*f=new Find{opendir(p.substr(0,slash).c_str()),p.substr(slash+1)};if(!f->d||!fillFind(f,fd)){if(f->d)closedir(f->d);delete f;return INVALID_HANDLE_VALUE;}return f;}
BOOL FindNextFileW(HANDLE h,WIN32_FIND_DATAW*fd){return fillFind((Find*)h,fd);}
BOOL FindClose(HANDLE h){Find*f=(Find*)h;closedir(f->d);delete f;return 1;}
DWORD GetPrivateProfileStringW(LPCWSTR sec,LPCWSTR key,LPCWSTR def,LPWSTR out,DWORD cap,LPCWSTR file){
 std::string s=readFile(mapPath(file));std::string want="["+lower(u8(sec))+"]";std::string k=lower(u8(key));bool in=false;std::string val=u8(def);
 size_t pos=0;while(pos<s.size()){size_t e=s.find('\n',pos);if(e==std::string::npos)e=s.size();std::string line=s.substr(pos,e-pos);while(!line.empty()&&(line.back()=='\r'||line.back()==' '))line.pop_back();pos=e+1;
  if(line.size()&&line[0]=='['){in=lower(line)==want;continue;}if(!in)continue;auto eq=line.find('=');if(eq==std::string::npos)continue;if(lower(line.substr(0,eq))==k){val=line.substr(eq+1);break;}}
 std::u16string wv=w16(val.c_str());if(wv.size()+1>cap)wv.resize(cap-1);memcpy(out,wv.c_str(),(wv.size()+1)*2);return (DWORD)wv.size();}
unsigned GetPrivateProfileIntW(LPCWSTR sec,LPCWSTR key,int def,LPCWSTR file){wchar_t b[32];GetPrivateProfileStringW(sec,key,L"",b,32,file);if(!b[0])return def;return (unsigned)atoi(u8(b).c_str());}
BOOL WritePrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR){return 1;}
int WideCharToMultiByte(unsigned,DWORD,LPCWSTR s,int n,char*out,int cap,LPCSTR,BOOL*){std::string o=u8(s);if(n==-1)o.push_back(0);if(!cap)return (int)o.size();if((int)o.size()>cap)return 0;memcpy(out,o.data(),o.size());return (int)o.size();}
int MultiByteToWideChar(unsigned,DWORD,LPCSTR s,int,LPWSTR out,int cap){std::u16string w=w16(s);if(!cap)return (int)w.size()+1;memcpy(out,w.c_str(),(w.size()+1)*2);return (int)w.size()+1;}
DWORD GetEnvironmentVariableW(LPCWSTR n,LPWSTR out,DWORD cap){if(u8(n)=="LOCALAPPDATA"){const wchar_t*v=L"C:\\Local";wcscpy(out,v);return 8;}return 0;}
DWORD GetCurrentProcessId(void){return 4242;}
DWORD GetCurrentThreadId(void){return 1;}
void GetLocalTime(SYSTEMTIME*t){time_t now=time(nullptr);struct tm*lt=localtime(&now);t->wYear=lt->tm_year+1900;t->wMonth=lt->tm_mon+1;t->wDay=lt->tm_mday;t->wHour=lt->tm_hour;t->wMinute=lt->tm_min;t->wSecond=lt->tm_sec;t->wMilliseconds=0;t->wDayOfWeek=0;}
LPVOID VirtualAlloc(LPVOID want,SIZE_T n,DWORD,DWORD){void*p=mmap(want,(n+0xfff)&~(SIZE_T)0xfff,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|(want?MAP_FIXED_NOREPLACE:0),-1,0);return p==MAP_FAILED?nullptr:p;}
BOOL VirtualFree(LPVOID p,SIZE_T,DWORD){(void)p;return 1;}
BOOL VirtualProtect(LPVOID,SIZE_T,DWORD,DWORD*old){*old=PAGE_EXECUTE_READWRITE;return 1;}
BOOL FlushInstructionCache(HANDLE,LPCVOID,SIZE_T){return 1;}
HANDLE GetCurrentProcess(void){return (HANDLE)(intptr_t)-1;}
static std::atomic<unsigned long long> tickNow{100000};
BOOL QueryPerformanceFrequency(LARGE_INTEGER*f){f->QuadPart=1000000;return 1;}
BOOL QueryPerformanceCounter(LARGE_INTEGER*c){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);c->QuadPart=(LONGLONG)t.tv_sec*1000000+t.tv_nsec/1000;return 1;}
ULONGLONG GetTickCount64(void){return tickNow;}
static std::mutex srw;void AcquireSRWLockExclusive(PSRWLOCK){srw.lock();}void ReleaseSRWLockExclusive(PSRWLOCK){srw.unlock();}void AcquireSRWLockShared(PSRWLOCK){}void ReleaseSRWLockShared(PSRWLOCK){}
struct Start{LPTHREAD_START_ROUTINE f;LPVOID a;};static void*runner(void*p){Start s=*(Start*)p;delete(Start*)p;s.f(s.a);return nullptr;}
static std::vector<pthread_t> threads;
HANDLE CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE f,LPVOID a,DWORD,LPDWORD){pthread_t t;if(pthread_create(&t,nullptr,runner,new Start{f,a}))return nullptr;threads.push_back(t);return (HANDLE)1;}
void Sleep(DWORD ms){usleep(ms*10);tickNow+=ms;}
DWORD GetLastError(void){return 0;}void SetLastError(DWORD){}
int MessageBoxW(HWND,LPCWSTR,LPCWSTR,unsigned){return 1;}
DWORD TlsAlloc(void){return 0;}BOOL TlsFree(DWORD){return 1;}LPVOID TlsGetValue(DWORD){return nullptr;}BOOL TlsSetValue(DWORD,LPVOID){return 1;}
BOOL SetFilePointerEx(HANDLE h,LARGE_INTEGER pos,PLARGE_INTEGER out,DWORD method){off_t r=lseek(((File*)h)->fd,(off_t)pos.QuadPart,method==FILE_BEGIN?SEEK_SET:method==1?SEEK_CUR:SEEK_END);if(r<0)return 0;if(out)out->QuadPart=r;return 1;}BOOL FlushFileBuffers(HANDLE){++flush_calls;return 1;}
NTSTATUS BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE*a,LPCWSTR,LPCWSTR,unsigned long){*a=(void*)1;return 0;}
NTSTATUS BCryptCreateHash(BCRYPT_ALG_HANDLE,BCRYPT_HASH_HANDLE*h,unsigned char*,unsigned long,unsigned char*,unsigned long,unsigned long){*h=(void*)1;return 0;}
NTSTATUS BCryptHashData(BCRYPT_HASH_HANDLE,unsigned char*,unsigned long,unsigned long){return 0;}
NTSTATUS BCryptFinishHash(BCRYPT_HASH_HANDLE,unsigned char*out,unsigned long n,unsigned long){memcpy(out,icDigest,n);return 0;}
NTSTATUS BCryptDestroyHash(BCRYPT_HASH_HANDLE){return 0;}
NTSTATUS BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE,unsigned long){return 0;}
}
