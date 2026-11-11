#pragma once
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void *BCRYPT_ALG_HANDLE;
typedef void *BCRYPT_HASH_HANDLE;
#define BCRYPT_SHA256_ALGORITHM L"SHA256"
__declspec(dllimport) NTSTATUS WINAPI BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, unsigned long);
__declspec(dllimport) NTSTATUS WINAPI BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, unsigned char *, unsigned long, unsigned char *, unsigned long, unsigned long);
__declspec(dllimport) NTSTATUS WINAPI BCryptHashData(BCRYPT_HASH_HANDLE, unsigned char *, unsigned long, unsigned long);
__declspec(dllimport) NTSTATUS WINAPI BCryptFinishHash(BCRYPT_HASH_HANDLE, unsigned char *, unsigned long, unsigned long);
__declspec(dllimport) NTSTATUS WINAPI BCryptDestroyHash(BCRYPT_HASH_HANDLE);
__declspec(dllimport) NTSTATUS WINAPI BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE, unsigned long);
#ifdef __cplusplus
}
#endif
