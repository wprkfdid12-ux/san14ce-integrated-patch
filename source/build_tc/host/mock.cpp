#include <array>
#include "mock.h"
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mutex>
namespace mock {
U8 *game; std::map<std::u16string, HMODULE> modules; std::map<std::u16string, std::array<U8,32>> digests;
std::map<std::string, void *> procs; std::map<std::u16string, unsigned> profile_ints; std::vector<std::u16string> existing_files;
std::atomic<uint64_t> tick{100000}; std::atomic<unsigned> readable_calls{0}; std::atomic<unsigned> createfile_calls{0},flush_calls{0}; std::vector<std::string> logs; S14_CoreAPI api; S14_Resolved resolved;
static std::mutex log_lock; static int failures = 0, passes = 0;
std::u16string w(const wchar_t *s) { std::u16string r; while (s && *s) r.push_back((char16_t)*s++); return r; }
std::array<U8,32> hex(const char *h) { std::array<U8,32> d{}; for (int i = 0; i < 32; i++) sscanf(h + 2 * i, "%2hhx", &d[i]); return d; }
void check(bool ok, const char *what) { if (ok) { ++passes; printf("  통과: %s\n", what); } else { ++failures; printf("  실패: %s\n", what); } }
int finish() { printf("결과: 통과 %d / 실패 %d\n", passes, failures); return failures ? 1 : 0; }
U8 *alloc_exec(size_t n, UPTR near) {
  n = (n + 0xfff) & ~(size_t)0xfff;
  if (near) {
    for (UPTR off = 0x10000; off < 0x40000000; off += 0x10000) {
      for (int s = 0; s < 2; s++) {
        UPTR want = s ? (near & ~(UPTR)0xffff) - off : (near & ~(UPTR)0xffff) + off;
        void *p = mmap((void *)want, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        if (p != MAP_FAILED) return (U8 *)p;
      }
    }
    return nullptr;
  }
  void *p = mmap(nullptr, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return p == MAP_FAILED ? nullptr : (U8 *)p;
}
void thunk(U8 *at, void *target) { at[0] = 0x48; at[1] = 0xb8; memcpy(at + 2, &target, 8); at[10] = 0xff; at[11] = 0xe0; }
void native(U32 rva, void *target) { thunk(game + rva, target); }
struct Region { UPTR lo, hi; };
static std::vector<Region> regions;
static I32 Readable(const void *p, UPTR n) {
  ++readable_calls; UPTR a = (UPTR)p;
  for (auto &r : regions) if (a >= r.lo && a + n <= r.hi) return 1;
  // 힙/정적 영역은 주소 공간 확인 대신 mincore로 판정
  if (!a) return 0;
  UPTR page = a & ~(UPTR)0xfff, end = (a + n + 0xfff) & ~(UPTR)0xfff; unsigned char v;
  for (UPTR q = page; q < end; q += 0x1000) if (mincore((void *)q, 0x1000, &v)) return 0;
  return 1;
}
static I32 InstallHooks(const char *id, const S14_Hook *h, U32 n) {
  for (U32 i = 0; i < n; i++) if (memcmp(h[i].target, h[i].expected, h[i].length)) { std::lock_guard<std::mutex> g(log_lock); logs.push_back(std::string("InstallHooks 거부: ") + id); return 0; }
  for (U32 i = 0; i < n; i++) memcpy(h[i].target, h[i].replacement, h[i].length);
  return 1;
}
static U8 *MakeRelay(const void *target, const void *dest) { U8 *r = alloc_exec(0x1000, (UPTR)target); if (!r) return nullptr; r[0] = 0xff; r[1] = 0x25; memset(r + 2, 0, 4); memcpy(r + 6, &dest, 8); return r; }
static void FreeRelay(void *) {}
static void Log(const char *s) { std::lock_guard<std::mutex> g(log_lock); logs.push_back(s); printf("    [기록] %s\n", s); }
void add_region(void *p, size_t n) { regions.push_back({(UPTR)p, (UPTR)p + n}); }
void init() {
  game = (U8 *)mmap((void *)0x140000000, 0x29ff000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
  if (game == MAP_FAILED) { perror("game"); exit(2); }
  add_region(game, 0x29ff000);
  resolved = {}; resolved.size = sizeof resolved; resolved.abi = S14_ABI; resolved.layout_abi = 1; resolved.image_size = 0x29ff000; resolved.image = game;
  api = {}; api.size = sizeof api; api.abi = S14_ABI; api.resolved = &resolved; api.Readable = Readable; api.InstallHooks = InstallHooks; api.MakeRelay = MakeRelay; api.FreeRelay = FreeRelay; api.Log = Log;
}
}
using namespace mock;
static thread_local std::u16string hashing;
extern "C" {
static bool ends_with(const std::u16string &s, const std::u16string &t) { return s.size() >= t.size() && !s.compare(s.size() - t.size(), t.size(), t); }
static std::u16string lower(std::u16string s) { for (auto &c : s) if (c >= 'A' && c <= 'Z') c += 32; return s; }
HMODULE GetModuleHandleW(LPCWSTR n) { if (!n) return (HMODULE)game; auto k = lower(w(n)); for (auto &m : modules) if (lower(m.first) == k) return m.second; return nullptr; }
std::atomic<int> loadlibrary_calls{0};
HMODULE LoadLibraryExW(LPCWSTR n, HANDLE, DWORD) { loadlibrary_calls++; auto k = lower(w(n)); for (auto &m : modules) if (ends_with(k, lower(m.first))) return m.second; return nullptr; }
void *GetProcAddress(HMODULE, LPCSTR n) { auto i = procs.find(n); return i == procs.end() ? nullptr : i->second; }
DWORD GetModuleFileNameW(HMODULE m, LPWSTR out, DWORD cap) {
  std::u16string p = u"C:\\Game\\SAN14CE_KO.exe";
  for (auto &e : modules) if (e.second == m) p = u"C:\\Game\\" + e.first;
  if (p.size() + 1 > cap) return 0; for (size_t i = 0; i < p.size(); i++) out[i] = p[i]; out[p.size()] = 0; return (DWORD)p.size();
}
struct File { std::u16string path; bool done; };
HANDLE CreateFileW(LPCWSTR n, DWORD, DWORD, void *, DWORD disp, DWORD, HANDLE) {
  ++createfile_calls; auto p = w(n); for (auto &d : digests) if (ends_with(lower(p), lower(d.first))) { hashing = d.first; return new File{p, false}; }
  if (disp == OPEN_ALWAYS || disp == CREATE_ALWAYS) return new File{p, false};   // R20: 새로 만드는 파일은 성공(쓰기는 세지만 보관하지 않음)
  return INVALID_HANDLE_VALUE;
}
BOOL ReadFile(HANDLE, LPVOID, DWORD, LPDWORD n, void *) { *n = 0; return 1; }
BOOL WriteFile(HANDLE, LPCVOID, DWORD s, LPDWORD n, void *) { *n = s; return 1; }
BOOL CloseHandle(HANDLE h) { if (h && h != INVALID_HANDLE_VALUE && (UPTR)h > 0x1000) {} return 1; }
BOOL GetFileSizeEx(HANDLE, PLARGE_INTEGER s) { s->QuadPart = 0; return 1; }
BOOL SetFilePointerEx(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD) { return 1; }
BOOL FlushFileBuffers(HANDLE) { ++flush_calls; return 1; }
BOOL CreateDirectoryW(LPCWSTR, void *) { return 1; }
DWORD GetFileAttributesW(LPCWSTR n) { auto p = lower(w(n)); for (auto &e : existing_files) if (ends_with(p, lower(e))) return 0x20; return INVALID_FILE_ATTRIBUTES; }
unsigned GetPrivateProfileIntW(LPCWSTR s, LPCWSTR k, int def, LPCWSTR f) {
  auto p = lower(w(f)); auto key = lower(w(k));
  for (auto &e : profile_ints) { auto bar = e.first.find(u'|'); if (ends_with(p, lower(e.first.substr(0, bar))) && lower(e.first.substr(bar + 1)) == key) return e.second; }
  return def;
}
BOOL WritePrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR) { return 1; }
LPVOID VirtualAlloc(LPVOID want, SIZE_T n, DWORD, DWORD) {
  void *p = mmap(want, (n + 0xfff) & ~(SIZE_T)0xfff, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | (want ? MAP_FIXED_NOREPLACE : 0), -1, 0);
  return p == MAP_FAILED ? nullptr : p;
}
BOOL VirtualFree(LPVOID p, SIZE_T, DWORD) { munmap(p, 0x1000); return 1; }
BOOL VirtualProtect(LPVOID, SIZE_T, DWORD np, DWORD *old) { *old = PAGE_EXECUTE_READWRITE; return 1; }
BOOL FlushInstructionCache(HANDLE, LPCVOID, SIZE_T) { return 1; }
HANDLE GetCurrentProcess(void) { return (HANDLE)(intptr_t)-1; }
static pthread_key_t keys[64]; static int key_count;
DWORD TlsAlloc(void) { if (key_count == 64) return TLS_OUT_OF_INDEXES; pthread_key_create(&keys[key_count], nullptr); return key_count++; }
BOOL TlsFree(DWORD) { return 1; }
LPVOID TlsGetValue(DWORD i) { return i < 64 ? pthread_getspecific(keys[i]) : nullptr; }
BOOL TlsSetValue(DWORD i, LPVOID v) { return i < 64 && !pthread_setspecific(keys[i], v); }
ULONGLONG GetTickCount64(void) { return tick.load(); }
void AcquireSRWLockExclusive(PSRWLOCK l) { static std::mutex m; (void)l; m.lock(); m.unlock(); }
void ReleaseSRWLockExclusive(PSRWLOCK) {}
void AcquireSRWLockShared(PSRWLOCK) {}
void ReleaseSRWLockShared(PSRWLOCK) {}
struct Start { LPTHREAD_START_ROUTINE f; LPVOID a; };
static void *runner(void *p) { Start s = *(Start *)p; delete (Start *)p; s.f(s.a); return nullptr; }
std::vector<pthread_t> threads;
HANDLE CreateThread(void *, SIZE_T, LPTHREAD_START_ROUTINE f, LPVOID a, DWORD, LPDWORD) { pthread_t t; if (pthread_create(&t, nullptr, runner, new Start{f, a})) return nullptr; threads.push_back(t); return (HANDLE)1; }
void Sleep(DWORD ms) { usleep(ms * 20); tick += ms; } // 검사에서는 50배 빠르게 진행
DWORD GetLastError(void) { return 0; }
void SetLastError(DWORD) {}
int MessageBoxW(HWND, LPCWSTR, LPCWSTR, unsigned) { return 1; }
wchar_t *wcscpy(wchar_t *__restrict d, const wchar_t *__restrict s) noexcept { wchar_t *r = d; while ((*d++ = *s++)) {} return r; }
NTSTATUS BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *a, LPCWSTR, LPCWSTR, unsigned long) { *a = (void *)1; return 0; }
NTSTATUS BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *h, unsigned char *, unsigned long, unsigned char *, unsigned long, unsigned long) { *h = (void *)1; return 0; }
NTSTATUS BCryptHashData(BCRYPT_HASH_HANDLE, unsigned char *, unsigned long, unsigned long) { return 0; }
NTSTATUS BCryptFinishHash(BCRYPT_HASH_HANDLE, unsigned char *out, unsigned long n, unsigned long) { memcpy(out, digests[hashing].data(), n); return 0; }
NTSTATUS BCryptDestroyHash(BCRYPT_HASH_HANDLE) { return 0; }
NTSTATUS BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE, unsigned long) { return 0; }
}
