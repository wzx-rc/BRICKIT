#include "ArchApp.h"
#include "ArchJoinery.h"
#include "ArchUi.h"
#include "Platform.h"
#include "Image.h"
#include "vp3d/EnvImage.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace archviz {

using namespace vp3d;

static float3 linHex(int hex) {
    return float3(srgbToLinear(((hex >> 16) & 255) / 255.0f), srgbToLinear(((hex >> 8) & 255) / 255.0f), srgbToLinear((hex & 255) / 255.0f));
}
static std::string fmtm(float v) { char b[32]; snprintf(b, sizeof b, "%.2f", v); return b; }

static bool evalNumber(const char* s, float& out) {   // "3.2+0.15", "3,20", "320cm", "2400 mm"
    std::string t;
    for (const char* p = s; *p; ++p) if (*p != ' ') t += (*p == ',') ? '.' : *p;
    float scale = 1.0f;
    auto ends = [&](const char* suf) { const size_t n = strlen(suf); return t.size() > n && t.compare(t.size() - n, n, suf) == 0; };
    if (ends("mm")) { scale = 0.001f; t.resize(t.size() - 2); }
    else if (ends("cm")) { scale = 0.01f; t.resize(t.size() - 2); }
    else if (ends("m")) t.resize(t.size() - 1);
    if (t.empty()) return false;
    float sum = 0;
    size_t i = 0;
    int sign = 1;
    bool any = false;
    while (i < t.size()) {
        if (t[i] == '+') { sign = 1; ++i; continue; }
        if (t[i] == '-') { sign = -1; ++i; continue; }
        char* e = nullptr;
        const float v = std::strtof(t.c_str() + i, &e);
        if (e == t.c_str() + i) return false;
        sum += sign * v;
        any = true;
        i = e - t.c_str();
        sign = 1;
    }
    if (!any) return false;
    out = sum * scale;
    return true;
}

void parseOptions(const std::vector<std::wstring>& args, Options& o) {
    for (size_t i = 1; i < args.size(); ++i) {
        const std::wstring& a = args[i];
        auto next = [&]() { return i + 1 < args.size() ? args[++i] : std::wstring(); };
        auto nextA = [&]() { return brickit::plat::toUtf8(next()); };
        if (a == L"--shot") o.shotPath = next();
        else if (a == L"--frames") o.shotFrames = std::stoi(next());
        else if (a == L"--spp") o.shotSpp = std::stoi(next());
        else if (a == L"--size") { const std::wstring s = next(); const size_t x = s.find(L'x'); if (x != std::wstring::npos) { o.width = std::stoi(s.substr(0, x)); o.height = std::stoi(s.substr(x + 1)); } }
        else if (a == L"--ui-scale") o.uiScale = std::stof(next());
        else if (a == L"--view") o.view = nextA();
        else if (a == L"--quality") o.quality = nextA();
        else if (a == L"--env") o.env = nextA();
        else if (a == L"--sun-rot") o.sunRot = std::stof(next());
        else if (a == L"--script") o.script = nextA();
        else if (a == L"--demo") o.demo = true;
        else if (a == L"--clean") o.clean = true;
        else if (a == L"--save-to") o.saveTo = next();
        else if (a == L"--cam") { o.camYaw = std::stof(next()); o.camPitch = std::stof(next()); o.camDist = std::stof(next()); }
        else if (a.size() && a[0] != L'-') o.file = a;
    }
}

// ---------------------------------------------------------------------------- init

bool ArchApp::initCommon(const Options& opt, std::unique_ptr<brickit::GfxRenderer> r, std::unique_ptr<brickit::GfxGui> g,
                         const std::wstring& exeDir, float dpi) {
    m_opt = opt;
    m_r = std::move(r);
    m_g = std::move(g);
    m_exeDir = exeDir;
    m_dpi = dpi;
    m_ui = opt.uiScale > 0 ? opt.uiScale : dpi;
    m_view = opt.view == "3d" ? kView3D : opt.view == "plan" ? kViewPlan : kViewSplit;
    m_quality = opt.quality == "rt" ? 1 : opt.quality == "dlss" ? 2 : 0;

    const std::wstring dir = brickit::plat::dataDir() + L"/ArchViz";
    brickit::plat::makeDir(dir);
    m_savePath = dir + L"/last.archviz.json";
    brickit::plat::makeDir(dir + L"/models");
    scanAssets(dir + L"/models", m_assets);
    if (opt.shotPath.empty()) loadPrefs();   // (captures: the defaults)
    bool loaded = false;
    if (!opt.file.empty()) { loaded = load(opt.file); if (loaded) m_savePath = opt.file; }   // (an opened building is saved in its own file)
    else if (!opt.demo && opt.shotPath.empty()) loaded = load(m_savePath);
    if (!loaded) m_b = Building::demoHouse();
    m_b.update();

    setupLook();
    frameAll();
    if (opt.camYaw > -999) { m_scene.camera.yaw = radians(opt.camYaw); m_scene.camera.pitch = radians(opt.camPitch); m_scene.camera.distance = opt.camDist; }

    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 14 * m_ui;
    st.FrameRounding = 7 * m_ui;
    st.PopupRounding = 10 * m_ui;
    st.GrabRounding = 6 * m_ui;
    st.WindowBorderSize = 1;
    st.FrameBorderSize = 1;
    st.WindowPadding = ImVec2(10 * m_ui, 10 * m_ui);
    st.FramePadding = ImVec2(8 * m_ui, 5 * m_ui);
    st.ItemSpacing = ImVec2(8 * m_ui, 7 * m_ui);
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(1, 1, 1, 0.92f);
    c[ImGuiCol_PopupBg] = ImVec4(1, 1, 1, 0.98f);
    c[ImGuiCol_Border] = ImVec4(20 / 255.f, 30 / 255.f, 50 / 255.f, 0.10f);
    c[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(kInk);
    c[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(kInk3);
    c[ImGuiCol_FrameBg] = ImVec4(1, 1, 1, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.97f, 0.98f, 1, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.95f, 0.97f, 1, 1);
    c[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(kHollow);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.88f, 0.91f, 0.96f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.86f, 0.98f, 1);
    c[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(kAccent);
    c[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(kAccent);
    c[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(kAccent);
    c[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(kAccentSoft);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.90f, 1, 1);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.18f, 0.42f, 1, 0.25f);
    c[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(kLine);
    m_sceneDirty = true;
    return true;
}

void ArchApp::shutdownCommon() {
    if (m_opt.shotPath.empty()) save(m_savePath);
    savePrefs();
    if (m_r) m_r->waitIdle();
    if (m_g) for (ImTextureID t : m_thumbs) if (t) m_g->releaseTexture(t);
    m_thumbs.clear();
    if (m_g) m_g->shutdown();
    if (m_r) m_r->shutdown();
}

void ArchApp::setupLook() {
    Environment& e = m_scene.env;
    e.top = linHex(0xcfe3f5);
    e.horizon = linHex(0xf2f6fa);
    e.bottom = linHex(0xdfe5ec);
    e.hemiSky = linHex(0xcfe3f5);
    e.hemiGround = linHex(0xdfe3e8);
    e.hemiIntensity = 0.5f;
    m_env = m_opt.env == "studio" ? 0 : m_opt.env == "sun" ? 1 : (m_r->pathTracing() ? 1 : 0);
    m_envRot = m_opt.sunRot >= 0 ? m_opt.sunRot : (m_env == 1 ? 250.0f : 200.0f);
    applyEnv();
    m_scene.gridScale = 1.0f;
    m_scene.gridLine = (34u << 24) | (0xa8u << 16) | (0x96u << 8) | 0x8au;
    m_scene.gridFade = float3(6, -4.5f, 14);
    Camera& cam = m_scene.camera;
    cam.fovY = radians(35.0f);
    cam.yaw = radians(38.0f);
    cam.pitch = radians(38.0f);

    m_rs.exposure = 1.0f;
    m_rs.tonemapper = 0;
    m_rs.vignette = 0.06f;
    m_rs.denoise = true;
    m_rs.denoiseEdgeLuma = 4.0f;
    m_rs.denoisePasses = 4;
    m_rs.fireflyClamp = 8.0f;
    m_rs.filterGlossy = 0.15f;
    m_rs.maxBounces = 8;   // (glass: the light comes in through two panes and back)
    m_rs.idPass = false;
    m_rs.selectionStyle = 2;
    m_rs.editorGrid = 0.0f;
}

// The environment: its HDRI (read once), turned around the building, and the sun where the image has its own (the
// brightest spot when it stands out, as BrickIt), so that the traced sun shadows follow the sky.
void ArchApp::applyEnv() {
    Environment& e = m_scene.env;
    const int k = m_env;
    if (!m_envImg[k]) {
        auto img = std::make_shared<EnvImage>();
        std::string err;
        const std::wstring hdr = m_exeDir + (k == 1 ? L"/assets/hdri/kloofendal_48d_partly_cloudy_puresky_2k.hdr" : L"/assets/hdri/studio_small_09_2k.hdr");
        if (!brickit::plat::fileExists(hdr) || !loadEnvImage(hdr, *img, &err)) makeProceduralSky(*img);
        float peak = 0;
        double mean = 0;
        uint32_t px = 0, py = 0;
        for (uint32_t y = 0; y < img->height; ++y)
            for (uint32_t x = 0; x < img->width; ++x) {
                const float3 c = img->texel(x, y);
                const float l = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
                mean += l;
                if (l > peak) { peak = l; px = x; py = y; }
            }
        mean /= std::max(1.0, (double)img->width * img->height);
        m_envHasSun[k] = img->width > 0 && peak > 12.0f * (float)mean;
        m_envSunAz[k] = ((px + 0.5f) / std::max(1u, img->width) - 0.5f) * 2.0f * kPi;
        m_envSunEl[k] = kPi * 0.5f - (py + 0.5f) / std::max(1u, img->height) * kPi;
        m_envImg[k] = img;
    }
    e.hdri = m_envImg[k];
    e.hdriIntensity = 1.0f;
    e.hdriRotation = radians(m_envRot);
    e.visibleToCamera = k == 1;   // the sky behind the building; the studio keeps the plain background of the maquette
    m_scene.sun.enabled = true;
    if (m_envHasSun[k]) {
        const float phi = m_envSunAz[k] + radians(m_envRot), el = std::clamp(m_envSunEl[k], radians(15.0f), radians(75.0f));
        m_scene.sun.direction = normalize(float3(std::sin(phi) * std::cos(el), std::sin(el), -std::cos(phi) * std::cos(el)));
    } else {
        const float az = radians(135.0f + m_envRot - 200.0f), el = radians(38.0f);
        m_scene.sun.direction = normalize(float3(std::cos(el) * std::cos(az), std::sin(el), std::cos(el) * std::sin(az)));
    }
    m_scene.sun.color = linHex(k == 1 ? 0xfff1e0 : 0xfff3e4);
    m_scene.sun.intensity = k == 1 ? 3.4f : 3.0f;
    if (m_r) m_r->resetAccumulation();
}

void ArchApp::renderSettings(RenderSettings& rs) {
    rs = m_rs;
    const bool moving = ImGui::GetTime() - m_lastMove < 0.25 && m_opt.shotPath.empty();
    if (m_quality == 0) {   // HQ: the BrickIt HQ (raster, shadows / occlusion / reflections traced, rounded edges)
        rs.raster = true;
        rs.msaaSamples = 4;
        rs.rasterRT = true;
        rs.rasterAO = 0.3f;   // metres: contact occlusion only (2 rays a pixel on a fixed pattern: a long range dithers the walls)
        rs.reflSamples = 1;
        rs.reflMaxRough = 0.35f;
        rs.rasterBevel = true;
        rs.ssao = false;
        rs.stillAA = true;
        rs.samplesPerFrame = 1;
        rs.rasterSSS = 0.0f;
        rs.bloom = 0.25f;
    } else {                // Ray tracing (1) / RT DLSS (2): the path tracer of WZX Engine
        rs.oidn = rs.dlss = m_quality == 2;   // Ray tracing: the a-trous filter; RT DLSS: OIDN still, DLSS Ray Reconstruction moving (as BrickIt)
        rs.adaptive = false;
        rs.raster = false;
        rs.ddgi = false;
        rs.traceStride = moving ? 4 : 1;
        rs.samplesPerFrame = moving ? 1 : (m_opt.shotPath.empty() ? 4 : 8);
    }
}

void ArchApp::frameAll() {
    float2 lo(1e9f, 1e9f), hi(-1e9f, -1e9f);
    for (const Node& n : m_b.nodes) if (n.alive) { lo = float2(std::min(lo.x, n.p.x), std::min(lo.y, n.p.y)); hi = float2(std::max(hi.x, n.p.x), std::max(hi.y, n.p.y)); }
    if (lo.x > hi.x) { lo = float2(-5, -5); hi = float2(5, 5); }
    const float2 c = (lo + hi) * 0.5f;
    const float size = std::max(4.0f, std::max(hi.x - lo.x, hi.y - lo.y));
    Camera& cam = m_scene.camera;
    cam.target = world(c, m_b.levels[m_level].elevation + 0.6f);
    cam.distance = size * 1.9f + 4.0f;
    m_planCentre = c;
    const float pw = std::max(200.0f, m_rp.z - m_rp.x), ph = std::max(200.0f, m_rp.w - m_rp.y);
    m_planScale = std::min(pw, ph) * 0.62f / size;
    if (m_rp.z <= m_rp.x) m_planScale = 40 * m_ui;
    m_lastMove = ImGui::GetCurrentContext() ? ImGui::GetTime() : 0;
}

// ---------------------------------------------------------------------------- persistence, undo

void ArchApp::save(const std::wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return;
    const std::string s = m_b.toJson();
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
}

bool ArchApp::load(const std::wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) return false;
    std::string s;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
    fclose(f);
    Building b;
    if (!b.fromJson(s)) return false;
    m_b = b;
    return true;
}

void ArchApp::pushUndo() {
    m_undo.push_back(m_b.toJson());
    if (m_undo.size() > 200) m_undo.erase(m_undo.begin());
    m_redo.clear();
}

void ArchApp::undo() {
    if (m_undo.empty()) return;
    m_redo.push_back(m_b.toJson());
    m_b.fromJson(m_undo.back());
    m_undo.pop_back();
    m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1;
    m_level = std::min(m_level, (int)m_b.levels.size() - 1);
    edited();
}

void ArchApp::redo() {
    if (m_redo.empty()) return;
    m_undo.push_back(m_b.toJson());
    m_b.fromJson(m_redo.back());
    m_redo.pop_back();
    m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1;
    edited();
}

void ArchApp::edited() {
    m_b.update();
    m_sceneDirty = true;
    m_lastMove = ImGui::GetTime();
    if (m_opt.shotPath.empty()) save(m_savePath);
}

void ArchApp::touched() {
    m_r->resetAccumulation();
    m_lastMove = ImGui::GetTime();
    if (m_opt.shotPath.empty()) save(m_savePath);
}

void ArchApp::toast(const std::string& s) { m_toast = s; m_toastT = ImGui::GetTime(); }

// ---------------------------------------------------------------------------- furnish: the objects of the library

// The assets used by the items and not in the scene yet: read once, kept at the head of the scene (the building is
// rebuilt after them).
void ArchApp::ensureAssets() {
    for (const Item& it : m_b.items) {
        for (AssetInfo& a : m_assets) {
            if (a.slug != it.asset || a.firstMesh >= 0 || a.partCount < 0) continue;
            m_scene.objects.clear();
            m_scene.meshes.resize(m_map.keepMeshes);
            m_scene.meshNames.resize(m_map.keepMeshes);
            m_scene.materials.resize(m_map.keepMaterials);
            std::string err;
            if (!loadAsset(a, m_scene, &err)) { a.partCount = -1; toast(err); }   // (-1: not tried again)
            m_map.keepMeshes = (int)m_scene.meshes.size();
            m_map.keepMaterials = (int)m_scene.materials.size();
            m_sceneDirty = true;
        }
    }
}

int ArchApp::itemAt(float2 p) const {
    int best = -1;
    float bestArea = 1e30f;
    for (int i = 0; i < (int)m_b.items.size(); ++i) {
        const Item& it = m_b.items[i];
        if (it.level != m_level) continue;
        const float2 ax(std::cos(it.yaw), std::sin(it.yaw)), d = p - it.p;
        if (std::fabs(dot2(d, ax)) > it.size.x * 0.5f + m_tol * 0.3f || std::fabs(dot2(d, perp(ax))) > it.size.y * 0.5f + m_tol * 0.3f) continue;
        const float area = it.size.x * it.size.y;
        if (area < bestArea) { bestArea = area; best = i; }
    }
    return best;
}

float2 ArchApp::snapItem(const Item& it, float2 p, bool free) const {
    if (free) return p;
    p = float2(gridSnap(p.x), gridSnap(p.y));
    const float2 ax(std::cos(it.yaw), std::sin(it.yaw)), ay = perp(ax);
    const float hx = it.size.x * 0.5f, hy = it.size.y * 0.5f;
    for (int pass = 0; pass < 2; ++pass)   // (twice: a corner has two walls)
        for (int w = 0; w < (int)m_b.walls.size(); ++w) {
            const Wall& W = m_b.walls[w];
            if (!W.alive || m_b.nodes[W.a].level != it.level) continue;
            const WallGeom& g = m_b.geom[w];
            const float along = dot2(p - g.a, g.u), d = dot2(p - g.a, g.n);
            const float extU = hx * std::fabs(dot2(ax, g.u)) + hy * std::fabs(dot2(ay, g.u));
            if (along < -extU * 0.5f || along > g.len + extU * 0.5f) continue;
            const float ext = hx * std::fabs(dot2(ax, g.n)) + hy * std::fabs(dot2(ay, g.n));
            const float gap = std::fabs(d) - W.thickness * 0.5f - ext;
            if (gap < 0.20f && gap > -ext) p = p + g.n * ((d >= 0 ? 1.0f : -1.0f) * -gap);   // flush against the face
        }
    return p;
}

void ArchApp::addItem(int asset, float2 p, float yaw) {
    if (asset < 0 || asset >= (int)m_assets.size()) return;
    const AssetInfo& a = m_assets[asset];
    pushUndo();
    Item it;
    it.asset = a.slug;
    it.yaw = yaw;
    it.level = m_level;
    it.size = float2(a.size.x, a.size.z);
    it.height = a.size.y;
    it.p = p;
    m_b.items.push_back(it);
    m_selKind = 4; m_selWall = m_selOp = m_selRoom = -1; m_selItem = (int)m_b.items.size() - 1;
    edited();
}

// ---------------------------------------------------------------------------- view transforms

ImVec2 ArchApp::planToScreen(float2 p) const {
    return ImVec2((m_rp.x + m_rp.z) * 0.5f + (p.x - m_planCentre.x) * m_planScale, (m_rp.y + m_rp.w) * 0.5f - (p.y - m_planCentre.y) * m_planScale);
}

float2 ArchApp::screenToPlan(ImVec2 s) const {
    return float2(m_planCentre.x + (s.x - (m_rp.x + m_rp.z) * 0.5f) / m_planScale, m_planCentre.y - (s.y - (m_rp.y + m_rp.w) * 0.5f) / m_planScale);
}

bool ArchApp::project(float2 p, float z, ImVec2& out) const {
    const float w = m_r3.z - m_r3.x, h = m_r3.w - m_r3.y;
    if (w <= 0 || h <= 0) return false;
    float2 ndc;
    float depth;
    if (!m_scene.camera.project(world(p, m_b.levels[m_level].elevation + z), w / h, ndc, &depth) || depth <= 0.05f) return false;
    out = ImVec2(m_r3.x + (ndc.x * 0.5f + 0.5f) * w, m_r3.y + (0.5f - ndc.y * 0.5f) * h);
    return true;
}

bool ArchApp::rayToPlan(ImVec2 s, float2& out, float elev) const {
    const float w = m_r3.z - m_r3.x, h = m_r3.w - m_r3.y;
    if (w <= 0 || h <= 0) return false;
    float3 o, d;
    m_scene.camera.ray((s.x - m_r3.x) / w * 2 - 1, 1 - (s.y - m_r3.y) / h * 2, w / h, o, d);
    if (d.y > -1e-4f) return false;
    const float t = (elev - o.y) / d.y;
    if (t <= 0) return false;
    const float3 p = o + d * t;
    out = float2(p.x, -p.z);
    return true;
}

void ArchApp::cameraInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (m_uiHover && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) return;
    const ImVec2 md = io.MouseDelta;
    static int dragView = 0;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !(io.KeyAlt && ImGui::IsMouseDown(ImGuiMouseButton_Left))) dragView = 0;
    else if (dragView == 0) dragView = inRect(io.MousePos, m_rp) ? 2 : inRect(io.MousePos, m_r3) ? 1 : 0;
    const int over = inRect(io.MousePos, m_rp) ? 2 : inRect(io.MousePos, m_r3) ? 1 : 0;
    Camera& cam = m_scene.camera;
    const float kNear = 0.5f;   // the closest orbit: there the view is "inside" (it looks around from the eye and walks)
    if (over == 1 && io.MouseWheel != 0 && !m_uiHover) {
        float3 r, u, f;
        cam.basis(r, u, f);
        if (io.MouseWheel > 0 && cam.distance <= kNear * 1.01f) {
            cam.target = cam.target + f * (0.45f * io.MouseWheel);   // at the closest: the wheel walks forward
        } else {
            const float d0 = cam.distance;
            cam.distance = std::clamp(d0 * std::pow(0.88f, io.MouseWheel), kNear, 400.0f);
            if (cam.distance > kNear * 1.01f) cam.pitch = std::max(cam.pitch, radians(-20.0f));   // (out of a room: the eye back above the floor)
            // towards the floor point under the mouse: the view closes in on what is pointed at (and so enters a room)
            float2 hit;
            if (io.MouseWheel > 0 && rayToPlan(io.MousePos, hit, m_b.levels[m_level].elevation)) {
                const float3 h = world(hit, cam.target.y);
                const float3 dlt = h - cam.target;
                if (length(dlt) < d0 * 3.0f) cam.target = cam.target + dlt * (1.0f - cam.distance / d0);
            }
        }
        m_lastMove = ImGui::GetTime();
    }
    // arrow keys: walk on the floor (forward / back / sideways), Page Up / Down: the height of the eye
    if (m_view != kViewPlan && !io.WantTextInput && !io.KeyCtrl) {
        const float fwd = (ImGui::IsKeyDown(ImGuiKey_UpArrow) ? 1.0f : 0.0f) - (ImGui::IsKeyDown(ImGuiKey_DownArrow) ? 1.0f : 0.0f);
        const float side = (ImGui::IsKeyDown(ImGuiKey_RightArrow) ? 1.0f : 0.0f) - (ImGui::IsKeyDown(ImGuiKey_LeftArrow) ? 1.0f : 0.0f);
        const float up = (ImGui::IsKeyDown(ImGuiKey_PageUp) ? 1.0f : 0.0f) - (ImGui::IsKeyDown(ImGuiKey_PageDown) ? 1.0f : 0.0f);
        if (fwd != 0 || side != 0 || up != 0) {
            const float v = (io.KeyShift ? 6.0f : 2.2f) * std::min(io.DeltaTime, 0.05f);
            const float3 f(-std::sin(cam.yaw), 0, -std::cos(cam.yaw)), r(std::cos(cam.yaw), 0, -std::sin(cam.yaw));
            cam.target = cam.target + f * (fwd * v) + r * (side * v) + float3(0, up * v * 0.6f, 0);
            m_lastMove = ImGui::GetTime();
        }
    }
    if (over == 2 && io.MouseWheel != 0 && !m_uiHover) {
        const float2 before = screenToPlan(io.MousePos);
        m_planScale = std::clamp(m_planScale * std::pow(1.15f, io.MouseWheel), 4.0f, 2000.0f);
        const float2 after = screenToPlan(io.MousePos);
        m_planCentre = m_planCentre + (before - after);
    }
    if (dragView == 0 || (md.x == 0 && md.y == 0)) return;
    const bool pan = ImGui::IsMouseDown(ImGuiMouseButton_Middle) || (io.KeyShift && ImGui::IsMouseDown(ImGuiMouseButton_Right));
    if (dragView == 2) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ImGui::IsMouseDown(ImGuiMouseButton_Right))
            m_planCentre = m_planCentre - float2(md.x / m_planScale, -md.y / m_planScale);
        return;
    }
    if (pan) {
        float3 r, u, f;
        cam.basis(r, u, f);
        const float k = cam.distance * 0.0016f / m_ui;
        cam.target = cam.target - r * (md.x * k) + u * (md.y * k);
    } else {
        const bool inside = cam.distance <= kNear * 1.01f;   // inside: the eye stays where it is, the view turns around it
        const float3 eye = cam.eye();
        cam.yaw -= md.x * 0.006f;
        cam.pitch = std::clamp(cam.pitch + md.y * 0.006f, radians(inside ? -75.0f : -20.0f), radians(89.0f));
        if (inside) cam.target = cam.target + (eye - cam.eye());
    }
    m_lastMove = ImGui::GetTime();
}

// ---------------------------------------------------------------------------- snapping

float ArchApp::gridSnap(float v) const {
    if (!m_snapGrid || m_gridStep <= 0 || ImGui::GetIO().KeyShift) return v;
    return std::round(v / m_gridStep) * m_gridStep;
}

// The preferences of the interface (snapping, library): "key value" lines in ArchViz/prefs.txt.
void ArchApp::loadPrefs() {
    FILE* f = nullptr;
    if (_wfopen_s(&f, (brickit::plat::dataDir() + L"/ArchViz/prefs.txt").c_str(), L"rb") != 0 || !f) return;
    char key[64];
    float v;
    while (fscanf(f, "%63s %f", key, &v) == 2) {
        const std::string k = key;
        if (k == "snapGrid") m_snapGrid = v != 0;
        else if (k == "snapAlign") m_snapAlign = v != 0;
        else if (k == "gridStep") m_gridStep = std::clamp(v, 0.01f, 5.0f);
        else if (k == "rotStep") m_rotStep = std::clamp((int)v, 0, 90);
        else if (k == "libDock") m_libDock = std::clamp((int)v, 0, 2);
        else if (k == "libW") m_libW = std::clamp(v, 200.0f, 2000.0f);
        else if (k == "libH") m_libH = std::clamp(v, 120.0f, 1200.0f);
        else if (k == "libFold") m_libFold = v != 0;
    }
    fclose(f);
}

void ArchApp::savePrefs() {
    if (!m_opt.shotPath.empty()) return;
    FILE* f = nullptr;
    if (_wfopen_s(&f, (brickit::plat::dataDir() + L"/ArchViz/prefs.txt").c_str(), L"wb") != 0 || !f) return;
    fprintf(f, "snapGrid %d\nsnapAlign %d\ngridStep %g\nrotStep %d\nlibDock %d\nlibW %g\nlibH %g\nlibFold %d\n", m_snapGrid ? 1 : 0, m_snapAlign ? 1 : 0, m_gridStep,
            m_rotStep, m_libDock, m_libW, m_libH, m_libFold ? 1 : 0);
    fclose(f);
}

float2 ArchApp::snap(float2 raw, bool hasRef, float2 ref) {
    m_guides.clear();
    m_snapLabel.clear();
    if (ImGui::GetIO().KeyShift) return raw;
    const float tol = m_tol;
    // 1. a node
    int bn = -1;
    float bd = tol;
    for (int i = 0; i < (int)m_b.nodes.size(); ++i) {
        const Node& n = m_b.nodes[i];
        if (!n.alive || n.level != m_level) continue;
        const float d = len2(n.p - raw);
        if (d < bd) { bd = d; bn = i; }
    }
    if (bn >= 0) { m_snapLabel = "Endpoint"; return m_b.nodes[bn].p; }
    if (hasRef && len2(ref - raw) < tol) { m_snapLabel = "Start"; return ref; }
    // (the alignment assistant, an option: the middles of the walls, the alignments with the corners, the angles)
    if (m_snapAlign) {
        for (int w = 0; w < (int)m_b.walls.size(); ++w) {
            const Wall& W = m_b.walls[w];
            if (!W.alive || m_b.nodes[W.a].level != m_level) continue;
            const float2 mid = (m_b.nodes[W.a].p + m_b.nodes[W.b].p) * 0.5f;
            if (len2(mid - raw) < tol * 0.8f) { m_snapLabel = "Midpoint"; return mid; }
        }
    }
    // 2. alignments with the nodes (and the start point)
    float2 p = raw;
    bool ax = false, ay = false;
    float bx = tol * 0.7f, by = tol * 0.7f;
    float2 nx, ny;
    auto align = [&](float2 q) {
        if (std::fabs(raw.x - q.x) < bx) { bx = std::fabs(raw.x - q.x); p.x = q.x; ax = true; nx = q; }
        if (std::fabs(raw.y - q.y) < by) { by = std::fabs(raw.y - q.y); p.y = q.y; ay = true; ny = q; }
    };
    if (m_snapAlign) {
        for (const Node& n : m_b.nodes) if (n.alive && n.level == m_level) align(n.p);
        if (hasRef) align(ref);
    }
    // 3. angle from the start (15 degrees)
    bool angled = false;
    if (m_snapAlign && hasRef && !(ax && ay)) {
        const float2 d = raw - ref;
        const float L = len2(d);
        if (L > 0.05f) {
            const float step = radians(15.0f);
            const float a = std::round(std::atan2(d.y, d.x) / step) * step;
            const float2 dir(std::cos(a), std::sin(a));
            const float2 proj = ref + dir * dot2(d, dir);
            if (len2(proj - raw) < tol * 1.2f) {
                float l = gridSnap(dot2(d, dir));
                if (ax && std::fabs(dir.x) > 0.05f) l = (p.x - ref.x) / dir.x;        // the angle line meets the alignment
                else if (ay && std::fabs(dir.y) > 0.05f) l = (p.y - ref.y) / dir.y;
                p = ref + dir * l;
                angled = true;
                char b[32];
                snprintf(b, sizeof b, "Angle %d°", ((int)std::lround(degrees(a)) + 360) % 360);
                m_snapLabel = b;
                m_guides.push_back({ref, ref + dir * (l + 1.5f)});
            }
        }
    }
    if (angled) {
        if (ax) m_guides.push_back({nx, float2(nx.x, p.y)});
        if (ay) m_guides.push_back({ny, float2(p.x, ny.y)});
        return p;
    }
    // 4. on a wall (its axis)
    if (!ax && !ay) {
        float along;
        const int w = m_b.wallAt(raw, m_level, tol * 0.5f, &along);
        if (w >= 0) {
            const WallGeom& g = m_b.geom[w];
            const float s = std::clamp(gridSnap(along), 0.0f, g.len);
            m_snapLabel = "On wall";
            return g.a + g.u * s;
        }
    }
    // 5. the grid (5 cm) on the free coordinates
    if (!ax) p.x = gridSnap(raw.x);
    if (!ay) p.y = gridSnap(raw.y);
    if (!ax && !ay && m_snapGrid) m_snapLabel = "Grid";
    if (ax) { m_guides.push_back({nx, float2(nx.x, p.y)}); m_snapLabel = "Aligned"; }
    if (ay) { m_guides.push_back({ny, float2(p.x, ny.y)}); m_snapLabel = "Aligned"; }
    return p;
}

// ---------------------------------------------------------------------------- edits

void ArchApp::commitWall(float2 a, float2 b) {
    if (len2(b - a) < 0.05f) return;
    pushUndo();
    m_b.addWall(a, b, m_level, m_wallThick, m_b.levels[m_level].height);
    edited();
}

void ArchApp::commitRoom(float2 a, float2 b) {
    const float2 lo(std::min(a.x, b.x), std::min(a.y, b.y)), hi(std::max(a.x, b.x), std::max(a.y, b.y));
    if (hi.x - lo.x < 0.4f || hi.y - lo.y < 0.4f) return;
    pushUndo();
    const float h = m_b.levels[m_level].height;
    const float2 c[4] = {lo, float2(hi.x, lo.y), hi, float2(lo.x, hi.y)};
    for (int i = 0; i < 4; ++i) m_b.addWall(c[i], c[(i + 1) % 4], m_level, m_wallThick, h);
    const float2 mid = (lo + hi) * 0.5f;
    bool named = false;
    for (const RoomLabel& l : m_b.labels) if (l.alive && l.level == m_level && len2(l.p - mid) < 0.3f) named = true;
    if (!named) {
        int n = 1;
        for (const RoomLabel& l : m_b.labels) if (l.alive) ++n;
        m_b.labels.push_back({mid, "Room " + std::to_string(n), m_newFloor, m_level, true});
    }
    edited();
    m_selRoom = m_b.roomAt(mid, m_level);
    m_selKind = m_selRoom >= 0 ? 3 : 0;
    m_sceneDirty = true;
}

void ArchApp::pickSelect(float2 p) {
    m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1;
    m_selDirty = true;
    if (m_step == 1) {   // Furnish: the objects (the walls are edited in Build)
        int it = m_hoverView == 1 ? itemAtRay(m_mouse) : -1;   // (3D: the box under the mouse, else its footprint)
        if (it < 0) it = itemAt(p);
        if (it >= 0) { m_selKind = 4; m_selItem = it; }
        return;
    }
    float along;
    const int w = m_b.wallAt(p, m_level, m_tol * 0.6f, &along);
    if (w >= 0) {
        const Wall& W = m_b.walls[w];
        for (int i = 0; i < (int)W.ops.size(); ++i)
            if (std::fabs(along - W.ops[i].pos) <= W.ops[i].width * 0.5f) { m_selKind = 2; m_selWall = w; m_selOp = i; break; }
        if (!m_selKind) { m_selKind = 1; m_selWall = w; }
    } else {
        const int r = m_b.roomAt(p, m_level);
        if (r >= 0) { m_selKind = 3; m_selRoom = r; }
    }
}

void ArchApp::deleteSelection() {
    if (m_selKind == 4 && m_selItem >= 0 && m_selItem < (int)m_b.items.size()) {
        pushUndo();
        m_b.items.erase(m_b.items.begin() + m_selItem);
        m_selKind = 0; m_selItem = -1;
        edited();
        return;
    }
    if (m_selKind == 1 && m_selWall >= 0) { pushUndo(); m_b.removeWall(m_selWall); m_b.compact(); }
    else if (m_selKind == 2 && m_selWall >= 0) { pushUndo(); m_b.removeOpening(m_selWall, m_selOp); }
    else if (m_selKind == 3 && m_selRoom >= 0) {   // a room: its name (the walls stay, they may be shared)
        const int l = m_b.rooms[m_selRoom].label;
        if (l < 0) return;
        pushUndo();
        m_b.labels[l].alive = false;
        m_b.compact();
    } else return;
    m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1;
    edited();
}

// ---------------------------------------------------------------------------- numbers typed

void ArchApp::openEdit(int target, ImVec2 at, const char* initial) {
    m_edit = target;
    m_editAt = at;
    snprintf(m_editBuf, sizeof m_editBuf, "%s", initial ? initial : "");
    m_editFocus = true;
}

void ArchApp::applyEdit(float v) {
    const int t = m_edit;
    m_edit = kEditNone;
    if (v <= 0 && t != kEditOpLeft && t != kEditOpRight) return;
    switch (t) {
    case kEditLength: {
        float2 d = m_cur - m_start;
        if (len2(d) < 1e-3f) d = float2(1, 0);
        const float2 e = m_start + norm2(d) * v;
        commitWall(m_start, e);
        m_start = e;
        m_cur = e;
        break;
    }
    case kEditWidth: {
        m_typedW = v;
        openEdit(kEditDepth, m_editAt, "");
        break;
    }
    case kEditDepth: {
        const float2 d = m_cur - m_start;
        const float w = m_typedW > 0 ? m_typedW : std::fabs(d.x);
        const float sx = d.x < 0 ? -1.0f : 1.0f, sy = d.y < 0 ? -1.0f : 1.0f;
        commitRoom(m_start, m_start + float2(sx * w, sy * v));
        m_typedW = -1;
        m_drawing = false;
        break;
    }
    case kEditWallLen:
        if (m_selWall >= 0) { pushUndo(); m_b.setWallLength(m_selWall, v); edited(); }
        break;
    case kEditOpLeft: case kEditOpRight: case kEditOpWidth:
        if (m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0) {
            pushUndo();
            Opening& o = m_b.walls[m_selWall].ops[m_selOp];
            const WallGeom& g = m_b.geom[m_selWall];
            const float lo = std::max(g.sL0, g.sR0), hi = std::min(g.sL1, g.sR1);
            if (t == kEditOpLeft) o.pos = lo + v + o.width * 0.5f;
            else if (t == kEditOpRight) o.pos = hi - v - o.width * 0.5f;
            else { const float s0 = o.pos - o.width * 0.5f; o.width = v; o.pos = s0 + v * 0.5f; }
            m_b.clampOpening(m_selWall, o);
            edited();
        }
        break;
    }
}

ImVec4 ArchApp::pill(ImDrawList* dl, ImVec2 c, const char* text, int style, int target) {
    ImFont* f = m_g->fontBold ? m_g->fontBold : ImGui::GetFont();
    const float fs = 13.0f * m_ui;
    const ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0, text);
    const float ph = 22 * m_ui, pw = ts.x + 18 * m_ui;
    const ImVec2 a(c.x - pw * 0.5f, c.y - ph * 0.5f), b(c.x + pw * 0.5f, c.y + ph * 0.5f);
    ImU32 bg = kWhite, fg = kAccent, br = kAccent;
    if (style == 1) { bg = kAccent; fg = kWhite; br = kAccent; }
    if (style == 2) { bg = kWhite; fg = IM_COL32(194, 106, 10, 255); br = kDraw; }
    if (style == 3) { bg = kOk; fg = kWhite; br = kOk; }
    if (style == 4) { bg = IM_COL32(255, 255, 255, 235); fg = kInk; br = IM_COL32(20, 30, 50, 30); }
    dl->AddRectFilled(ImVec2(a.x, a.y + 2 * m_ui), ImVec2(b.x, b.y + 2 * m_ui), IM_COL32(20, 30, 50, 30), ph * 0.5f);
    dl->AddRectFilled(a, b, bg, ph * 0.5f);
    dl->AddRect(a, b, br, ph * 0.5f, 0, 1.2f * m_ui);
    dl->AddText(f, fs, ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), fg, text);
    const ImVec4 r(a.x, a.y, b.x, b.y);
    if (target != kEditNone) m_pills.push_back({r, target});
    return r;
}

// ---------------------------------------------------------------------------- tool input

void ArchApp::toolInput() {
    ImGuiIO& io = ImGui::GetIO();
    m_mouse = io.MousePos;
    m_hoverView = 0;
    const float elev = m_b.levels[m_level].elevation;
    // (an object dragged out of the library: Dear ImGui keeps the mouse while the button is down, the views take it back)
    if (!m_uiHover || (m_libDragPlace && !inRect(m_mouse, m_libRect))) {
        if (inRect(m_mouse, m_rp)) {
            m_hoverView = 2;
            m_raw = screenToPlan(m_mouse);
            m_tol = 11 * m_ui / m_planScale;
        } else if (inRect(m_mouse, m_r3) && rayToPlan(m_mouse, m_raw, elev)) {
            m_hoverView = 1;
            m_tol = m_scene.camera.pixelSize(world(m_raw, elev), m_r3.w - m_r3.y) * 11 * m_ui;
        }
    }
    // keys
    const bool typing = io.WantTextInput;
    if (!typing) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) { io.KeyShift ? redo() : undo(); }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) redo();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) { save(m_savePath); toast("Saved"); }
        if (!io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_V)) { m_tool = m_furnishTool = kSelect; m_drawing = false; }
            // Furnish: G moves, R turns (the manipulators); Build: R is the room
            if (ImGui::IsKeyPressed(ImGuiKey_G) && m_step == 1) m_tool = m_furnishTool = kMoveTool;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) { if (m_step == 1) m_tool = m_furnishTool = kRotateTool; else { m_tool = kRoom; m_drawing = false; } }
            if (ImGui::IsKeyPressed(ImGuiKey_W)) { m_tool = kWall; m_drawing = false; m_step = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_N)) { m_tool = kWindowTool; m_drawing = false; m_step = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_D)) { m_tool = kDoorTool; m_drawing = false; m_step = 0; }
            // an object turned: Q / E by 15 degrees, with Shift by 90
            const float turn = (ImGui::IsKeyPressed(ImGuiKey_Q) ? 1.0f : 0.0f) - (ImGui::IsKeyPressed(ImGuiKey_E) ? 1.0f : 0.0f);
            if (turn != 0) {
                const float a = turn * radians(io.KeyShift ? 90.0f : 15.0f);
                if (m_tool == kPlace) m_placeYaw += a;
                else if (m_selKind == 4 && m_selItem >= 0 && m_selItem < (int)m_b.items.size()) {
                    pushUndo();
                    m_b.items[m_selItem].yaw += a;
                    placeItem(m_scene, m_map, m_b, m_selItem);
                    touched();
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F)) frameAll();
            if (ImGui::IsKeyPressed(ImGuiKey_O) && m_selKind == 2) toggleOpening(m_selWall, m_selOp);   // open / close the selected opening
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) deleteSelection();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            if (m_drawing) m_drawing = false;
            else { m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1; m_selDirty = true; m_libInfo = -1; if (m_tool == kPlace) { m_tool = m_furnishTool; m_libDragPlace = false; } }
        }
        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && m_tool == kWall) m_drawing = false;
        // digits while drawing: the exact length / size
        if (m_drawing && m_edit == kEditNone && (m_tool == kWall || m_tool == kRoom)) {
            for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
                const ImWchar ch = io.InputQueueCharacters[i];
                if ((ch >= '0' && ch <= '9') || ch == '.' || ch == ',') {
                    const char s[2] = {(char)ch, 0};
                    openEdit(m_tool == kWall ? kEditLength : kEditWidth, ImVec2(m_mouse.x + 18 * m_ui, m_mouse.y + 14 * m_ui), s);
                    break;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Tab) && m_edit == kEditNone)
                openEdit(m_tool == kWall ? kEditLength : kEditWidth, ImVec2(m_mouse.x + 18 * m_ui, m_mouse.y + 14 * m_ui), "");
        }
    }
    // an object dragged out of the library: dropped where the button is released (outside the views: put back)
    if (m_libDragPlace && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_libDragPlace = false;
        if (m_tool == kPlace && m_hoverView != 0 && m_placeAsset >= 0 && m_placeAsset < (int)m_assets.size()) {
            Item it;
            it.yaw = m_placeYaw; it.level = m_level;
            it.size = float2(m_assets[m_placeAsset].size.x, m_assets[m_placeAsset].size.z);
            addItem(m_placeAsset, snapItem(it, m_raw, io.KeyShift), m_placeYaw);
        }
        if (m_tool == kPlace) m_tool = m_furnishTool;
        return;
    }
    if (joineryInput()) { m_moving = false; return; }   // (the handles of the leaves of the selected opening first)
    if (gizmoInput()) { m_moving = false; return; }   // (the manipulator first: its handles are over the objects)
    const bool furnishPick = m_step == 1 && (m_tool == kSelect || m_tool == kMoveTool || m_tool == kRotateTool);
    if (m_hoverView == 0) {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) m_moving = false;
        // (a view towards the horizon: no floor under the mouse, the objects are still picked)
        if (furnishPick && !m_uiHover && inRect(m_mouse, m_r3) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt) {
            const int it = itemAtRay(m_mouse);
            m_selKind = it >= 0 ? 4 : 0; m_selWall = m_selOp = m_selRoom = -1; m_selItem = it;
            m_selDirty = true;
        }
        return;
    }
    // a pill clicked: its value typed
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        for (const Pill& p : m_pillsPrev)
            if (inRect(m_mouse, p.rect)) {
                float v = 0;
                if (p.target == kEditWallLen && m_selWall >= 0) v = m_b.wallLength(m_selWall);
                char b[32];
                snprintf(b, sizeof b, "%.2f", v);
                openEdit(p.target, ImVec2(p.rect.x, p.rect.w + 4 * m_ui), p.target == kEditWallLen ? b : "");
                return;
            }
    }
    const bool hasRef = m_drawing && m_tool == kWall;
    m_cur = snap(m_raw, hasRef || (m_drawing && m_tool == kRoom), m_start);
    if (m_drawing && m_tool == kRoom) {   // a rectangle: no angle guides from the corner
        m_guides.erase(std::remove_if(m_guides.begin(), m_guides.end(), [&](const Guide& g) { return len2(g.a - m_start) < 1e-4f && m_snapLabel.rfind("Angle", 0) == 0; }), m_guides.end());
    }
    const bool lclick = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt;
    switch (m_tool) {
    case kSelect: case kMoveTool: case kRotateTool: {
        if (lclick) {
            pickSelect(m_raw);
            m_pressAt = m_mouse;
            m_moving = false;
            if (m_selKind == 4) {
                pushUndo();
                m_moving = true;
                m_dragged = false;
                m_moveOff = m_b.items[m_selItem].p - m_raw;
            } else if (m_selKind == 1 || m_selKind == 2) {
                pushUndo();
                m_moving = true;
                m_dragged = false;
                const WallGeom& g = m_b.geom[m_selWall];
                m_moveRef = m_selKind == 1 ? dot2(m_raw - g.a, g.n) : dot2(m_raw - g.a, g.u);
                m_start = m_raw;
            }
        }
        if (m_moving && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float dpx = std::hypot(m_mouse.x - m_pressAt.x, m_mouse.y - m_pressAt.y);
            if (dpx > 5 * m_ui) m_dragged = true;
            if (m_dragged && m_selKind == 4) {   // an object: its transform alone, nothing rebuilt
                Item& it = m_b.items[m_selItem];
                it.p = snapItem(it, m_raw + m_moveOff, io.KeyShift);
                placeItem(m_scene, m_map, m_b, m_selItem);
                m_r->resetAccumulation();
                m_lastMove = ImGui::GetTime();
            } else if (m_dragged) {
                m_b.fromJson(m_undo.back());   // from the state at the press: no drift
                const WallGeom g = m_b.geom[m_selWall];
                if (m_selKind == 1) {
                    float d = dot2(m_raw - g.a, g.n) - m_moveRef;
                    d = gridSnap(d);
                    m_b.moveWall(m_selWall, d);
                } else {
                    float d = dot2(m_raw - g.a, g.u) - m_moveRef;
                    d = gridSnap(d);
                    Opening& o = m_b.walls[m_selWall].ops[m_selOp];
                    o.pos += d;
                    m_b.update();
                    m_b.clampOpening(m_selWall, o);
                }
                m_b.update();
                m_sceneDirty = true;
                m_lastMove = ImGui::GetTime();
            }
        }
        if (m_moving && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_moving = false;
            if (!m_dragged) m_undo.pop_back();
            else if (m_selKind == 4) touched();
            else edited();
        }
        break;
    }
    case kPlace: {
        if (m_placeAsset < 0 || m_placeAsset >= (int)m_assets.size()) { m_tool = kSelect; break; }
        Item it;
        it.yaw = m_placeYaw; it.level = m_level;
        it.size = float2(m_assets[m_placeAsset].size.x, m_assets[m_placeAsset].size.z);
        m_placeAt = snapItem(it, m_raw, io.KeyShift);
        if (lclick && !m_libDragPlace) { addItem(m_placeAsset, m_placeAt, m_placeYaw); m_tool = m_furnishTool; }
        break;
    }
    case kRoom: {
        if (lclick) {
            if (!m_drawing) { m_drawing = true; m_dragged = false; m_start = m_cur; m_pressAt = m_mouse; m_typedW = -1; }
            else { commitRoom(m_start, m_cur); m_drawing = false; }
        }
        if (m_drawing && ImGui::IsMouseDown(ImGuiMouseButton_Left) && std::hypot(m_mouse.x - m_pressAt.x, m_mouse.y - m_pressAt.y) > 8 * m_ui) m_dragged = true;
        if (m_drawing && m_dragged && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_edit == kEditNone) { commitRoom(m_start, m_cur); m_drawing = false; }
        break;
    }
    case kWall: {
        if (lclick) {
            if (!m_drawing) { m_drawing = true; m_start = m_chainFirst = m_cur; }
            else {
                commitWall(m_start, m_cur);
                if (len2(m_cur - m_chainFirst) < 0.01f) m_drawing = false;
                else m_start = m_cur;
            }
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) m_drawing = false;
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && io.MouseDragMaxDistanceSqr[1] < 25 * m_ui * m_ui) m_drawing = false;
        break;
    }
    case kWindowTool: case kDoorTool: {
        m_hoverWall = m_b.wallAt(m_raw, m_level, m_tol, &m_hoverAlong);
        m_ghostOk = false;
        if (m_hoverWall >= 0) {
            Opening o;
            o.kind = m_tool == kDoorTool ? kDoor : kWindow;
            o.width = m_tool == kDoorTool ? m_doorW : m_winW;
            o.height = m_tool == kDoorTool ? m_doorH : m_winH;
            o.sill = m_tool == kDoorTool ? 0 : m_winSill;
            if (o.kind == kWindow && o.width >= 2.4f) { o.kind = kSlider; o.sill = 0; o.height = 2.25f; }
            o.pos = gridSnap(m_hoverAlong);
            m_b.clampOpening(m_hoverWall, o);
            const Wall& W = m_b.walls[m_hoverWall];
            bool overlap = false;
            for (const Opening& e : W.ops) if (std::fabs(e.pos - o.pos) < (e.width + o.width) * 0.5f + 0.05f) overlap = true;
            m_ghost = o;
            m_ghostOk = !overlap;
            if (lclick && m_ghostOk) {
                pushUndo();
                m_b.walls[m_hoverWall].ops.push_back(o);
                m_selKind = 2; m_selWall = m_hoverWall; m_selOp = (int)m_b.walls[m_hoverWall].ops.size() - 1;
                edited();
            }
        }
        break;
    }
    }
}

// ---------------------------------------------------------------------------- icons (thin outline)

void ArchApp::drawIcon(ImDrawList* dl, int k, ImVec2 c, float s, ImU32 col) {
    const float u = s / 24.0f, th = 1.7f * m_ui;
    auto P = [&](float x, float y) { return ImVec2(c.x + (x - 12) * u, c.y + (y - 12) * u); };
    const ImU32 fill = alpha(col, 0.16f);
    switch (k) {
    case kSelect: {
        ImVec2 p[] = {P(6, 4), P(18, 11), P(12.8f, 12.3f), P(15, 18), P(12.8f, 19), P(10.5f, 13.4f), P(6, 17)};
        dl->AddConcavePolyFilled(p, 7, fill);
        dl->AddPolyline(p, 7, col, ImDrawFlags_Closed, th);
        break;
    }
    case kRoom:
        dl->AddRectFilled(P(4, 5), P(20, 19), fill, 1.5f * u);
        dl->AddRect(P(4, 5), P(20, 19), col, 1.5f * u, 0, th);
        dl->AddLine(P(11, 5), P(11, 12), col, th); dl->AddLine(P(11, 12), P(20, 12), col, th); dl->AddLine(P(4, 12), P(7, 12), col, th);
        break;
    case kWall:
        dl->AddRectFilled(P(3, 8), P(21, 17), fill);
        dl->AddRect(P(3, 8), P(21, 17), col, 0, 0, th);
        dl->AddLine(P(3, 12.5f), P(21, 12.5f), col, th);
        for (float x : {9.0f, 15.0f}) dl->AddLine(P(x, 8), P(x, 12.5f), col, th);
        for (float x : {6.0f, 12.0f, 18.0f}) dl->AddLine(P(x, 12.5f), P(x, 17), col, th);
        break;
    case kWindowTool:
        dl->AddRectFilled(P(5, 4), P(19, 20), fill, 1.5f * u);
        dl->AddRect(P(5, 4), P(19, 20), col, 1.5f * u, 0, th);
        dl->AddLine(P(12, 4), P(12, 20), col, th); dl->AddLine(P(5, 12), P(19, 12), col, th);
        break;
    case kDoorTool:
        dl->AddRectFilled(P(6, 4), P(16, 20), fill);
        dl->AddLine(P(6, 20), P(6, 4), col, th); dl->AddLine(P(6, 4), P(16, 4), col, th); dl->AddLine(P(16, 4), P(16, 20), col, th);
        dl->AddLine(P(4, 20), P(20, 20), col, th); dl->AddCircleFilled(P(13.5f, 12.5f), 1.2f * u, col);
        break;
    case kMoveTool:   // four arrows
        for (int i = 0; i < 4; ++i) {
            const float dx = i == 0 ? 1.0f : i == 1 ? -1.0f : 0.0f, dy = i == 2 ? 1.0f : i == 3 ? -1.0f : 0.0f;
            dl->AddLine(P(12 + dx * 2.5f, 12 + dy * 2.5f), P(12 + dx * 8.5f, 12 + dy * 8.5f), col, th);
            const ImVec2 t[3] = {P(12 + dx * 6 - dy * 2.6f, 12 + dy * 6 - dx * 2.6f), P(12 + dx * 9, 12 + dy * 9), P(12 + dx * 6 + dy * 2.6f, 12 + dy * 6 + dx * 2.6f)};
            dl->AddPolyline(t, 3, col, 0, th);
        }
        dl->AddCircleFilled(P(12, 12), 1.6f * u, col, 12);
        break;
    case kRotateTool: {   // a ring with its arrow head
        dl->PathArcTo(P(12, 12), 7.5f * u, 0.9f, 6.0f, 28);
        dl->PathStroke(col, 0, th);
        const ImVec2 t[3] = {P(15.2f, 3.6f), P(19.6f, 8.6f), P(13.4f, 9.6f)};
        dl->AddPolyline(t, 3, col, 0, th);
        dl->AddCircleFilled(P(12, 12), 1.6f * u, col, 12);
        break;
    }
    case 10:   // undo
        dl->AddLine(P(9, 7), P(5, 11), col, th); dl->AddLine(P(5, 11), P(9, 15), col, th);
        dl->PathArcTo(P(14, 16), 5 * u, -1.5708f, 1.5708f); dl->PathStroke(col, 0, th);
        dl->AddLine(P(5, 11), P(14, 11), col, th); dl->AddLine(P(14, 21), P(11, 21), col, th);
        break;
    case 11:   // redo
        dl->AddLine(P(15, 7), P(19, 11), col, th); dl->AddLine(P(19, 11), P(15, 15), col, th);
        dl->PathArcTo(P(10, 16), 5 * u, 1.5708f, 4.7124f); dl->PathStroke(col, 0, th);
        dl->AddLine(P(19, 11), P(10, 11), col, th); dl->AddLine(P(10, 21), P(13, 21), col, th);
        break;
    case 12:   // home (logo)
        dl->AddLine(P(4, 11), P(12, 4), col, th); dl->AddLine(P(12, 4), P(20, 11), col, th);
        dl->AddRect(P(6, 10), P(18, 20), col, 0, 0, th);
        dl->AddRect(P(10, 14), P(14, 20), col, 0, 0, th);
        break;
    case 13:   // plus
        dl->AddLine(P(12, 5), P(12, 19), col, th); dl->AddLine(P(5, 12), P(19, 12), col, th);
        break;
    case 14:   // save
        dl->AddRect(P(5, 5), P(19, 19), col, 2 * u, 0, th);
        dl->AddRect(P(8, 5), P(16, 10), col, 0, 0, th);
        dl->AddRect(P(8, 14), P(16, 19), col, 0, 0, th);
        break;
    case 15:   // plate (level)
    {
        ImVec2 p[] = {P(3, 14), P(12, 9), P(21, 14), P(12, 19)};
        dl->AddConvexPolyFilled(p, 4, fill);
        dl->AddPolyline(p, 4, col, ImDrawFlags_Closed, th);
        break;
    }
    }
}

// ---------------------------------------------------------------------------- the plan (2D view)

void ArchApp::drawPlan(ImDrawList* dl) {
    const ImVec4 R = m_rp;
    if (R.z <= R.x) return;
    dl->PushClipRect(ImVec2(R.x, R.y), ImVec2(R.z, R.w), true);
    dl->AddRectFilled(ImVec2(R.x, R.y), ImVec2(R.z, R.w), IM_COL32(251, 252, 253, 255));
    // grid: 1 m (0.5 m when zoomed in), 5 m stronger
    const float2 lo = screenToPlan(ImVec2(R.x, R.w)), hi = screenToPlan(ImVec2(R.z, R.y));
    const float step = m_planScale > 60 ? 0.5f : (m_planScale > 12 ? 1.0f : 5.0f);
    for (float x = std::floor(lo.x / step) * step; x <= hi.x; x += step) {
        const bool major = std::fabs(std::fmod(std::fabs(x) + 1e-3f, 5.0f)) < 2e-3f;
        const bool unit = std::fabs(x - std::round(x)) < 1e-3f;
        const ImVec2 s = planToScreen(float2(x, 0));
        dl->AddLine(ImVec2(s.x, R.y), ImVec2(s.x, R.w), IM_COL32(31, 39, 51, major ? 26 : unit ? 15 : 8));
    }
    for (float y = std::floor(lo.y / step) * step; y <= hi.y; y += step) {
        const bool major = std::fabs(std::fmod(std::fabs(y) + 1e-3f, 5.0f)) < 2e-3f;
        const bool unit = std::fabs(y - std::round(y)) < 1e-3f;
        const ImVec2 s = planToScreen(float2(0, y));
        dl->AddLine(ImVec2(R.x, s.y), ImVec2(R.z, s.y), IM_COL32(31, 39, 51, major ? 26 : unit ? 15 : 8));
    }
    ImFont* fb = m_g->fontBold ? m_g->fontBold : ImGui::GetFont();
    ImFont* fr = ImGui::GetFont();
    const float fs = 13 * m_ui, fsm = 11.5f * m_ui;
    auto poly = [&](const std::vector<float2>& p, ImU32 fill, ImU32 stroke, float th) {
        std::vector<ImVec2> s;
        for (const float2& q : p) s.push_back(planToScreen(q));
        if (fill && s.size() >= 3) {   // own ear clipping (AddConcavePolyFilled misses with the flipped plan Y)
            std::vector<int> tris;
            if (polyArea(p) >= 0) triangulate(p, tris);
            else {   // (the ear clipping wants counter-clockwise)
                std::vector<float2> r(p.rbegin(), p.rend());
                triangulate(r, tris);
                for (int& i : tris) i = (int)p.size() - 1 - i;
            }
            const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
            dl->PrimReserve((int)tris.size(), (int)tris.size());
            for (int i : tris) dl->PrimVtx(s[i], uv, fill);
        }
        if (stroke) dl->AddPolyline(s.data(), (int)s.size(), stroke, ImDrawFlags_Closed, th);
    };
    // the level below, for reference
    if (m_level > 0)
        for (int w = 0; w < (int)m_b.walls.size(); ++w) {
            const Wall& W = m_b.walls[w];
            if (!W.alive || m_b.nodes[W.a].level != m_level - 1) continue;
            const WallGeom& g = m_b.geom[w];
            poly({g.aL, g.bL, g.bR, g.aR}, IM_COL32(31, 39, 51, 22), IM_COL32(31, 39, 51, 40), 1);
        }
    // rooms
    static const ImU32 floorCol[kFloorCount] = {IM_COL32(246, 239, 228, 255), IM_COL32(238, 240, 242, 255), IM_COL32(232, 232, 230, 255)};
    for (int r = 0; r < (int)m_b.rooms.size(); ++r) {
        const Room& Rm = m_b.rooms[r];
        if (Rm.level != m_level) continue;
        const int fm = Rm.label >= 0 ? m_b.labels[Rm.label].floor : kFloorConcrete;
        poly(Rm.poly, r == m_selRoom && m_selKind == 3 ? IM_COL32(220, 231, 255, 255) : floorCol[std::clamp(fm, 0, kFloorCount - 1)], 0, 0);
    }
    // walls: the fills (walls and joints) first, then the outlines and the openings
    std::vector<int> degree(m_b.nodes.size(), 0);
    for (const Wall& W : m_b.walls) if (W.alive) { ++degree[W.a]; ++degree[W.b]; }
    for (int w = 0; w < (int)m_b.walls.size(); ++w) {
        const Wall& W = m_b.walls[w];
        if (!W.alive || m_b.nodes[W.a].level != m_level) continue;
        const WallGeom& g = m_b.geom[w];
        const bool sel = (m_selKind == 1 && m_selWall == w);
        poly({g.aL, g.bL, g.bR, g.aR}, sel ? kAccent : (W.thickness > 0.2f ? IM_COL32(74, 82, 98, 255) : IM_COL32(138, 145, 156, 255)), 0, 0);
    }
    for (const Joint& J : m_b.joints)
        if (J.level == m_level) poly(J.poly, IM_COL32(74, 82, 98, 255), 0, 0);
    for (int w = 0; w < (int)m_b.walls.size(); ++w) {
        const Wall& W = m_b.walls[w];
        if (!W.alive || m_b.nodes[W.a].level != m_level) continue;
        const WallGeom& g = m_b.geom[w];
        const bool sel = (m_selKind == 1 && m_selWall == w);
        const ImU32 fill = sel ? kAccent : (W.thickness > 0.2f ? IM_COL32(74, 82, 98, 255) : IM_COL32(138, 145, 156, 255));
        // the sides; the ends only where the wall stops (corners and joints stay one solid piece)
        dl->AddLine(planToScreen(g.aL), planToScreen(g.bL), kInk, 1.0f);
        dl->AddLine(planToScreen(g.aR), planToScreen(g.bR), kInk, 1.0f);
        if (degree[W.a] == 1) dl->AddLine(planToScreen(g.aL), planToScreen(g.aR), kInk, 1.0f);
        if (degree[W.b] == 1) dl->AddLine(planToScreen(g.bL), planToScreen(g.bR), kInk, 1.0f);
        (void)fill;
        const float t = W.thickness * 0.5f;
        for (int i = 0; i < (int)W.ops.size(); ++i) {
            const Opening& o = W.ops[i];
            const float s0 = o.pos - o.width * 0.5f, s1 = o.pos + o.width * 0.5f;
            auto P = [&](float s, float off) { return g.a + g.u * s + g.n * off; };
            poly({P(s0, t + 0.01f), P(s1, t + 0.01f), P(s1, -t - 0.01f), P(s0, -t - 0.01f)}, IM_COL32(251, 252, 253, 255), 0, 0);
            const bool osel = m_selKind == 2 && m_selWall == w && m_selOp == i;
            const ImU32 c = osel ? kAccent : kInk;
            auto L = [&](float2 a, float2 b, float th = 1.0f) { dl->AddLine(planToScreen(a), planToScreen(b), c, th * m_ui); };
            L(P(s0, t), P(s0, -t)); L(P(s1, t), P(s1, -t));
            // the joinery symbol: glass lines, the swing of each leaf (side -n by default: inwards for the outer walls drawn
            // counter-clockwise), the tracks and panels of the sliding ones (ArchJoinery: same leaves, hinges and sides)
            const float side = m_b.openingSide(w, o);
            const bool rightH = (o.flip & 2) != 0;
            const int nl = joineryLeaves(o);
            auto swing = [&](float sh, float sFree, float th) {   // a leaf hinged at sh whose free edge is at sFree when closed
                const float2 hinge = P(sh, side * t), leaf = hinge + g.n * (side * std::fabs(sFree - sh));
                L(hinge, leaf, th);
                const float2 cdir = g.u * (sFree > sh ? 1.0f : -1.0f);
                const float a0 = std::atan2(-(g.n.y * side), g.n.x * side), a1 = std::atan2(-cdir.y, cdir.x);
                float d = a1 - a0;
                while (d > kPi) d -= 2 * kPi;
                while (d < -kPi) d += 2 * kPi;
                dl->PathArcTo(planToScreen(hinge), std::fabs(sFree - sh) * m_planScale, a0, a0 + d, 24);
                dl->PathStroke(alpha(c, 0.6f), 0, 1.0f * m_ui);
            };
            const bool slide = o.kind == kSlider || (o.kind == kWindow && o.style == kWinSliding);
            if (slide) {
                L(P(s0, t), P(s1, t)); L(P(s0, -t), P(s1, -t));
                if (o.kind == kSlider && o.style == kSlidePocket) {   // the leaf and the arrow into the wall
                    L(P(s0, 0), P(s1, 0), 1.6f);
                    const float sa = rightH ? s1 : s0, sb = rightH ? s1 + 0.4f : s0 - 0.4f;
                    L(P(sa, 0.02f), P(sb, 0.02f), 1.0f);
                } else {
                    const float pw = (o.width + (nl - 1) * 0.05f) / nl;
                    for (int i = 0; i < nl; ++i) {
                        const float a = s0 + i * (pw - 0.05f), off = (i - (nl - 1) * 0.5f) * 0.05f;
                        L(P(a, off), P(a + pw, off), 1.5f);
                    }
                }
            } else if (o.kind == kWindow) {
                L(P(s0, t), P(s1, t)); L(P(s0, -t), P(s1, -t)); L(P(s0, 0.03f), P(s1, 0.03f)); L(P(s0, -0.03f), P(s1, -0.03f));
                if (o.style == kWinCasement || o.style == kWinTiltTurn) {
                    const float lw = o.width / nl;
                    for (int i = 0; i < nl; ++i) {
                        const bool left = nl == 2 ? i == 0 : !rightH;
                        const float a = s0 + i * lw, b = a + lw;
                        swing(left ? a : b, left ? b : a, 1.0f);
                    }
                }
            } else {   // doors (an entrance door: its side light on the lock side)
                float a = s0, b = s1;
                if (o.style == kDoorEntrance) {
                    const float sl = std::min(0.42f, o.width * 0.3f);
                    if (rightH) { L(P(s0 + 0.03f, 0.03f), P(s0 + sl, 0.03f)); L(P(s0 + 0.03f, -0.03f), P(s0 + sl, -0.03f)); a = s0 + sl; }
                    else { L(P(s1 - sl, 0.03f), P(s1 - 0.03f, 0.03f)); L(P(s1 - sl, -0.03f), P(s1 - 0.03f, -0.03f)); b = s1 - sl; }
                }
                const float lw = (b - a) / nl;
                for (int i = 0; i < nl; ++i) {
                    const bool left = nl == 2 ? i == 0 : !rightH;
                    const float x0 = a + i * lw, x1 = x0 + lw;
                    swing(left ? x0 : x1, left ? x1 : x0, 1.4f);
                }
            }
            if (osel) poly({P(s0, t + 0.05f), P(s1, t + 0.05f), P(s1, -t - 0.05f), P(s0, -t - 0.05f)}, 0, kAccent, 2 * m_ui);
        }
    }
    // the objects: their footprint, the front marked
    auto itemRect = [&](float2 c, float yaw, float2 size, ImU32 fill, ImU32 stroke, float th, const char* label) {
        const float2 ax(std::cos(yaw), std::sin(yaw)), ay = perp(ax);
        const float2 hx = ax * (size.x * 0.5f), hy = ay * (size.y * 0.5f);
        poly({c - hx - hy, c + hx - hy, c + hx + hy, c - hx + hy}, fill, stroke, th);
        dl->AddLine(planToScreen(c - hx * 0.6f - hy * 0.86f), planToScreen(c + hx * 0.6f - hy * 0.86f), stroke, th);   // its front (asset -Y)
        if (label && m_planScale * std::min(size.x, size.y) > 46 * m_ui) {
            const ImVec2 ts = fr->CalcTextSizeA(fsm, FLT_MAX, 0, label), sc = planToScreen(c);
            dl->AddText(fr, fsm, ImVec2(sc.x - ts.x * 0.5f, sc.y - ts.y * 0.5f), kInk2, label);
        }
    };
    for (int i = 0; i < (int)m_b.items.size(); ++i) {
        const Item& it = m_b.items[i];
        if (it.level != m_level) continue;
        const bool sel = m_selKind == 4 && m_selItem == i;
        const char* name = it.asset.c_str();
        for (const AssetInfo& a : m_assets) if (a.slug == it.asset) name = a.name.c_str();
        itemRect(it.p, it.yaw, it.size, sel ? IM_COL32(220, 231, 255, 235) : IM_COL32(255, 255, 255, 215), sel ? kAccent : IM_COL32(110, 118, 132, 255), (sel ? 2.0f : 1.0f) * m_ui, name);
    }
    if (m_tool == kPlace && m_hoverView != 0 && m_placeAsset >= 0 && m_placeAsset < (int)m_assets.size()) {
        const AssetInfo& a = m_assets[m_placeAsset];
        itemRect(m_placeAt, m_placeYaw, float2(a.size.x, a.size.z), IM_COL32(240, 138, 28, 50), kDraw, 1.6f * m_ui, a.name.c_str());
    }
    // room names
    for (const Room& Rm : m_b.rooms) {
        if (Rm.level != m_level || m_planScale < 14) continue;
        const ImVec2 c = planToScreen(Rm.label >= 0 && pointInPoly(m_b.labels[Rm.label].p, Rm.poly) ? m_b.labels[Rm.label].p : Rm.centre);
        const std::string name = Rm.label >= 0 ? m_b.labels[Rm.label].name : "Room";
        char area[32];
        snprintf(area, sizeof area, "%.2f m²", Rm.area);
        const ImVec2 ns = fb->CalcTextSizeA(fs, FLT_MAX, 0, name.c_str()), as = fr->CalcTextSizeA(fsm, FLT_MAX, 0, area);
        dl->AddText(fb, fs, ImVec2(c.x - ns.x * 0.5f, c.y - ns.y), kInk, name.c_str());
        dl->AddText(fr, fsm, ImVec2(c.x - as.x * 0.5f, c.y + 1), kInk2, area);
    }
    // overall dimensions (south and west of the level)
    {
        float2 a(1e9f, 1e9f), b(-1e9f, -1e9f);
        for (int w = 0; w < (int)m_b.walls.size(); ++w) {
            if (!m_b.walls[w].alive || m_b.nodes[m_b.walls[w].a].level != m_level) continue;
            const WallGeom& g = m_b.geom[w];
            for (float2 q : {g.aL, g.aR, g.bL, g.bR}) { a = float2(std::min(a.x, q.x), std::min(a.y, q.y)); b = float2(std::max(b.x, q.x), std::max(b.y, q.y)); }
        }
        if (a.x < b.x && m_planScale > 10) {
            const float off = 34 * m_ui / m_planScale;
            const ImVec2 p0 = planToScreen(float2(a.x, a.y - off)), p1 = planToScreen(float2(b.x, a.y - off));
            dl->AddLine(p0, p1, kInk2, 1);
            for (ImVec2 p : {p0, p1}) { dl->AddLine(ImVec2(p.x - 4 * m_ui, p.y + 4 * m_ui), ImVec2(p.x + 4 * m_ui, p.y - 4 * m_ui), kInk2, 1.2f); dl->AddLine(ImVec2(p.x, p.y - 7 * m_ui), ImVec2(p.x, p.y + 7 * m_ui), kInk2, 1); }
            std::string t = fmtm(b.x - a.x);
            ImVec2 ts = fr->CalcTextSizeA(fsm, FLT_MAX, 0, t.c_str());
            dl->AddText(fr, fsm, ImVec2((p0.x + p1.x - ts.x) * 0.5f, p0.y - ts.y - 3 * m_ui), kInk2, t.c_str());
            const ImVec2 q0 = planToScreen(float2(a.x - off, a.y)), q1 = planToScreen(float2(a.x - off, b.y));
            dl->AddLine(q0, q1, kInk2, 1);
            for (ImVec2 p : {q0, q1}) { dl->AddLine(ImVec2(p.x - 4 * m_ui, p.y + 4 * m_ui), ImVec2(p.x + 4 * m_ui, p.y - 4 * m_ui), kInk2, 1.2f); dl->AddLine(ImVec2(p.x - 7 * m_ui, p.y), ImVec2(p.x + 7 * m_ui, p.y), kInk2, 1); }
            t = fmtm(b.y - a.y);
            ts = fr->CalcTextSizeA(fsm, FLT_MAX, 0, t.c_str());
            dl->AddText(fr, fsm, ImVec2(q0.x - ts.x - 5 * m_ui, (q0.y + q1.y - ts.y) * 0.5f), kInk2, t.c_str());
        }
    }
    // selection: the length of the wall, the distances of the opening
    if (m_selKind == 1 && m_selWall >= 0 && m_selWall < (int)m_b.walls.size()) {
        const WallGeom& g = m_b.geom[m_selWall];
        const float off = m_b.walls[m_selWall].thickness * 0.5f + 26 * m_ui / m_planScale;
        const ImVec2 a = planToScreen(g.a + g.n * off), b = planToScreen(g.b + g.n * off);
        dl->AddLine(a, b, kAccent, 1.2f * m_ui);
        pill(dl, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), (fmtm(g.len) + " m").c_str(), 1, kEditWallLen);
    }
    if (m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0 && m_selOp < (int)m_b.walls[m_selWall].ops.size()) {
        const WallGeom& g = m_b.geom[m_selWall];
        const Opening& o = m_b.walls[m_selWall].ops[m_selOp];
        const float lo = std::max(g.sL0, g.sR0), hi = std::min(g.sL1, g.sR1);
        const float s0 = o.pos - o.width * 0.5f, s1 = o.pos + o.width * 0.5f;
        const float off = m_b.walls[m_selWall].thickness * 0.5f + 24 * m_ui / m_planScale;
        auto P = [&](float s) { return planToScreen(g.a + g.u * s + g.n * off); };
        dl->AddLine(P(lo), P(hi), kAccent, 1.0f * m_ui);
        for (float s : {lo, s0, s1, hi}) { const ImVec2 p = P(s); dl->AddCircleFilled(p, 2.5f * m_ui, kAccent); }
        const ImVec2 a = P((lo + s0) * 0.5f), b = P((s0 + s1) * 0.5f), c = P((s1 + hi) * 0.5f);
        pill(dl, a, fmtm(s0 - lo).c_str(), 0, kEditOpLeft);
        pill(dl, b, fmtm(o.width).c_str(), 1, kEditOpWidth);
        pill(dl, c, fmtm(hi - s1).c_str(), 0, kEditOpRight);
    }
    // previews
    if (m_drawing && m_tool == kWall) {
        const float2 d = norm2(m_cur - m_start), n = perp(d);
        const float t = m_wallThick * 0.5f;
        poly({m_start + n * t, m_cur + n * t, m_cur - n * t, m_start - n * t}, IM_COL32(240, 138, 28, 70), kDraw, 1.4f * m_ui);
        const float off = t + 22 * m_ui / m_planScale;
        const ImVec2 a = planToScreen(m_start + n * off), b = planToScreen(m_cur + n * off);
        char txt[32];
        snprintf(txt, sizeof txt, "%.2f m", len2(m_cur - m_start));
        pill(dl, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), txt, 2, kEditLength);
    }
    if (m_drawing && m_tool == kRoom) {
        const float2 a(std::min(m_start.x, m_cur.x), std::min(m_start.y, m_cur.y)), b(std::max(m_start.x, m_cur.x), std::max(m_start.y, m_cur.y));
        const float t = m_wallThick * 0.5f;
        poly({float2(a.x - t, a.y - t), float2(b.x + t, a.y - t), float2(b.x + t, b.y + t), float2(a.x - t, b.y + t)}, IM_COL32(47, 107, 255, 30), kAccent, 1.4f * m_ui);
        poly({float2(a.x + t, a.y + t), float2(b.x - t, a.y + t), float2(b.x - t, b.y - t), float2(a.x + t, b.y - t)}, IM_COL32(255, 255, 255, 120), kAccent, 1.0f * m_ui);
        const float off = 26 * m_ui / m_planScale;
        char tw[32], td[32];
        snprintf(tw, sizeof tw, "%.2f m", b.x - a.x);
        snprintf(td, sizeof td, "%.2f m", b.y - a.y);
        pill(dl, planToScreen(float2((a.x + b.x) * 0.5f, a.y - t - off)), tw, m_typedW > 0 ? 3 : 0, kEditWidth);
        pill(dl, planToScreen(float2(b.x + t + off, (a.y + b.y) * 0.5f)), td, 0, kEditDepth);
        char ar[32];
        snprintf(ar, sizeof ar, "%.1f m²", std::max(0.0f, (b.x - a.x - 2 * t) * (b.y - a.y - 2 * t)));
        pill(dl, planToScreen((a + b) * 0.5f), ar, 4);
    }
    if ((m_tool == kWindowTool || m_tool == kDoorTool) && m_hoverWall >= 0 && m_hoverView) {
        const WallGeom& g = m_b.geom[m_hoverWall];
        const float t = m_b.walls[m_hoverWall].thickness * 0.5f + 0.03f;
        const float s0 = m_ghost.pos - m_ghost.width * 0.5f, s1 = m_ghost.pos + m_ghost.width * 0.5f;
        auto P = [&](float s, float o) { return g.a + g.u * s + g.n * o; };
        poly({P(s0, t), P(s1, t), P(s1, -t), P(s0, -t)}, m_ghostOk ? IM_COL32(47, 107, 255, 90) : IM_COL32(220, 60, 50, 90), m_ghostOk ? kAccent : IM_COL32(220, 60, 50, 255), 1.5f * m_ui);
        const float lo = std::max(g.sL0, g.sR0), hi = std::min(g.sL1, g.sR1);
        const float off = t + 22 * m_ui / m_planScale;
        pill(dl, planToScreen(P((lo + s0) * 0.5f, off)), fmtm(s0 - lo).c_str(), 0);
        pill(dl, planToScreen(P((s1 + hi) * 0.5f, off)), fmtm(hi - s1).c_str(), 0);
    }
    // guides and the cursor
    if (m_hoverView && m_tool != kSelect) {
        for (const Guide& gd : m_guides) {
            const ImVec2 a = planToScreen(gd.a), b = planToScreen(gd.b);
            const float L = std::hypot(b.x - a.x, b.y - a.y);
            for (float s = 0; s < L; s += 8 * m_ui) {
                const float e = std::min(L, s + 4 * m_ui);
                dl->AddLine(ImVec2(a.x + (b.x - a.x) * s / L, a.y + (b.y - a.y) * s / L), ImVec2(a.x + (b.x - a.x) * e / L, a.y + (b.y - a.y) * e / L), kDraw, 1.2f * m_ui);
            }
        }
        const ImVec2 c = planToScreen(m_cur);
        const float k = 7 * m_ui;
        dl->AddLine(ImVec2(c.x - 2 * k, c.y), ImVec2(c.x + 2 * k, c.y), kDraw, 1.2f * m_ui);
        dl->AddLine(ImVec2(c.x, c.y - 2 * k), ImVec2(c.x, c.y + 2 * k), kDraw, 1.2f * m_ui);
        if (!m_snapLabel.empty()) dl->AddRect(ImVec2(c.x - 5 * m_ui, c.y - 5 * m_ui), ImVec2(c.x + 5 * m_ui, c.y + 5 * m_ui), kDraw, 0, 0, 1.5f * m_ui);
    }
    // north
    {
        const ImVec2 c(R.z - 38 * m_ui, R.w - 96 * m_ui);
        dl->AddCircleFilled(c, 20 * m_ui, IM_COL32(255, 255, 255, 230));
        dl->AddCircle(c, 20 * m_ui, IM_COL32(20, 30, 50, 30), 0, 1);
        dl->AddLine(ImVec2(c.x, c.y - 15 * m_ui), ImVec2(c.x, c.y - 6 * m_ui), kAccent, 2 * m_ui);
        const ImVec2 ts = fb->CalcTextSizeA(fs, FLT_MAX, 0, "N");
        dl->AddText(fb, fs, ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f + 3 * m_ui), kInk2, "N");
    }
    drawLeafHandles(dl, true);   // the open / close handles of the selected opening, on its symbol
    dl->PopClipRect();
}

// ---------------------------------------------------------------------------- 3D overlays

void ArchApp::draw3DOverlay(ImDrawList* dl) {
    const ImVec4 R = m_r3;
    if (R.z <= R.x) return;
    dl->PushClipRect(ImVec2(R.x, R.y), ImVec2(R.z, R.w), true);
    const Camera& cam = m_scene.camera;
    const float elev = m_b.levels[m_level].elevation;
    // a box seen from the camera: its visible faces filled, its outline
    auto ghostBox = [&](float2 a, float2 b, float t, float z0, float z1, ImU32 fill, ImU32 top, ImU32 stroke) {
        const float2 u = norm2(b - a), n = perp(u);
        const float2 c[4] = {a + n * t, b + n * t, b - n * t, a - n * t};
        ImVec2 lo[4], hi[4];
        for (int i = 0; i < 4; ++i) if (!project(c[i], z0, lo[i]) || !project(c[i], z1, hi[i])) return;
        const float3 eye = cam.eye();
        const float2 e2(eye.x, -eye.z);
        for (int i = 0; i < 4; ++i) {   // side faces facing the eye
            const float2 p = c[i], q = c[(i + 1) % 4];
            const float2 outn = -perp(norm2(q - p));   // (c is clockwise around the box: right side is out)
            if (dot2(e2 - (p + q) * 0.5f, outn) < 0 && dot2(e2 - (p + q) * 0.5f, -outn) < 0) continue;
            if (dot2(e2 - (p + q) * 0.5f, outn) <= 0) continue;
            ImVec2 f[4] = {lo[i], lo[(i + 1) % 4], hi[(i + 1) % 4], hi[i]};
            dl->AddConvexPolyFilled(f, 4, fill);
        }
        dl->AddConvexPolyFilled(hi, 4, top);
        for (int i = 0; i < 4; ++i) { dl->AddLine(hi[i], hi[(i + 1) % 4], stroke, 1.2f * m_ui); dl->AddLine(lo[i], hi[i], stroke, 1.0f * m_ui); dl->AddLine(lo[i], lo[(i + 1) % 4], alpha(stroke, 0.5f), 1.0f * m_ui); }
    };
    const float H = m_b.levels[m_level].height;
    {   // the object selected / being placed: its box
        auto itemBox = [&](float2 c, float yaw, float2 size, float h, ImU32 fill, ImU32 top, ImU32 stroke) {
            const float2 hx = float2(std::cos(yaw), std::sin(yaw)) * (size.x * 0.5f);
            ghostBox(c - hx, c + hx, size.y * 0.5f, 0.02f, 0.02f + h, fill, top, stroke);
        };
        if (m_selKind == 4 && m_selItem >= 0 && m_selItem < (int)m_b.items.size() && m_b.items[m_selItem].level == m_level) {
            const Item& it = m_b.items[m_selItem];
            itemBox(it.p, it.yaw, it.size, it.height, 0, 0, alpha(kAccent, 0.9f));
        }
        if (m_tool == kPlace && m_hoverView != 0 && m_placeAsset >= 0 && m_placeAsset < (int)m_assets.size()) {
            const AssetInfo& a = m_assets[m_placeAsset];
            itemBox(m_placeAt, m_placeYaw, float2(a.size.x, a.size.z), a.size.y, IM_COL32(240, 138, 28, 40), IM_COL32(240, 138, 28, 70), kDraw);
        }
    }
    if (m_drawing && m_tool == kWall) {
        ghostBox(m_start, m_cur, m_wallThick * 0.5f, 0, H, IM_COL32(240, 138, 28, 60), IM_COL32(240, 138, 28, 140), kDraw);
        ImVec2 p;
        char txt[32];
        snprintf(txt, sizeof txt, "%.2f m", len2(m_cur - m_start));
        if (project((m_start + m_cur) * 0.5f, H + 0.35f, p)) pill(dl, p, txt, 2, kEditLength);
    }
    if (m_drawing && m_tool == kRoom) {
        const float2 a(std::min(m_start.x, m_cur.x), std::min(m_start.y, m_cur.y)), b(std::max(m_start.x, m_cur.x), std::max(m_start.y, m_cur.y));
        const float2 c[4] = {a, float2(b.x, a.y), b, float2(a.x, b.y)};
        const float h = H * 0.7f;   // the walls rising
        for (int i = 0; i < 4; ++i) ghostBox(c[i], c[(i + 1) % 4], m_wallThick * 0.5f, 0, h, IM_COL32(47, 107, 255, 45), IM_COL32(47, 107, 255, 110), alpha(kAccent, 0.9f));
        ImVec2 p;
        char tw[32], td[32];
        snprintf(tw, sizeof tw, "%.2f m", b.x - a.x);
        snprintf(td, sizeof td, "%.2f m", b.y - a.y);
        if (project(float2((a.x + b.x) * 0.5f, a.y - 0.9f), 0, p)) pill(dl, p, tw, m_typedW > 0 ? 3 : 0, kEditWidth);
        if (project(float2(b.x + 0.9f, (a.y + b.y) * 0.5f), 0, p)) pill(dl, p, td, 0, kEditDepth);
    }
    if ((m_tool == kWindowTool || m_tool == kDoorTool) && m_hoverWall >= 0 && m_hoverView) {
        const WallGeom& g = m_b.geom[m_hoverWall];
        const float t = m_b.walls[m_hoverWall].thickness * 0.5f + 0.02f;
        const float s0 = m_ghost.pos - m_ghost.width * 0.5f, s1 = m_ghost.pos + m_ghost.width * 0.5f;
        const ImU32 c = m_ghostOk ? kAccent : IM_COL32(220, 60, 50, 255);
        for (float side : {1.0f, -1.0f}) {
            ImVec2 q[4];
            const float2 a = g.a + g.u * s0 + g.n * (side * t), b = g.a + g.u * s1 + g.n * (side * t);
            if (project(a, m_ghost.sill, q[0]) && project(b, m_ghost.sill, q[1]) && project(b, m_ghost.sill + m_ghost.height, q[2]) && project(a, m_ghost.sill + m_ghost.height, q[3])) {
                dl->AddConvexPolyFilled(q, 4, alpha(c, 0.22f));
                dl->AddPolyline(q, 4, c, ImDrawFlags_Closed, 2 * m_ui);
            }
        }
        const float lo = std::max(g.sL0, g.sR0), hi = std::min(g.sL1, g.sR1);
        ImVec2 p;
        const float2 off = g.n * (t + 0.05f);
        if (project(g.a + g.u * ((lo + s0) * 0.5f) + off, m_ghost.sill + m_ghost.height * 0.5f, p)) pill(dl, p, fmtm(s0 - lo).c_str(), 0);
        if (project(g.a + g.u * ((s1 + hi) * 0.5f) + off, m_ghost.sill + m_ghost.height * 0.5f, p)) pill(dl, p, fmtm(hi - s1).c_str(), 0);
    }
    if (m_selKind == 1 && m_selWall >= 0 && m_selWall < (int)m_b.walls.size()) {
        const WallGeom& g = m_b.geom[m_selWall];
        ImVec2 p;
        if (project((g.a + g.b) * 0.5f, m_b.walls[m_selWall].height + 0.4f, p)) pill(dl, p, (fmtm(g.len) + " m").c_str(), 1, kEditWallLen);
    }
    if (m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0 && m_selOp < (int)m_b.walls[m_selWall].ops.size()) {
        const WallGeom& g = m_b.geom[m_selWall];
        const Opening& o = m_b.walls[m_selWall].ops[m_selOp];
        const float lo = std::max(g.sL0, g.sR0), hi = std::min(g.sL1, g.sR1);
        const float s0 = o.pos - o.width * 0.5f, s1 = o.pos + o.width * 0.5f;
        const float z = std::min(m_b.walls[m_selWall].height, o.sill + o.height) + 0.3f;
        // the distances on the side facing the camera
        const float3 eye = cam.eye();
        const float side = dot2(float2(eye.x, -eye.z) - g.a, g.n) > 0 ? 1.0f : -1.0f;
        const float2 off = g.n * (side * (m_b.walls[m_selWall].thickness * 0.5f + 0.02f));
        ImVec2 a, b, c, d;
        if (project(g.a + g.u * lo + off, z, a) && project(g.a + g.u * s0 + off, z, b) && project(g.a + g.u * s1 + off, z, c) && project(g.a + g.u * hi + off, z, d)) {
            dl->AddLine(a, d, kAccent, 1.2f * m_ui);
            for (ImVec2 q : {a, b, c, d}) dl->AddCircleFilled(q, 3 * m_ui, kAccent);
            pill(dl, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f - 14 * m_ui), fmtm(s0 - lo).c_str(), 0, kEditOpLeft);
            pill(dl, ImVec2((b.x + c.x) * 0.5f, (b.y + c.y) * 0.5f - 14 * m_ui), fmtm(o.width).c_str(), 1, kEditOpWidth);
            pill(dl, ImVec2((c.x + d.x) * 0.5f, (c.y + d.y) * 0.5f - 14 * m_ui), fmtm(hi - s1).c_str(), 0, kEditOpRight);
        }
        ImVec2 s;
        if (o.sill > 0.01f && project(g.a + g.u * s0 + off, o.sill * 0.5f, s)) pill(dl, ImVec2(s.x - 26 * m_ui, s.y), fmtm(o.sill).c_str(), 0);
    }
    // guides and the cursor on the floor
    if (m_hoverView && m_tool != kSelect) {
        for (const Guide& gd : m_guides) {
            ImVec2 a, b;
            if (!project(gd.a, 0.01f, a) || !project(gd.b, 0.01f, b)) continue;
            const float L = std::hypot(b.x - a.x, b.y - a.y);
            for (float s = 0; s < L; s += 9 * m_ui) {
                const float e = std::min(L, s + 5 * m_ui);
                dl->AddLine(ImVec2(a.x + (b.x - a.x) * s / L, a.y + (b.y - a.y) * s / L), ImVec2(a.x + (b.x - a.x) * e / L, a.y + (b.y - a.y) * e / L), kDraw, 1.4f * m_ui);
            }
        }
        ImVec2 c;
        if (project(m_cur, 0.01f, c)) {
            dl->AddCircleFilled(c, 6 * m_ui, kWhite);
            dl->AddCircle(c, 6 * m_ui, kDraw, 0, 2 * m_ui);
            if (!m_snapLabel.empty() && m_hoverView == 1) {
                ImFont* f = m_g->fontBold ? m_g->fontBold : ImGui::GetFont();
                const float fs = 12 * m_ui;
                const ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0, m_snapLabel.c_str());
                const ImVec2 a(c.x + 12 * m_ui, c.y + 10 * m_ui), b(a.x + ts.x + 14 * m_ui, a.y + ts.y + 8 * m_ui);
                dl->AddRectFilled(a, b, kDraw, 7 * m_ui);
                dl->AddText(f, fs, ImVec2(a.x + 7 * m_ui, a.y + 4 * m_ui), kWhite, m_snapLabel.c_str());
            }
        }
    }
    (void)elev;
    drawGizmo(dl);
    if (m_scriptHover.leaf >= 0 && !m_opt.script.empty()) m_leafHover = m_scriptHover;
    drawLeafHandles(dl, false);
    dl->PopClipRect();
}

// ---------------------------------------------------------------------------- the interface

static bool segButtons(const char* id, const char* const* items, int n, int& cur, float ui, ImFont* bold) {
    ImGui::PushID(id);
    bool changed = false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 26 * ui;
    float w = 0;
    std::vector<float> ws;
    for (int i = 0; i < n; ++i) { ws.push_back(ImGui::CalcTextSize(items[i]).x + 20 * ui); w += ws.back(); }
    const ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(p, ImVec2(p.x + w + 4 * ui, p.y + h + 4 * ui), kHollow, 9 * ui);
    float x = p.x + 2 * ui;
    for (int i = 0; i < n; ++i) {
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + 2 * ui));
        if (ImGui::InvisibleButton(items[i], ImVec2(ws[i], h))) { cur = i; changed = true; }
        const bool on = cur == i;
        if (on) {
            dl->AddRectFilled(ImVec2(x, p.y + 3 * ui), ImVec2(x + ws[i], p.y + h + 3 * ui), IM_COL32(20, 30, 50, 22), 7 * ui);
            dl->AddRectFilled(ImVec2(x, p.y + 2 * ui), ImVec2(x + ws[i], p.y + h + 2 * ui), kWhite, 7 * ui);
        }
        ImFont* f = on && bold ? bold : ImGui::GetFont();
        const float fs = ImGui::GetFontSize();
        const ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0, items[i]);
        dl->AddText(f, fs, ImVec2(x + (ws[i] - ts.x) * 0.5f, p.y + 2 * ui + (h - ts.y) * 0.5f), on ? kInk : kInk2, items[i]);
        x += ws[i];
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(w + 4 * ui, h + 4 * ui));   // one item: SameLine works after it
    ImGui::PopID();
    return changed;
}

static bool fieldM(const char* label, float* v, float ui, float lo = 0.01f, float hi = 100.0f) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%s", label);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 96 * ui - ImGui::GetStyle().ItemSpacing.x * 0 - 0);
    ImGui::SetNextItemWidth(96 * ui);
    ImGui::PushID(label);
    float t = *v;
    const bool ch = ImGui::InputFloat("##v", &t, 0, 0, "%.2f m", ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopID();
    if (ch) { *v = std::clamp(t, lo, hi); return true; }
    return false;
}

void ArchApp::drawUi() {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 ds = io.DisplaySize;
    const float k = m_ui;
    const ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    ImFont* bold = m_g->fontBold;
    const float M = 14 * k;

    // ---- top left: the project
    ImGui::SetNextWindowPos(ImVec2(M, M));
    ImGui::Begin("##project", nullptr, fl);
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float s = 28 * k;
        dl->AddRectFilledMultiColor(p, ImVec2(p.x + s, p.y + s), IM_COL32(91, 140, 255, 255), IM_COL32(91, 140, 255, 255), kAccent, kAccent);
        dl->AddRect(p, ImVec2(p.x + s, p.y + s), IM_COL32(255, 255, 255, 70), 8 * k);
        drawIcon(dl, 12, ImVec2(p.x + s * 0.5f, p.y + s * 0.5f), 18 * k, kWhite);
        ImGui::Dummy(ImVec2(s, s));
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (bold) ImGui::PushFont(bold, 0.0f);
        ImGui::Text("ArchViz");
        if (bold) ImGui::PopFont();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "%s", m_b.levels[m_level].name.c_str());
        ImGui::EndGroup();
        ImGui::SameLine(0, 14 * k);
        const ImVec2 bs(30 * k, 30 * k);
        for (int i = 0; i < 3; ++i) {
            ImGui::PushID(i);
            const ImVec2 q = ImGui::GetCursorScreenPos();
            const bool en = i == 0 ? !m_undo.empty() : i == 1 ? !m_redo.empty() : true;
            if (ImGui::InvisibleButton("b", bs) && en) { if (i == 0) undo(); else if (i == 1) redo(); else { save(m_savePath); toast("Saved"); } }
            if (ImGui::IsItemHovered()) dl->AddRectFilled(q, ImVec2(q.x + bs.x, q.y + bs.y), kHollow, 8 * k);
            drawIcon(dl, i == 0 ? 10 : i == 1 ? 11 : 14, ImVec2(q.x + bs.x * 0.5f, q.y + bs.y * 0.5f), 19 * k, en ? IM_COL32(58, 66, 80, 255) : kInk3);
            ImGui::PopID();
            ImGui::SameLine(0, 2 * k);
        }
        ImGui::Dummy(ImVec2(1, 1));
    }
    ImGui::End();

    // ---- top centre: the steps of the pipeline
    {
        static const char* steps[] = {"1  Build", "2  Furnish", "3  Light", "4  Render"};
        float w = 0;
        for (const char* s : steps) w += ImGui::CalcTextSize(s).x + 20 * k;
        ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f, M), 0, ImVec2(0.5f, 0));
        ImGui::Begin("##steps", nullptr, fl);
        int cur = m_step;
        if (segButtons("steps", steps, 4, cur, k, bold)) {
            if (cur >= 2) toast("Light and Render come in the next versions");
            else if (cur != m_step) {
                m_step = cur;
                m_tool = kSelect; m_drawing = false; m_edit = kEditNone;
                m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1; m_selDirty = true;
                
            }
        }
        ImGui::End();
    }

    // ---- top right: views and render
    ImGui::SetNextWindowPos(ImVec2(ds.x - M, M), 0, ImVec2(1, 0));
    ImGui::Begin("##render", nullptr, fl);
    {
        static const char* views[] = {"3D", "Split", "Plan"};
        int v = m_view;
        if (segButtons("views", views, 3, v, k, bold) && v != m_view) { m_view = v; m_lastMove = ImGui::GetTime(); }
        ImGui::SameLine(0, 10 * k);
        static const char* q[] = {"HQ", "Ray tracing", "RT DLSS"};
        int qq = m_quality;
        if (segButtons("quality", q, 3, qq, k, bold) && qq != m_quality) { m_quality = qq; m_r->resetAccumulation(); }
        // the light: the soft studio or the sky with its sun, turned around the building
        static const char* envs[] = {"Studio", "Sun sky"};
        int ev = m_env;
        if (segButtons("env", envs, 2, ev, k, bold) && ev != m_env) { m_env = ev; applyEnv(); }
        ImGui::SameLine(0, 10 * k);
        const float rowY = ImGui::GetCursorPosY();
        ImGui::SetNextItemWidth(104 * k);
        ImGui::SetCursorPosY(rowY + 3 * k);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.18f, 0.42f, 1, 0.28f));   // (light: the value reads through it)
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.18f, 0.42f, 1, 0.45f));
        if (ImGui::SliderFloat("##sunrot", &m_envRot, 0.0f, 360.0f, "Sun  %.0f\xc2\xb0")) applyEnv();
        ImGui::PopStyleColor(2);
        ImGui::SameLine(0, 8 * k);
        ImGui::SetCursorPosY(rowY + 3 * k);
        if (ImGui::Checkbox("Ceiling", &m_ceiling)) m_sceneDirty = true;   // closed rooms: interior renders
    }
    ImGui::End();

    // ---- left: the levels
    ImGui::SetNextWindowPos(ImVec2(M, 90 * k));
    ImGui::Begin("##levels", nullptr, fl);
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "LEVELS");
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = (int)m_b.levels.size() - 1; i >= 0; --i) {
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImVec2 sz(150 * k, 32 * k);
            if (ImGui::InvisibleButton("lv", sz)) { m_level = i; m_drawing = false; m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1; m_sceneDirty = true; }
            const bool on = i == m_level;
            if (on) {
                dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), kWhite, 9 * k);
                dl->AddRect(p, ImVec2(p.x + sz.x, p.y + sz.y), alpha(kAccent, 0.45f), 9 * k, 0, 1.2f * k);
            } else if (ImGui::IsItemHovered()) dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), kHollow, 9 * k);
            drawIcon(dl, 15, ImVec2(p.x + 16 * k, p.y + sz.y * 0.5f), 20 * k, on ? kAccent : kInk2);
            ImFont* f = on && bold ? bold : ImGui::GetFont();
            const float fs = ImGui::GetFontSize();
            dl->AddText(f, fs, ImVec2(p.x + 32 * k, p.y + (sz.y - fs) * 0.5f), on ? kInk : kInk2, m_b.levels[i].name.c_str());
            char e[24];
            snprintf(e, sizeof e, "%+.2f", m_b.levels[i].elevation);
            const ImVec2 ts = ImGui::CalcTextSize(e);
            dl->AddText(ImVec2(p.x + sz.x - ts.x - 8 * k, p.y + (sz.y - fs) * 0.5f), kInk3, e);
            ImGui::PopID();
        }
        const ImVec2 p = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("addlv", ImVec2(150 * k, 28 * k))) { pushUndo(); m_level = m_b.addLevel(); edited(); toast("Level added: the level below shows in the plan"); }
        drawIcon(dl, 13, ImVec2(p.x + 16 * k, p.y + 14 * k), 16 * k, kAccent);
        dl->AddText(ImVec2(p.x + 32 * k, p.y + (28 * k - ImGui::GetFontSize()) * 0.5f), kAccent, "Add level");
    }
    ImGui::End();

    // ---- the library of objects (Furnish): ArchLibrary.cpp
    m_libRect = ImVec4(0, 0, 0, 0);
    if (m_step == 1) drawLibrary();
    const float libBottom = (m_step == 1 && m_libDock == 0) ? (m_libRect.w - m_libRect.y) + 8 * k : 0.0f;   // the room it takes
    // (the tools only move up when the library reaches under them)
    const float toolsUp = (libBottom > 0 && (m_toolsRect.z <= m_toolsRect.x || m_libRect.z > m_toolsRect.x - 8 * k)) ? libBottom : 0.0f;
    const float libRight = (m_step == 1 && m_libDock == 2) ? (m_libRect.z - m_libRect.x) + 8 * k : 0.0f;

    // ---- bottom: the tools
    {
        // Build: the tools of the walls; Furnish: Select and the manipulators (the objects come from the library)
        static const int buildTools[] = {kSelect, kRoom, kWall, kWindowTool, kDoorTool}, furnishTools[] = {kSelect, kMoveTool, kRotateTool};
        static const char* buildNames[] = {"Select", "Room", "Wall", "Window", "Door"};
        static const char* buildKeys[] = {"V", "R", "W", "N", "D"};
        static const char* furnishNames[] = {"Select", "Move", "Rotate"};
        static const char* furnishKeys[] = {"V", "G", "R"};
        const int* tools = m_step == 1 ? furnishTools : buildTools;
        const char* const* names = m_step == 1 ? furnishNames : buildNames;
        const char* const* keys = m_step == 1 ? furnishKeys : buildKeys;
        const int toolCount = m_step == 1 ? 3 : 5;
        const float dockX = m_view == kViewSplit ? (m_r3.x + m_r3.z) * 0.5f : ds.x * 0.5f;
        ImGui::SetNextWindowPos(ImVec2(dockX, ds.y - M - toolsUp), 0, ImVec2(0.5f, 1));
        ImGui::Begin("##tools", nullptr, fl);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 dockPos = ImGui::GetWindowPos(), dockSize = ImGui::GetWindowSize();
        m_toolsRect = ImVec4(dockPos.x, dockPos.y, dockPos.x + dockSize.x, dockPos.y + dockSize.y);
        for (int ti = 0; ti < toolCount; ++ti) {
            const int i = tools[ti];
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImVec2 sz(58 * k, 62 * k);
            if (ImGui::InvisibleButton("t", sz)) { m_tool = i; if (m_step == 1) m_furnishTool = i; m_drawing = false; m_edit = kEditNone; }
            const bool on = m_tool == i;
            const ImVec2 b0(p.x + 7 * k, p.y), b1(p.x + sz.x - 7 * k, p.y + 44 * k);
            if (on) {
                dl->AddRectFilled(ImVec2(b0.x, b0.y + 3 * k), ImVec2(b1.x, b1.y + 3 * k), IM_COL32(47, 107, 255, 70), 12 * k);
                dl->AddRectFilledMultiColor(b0, b1, IM_COL32(91, 140, 255, 255), IM_COL32(91, 140, 255, 255), kAccent, kAccent);
                dl->AddRect(b0, b1, IM_COL32(255, 255, 255, 80), 12 * k);
            } else if (ImGui::IsItemHovered()) dl->AddRectFilled(b0, b1, kHollow, 12 * k);
            drawIcon(dl, i, ImVec2((b0.x + b1.x) * 0.5f, (b0.y + b1.y) * 0.5f), 24 * k, on ? kWhite : IM_COL32(58, 66, 80, 255));
            const ImVec2 ts = ImGui::CalcTextSize(names[ti]);
            dl->AddText(ImVec2(p.x + (sz.x - ts.x) * 0.5f, p.y + 46 * k), on ? kInk : kInk2, names[ti]);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s (%s)", names[ti], keys[ti]);
            ImGui::PopID();
            ImGui::SameLine(0, 2 * k);
        }
        ImGui::Dummy(ImVec2(1, 1));
        ImGui::End();

        // ---- beside the tools: the snapping options (the grid and its step, the alignment assistant)
        ImGui::SetNextWindowPos(ImVec2(dockPos.x + dockSize.x + 8 * k, dockPos.y + dockSize.y), 0, ImVec2(0, 1));
        ImGui::Begin("##snap", nullptr, fl);
        {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "SNAP");
            bool ch = false;
            ch |= ImGui::Checkbox("Grid", &m_snapGrid);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Points, lengths and moves land on the grid (Shift: free)");
            ImGui::SameLine(0, 6 * k);
            static const float steps[] = {0.05f, 0.10f, 0.25f, 0.50f, 1.00f};
            static const char* stepNames[] = {"5 cm", "10 cm", "25 cm", "50 cm", "1 m"};
            int cur = 1;
            for (int i = 0; i < 5; ++i) if (std::fabs(steps[i] - m_gridStep) < 1e-3f) cur = i;
            ImGui::SetNextItemWidth(78 * k);
            if (ImGui::BeginCombo("##step", stepNames[cur])) {
                for (int i = 0; i < 5; ++i) if (ImGui::Selectable(stepNames[i], i == cur)) { m_gridStep = steps[i]; ch = true; }
                ImGui::EndCombo();
            }
            ch |= ImGui::Checkbox("Align assistant", &m_snapAlign);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Guides from the corners and the middles of the walls,\nangles by 15\xc2\xb0 from the last point");
            if (ch) savePrefs();
        }
        ImGui::End();
    }

    // ---- right: the inspector
    ImGui::SetNextWindowPos(ImVec2(ds.x - M - libRight, 106 * k), 0, ImVec2(1, 0));
    ImGui::SetNextWindowSizeConstraints(ImVec2(270 * k, 0), ImVec2(270 * k, ds.y));
    ImGui::Begin("##inspector", nullptr, fl);
    {
        auto title = [&](const char* t, const char* sub) {
            if (bold) ImGui::PushFont(bold, 0.0f);
            ImGui::TextUnformatted(t);
            if (bold) ImGui::PopFont();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%s", sub);
            ImGui::Spacing();
        };
        auto section = [&](const char* t) { ImGui::Spacing(); ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "%s", t); };
        if (m_tool == kPlace && m_placeAsset >= 0 && m_placeAsset < (int)m_assets.size()) {
            const AssetInfo& a = m_assets[m_placeAsset];
            title(a.name.c_str(), "Click in the room to place it");
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%.2f x %.2f m, %.2f m high\n%d parts, %d textures", a.size.x, a.size.z, a.size.y, a.parts, a.textures);
            section("PLACEMENT");
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "Q / E turn it by 15\xc2\xb0 (Shift: 90\xc2\xb0).\nIt snaps against the walls,\nShift: free. Esc cancels.");
        } else if (m_selKind == 4 && m_selItem >= 0 && m_selItem < (int)m_b.items.size()) {
            Item& it = m_b.items[m_selItem];
            const char* name = it.asset.c_str();
            for (const AssetInfo& a : m_assets) if (a.slug == it.asset) name = a.name.c_str();
            title(name, m_tool == kMoveTool ? "Arrows, squares or the centre sphere" : m_tool == kRotateTool ? "Drag the ring to turn it" : "Drag it to move it, Delete removes it");
            section("POSITION");
            Item e = it;
            bool ch = false;
            ch |= fieldM("X", &e.p.x, k, -1000.0f, 1000.0f);
            ch |= fieldM("Y", &e.p.y, k, -1000.0f, 1000.0f);
            ch |= fieldM("Above floor", &e.elev, k, 0.0f, 20.0f);
            section("ROTATION");
            float deg = std::round(e.yaw * 180.0f / kPi * 10.0f) / 10.0f;
            deg = deg - 360.0f * std::floor(deg / 360.0f);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%.0f\xc2\xb0", deg);
            ImGui::SameLine(70 * k);
            if (ImGui::Button("-90\xc2\xb0")) { e.yaw -= kPi * 0.5f; ch = true; }
            ImGui::SameLine();
            if (ImGui::Button("-15\xc2\xb0")) { e.yaw -= radians(15.0f); ch = true; }
            ImGui::SameLine();
            if (ImGui::Button("+15\xc2\xb0")) { e.yaw += radians(15.0f); ch = true; }
            ImGui::SameLine();
            if (ImGui::Button("+90\xc2\xb0")) { e.yaw += kPi * 0.5f; ch = true; }
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "Q / E turn it too (Shift: 90\xc2\xb0)");
            if (m_tool == kRotateTool) {   // the step of the ring, as BrickIt's Tourner
                section("RING STEP");
                static const char* rs[] = {"15\xc2\xb0", "45\xc2\xb0", "90\xc2\xb0", "Free"};
                static const int rv[] = {15, 45, 90, 0};
                int cur = 0;
                for (int i = 0; i < 4; ++i) if (rv[i] == m_rotStep) cur = i;
                if (segButtons("rotstep", rs, 4, cur, k, bold)) { m_rotStep = rv[cur]; savePrefs(); }
            }
            if (ch) { pushUndo(); m_b.items[m_selItem] = e; placeItem(m_scene, m_map, m_b, m_selItem); touched(); }
        } else if (m_step == 1) {
            title("Furnish", "Click an object of the library,\nor drag it into a room");
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%d object(s) placed", (int)m_b.items.size());
        } else if (m_selKind == 1 && m_selWall >= 0 && m_selWall < (int)m_b.walls.size()) {
            Wall& W = m_b.walls[m_selWall];
            title("Wall", "Drag it to move it, Delete removes it");
            section("SIZE");
            float len = m_b.wallLength(m_selWall), th = W.thickness, h = W.height;
            if (fieldM("Length", &len, k, 0.1f)) { pushUndo(); m_b.setWallLength(m_selWall, len); edited(); }
            if (fieldM("Thickness", &th, k, 0.05f, 1.0f)) { pushUndo(); m_b.walls[m_selWall].thickness = th; edited(); }
            if (fieldM("Height", &h, k, 0.3f, 10.0f)) { pushUndo(); m_b.walls[m_selWall].height = h; edited(); }
            section("PRESETS");
            if (ImGui::Button("Exterior 30")) { pushUndo(); m_b.walls[m_selWall].thickness = 0.30f; edited(); }
            ImGui::SameLine();
            if (ImGui::Button("Interior 12")) { pushUndo(); m_b.walls[m_selWall].thickness = 0.12f; edited(); }
            ImGui::SameLine();
            if (ImGui::Button("Interior 7")) { pushUndo(); m_b.walls[m_selWall].thickness = 0.07f; edited(); }
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "%d opening(s)", (int)W.ops.size());
        } else if (m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0 && m_selOp < (int)m_b.walls[m_selWall].ops.size()) {
            Opening o = m_b.walls[m_selWall].ops[m_selOp];
            title(openingName(o.kind), "Drag it along the wall, Delete removes it");
            section("SIZE & POSITION");
            const WallGeom& g = m_b.geom[m_selWall];
            float left = (o.pos - o.width * 0.5f) - std::max(g.sL0, g.sR0);
            bool ch = false;
            ch |= fieldM("Width", &o.width, k, 0.3f);
            ch |= fieldM("Height", &o.height, k, 0.3f);
            if (o.kind == kWindow) ch |= fieldM("Sill", &o.sill, k, 0.0f);
            if (fieldM("From corner", &left, k, 0.0f)) { o.pos = std::max(g.sL0, g.sR0) + left + o.width * 0.5f; ch = true; }
            section("TYPE");
            static const char* kinds[] = {"Window", "Door", "Sliding"};
            int kk = o.kind;
            if (segButtons("kind", kinds, 3, kk, k, bold) && kk != o.kind) {
                o.kind = kk;
                if (kk == kDoor) { o.sill = 0; o.height = 2.10f; o.width = std::min(o.width, 1.0f); }
                if (kk == kSlider) { o.sill = 0; o.height = 2.25f; o.width = std::max(o.width, 2.0f); }
                if (kk == kWindow) { o.sill = 0.9f; o.height = 1.35f; }
                o.style = 0; o.leaves = 0;
                ch = true;
            }
            // joinery: the style of the kind, the leaves, the glazing bars, the finish, the hinge and swing sides
            {
                const char* styles[8];
                const int ns = joineryStyleCount(o.kind);
                for (int i = 0; i < ns; ++i) styles[i] = joineryStyleName(o.kind, i);
                int st = std::clamp(o.style, 0, ns - 1);
                if (segButtons("jstyle", styles, ns, st, k, bold) && st != o.style) { o.style = st; o.leaves = 0; ch = true; }
                const int maxL = joineryMaxLeaves(o);
                if (maxL > 1) {
                    section(o.kind == kSlider || (o.kind == kWindow && o.style == kWinSliding) ? "PANELS" : "LEAVES");
                    static const char* ln[] = {"Auto", "1", "2", "3", "4"};
                    int cur = std::clamp(o.leaves, 0, maxL);
                    if (segButtons("jleaves", ln, maxL + 1, cur, k, bold) && cur != o.leaves) { o.leaves = cur; ch = true; }
                }
                section("GLAZING BARS");
                auto stepper = [&](const char* id, const char* label, int& v, int lo, int hi) {
                    ImGui::PushID(id);
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%s", label);
                    ImGui::SameLine(90 * k);
                    if (ImGui::Button("-", ImVec2(26 * k, 0)) && v > lo) { --v; ch = true; }
                    ImGui::SameLine();
                    ImGui::Text("%d", v);
                    ImGui::SameLine();
                    if (ImGui::Button("+", ImVec2(26 * k, 0)) && v < hi) { ++v; ch = true; }
                    ImGui::PopID();
                };
                stepper("jcols", "Across", o.cols, 1, 8);
                stepper("jrows", "Up", o.rows, 1, 8);
                section("FINISH");
                static const char* fn[] = {"Anthracite", "White", "Oak", "Black"};
                int f = std::clamp(o.finish, 0, kFinishCount - 1);
                if (segButtons("jfinish", fn, kFinishCount, f, k, bold) && f != o.finish) { o.finish = f; ch = true; }
                section("OPENING");
                const bool sliding = o.kind == kSlider || (o.kind == kWindow && o.style == kWinSliding);
                if (!(o.kind == kWindow && o.style == kWinFixed)) {
                    if (ImGui::Button(sliding ? ((o.flip & 2) ? "Slides right" : "Slides left") : ((o.flip & 2) ? "Hinged right" : "Hinged left"))) { o.flip ^= 2; ch = true; }
                    ImGui::SameLine();
                    if (ImGui::Button((o.flip & 1) ? "Opens outwards" : "Opens inwards")) { o.flip ^= 1; ch = true; }
                    float pct = o.open * 100.0f;
                    ImGui::SetNextItemWidth(150 * k);
                    if (ImGui::SliderFloat("##jopen", &pct, 0.0f, 100.0f, "%.0f %% open")) {   // live, no rebuild
                        m_b.walls[m_selWall].ops[m_selOp].setAllOpen(std::clamp(pct / 100.0f, 0.0f, 1.0f));
                        o.setAllOpen(m_b.walls[m_selWall].ops[m_selOp].open);
                        placeJoinery(m_scene, m_map, m_b, m_selWall, m_selOp);
                        m_r->resetAccumulation();
                        m_lastMove = ImGui::GetTime();
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit() && m_opt.shotPath.empty()) save(m_savePath);
                    ImGui::SameLine();
                    if (ImGui::Button(o.open > 0.5f ? "Close" : "Open")) toggleOpening(m_selWall, m_selOp);
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "O opens / closes it");
                }
            }
            if (ch) { pushUndo(); m_b.clampOpening(m_selWall, o); m_b.walls[m_selWall].ops[m_selOp] = o; edited(); }
        } else if (m_selKind == 3 && m_selRoom >= 0 && m_selRoom < (int)m_b.rooms.size()) {
            const Room& Rm = m_b.rooms[m_selRoom];
            char sub[64];
            snprintf(sub, sizeof sub, "%.2f m² inside the walls", Rm.area);
            title("Room", sub);
            if (Rm.label < 0) {
                if (ImGui::Button("Name this room")) { pushUndo(); m_b.labels.push_back({Rm.centre, "Room", kFloorWood, m_level, true}); edited(); }
            } else {
                RoomLabel& L = m_b.labels[Rm.label];
                char buf[64];
                snprintf(buf, sizeof buf, "%s", L.name.c_str());
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##name", buf, sizeof buf, ImGuiInputTextFlags_EnterReturnsTrue)) { pushUndo(); L.name = buf; edited(); }
                section("FLOOR");
                static const char* fl3[] = {"Oak", "Tiles", "Concrete"};
                int f = L.floor;
                if (segButtons("floor", fl3, 3, f, k, bold) && f != L.floor) { pushUndo(); m_b.labels[Rm.label].floor = f; edited(); }
            }
        } else if (m_tool == kRoom || m_tool == kWall) {
            title(m_tool == kRoom ? "New room" : "Wall", m_tool == kRoom ? "Drag a rectangle, or click two corners" : "Click points, double-click to finish");
            section("WALLS");
            fieldM("Thickness", &m_wallThick, k, 0.05f, 1.0f);
            float h = m_b.levels[m_level].height;
            if (fieldM("Height", &h, k, 1.8f, 8.0f)) { pushUndo(); m_b.levels[m_level].height = h; for (Wall& w : m_b.walls) if (w.alive && m_b.nodes[w.a].level == m_level) w.height = h; edited(); }
            static const char* pr[] = {"Ext. 30", "Int. 12", "Int. 7"};
            int p = m_wallThick > 0.2f ? 0 : m_wallThick > 0.1f ? 1 : 2;
            if (segButtons("preset", pr, 3, p, k, bold)) m_wallThick = p == 0 ? 0.30f : p == 1 ? 0.12f : 0.07f;
            if (m_tool == kRoom) {
                section("FLOOR");
                static const char* fl3[] = {"Oak", "Tiles", "Concrete"};
                segButtons("nfloor", fl3, 3, m_newFloor, k, bold);
            }
            section("PRECISION");
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "Type a value while drawing,\nTab for the next one, Enter.\nShift: no snapping.");
        } else if (m_tool == kWindowTool || m_tool == kDoorTool) {
            title(m_tool == kDoorTool ? "Door" : "Window", "Hover a wall, click to place");
            section("SIZE");
            if (m_tool == kWindowTool) { fieldM("Width", &m_winW, k, 0.3f, 6.0f); fieldM("Height", &m_winH, k, 0.3f, 3.0f); fieldM("Sill", &m_winSill, k, 0.0f, 2.0f); }
            else { fieldM("Width", &m_doorW, k, 0.5f, 3.0f); fieldM("Height", &m_doorH, k, 1.8f, 3.0f); }
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "Snaps to the grid step, Shift: free.");
        } else {
            title("Select", "Click a wall, an opening or a room");
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "Drag a wall to move it,\nan opening to slide it.\nClick a blue value to type it.");
        }
    }
    ImGui::End();

    // ---- the number being typed
    if (m_edit != kEditNone) {
        ImGui::SetNextWindowPos(m_editAt);
        ImGui::Begin("##edit", nullptr, fl);
        static const char* labels[] = {"", "Length", "Width", "Depth", "Length", "From corner", "To corner", "Width"};
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%s", labels[m_edit]);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90 * k);
        if (m_editFocus) { ImGui::SetKeyboardFocusHere(); m_editFocus = false; }
        const bool enter = ImGui::InputText("##num", m_editBuf, sizeof m_editBuf, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        const bool tab = ImGui::IsKeyPressed(ImGuiKey_Tab);
        ImGui::SameLine();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "m");
        float v;
        if ((enter || tab) && evalNumber(m_editBuf, v)) applyEdit(v);
        else if (enter) m_edit = kEditNone;
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_edit = kEditNone;
        ImGui::End();
    }

    // ---- bottom left: what to do now
    {
        const char* hint = "";
        switch (m_tool) {
        case kSelect: hint = m_step == 1 ? "Click an object  ·  drag to move  ·  G move, R rotate  ·  Delete  ·  wheel: closer, arrows: walk" : "Click to select  ·  drag to move  ·  Delete  ·  Ctrl+Z undo"; break;
        case kRoom: hint = m_drawing ? "Release or click the opposite corner  ·  type the width, Tab, the depth" : "Drag a rectangle on the ground or in the plan"; break;
        case kWall: hint = m_drawing ? "Click the next point  ·  type a length  ·  double-click, Enter or right-click to finish" : "Click the first point of the wall"; break;
        case kWindowTool: case kDoorTool: hint = "Hover a wall and click  ·  Shift: no snapping"; break;
        case kPlace: hint = m_libDragPlace ? "Release to drop it  ·  Shift: free" : "Click to place  ·  Q / E turn  ·  Shift: free  ·  Esc cancels"; break;
        case kMoveTool: hint = "Arrows: along an axis  ·  squares: in a plane  ·  sphere: free on the floor  ·  Shift: no grid"; break;
        case kRotateTool: hint = "Drag the ring  ·  Q / E turn by 15\xc2\xb0  ·  Shift: free angle"; break;
        }
        ImFont* f = ImGui::GetFont();
        const float fs = ImGui::GetFontSize();
        const ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0, hint);
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const float hx = (m_step == 1 && m_libDock == 1) ? m_libRect.z + 8 * k : M;   // (beside a library on the left)
        float hy = ds.y - M - libBottom;
        if (hx + ts.x + 20 * k > m_toolsRect.x - 8 * k && m_toolsRect.w > m_toolsRect.y) hy = m_toolsRect.y - 8 * k;   // (no room beside the tools: above them)
        const ImVec2 a(hx, hy - ts.y - 14 * k), b(hx + ts.x + 20 * k, hy);
        dl->AddRectFilled(a, b, IM_COL32(31, 39, 51, 215), 9 * k);
        dl->AddText(f, fs, ImVec2(a.x + 10 * k, a.y + 7 * k), kWhite, hint);
        if (!m_toast.empty() && ImGui::GetTime() - m_toastT < 3.0) {
            const ImVec2 t2 = f->CalcTextSizeA(fs, FLT_MAX, 0, m_toast.c_str());
            const ImVec2 c(ds.x * 0.5f, 74 * k);
            dl->AddRectFilled(ImVec2(c.x - t2.x * 0.5f - 12 * k, c.y), ImVec2(c.x + t2.x * 0.5f + 12 * k, c.y + t2.y + 14 * k), IM_COL32(31, 39, 51, 230), 9 * k);
            dl->AddText(f, fs, ImVec2(c.x - t2.x * 0.5f, c.y + 7 * k), kWhite, m_toast.c_str());
        }
    }
}

// ---------------------------------------------------------------------------- test states (captures)

void ArchApp::runScript() {
    // ";"-separated commands, plan coordinates in metres:
    //   room x0 y0 x1 y1 (a room being dragged) | wall x0 y0 x1 y1 (a wall being drawn) | window x y | door x y (ghost)
    //   select x y | addroom x0 y0 x1 y1 | tool name | level n | addlevel | view 3d|split|plan
    std::string s = m_opt.script;
    size_t i = 0;
    while (i < s.size()) {
        size_t j = s.find(';', i);
        if (j == std::string::npos) j = s.size();
        const std::string c = s.substr(i, j - i);
        i = j + 1;
        char cmd[32] = {};
        float a = 0, b = 0, cc = 0, d = 0;
        const int n = sscanf(c.c_str(), " %31s %f %f %f %f", cmd, &a, &b, &cc, &d);
        if (n < 1) continue;
        const std::string k = cmd;
        if (k == "room" && n == 5) { m_tool = kRoom; m_drawing = true; m_start = float2(a, b); m_cur = float2(cc, d); }
        else if (k == "wall" && n == 5) { m_tool = kWall; m_drawing = true; m_start = float2(a, b); m_cur = float2(cc, d); m_chainFirst = m_start; }
        else if ((k == "window" || k == "door") && n == 3) {
            m_tool = k == "door" ? kDoorTool : kWindowTool;
            m_hoverView = 2;
            m_hoverWall = m_b.wallAt(float2(a, b), m_level, 0.2f, &m_hoverAlong);
            if (m_hoverWall >= 0) {
                Opening o;
                o.kind = k == "door" ? kDoor : kWindow;
                o.width = k == "door" ? m_doorW : m_winW; o.height = k == "door" ? m_doorH : m_winH; o.sill = k == "door" ? 0 : m_winSill;
                o.pos = m_hoverAlong;
                m_b.clampOpening(m_hoverWall, o);
                m_ghost = o; m_ghostOk = true;
            }
        }
        else if (k == "select" && n == 3) { m_tool = kSelect; m_tol = 0.2f; pickSelect(float2(a, b)); }
        else if (k == "addroom" && n == 5) { commitRoom(float2(a, b), float2(cc, d)); }
        else if (k == "item") {   // item slug x y yawDegrees
            char slug[64] = {};
            float x = 0, y = 0, yaw = 0;
            if (sscanf(c.c_str(), " %*s %63s %f %f %f", slug, &x, &y, &yaw) >= 3)
                for (int ai = 0; ai < (int)m_assets.size(); ++ai) if (m_assets[ai].slug == slug) { m_step = 1; addItem(ai, float2(x, y), radians(yaw)); }
        }
        else if (k == "place") {   // place slug x y: the ghost of an object at the mouse
            char slug[64] = {};
            float x = 0, y = 0;
            if (sscanf(c.c_str(), " %*s %63s %f %f", slug, &x, &y) >= 3)
                for (int ai = 0; ai < (int)m_assets.size(); ++ai) if (m_assets[ai].slug == slug) { m_step = 1; m_tool = kPlace; m_placeAsset = ai; m_placeAt = float2(x, y); m_hoverView = 2; }
        }
        else if (k == "step") { m_step = a >= 1 ? 1 : 0; m_tool = kSelect; }
        else if (k == "libdock" && n == 2) { m_libDock = std::clamp((int)a, 0, 2); m_step = 1; }   // 0 bottom, 1 left, 2 right
        else if (k == "libinfo" && n == 2) { m_libInfo = (int)a; m_step = 1; }
        else if (k == "selitem" && n == 2) { m_step = 1; m_selKind = 4; m_selItem = std::clamp((int)a, 0, (int)m_b.items.size() - 1); m_selDirty = true; }
        else if (k == "snap" && n == 4) { m_snapGrid = a >= 1; m_gridStep = b; m_snapAlign = cc >= 1; }   // snap grid(0|1) step align(0|1)
        else if (k == "ceiling" && n == 2) { m_ceiling = a >= 1; m_sceneDirty = true; }
        else if (k == "clear") { m_b.clear(); m_level = 0; edited(); }   // an empty building (prepared scenes)
        else if (k == "height" && n == 2) { m_b.levels[m_level].height = m_wallHeight = a; }   // of the walls drawn next
        else if (k == "thick" && n == 2) { m_wallThick = a; }
        else if (k == "opset") {   // opset kind style leaves cols rows finish flip: the selected opening (select x y first)
            float v[7] = {-1, -1, -1, -1, -1, -1, -1};
            sscanf(c.c_str(), " %*s %f %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]);
            if (m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0 && m_selOp < (int)m_b.walls[m_selWall].ops.size()) {
                Opening& op = m_b.walls[m_selWall].ops[m_selOp];
                if (v[0] >= 0) op.kind = (int)v[0];
                if (v[1] >= 0) op.style = (int)v[1];
                if (v[2] >= 0) op.leaves = (int)v[2];
                if (v[3] >= 1) op.cols = (int)v[3];
                if (v[4] >= 1) op.rows = (int)v[4];
                if (v[5] >= 0) op.finish = (int)v[5];
                if (v[6] >= 0) op.flip = (int)v[6];
                m_b.clampOpening(m_selWall, op);
                edited();
            }
        }
        else if (k == "openall" && n == 2) {   // every opening this far open (0..1)
            for (Wall& Wl : m_b.walls) for (Opening& op : Wl.ops) op.setAllOpen(std::clamp(a, 0.0f, 1.0f));
            placeJoinery(m_scene, m_map, m_b);
        }
        else if (k == "opopen" && n == 2 && m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0) {
            m_b.walls[m_selWall].ops[m_selOp].setAllOpen(std::clamp(a, 0.0f, 1.0f));
            placeJoinery(m_scene, m_map, m_b);
        }
        else if (k == "leafopen" && n == 3 && m_selKind == 2 && m_selWall >= 0 && m_selOp >= 0) {   // leafopen i v (tilt & turn: v < 0 tilts)
            m_b.walls[m_selWall].ops[m_selOp].leafOpen[std::clamp((int)a, 0, 3)] = std::clamp(b, -1.0f, 1.0f);
            joinerySyncOpen(m_b.walls[m_selWall].ops[m_selOp]);
            placeJoinery(m_scene, m_map, m_b);
        }
        else if (k == "handlehover" && n == 3) {   // handlehover leaf tilt: that handle shown hovered (captures)
            m_leafHover.leaf = -1;
            int seen = 0;
            for (int i = 0; i < (int)m_map.leaves.size(); ++i)
                if (m_map.leaves[i].wall == m_selWall && m_map.leaves[i].op == m_selOp && seen++ == (int)a) { m_leafHover.leaf = i; m_leafHover.tilt = b > 0.5f; }
            m_scriptHover = m_leafHover;
        }
        else if (k == "addop") {   // addop kind(0 window, 1 door, 2 sliding door) x y width height sill
            float kind = 0, x = 0, y = 0, w = 1, h = 1, sill = 0, along = 0;
            if (sscanf(c.c_str(), " %*s %f %f %f %f %f %f", &kind, &x, &y, &w, &h, &sill) == 6) {
                const int wi = m_b.wallAt(float2(x, y), m_level, 0.2f, &along);
                if (wi >= 0) {
                    Opening o;
                    o.kind = (int)kind; o.pos = along; o.width = w; o.height = h; o.sill = sill;
                    m_b.clampOpening(wi, o);
                    m_b.walls[wi].ops.push_back(o);
                    edited();
                }
            }
        }
        else if (k == "label") {   // label name x y floor(0 wood, 1 tile, 2 concrete)
            char name[64] = {};
            float x = 0, y = 0, fl = 0;
            if (sscanf(c.c_str(), " %*s %63s %f %f %f", name, &x, &y, &fl) == 4) {
                const int room = m_b.roomAt(float2(x, y), m_level);   // (replaces the name the room already has)
                for (RoomLabel& l : m_b.labels) if (l.alive && l.level == m_level && room >= 0 && m_b.roomAt(l.p, m_level) == room) l.alive = false;
                m_b.labels.push_back({float2(x, y), name, (int)fl, m_level, true});
                edited();
            }
        }
        else if (k == "cam" && n == 4) { m_scene.camera.yaw = radians(a); m_scene.camera.pitch = radians(b); m_scene.camera.distance = cc; }   // cam yaw pitch distance
        else if (k == "target" && n == 4) { m_scene.camera.target = world(float2(a, b), cc); }   // target x y height
        else if (k == "fov" && n == 2) { m_scene.camera.fovY = radians(a); }
        else if (k == "deselect") { m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1; m_selDirty = true; }
        else if (k == "addwall" && n == 5) { commitWall(float2(a, b), float2(cc, d)); }
        else if (k == "level" && n == 2) { m_level = std::clamp((int)a, 0, (int)m_b.levels.size() - 1); m_sceneDirty = true; }
        else if (k == "addlevel") { m_level = m_b.addLevel(); edited(); }
        else if (k == "tool") {
            char t[32] = {};
            sscanf(c.c_str(), " %*s %31s", t);
            const std::string tn = t;
            m_tool = tn == "select" ? kSelect : tn == "wall" ? kWall : tn == "window" ? kWindowTool : tn == "door" ? kDoorTool : tn == "move" ? kMoveTool : tn == "rotate" ? kRotateTool : kRoom;
            if (m_tool == kMoveTool || m_tool == kRotateTool) { m_step = 1; m_furnishTool = m_tool; }
        }
    }
}

// ---------------------------------------------------------------------------- frame

void ArchApp::toggleOpening(int wall, int op, int leaf, bool tilt) {
    if (wall < 0 || wall >= (int)m_b.walls.size() || op < 0 || op >= (int)m_b.walls[wall].ops.size()) return;
    const Opening& o = m_b.walls[wall].ops[op];
    const float cur = leaf < 0 ? o.open : o.leafOpen[std::min(leaf, 3)];
    const float full = tilt ? -1.0f : 1.0f;
    const float to = std::fabs(cur) > 0.5f ? 0.0f : full;
    for (size_t i = 0; i < m_opAnim.size(); ++i)   // (one animation per leaf: a new one replaces it)
        if (m_opAnim[i].wall == wall && m_opAnim[i].op == op && (m_opAnim[i].leaf == leaf || leaf < 0 || m_opAnim[i].leaf < 0)) { m_opAnim.erase(m_opAnim.begin() + i); --i; }
    m_opAnim.push_back({wall, op, leaf, cur, to, ImGui::GetTime()});
}

// The openings being opened / closed: their leaves move (placeJoinery), the path tracer starts again each frame.
void ArchApp::animateOpenings() {
    if (m_opAnim.empty()) return;
    const double now = ImGui::GetTime();
    bool done = false;
    for (size_t i = 0; i < m_opAnim.size();) {
        OpAnim& a = m_opAnim[i];
        if (a.wall >= (int)m_b.walls.size() || a.op >= (int)m_b.walls[a.wall].ops.size()) { m_opAnim.erase(m_opAnim.begin() + i); continue; }
        Opening& o = m_b.walls[a.wall].ops[a.op];
        const float dur = (o.kind == kWindow ? 0.7f : 0.95f) * std::max(0.25f, std::fabs(a.to - a.from));
        float t = std::clamp((float)((now - a.t0) / dur), 0.0f, 1.0f);
        const float e = t * t * (3.0f - 2.0f * t);   // ease in and out
        const float val = a.from + (a.to - a.from) * e;
        if (a.leaf < 0) o.setAllOpen(val);
        else { o.leafOpen[std::min(a.leaf, 3)] = val; joinerySyncOpen(o); }
        placeJoinery(m_scene, m_map, m_b, a.wall, a.op);
        if (t >= 1.0f) { m_opAnim.erase(m_opAnim.begin() + i); done = true; }
        else ++i;
    }
    m_r->resetAccumulation();
    m_lastMove = now;
    if (done && m_opAnim.empty() && m_opt.shotPath.empty()) save(m_savePath);
}

void ArchApp::frame() {
    m_g->newFrame();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 ds = io.DisplaySize;
    m_pillsPrev.swap(m_pills);
    m_pills.clear();
    const float split = std::floor(ds.x * 0.42f);
    if (m_view == kView3D) { m_r3 = ImVec4(0, 0, ds.x, ds.y); m_rp = ImVec4(0, 0, 0, 0); }
    else if (m_view == kViewPlan) { m_rp = ImVec4(0, 0, ds.x, ds.y); m_r3 = ImVec4(0, 0, 0, 0); }
    else { m_rp = ImVec4(0, 0, split, ds.y); m_r3 = ImVec4(split + 1, 0, ds.x, ds.y); }
    if (m_frame == 1) frameAll();
    if (m_frame == 2 && !m_opt.script.empty()) { runScript(); if (!m_opt.saveTo.empty()) save(m_opt.saveTo); }

    m_uiHover = io.WantCaptureMouse;
    if (!m_opt.clean) drawUi();
    cameraInput();
    if (m_opt.script.empty() || m_opt.shotPath.empty()) toolInput();   // (a scripted capture keeps its state)

    if (m_sceneDirty) ensureAssets();
    animateOpenings();
    if (m_sceneDirty || m_selDirty) {
        BuildOptions bo;
        bo.activeLevel = m_level;
        bo.selectedWall = m_selKind == 1 ? m_selWall : -1;
        bo.selectedOpeningWall = m_selKind == 2 ? m_selWall : -1;
        bo.selectedOpening = m_selKind == 2 ? m_selOp : -1;
        bo.assets = &m_assets;
        bo.ceilings = m_ceiling;
        if (m_sceneDirty) buildScene(m_b, m_scene, bo, m_map);
        else applySelection(m_scene, m_map, bo);   // (the tint alone: the meshes stay)
        m_sceneDirty = m_selDirty = false;
        m_r->resetAccumulation();
    }
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(238, 242, 246, 255));
    if (m_r3.z > m_r3.x) {
        RenderSettings rs;
        renderSettings(rs);
        {   // a new view restarts the accumulation of the path tracer (else the old image stays, frozen over the new one)
            const Camera& c = m_scene.camera;
            const float key[10] = {c.target.x, c.target.y, c.target.z, c.yaw, c.pitch, c.distance, c.fovY, m_r3.z - m_r3.x, m_r3.w - m_r3.y, (float)m_quality};
            if (std::memcmp(key, m_camKey, sizeof key) != 0) {
                std::memcpy(m_camKey, key, sizeof key);
                m_r->resetAccumulation();
                if (m_frame > 3) m_lastMove = ImGui::GetTime();
            }
        }
        // path tracing: half the resolution while the view moves (and 0.25 s after), as BrickIt; full size to converge
        const bool moving = ImGui::GetTime() - m_lastMove < 0.25 && m_opt.shotPath.empty();
        const float scale = (m_quality >= 1 && m_r->pathTracing() && moving) ? 0.5f : 1.0f;
        renderSettings(rs);   // (after the view test: its "moving" is this frame's)
        const vp3d::ViewRect vp{0, 0, std::max(16, (int)((m_r3.z - m_r3.x) * scale)), std::max(16, (int)((m_r3.w - m_r3.y) * scale))};
        m_r->renderToTexture(m_scene, rs, vp, -1);
        bg->AddImage(m_g->viewportTexture(), ImVec2(m_r3.x, m_r3.y), ImVec2(m_r3.z, m_r3.w));
        if (!m_opt.clean) draw3DOverlay(bg);
    }
    drawPlan(bg);
    if (m_view == kViewSplit) bg->AddLine(ImVec2(split + 0.5f, 0), ImVec2(split + 0.5f, ds.y), IM_COL32(20, 30, 50, 40), 1.0f);
    m_g->render(m_r3.z > m_r3.x);
    ++m_frame;
    if (!m_opt.shotPath.empty()) {
        const bool ready = (m_quality == 0 || !m_r->pathTracing()) ? m_frame >= m_opt.shotFrames : (int)m_r->sampleCount() >= m_opt.shotSpp;
        if (ready && m_frame > 6) {
            std::vector<uint8_t> img;
            uint32_t w, h;
            if (m_r->readBackBuffer(img, w, h)) {
                std::vector<uint8_t> png;
                FILE* f = nullptr;
                if (brickit::encodePng(img, w, h, png) && _wfopen_s(&f, m_opt.shotPath.c_str(), L"wb") == 0 && f) {
                    fwrite(png.data(), 1, png.size(), f);
                    fclose(f);
                }
            }
            m_running = false;
        }
    }
    m_r->present(m_opt.shotPath.empty());
}

} // namespace archviz
