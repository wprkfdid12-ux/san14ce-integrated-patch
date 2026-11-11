// Execute the production upgrade backend on real temporary files. Only Win32
// filesystem/hash primitives are supplied by this POSIX host adapter.
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <openssl/sha.h>
#include "../install_contract.h"
#include "../install_upgrade.h"
namespace fs=std::filesystem;
using std::wstring;using fl_install::Manifest;
using DWORD=unsigned;using PUCHAR=unsigned char *;using HANDLE=std::ifstream *;using BCRYPT_ALG_HANDLE=void *;
struct Hash { std::vector<unsigned char> data; };using BCRYPT_HASH_HANDLE=Hash *;
struct LARGE_INTEGER {long long QuadPart;};
constexpr unsigned GENERIC_READ=1,FILE_SHARE_READ=1,OPEN_EXISTING=1,MOVEFILE_REPLACE_EXISTING=1,MOVEFILE_WRITE_THROUGH=2;
#define INVALID_HANDLE_VALUE nullptr
#define TRUE 1
#define BCRYPT_SHA256_ALGORITHM L"SHA256"
static fs::path path(wstring p){for(auto &c:p)if(c==L'\\')c=L'/';return fs::path(p);}
static int fail_step=-1,operations=0;
static bool write_allowed(){return operations++!=fail_step;}
static bool exists(const wstring &p){return fs::exists(path(p));}
static bool regular(const wstring &p){return fs::is_regular_file(path(p))&&!fs::is_symlink(path(p));}
static bool write_new(const wstring &p,const unsigned char *b,size_t n){if(exists(p)||!write_allowed())return false;std::ofstream f(path(p),std::ios::binary);f.write((const char *)b,n);return f.good();}
static bool hash_file(const wstring &p,unsigned char out[32]){if(!regular(p))return false;std::ifstream f(path(p),std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});SHA256(b.data(),b.size(),out);return true;}
static bool matches(const wstring &p,const unsigned char h[32]){unsigned char actual[32];return hash_file(p,actual)&&!memcmp(h,actual,32);}
static HANDLE CreateFileW(const wchar_t *p,unsigned,unsigned,void *,unsigned,unsigned,void *){auto f=new std::ifstream(path(p),std::ios::binary);if(!*f){delete f;return nullptr;}return f;}
static bool GetFileSizeEx(HANDLE f,LARGE_INTEGER *s){f->seekg(0,std::ios::end);s->QuadPart=f->tellg();f->seekg(0);return s->QuadPart>=0;}
static bool ReadFile(HANDLE f,void *out,unsigned n,unsigned *got,void *){f->read((char *)out,n);*got=unsigned(f->gcount());return *got==n;}
static void CloseHandle(HANDLE f){delete f;}
static bool CopyFileW(const wchar_t *a,const wchar_t *b,int){if(!write_allowed())return false;std::error_code ec;return fs::copy_file(path(a),path(b),fs::copy_options::none,ec);}
static bool MoveFileExW(const wchar_t *a,const wchar_t *b,unsigned){if(!write_allowed())return false;std::error_code ec;fs::rename(path(a),path(b),ec);return !ec;}
static bool DeleteFileW(const wchar_t *p){if(!write_allowed())return false;return fs::remove(path(p));}
static int BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *a,const wchar_t *,void *,unsigned){*a=(void *)1;return 0;}
static int BCryptCreateHash(BCRYPT_ALG_HANDLE,BCRYPT_HASH_HANDLE *h,void *,unsigned,void *,unsigned,unsigned){*h=new Hash;return 0;}
static int BCryptHashData(BCRYPT_HASH_HANDLE h,PUCHAR p,unsigned n,unsigned){h->data.insert(h->data.end(),p,p+n);return 0;}
static int BCryptFinishHash(BCRYPT_HASH_HANDLE h,unsigned char *out,unsigned,unsigned){SHA256(h->data.data(),h->data.size(),out);return 0;}
static void BCryptDestroyHash(BCRYPT_HASH_HANDLE h){delete h;}
static void BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE,unsigned){}
namespace fl_install{static bool plain_directory(const wstring &p){return fs::is_directory(path(p))&&!fs::is_symlink(path(p));}}
static unsigned char fl_runtime[]={8,9,10,11},fl_runtime_sha256[32],proxy_bytes[]={12,13,14},proxy_hash[32];
struct Payload {unsigned char *bytes;size_t size;unsigned char *sha256;};static Payload fl_proxy_payloads[4];
static wstring self_path;
struct Disk {
 wstring dir,game;Manifest manifest={};bool active=false;
 bool running(){return active;}
 wstring path(unsigned i,unsigned slot){return i==3?game+L"/"+fl_install::slot_names[slot]:dir+L"/"+(i==0?L"runtime":i==1?L"setup":L"install.dat");}
 bool owned(){std::ifstream f(::path(dir+L"/install.dat"),std::ios::binary);f.read((char *)&manifest,sizeof manifest);return f.gcount()==sizeof manifest&&fl_install::valid_manifest(manifest);}
 bool verify(){for(unsigned i:{0u,1u,3u}){auto h=i==0?manifest.runtime:i==1?manifest.manager:manifest.proxy;if(exists(path(i,manifest.slot))&&!matches(path(i,manifest.slot),h))return false;}return true;}
};
#include "../install_upgrade_disk.h"
int main(){
 auto temp=fs::temp_directory_path()/"fl24_upgrade_actual_backend";fs::remove_all(temp);fs::create_directories(temp);
 SHA256(fl_runtime,sizeof fl_runtime,fl_runtime_sha256);SHA256(proxy_bytes,sizeof proxy_bytes,proxy_hash);for(auto &p:fl_proxy_payloads)p={proxy_bytes,sizeof proxy_bytes,proxy_hash};
 unsigned cases=0;
 for(int stop=-1;stop<28;++stop){auto root=temp/std::to_string(stop+1);fs::create_directories(root/"owned");Disk d;d.game=root.wstring();d.dir=(root/"owned").wstring();self_path=(root/"downloaded_setup").wstring();fail_step=-1;
  unsigned char old[]={1,2,3},manager[]={4,5,6,7};assert(write_new(self_path,manager,sizeof manager));
  memcpy(d.manifest.magic,fl_install::manifest_magic,16);d.manifest.revision=23;d.manifest.slot=2;
  SHA256(old,sizeof old,d.manifest.runtime);memcpy(d.manifest.proxy,d.manifest.runtime,32);memcpy(d.manifest.manager,d.manifest.runtime,32);
  for(unsigned i:{0u,1u,3u})assert(write_new(d.path(i,2),old,sizeof old));assert(write_new(d.path(2,2),(unsigned char *)&d.manifest,sizeof d.manifest));
  wstring foreign=d.game+L"/version.dll";assert(write_new(foreign,manager,sizeof manager));unsigned char foreign_hash[32];hash_file(foreign,foreign_hash);
  fail_step=stop;operations=0;UpgradeDisk first(d);auto result=fl_install::upgrade(first);assert(result==fl_install::installed||result==fl_install::failed);
  fail_step=-1;UpgradeDisk resume(d);assert(fl_install::upgrade(resume)==fl_install::installed);assert(d.owned()&&d.verify());assert(matches(d.path(0,2),fl_runtime_sha256)&&matches(d.path(3,2),proxy_hash)&&matches(foreign,foreign_hash));assert(!exists(d.dir+L"/upgrade.dat"));++cases;
  // A foreign changed file is neither replaced nor deleted on another attempt.
  {std::ofstream f(::path(d.path(0,2)),std::ios::binary|std::ios::trunc);f<<"foreign";}
  UpgradeDisk changed(d);assert(fl_install::upgrade(changed)==fl_install::changed);++cases;
 }
 fs::remove_all(temp);printf("PASS %u actual-backend cases: journal recovery, SHA256 ownership, atomic manifest-last activation, foreign-file protection\n",cases);
}
