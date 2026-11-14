#include "Platform.h"
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <winhttp.h>
#include <wincrypt.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")
#else
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#endif

namespace brickit::plat {

static std::wstring g_dataDir, g_reportsDir;

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
void setReportsDir(const std::wstring& dir) { g_reportsDir = dir; }

std::wstring reportsDir() {
    std::wstring d = g_reportsDir;
#ifdef _WIN32
    if (d.empty()) d = dataDir() + L"\\crashlogs";
#else
    if (d.empty()) d = dataDir() + L"/crashlogs";
#endif
    makeDir(d);
    return d;
}

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
void shareFile(const std::wstring&) {}
void setAppIcon(int which) {   // the resources 1 (A) and 2 (B) of brickit.rc, on the window (title bar, taskbar, Alt+Tab)
    if (!g_window) return;
    HINSTANCE hi = GetModuleHandleW(nullptr);
    const LPCWSTR id = MAKEINTRESOURCEW(which == 1 ? 2 : 1);
    HICON big = (HICON)LoadImageW(hi, id, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    HICON smallIco = (HICON)LoadImageW(hi, id, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (big) SendMessageW(g_window, WM_SETICON, ICON_BIG, (LPARAM)big);
    if (smallIco) SendMessageW(g_window, WM_SETICON, ICON_SMALL, (LPARAM)smallIco);   // (« small » is a macro of the Windows headers)
}

// ---- network: WinHTTP (the system's TLS and proxy), one session for the app
HttpReply http(const char* method, const std::string& url, const std::vector<std::string>& headers, const std::string& body) {
    HttpReply r;
    static HINTERNET session = WinHttpOpen(L"BrickIt/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) { r.error = "WinHTTP indisponible"; return r; }
    const std::wstring wurl = fromUtf8(url);
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    static thread_local wchar_t host[256], path[4096], extra[4096];
    host[0] = path[0] = extra[0] = 0;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 4096;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 4096;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) { r.error = "adresse invalide"; return r; }
    HINTERNET con = WinHttpConnect(session, host, uc.nPort, 0);
    if (!con) { r.error = "connexion impossible"; return r; }
    const std::wstring target = std::wstring(path) + extra;
    HINTERNET req = WinHttpOpenRequest(con, fromUtf8(method).c_str(), target.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                       uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!req) { WinHttpCloseHandle(con); r.error = "requête impossible"; return r; }
    WinHttpSetTimeouts(req, 10000, 10000, 20000, 30000);
    std::wstring hs;
    for (const std::string& hd : headers) hs += fromUtf8(hd) + L"\r\n";
    BOOL ok = WinHttpSendRequest(req, hs.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : hs.c_str(), (DWORD)-1L,
                                 body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (ok) ok = WinHttpReceiveResponse(req, nullptr);
    if (!ok) {
        char b[64];
        snprintf(b, sizeof(b), "pas de réponse (erreur %lu)", GetLastError());
        r.error = b;
    } else {
        DWORD st = 0, sz = sizeof(st);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &st, &sz, WINHTTP_NO_HEADER_INDEX);
        r.status = (int)st;
        for (;;) {
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(req, &avail) || avail == 0) break;
            const size_t at = r.body.size();
            r.body.resize(at + avail);
            DWORD got = 0;
            if (!WinHttpReadData(req, &r.body[at], avail, &got)) { r.body.resize(at); break; }
            r.body.resize(at + got);
        }
    }
    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    return r;
}

std::string protectSecret(const std::string& s) {
    DATA_BLOB in = {(DWORD)s.size(), (BYTE*)s.data()}, out = {};
    if (!CryptProtectData(&in, L"BrickIt", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) return std::string();
    DWORD n = 0;
    CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &n);
    std::string b64(n, '\0');
    CryptBinaryToStringA(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &b64[0], &n);
    b64.resize(n);
    LocalFree(out.pbData);
    return b64;
}

std::string unprotectSecret(const std::string& b64) {
    DWORD n = 0;
    if (b64.empty() || !CryptStringToBinaryA(b64.c_str(), (DWORD)b64.size(), CRYPT_STRING_BASE64, nullptr, &n, nullptr, nullptr)) return std::string();
    std::vector<BYTE> raw(n);
    CryptStringToBinaryA(b64.c_str(), (DWORD)b64.size(), CRYPT_STRING_BASE64, raw.data(), &n, nullptr, nullptr);
    DATA_BLOB in = {n, raw.data()}, out = {};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) return std::string();
    std::string s((const char*)out.pbData, out.cbData);
    LocalFree(out.pbData);
    return s;
}

std::string deviceName() { return "PC"; }
#elif !defined(__APPLE__)
// Linux test build: no dialogs (scripted runs give the paths on the command line).
HttpReply http(const char*, const std::string&, const std::vector<std::string>&, const std::string&) { HttpReply r; r.error = "network not available"; return r; }
std::string protectSecret(const std::string& s) { return s; }
std::string unprotectSecret(const std::string& s) { return s; }
std::string deviceName() { return "Linux"; }
void setWindow(void*) {}
void errorBox(const std::string& title, const std::string& text) { fprintf(stderr, "%s: %s\n", title.c_str(), text.c_str()); }
void confirm(const std::wstring&, const std::wstring&, std::function<void()> onOk) { if (onOk) onOk(); }
void openFileDialog(const wchar_t*, const wchar_t*, std::function<void(const std::wstring&)>, const std::wstring&) {}
void saveFileDialog(const wchar_t*, const wchar_t*, const std::wstring&, std::function<void(const std::wstring&)>) {}
void shareFile(const std::wstring&) {}
void setAppIcon(int) {}
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
