// BrickIt - the studio stage and what a scene keeps of its look (Wzx 24/11/2026: « une scène Maya avec un shader et un
// lighting de la brique jaune à essayer de reproduire dans une scène dédiée »).
//
// The studio stage: a cyclorama (Stage.cpp, addCyclo) instead of the dark floor and its grid, no sun, no rim light, no
// hemisphere: the HDRI at its true intensity and the area lights of the Lighting panel (softboxes) light the model, as in
// a lookdev scene of Maya / Arnold. The studio plastic: the shader of that scene (no varnish, a specular of IOR 1.5 whose
// roughness is a fingerprints map projected on the part, half of the light under the surface, rounded edges).
//
// A scene file keeps this look in a "lookdev" block: the stage, the HDRI and its rotation and intensity, the engine and
// the view transform, the lights and cameras of the panels (they were lost with the session before) and the materials
// edited for this scene only. A scene without the block opens as before, the settings of the user back.
#include "BrickApp.h"
#include "Stage.h"
#include "Platform.h"
#include "app/Json.h"
#include "vp3d/AssetImport.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace brickit {

// (measured against Arnold renders of the Maya scene, kick 24/11/2026: clay within 3 % on the long face and 1 % on the floor,
// the reflections alone within 3 to 7 % for a roughness of 0.08 to 0.4, the shader within 5 % in linear values)
static const float kStudioBg = 0.80f;          // what the camera sees beyond the cyclo (linear)
static const float kStudioRough = 0.30f;       // the studio plastic without its fingerprints map
static const float kStudioFpScale = 0.2f;      // the fingerprints: repeats per cm (aiTriplanar scale 0.2)
static const float kStudioSss = 0.5f, kStudioSssRadius = 0.02f;   // (aiStandardSurface: subsurface 0.5, radius 1 x scale 0.02 cm)
static const float kStudioBevel = 0.012f;      // (aiRoundCorners radius 0.03 cm: the bevel of the engine reads 2.5 x wider, measured on the edges)

// a colour of the scene (linear, sRGB primaries): in ACEScg primaries when the scene is rendered there; else in the sRGB gamut
float3 BrickApp::gamut(const float3& c) const {
    if (!m_wideGamut) return float3(std::clamp(c.x, 0.0f, 1.0f), std::clamp(c.y, 0.0f, 1.0f), std::clamp(c.z, 0.0f, 1.0f));
    const float3 a(0.61319f * c.x + 0.33951f * c.y + 0.04737f * c.z, 0.07021f * c.x + 0.91634f * c.y + 0.01345f * c.z, 0.02062f * c.x + 0.10957f * c.y + 0.86961f * c.z);
    return float3(std::max(a.x, 0.0f), std::max(a.y, 0.0f), std::max(a.z, 0.0f));
}

std::wstring BrickApp::studioFile(const wchar_t* name) const {
    for (const std::wstring& d : {plat::dataDir() + L"\\studio\\", m_exeDir + L"\\assets\\studio\\"})
        if (plat::fileExists(d + name)) return d + name;
    return std::wstring();
}

// the fingerprints map of the studio plastic in the textures of the scene (decoded once, added again after a new stage)
void BrickApp::ensureStudioTex() {
    if (m_fpTex >= 0 && m_fpTex < (int)m_scene.textures.size()) return;
    m_fpTex = -1;
    if (!m_fpLoaded) {
        m_fpLoaded = true;
        const std::wstring p = studioFile(L"FingerPrints_Diffuse.jpg");
        std::string err;
        if (p.empty() || !vp3d::loadImage(p, m_fpImage, &err, 2048)) m_fpImage = vp3d::Texture();
    }
    if (m_fpImage.width > 0) m_fpTex = m_scene.addTexture(m_fpImage);
}

// a plastic of the palette as the studio plastic (setLookdev)
void BrickApp::studioPlastic(vp3d::Material& m, bool sssFromEdit) {
    ensureStudioTex();
    m.coat = 0.0f;
    m.specular = m_opt.ptSpec >= 0 ? m_opt.ptSpec : 0.5f;
    m.flags |= 16u;   // MAT_FLAG_FRESNEL: the exact Fresnel of IOR 1.5
    m.roughness = m_opt.ptRough >= 0 ? m_opt.ptRough : kStudioRough;
    if (m_fpTex >= 0 && m_opt.ptRough < 0) {
        m.roughnessTex = m_fpTex;
        m.roughTexScale = kStudioFpScale;
        m.roughTexStrength = m_opt.studioFp >= 0 ? m_opt.studioFp : 1.0f;
        m.flags |= 8u;   // MAT_FLAG_ROUGH_MAP
    }
    m.bevelRadius = kStudioBevel;
    if (m_opt.studioBase >= 0) { m.baseColor = float3(m_opt.studioBase); m.subsurface = 0.0f; return; }
    if (!sssFromEdit) {
        m.subsurface = m_opt.ptSssAll >= 0 ? m_opt.ptSssAll : kStudioSss;
        m.sssRadius = float3(kStudioSssRadius);
    }
}

// the size of the studio for the model (1 = the brick 1 x 4 of the lookdev scene, 1.7 cm from its centre to a corner) and
// the centre of the model
float BrickApp::studioScale(float3* centre) const {
    float3 mn(1e30f), mx(-1e30f);
    for (const Brick& b : m_bricks) { const Box bx = m_build.aabbOf(b.part, b.rot, b.pos); mn = minv(mn, bx.mn); mx = maxv(mx, bx.mx); }
    const bool any = mn.x <= mx.x;
    if (centre) *centre = any ? (mn + mx) * 0.5f : float3(0.0f, 0.6f, 0.0f);
    return any ? std::max(1.0f, length(mx - mn) * 0.5f / 1.7f) : 1.0f;
}

// The three softboxes of the lookdev scene around c, k times bigger and farther (aiAreaLight quads, not normalised: their
// radiance is intensity x 2^exposure; they shine along -Z of their matrix: U = its X row, V = minus its Y row).
void BrickApp::studioRig(const float3& c, float k) {
    auto box = [&](const char* name, float3 rel, float3 u, float3 v, float3 n, float size, float radiance) {
        UserLight l;
        l.name = name;
        l.pos = c + rel * k;
        l.rot = qNormalize(qFromBasis(normalize(u), normalize(v), normalize(n)));
        l.w = l.h = size * k;
        l.intensity = radiance;
        l.color = float3(1.0f);
        m_lights.push_back(l);
    };
    box("Softbox left", float3(-40.882f, 17.017f, 8.530f), float3(0, -1, 0), float3(0, 0, -1), float3(1, 0, 0), 36.66f, 4.0f);
    box("Softbox right", float3(39.456f, 17.017f, 8.530f), float3(0, -1, 0), float3(0, 0, 1), float3(-1, 0, 0), 36.66f, 4.0f);
    box("Softbox top", float3(6.854f, 17.615f, 6.495f), float3(14.647f, -3.255f, -0.488f), float3(-0.643f, -5.014f, 14.136f), float3(-3.228f, -13.771f, -5.032f), 30.02f, 3.0f);
}

// The stage of Render Settings › Environment. Studio on a scene that has no light yet: the softboxes of the lookdev scene
// at the size of the model and the studio HDRI (the folder studio of the data, when it is there), what the user had
// kept aside; back to the BrickIt stage: those softboxes go, the environment of the user comes back.
void BrickApp::setStudio(bool on) {
    if (on == m_studio) return;
    m_studio = on;
    if (on) {
        if (m_lights.empty()) {
            float3 c;
            const float k = studioScale(&c);
            studioRig(c, k);
            m_studioAuto = true;
            syncLights();
        }
        const std::wstring hdr = studioFile(L"HDR_02_Studio.exr");
        if (!hdr.empty() && !m_envKept) {
            m_envKept = true;
            m_keep.hdri = m_hdriPath;
            m_keep.rot = m_hdriRotDeg;
            m_keep.intensity = m_hdriInt;
            m_keep.tonemapper = (int)m_rs.tonemapper;
            m_keep.engine = -1;   // (the engine stays the one chosen)
            if (std::find(m_hdriPaths.begin(), m_hdriPaths.end(), hdr) == m_hdriPaths.end()) m_hdriPaths.push_back(hdr);
            m_hdriRotDeg = 51.0f;
            m_hdriInt = 0.5f;
            setHdri(hdr);
        }
    } else {
        if (m_studioAuto) {   // (the softboxes the stage brought, if they are still the only lights)
            m_studioAuto = false;
            if (m_lights.size() == 3 && m_lights[0].name.rfind("Softbox", 0) == 0) { m_lights.clear(); m_selLight = -1; syncLights(); }
        }
        if (m_envKept && !m_sceneLook) {
            m_envKept = false;
            m_hdriRotDeg = m_keep.rot;
            m_hdriInt = m_keep.intensity;
            if (m_keep.hdri != m_hdriPath) setHdri(m_keep.hdri);
        }
    }
    applyLook();
    savePrefs();
}

// the look of the studio stage, over the one applyLook made (called at its end)
void BrickApp::studioLook() {
    const bool on = m_studio;
    if (!m_scene.objects.empty()) m_scene.objects[0].visible = !on && !m_opt.noGround;
    if (m_cycloObj >= 0 && m_cycloObj < (int)m_scene.objects.size()) {
        vp3d::Object& c = m_scene.objects[m_cycloObj];
        c.visible = on;
        if (on) {   // big enough for the model: the cyclo of the lookdev scene (a brick), scaled with it, its floor under the model
            float3 mc;
            const float k = studioScale(&mc);
            c.scale = float3(k);
            c.position = float3(mc.x, 0.0f, mc.z);
            if (m_studioFloorY > -1e20f) { c.position = m_studioCyclo; c.scale = float3(m_studioCycloK); }   // (a scene that says where its cyclo is, and its size)
        }
    }
    if (!on) return;
    m_scene.sun.enabled = false;
    if (m_rimLight >= 0) m_scene.lights[m_rimLight].enabled = false;
    m_scene.env.hemiIntensity = 0.0f;
    m_scene.env.hdriIntensity = m_opt.hdriInt >= 0 ? m_opt.hdriInt : m_hdriInt;   // (as it is: no factor of the path tracer)
    if (m_hdriImg && !m_opt.noEnv) m_scene.env.hdri = m_hdriImg;
    m_scene.env.visibleToCamera = false;
    m_scene.env.top = m_scene.env.horizon = m_scene.env.bottom = float3(kStudioBg);
    m_scene.env.intensity = 1.0f;
    m_rs.bgOverride = float3(kStudioBg);
    m_rs.envSpecBoost = 1.0f;
    m_rs.exposure = m_opt.ptExposure > 0 ? m_opt.ptExposure : 1.0f;
    m_rs.sssScale = 1.0f;
    m_scene.gridLine = 0u;
}

// ------------------------------------------------------------------ the "lookdev" block of a scene file

static std::string f3(const float3& v) { char b[96]; snprintf(b, sizeof(b), "[%.5f, %.5f, %.5f]", v.x, v.y, v.z); return b; }
static std::string jq(const std::string& s) {
    std::string o = "\"";
    for (char c : s) { if (c == '"' || c == '\\') o.push_back('\\'); o.push_back(c); }
    return o + "\"";
}

bool BrickApp::hasSceneLook() const { return m_studio || m_studioShader || m_wideGamut || !m_lights.empty() || !m_cams.empty() || !m_sceneMats.empty(); }

std::string BrickApp::sceneLookJson() const {
    if (!hasSceneLook()) return std::string();
    char b[512];
    std::string j = ",\n \"lookdev\": {";
    j += std::string("\"studio\": ") + (m_studio ? "true" : "false") + ", \"studioPlastic\": " + (m_studioShader ? "true" : "false") + ", \"wideGamut\": " + (m_wideGamut ? "true" : "false");
    if (m_studio) {   // the stage: its environment and its render, part of the scene
        snprintf(b, sizeof(b), ", \"hdri\": %s, \"hdriRotation\": %.2f, \"hdriIntensity\": %.4f, \"engine\": %d, \"viewTransform\": %d",
                 jq(plat::toUtf8(m_hdriPath)).c_str(), m_hdriRotDeg, m_hdriInt, m_rcMode ? 4 : pathTraced() ? (m_rtDlss ? 3 : 2) : m_quality, (int)m_rs.tonemapper);
        j += b;
        if (m_studioFloorY > -1e20f) j += ", \"cyclo\": " + f3(m_studioCyclo) + ", \"cycloScale\": " + std::to_string(m_studioCycloK);
    }
    j += ",\n  \"lights\": [";
    for (size_t i = 0; i < m_lights.size(); ++i) {
        const UserLight& l = m_lights[i];
        snprintf(b, sizeof(b), "%s\n   {\"name\": %s, \"pos\": %s, \"rot\": [%.6f, %.6f, %.6f, %.6f], \"w\": %.4f, \"h\": %.4f, \"color\": %s, \"intensity\": %.4f, \"visible\": %s, \"targetOn\": %s, \"target\": %s}",
                 i ? "," : "", jq(l.name).c_str(), f3(l.pos).c_str(), l.rot.x, l.rot.y, l.rot.z, l.rot.w, l.w, l.h, f3(l.color).c_str(), l.intensity, l.visible ? "true" : "false", l.targetOn ? "true" : "false", f3(l.target).c_str());
        j += b;
    }
    j += "],\n  \"cameras\": [";
    const float deg = 180.0f / 3.14159265f;
    for (size_t i = 0; i < m_cams.size(); ++i) {
        const UserCam& c = m_cams[i];
        snprintf(b, sizeof(b), "%s\n   {\"name\": %s, \"target\": %s, \"yaw\": %.4f, \"pitch\": %.4f, \"distance\": %.4f, \"fov\": %.4f, \"locked\": %s, \"targetOn\": %s}",
                 i ? "," : "", jq(c.name).c_str(), f3(c.cam.target).c_str(), c.cam.yaw * deg, c.cam.pitch * deg, c.cam.distance, c.cam.fovY * deg, c.locked ? "true" : "false", c.targetOn ? "true" : "false");
        j += b;
    }
    j += "],\n  \"activeCamera\": " + std::to_string(m_inCam) + ",\n  \"materials\": [";
    bool first = true;
    for (const auto& [id, e] : m_sceneMats) {
        snprintf(b, sizeof(b), "%s\n   {\"id\": %d, \"c\": [%.4f, %.4f, %.4f], \"metal\": %.3f, \"trans\": %.3f, \"rough\": %.3f, \"sss\": %.3f, \"sssScale\": %.3f}",
                 first ? "" : ",", id, e.srgb.x, e.srgb.y, e.srgb.z, e.metal, e.trans, e.rough, e.sss, e.sssScale);
        j += b;
        if (e.ap1.x >= 0) { j.pop_back(); j += ", \"acescg\": " + f3(e.ap1) + "}"; }
        first = false;
    }
    j += "]\n }";
    return j;
}

// the scene as its file: the bricks (Builder::toJson) and the lookdev block before its last brace
std::string BrickApp::sceneJson() const {
    std::string t = m_build.toJson(m_scene.camera);
    const std::string look = sceneLookJson();
    if (look.empty()) return t;
    const size_t k = t.rfind('}');
    if (k == std::string::npos) return t;
    size_t e = k;
    while (e > 0 && (t[e - 1] == '\n' || t[e - 1] == ' ' || t[e - 1] == '\r')) --e;
    return t.substr(0, e) + look + "\n}\n";
}

// after a scene was read: its lookdev block applied; a scene without one after a scene with one: the lights, cameras and
// scene materials of that one go, the environment and the render of the user come back
void BrickApp::applySceneLook(const std::string& text) {
    if (m_frame < 2) { m_pendingLook = text; return; }   // (a scene opened at launch: once the stage and the renderer are up)
    app::Json p;
    const bool parsed = text.find("\"lookdev\"") != std::string::npos && app::Json::parse(text, p) && p["lookdev"].type == app::Json::Object;
    auto v3 = [](const app::Json& a, float3 def) { return a.type == app::Json::Array && a.a.size() >= 3 ? float3((float)a[0].num(), (float)a[1].num(), (float)a[2].num()) : def; };
    const bool had = m_sceneLook;
    if (!parsed) {
        if (!had) return;
        m_sceneLook = false;
        if (m_inCam >= 0) exitCamera();
        m_lights.clear();
        m_cams.clear();
        m_selLight = m_selCam = -1;
        m_sceneMats.clear();
        m_studioShader = false;
        m_wideGamut = false;
        m_studioFloorY = -1e30f;
        if (m_envKept) {   // the environment and the render as they were before the scene
            m_envKept = false;
            m_hdriRotDeg = m_keep.rot;
            m_hdriInt = m_keep.intensity;
            m_rs.tonemapper = m_keep.tonemapper;
            if (m_keep.engine >= 0) setRC(m_keep.engine == 4);
            if (m_keep.engine < 0) {}   // (kept by the stage switch: the engine as it is)
            else if (m_keep.engine >= 2 && m_keep.engine <= 3) { m_rtDlss = m_keep.engine == 3; m_rs.raster = false; }
            else if (m_keep.engine < 2) { m_quality = m_keep.engine; m_rs.msaaSamples = m_quality ? 4 : 1; m_rs.raster = true; }
            if (m_keep.hdri != m_hdriPath) setHdri(m_keep.hdri);
        }
        m_studio = false;
        syncLights();
        applyMatEdits();
        applyLook();
        return;
    }
    const app::Json& L = p["lookdev"];
    if (m_inCam >= 0) exitCamera();
    m_sceneLook = true;
    m_lights.clear();
    m_cams.clear();
    m_selLight = m_selCam = -1;
    m_sceneMats.clear();
    for (const app::Json& lj : L["lights"].a) {
        UserLight l;
        l.name = lj["name"].str("Area light");
        l.pos = v3(lj["pos"], l.pos);
        if (lj["rot"].type == app::Json::Array && lj["rot"].a.size() >= 4) l.rot = float4((float)lj["rot"][0].num(), (float)lj["rot"][1].num(), (float)lj["rot"][2].num(), (float)lj["rot"][3].num(1));
        l.w = (float)lj["w"].num(12);
        l.h = (float)lj["h"].num(12);
        l.color = v3(lj["color"], l.color);
        l.intensity = (float)lj["intensity"].num(25);
        l.visible = lj["visible"].type != app::Json::Bool || lj["visible"].b;
        l.targetOn = lj["targetOn"].type == app::Json::Bool && lj["targetOn"].b;
        l.target = v3(lj["target"], l.target);
        m_lights.push_back(l);
    }
    const float rad = 3.14159265f / 180.0f;
    for (const app::Json& cj : L["cameras"].a) {
        UserCam c;
        c.name = cj["name"].str("Camera");
        c.cam = m_scene.camera;
        c.cam.dof = false;
        c.cam.ortho = false;
        c.cam.target = v3(cj["target"], c.cam.target);
        c.cam.yaw = (float)cj["yaw"].num() * rad;
        c.cam.pitch = (float)cj["pitch"].num() * rad;
        c.cam.distance = (float)cj["distance"].num(20);
        c.cam.fovY = (float)cj["fov"].num(32) * rad;
        c.locked = cj["locked"].type == app::Json::Bool && cj["locked"].b;
        c.targetOn = cj["targetOn"].type == app::Json::Bool && cj["targetOn"].b;
        m_cams.push_back(c);
    }
    for (const app::Json& mj : L["materials"].a) {
        MatEdit e;
        e.srgb = v3(mj["c"], e.srgb);
        e.metal = (float)mj["metal"].num(0);
        e.trans = (float)mj["trans"].num(0);
        e.rough = (float)mj["rough"].num(0.24);
        e.sss = (float)mj["sss"].num(-1);
        e.sssScale = (float)mj["sssScale"].num(1);
        if (mj["acescg"].type == app::Json::Array) e.ap1 = v3(mj["acescg"], e.ap1);
        m_sceneMats[(int)mj["id"].num(-1)] = e;
    }
    m_studioShader = L["studioPlastic"].type == app::Json::Bool && L["studioPlastic"].b;
    m_wideGamut = L["wideGamut"].type == app::Json::Bool && L["wideGamut"].b;
    const bool studio = L["studio"].type == app::Json::Bool && L["studio"].b;
    m_studioFloorY = -1e30f;
    if (L["cyclo"].type == app::Json::Array) { m_studioCyclo = v3(L["cyclo"], float3(0.0f)); m_studioFloorY = m_studioCyclo.y; m_studioCycloK = (float)L["cycloScale"].num(1.0); }
    if (studio) {
        if (!m_envKept) {   // (kept once: a second studio scene does not replace what the user had)
            m_envKept = true;
            m_keep.hdri = m_hdriPath;
            m_keep.rot = m_hdriRotDeg;
            m_keep.intensity = m_hdriInt;
            m_keep.tonemapper = (int)m_rs.tonemapper;
            m_keep.engine = m_rcMode ? 4 : pathTraced() ? (m_rtDlss ? 3 : 2) : m_quality;
        }
        if (L["hdriRotation"].type == app::Json::Number) m_hdriRotDeg = (float)L["hdriRotation"].n;
        if (L["hdriIntensity"].type == app::Json::Number) m_hdriInt = (float)L["hdriIntensity"].n;
        if (L["viewTransform"].type == app::Json::Number) m_rs.tonemapper = (uint32_t)L["viewTransform"].n;
        if (L["engine"].type == app::Json::Number && !m_ipad) {
            const int e = (int)L["engine"].n;
            setRC(e == 4);
            if (e == 2 || e == 3) { m_rtDlss = e == 3; m_rs.raster = false; }
            else if (e < 2) { m_quality = e; m_rs.msaaSamples = m_quality ? 4 : 1; m_rs.raster = true; }
        }
        if (L["hdri"].type == app::Json::String) {   // its HDRI: the file of the scene, or the one of that name in the studio folder
            std::wstring h = plat::fromUtf8(L["hdri"].s);
            if (!h.empty() && !plat::fileExists(h)) {
                const size_t s = h.find_last_of(L"\\/");
                const std::wstring alt = studioFile(h.substr(s == std::wstring::npos ? 0 : s + 1).c_str());
                if (!alt.empty()) h = alt;
            }
            if (h.empty() || plat::fileExists(h)) {
                if (!h.empty() && std::find(m_hdriPaths.begin(), m_hdriPaths.end(), h) == m_hdriPaths.end()) m_hdriPaths.push_back(h);
                if (h != m_hdriPath || !m_hdriImg) setHdri(h);
            }
        }
    }
    m_studio = studio;
    syncLights();
    applyMatEdits();   // (the materials of the scene, then setLookdev)
    applyLook();
    const int ac = (int)L["activeCamera"].num(-1);
    if (ac >= 0 && ac < (int)m_cams.size()) enterCamera(ac);
}

// ------------------------------------------------------------------ the lookdev scene of the yellow brick

// The scene of Wzx's Maya file (Wall-E_Lighting_06_Shading_RD.mb, read with mayapy 24/11/2026), in its own coordinates
// (cm, Y up, as BrickIt): a yellow brick 1 x 4 turned 17.2 degrees, 2.04 cm above the floor of the cyclo, three softboxes
// (aiAreaLight, not normalised: their radiance is intensity x 2^exposure), the dome HDR_02_Studio at 0.5 turned so that
// its white wall is where Arnold shows it (51 degrees), BrickCam (90 mm on a 36 mm film).
void BrickApp::studioBrickScene() {
    const int part = m_lib.find("3010");
    if (part < 0) { toast("Brick 1 x 4 (3010) not found in the library"); return; }
    m_lib.get("3010");   // (its geometry: the bounds below)
    pushUndo();
    m_bricks.clear();
    m_selected = -1;
    m_multi.clear();
    if (m_inCam >= 0) exitCamera();
    Brick b;
    b.part = part;
    b.color = 0;
    for (size_t i = 0; i < m_lib.palette.size(); ++i) if (m_lib.palette[i].id == 14) b.color = (int)i;   // Yellow
    const float yaw = 17.2f * 3.14159265f / 180.0f;
    b.rot = float4(0.0f, std::sin(yaw * 0.5f), 0.0f, std::cos(yaw * 0.5f));
    b.free = true;
    b.pos = float3(0.0f);
    {   // its centre where the brick of the Maya scene is
        const Box bx = m_build.aabbOf(b.part, b.rot, b.pos);
        const float3 c = (bx.mn + bx.mx) * 0.5f;
        b.pos = float3(-0.29f - c.x, 2.04f - bx.mn.y, -1.165f - c.z);
    }
    m_bricks.push_back(b);
    m_lights.clear();
    m_studioAuto = false;
    studioRig(float3(-0.29f, 2.64f, -1.165f), 1.0f);   // (around the centre of the brick of the Maya scene)
    m_cams.clear();
    {
        UserCam c;
        c.name = "BrickCam 90 mm";
        c.cam = m_scene.camera;
        c.cam.dof = false;
        c.cam.ortho = false;
        const float3 eye(-4.5943f, 4.9574f, 4.9905f), fwd = normalize(float3(0.52985f, -0.30467f, -0.79148f));
        c.cam.distance = 7.857f;
        c.cam.target = eye + fwd * c.cam.distance;
        c.cam.pitch = std::asin(-fwd.y);
        c.cam.yaw = std::atan2(-fwd.x, -fwd.z);
        c.cam.fovY = 22.62f * 3.14159265f / 180.0f;
        c.locked = true;
        m_cams.push_back(c);
    }
    m_sceneLook = true;
    m_sceneMats.clear();
    {   // BRICK_YELLOW_shd: base 0.7 x (1, 0.4755, 0.026) for half of the light, the subsurface colour (1, 0.4231, 0) for
        // the other half, in ACEScg: (0.85, 0.378, 0.009), in sRGB primaries (1.21, 0.32, -0.06), kept in gamut
        MatEdit e;
        e.srgb = float3(1.0f, 0.602f, 0.0f);   // (linear 1.0, 0.32, 0.0)
        e.ap1 = float3(0.85f, 0.378f, 0.0091f);
        e.rough = kStudioRough;
        e.sss = -1;
        m_sceneMats[14] = e;
    }
    m_studioShader = true;
    m_wideGamut = m_opt.wideGamut != 0;   // (ACEScg, as the Maya scene)
    m_studioCyclo = float3(0.0f);
    m_studioFloorY = 0.0f;
    m_studioCycloK = 1.0f;
    if (!m_envKept) {
        m_envKept = true;
        m_keep.hdri = m_hdriPath;
        m_keep.rot = m_hdriRotDeg;
        m_keep.intensity = m_hdriInt;
        m_keep.tonemapper = (int)m_rs.tonemapper;
        m_keep.engine = m_rcMode ? 4 : pathTraced() ? (m_rtDlss ? 3 : 2) : m_quality;
    }
    const std::wstring hdr = studioFile(L"HDR_02_Studio.exr");
    if (!hdr.empty()) {
        if (std::find(m_hdriPaths.begin(), m_hdriPaths.end(), hdr) == m_hdriPaths.end()) m_hdriPaths.push_back(hdr);
        setHdri(hdr);
    }
    m_hdriRotDeg = 51.0f;
    m_hdriInt = 0.5f;
    m_rs.tonemapper = m_opt.tonemapper;   // (ACES, as the view of the Maya scene)
    if (!m_ipad) { setRC(false); m_rtDlss = false; m_rs.raster = false; }
    m_studio = true;
    rebuildScene();
    syncLights();
    applyMatEdits();
    applyLook();
    enterCamera(0);
    m_modelName = "Yellow brick studio";
}

} // namespace brickit
