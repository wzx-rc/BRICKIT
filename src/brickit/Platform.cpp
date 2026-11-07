#include "Platform.h"
#include <cstdint>
#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#endif

namespace brickit::plat {

static std::wstring g_dataDir;

std::string toUtf8(const std::wstring& w) {
    std::string s;
    for (wchar_t wc : w) {
        uint32_t c = (uint32_t)wc;
        if (c < 0x80) s += (char)c;
        else if (c < 0x800) { s += (char)(0xC0 | (c >> 6)); s += (char)(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { s += (char)(0xE0 | (c >> 12)); s += (char)(0x80 | ((c >> 6) & 0x3F)); s += (char)(0x80 | (c & 0x3F)); }
        else { s += (char)(0xF0 | (c >> 18)); s += (char)(0x80 | ((c >> 12) & 0x3F)); s += (char)(0x80 | ((c >> 6) & 0x3F)); s += (char)(0x80 | (c & 0x3F)); }
    }
    return s;
}

std::wstring fromUtf8(const std::string& s) {
    std::wstring w;
    for (size_t i = 0; i < s.size();) {
        uint32_t c = (uint8_t)s[i];
        int n = c < 0x80 ? 0 : c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
        if (n) c &= (0x3F >> n);
        ++i;
        for (int k = 0; k < n && i < s.size(); ++k, ++i) c = (c << 6) | ((uint8_t)s[i] & 0x3F);
        w += (wchar_t)c;
    }
    return w;
}

std::string nativePath(const std::wstring& w) {
    std::string s = toUtf8(w);
#ifndef _WIN32
    for (char& c : s)
        if (c == '\\') c = '/';
#endif
    return s;
}

bool fileExists(const std::wstring& path) {
#ifdef _WIN32
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(nativePath(path).c_str(), &st) == 0;
#endif
}

bool makeDir(const std::wstring& path) {
#ifdef _WIN32
    return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(nativePath(path).c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

void setDataDir(const std::wstring& dir) { g_dataDir = dir; }

std::wstring dataDir() {
    std::wstring d = g_dataDir;
#ifdef _WIN32
    if (d.empty()) {
        PWSTR prof = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &prof)) && prof) d = std::wstring(prof) + L"\\BrickIt";
        if (prof) CoTaskMemFree(prof);
    }
#else
    if (d.empty()) {
        const char* home = std::getenv("HOME");
        if (home) d = fromUtf8(home) + L"/BrickIt";
    }
#endif
    if (!d.empty()) makeDir(d);
    return d;
}

} // namespace brickit::plat
