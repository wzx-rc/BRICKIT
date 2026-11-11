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
void crashSetActive(bool) {}
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
#else   // iPad, Linux: signals (SIGSEGV, SIGBUS, SIGABRT...), the stack by backtrace + dladdr, a watchdog for freezes
// On the iPad the report goes to Documents/Rapports (Fichiers › BrickIt › Rapports). The stack gives each frame as
// image + offset: « atos -o BrickIt.app.dSYM/Contents/Resources/DWARF/BrickIt -l <adresse de chargement> <adresses> »
// gives the function names (the dSYM of each build is in the release « ipad », next to the .ipa). After the report,
// the default handler runs again: iOS still writes its own .ips.
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <dlfcn.h>
#include <exception>
#include <execinfo.h>
#include <fcntl.h>
#include <mutex>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <thread>
#include <typeinfo>
#include <unistd.h>
#include <vector>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <sys/utsname.h>
#endif
#include "Platform.h"

namespace brickit {

namespace {
std::string g_dir, g_version;
std::mutex g_mx;
std::vector<std::string> g_crumbs;   // a ring of the last lines
size_t g_next = 0;
std::function<std::string()> g_state;
std::atomic<long long> g_beat{0};
std::atomic<bool> g_active{true};
std::atomic<bool> g_inCrash{false};
pthread_t g_main;
char g_terminateWhat[512] = "";   // the C++ exception that ended the app (terminate), for the SIGABRT report
const size_t kCrumbs = 400;
const int kSigs[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP};
// the stack of the main thread, taken by itself on SIGUSR2 (the watchdog asks, then reads it)
void* g_hangStack[128];
std::atomic<int> g_hangDepth{-1};

std::string nowText(bool file) {
    timeval tv;
    gettimeofday(&tv, nullptr);
    tm t;
    localtime_r(&tv.tv_sec, &t);
    char b[64];
    if (file) snprintf(b, sizeof(b), "%04d%02d%02d_%02d%02d%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    else snprintf(b, sizeof(b), "%02d:%02d:%02d.%03d", t.tm_hour, t.tm_min, t.tm_sec, (int)(tv.tv_usec / 1000));
    return b;
}
std::string crumbsText(bool tryOnly) {
    std::unique_lock<std::mutex> lk(g_mx, std::defer_lock);
    if (tryOnly) { if (!lk.try_lock()) return "(journal occupé au moment du plantage)\n"; }
    else lk.lock();
    std::string s;
    const size_t n = g_crumbs.size(), start = n < kCrumbs ? 0 : g_next;
    for (size_t k = 0; k < n; ++k) s += g_crumbs[(start + k) % n] + "\n";
    return s;
}
const char* sigName(int sig) {
    switch (sig) {
    case SIGSEGV: return "SIGSEGV (accès mémoire invalide)";
    case SIGBUS: return "SIGBUS (accès mémoire invalide)";
    case SIGILL: return "SIGILL (instruction illégale)";
    case SIGFPE: return "SIGFPE (calcul invalide)";
    case SIGABRT: return "SIGABRT (abandon : abort, exception C++, assertion)";
    case SIGTRAP: return "SIGTRAP (piège : vérification échouée)";
    }
    return "signal";
}
// where the app is loaded (atos -l) and the UUID of the binary (matches the dSYM: dwarfdump --uuid)
std::string imageText() {
    std::string s;
#ifdef __APPLE__
    const mach_header* h = _dyld_get_image_header(0);
    char b[256];
    snprintf(b, sizeof(b), "Binaire : %s\nAdresse de chargement : %p (glissement 0x%lx)\n", _dyld_get_image_name(0), (const void*)h,
             (unsigned long)_dyld_get_image_vmaddr_slide(0));
    s += b;
    if (h && h->magic == MH_MAGIC_64) {
        const load_command* lc = (const load_command*)((const char*)h + sizeof(mach_header_64));
        for (uint32_t k = 0; k < h->ncmds; ++k, lc = (const load_command*)((const char*)lc + lc->cmdsize))
            if (lc->cmd == LC_UUID) {
                const uint8_t* u = ((const uuid_command*)lc)->uuid;
                snprintf(b, sizeof(b), "UUID : %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n", u[0], u[1], u[2], u[3], u[4],
                         u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]);
                s += b;
            }
    }
    utsname un;
    if (uname(&un) == 0) s += std::string("Appareil : ") + un.machine + " · " + un.sysname + " " + un.release + "\n";
#endif
    return s;
}
std::string framesText(void* const* f, int n, int skip) {
    std::string s;
    char b[640];
    for (int k = skip; k < n; ++k) {
        Dl_info di{};
        const uintptr_t a = (uintptr_t)f[k];
        if (dladdr(f[k], &di) && di.dli_fname) {
            const char* img = strrchr(di.dli_fname, '/');
            img = img ? img + 1 : di.dli_fname;
            if (di.dli_sname)
                snprintf(b, sizeof(b), "%2d  %-22s 0x%016lx  %s + %lu\n", k - skip, img, (unsigned long)a, di.dli_sname, (unsigned long)(a - (uintptr_t)di.dli_saddr));
            else
                snprintf(b, sizeof(b), "%2d  %-22s 0x%016lx  %s + 0x%lx\n", k - skip, img, (unsigned long)a, img, (unsigned long)(a - (uintptr_t)di.dli_fbase));
        } else snprintf(b, sizeof(b), "%2d  ?                      0x%016lx\n", k - skip, (unsigned long)a);
        s += b;
    }
    return s;
}
std::string header(const char* kind) {
    std::string s = std::string("BrickIt - ") + kind + " - " + nowText(false) + "\n";
    s += "Version : " + g_version + "\n" + imageText();
    return s;
}
std::string writeText(const std::string& name, const std::string& text) {
    mkdir(g_dir.c_str(), 0755);
    std::string p = g_dir + "/" + name;
    int fd = open(p.c_str(), O_CREAT | O_WRONLY | O_TRUNC | (name == "seen.txt" ? 0 : O_EXCL), 0644);
    const size_t dot = name.rfind('.');
    for (int k = 2; fd < 0 && errno == EEXIST && k < 20; ++k) {   // (two reports in the same second: _2, _3...)
        p = g_dir + "/" + name.substr(0, dot) + "_" + std::to_string(k) + name.substr(dot);
        fd = open(p.c_str(), O_CREAT | O_WRONLY | O_EXCL, 0644);
    }
    if (fd >= 0) {
        for (size_t o = 0; o < text.size();) {
            const ssize_t w = write(fd, text.data() + o, text.size() - o);
            if (w <= 0) break;
            o += (size_t)w;
        }
        fsync(fd);
        close(fd);
    }
    return p;
}
void appendText(const std::string& path, const std::string& text) {
    const int fd = open(path.c_str(), O_WRONLY | O_APPEND);
    if (fd < 0) return;
    if (write(fd, text.data(), text.size()) < 0) {}
    fsync(fd);
    close(fd);
}
std::string readText(const std::string& path, size_t maxBytes) {
    std::string s;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return s;
    char b[8192];
    size_t n;
    while (s.size() < maxBytes && (n = fread(b, 1, sizeof(b), f)) > 0) s.append(b, n);
    fclose(f);
    return s;
}
// the newest report (crash_, hang_, metrickit_) newer than `after` (mtime), with its time
std::string newestReport(time_t after, time_t* when, bool ours = true, bool apple = true) {
    std::string best;
    time_t bt = after;
    if (DIR* d = opendir(g_dir.c_str())) {
        while (dirent* e = readdir(d)) {
            const std::string n = e->d_name;
            const bool o = n.rfind("crash_", 0) == 0 || n.rfind("hang_", 0) == 0, a = n.rfind("metrickit_", 0) == 0;
            if (!((o && ours) || (a && apple))) continue;
            struct stat st;
            const std::string p = g_dir + "/" + n;
            if (stat(p.c_str(), &st) == 0 && st.st_mtime > bt) { bt = st.st_mtime; best = p; }
        }
        closedir(d);
    }
    if (when) *when = bt;
    return best;
}

bool isMainThread() {
#ifdef __APPLE__
    return pthread_main_np() != 0;
#else
    return pthread_equal(pthread_self(), g_main) != 0;
#endif
}

void onSignal(int sig, siginfo_t* si, void*) {
    if (g_inCrash.exchange(true)) { signal(sig, SIG_DFL); raise(sig); return; }   // (a crash in the report itself)
    void* frames[128];
    const int n = backtrace(frames, 128);
    std::string s = header("PLANTAGE");
    char b[512];
    snprintf(b, sizeof(b), "Signal : %d %s, adresse en cause %p\n", sig, sigName(sig), si ? si->si_addr : nullptr);
    s += b;
    if (sig == SIGABRT && g_terminateWhat[0]) s += std::string("Exception C++ : ") + g_terminateWhat + "\n";
    s += "\n--- Pile (fil " + std::string(isMainThread() ? "principal" : "secondaire") + ") ---\n" + framesText(frames, n, 2);
    s += "\n--- Journal (les dernières actions, la plus récente en bas) ---\n" + crumbsText(true);
    const std::string path = writeText("crash_" + nowText(true) + ".txt", s);   // (written before the state: the state may crash too)
    std::string st = "\n--- État de l'app ---\n";
    if (g_state) { try { st += g_state(); } catch (...) { st += "(état illisible)\n"; } }
    appendText(path, st);
    // the default handler: the app ends as before, iOS writes its .ips
    signal(sig, SIG_DFL);
    raise(sig);
}

void onHangSignal(int) {   // on the main thread: its own stack, for the watchdog
    g_hangDepth.store(backtrace(g_hangStack, 128));
}

void watchdog() {
    long long last = g_beat.load();
    int still = 0;
    bool reported = false;
    for (;;) {
        sleep(1);
        const long long b = g_beat.load();
        if (b != last || !g_active.load()) { last = b; still = 0; reported = false; continue; }   // (in the background the loop stops: not a freeze)
        if (b == 0 || reported) continue;
        if (++still < 15) continue;
        reported = true;
        std::string s = header("GEL (la boucle d'affichage arrêtée depuis 15 s)");
        g_hangDepth.store(-1);
        pthread_kill(g_main, SIGUSR2);
        for (int k = 0; k < 50 && g_hangDepth.load() < 0; ++k) usleep(10000);
        const int n = g_hangDepth.load();
        s += "\n--- Pile du fil principal ---\n" + (n > 0 ? framesText(g_hangStack, n, 2) : std::string("(non obtenue)\n"));
        s += "\n--- Journal ---\n" + crumbsText(true);
        writeText("hang_" + nowText(true) + ".txt", s);
    }
}
}  // namespace

void crashInit(const std::wstring& dir, const std::wstring& version) {
    g_dir = plat::nativePath(dir);
    g_version = plat::toUtf8(version);
    mkdir(g_dir.c_str(), 0755);
    g_crumbs.reserve(kCrumbs);
    g_main = pthread_self();
    // an own stack for the handler (a stack overflow has none left)
    static std::vector<char> alt(256 * 1024);
    stack_t ss{};
    ss.ss_sp = alt.data();
    ss.ss_size = alt.size();
    sigaltstack(&ss, nullptr);
    struct sigaction sa{};
    sa.sa_sigaction = onSignal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    for (int sig : kSigs) sigaction(sig, &sa, nullptr);
    struct sigaction hs{};
    hs.sa_handler = onHangSignal;
    sigemptyset(&hs.sa_mask);
    sigaction(SIGUSR2, &hs, nullptr);
    std::set_terminate([] {   // an exception nobody caught: its text, then abort (the SIGABRT report)
        if (std::exception_ptr e = std::current_exception()) {
            try { std::rethrow_exception(e); }
            catch (const std::exception& x) { snprintf(g_terminateWhat, sizeof(g_terminateWhat), "%s : %s", typeid(x).name(), x.what()); }
            catch (...) { snprintf(g_terminateWhat, sizeof(g_terminateWhat), "(exception inconnue)"); }
        }
        std::abort();
    });
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
void crashSetActive(bool active) { g_active.store(active); crumb(active ? "App au premier plan" : "App en arrière-plan"); }
void crashSetState(std::function<std::string()> f) { g_state = std::move(f); }

std::wstring crashWriteReport(const char* kind) {
    if (g_dir.empty()) return std::wstring();
    std::string s = header(kind);
    s += "\n--- État de l'app ---\n";
    if (g_state) { try { s += g_state(); } catch (...) { s += "(état illisible)\n"; } }
    s += "\n--- Journal (les dernières actions, la plus récente en bas) ---\n" + crumbsText(false);
    for (int k = 0; k < 2; ++k) {   // the last crash / freeze of BrickIt, the last diagnostic of iOS (MetricKit), joined: one file to send
        time_t t = 0;
        const std::string last = newestReport(0, &t, k == 0, k == 1);
        if (last.empty()) continue;
        char b[64];
        tm lt;
        localtime_r(&t, &lt);
        strftime(b, sizeof(b), "%d/%m %H:%M", &lt);
        s += std::string("\n\n=== ") + (k == 0 ? "Dernier plantage ou gel" : "Dernier diagnostic iOS (MetricKit)") + " (" + last.substr(last.find_last_of('/') + 1) + ", " + b + ") ===\n" + readText(last, 2 << 20);
    }
    return plat::fromUtf8(writeText("report_" + nowText(true) + ".txt", s));
}

std::wstring crashLastUnseen() {
    if (g_dir.empty()) return std::wstring();
    struct stat st;
    const std::string seen = g_dir + "/seen.txt";
    const time_t after = stat(seen.c_str(), &st) == 0 ? st.st_mtime : 0;
    const std::string best = newestReport(after, nullptr);
    writeText("seen.txt", "vu\n");
    return plat::fromUtf8(best);
}

}  // namespace brickit
#endif
