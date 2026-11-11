// 리눅스 검사용 가짜 윈도우/게임 환경
#pragma once
#include <windows.h>
#include <array>
#include <bcrypt.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
typedef uint8_t U8; typedef uint16_t U16; typedef uint32_t U32; typedef int32_t I32; typedef uintptr_t UPTR;
extern "C" {
#include "../src/mod_api.h"
}
namespace mock {
extern U8 *game;                       // 0x29ff000 크기 가짜 게임 이미지 (실행 가능)
extern std::map<std::u16string, HMODULE> modules;
extern std::map<std::u16string, std::array<U8,32>> digests; // 파일 이름 끝부분 -> 해시 결과
extern std::map<std::string, void *> procs;                 // GetProcAddress 결과
extern std::map<std::u16string, unsigned> profile_ints;     // "파일끝|키" -> 값
extern std::vector<std::u16string> existing_files;
extern std::atomic<uint64_t> tick;
extern std::atomic<unsigned> readable_calls;
extern std::atomic<unsigned> createfile_calls,flush_calls;   // R20: 파일 열기·디스크 동기화 횟수
extern std::vector<std::string> logs;
extern S14_CoreAPI api;
void init();
U8 *alloc_exec(size_t n, UPTR near = 0);
void thunk(U8 *at, void *target);      // movabs rax,target; jmp rax
void native(U32 rva, void *target);    // 게임 함수 자리에 가짜 함수 연결
std::u16string w(const wchar_t *s);
std::array<U8,32> hex(const char *h);
void check(bool ok, const char *what);
int finish();
}
