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
    std::string quality;            // hq | rt | dlss
    std::string script;             // test states (see ArchApp::runScript)
    std::wstring file;              // building to open
    bool demo = false;              // the demo house, not the last session
    std::wstring saveTo;            // the building after the script, written to this file (prepared scenes)
    bool clean = false;             // captures without the interface (render tests)
    float camYaw = -1000, camPitch = -1000, camDist = -1;
    std::string env;                // studio | sun (the sky HDRI with its sun), default: sun where the path tracer runs
    float sunRot = -1;              // degrees, rotation of the sky around the building
};
void parseOptions(const std::vector<std::wstring>& args, Options& o);

// kPlace: an object of the library in hand (Furnish); kMoveTool / kRotateTool: the manipulators of BrickIt on the objects
enum Tool { kSelect = 0, kRoom, kWall, kWindowTool, kDoorTool, kStairTool, kToolCount, kPlace = kToolCount, kMoveTool, kRotateTool };
constexpr int kLightTool = 30;   // the light tools: kLightTool + FixtureKind (their icons too)
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
    int m_quality = 0;              // 0 HQ (raster + soft GI), 1 ray tracing, 2 RT DLSS (OIDN still, DLSS moving)
    double m_lastMove = -10;
    void setupLook();
    // environment: 0 the soft studio (maquette look), 1 the sky HDRI with its sun (realistic)
    int m_env = 0;
    float m_envRot = 200;           // degrees
    vp3d::EnvImagePtr m_envImg[2];
    bool m_envHasSun[2] = {false, false};
    float m_envSunAz[2] = {0, 0}, m_envSunEl[2] = {0, 0};
    void applyEnv();
    float m_camKey[10] = {};        // the view of the last frame (a new view restarts the path tracer)
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
    int m_furnishTool = kSelect;    // Select, Move or Rotate: the tool an object placed goes back to
    int m_level = 0;
    float m_wallThick = 0.30f, m_wallHeight = 2.70f;
    float m_winW = 1.20f, m_winH = 1.35f, m_winSill = 0.90f, m_doorW = 0.90f, m_doorH = 2.10f;
    int m_newFloor = 0;
    // the finishes (ArchMesh): the room on a side of a wall, the faces looking into a room; a colour being dragged in the
    // picker makes one undo step
    int roomBeside(int w, int side) const;
    std::vector<std::pair<int, int>> roomFaces(int room) const;
    std::vector<std::pair<int, int>> facadeFaces() const;   // the outer faces of the walls around rooms (every level)
    bool m_colourLive = false;
    int m_faceSide = 0;   // the face of the selected wall being edited (0: faceL)
    bool finishEdit() { const bool first = !m_colourLive; m_colourLive = ImGui::IsAnyItemActive(); return first; }
    // selection: 0 none, 1 wall, 2 opening, 3 room, 4 item (furniture)
    int m_selKind = 0, m_selWall = -1, m_selOp = -1, m_selRoom = -1, m_selItem = -1;
    // ---- stairs (ArchStair.h): selection kind 5; the one being drawn (its foot pressed, the direction dragged)
    int m_selStair = -1;
    Stair m_stairGhost;
    bool m_stairDrawing = false;
    float m_stairYaw = 0;
    float2 m_stairOff;
    int stairAt(float2 p) const;
    void drawStairPlan(ImDrawList* dl, const Stair& s, ImU32 col, bool ghost);
    void drawRoofPlan(ImDrawList* dl);
    // ---- the Light step (ArchLight.h, ArchLightUi.cpp): fixtures, selection kind 6; the one under the mouse of a
    // light tool (kLightTool + kind); the exposure (auto: from the image shown, only brighter than 0 EV) and the sun
    int m_selFixture = -1;
    Fixture m_fixGhost;
    bool m_fixGhostOk = false;
    float2 m_fixOff;
    int fixtureAt(float2 p) const;
    Fixture placeFixture(int kind, float2 p, bool free) const;   // snapped (a wall light: against the nearest wall)
    void drawFixturePlan(ImDrawList* dl, const Fixture& f, ImU32 col);
    void lightInspector(float k, ImFont* bold);
    void lightPanel(float k, ImFont* bold);
    bool m_autoExposure = true;
    bool m_night = false;              // the clock's sun is down: no sun from the sky image either
    float m_ev = 0.0f;                 // exposure, EV above the default
    double m_aeLast = 0;
    int m_aeCalm = 0;                  // frames the auto exposure has not moved (a capture waits for it)
    void autoExposure();               // after the frame is drawn: measures it, moves m_ev
    void applySun();                   // the sun of the clock (Building::sunClock) or of the sky image
    bool m_ceiling = false;         // slabs over the rooms (interior renders), off for the open model view
    bool m_selDirty = false;        // the selection tint changed (no geometry to rebuild)
    // furnish: the library of objects (dataDir/ArchViz/models), the one being placed
    int m_step = 0;                 // 0 Build, 1 Furnish
    std::vector<AssetInfo> m_assets;
    int m_placeAsset = -1;
    float m_placeYaw = 0;
    float2 m_placeAt, m_moveOff;
    void ensureAssets();            // loads the assets of the items into the scene (before a rebuild)
    float2 snapItem(const Item& it, float2 p, bool free) const;   // 5 cm, flush against the walls nearby
    int itemAt(float2 p) const;
    void addItem(int asset, float2 p, float yaw);
    void touched();                 // an item moved: saved, nothing rebuilt
    // ---- joinery: the leaves of an opening opened / closed, animated (Opening::open, no geometry rebuilt)
    struct OpAnim { int wall = -1, op = -1, leaf = -1; float from = 0, to = 1; double t0 = 0; };   // leaf -1: all of them
    std::vector<OpAnim> m_opAnim;
    void toggleOpening(int wall, int op, int leaf = -1, bool tilt = false);
    void animateOpenings();
    // ---- ArchJoineryUi.cpp: the open / close handles of the leaves of the selected opening (3D view and plan)
    struct LeafHandle { int leaf = -1; bool tilt = false; bool plan = false; };   // leaf: index in m_map.leaves
    struct LeafDrag { LeafHandle h; ImVec2 press; float start = 0; bool moved = false, active = false; };
    LeafHandle m_leafHover;
    LeafDrag m_leafDrag;
    LeafHandle m_scriptHover;       // (captures: a handle shown hovered)
    bool leafHandleScreen(int li, bool tilt, bool plan, float t, ImVec2& out) const;
    LeafHandle pickLeafHandle(ImVec2 s) const;
    int leafAtRay(ImVec2 s) const;  // 3D view: the leaf under the mouse (its box), -1 = none
    void leafDragTo(ImVec2 s);
    bool joineryInput();            // true: the mouse belongs to a handle this frame
    void drawLeafHandles(ImDrawList* dl, bool plan);
    int itemAtRay(ImVec2 s) const;  // 3D view: the object under the mouse (its box), -1 = none

    // ---- ArchGizmo.cpp: the move / rotate manipulators of BrickIt on the selected object (3D view). Move: three
    // arrows (x, up, y), the squares of the planes between them, the centre sphere = free on the floor (flush against
    // the walls). Rotate: the ring around the vertical. By the grid step / 15 degrees, Shift: free.
    struct GizmoHandle { int kind = -1; int axis = 0; };   // kind 0 arrow, 1 ring, 2 sphere, 3 plane normal to axis
    struct GizmoDrag {
        GizmoHandle h;
        int item = -1, steps = 0;
        Item start;
        vp3d::float3 pivot, v0;
        float t0 = 0, angle = 0;
        bool hasV0 = false;
        vp3d::float2 off;
    };
    GizmoHandle m_gzHover;
    GizmoDrag m_gz;
    bool m_gzDragging = false;
    int m_rotStep = 15;             // degrees of the ring (15, 45, 90; 0 = free)
    bool gizmoVisible() const;
    vp3d::float3 gizmoPivot(const Item& it) const;
    float gizmoScale(const vp3d::float3& piv) const;
    bool projectW(const vp3d::float3& p, ImVec2& out) const;        // 3D view: world point -> screen
    void rayW(ImVec2 s, vp3d::float3& o, vp3d::float3& d) const;    // 3D view: screen -> world ray
    GizmoHandle pickGizmo(ImVec2 s) const;
    void gizmoStart(const GizmoHandle& h, ImVec2 s);
    void gizmoDragTo(ImVec2 s);
    bool gizmoInput();              // true: the mouse belongs to the manipulator this frame
    void drawGizmo(ImDrawList* dl);

    // ---- ArchLibrary.cpp: the library of objects, on the base of BrickIt's: docked left / right / bottom (its grip
    // resizes it, dragged to an edge of the screen it docks there, a click folds it), search, categories, a grid of
    // thumbnails; a card clicked = the object in hand, dragged out of the library = in hand under the cursor and
    // dropped on release, held = its sheet.
    int m_libDock = 1;              // 0 bottom, 1 left, 2 right
    float m_libW = 318, m_libH = 214;   // (unscaled pixels)
    bool m_libFold = false;
    char m_libSearch[64] = {};
    std::string m_libCat;           // "" = all
    int m_libPress = -1, m_libInfo = -1;
    ImVec2 m_libPressPos;
    double m_libPressTime = 0;
    bool m_libLong = false, m_libDragPlace = false;
    int m_libGrip = 0, m_libDockPreview = -1;
    ImVec2 m_libGripAt;
    float m_libGrip0 = 0;
    ImVec4 m_libRect{0, 0, 0, 0};   // where it is this frame (the other panels make room)
    std::vector<ImTextureID> m_thumbs;
    std::vector<char> m_thumbTried;
    ImTextureID thumbOf(int asset);
    void drawLibrary();
    void libPressEnd();
    void drawThumbFallback(ImDrawList* dl, ImVec2 a, ImVec2 b, const AssetInfo& as);

    // ---- snapping options (walls, rooms, objects) and the preferences kept between the sessions
    bool m_snapGrid = true, m_snapAlign = true;
    float m_gridStep = 0.10f;       // metres
    float gridSnap(float v) const;  // to the grid step when the grid is on and Shift is up
    void loadPrefs();
    void savePrefs();
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
    ImVec4 m_toolsRect{0, 0, 0, 0}; // the dock of the tools this frame (the hint keeps clear of it)

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
