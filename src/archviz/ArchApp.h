// ArchViz - the building editor (step 1 « Build » of the ArchViz pipeline: Build, Furnish, Light, Render).
// Proposal B « Maquette vivante » chosen by Wzx on 26/11/2026, with a synchronised 2D plan: the same tools work in the
// 3D view (HQ / ray traced, the BrickIt renderer) and in the plan; every edit shows in both at once.
// Portable: the PC shell (ArchAppWin.cpp, DirectX 12) and the Linux test build (Lit, offscreen) drive the same class.
#pragma once
#include "ArchMesh.h"
#include "Gfx.h"
#include "imgui.h"
#include "vp3d/RenderTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace archviz {

struct Options {
    int width = 1600, height = 1000;
    float uiScale = 0;              // 0 = the screen DPI
    std::wstring shotPath;          // capture after shotFrames then quit
    int shotFrames = 30;
    int shotSpp = 64;               // ray tracing captures: samples
    std::string view;               // 3d | split | plan
    std::string quality;            // hq | rt
    std::string script;             // test states (see ArchApp::runScript)
    std::wstring file;              // building to open
    bool demo = false;              // the demo house, not the last session
    float camYaw = -1000, camPitch = -1000, camDist = -1;
};
void parseOptions(const std::vector<std::wstring>& args, Options& o);

enum Tool { kSelect = 0, kRoom, kWall, kWindowTool, kDoorTool, kToolCount };
enum View { kView3D = 0, kViewSplit, kViewPlan };

class ArchApp {
public:
    bool initCommon(const Options& opt, std::unique_ptr<brickit::GfxRenderer> r, std::unique_ptr<brickit::GfxGui> g,
                    const std::wstring& exeDir, float dpi);
    void frame();
    void shutdownCommon();
    bool running() const { return m_running; }
#ifdef _WIN32
    bool init(HINSTANCE inst, const Options& opt);
    int run();
    LRESULT wndProc(HWND h, UINT m, WPARAM w, LPARAM l);
#endif

private:
    // ---- model and scene
    Building m_b;
    vp3d::Scene m_scene;
    SceneMap m_map;
    bool m_sceneDirty = true;
    std::vector<std::string> m_undo, m_redo;
    void pushUndo();
    void undo();
    void redo();
    void edited();                  // after any change of m_b: geometry, rooms, scene, autosave
    void save(const std::wstring& path);
    bool load(const std::wstring& path);
    std::wstring m_savePath;

    // ---- render
    std::unique_ptr<brickit::GfxRenderer> m_r;
    std::unique_ptr<brickit::GfxGui> m_g;
    vp3d::RenderSettings m_rs;
    int m_quality = 0;              // 0 HQ (raster + soft GI), 1 ray tracing
    double m_lastMove = -10;
    void setupLook();
    void renderSettings(vp3d::RenderSettings& rs);

    // ---- views
    int m_view = kViewSplit;
    ImVec4 m_r3{0, 0, 0, 0}, m_rp{0, 0, 0, 0};   // 3D and plan rectangles (x0, y0, x1, y1)
    float2 m_planCentre{6, 4.5f};
    float m_planScale = 40;         // pixels per metre
    ImVec2 planToScreen(float2 p) const;
    float2 screenToPlan(ImVec2 s) const;
    bool project(float2 p, float z, ImVec2& out) const;   // 3D view: plan point at height z above the active level
    bool rayToPlan(ImVec2 s, float2& out, float elev) const;
    void cameraInput();
    void frameAll();

    // ---- editing
    int m_tool = kRoom;
    int m_level = 0;
    float m_wallThick = 0.30f, m_wallHeight = 2.70f;
    float m_winW = 1.20f, m_winH = 1.35f, m_winSill = 0.90f, m_doorW = 0.90f, m_doorH = 2.10f;
    int m_newFloor = 0;
    // selection: 0 none, 1 wall, 2 opening, 3 room
    int m_selKind = 0, m_selWall = -1, m_selOp = -1, m_selRoom = -1;
    // hover
    int m_hoverView = 0;            // 0 none, 1 3D, 2 plan
    ImVec2 m_mouse;
    float2 m_raw, m_cur;            // raw and snapped plan points under the mouse
    float m_tol = 0.2f;             // snapping distance (metres) at the mouse
    struct Guide { float2 a, b; };
    std::vector<Guide> m_guides;
    std::string m_snapLabel;
    // drawing
    bool m_drawing = false, m_dragged = false;
    float2 m_start, m_chainFirst;
    ImVec2 m_pressAt;
    int m_hoverWall = -1;
    float m_hoverAlong = 0;
    Opening m_ghost;
    bool m_ghostOk = false;
    // dragging a selection (select tool)
    bool m_moving = false;
    float m_moveRef = 0;
    void toolInput();
    float2 snap(float2 raw, bool hasRef, float2 ref);
    void commitWall(float2 a, float2 b);
    void commitRoom(float2 a, float2 b);
    void pickSelect(float2 p);
    void deleteSelection();

    // ---- numbers typed (pills, Tab, digits)
    enum EditTarget { kEditNone, kEditLength, kEditWidth, kEditDepth, kEditWallLen, kEditOpLeft, kEditOpRight, kEditOpWidth };
    struct Pill { ImVec4 rect; int target; };
    std::vector<Pill> m_pills, m_pillsPrev;
    int m_edit = kEditNone;
    bool m_editFocus = false;
    char m_editBuf[32] = {};
    ImVec2 m_editAt;
    float m_typedW = -1, m_typedD = -1;   // room: width typed, waiting for the depth
    void openEdit(int target, ImVec2 at, const char* initial);
    void applyEdit(float v);
    ImVec4 pill(ImDrawList* dl, ImVec2 c, const char* text, int style, int target = kEditNone);

    // ---- drawing
    void drawPlan(ImDrawList* dl);
    void draw3DOverlay(ImDrawList* dl);
    void drawUi();
    void drawIcon(ImDrawList* dl, int kind, ImVec2 c, float s, ImU32 col);
    bool m_uiHover = false;

    // ---- misc
    Options m_opt;
    std::wstring m_exeDir;
    float m_dpi = 1, m_ui = 1;
    bool m_running = true;
    int m_frame = 0;
    std::string m_toast;
    double m_toastT = -10;
    void toast(const std::string& s);
    void runScript();
#ifdef _WIN32
    HWND m_hwnd = nullptr;
    bool m_minimized = false;
#endif
};

} // namespace archviz
