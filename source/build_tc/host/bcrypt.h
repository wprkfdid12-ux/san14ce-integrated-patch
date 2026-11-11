#pragma once
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void *BCRYPT_ALG_HANDLE; typedef void *BCRYPT_HASH_HANDLE;
#define BCRYPT_SHA256_ALGORITHM L"SHA256"
NTSTATUS BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE*,LPCWSTR,LPCWSTR,unsigned long);
NTSTATUS BCryptCreateHash(BCRYPT_ALG_HANDLE,BCRYPT_HASH_HANDLE*,unsigned char*,unsigned long,unsigned char*,unsigned long,unsigned long);
NTSTATUS BCryptHashData(BCRYPT_HASH_HANDLE,unsigned char*,unsigned long,unsigned long);
NTSTATUS BCryptFinishHash(BCRYPT_HASH_HANDLE,unsigned char*,unsigned long,unsigned long);
NTSTATUS BCryptDestroyHash(BCRYPT_HASH_HANDLE); NTSTATUS BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE,unsigned long);
#ifdef __cplusplus
}
#endif
