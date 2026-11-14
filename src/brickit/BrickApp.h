// BrickIt - native brick builder (PC, DirectX 12) on the vp3d renderer, with the WZX Engine Dear ImGui theme.
//   BrickApp.cpp  window, loop, stage, scene sync, test scenes
//   BrickEdit.cpp mouse and keys: brick in hand, selection, undo, files
//   BrickUi.cpp   panels: toolbar, library (two rows at the bottom), colours (right), stats
//   Thumbs.cpp    part thumbnails rendered by the engine (atlas, cached on disk)
#pragma once
#include "Build.h"
#include "Pdf.h"
#include "Mech.h"
#include "Library.h"
#include "Gfx.h"
#include "Svg.h"
#include "BrickTheme.h"
#include "Cloud.h"
#include <windows.h>   // (iPad / Linux: compat/windows.h, the portable subset)
#include <string>
#include <map>
#include <set>
#include <memory>
#include <vector>

namespace brickit {

struct Ui;   // drawing helper of BrickUi.cpp
// view of a thumbnail (Thumbs.cpp): angles, zoom (1 = the box at 85 % of the image), pan (fractions of the half view)
struct ThumbView { float yaw = 0, pitch = 0, zoom = 1, panX = 0, panY = 0, light = 0; };   // (light: the lights turned around the part, degrees)
struct ThumbLook { float key = 32.0f, rim = 4.0f, env = 1.05f, exposure = 1.0f, bevel = 0.04f; int spp = 256, bounces = 4, neutral = 1; float base = 0.85f; int tm = 0; };   // (neutral: Wzx chose B, 06/11/2026)   // light of the thumbnails (spp > 0: path traced)   // light of the thumbnails

using namespace vp3d;

struct Options {
    int width = 1600, height = 900;
    std::wstring shotPath;    // --shot file.png: window capture then exit
    int shotSpp = 64;         // --spp N: path tracing samples before the capture
    int shotFrames = 20;      // raster: frames before the capture
    bool grid = false;        // --grid: every part of the library side by side
    int bench = 0;            // --bench N: N random bricks
    bool pathTrace = false;   // --pt: progressive path tracing
    int bounces = -1;         // --bounces N (path tracing, 0 = primary rays only)
    int bevelSpp = -1;
    bool rc = false;          // --rc: Radiance Cascades mode
    bool noGiBounds = false;  // --no-gi-bounds
    std::wstring hdri;        // --hdri name (part of a file name of assets\hdri, "ciel" = procedural sky) or path
    float hdriRot = -1, hdriInt = -1;   // --hdri-rot degrees, --hdri-int
    int gridColor = -1;       // --grid-color RRGGBB (hex)
    float gridAlpha = -1;     // --grid-alpha 0..1
    int softGI = -1;          // --soft-gi 0 / 1
    float giStrength = -1;    // --gi-strength 0..2
    int showProbes = -1;     // --show-probes 0 / 1
    int softGIProbes = -1;   // --soft-gi-probes n: probe budget of the soft GI (-1: the setting)
    float softGIMargin = 1.0f;  // --soft-gi-margin k (measures: the margin of the probe volume around the bricks, x k)
    int giAgeCap = 400;   // --gi-age-cap n: updates after which a probe is converged and left alone (0: always updated)
    float hqSSS = -1;   // --hq-sss s: subsurface of the HQ view (0 = off, -1: the setting)
    std::wstring thumbSampleDir; std::string thumbSampleParts;   // --thumb-sample <dir> <part,part,...>
    std::string thumbEdit;      // --thumb-edit <part>: the thumbnail editor open (captures); --thumb-edit-save: saved at frame 90
    bool thumbEditSave = false;
    std::string voxelEdit;      // --voxel-edit <part>: the Voxel Editor open (captures)
    bool veSolid = false;       // --ve-solid: without the X-ray
    bool crashTest = false;
    bool excavator = false;     // --excavator: the demo of the excavator and the gears (captures)     // --crash-test: a crash at frame 30 (the crash log checked)
    std::wstring bacFile;       // --bac-file <file>: the tray there (tests); --bac-test: the tray open with the selection + bricks 590..599
    bool bacTest = false;
    std::wstring thumbDir;      // --thumb-dir <dir>: the overrides there instead of %LOCALAPPDATA% (tests)
    std::wstring hqThumbs;     // --build-hq-thumbs pack [--hq-cat Brick] [--hq-px 512] [--hq-spp 2048]: HD thumbnails of one category
    std::string hqCat = "Brick";
    int hqPx = 512, hqSpp = 2048;
    std::wstring fullThumbs;   // --build-full-thumbs pack: renders the thumbnails of the full library and exits
    std::wstring importPath;   // --import model.mpd: an LDraw model (OMR), the reading mode opened
    int readStep = -1;         // --read-step n: the page read for the capture
    std::wstring importReport; // --import-report file: pieces, steps, parts not found
    std::wstring groupSnapTest;   // --group-snap-test file
    std::wstring groupTest;   // --group-test file: the groups checked (BrickGroups.cpp)
    bool useDefaults = false;   // --use-defaults: a capture with the default settings (assets / %LOCALAPPDATA%)
    int stillAA = -1;   // --still-aa 0 / 1
    int touchHud = -1;  // --touch-hud 0 / 1
    int theme = -1;     // --theme 1 clair / 2 sombre (captures)
    std::string skin;   // --skin classic | creme | minimal | sommet (captures; the theme of Réglages › Thème)
    int skinTint = -1, skinAccent = -1, settingsTab = -1;
    int libCards = -1;     // --lib-cards 0 / 1: the cards of the drawer, Image / Fiche (captures)
    std::string stFam;     // --st-fam basique / technic ...: the family of the drawer (captures)
    int stSub = -2;
    int libDock = -1, libClassic = -1, highPoly = -1;   // (--high-poly 0|1)   // (captures: --lib-dock 0|1|2, --lib-classic 0|1)
    int manualDock = -1; float manualW = 0, manualH = 0;   // --manual-dock 0|1|2, --manual-size w h (points; captures)
    bool stPaint = false, stGroups = false;   // --st-paint, --st-groups: the mode Peindre, the panel Groupes (captures)
    float cardScale = 0;   // --card-scale k: the size of the cards of the drawer (captures)
    int stPopup = 0;       // --st-popup N: a popup of the Studio header open (8: the scene menu) (captures)        // --st-sub N: its category (-1: Toutes) (captures)
    int cardSheet = -1;    // --card-sheet N: the sheet of the card N of the drawer shown (captures)
    bool ipadCreme = false;   // --ipad-creme: the cream touch kit instead of Studio (implied by --ipad-open)
    bool stMenu = false;   // --st-menu: the Studio marking menu open on the selection (captures)   // --skin-tint N, --skin-accent N, --settings-tab 0 / 1
    bool ipad = false;  // --ipad: the iPad version (set by the iOS shell): touch gestures of the web, simpler interface
    std::string ipadOpen;    // --ipad-open home | lib | palette | new: a panel of the iPad open (captures)
    std::string touchTest;   // --touch-test orbit | pinch | undo | pick | hold2: scripted finger gestures (BrickTouch.cpp)
    int freeRot = 0;    // --free-rot n: the selection turned n x 15 degrees at start (captures)
    bool colorArc = false;   // --color-arc: the colour arc of the touch HUD open (captures)
    float arcPhase = -1;     // --arc-phase t: the arrival of the swatches frozen at t (0..1, captures of the animation)
    float dof = -1, bloom = -1;   // --dof s, --bloom s (-1: the settings)
    float renderScale = -1;  // --render-scale s: the image rendered at s of the window (-1: the setting)
    int giSoft = 1;          // --gi-soft 0: the sharp visibility weights of the probes (comparison)
    float giSmooth = 0.0f;   // --gi-smooth r: probe GI read over the surface (radius in spacings, 0 = off; 0.5: +6 ms at 2000 x 1125)
    int giHold = 1;   // --gi-hold 0: the probes go on updating while a brick is carried (comparison)
    int giCarryBrick = -1, giCarryFrom = 0, giCarryTo = 0;   // --gi-carry i f1 f2: brick i carried in the air from frame f1, put down 3.5 cm in front at f2 (GI hold)
    int giEditBrick = -1, giEditOff = 0, giEditOn = 0;   // --gi-edit i f1 f2: brick i removed at frame f1, back at f2 (GI reactivity)  // --soft-gi-probes n (measures: probe budget of the soft GI)
    std::wstring writeHouse;  // --write-house file: builds the GI reference house (light grey bricks) and saves it
    bool giLookdev = false;   // --gi-lookdev: one light grey material, rough, no specular / coat, a light SSS
    float hqAO = -1;          // --hq-ao cm
    int hqRefl = -1;          // --hq-refl rays
    int aa = -1, shadows = -1;   // --aa 0..2 (MSAA 4x / 8x / AAA), --shadows 0..4 (ray traced shadows of the RC mode)        // --bevel-spp N (0 = bevel off)
    bool opaque = false;      // --opaque: benchmark bricks in standard colours only
    int denoise = -1;         // --denoise 0|1
    int ppf = 1;              // --ppf N: path tracing samples per frame
    bool noHdri = false, noGround = false;   // --no-hdri, --no-ground (render cost tests)
    float groundSize = 2000.0f;   // --ground size (cm)
    float ptMoveScale = 0.5f;     // path tracing resolution while the view moves
    bool sss = true;              // --no-sss
    bool demo = false;            // --demo: showcase of a few bricks (beauty captures)
    bool cleanGround = false;     // --clean: floor without the grid lines
    float exposure = 1.3f;        // --exposure
    int tonemapper = 0;           // --tonemap 0 ACES (default, as the web builder), 1 AgX, 2 AgX Punchy, 3 Standard
    float yaw = -1000, pitch = -1000;   // --yaw / --pitch degrees (camera)
    int stride = 0;           // --stride 1|4: path tracing pixels per frame (0 = 4 interactive, 1 for captures)
    float zoom = 1.0f;        // --zoom f: camera distance x f after framing everything
    bool noUi = false;        // --no-ui: viewport and stats only (benchmarks, beauty captures)
    float uiScale = 0.0f;     // --ui-scale f: forced UI scale (1 = 100 %, 1.5 = 150 %), 0 = the monitor's
    std::wstring openPath;    // --open scene.v3d.json
    // scripted UI test (captures): spawn a part in hand at a screen point, select a brick, a library category
    std::string spawnPart;    // --spawn PART
    float spawnX = 0.5f, spawnY = 0.5f;   // --at x y (fractions of the viewport)
    int selectBrick = -1;     // --select N
    int mode = -1;
    std::string infoPart;   // --info <part>: open the part sheet (captures)
    bool pivotEdit = false;
    bool dev = false;
    std::wstring pdfPath;       // --pdf <file>: import a PDF into the manual (captures)
    bool manual = false;        // --manual: show the manual window
    bool floatTray = false, optsRight = false;   // --float-tray x y, --opts-right (captures)
    float floatX = 0, floatY = 0;
    std::string ttBase, ttTop;
    std::string mouseTest;
    bool mechTests = false;     // --mechtests: the battery of mechanics tests, then quit
    int quality = -1;
    bool settings = false;
    bool cornell = false;       // --cornell
    bool noEnv = false;         // --no-env
    std::wstring perfTrace;     // --perf-trace <file> <frames> [orbit]: the time of each part of each frame, then quits
    int perfFrames = 600, perfOrbit = 0;
    bool groupBarTest = false;   // --group-bar-test: the bricks of the scene grouped and selected (the box and the bar of the group, captures)
    std::wstring cloudSelfTest;   // --cloud-selftest <file>: a sign-in with a wrong password (the network, the answer of Supabase), then quits
    std::wstring ptTrace;       // --pt-trace <file>      // --settings: the settings panel open (captures)           // --quality 0 | 1      // --mouse-test marquee | gizmo | ring | mech  // --turntable-test <base> <top>: mechanics self-test
    std::string devRoundtrip;
    std::wstring mayaOut;       // --maya <file.zip>: export the scene to Maya then quit (tests)   // --dev-roundtrip <part>: OBJ export / import self-test
    int libCategory = -1;     // --cat N
    bool focusDim = false;    // --focus
    std::wstring savePath;    // --save file: the scene saved at the capture (file format test)
    bool duplicate = false;   // --dup: Ctrl+D on the selection at start
    bool mechDemo = false;    // --mechdemo: Technic gear train
    bool mech = false;        // --mech: start in Mechanical mode
    float motor = 0;          // --motor deg: value of the first command (Mechanical mode test)
};

void parseOptions(const std::vector<std::wstring>& args, Options& opt);   // args[0] = the program

class BrickApp {
public:
#ifdef _WIN32
    // BrickAppWin.cpp: the PC shell (window, DirectX 12, message loop)
    bool init(HINSTANCE inst, const Options& opt);
    int run();
    LRESULT wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
#endif
    // The shared part, the same on every platform: the shell creates the window and the graphics backend, then
    // initCommon loads everything; frame() once per display refresh; shutdownCommon on exit.
    bool initCommon(const Options& opt, std::unique_ptr<GfxRenderer> renderer, std::unique_ptr<GfxGui> gui,
                    const std::wstring& exeDir, float dpi);
    void frame();
    void shutdownCommon();
    bool running() const { return m_running; }
    void saveSession() { if (userSession()) { savePrefs(); autosave(); } }   // iPad: before the app may be ended
    void resize(uint32_t width, uint32_t height);   // the drawable changed size (iPad rotation, split view)
    // Fingers and Apple Pencil (iPad shell; BrickTouch.cpp): raw touches in pixels, queued and read at the next frame.
    // A touch that starts on a panel is the mouse of the PC for the UI; one that starts in the 3D view follows the touch
    // gestures of the web builder (orbit, pinch / pan, press to take a brick, second finger turns it, two-finger tap undo).
    void touchBegan(uint64_t id, float x, float y, bool pen) { m_touchQueue.push_back({0, id, x, y, pen}); }
    void touchMoved(uint64_t id, float x, float y) { m_touchQueue.push_back({1, id, x, y, false}); }
    void touchEnded(uint64_t id, float x, float y, bool cancelled) { m_touchQueue.push_back({cancelled ? 3 : 2, id, x, y, false}); }
    // a scene handed by the system (iPad: a .brick from Fichiers, AirDrop, Mail; PC: the command line)
    bool openSceneFile(const std::wstring& path) { return openScene(path); }

private:
    // BrickApp.cpp
    void showWindow();   // the platform window becomes visible (PC; nothing on the iPad)
    void cameraInput();
    void rebuildScene();      // bricks -> vp3d scene (meshes once per part, one object per brick)
    void syncObjects();       // brick transforms and materials only
    int meshOf(int part);     // scene mesh of a part (uploaded once)
    void buildPartGrid();
    void buildBench(int count);
    void buildDemo();
    int colorIndex(int ldrawId) const;   // palette index of an LDraw colour code
    void setupStage();        // ground, HDRI, materials of the palette
    void buildHouse();        // the GI reference house (--write-house)
    void setLookdev(bool on); // GI look development: every colour the same light grey (live, Réglages › Lumière)
    void openTestHouse();
    int m_veKeysFrame = -100;   // the last frame the Voxel Editor had the keyboard
    void buildExcavatorDemo();   // Scène › Démo : pelleteuse et engrenages
    void buildMechShowcase(float originX);   // the stations of the mechanical demo (BrickMechTests.cpp)     // Dossier › Ouvrir la maison test (docs/brickit/tests/maison_gi.v3d.json)
    // Réglages > Lumière (native addition): environment image, its rotation and intensity, colour of the ground grid;
    // the same scene for every renderer (Rapide, HQ, RC, path tracing), saved in brickit_prefs.json ("light")
    void buildHdriList();     // L"" (procedural sky), assets\hdri\*.hdr | *.exr, then the user's HDRI
    void setHdri(const std::wstring& path);
    float3 hdriSunDir() const;   // the sun follows the HDRI (Wzx 09/11/2026): its brightest spot, turned with the HDRI
    bool m_hdriSun = false;      // the HDRI has a sun (a spot far brighter than the rest)
    float m_hdriSunAz = 0, m_hdriSunEl = 0;   // where, in the image (radians: azimuth of its column, elevation)
    void loadHdriDialog();
    uint32_t gridLineValue() const;
    std::vector<std::wstring> m_hdriPaths, m_hdriCustom;
    std::wstring m_hdriPath = L"*";   // "*": the studio of the web (first start)
    std::map<std::wstring, vp3d::EnvImagePtr> m_hdriCache;
    float m_hdriRotDeg = 0.0f, m_hdriInt = 0.55f, m_hdriLoadMs = 0.0f;
    float3 m_gridRgb{0.6f, 0.64f, 0.72f};
    float m_gridAlpha = 0.0f;   // 0 = the default lines
    int m_setDrag = -1;         // slider of the settings being dragged
    void applyLook();         // real time = the dark stage of the web; path tracing = the bright studio
    void frameAll();
    bool savePng(const std::wstring& path, std::vector<uint8_t> rgba, uint32_t w, uint32_t h);
    void toast(const std::string& text);

    // BrickTouch.cpp: the touch gestures of the web (canvas pointer events), iPad
    enum { TG_NONE, TG_MOUSE, TG_ORBIT, TG_TWO, TG_PRESS, TG_HOLD, TG_GIZMO, TG_MARQUEE, TG_GROUP, TG_AIM, TG_MENU, TG_VENAV };   // TG_VENAV: two fingers turn / zoom the view of the Voxel Editor   // TG_MENU: a long press opened the marking menu (Studio)
    struct TouchEvent { int type; uint64_t id; float x, y; bool pen; };   // type 0 began, 1 moved, 2 ended, 3 cancelled
    static constexpr float kIpadTopStrip = 12;   // iPad: points above the top bar, no touch starts there (the system swipe)
    struct TouchPt { float x, y, sx, sy; double t; bool pen; };
    struct TouchGesture {
        int type = 0;                 // TG_* (BrickTouch.cpp)
        uint64_t pid = 0;             // the finger that leads it
        int f0 = 0;                   // TG_PRESS: the frame of the press (the marking menu waits a few frames too)
        bool hasPid = false;          // a finger leads it
        bool kept = false;            // a brick kept in hand after the Pencil was cancelled: the next touch carries it
        int hit = -1;                 // press: the brick touched
        float3 hitPoint;
        float sx = 0, sy = 0, moved = 0;
        bool tap = false;
        bool paint = false;           // TG_MOUSE in the view: the Pinceau / Pipette (a second finger turns it into the camera)
        bool second = false, rotated = false;   // a second finger while a brick is in hand: swipe turns / tilts, tap turns
        uint64_t secondId = 0;
        float ax = 0, ay = 0;
        float d = 0, mx = 0, my = 0;  // two fingers: last distance and middle
        float ang = 0, twist = 0;     // two fingers: last angle, the turn accumulated (a twist turns the view past 4 degrees)
        bool twisting = false, centred = false;
        double t0 = 0;
        int fingers = 0;              // most fingers down at once (tap of 2 = undo, 3 = redo)
        bool still = true;            // no finger moved since the second came
    };
    std::vector<TouchEvent> m_touchQueue;
    std::map<uint64_t, TouchPt> m_touches;
    TouchGesture m_tg;
    double m_lastTapTime = -10;
    int m_lastTapBrick = -1;
    void touchPump();                 // frame(), before the UI: the queued touches
    void touchDown(const TouchEvent& e);
    void touchMove(const TouchEvent& e);
    void touchUp(const TouchEvent& e);
    bool touchOverUi(float x, float y) const;
    void touchCamera();               // after a camera change by the fingers
    void touchTestFrame();            // --touch-test
    void finishMarquee(bool add);     // the selection rectangle released (mouse and finger)
    ImVec2 m_holdPos{0, 0};           // where the brick in hand was last aimed (screen pixels)
    bool m_ipad = false;              // Options::ipad

    // BrickEdit.cpp
    View view() const;
    void editInput();                       // viewport mouse and keys
    int pickBrick(float sx, float sy, float3* point = nullptr, int exclude = -1);
    void spawn(int part, float sx, float sy);
    void beginMove(int brick, float sx, float sy);
    void updateHeld(float sx, float sy);
    void dropHeld(bool keepCopy = false);
    void cancelHeld();
    void select(int brick, bool add = false);
    // groups (BrickGroups.cpp)
    std::vector<int> m_groupCtx;   // the group entered by a double click (and its parents), outermost first
    int m_isolateGroup = -1;       // outliner: only this group shown (-1 = all)
    int unitGroup(int brick) const;              // the group selected with the brick at the level entered, -1 = alone
    std::vector<int> unitOf(int brick) const;    // the bricks selected with it
    std::vector<int> groupMembers(int group) const;
    bool brickHidden(int i) const;
    void fitGroupContext(int brick);
    void enterGroup(int brick);
    bool leaveGroup();
    void groupSelected();
    void ungroupSelected();
    void rotateGroup(const std::vector<int>& s, int dir);
    bool duplicateGroup(const std::vector<int>& s);
    bool m_groupDrag = false;
    std::vector<int> m_groupDragBricks;
    std::vector<float3> m_groupDragPos;
    float3 m_groupDragStart;
    float m_groupDragY = 0;
    int m_groupLead = -1;                // the member grabbed: the group snaps through its connectors (its own Held)
    Held m_groupHeld;
    void beginGroupDrag(const std::vector<int>& s, float sx, float sy);
    void updateGroupDrag(float sx, float sy);
    void endGroupDrag();
    void uiGroups();   // the outliner of the groups (BrickUi.cpp)
    void groupTest();  // --group-test file
    void groupSnapTest();   // --group-snap-test file: a group dragged onto studs / into Technic holes
    std::string prefsJson();
    void applyPrefs(const std::string& text);
    bool readDefaultPrefs(std::string& text);
    void saveDefaultPrefs();     // Réglages › Enregistrer comme réglages par défaut
    void resetToDefaultPrefs();  // Réglages › Rétablir
    void openOfficialHouse();
    // LDraw models and the reading of their instructions (BrickLDraw.cpp, BrickRead.cpp)
    bool importLDraw(const std::wstring& path);
    bool hasSteps() const;
    void startReading(int step);
    void stopReading();
    void readStepTo(int step);
    void frameReadStep();
    bool readHidden(int i) const;
    std::vector<int> readStepBricks() const;
    void readInput();
    void uiReader();
    bool m_reading = false;
    int m_readStep = 1, m_readSteps = 0;
    std::vector<std::string> m_readTitles;   // per step: the submodel built ("" = the model)
    std::string m_modelName, m_importReport;
    std::vector<int> m_groupsOpen;   // outliner: the groups unfolded
    int m_renameGroup = -1;
    char m_renameBuf[64] = {};
    std::vector<int> selection() const;
    void deleteSelected();
    void duplicateSelected();
    void rotateSelected(int dir);
    void applyColor(int color);
    void pushUndo();
    void undo();
    void redo();
    void restore(const std::vector<Brick>& snap);
    bool saveScene(const std::wstring& path);
    bool openScene(const std::wstring& path);
    void shareScene();               // iPad: Partager (the .brick to another app)
    void sendReport();               // iPad: Envoyer un rapport (state + journal + last crash, share sheet)
    void saveDialog();
    void openDialog();
    void noteRecent(int part);

    // BrickUi.cpp: the interface of the web builder v26, element by element (CSS colours, sizes, icons, texts)
    void drawUi();
    void uiHeader();
    void uiRadial();
    void uiColors();
    void uiColorsTouch();                   // iPad: a column of round swatches, the whole palette beside it on a tap
    void uiToolDock();                      // iPad: the tools of the selection, big labelled buttons on the left
    std::vector<int> m_recentColors;        // the last colours applied, most recent first
    bool m_paletteOpen = false;
    // iPad interface v5 (BrickUiIpad.inl)
    void drawUiIpad();
    void ipadPucks();
    void ipadDrawer();                      // the library drawer (m_uiRects[1])
    void ipadBubbles();
    void ipadColorArc();
    void ipadNuancier();
    void ipadHomeMenu();
    void ipadNewScene();
    void newScene(int kind);                // 0 green baseplate, 1 free floor
    bool selectionRect(ImVec2& a, ImVec2& b) const;   // the screen rectangle of the selection
    bool m_libOpen = false, m_homeOpen = false, m_newSceneOpen = false;
    int m_dockPress = -1;                   // the dock button pressed (drag up = take the part)
    ImVec2 m_dockPressPos{0, 0};
    int m_paletteFilter = 0;                // 0 all, 1 in the scene, 2 for this part
    int m_lightPreset = 0;                  // 0 morning, 1 noon, 2 evening
    bool m_pencilOnly = false;              // fingers never build (camera only)
    int m_ipadUiWas = -1;                   // (the interface of the last frame: the stage follows a change)
    bool m_stMenuWait = false;   // touch: the marking menu waits until ImGui has seen the finger down (the event lands a frame later)
    bool m_ipadCreme = false;               // iPad: the cream touch kit 2.0 (Thème › Crème iPad) instead of the Studio interface
    // UI kit 2.0 (docs/brickit/ui_kit/KIT.md): the kit's images, colours and drawing components
    struct KitColor { int ldraw = 0; std::string nom, famille; int index = -1; };
    std::map<std::string, ImTextureID> m_kitTex;
    std::vector<KitColor> m_kitColorList;
    bool m_kitColorsLoaded = false;
    std::vector<int> m_drawerParts;         // the parts of the drawer's tab (cached)
    std::string m_drawerKey;
    int m_libSel = -1;                      // the part last taken from the drawer / the dock
    int m_selKey = -1;                      // the selection the bubbles appeared for
    double m_selT0 = 0;
    ImTextureID kitTex(const std::string& name);
    void kitColors();
    std::string colorNameFr(int index);
    void kitIcon(ImDrawList* dl, const char* name, ImVec2 c, float size, ImU32 tint, bool shadow = false);
    void kitShadowCircle(ImDrawList* dl, ImVec2 c, float diam, ImU32 tint, float dy);
    void kitShadowRect(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 tint, float dy, float blur);
    void kitStud(ImDrawList* dl, ImVec2 c, float size, int color, int where, bool current);   // where: 0 scene, 1 sheet
    bool kitGalet(Ui& u, ImVec2 c, float diam, const char* icon, int state);                  // 0 normal 1 active 2 inactive 3 white
    bool kitBubble(Ui& u, ImVec2 c, float diam, const char* icon, float scale, bool red = false);
    void kitSheet(ImDrawList* dl, ImVec2 a, ImVec2 b, ImDrawFlags flags = 0);
    int kitSegmented(Ui& u, ImVec2 a, float h, const std::vector<const char*>& opts, int cur, bool alignRight);
    bool kitRow(Ui& u, ImVec2 a, float w, const char* icon, const char* label, const char* note, bool red = false);
    void kitLabel(ImDrawList* dl, ImVec2 centre, const std::string& t);
    void ipadTopBar();
    void ipadCorners();
    void ipadDock();
    void ipadMoreMenu(ImVec2 bubble);
    void ipadMessage();
    void shareImage();
    void uiTray();
    void uiMechPanel();
    void uiHud();
    void uiInfoSheet();
    void uiPopups();
    void drawToast();
    void exportLdrDialog();                 // the scene as an LDraw model (.ldr)
    void buildNav();                        // library families / categories counts
    std::vector<int> visibleParts() const;  // parts of the current family / category / search
    const std::string& partCat(int part) const;    // LDraw category of a part
    void tiltSelected(int dir);             // radial: tilt towards the viewer (up / down)
    void turnSelectedFree(int dir);         // touch HUD: 15 degrees about the vertical axis through the centre
    void uiSelHud();                        // touch HUD around the selection: turn 15 degrees, duplicate, colour arc
    void drawVoxels();                      // voxel display (studs yellow, pins cyan, axles magenta)

    // Mechanical mode (BrickMech.cpp)
    void mechEnter();
    void mechExit();
    void mechUpdate(float dt);
    void mechApplyPose();     // bricks drawn at the pose of their body
    void drawMechOverlay();
    void buildMechDemo();

    // BrickPrefs.cpp: what the web keeps in localStorage, in files next to the exe; scene of the first launch; .ldr
    void loadPrefs();
    void savePrefs();
    void autosave();                 // vp3d-builder-scene: the scene after every change
    bool restoreAutosave();
    static std::wstring versionString();   // « BrickIt - 0.02 - main - PC Win Local - <commit> » (build.bat: VERSION + git)
    bool m_autosaveGuard = false;          // the scene restored had parts not found: the autosave file is not overwritten
    void writeStartLog();                  // brickit_start.log next to the exe: where the libraries were found
    void sampleScene();
    std::string sceneLdr() const;
    void uiSceneSheet();
    void pickUpSelected();           // Space: the selected brick comes into the hand
    void cycleGrab();                // C: next pivot voxel of the selection / brick in hand

    // BrickAnim.cpp: animations of the web (brick in hand, drop, settle), placement preview (ghost, visor, drop
    // line, ring, glow of the aimed voxel), selection rectangle
    void updateAnim(float dt);
    void updateFocus(float dt);             // focus distance of the depth of field (Réglages › Mise au point)
    void updateGridFade();                  // radial fade of the ground grid around the model
    void finishDrop();
    void drawHoldOverlay();
    void placeAnimated(int brick, const float3& pos, const float4& rot, float squash, float tiltX, float tiltZ);
    struct Settle { int brick; float s, v, t; };
    std::vector<Settle> m_settles;
    int m_ghostObject = -1;            // translucent copy of the brick in hand at its landing spot
    bool m_marquee = false;
    ImVec2 m_marquee0, m_marquee1;

    // BrickGizmo.cpp: move / rotate manipulators of the web (arrows by studs / plates, rings by 90 degrees, centre
    // sphere = free move in hand)
    struct GizmoHandle { int kind = -1; int axis = 0; };   // kind 0 move, 1 rotate, 2 free
    struct GizmoDrag {
        GizmoHandle h;
        int brick = -1, steps = 0;
        float3 startPos, pivot, v0;
        float4 startRot{0, 0, 0, 1};
        float t0 = 0;
        bool hasV0 = false;
        std::vector<int> others;
        std::vector<Brick> othersStart;
    };
    bool gizmoVisible() const;
    float3 gizmoPivot(int b) const;
    float gizmoScale(const float3& piv) const;
    GizmoHandle pickGizmo(float sx, float sy) const;
    void gizmoStart(const GizmoHandle& h, float sx, float sy);
    void gizmoDragTo(float sx, float sy);
    void drawGizmo();
    // pivot edit (P): the voxels of the focus brick as markers, the cursor aims the nearest one
    int focusBrick() const { return m_holding ? m_held.brick : m_selected; }
    void aimPivot(float sx, float sy);
    void drawPivots();
    GizmoHandle m_gizmoHover;
    bool m_gizmoDragging = false, m_leftOrbit = false;
    GizmoDrag m_gz;

    // BrickDev.cpp / BrickUi.cpp: DEV mode of the web (connectors by type on every brick, LDraw outline over a
    // replacement, export for Maya, re-import with review, replacements kept in dev_overrides next to the exe)
    struct DevOverride { std::string id, label, baseRelease, importedAt, fileName; int triangles = 0, ldrawTriangles = 0; MeshData geometry; };
    struct DevReview {
        bool open = false, swapped = false, wasUse = true;
        int part = -1;
        MeshData g, prev;
        std::string fileName, note;
        float scale = 1;
        float3 offset, size, ref;
    };
    std::map<std::string, DevOverride> m_devOverrides;
    bool m_devShowVox = true, m_devShowEdges = true, m_devUseOverrides = true;
    DevReview m_devReview;
    std::map<int, std::vector<std::pair<float3, float3>>> m_devEdgeCache;
    std::string devId(int part) const;
    int devPartById(const std::string& id) const;
    std::wstring devDir() const;
    std::string devObjText(int part, const MeshData& g, const float3& origin, bool markers) const;
    std::string devReadme(int part) const;
    void devExport(int part);
    bool devParseObj(const std::string& text, MeshData& g, std::map<std::string, std::string>& header) const;
    void devFit(int part, MeshData& g, DevReview& r);
    void devSetGeometry(int part, const MeshData& g);
    void devApplyAll();
    void devLoadAll();
    bool devSave(const DevOverride& o, const std::string& obj);
    void devDelete(const std::string& id);
    void devImport(int forcedPart);
    void devImportFile(const std::wstring& path, int forcedPart);   // (after the file dialog)
    void devReviewCancel();
    void devReviewRecenter();
    void devReviewKeep();
    void devRevert(int part);
    void devSetOn(bool on);
    const std::vector<std::pair<float3, float3>>& devEdges(int part);
    void drawDevOverlay();
    void uiDevPanel();
    void uiDevSheet();
    void uiDevReview();

    // BrickMaya.cpp: scene export to Maya (web: maya_export.js + maya_template.py)
    struct MayaStats { int bricks = 0, parts = 0, bodies = 0, joints = 0, cmds = 0; std::vector<std::string> fails; double coupling = 0; };
    bool mayaExportScript(std::string& py, MayaStats& st, std::string& err);
    void mayaExportScene(const std::wstring& forcedPath = std::wstring());
    void mayaWriteZip(const std::wstring& path, const std::wstring& forcedPath, const std::string& zip, const std::string& py,
                      const std::string& nm, const MayaStats& st);   // (after the file dialog)

    // BrickManual.cpp / BrickUi.cpp: instruction manual of the web (steps of the scene read vertically, large book,
    // imported PDF read vertically, resizable window)
    void manualDirty();
    void renderSteps();
    ImTextureID stepImage(int n);
    float manualPage(Ui& u, ImVec2 a, float w, int n, int corner, bool big, bool draw);
    void pdfView(Ui& u, ImVec2 a, ImVec2 b, float& scroll, bool main);
    // iPad: a finger or the Pencil dragged on a page scrolls it, as in any iPad app, with a little inertia (Wzx 10/11/2026)
    void dragScroll(Ui& u, ImVec2 a, ImVec2 b, float& scroll, int id);
    int m_dsId = -1, m_dsFling = -1;
    float m_dsVel = 0;
    void uiManual();
    void uiManualBig();
    void pdfOpen(const std::wstring& path, const std::string& name, bool save);
    void pdfPump();
    void pdfDialog();
    std::map<int, ImTextureID> m_stepTex;
    std::vector<int> m_stepWanted;
    std::map<int, int> m_hiMaterial;     // material -> its copy highlighted (current step)
    int m_manRight = 2;
    float m_manW = 0, m_manH = 0, m_manScroll = 0, m_bigScroll = 0;
    int m_manDock = 0;                // Notice docked (Wzx 11/11/2026): 0 no, 1 in the dock space on the left, 2 on the right
    float m_manDockX = 0;             // its x this frame (after the library docked on that side)
    float manualWidth() const;
    void manualReserveDock();         // drawUiStudio, after the library: the docked Notice takes its band (m_dockL / m_dockR)
    bool m_manBig = false, m_pdfBig = false, m_manResizing = false;
    ImVec2 m_manResize0;
    float m_manW0 = 0, m_manH0 = 0;
    std::unique_ptr<PdfReader> m_pdf;
    bool m_pdfOn = false, m_pdfOpening = false, m_pdfScrollSmooth = false, m_prefsDirty = false;
    std::string m_pdfName, m_pdfMsg;
    int m_pdfPage = 1, m_pdfScrollTo = 0, m_bigScrollTo = 0;
    float m_pdfScroll = 0, m_pdfScrollGoal = -1;
    double m_pdfLock = 0;
    std::vector<std::pair<float, float>> m_pdfSizes;
    struct PdfTex { ImTextureID tex = 0; int w = 0, h = 0; bool asked = false; };
    std::vector<PdfTex> m_pdfTex;

    // BrickMouseTest.cpp: scripted mouse tests
public:
    struct MouseStep { int frame; float x, y; int btn; };   // btn -1 move, 0 press, 1 release
private:
    void mouseTestInput();
    void mouseTestFrame();
    void runMechTests();   // BrickMechTests.cpp (--mechtests)
    std::vector<MouseStep> m_mouseScript;
    Brick m_mtBefore;
    std::vector<double> m_mtQ;
    int m_mtBrick = -1;
    std::string m_mtDiag;
    float m_mtScroll0 = 0, m_mtScrollRel = 0;

    // Thumbs.cpp
    void buildThumbs();
    int buildHqThumbs();         // --build-hq-thumbs pack: the HD thumbnails of one LDraw category
    int m_hqSpp = 0;             // (while it runs: the samples of the path tracing, the denoiser light)
    std::vector<uint8_t> m_hqThumbData;   // full\thumbs_hq.pack ("BTF1"): the HD thumbnails, preferred over the others
    std::vector<std::pair<uint32_t, uint32_t>> m_hqThumbAt;
    int buildFullThumbs();       // --build-full-thumbs pack: the thumbnails of the full library
    void loadFullThumbIndex();
    bool lazyThumb(int part, ImTextureID& tex);
    bool thumbRgba(int part, std::vector<uint8_t>& rgba, int& w, int& h);   // the image of a part from the packs (HD first)
    ImTextureID sheetTexture(int part, int px);   // the image of the sheet of a card, at the size it is drawn
    ImTextureID m_sheetTex = 0;
    int m_sheetTexPart = -1, m_sheetTexPx = 0;
    std::vector<uint8_t> m_fullThumbData;
    std::vector<std::pair<uint32_t, uint32_t>> m_fullThumbAt;   // per part: offset, size in m_fullThumbData
    std::unordered_map<int, std::pair<ImTextureID, int>> m_lazyThumbs;   // part -> texture, frame last used
    int m_lazyThumbBudget = 0;
    static bool decodeImage(const uint8_t* data, size_t size, std::vector<uint8_t>& rgba, int& w, int& h);
    int setupStudio(Scene& ts);
    int setupStudio2(Scene& ts, int& glassMat);   // thumbnails v2: + a soft fill light, a glass material
    bool renderThumb2(Scene& ts, int obj, const Part& p, const ThumbView& v, int px, std::vector<uint8_t>& rgba);
    static ThumbView defaultThumbView();
    static RenderSettings thumbSettings();
    static ThumbLook thumbLook();
    static void placeThumbLights(Scene& ts, const Part& p, float lightDeg = 0.0f);
    static void fitThumbCamera(Camera& c, const Part& p, const ThumbView& v);
    // Edit thumbnail (DEV, BrickThumbEdit.cpp): right click menu, viewer, overrides in %LOCALAPPDATA%\\BrickIt\\thumb_overrides
    std::wstring thumbOverrideDir() const;
    ImTextureID overrideTexture(const std::vector<uint8_t>& rgba, int w, int h);
    void loadThumbOverrides();
    bool readThumbView(int part, ThumbView& v);
    void openThumbEditor(int part);
    void renderThumbEditor();
    void saveThumbOverride();
    void uiCtxMenu();
    void frameSelection();   // F: the view on the selection (a glide)
    void frameAnimate();
    void orbitOnSelection();   // touch: the view glides to turn around the selection (Wzx 14/11/2026)
    bool m_frameAnim = false;
    double m_frameT0 = 0;
    float3 m_frameFromT, m_frameToT;
    float m_frameFromD = 0, m_frameToD = 0;
    void setHighPoly(bool on);   // HD: the parts with their HD mesh (48-segment primitives, bevelled studs)
    bool m_highPoly = false;
    // Voxel Editor (mode DEV, BrickVoxelEditor.inl): the connectors of a part placed by hand, assisted
    struct VeSug { std::string label, detail; std::vector<float3> v; std::vector<uint8_t> t; };
    int m_vePart = -1, m_veMat = -1, m_veGlass = -1, m_veObj = -1, m_veSel = -1, m_veTool = 1, m_veType = 0, m_veMirror = 0, m_veSugHot = -1, m_veCtx = -1;
    bool m_veBuilt = false, m_veDirty = false, m_veLineA = false, m_veAutoType = true, m_veXray = true, m_veXrayShown = true, m_veNav = false;
    Scene m_veScene;
    ThumbView m_veView;
    ImVec4 m_veRect{0, 0, 0, 0};
    float3 m_veLineP;
    std::vector<float3> m_veVox;
    std::vector<uint8_t> m_veConn;
    std::vector<std::pair<std::vector<float3>, std::vector<uint8_t>>> m_veUndo;
    std::vector<VeSug> m_veSugs;
    std::vector<int> m_veSimilar;
    struct VeCyl { int axis; float u, v, r; bool outward; float lo, hi; };   // the cylinders found (the smart snap: their axis)
    std::vector<VeCyl> m_veCyl;
    void openVoxelEditor(int part);
    void renderVoxelEditor();
    void uiVoxelEditor();
    void veAnalyze();
    void vePush();
    void veAdd(const float3& q, uint8_t t);
    void veSave(bool validate);
    int veNextPart() const;
    void loadVoxEdits();
    void saveVoxEdits();
    void loadConnLabels();   // mode DEV: the source of the connectors of the parts, the labels set by hand
    void saveConnLabels();
    void stConnLegend();     // mode DEV: the legend of the colours of the cards (and their counts)
    bool m_connCountsDirty = true;
    int m_connCounts[5] = {0, 0, 0, 0, 0};
    // temporary brick tray (BrickBac.cpp): thumbnails in their colour, placed freely, kept in bac.json
    struct BacItem { int part = -1, color = 0; float x = 0, y = 0; ImTextureID tex = 0; };
    std::vector<BacItem> m_bac;
    bool m_bacOpen = false, m_bacDirty = false, m_bacBuilt = false, m_bacDragMoved = false, m_bacPanning = false, m_bacPanned = false;
    ImVec4 m_bacRect{0, 0, 0, 0};                // x, y, w, h
    float m_bacZoom = 1.0f;
    ImVec2 m_bacPan{0, 0}, m_bacGrab, m_bacPress;
    int m_bacMove = 0, m_bacDrag = -1, m_bacMat = -1, m_bacGlass = -1;
    Scene m_bacScene;
    std::wstring bacFile() const;
    void bacAdd(int part, int color, float x, float y);
    void bacAddSelection();
    void bacRenderPending();
    void loadBac();
    void saveBac();
    void uiBac();
    void uiThumbEditor();
    std::map<int, ImTextureID> m_thumbOverride;   // part -> its saved thumbnail
    int m_ctxPart = -1;                          // right click menu (DEV) on this part
    ImVec2 m_ctxPos;
    int m_teditPart = -1, m_teMat = -1, m_teGlass = -1, m_teObj = -1;   // thumbnail editor
    ThumbView m_teView;
    bool m_teSave = false, m_teBuilt = false, m_teDrag = false;
    Scene m_te;
    ImVec4 m_teRect{0, 0, 0, 0};
    int thumbSample();           // --thumb-sample dir parts
    // 3D viewer of the part sheet (web: partViewer): the part alone in the thumbnail studio, turned by dragging. While the
    // sheet is open the main view is frozen (its last image) and the renderer draws the viewer instead.
    void renderPreview();
    Scene m_pv;
    bool m_pvBuilt = false;
    int m_pvObj = -1, m_pvPart = -1, m_pvMat = -1;
    float m_pvYaw = 0, m_pvPitch = 0;
    bool m_pvDrag = false;
    ImVec4 m_pvRect{0, 0, 0, 0};
    ImTextureID m_frozenTex = 0;
    bool m_teLightDrag = false;     // Modifier la miniature: the slider of the lights held
    bool thumbUv(int part, ImTextureID& tex, ImVec2& uv0, ImVec2& uv1);

#ifdef _WIN32
    HWND m_hwnd = nullptr;
#endif
    Options m_opt;
    std::wstring m_exeDir;
    float m_dpi = 1.0f;
    bool m_running = true, m_minimized = false;
    std::unique_ptr<GfxRenderer> m_renderer;   // DirectX 12 (PC) or Lit (iPad), see Gfx.h
    std::unique_ptr<GfxGui> m_gui;
    RenderSettings m_rs;
    Scene m_scene;
    Library m_lib;
    Builder m_build{m_lib};
    std::vector<Brick>& m_bricks = m_build.bricks;
    std::vector<int> m_partMesh;       // part -> scene mesh (-1 = not uploaded)
    std::vector<int> m_colorMaterial;  // palette index -> scene material
    std::vector<int> m_dimMaterial;    // palette index -> same material at 50 % (focus mode)
    std::vector<int> m_tintMaterial;   // Mechanical mode: one material per body (Couleurs par corps)
    int m_blockedMaterial = -1;        // brick in hand that cannot be dropped (red ghost)
    int m_ghostMaterial = -1, m_ghostBlockedMaterial = -1;
    int m_rimLight = -1;
    bool m_startMaximized = false;
    bool m_cornell = false;
    int m_adaptiveSpp = 1;             // path tracing samples per frame (adaptive, see frame)            // --cornell: the WZX Engine Cornell box instead of bricks
    // native settings (gear button): UI scale over the screen scale, ambient occlusion of the HQ mode, automatic save
    float m_uiScaleUser = 1.0f;
    bool m_ssaoOn = true, m_autosaveOn = true;
    void uiSettings();
    // drag scrolling of the library rows (cards, families, categories): direction of the first pixels, inertia
    struct Swipe { int id = -1; ImVec2 start, last; int dir = 0; double t = 0; };
    Swipe m_swipe;
    ImVec2 m_mtCard{0, 0};             // mouse tests: the centre of a card of the library
    int m_swipeDragged = -1;           // row whose drag just ended (its release is not a click)
    float m_cardsVel = 0, m_famVel = 0, m_catVel = 0;
    // vertical: the library docked on a side (Wzx 10/11/2026): an up / down drag scrolls, a sideways drag takes the part
    bool swipeRow(int id, ImVec2 a, ImVec2 b, float& scroll, float& vel, float maxScroll, bool live, bool vertical = false);
    float m_clsVel = 0;
    bool swipeTaken(int id) const { return m_swipe.id == id && m_swipe.dir == 1; }   // the default size does not fit the screen               // cool rim light of the web (raster)   // placement preview (ghostMat of the web)
    int m_firstBrickObject = 0;        // scene objects before it: the stage
    ViewRect m_vp;
    int m_frame = 0;
    double m_cpuMs = 0, m_frameMs = 0;
    LARGE_INTEGER m_qpf{}, m_last{};
    std::string m_status;
    double m_lastMove = -10.0;   // time of the last camera or scene change (dynamic resolution)
    std::vector<Scene::MeshLod> m_lodsOff;   // the levels of detail while they are turned off (L)

    // editing
    Held m_held;
    bool m_holding = false;
    int m_selected = -1;
    std::vector<int> m_multi;
    int m_color = 0;                   // current colour (palette index)
    std::vector<std::vector<Brick>> m_undo, m_redo;
    bool m_focusDim = false;
    bool m_pressed = false, m_dragStarted = false;
    int m_pressBrick = -1;
    float3 m_pressPoint;
    ImVec2 m_pressPos, m_rmbPos;
    bool m_rmbMoved = false;
    std::wstring m_scenePath;
    std::vector<Box> m_pickBoxes;      // world boxes of the bricks (picking), for the scene revision m_pickVersion
    uint32_t m_pickVersion = 0;

    // UI (web builder state)
    struct NavItem { std::string label; int kind; std::string key; int fam = -1; };   // kept for --cat
    std::vector<std::string> m_families;
    std::vector<NavItem> m_nav;
    int m_navSel = 2;
    std::string m_fam = "Basique";     // "recent", "fav", "all" or an LDraw family
    std::string m_cat, m_sub;          // LDraw category / Technic sub-family ("" = Tout)
    std::string m_search;
    bool m_searchOpen = false;
    std::vector<int> m_recent, m_fav;
    int m_thumbSize = 0;               // 0 small, 1 medium, 2 large (64 / 84 / 112 px as the web; the web starts at 64)
    float m_ui = 1.0f;                 // UI scale (dpi or --ui-scale)
    std::string m_toast;
    double m_toastTime = -10.0;
    bool m_overUi = false;             // the mouse is over a panel this frame
    int m_mode = 1;                    // 0 select, 1 visor, 2 move, 3 rotate
    bool m_autoPivot = false, m_animOn = true, m_showVox = false, m_dev = false;
    int m_quality = 1;                 // 0 Rapide, 1 HQ
    // Radiance Cascades (native addition, Réglages): the Split Radiance Cascades of WZX Engine (real time GI, ray
    // traced, no history: sparse world probes around the visible surfaces), with the path tracing look
    bool m_rcMode = false;
    // HQ + GI douce (Tiny Glade like): the HQ view reads a world probe volume (DDGI, ray traced, long hysteresis,
    // bounded to the bricks): soft bounces and occlusion, stable, no ray per pixel for the GI
    bool m_softGI = true;
    float m_softGIStrength = 1.0f;
    int m_softGIProbes = 16384;       // Réglages › Lumière › Sondes GI: probe budget of the soft GI
    bool m_showProbes = false;        // Réglages › Lumière › Afficher les sondes
    size_t m_countsFor = 0;   // the library tray: parts counted for this many parts
    std::unordered_map<std::string, int> m_catCount, m_techSubCount;
    int m_availCount = 0;
    bool m_allColors = false;         // Couleurs: every colour offered, even those that do not exist for the part
    // PC theme 2.0 (BrickUi.cpp): Réglages › Thème (0 Système, 1 Clair, 2 Sombre), the HUD of the selection, its pipette
    int m_uiThemePref = 0;
    bool m_darkUi = true, m_sysDark = true;
    double m_sysThemeT = -1;
    // the theme module (BrickTheme.h): Réglages › Thème, the theme, and its tint and accent remembered per theme
    struct ThemeLook { int tint = 0, accent = 0; };
    std::string m_themeId = "glass";
    std::map<std::string, ThemeLook> m_themeLooks;
    theme::Tokens m_th;               // the theme resolved for this frame (BrickUi.cpp applyTheme)
    std::string m_thKey;              // what m_th was resolved for (id, mode, tint, accent)
    int m_setTab = 0;                 // Réglages: 1 = open on Thème (--settings-tab)
    int m_setSec = 0;                 // Réglages: the active section (Interface, Bibliothèque, Construction, Rendu, Caméra, Lumière, Thème, Fichiers)
    float m_setSecH[12] = {};          // Réglages: the height of each section as drawn last frame
    LARGE_INTEGER m_perfT0{}, m_perfA{}, m_perfB{}, m_perfD{}, m_perfE{}, m_perfT1{}, m_perfF{};   // (--perf-trace)
    float m_setScroll = 0;
    // ---- the cloud (BrickCloud.cpp, Cloud.h; Wzx 14/11/2026: the scenes between the iPad and the PC)
    static constexpr int kSetCloud = 8;   // Réglages › Cloud (its section)
    Cloud m_cloud;
    std::string m_cloudId, m_cloudSentKey, m_cloudMsg, m_cloudAuthMsg, m_cloudListErr;
    int m_cloudVer = 0, m_cloudChanges = 0;
    bool m_cloudAuto = true, m_cloudDirty = false, m_cloudSending = false, m_cloudChecking = false, m_cloudLoading = false, m_cloudListBusy = false;
    double m_cloudDirtyAt = 0, m_cloudPollAt = 0, m_cloudOkAt = -1, m_cloudSendAt = 0;   // (m_cloudSendAt: the next send, one per minute at most)
    std::vector<CloudScene> m_cloudList;
    float m_cloudScroll = 0;
    char m_cloudEmail[160] = {}, m_cloudPw[160] = {};
    void cloudInit();
    void cloudTick();
    void cloudNoteChange();
    void cloudForget();
    void cloudSend();
    void cloudCheck();
    void cloudOpen(const CloudScene& c, bool fromPoll);
    void cloudRefreshList();
    bool loadSceneText(const std::string& text, const std::string& name, bool camera);
    void stCloudList();
    // ---- the group selected: its box, its bar; presets (BrickGroups.cpp, Wzx 14/11/2026)
    void stGroupBar();
    // the scale of the UI at 100 %: the screen's, x 1.5 on the iPad (its 100 % is the 150 % of before, Wzx 14/11/2026)
    float uiBase() const { return (m_opt.uiScale > 0 ? m_opt.uiScale : m_dpi) * (m_ipad ? 1.5f : 1.0f); }
    bool inSelection(int brick) const;              // one of a selection of several
    std::vector<int> dragSetOf(int brick) const;    // what a drag on it moves: the selection, its group, or itself
    void stPresets();
    int barGroup(std::vector<int>& members, bool& open) const;
    bool savePreset(int group, const std::vector<int>& members);
    bool placePreset(const std::wstring& path, const std::string& name);
    std::vector<std::pair<std::string, std::wstring>> listPresets();
    std::wstring presetsDir() const;
    std::vector<std::pair<std::string, std::wstring>> m_presets;
    std::string m_presetsKey;
    float m_presetsScroll = 0;            // Réglages: scrolled (a section taller than the screen)
    std::map<std::string, ImTextureID> m_themeTex;
    bool applyTheme();                // true when the theme changed (the 3D stage then follows)
    void loadThemeFonts();
    ImTextureID themeTex(const char* file);
    void uiThemeTab(Ui& u, ImVec2 p0, ImVec2 p1);
    // the Studio interface (BrickUiStudio.inl, Maquette 4.0, Wzx 09/11/2026)
    bool m_studioUi = true;           // Réglages › Interface: Studio (new) / Classique
    bool m_paintMode = false;         // the mode PEINDRE of the header (its panel in place of the tools)
    bool m_paintGroup = true;         // Peindre › Groupe entier: a click paints the whole group of the brick (off: the brick alone)
    bool m_paintTool = false;         // tool Pinceau: a click on a brick gives it the current colour
    std::string m_stFam = "basique";  // the family of the drawer (Maquette 4.1)
    int m_stSub = -1;
    std::unordered_map<std::string, int> m_stFamSub;   // the last category of each family (back on a family: there)
    bool m_hideDeco = true;           // Réglages: the decorated parts (stickers, printed patterns) hidden from the library (Wzx 09/11/2026)                 // its category (-1: Toutes, the tiles of its categories)
    float m_stSubScroll = 0;
    std::unordered_map<std::string, float> m_stScrollMem;   // the scroll of the cards, kept per family and category
    std::string m_stScrollKey;          // the row of the categories, scrolled sideways
    bool m_stSizeDrag = false;        // the size picker held: the size follows the cursor until release
    std::vector<int> m_stSubFirst;    // the first part of each category (its image on the tile, Wzx 14/11/2026)
    std::vector<int> m_stSubCount;    // the parts of each category of the family (cached with m_stKey)
    int m_stW = 0, m_stL = 0;         // the size filter in studs (0: every size)
    float m_stCardScale = 1.0f;       // the size of the cards of the drawer (its slider, 0.6 to 2)
    bool m_stCardDrag = false;
    int m_stCardMode = 0;             // Réglages › Bibliothèque: 0 Image (the image alone), 1 Fiche (image, size, name, id)
    std::vector<int> m_stParts;
    std::string m_stKey;
    bool m_stMenu = false;            // the marking menu open (a press held still on a brick)
    bool m_stMenuShown = false;
    ImVec2 m_stMenuAt;
    double m_pressT = 0;
    int m_stMenuBtn = 0;              // the button that opened it (0 left held, 1 right held): its release acts
    bool m_stMenuSticky = false;      // opened by a quick right click: stays open, the next click acts
    int m_stMenuEat = -1;             // the frame whose click closed a sticky menu: that click is not passed to the view
    int m_rmbBrick = -1;              // the brick under a right press (Studio: right held still = the marking menu)
    double m_rmbT = 0;
    void drawUiStudio();
    void stHeader();
    void stTools();
    void stPaintPanel();
    void stPaintColors();   // PEINDRE: every colour in a strip under the header
    void nudgeSelected(int dx, int dy, int dz);   // the arrows of the keyboard: the selection moved by a stud (a plate up / down)
    struct PaintFx { ImVec2 p; int color; double t0; int kind; };   // 0 Pipette (a drop to the active colour), 1 / 2 a splash
    std::vector<PaintFx> m_paintFx;
    void stPaintFx();
    int m_colPress = -1;              // the colours on the right: the swatch pressed (a drag out of the panel takes a 1x1 of that colour, Wzx 10/11/2026)
    ImVec2 m_colPressPos, m_colCurAt = ImVec2(-1, -1);   // (the swatch of the current colour, for the touch test)
    bool paintHit(int brick, ImVec2 at, bool fx);
    ImVec2 m_paintSwatchAt = ImVec2(-1, -1);   // the active colour of the panel Peindre (where the drop of the Pipette lands)
    double m_paintSwatchPop = -10;
    bool m_paintStroke = false, m_paintCircle = false, m_paintUndo = false;   // the Pinceau held: a stroke; past a moment, the circle
    double m_paintStrokeT0 = 0, m_paintSampleT = 0;
    ImVec2 m_paintStrokeAt, m_paintSampleAt;
    std::vector<int> m_paintDone;     // the bricks of the stroke already painted
    float m_paintRadius = 17;         // the circle of the brush (px at the scale 1; halved, Wzx 09/11/2026)
    double m_paintT0 = -1;            // PEINDRE: when its colours came out (the arc pops like the marking menu)
    std::vector<float> m_paintHov;    // PEINDRE: the hover of each colour (it grows)
    // the library: docked at the bottom (0), on the left (1) or the right (2), its size (its grip), Studio or Classic
    int m_libDock = 0;
    float m_libH = 0, m_libW = 0;   // (0: the height of before, 540 px at the scale 1 on a side)
    bool m_libClassic = false;
    int m_libGrip = 0, m_libDockPreview = -1;
    ImVec2 m_libGripAt;
    float m_libGrip0 = 0;
    float m_dockL = 0, m_dockR = 0;   // the width the library takes on the left / the right
    std::set<std::string> m_clsOpen = {"basique", "technic"};   // Classic: the families open in the tree
    float m_clsTreeScroll = 0, m_clsGridScroll = 0;
    bool m_clsTechnic = false;
    void stDrawerClassic();
    void stLibGrip(Ui& u, ImVec2 a, ImVec2 b, bool live);
    void stLibPressEnd(Ui& u, ImVec2 p0, ImVec2 p1);
    int m_libFound = -1;              // the part found in the library from the marking menu (its card ringed)
    bool m_libFoundScroll = false;
    double m_stMenuT0 = 0;  // when the marking menu opened (the colours come out one by one)   // PEINDRE: the Pinceau and the Pipette in big, « Groupe entier »
    void paintOne(int brick);
    void stFile();         // the scene menu (m_popup 8): Nouvelle scène, Ouvrir, Enregistrer, Enregistrer sous
    void newScene();
    void saveNow();
    void stGroups();       // Groupes, Studio style
    float m_groupsScroll = 0;
    void stTool(int id);
    void stBadge();
    void stMore();
    void stColors();
    void stDrawer();
    void stSelHud();
    // Réglages › Brique en main (Wzx 10/11/2026): the look of the placement preview (the ghost and the yellow marks)
    int m_ghostCol = 0;               // 0 jaune, 1 blanc, 2 bleu, 3 vert, 4 rose, 5 la couleur de la brique
    float m_ghostAlphaPref = 0.32f;   // its opacity
    int m_appIcon = 0;                // Réglages › Icône de l'app: 0 A (viewfinder), 1 B (the brick alone); the header logo too
    int m_ghostGlow = 2;              // its glow: 0 none, 1 soft, 2 normal, 3 strong
    float3 ghostSrgb() const;         // the colour of the marks (sRGB 0..1)
    ImU32 ghostU32(float alpha) const;
    void applyGhostLook();            // into the ghost material and object
    void stRing();                    // Studio: the ring of the left hand (Réglages › Anneau tactile): turn, tilt, duplicate / place
    int stRingHit(float x, float y) const;   // the part of the ring under a point (-1 none, 0..3 the wedges, 4 the centre)
    void stRingAct(int part);
    ImVec2 m_ringC = ImVec2(-1, -1);  // the ring of this frame (its centre, its radius; -1 hidden)
    float m_ringR = 0;
    double m_ringFlashT = -10;        // a part touched: it lights up a moment
    int m_ringFlash = -1;
    void stHeld();
    void stCardSheet(int part, ImVec2 a, ImVec2 b, float t);
    int m_stHov = -1;                 // the card under the cursor, since m_stHovT (its sheet after 0.5 s)
    double m_stHovT = 0;
    double m_stSheetSeen = -1;        // when the sheet of a card was last shown (moving to the next card: at once)
    ImVec2 m_stHovA, m_stHovB;
    std::vector<int> stArcColors() const;
    bool settingsTabs(Ui& u, ImVec2 p);   // Réglages | Thème; true when clicked
    bool m_tabClicked = false;
    float m_themeTabH = 0;            // height of the Thème tab, as drawn last frame
    bool m_selHud = true;             // Réglages › HUD de sélection (on by default, apart from the iPad ring)
    bool m_pipette = false;           // the pipette of the HUD: the next brick clicked gives its colour
    std::map<std::string, ImTextureID> m_pcIcons;
    bool pcDarkNow();
    ImTextureID pcIcon(const char* name, bool white);
    void drawIcon(ImDrawList* dl, svg::Icon i, ImVec2 c, float size, ImU32 col);
    bool m_touchHud = true;           // Réglages › HUD tactile (iPad): the radial ring and the HUD around the selection
    bool m_arcOpen = false, m_arcDrag = false;   // colour arc of the touch HUD open; opened by a press still held (marking menu)
    static constexpr int HUD_ICONS = 7;          // turn left / right, duplicate, move, more, colour drop, hexagon (assets/brickit/hud)
    ImTextureID m_hudTex[HUD_ICONS] = {};
    void loadHudIcons();
    int m_hudKey = -1;                           // the selection the HUD eased in for
    double m_hudT0 = 0, m_arcT0 = 0, m_arcPickT = 0, m_moreT0 = 0;
    ImVec2 m_hudA, m_hudB;                       // the frame, following the selection smoothly
    float m_hudHover[8] = {}, m_arcHover[64] = {};
    int m_arcPick = -1;                          // the swatch picked (popping, the arc folding back)
    bool m_arcApplied = false, m_moreOpen = false;
    int m_arcPage = 0;
    ImVec2 m_arcPress;
    bool m_stillAA = false;           // Réglages › Anti-aliasing au repos (HQ: jittered frames accumulated while still)
    float m_dof = 0.0f, m_bloom = 0.0f;   // Réglages › Lumière › Profondeur de champ / Bloom (0 = off)
    int m_focusMode = 0;              // Réglages › Lumière › Mise au point: 0 auto (the selection / held brick), 1 centre of the screen, 2 manual
    float m_focusManual = 40.0f;      // manual focus distance (cm)
    bool m_focusReticle = true;       // the reticle of the Auto focus shown in the middle of the view
    float m_focusCur = -1.0f, m_focusAim = -1.0f;   // focus distance shown (eased towards the aim)
    float m_renderScale = 1.0f;       // Réglages › Échelle de rendu (1, 0.85, 0.75, 0.67)
    float m_selOutline = 0.22f, m_selFill = 0.15f;   // Réglages › Lumière: Contour / Voile de sélection
    float m_lumColumnH = 0.0f;        // Réglages: height of the Lumière column drawn last frame
    float m_hqSSSStrength = 1.0f;     // Réglages › Lumière › SSS (HQ): strength 0..2 (0 = off)
    bool m_softGIHold = true;   // Réglages › Lumière › Figer la GI pendant le déplacement: the probes held while a brick is carried
    bool m_giLookdev = false;                 // (not saved: a test view, off at the next launch)
    std::vector<Material> m_colorMaterialOrig;   // the palette materials as made, restored when the lookdev ends
    uint64_t m_giBoundsVersionSoft = ~0ull;
    float3 m_softLo{0, 0, 0}, m_softHi{0, 0, 0};
    float m_softMargin = 4.0f;
    uint64_t m_giBoundsVersion = ~0ull;   // scene version of the GI bounds (Radiance Cascades)
    void setRC(bool on);
    bool userSession() const;   // not a test / demo / capture run: the preferences and the autosave may be written
    bool pathTraced() const { return !m_rs.raster && !m_rcMode; }
    struct PanelState { bool visible = true, collapsed = false, floating = false; float x = 0, y = 0; };
    // grip of the library / brick actions: click folds, click-drag undocks into a floating window, dropped on the
    // bottom edge (library) or a side edge (actions) it docks again (web: dockable panels)
    struct GripDrag { int id = -1; ImVec2 start; bool moved = false; float ox = 0, oy = 0; };
    GripDrag m_grip;
    bool m_optsRight = false;
    bool gripPanel(int id, PanelState& P, ImVec2 ga, ImVec2 gb, ImVec2 panelTopLeft);
    void drawDropzone();
    PanelState m_pTray, m_pOpts, m_pColors, m_pHud{false, false}, m_pManual{false, false}, m_pGroups{false, false};
    int m_popup = 0;                   // 0 none, 1 quality, 2 tonemap, 3 windows, 4 scene
    ImVec2 m_popupAt;
    int m_libPress = -1;               // library card pressed (drag / long press)
    ImVec2 m_libPressPos;
    double m_libPressTime = 0;
    bool m_libLong = false;
    int m_infoPart = -1;               // part sheet (long press on a card)
    float m_cardsScroll = 0, m_famScroll = 0, m_catScroll = 0;
    ImVec4 m_stage{0, 0, 1, 1};        // the 3D view: between the top bar and the tray (as the web's main.stage)
    std::vector<ImVec4> m_uiRects;     // panels of this frame (x0, y0, x1, y1): the mouse over one = UI
    bool m_mechJoints = true, m_mechTint = false;
    bool m_sheetOpen = false;          // « Scène partagée » sheet
    std::string m_sheetText;
    bool m_pivotEdit = false;          // P
    int m_voxStep = 0;                 // V: voxels hidden / shown

    // Mechanical mode
    Mech m_mech{m_lib};
    MechModel m_mm;
    bool m_mechOn = false;
    std::string m_mechFlash;
    Mech::Drag m_mechDrag;
    float3 m_mechDragPlaneN;
    bool m_mechDragging = false;
    bool m_testCarry = false;   // --gi-carry: a brick carried by the test
    double m_mechFlashTime = -10;

    // thumbnails
    std::vector<ImTextureID> m_thumbAtlas;
    std::vector<int> m_thumbSlot;      // part -> slot (-1 none)
    int m_thumbPx = 128, m_thumbCols = 21;   // the web images are 240 x 192: 128 px cells stay sharp at 4K
};

} // namespace brickit
