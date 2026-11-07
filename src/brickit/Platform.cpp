#include "Platform.h"
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
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

#ifdef _WIN32
static HWND g_window = nullptr;
void setWindow(void* w) { g_window = (HWND)w; }

void errorBox(const std::string& title, const std::string& text) { MessageBoxA(g_window, text.c_str(), title.c_str(), MB_ICONERROR); }

void confirm(const std::wstring& title, const std::wstring& text, std::function<void()> onOk) {
    if (MessageBoxW(g_window, text.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONQUESTION) == IDOK && onOk) onOk();
}

static void fileDialog(bool save, const wchar_t* filter, const wchar_t* defExt, const std::wstring& suggested,
                       std::function<void(const std::wstring&)>& done) {
    wchar_t file[MAX_PATH] = L"";
    if (!suggested.empty()) wcsncpy_s(file, suggested.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn = {sizeof(ofn)};
    ofn.hwndOwner = g_window;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defExt;
    ofn.Flags = save ? (OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST) : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
    const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (ok && done) done(std::wstring(file));
}

void openFileDialog(const wchar_t* filter, const wchar_t* defExt, std::function<void(const std::wstring&)> done,
                    const std::wstring& initial) {
    fileDialog(false, filter, defExt, initial, done);
}
void saveFileDialog(const wchar_t* filter, const wchar_t* defExt, const std::wstring& suggested,
                    std::function<void(const std::wstring&)> done) {
    fileDialog(true, filter, defExt, suggested, done);
}
#elif !defined(__APPLE__)
// Linux test build: no dialogs (scripted runs give the paths on the command line).
void setWindow(void*) {}
void errorBox(const std::string& title, const std::string& text) { fprintf(stderr, "%s: %s\n", title.c_str(), text.c_str()); }
void confirm(const std::wstring&, const std::wstring&, std::function<void()> onOk) { if (onOk) onOk(); }
void openFileDialog(const wchar_t*, const wchar_t*, std::function<void(const std::wstring&)>, const std::wstring&) {}
void saveFileDialog(const wchar_t*, const wchar_t*, const std::wstring&, std::function<void(const std::wstring&)>) {}
#endif

} // namespace brickit::plat

#ifndef _WIN32
#include "WinCompat.h"
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>
#include <vector>

namespace brickit::wincompat {

struct Find {
    std::vector<std::pair<std::wstring, bool>> entries;   // name, directory
    size_t next = 0;
};

static void fill(const Find& f, WIN32_FIND_DATAW* fd) {
    const auto& e = f.entries[f.next];
    wcsncpy_s(fd->cFileName, e.first.c_str(), _TRUNCATE);
    fd->dwFileAttributes = e.second ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

HANDLE findFirst(const wchar_t* pattern, WIN32_FIND_DATAW* fd) {
    const std::string p = plat::nativePath(pattern);
    const size_t s = p.find_last_of('/');
    const std::string dir = s == std::string::npos ? "." : p.substr(0, s), mask = s == std::string::npos ? p : p.substr(s + 1);
    DIR* d = opendir(dir.c_str());
    if (!d) return INVALID_HANDLE_VALUE;
    Find* f = new Find;
    while (dirent* e = readdir(d)) {
        if (fnmatch(mask.c_str(), e->d_name, FNM_CASEFOLD) != 0) continue;
        struct stat st;
        const bool isDir = stat((dir + "/" + e->d_name).c_str(), &st) == 0 && S_ISDIR(st.st_mode);
        f->entries.push_back({plat::fromUtf8(e->d_name), isDir});
    }
    closedir(d);
    if (f->entries.empty()) { delete f; return INVALID_HANDLE_VALUE; }
    fill(*f, fd);
    return f;
}

BOOL findNext(HANDLE h, WIN32_FIND_DATAW* fd) {
    Find* f = (Find*)h;
    if (!f || ++f->next >= f->entries.size()) return FALSE;
    fill(*f, fd);
    return TRUE;
}

BOOL findClose(HANDLE h) {
    delete (Find*)h;
    return TRUE;
}

BOOL copyFile(const wchar_t* from, const wchar_t* to, BOOL failIfExists) {
    if (failIfExists && plat::fileExists(to)) return FALSE;
    FILE* a = std::fopen(plat::nativePath(from).c_str(), "rb");
    if (!a) return FALSE;
    FILE* b = std::fopen(plat::nativePath(to).c_str(), "wb");
    if (!b) { std::fclose(a); return FALSE; }
    char buf[65536];
    size_t n;
    bool ok = true;
    while ((n = std::fread(buf, 1, sizeof(buf), a)) > 0) ok &= std::fwrite(buf, 1, n, b) == n;
    std::fclose(a);
    std::fclose(b);
    return ok ? TRUE : FALSE;
}

BOOL deleteFile(const wchar_t* path) { return unlink(plat::nativePath(path).c_str()) == 0 ? TRUE : FALSE; }

DWORD fileAttributes(const wchar_t* path) {
    struct stat st;
    if (stat(plat::nativePath(path).c_str(), &st) != 0) return INVALID_FILE_ATTRIBUTES;
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

} // namespace brickit::wincompat
#endif
