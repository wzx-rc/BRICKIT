#include "BrickViewer.h"
#include "Platform.h"
#include "Stage.h"
#include "imgui.h"
#include "vp3d/EnvImage.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace brickit {

using vp3d::float3;

static bool readText(const std::string& path, std::string& out) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? (size_t)n : 0);
    size_t got = n > 0 ? std::fread(out.data(), 1, out.size(), f) : 0;
    std::fclose(f);
    return got == out.size();
}

bool BrickViewer::init(Diligent::IRenderDevice* device, Diligent::IDeviceContext* context, Diligent::IEngineFactory* factory,
                       Diligent::TEXTURE_FORMAT outputFormat, const std::string& shaderDirs, const ViewerOptions& opt,
                       std::string* error) {
    m_opt = opt;
    if (!opt.dataDir.empty()) plat::setDataDir(plat::fromUtf8(opt.dataDir));
    std::string err;
    if (!m_lib.load(plat::fromUtf8(opt.assets), &err)) {
        if (error) *error = "Library: " + err;
        return false;
    }
    m_log = "Library: " + std::to_string(m_lib.parts.size()) + " parts, " + std::to_string(m_lib.packCount) + " packs";
    setupStage();
    if (opt.bench > 0) makeBench(opt.bench);
    else {
        std::string path = opt.scene.empty() ? opt.assets + "/default_scene.v3d.json" : opt.scene;
        if (!openScene(path, &err)) m_log += "\nScene: " + err;
    }
    if (!m_renderer.init(device, context, factory, shaderDirs, outputFormat, error)) return false;
    m_settings.exposure = 1.3f;
    m_settings.bakedProbes = false;
    m_settings.ddgi = opt.ddgi;

    ImGui::CreateContext();
    m_imgui = true;
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 10.0f;
    st.FrameRounding = 8.0f;
    st.FramePadding = opt.touch ? ImVec2(16, 12) : ImVec2(10, 6);
    st.ItemSpacing = opt.touch ? ImVec2(12, 10) : ImVec2(8, 6);
    st.ScaleAllSizes(opt.uiScale);
    st.FontSizeBase = opt.touch ? 17.0f : 15.0f;
    st.FontScaleDpi = opt.uiScale;
    io.Fonts->AddFontDefaultVector();
    return m_gui.init(device, context, outputFormat, error);
}

void BrickViewer::shutdown() {
    m_renderer.shutdown();
    if (m_imgui) {
        m_gui.shutdown();
        ImGui::DestroyContext();
        m_imgui = false;
    }
}

// The stage of the PC builder (Stage.cpp): floor with the stud grid, cool rim light, the palette materials, the sky.
void BrickViewer::setupStage() {
    m_scene = vp3d::Scene();
    m_scene.name = "BrickIt";
    m_scene.unitsPerMeter = 100.0f;
    m_scene.gridScale = 0.8f;
    addGround(m_scene, 2000.0f, true, true);
    addRimLight(m_scene);
    m_colorMat = addPaletteMaterials(m_scene, m_lib, true);
    m_firstBrick = (int)m_scene.objects.size();
    m_partMesh.assign(m_lib.parts.size(), -1);
    if (m_opt.hdri != "none") {
        auto img = std::make_shared<vp3d::EnvImage>();
        std::string err;
        bool ok = true;
        if (m_opt.hdri == "sky") vp3d::makeProceduralSky(*img);
        else ok = vp3d::loadEnvImage(plat::fromUtf8(m_opt.hdri), *img, &err);
        if (ok) {
            m_scene.env.hdri = img;
            m_scene.env.hdriRotation = vp3d::radians(m_opt.hdriRotation);
            m_scene.env.hdriIntensity = m_opt.hdriIntensity;
        } else {
            m_log += "\nHDRI: " + err;
        }
    }
}

// One mesh per part (loaded once), one object per brick (BrickApp::rebuildScene).
void BrickViewer::rebuildObjects() {
    m_scene.objects.resize(m_firstBrick);
    m_shown = 0;
    for (const Brick& b : m_build.bricks) {
        vp3d::Object o;
        o.useRot = true;
        o.position = b.pos;
        o.rot = b.rot;
        o.material = m_colorMat.empty() ? 0 : m_colorMat[std::clamp(b.color, 0, (int)m_colorMat.size() - 1)];
        Part* p = b.part >= 0 ? m_lib.get(m_lib.parts[b.part].name) : nullptr;
        if (p && p->loaded) {
            if (m_partMesh[b.part] < 0) m_partMesh[b.part] = m_scene.addMesh(p->name, p->mesh);
            o.mesh = m_partMesh[b.part];
            ++m_shown;
        } else {
            o.visible = false;
        }
        m_scene.addObject(o);
    }
    ++m_scene.objectsVersion;
}

bool BrickViewer::openScene(const std::string& path, std::string* message) {
    std::string text;
    if (!readText(path, text)) {
        if (message) *message = "cannot read " + path;
        return false;
    }
    std::string msg;
    if (!m_build.fromJson(text, &m_scene.camera, &msg)) {
        if (message) *message = msg;
        return false;
    }
    rebuildObjects();
    m_log += "\nScene: " + std::to_string(m_build.bricks.size()) + " bricks, " + std::to_string(m_shown) + " shown (" +
             std::to_string(m_build.lastSkipped) + " parts not in the library)";
    return true;
}

// count random base bricks and plates in piles of 8 (instancing benchmark)
void BrickViewer::makeBench(int count, int seed) {
    std::mt19937 rng(seed);
    std::vector<int> parts, colors;
    for (int i = 0; i < (int)m_lib.parts.size(); ++i)
        if (m_lib.parts[i].base && m_lib.parts[i].loaded && (m_lib.parts[i].category == "Brick" || m_lib.parts[i].category == "Plate"))
            parts.push_back(i);
    if (parts.empty())
        for (int i = 0; i < (int)m_lib.parts.size(); ++i)
            if (m_lib.parts[i].loaded) parts.push_back(i);
    for (int c = 0; c < (int)m_lib.palette.size(); ++c)
        if (m_lib.palette[c].group == "standard" || m_lib.palette[c].group.empty()) colors.push_back(c);
    m_build.bricks.clear();
    if (parts.empty()) return;
    const int side = std::max(1, (int)std::ceil(std::sqrt(count / 8.0)));
    for (int i = 0; i < count; ++i) {
        Brick k;
        k.part = parts[rng() % parts.size()];
        k.color = colors.empty() ? 0 : colors[rng() % colors.size()];
        const int cell = i / 8, level = i % 8;
        k.pos = float3((cell % side) * 4 * kPitch, level * 3 * kPlate, (cell / side) * 4 * kPitch);
        m_build.bricks.push_back(k);
    }
    rebuildObjects();
    frameAll();
    m_log += "\nBench: " + std::to_string(count) + " bricks";
}

void BrickViewer::frameAll() {
    float3 mn(1e9f), mx(-1e9f);
    bool any = false;
    for (const Brick& b : m_build.bricks) {
        if (b.part < 0 || !m_lib.parts[b.part].loaded) continue;
        const Box bb = m_build.aabbOf(b.part, b.rot, b.pos);
        mn = vp3d::minv(mn, bb.mn);
        mx = vp3d::maxv(mx, bb.mx);
        any = true;
    }
    if (!any) return;
    vp3d::Camera& c = m_scene.camera;
    c.target = (mn + mx) * 0.5f;
    c.distance = vp3d::length(mx - mn) * 1.1f + 5.0f;
    c.yaw = vp3d::radians(35.0f);
    c.pitch = vp3d::radians(28.0f);
}

void BrickViewer::orbit(float dx, float dy) {
    vp3d::Camera& c = m_scene.camera;
    const float k = 0.005f / std::max(m_opt.uiScale, 1.0f);
    c.yaw -= dx * k;
    c.pitch = vp3d::clampf(c.pitch + dy * k, vp3d::radians(-5.0f), vp3d::radians(89.0f));
}

void BrickViewer::hud(float width, float height) {
    if (!m_opt.hud) return;
    const float s = m_opt.uiScale;
    ImGui::SetNextWindowPos(ImVec2(16 * s, 16 * s), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("BrickIt", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("BrickIt iPad (Lit) - %d briques - %.0f i/s - %u appels de dessin", m_shown, m_fps, m_renderer.drawCalls());
    if (ImGui::Button("Maison 31153")) {
        std::string err;
        openScene(m_opt.assets + "/default_scene.v3d.json", &err);
    }
    ImGui::SameLine();
    if (ImGui::Button("1000 briques")) makeBench(1000);
    ImGui::SameLine();
    if (ImGui::Button("5000 briques")) makeBench(5000);
    ImGui::SameLine();
    if (ImGui::Button("Cadrer")) frameAll();
    ImGui::Checkbox("GI douce (sondes raster)", &m_settings.ddgi);
    ImGui::End();
    (void)width;
    (void)height;
}

void BrickViewer::frame(Diligent::ITextureView* output, uint32_t width, uint32_t height, float dt, const TouchCamera& cam) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DeltaTime = dt > 0.0f ? dt : 1.0f / 60.0f;
    m_fpsTime += io.DeltaTime;
    if (++m_fpsFrames >= 30) {
        m_fps = m_fpsFrames / std::max(m_fpsTime, 1e-3f);
        m_fpsFrames = 0;
        m_fpsTime = 0;
    }
    ImGui::NewFrame();
    // camera: one finger (mouse) dragged on the 3D view orbits; two fingers move, pinch and turn
    if (!io.WantCaptureMouse && ImGui::IsMouseDragging(0, 2.0f)) orbit(io.MouseDelta.x, io.MouseDelta.y);
    if (cam.twoFingers) {
        vp3d::Camera& c = m_scene.camera;
        if (cam.pinch > 0.0f) c.distance = vp3d::clampf(c.distance / cam.pinch, 2.0f, 2000.0f);
        c.yaw -= cam.twist;
        float3 r, u, f;
        c.basis(r, u, f);
        const float k = c.distance * 2.0f * std::tan(c.fovY * 0.5f) / std::max((float)height, 1.0f);
        c.target = c.target - r * (cam.moveX * k) + u * (cam.moveY * k);
    }
    hud((float)width, (float)height);
    ImGui::Render();
    m_renderer.render(m_scene, m_settings, output, width, height);
    m_gui.render(ImGui::GetDrawData(), output);
}

} // namespace brickit
