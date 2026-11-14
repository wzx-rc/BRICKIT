// BrickIt - the few file system calls of the shared code (PC and iPad).
// Windows keeps its own API (_wfopen_s, swprintf_s, wide paths with backslashes); on other systems (iPad, the Linux
// test build) the same calls are provided here: wide paths are converted to UTF-8 and backslashes become slashes,
// so the shared code (Library, Builder, scenes) compiles unchanged.
#pragma once
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace brickit::plat {

std::string toUtf8(const std::wstring& w);
std::wstring fromUtf8(const std::string& s);
std::string nativePath(const std::wstring& w);   // UTF-8, '/' separators (non Windows)
bool fileExists(const std::wstring& path);
bool makeDir(const std::wstring& path);
// Folder of the user's BrickIt data (full library, tray, defaults). Windows: %USERPROFILE%\BrickIt. Elsewhere: the
// folder given by setDataDir (iPad: Application Support/BrickIt), else $HOME/BrickIt. Created when missing.
std::wstring dataDir();
void setDataDir(const std::wstring& dir);
// Folder of the crash / freeze reports. Windows: <dataDir>\crashlogs. iPad: Documents/Rapports (set by the shell,
// visible in Fichiers › BrickIt). Elsewhere: <dataDir>/crashlogs. Created when missing.
std::wstring reportsDir();
void setReportsDir(const std::wstring& dir);

// ---- system dialogs. The answer comes through a callback: at once on Windows (modal dialogs, as before), later on
// the iPad (the document picker / alert run while the app keeps drawing). Filters use the Windows format:
// L"Name\0*.a;*.b\0...\0"; defExt without the dot. The callback only runs when the user picked a file / said OK.
void setWindow(void* nativeWindow);   // owner of the dialogs (HWND on Windows)
void errorBox(const std::string& title, const std::string& text);
void confirm(const std::wstring& title, const std::wstring& text, std::function<void()> onOk);
void openFileDialog(const wchar_t* filter, const wchar_t* defExt, std::function<void(const std::wstring&)> done,
                    const std::wstring& initial = std::wstring());
void saveFileDialog(const wchar_t* filter, const wchar_t* defExt, const std::wstring& suggested,
                    std::function<void(const std::wstring&)> done);
// iPad: the share sheet with that file (AirDrop, Mail, Fichiers, other apps); elsewhere: nothing
void shareFile(const std::wstring& path);
// the icon of the app: 0 = A (with the viewfinder), 1 = B (without). iPad: the home screen icon (iOS confirms it with
// a message); PC: the icon of the window and the taskbar (the .exe keeps A in Explorer)
void setAppIcon(int which);

// ---- network (the cloud: Supabase, Wzx 14/11/2026). A blocking HTTPS request: call it from a worker thread, never
// from the frame. headers: "Name: value". status 0 = no answer (offline, DNS, TLS): error says why.
struct HttpReply { int status = 0; std::string body, error; };
HttpReply http(const char* method, const std::string& url, const std::vector<std::string>& headers, const std::string& body);
// a secret kept on disk (the session of the cloud): Windows DPAPI (only this Windows account can read it back), base64;
// elsewhere unchanged (the iPad keeps it in the app's own container)
std::string protectSecret(const std::string& s);
std::string unprotectSecret(const std::string& s);
// "PC" or "iPad": the device shown next to a scene of the cloud
std::string deviceName();

} // namespace brickit::plat

#ifndef _WIN32
#include <cerrno>
#include <cstdarg>
#include <cwchar>
inline int _wfopen_s(FILE** f, const wchar_t* path, const wchar_t* mode) {
    *f = std::fopen(brickit::plat::nativePath(path).c_str(), brickit::plat::toUtf8(mode).c_str());
    return *f ? 0 : errno;
}
template <size_t N>
inline int swprintf_s(wchar_t (&buf)[N], const wchar_t* fmt, ...) {
    va_list a;
    va_start(a, fmt);
    int n = std::vswprintf(buf, N, fmt, a);
    va_end(a);
    return n;
}
inline int _wtoi(const wchar_t* s) { return (int)std::wcstol(s, nullptr, 10); }
inline float _wtof(const wchar_t* s) { return std::wcstof(s, nullptr); }
#endif
