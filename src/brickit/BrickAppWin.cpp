// BrickIt - the PC shell: Win32 window, DirectX 12 backend (GfxDx12), message loop. Everything else is shared
// (BrickApp.cpp and the other files compile unchanged on the iPad).
#ifdef _WIN32
#include "BrickApp.h"
#include "GfxDx12.h"
#include "Platform.h"
#include "backends/imgui_impl_win32.h"
#include <shellapi.h>
#include <algorithm>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace brickit {

static BrickApp* g_app = nullptr;

static LRESULT CALLBACK StaticWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (g_app) return g_app->wndProc(h, m, w, l);
    return DefWindowProcW(h, m, w, l);
}

bool BrickApp::init(HINSTANCE inst, const Options& opt) {
    g_app = this;
    m_opt = opt;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    m_exeDir = exe;
    m_exeDir = m_exeDir.substr(0, m_exeDir.find_last_of(L"\\/"));
    SetCurrentDirectoryW(m_exeDir.c_str());

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    wc.lpszClassName = L"WzxBrickIt";
    RegisterClassExW(&wc);

    UINT sysDpi = GetDpiForSystem();
    m_dpi = sysDpi / 96.0f;
    RECT r = {0, 0, (LONG)(opt.width * m_dpi), (LONG)(opt.height * m_dpi)};
    AdjustWindowRectExForDpi(&r, WS_OVERLAPPEDWINDOW, FALSE, 0, sysDpi);
    // never larger than the work area of the screen (1600 x 900 at 200 % is 3200 x 1800 px: wider than a 3072 px
    // screen, the header tools, the colours and the library ended up off screen); interactive runs open maximised then
    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY), &mi);
    const int workW = mi.rcWork.right - mi.rcWork.left, workH = mi.rcWork.bottom - mi.rcWork.top;
    int winW = r.right - r.left, winH = r.bottom - r.top;
    const bool tooBig = winW > workW || winH > workH;
    if (tooBig && opt.shotPath.empty()) { winW = std::min(winW, workW); winH = std::min(winH, workH); }
    m_hwnd = CreateWindowExW(0, wc.lpszClassName, versionString().c_str(), WS_OVERLAPPEDWINDOW, opt.shotPath.empty() && tooBig ? mi.rcWork.left : CW_USEDEFAULT,
                             opt.shotPath.empty() && tooBig ? mi.rcWork.top : CW_USEDEFAULT, winW, winH, nullptr, nullptr, inst, nullptr);
    if (!m_hwnd) return false;
    m_startMaximized = tooBig && opt.shotPath.empty();
    m_dpi = GetDpiForWindow(m_hwnd) / 96.0f;

    RECT cr;
    GetClientRect(m_hwnd, &cr);
    plat::setWindow(m_hwnd);
    std::string err;
    auto renderer = std::make_unique<GfxRendererDx12>();
    if (!renderer->r.init(m_hwnd, cr.right - cr.left, cr.bottom - cr.top, m_exeDir + L"\\shaders", &err)) {
        MessageBoxA(m_hwnd, err.c_str(), "BrickIt - DirectX 12", MB_ICONERROR);
        return false;
    }
    const float ui = opt.uiScale > 0 ? m_opt.uiScale : m_dpi;   // fonts and layout at the UI scale (100 % / 150 % tests)
    auto gui = std::make_unique<GfxGuiDx12>();
    if (!gui->init(m_hwnd, *renderer, ui)) {
        MessageBoxA(m_hwnd, "Dear ImGui initialisation failed", "BrickIt", MB_ICONERROR);
        return false;
    }
    if (!initCommon(opt, std::move(renderer), std::move(gui), m_exeDir, m_dpi)) return false;
    ShowWindow(m_hwnd, m_startMaximized ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    return true;
}

void BrickApp::showWindow() { ShowWindow(m_hwnd, m_startMaximized ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT); }

int BrickApp::run() {
    MSG msg;
    while (m_running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) m_running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!m_running) break;
        if (m_minimized) { Sleep(30); continue; }
        frame();
    }
    shutdownCommon();
    return 0;
}

LRESULT BrickApp::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return 1;
    switch (msg) {
    case WM_CLOSE: m_running = false; DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_SIZE:
        m_minimized = wp == SIZE_MINIMIZED;
        if (!m_minimized && m_renderer && m_renderer->ready()) {
            UINT w = LOWORD(lp), h = HIWORD(lp);
            if (w > 0 && h > 0 && (w != m_renderer->width() || h != m_renderer->height())) {
                m_renderer->resize(w, h);
                m_gui->onResize();
            }
        }
        return 0;
    case WM_DPICHANGED: {
        RECT* r = (RECT*)lp;
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace brickit

// --------------------------------------------------------------------------- entry point

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    brickit::Options opt;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;
    for (int i = 0; i < argc; ++i) args.push_back(argv[i]);
    brickit::parseOptions(args, opt);
    LocalFree(argv);
    brickit::BrickApp app;
    if (!app.init(inst, opt)) return 1;
    return app.run();
}
#endif
