// BrickIt - crash log: see CrashLog.h
#include "CrashLog.h"
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <atomic>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace brickit {

namespace {
std::wstring g_dir, g_version;
std::mutex g_mx;
std::vector<std::string> g_crumbs;   // a ring of the last lines
size_t g_next = 0;
std::function<std::string()> g_state;
std::atomic<long long> g_beat{0};
DWORD g_mainThread = 0;
HANDLE g_mainHandle = nullptr;
std::atomic<bool> g_inCrash{false};
const size_t kCrumbs = 400;

std::string nowText(bool file) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char b[64];
    if (file) snprintf(b, sizeof(b), "%04d%02d%02d_%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    else snprintf(b, sizeof(b), "%02d:%02d:%02d.%03d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    return b;
}
std::string narrowW(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}
std::string crumbsText() {
    std::lock_guard<std::mutex> lk(g_mx);
    std::string s;
    const size_t n = g_crumbs.size(), start = n < kCrumbs ? 0 : g_next;
    for (size_t k = 0; k < n; ++k) s += g_crumbs[(start + k) % n] + "\n";
    return s;
}
const char* codeName(DWORD c) {
    switch (c) {
    case EXCEPTION_ACCESS_VIOLATION: return "ACCESS_VIOLATION (lecture / écriture d'une adresse invalide)";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_STACK_OVERFLOW: return "STACK_OVERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "IN_PAGE_ERROR";
    case 0xE06D7363: return "C++ exception (non attrapée)";
    case 0xC0000409: return "STACK_BUFFER_OVERRUN / fail fast";
    case 0xC0000374: return "HEAP_CORRUPTION";
    default: return "?";
    }
}
// the stack of a thread from a context: module!function+offset (file:line) per frame
std::string stackText(HANDLE thread, CONTEXT ctx) {
    HANDLE proc = GetCurrentProcess();
    static bool symOk = false;
    if (!symOk) {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        symOk = SymInitialize(proc, nullptr, TRUE) != FALSE;
    }
    STACKFRAME64 f{};
    f.AddrPC.Offset = ctx.Rip;
    f.AddrFrame.Offset = ctx.Rbp;
    f.AddrStack.Offset = ctx.Rsp;
    f.AddrPC.Mode = f.AddrFrame.Mode = f.AddrStack.Mode = AddrModeFlat;
    std::string s;
    for (int i = 0; i < 64; ++i) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, thread, &f, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) break;
        if (!f.AddrPC.Offset) break;
        char line[1024];
        char modName[MAX_PATH] = "?";
        const DWORD64 base = SymGetModuleBase64(proc, f.AddrPC.Offset);
        if (base) { char p[MAX_PATH]; if (GetModuleFileNameA((HMODULE)base, p, MAX_PATH)) { const char* b = strrchr(p, '\\'); snprintf(modName, sizeof(modName), "%s", b ? b + 1 : p); } }
        alignas(SYMBOL_INFO) char symBuf[sizeof(SYMBOL_INFO) + 512];
        SYMBOL_INFO* sym = (SYMBOL_INFO*)symBuf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 511;
        DWORD64 disp = 0;
        IMAGEHLP_LINE64 ln{};
        ln.SizeOfStruct = sizeof(ln);
        DWORD ldisp = 0;
        const bool hasSym = SymFromAddr(proc, f.AddrPC.Offset, &disp, sym) != FALSE;
        const bool hasLine = SymGetLineFromAddr64(proc, f.AddrPC.Offset, &ldisp, &ln) != FALSE;
        if (hasSym && hasLine) snprintf(line, sizeof(line), "  #%02d %s!%s+0x%llx  (%s:%lu)\n", i, modName, sym->Name, (unsigned long long)disp, ln.FileName, ln.LineNumber);
        else if (hasSym) snprintf(line, sizeof(line), "  #%02d %s!%s+0x%llx\n", i, modName, sym->Name, (unsigned long long)disp);
        else snprintf(line, sizeof(line), "  #%02d %s+0x%llx\n", i, modName, (unsigned long long)(f.AddrPC.Offset - base));
        s += line;
    }
    return s;
}
std::wstring writeText(const std::wstring& name, const std::string& text) {
    CreateDirectoryW(g_dir.c_str(), nullptr);
    const std::wstring p = g_dir + L"\\" + name;
    FILE* f = nullptr;
    if (_wfopen_s(&f, p.c_str(), L"wb") == 0 && f) { fwrite(text.data(), 1, text.size(), f); fclose(f); }
    return p;
}
std::string header(const char* kind) {
    std::string s = std::string("BrickIt - ") + kind + " - " + nowText(false) + "\n";
    s += "Version : " + narrowW(g_version) + "\n";
    return s;
}
std::string tail() {
    std::string s = "\n--- État de l'app ---\n";
    if (g_state) { try { s += g_state(); } catch (...) { s += "(état illisible)\n"; } }
    s += "\n--- Journal (les dernières actions, la plus récente en bas) ---\n" + crumbsText();
    return s;
}

LONG WINAPI onCrash(EXCEPTION_POINTERS* ep) {
    if (g_inCrash.exchange(true)) return EXCEPTION_CONTINUE_SEARCH;
    const std::string stamp = nowText(true);
    std::string s = header("PLANTAGE");
    char b[512];
    const EXCEPTION_RECORD* er = ep->ExceptionRecord;
    snprintf(b, sizeof(b), "Exception : 0x%08lX %s à l'adresse %p\n", er->ExceptionCode, codeName(er->ExceptionCode), er->ExceptionAddress);
    s += b;
    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
        snprintf(b, sizeof(b), "Accès : %s de l'adresse 0x%llx\n", er->ExceptionInformation[0] == 0 ? "lecture" : er->ExceptionInformation[0] == 1 ? "écriture" : "exécution", (unsigned long long)er->ExceptionInformation[1]);
        s += b;
    }
    s += "\n--- Pile ---\n" + stackText(GetCurrentThread(), *ep->ContextRecord) + tail();
    writeText(L"crash_" + std::wstring(stamp.begin(), stamp.end()) + L".txt", s);
    // the minidump (Visual Studio opens it with BrickIt.pdb next to the exe)
    const std::wstring dmp = g_dir + L"\\crash_" + std::wstring(stamp.begin(), stamp.end()) + L".dmp";
    HANDLE h = CreateFileW(dmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei{GetCurrentThreadId(), ep, FALSE};
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h, (MINIDUMP_TYPE)(MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory), &mei, nullptr, nullptr);
        CloseHandle(h);
    }
    return EXCEPTION_CONTINUE_SEARCH;   // (Windows then closes the app as before)
}
void raiseForLog() { RaiseException(0xE0000001, 0, 0, nullptr); }   // (abort, terminate, pure call: a report through onCrash)

void watchdog() {
    long long last = g_beat.load();
    int still = 0;
    bool reported = false;
    for (;;) {
        Sleep(1000);
        const long long b = g_beat.load();
        if (b != last) { last = b; still = 0; reported = false; continue; }
        if (b == 0 || reported) continue;   // (not started yet, or already written for this freeze)
        if (++still < 15) continue;
        reported = true;
        std::string s = header("GEL (la boucle d'affichage arrêtée depuis 15 s)");
        if (g_mainHandle && SuspendThread(g_mainHandle) != (DWORD)-1) {
            CONTEXT ctx{};
            ctx.ContextFlags = CONTEXT_FULL;
            if (GetThreadContext(g_mainHandle, &ctx)) s += "\n--- Pile du fil principal ---\n" + stackText(g_mainHandle, ctx);
            ResumeThread(g_mainHandle);
        }
        s += "\n--- Journal ---\n" + crumbsText();   // (not the state: the main thread holds it, it may be locked)
        const std::string stamp = nowText(true);
        writeText(L"hang_" + std::wstring(stamp.begin(), stamp.end()) + L".txt", s);
    }
}
}  // namespace

void crashInit(const std::wstring& dir, const std::wstring& version) {
    g_dir = dir;
    g_version = version;
    g_crumbs.reserve(kCrumbs);
    SetUnhandledExceptionFilter(onCrash);
    signal(SIGABRT, [](int) { raiseForLog(); });
    _set_purecall_handler([] { raiseForLog(); });
    std::set_terminate([] { raiseForLog(); std::abort(); });
    g_mainThread = GetCurrentThreadId();
    g_mainHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, g_mainThread);
    std::thread(watchdog).detach();
    crumb("Démarrage");
}

void crumb(const char* fmt, ...) {
    char b[600];
    va_list a;
    va_start(a, fmt);
    vsnprintf(b, sizeof(b), fmt, a);
    va_end(a);
    std::lock_guard<std::mutex> lk(g_mx);
    std::string line = nowText(false) + "  " + b;
    if (g_crumbs.size() < kCrumbs) g_crumbs.push_back(line);
    else { g_crumbs[g_next] = line; g_next = (g_next + 1) % kCrumbs; }
}

void crashHeartbeat() { g_beat.fetch_add(1); }
void crashSetState(std::function<std::string()> f) { g_state = std::move(f); }

std::wstring crashWriteReport(const char* kind) {
    const std::string stamp = nowText(true);
    return writeText(L"report_" + std::wstring(stamp.begin(), stamp.end()) + L".txt", header(kind) + tail());
}

std::wstring crashLastUnseen() {
    // the newest crash_ / hang_ file newer than the marker <dir>\seen.txt; the marker then touched
    WIN32_FIND_DATAW fd;
    FILETIME seen{};
    {
        HANDLE h = FindFirstFileW((g_dir + L"\\seen.txt").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) { seen = fd.ftLastWriteTime; FindClose(h); }
    }
    std::wstring best;
    FILETIME bt{};
    for (const wchar_t* pat : {L"\\crash_*.txt", L"\\hang_*.txt"}) {
        HANDLE h = FindFirstFileW((g_dir + pat).c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (CompareFileTime(&fd.ftLastWriteTime, &seen) > 0 && CompareFileTime(&fd.ftLastWriteTime, &bt) > 0) { bt = fd.ftLastWriteTime; best = g_dir + L"\\" + fd.cFileName; }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    writeText(L"seen.txt", "vu\n");
    return best;
}

}  // namespace brickit
#else   // iPad, Linux: the journal of the last actions only (no handlers, no minidump)
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <vector>

namespace brickit {
namespace {
std::mutex g_mx;
std::vector<std::string> g_crumbs;
size_t g_next = 0;
}
void crashInit(const std::wstring&, const std::wstring&) {}
void crumb(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::lock_guard<std::mutex> lk(g_mx);
    if (g_crumbs.size() < 400) g_crumbs.push_back(buf);
    else g_crumbs[g_next++ % 400] = buf;
}
void crashHeartbeat() {}
void crashSetState(std::function<std::string()>) {}
std::wstring crashWriteReport(const char*) { return std::wstring(); }
std::wstring crashLastUnseen() { return std::wstring(); }
}  // namespace brickit
#endif
