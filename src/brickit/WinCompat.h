// BrickIt - the small Win32 / MSVC calls of the shared code, provided on the other systems (iPad, Linux test build).
// Windows itself never includes this file: the PC build keeps calling the real API, so its behaviour is unchanged.
// Only plain file, time, text and environment calls are here; windows, dialogs, rendering and images go through
// Platform.h / Gfx.h instead.
#pragma once
#ifndef _WIN32
#include "Platform.h"
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <string>
#include <thread>
#include <unistd.h>

using BOOL = int;
using UINT = unsigned int;
using DWORD = uint32_t;
using HANDLE = void*;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif
#define _TRUNCATE ((size_t)-1)
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10u
#define FILE_ATTRIBUTE_NORMAL 0x80u
#define CP_UTF8 65001u

union LARGE_INTEGER { int64_t QuadPart; };

struct WIN32_FIND_DATAW {
    DWORD dwFileAttributes = 0;
    wchar_t cFileName[MAX_PATH] = {};
};

struct SYSTEMTIME {
    uint16_t wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};

namespace brickit::wincompat {
HANDLE findFirst(const wchar_t* pattern, WIN32_FIND_DATAW* fd);
BOOL findNext(HANDLE h, WIN32_FIND_DATAW* fd);
BOOL findClose(HANDLE h);
BOOL copyFile(const wchar_t* from, const wchar_t* to, BOOL failIfExists);
BOOL deleteFile(const wchar_t* path);
DWORD fileAttributes(const wchar_t* path);
} // namespace brickit::wincompat

inline HANDLE FindFirstFileW(const wchar_t* p, WIN32_FIND_DATAW* fd) { return brickit::wincompat::findFirst(p, fd); }
inline BOOL FindNextFileW(HANDLE h, WIN32_FIND_DATAW* fd) { return brickit::wincompat::findNext(h, fd); }
inline BOOL FindClose(HANDLE h) { return brickit::wincompat::findClose(h); }
inline BOOL CopyFileW(const wchar_t* a, const wchar_t* b, BOOL failIfExists) { return brickit::wincompat::copyFile(a, b, failIfExists); }
inline BOOL DeleteFileW(const wchar_t* p) { return brickit::wincompat::deleteFile(p); }
inline BOOL CreateDirectoryW(const wchar_t* p, void*) { return brickit::plat::makeDir(p) ? TRUE : FALSE; }
inline DWORD GetFileAttributesW(const wchar_t* p) { return brickit::wincompat::fileAttributes(p); }

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* f) { f->QuadPart = 1000000000LL; return TRUE; }
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* c) {
    c->QuadPart = (int64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return TRUE;
}
inline uint64_t GetTickCount64() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void Sleep(DWORD ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline void GetLocalTime(SYSTEMTIME* st) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    st->wYear = (uint16_t)(tm.tm_year + 1900); st->wMonth = (uint16_t)(tm.tm_mon + 1); st->wDayOfWeek = (uint16_t)tm.tm_wday;
    st->wDay = (uint16_t)tm.tm_mday; st->wHour = (uint16_t)tm.tm_hour; st->wMinute = (uint16_t)tm.tm_min; st->wSecond = (uint16_t)tm.tm_sec;
    st->wMilliseconds = (uint16_t)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
}
inline DWORD GetEnvironmentVariableA(const char* name, char* buf, DWORD size) {
    const char* v = std::getenv(name);
    if (!v) return 0;
    const size_t n = std::strlen(v);
    if (n + 1 > size) return (DWORD)(n + 1);
    std::memcpy(buf, v, n + 1);
    return (DWORD)n;
}

// UTF-8 <-> wide text (only CP_UTF8 is used by BrickIt)
inline int MultiByteToWideChar(UINT, DWORD, const char* s, int n, wchar_t* out, int outSize) {
    const std::wstring w = brickit::plat::fromUtf8(std::string(s, n < 0 ? std::strlen(s) : (size_t)n));
    if (!out || outSize == 0) return (int)w.size();
    const int k = std::min((int)w.size(), outSize);
    std::wmemcpy(out, w.data(), k);
    return k;
}
inline int WideCharToMultiByte(UINT, DWORD, const wchar_t* w, int n, char* out, int outSize, const char*, BOOL*) {
    const std::string s = brickit::plat::toUtf8(std::wstring(w, n < 0 ? std::wcslen(w) : (size_t)n));
    if (!out || outSize == 0) return (int)s.size();
    const int k = std::min((int)s.size(), outSize);
    std::memcpy(out, s.data(), k);
    return k;
}

// MSVC "secure" string functions
template <size_t N>
inline int wcsncpy_s(wchar_t (&dst)[N], const wchar_t* src, size_t count) {
    size_t n = std::wcslen(src);
    if (count != _TRUNCATE && count < n) n = count;
    if (n >= N) n = N - 1;
    std::wmemcpy(dst, src, n);
    dst[n] = 0;
    return 0;
}
template <size_t N>
inline int strncpy_s(char (&dst)[N], const char* src, size_t count) {
    size_t n = std::strlen(src);
    if (count != _TRUNCATE && count < n) n = count;
    if (n >= N) n = N - 1;
    std::memcpy(dst, src, n);
    dst[n] = 0;
    return 0;
}
template <size_t N>
inline int strcpy_s(char (&dst)[N], const char* src) { return strncpy_s(dst, src, _TRUNCATE); }
template <size_t N, typename... A>
inline int sprintf_s(char (&dst)[N], const char* fmt, A... a) { return std::snprintf(dst, N, fmt, a...); }
#define sscanf_s sscanf
#define fscanf_s fscanf
#define swscanf_s swscanf
inline int gmtime_s(std::tm* out, const std::time_t* t) { return gmtime_r(t, out) ? 0 : 1; }
inline int localtime_s(std::tm* out, const std::time_t* t) { return localtime_r(t, out) ? 0 : 1; }
inline DWORD GetCurrentDirectoryW(DWORD size, wchar_t* buf) {
    char b[4096];
    if (!getcwd(b, sizeof(b))) return 0;
    const std::wstring w = brickit::plat::fromUtf8(b);
    if (w.size() + 1 > size) return (DWORD)(w.size() + 1);
    std::wmemcpy(buf, w.c_str(), w.size() + 1);
    return (DWORD)w.size();
}
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#include <strings.h>
#endif
