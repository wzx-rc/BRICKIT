// BrickIt on the Lit renderer: the part of the iPad app that does not depend on iOS. The same class runs in the
// iPad app (ios/main_ios.mm, Metal through MoltenVK) and in BrickItLitHeadless (Linux / Windows), so that the image
// of the iPad can be checked without the iPad.
//
// J1 (docs/brickit/PORTAGE_IPAD.md): the shared BrickIt code (library, scene file, stage, palette materials) drawn by
// Lit with instancing, the camera at the fingers, and a small HUD. The full builder UI (BrickApp) comes next.
#pragma once
#include "Build.h"
#include "Library.h"
#include "lit/LitRenderer.h"
#include "ImGuiDiligent.h"

#include <memory>
#include <string>
#include <vector>

namespace brickit {

struct ViewerOptions {
    std::string assets;          // folder of base.json, base.bin, technic/, default_scene.v3d.json
    std::string dataDir;         // the user's data (full library in dataDir/full), may be empty
    std::string scene;           // .v3d.json to open; empty: assets/default_scene.v3d.json
    int bench = 0;               // > 0: that many random bricks instead of a scene (instancing benchmark)
    std::string hdri = "sky";    // "sky" (procedural, Wzx's default), "none" or an HDR file
    float hdriRotation = 232.6f, hdriIntensity = 1.136f;   // degrees, Wzx's defaults (default_prefs.json)
    float uiScale = 1.0f;        // pixels per point (iPad: the screen scale)
    bool touch = false;          // larger targets for fingers
    bool hud = true;
    bool ddgi = false;           // soft GI: DDGI probes captured in raster (no ray tracing)
};

// Two fingers on the screen this frame (pixels).
struct TouchCamera {
    bool twoFingers = false;
    float centerX = 0, centerY = 0;   // centre of the two fingers
    float moveX = 0, moveY = 0;       // move of the centre since the previous frame
    float pinch = 1.0f;               // distance ratio since the previous frame
    float twist = 0.0f;               // turn since the previous frame (radians)
};

class BrickViewer {
public:
    bool init(Diligent::IRenderDevice* device, Diligent::IDeviceContext* context, Diligent::IEngineFactory* factory,
              Diligent::TEXTURE_FORMAT outputFormat, const std::string& shaderDirs, const ViewerOptions& opt,
              std::string* error);
    void shutdown();
    // One frame into output (width x height pixels). The ImGui input (mouse / touch) is given before this call.
    void frame(Diligent::ITextureView* output, uint32_t width, uint32_t height, float dt, const TouchCamera& cam);

    // one finger on the 3D view (not on the HUD): orbit
    void orbit(float dxPixels, float dyPixels);
    void frameAll();
    bool openScene(const std::string& path, std::string* message = nullptr);
    void makeBench(int count, int seed = 1);

    const vp3d::Scene& scene() const { return m_scene; }
    lit::LitRenderer& renderer() { return m_renderer; }
    size_t brickCount() const { return m_build.bricks.size(); }
    int shownBricks() const { return m_shown; }
    const std::string& log() const { return m_log; }

private:
    void setupStage();
    void rebuildObjects();
    void hud(float width, float height);

    ViewerOptions m_opt;
    Library m_lib;
    Builder m_build{m_lib};
    vp3d::Scene m_scene;
    std::vector<int> m_colorMat, m_partMesh;
    int m_firstBrick = 0, m_shown = 0;
    lit::LitRenderer m_renderer;
    lit::Settings m_settings;
    player::ImGuiDiligent m_gui;
    bool m_imgui = false;
    float m_fps = 0, m_fpsTime = 0;
    int m_fpsFrames = 0;
    std::string m_log;
};

} // namespace brickit
