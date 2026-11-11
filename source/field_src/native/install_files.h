#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <string>
#include "install_contract.h"
namespace fl_install {
inline bool path_exists(const std::wstring &p) {
 DWORD a=GetFileAttributesW(p.c_str());
 if(a!=INVALID_FILE_ATTRIBUTES)return true;
 DWORD error=GetLastError();
 // An inaccessible name is occupied, never a vacant slot.
 return error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND;
}
inline bool regular_file(const std::wstring &p) {
 DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));
}
inline bool plain_directory(const std::wstring &p) {
 DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_DIRECTORY)&&!(a&FILE_ATTRIBUTE_REPARSE_POINT);
}
inline bool hash_file(const std::wstring &path,unsigned char out[32]) {
 HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;bool ok=false;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)>=0&&BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)>=0) {
  unsigned char b[65536];DWORD n=0;ok=true;
  for(;;){if(!ReadFile(f,b,sizeof b,&n,NULL)){ok=false;break;}if(!n)break;if(BCryptHashData(hash,b,n,0)<0){ok=false;break;}}
  if(ok)ok=BCryptFinishHash(hash,out,32,0)>=0;
 }
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
inline bool matches(const std::wstring &p,const unsigned char hash[32]) {
 unsigned char h[32];return regular_file(p)&&hash_file(p,h)&&!std::memcmp(hash,h,32);
}
inline bool read_manifest(const std::wstring &directory,Manifest &m) {
 if(!plain_directory(directory))return false;
 std::wstring path=directory+L"\\install.dat";if(!regular_file(path))return false;
 HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return false;
 DWORD n=0;LARGE_INTEGER z={};
 bool ok=GetFileSizeEx(f,&z)&&z.QuadPart==sizeof m&&ReadFile(f,&m,sizeof m,&n,NULL)&&n==sizeof m;
 CloseHandle(f);return ok&&valid_manifest(m);
}
}
