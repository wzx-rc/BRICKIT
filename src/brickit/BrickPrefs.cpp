// BrickIt - preferences and automatic save, as the web keeps them in localStorage (vp3d-builder-prefs,
// vp3d-builder-scene, vp3d-recent-parts, vp3d-fav-parts, vp3d-thumb-size): JSON files next to the exe. The sample scene
// of a first launch, the .ldr export and the « Scène partagée » sheet.
#include "BrickApp.h"
#include "app/Json.h"
#include <cmath>
#include <cstdio>

namespace brickit {

static bool readText(const std::wstring& path, std::string& out) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) return false;
    char buf[65536];
    size_t n;
    out.clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    fclose(f);
    return true;
}
static void writeText(const std::wstring& path, const std::string& t) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return;
    fwrite(t.data(), 1, t.size(), f);
    fclose(f);
}
static std::string narrowW(const std::wstring& w) {
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
static std::wstring widenU(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
static std::string jstr(const std::string& s) {
    std::string o = "\"";
    for (char c : s) { if (c == '"' || c == '\\') o.push_back('\\'); o.push_back(c); }
    return o + "\"";
}

// Test, demo and capture runs (and the mechanics diagnostic) never write the preferences or the autosaved scene of the
// user: a --mechtests run had replaced brickit_scene.v3d.json by a test scene.
bool BrickApp::userSession() const {
    return m_opt.shotPath.empty() && !m_opt.mechTests && !m_opt.demo && !m_opt.grid && m_opt.bench <= 0 && !m_opt.mechDemo &&
           !m_opt.mech && !m_opt.cornell && m_opt.mouseTest.empty();
}

// The defaults of a first launch (no brickit_prefs.json): those saved by Réglages › « Enregistrer comme réglages par
// défaut » (%LOCALAPPDATA%\\BrickIt\\default_prefs.json), else the built-in ones (assets\\brickit\\default_prefs.json, the
// look Wzx chose)
static std::wstring userDefaultsPath() {
    const std::wstring d = brickitDataDir();
    if (d.empty()) return std::wstring();
    const std::wstring f = d + L"\\default_prefs.json";
    if (GetFileAttributesW(f.c_str()) == INVALID_FILE_ATTRIBUTES)   // (saved by an earlier build in AppData: moved here)
        for (const std::wstring& o : legacyDataDirs())
            if (CopyFileW((o + L"\\default_prefs.json").c_str(), f.c_str(), TRUE)) break;
    return f;
}

bool BrickApp::readDefaultPrefs(std::string& t) {
    const std::wstring u = userDefaultsPath();
    return (!u.empty() && readText(u, t)) || readText(m_exeDir + L"\\assets\\brickit\\default_prefs.json", t);
}

void BrickApp::saveDefaultPrefs() {
    const std::wstring u = userDefaultsPath();
    if (u.empty()) return;
    CreateDirectoryW(u.substr(0, u.find_last_of(L'\\')).c_str(), nullptr);
    writeText(u, prefsJson());
    toast("Réglages enregistrés comme réglages par défaut");
}

void BrickApp::resetToDefaultPrefs() {
    std::string t;
    if (!readDefaultPrefs(t)) { toast("Pas de réglages par défaut"); return; }
    applyPrefs(t);
    applyLook();
    m_renderer->resetAccumulation();
    savePrefs();
    toast("Réglages par défaut rétablis");
}

void BrickApp::savePrefs() {
    if (!userSession()) return;   // tests and captures never touch the preferences of the user
    writeText(m_exeDir + L"\\brickit_prefs.json", prefsJson());
}

std::string BrickApp::prefsJson() {
    std::string j = "{";
    char b[256];
    snprintf(b, sizeof(b), "\"safety50\": %s, \"gridSnap\": %s, \"autoPivot\": %s, \"animOn\": %s, \"focusDim\": %s, \"mode\": %d, \"quality\": %d, \"tonemap\": %d, \"colorId\": %d, \"thumbSize\": %d,",
             m_build.safety ? "true" : "false", m_build.gridSnap ? "true" : "false", m_autoPivot ? "true" : "false", m_animOn ? "true" : "false",
             m_focusDim ? "true" : "false", m_mode, m_quality, m_rs.tonemapper, m_lib.palette[m_color].id, m_thumbSize);
    j += b;
    j += "\"nav\": {\"fam\": " + jstr(m_fam) + ", \"cat\": " + jstr(m_cat) + ", \"sub\": " + jstr(m_sub) + "},";
    auto panel = [&](const char* n, const PanelState& p) { char t[192]; snprintf(t, sizeof(t), "\"%s\": {\"visible\": %s, \"collapsed\": %s, \"floating\": %s, \"x\": %d, \"y\": %d}", n, p.visible ? "true" : "false", p.collapsed ? "true" : "false", p.floating ? "true" : "false", (int)p.x, (int)p.y); return std::string(t); };
    j += std::string("\"optsRight\": ") + (m_optsRight ? "true" : "false") + ",";
    j += "\"panels\": {" + panel("tray", m_pTray) + ", " + panel("opts", m_pOpts) + ", " + panel("colors", m_pColors) + ", " + panel("hud", m_pHud) + ", " + panel("manual", m_pManual) + "},";
    auto names = [&](const std::vector<int>& v) { std::string s = "["; for (size_t i = 0; i < v.size(); ++i) s += (i ? ", " : "") + jstr(m_lib.parts[v[i]].name); return s + "]"; };
    j += std::string("\"dev\": {\"on\": ") + (m_dev ? "true" : "false") + ", \"showVox\": " + (m_devShowVox ? "true" : "false") + ", \"showEdges\": " + (m_devShowEdges ? "true" : "false") + ", \"useOverrides\": " + (m_devUseOverrides ? "true" : "false") + "},";
    { char mb[160]; snprintf(mb, sizeof(mb), "\"manual\": {\"w\": %d, \"h\": %d, \"dock\": %d, \"pdfPage\": %d, \"pdfName\": ", (int)m_manW, (int)m_manH, m_manDock, m_pdfPage); j += mb; j += jstr(m_pdfName) + "},"; }
    { char sb[400]; snprintf(sb, sizeof(sb), "\"settings\": {\"uiScale\": %.2f, \"ssao\": %s, \"autosave\": %s, \"showVox\": %s, \"renderScale\": %.2f, \"stillAA\": %s, \"touchHud\": %s, \"selHud\": %s, \"uiTheme\": %d, \"lightPreset\": %d, \"pencilOnly\": %s},", m_uiScaleUser, m_ssaoOn ? "true" : "false", m_autosaveOn ? "true" : "false", m_showVox ? "true" : "false", m_renderScale, m_stillAA ? "true" : "false", m_touchHud ? "true" : "false", m_selHud ? "true" : "false", m_uiThemePref, m_lightPreset, m_pencilOnly ? "true" : "false"); j += sb; }
    // the scene of the cloud (BrickCloud.cpp): its id, the version last sent or read, the automatic sync
    j += "\"cloud\": {\"id\": " + jstr(m_cloudId) + ", \"ver\": " + std::to_string(m_cloudVer) + ", \"auto\": " + (m_cloudAuto ? "true" : "false") + ", \"pending\": " + (m_cloudDirty ? "true" : "false") + "},";   // (pending: changes not sent yet, sent at the next start)
    {   // Réglages › Thème: the theme, and the tint and accent of each theme (BrickTheme.h; PLUM's ids)
        j += "\"theme\": {\"id\": " + jstr(m_themeId) + ", \"looks\": {";
        bool first = true;
        for (const auto& kv : m_themeLooks) {
            char lb[96];
            snprintf(lb, sizeof(lb), "%s%s: [%d, %d]", first ? "" : ", ", jstr(kv.first).c_str(), kv.second.tint, kv.second.accent);
            j += lb;
            first = false;
        }
        j += "}, \"studioUi\": ";
        j += m_studioUi ? "true" : "false";
        j += ", \"libCards\": " + std::to_string(m_stCardMode);
        j += ", \"cardScale\": " + std::to_string(m_stCardScale);
        j += std::string(", \"highPoly\": ") + (m_highPoly ? "true" : "false");
        j += ", \"libDock\": " + std::to_string(m_libDock) + ", \"libH\": " + std::to_string(m_libH) + ", \"libW\": " + std::to_string(m_libW);
        j += std::string(", \"libClassic\": ") + (m_libClassic ? "true" : "false");
        j += ", \"paintGroup\": ";
        j += m_paintGroup ? "true" : "false";
        j += std::string(", \"bevel\": ") + (m_rs.bevel ? "true" : "false");
        j += ", \"appIcon\": " + std::to_string(m_appIcon);
        j += ", \"ghostCol\": " + std::to_string(m_ghostCol) + ", \"ghostAlpha\": " + std::to_string(m_ghostAlphaPref) + ", \"ghostGlow\": " + std::to_string(m_ghostGlow);
        j += ", \"hideDeco\": ";
        j += m_hideDeco ? "true" : "false";
        j += "},";
    }
    {   // Réglages > Lumière
        char lb[512];
        const uint32_t rgb = (uint32_t)std::lround(m_gridRgb.x * 255) | ((uint32_t)std::lround(m_gridRgb.y * 255) << 8) | ((uint32_t)std::lround(m_gridRgb.z * 255) << 16);
        snprintf(lb, sizeof(lb), "\"light\": {\"rotation\": %.1f, \"intensity\": %.3f, \"gridRgb\": %u, \"gridAlpha\": %.3f, \"softGI\": %s, \"giStrength\": %.3f, \"giHold\": %s, \"sssStrength\": %.3f, \"giProbes\": %d, \"showProbes\": %s, \"selOutline\": %.3f, \"selFill\": %.3f, \"dof\": %.3f, \"bloom\": %.3f, \"focusMode\": %d, \"focusCm\": %.1f, \"focusReticle\": %s, \"hdri\": ", m_hdriRotDeg, m_hdriInt, rgb, m_gridAlpha, m_softGI ? "true" : "false", m_softGIStrength, m_softGIHold ? "true" : "false", m_hqSSSStrength, m_softGIProbes, m_showProbes ? "true" : "false", m_selOutline, m_selFill, m_dof, m_bloom, m_focusMode, m_focusManual, m_focusReticle ? "true" : "false");
        j += lb;
        j += jstr(narrowW(m_hdriPath)) + ", \"custom\": [";
        for (size_t i = 0; i < m_hdriCustom.size(); ++i) j += (i ? ", " : "") + jstr(narrowW(m_hdriCustom[i]));
        j += "]},";
    }
    j += "\"recentParts\": " + names(m_recent) + ", \"favParts\": " + names(m_fav) + "}\n";
    return j;
}

void BrickApp::loadPrefs() {
    std::string t;
    if (!readText(m_exeDir + L"\\brickit_prefs.json", t) && !readDefaultPrefs(t)) return;   // a first launch: the defaults
    applyPrefs(t);
}

void BrickApp::applyPrefs(const std::string& t) {
    app::Json p;
    if (!app::Json::parse(t, p)) return;
    auto flag = [&](const char* k, bool& v) { if (p[k].type == app::Json::Bool) v = p[k].b; };
    flag("safety50", m_build.safety);
    flag("gridSnap", m_build.gridSnap);
    flag("autoPivot", m_autoPivot);
    flag("animOn", m_animOn);
    flag("focusDim", m_focusDim);
    flag("optsRight", m_optsRight);
    if (p["mode"].type == app::Json::Number) m_mode = (int)p["mode"].n;
    if (p["quality"].type == app::Json::Number) m_quality = (int)p["quality"].n;
    if (p["tonemap"].type == app::Json::Number) m_rs.tonemapper = (int)p["tonemap"].n;
    if (p["thumbSize"].type == app::Json::Number) m_thumbSize = std::clamp((int)p["thumbSize"].n, 0, 2);
    if (p["colorId"].type == app::Json::Number) m_color = colorIndex((int)p["colorId"].n);
    if (p["nav"].type == app::Json::Object) { m_fam = p["nav"]["fam"].str(m_fam); m_cat = p["nav"]["cat"].str(); m_sub = p["nav"]["sub"].str(); }
    auto panel = [&](const char* n, PanelState& ps) {
        const app::Json& q = p["panels"][n];
        if (q["visible"].type == app::Json::Bool) ps.visible = q["visible"].b;
        if (q["collapsed"].type == app::Json::Bool) ps.collapsed = q["collapsed"].b;
        if (q["floating"].type == app::Json::Bool) ps.floating = q["floating"].b;
        ps.x = (float)q["x"].num(ps.x);
        ps.y = (float)q["y"].num(ps.y);
    };
    panel("tray", m_pTray); panel("opts", m_pOpts); panel("colors", m_pColors); panel("hud", m_pHud); panel("manual", m_pManual);
    auto list = [&](const char* k, std::vector<int>& v) { v.clear(); for (const app::Json& n : p[k].a) { int i = m_lib.find(n.str()); if (i >= 0) v.push_back(i); } };
    list("recentParts", m_recent);
    list("favParts", m_fav);
    {   // Réglages > Lumière
        const app::Json& lt = p["light"];
        if (lt.type == app::Json::Object) {
            if (lt["rotation"].type == app::Json::Number) m_hdriRotDeg = (float)lt["rotation"].n;
            if (lt["intensity"].type == app::Json::Number) m_hdriInt = (float)lt["intensity"].n;
            if (lt["gridAlpha"].type == app::Json::Number) m_gridAlpha = (float)lt["gridAlpha"].n;
            if (lt["gridRgb"].type == app::Json::Number) {
                const uint32_t c = (uint32_t)lt["gridRgb"].n;
                m_gridRgb = float3((c & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, ((c >> 16) & 255) / 255.0f);
            }
            if (lt["hdri"].type == app::Json::String) m_hdriPath = widenU(lt["hdri"].str());
            if (lt["softGI"].type == app::Json::Bool) m_softGI = lt["softGI"].b;
        if (lt["giStrength"].type == app::Json::Number) m_softGIStrength = std::clamp((float)lt["giStrength"].n, 0.0f, 2.0f);
        if (lt["giHold"].type == app::Json::Bool) m_softGIHold = lt["giHold"].b;
        // (the depth of field is not taken back: the app always opens without it, Wzx 09/11/2026)
        if (lt["focusMode"].type == app::Json::Number) m_focusMode = std::clamp((int)lt["focusMode"].n, 0, 2);
        if (lt["focusReticle"].type == app::Json::Bool) m_focusReticle = lt["focusReticle"].b;
        if (lt["focusCm"].type == app::Json::Number) m_focusManual = std::clamp((float)lt["focusCm"].n, 2.0f, 400.0f);
        if (lt["bloom"].type == app::Json::Number) m_bloom = std::clamp((float)lt["bloom"].n, 0.0f, 1.0f);
        if (lt["selOutline"].type == app::Json::Number) m_selOutline = std::clamp((float)lt["selOutline"].n, 0.0f, 1.0f);
        if (lt["selFill"].type == app::Json::Number) m_selFill = std::clamp((float)lt["selFill"].n, 0.0f, 0.5f);
        if (lt["showProbes"].type == app::Json::Bool) m_showProbes = lt["showProbes"].b;
        if (lt["giProbes"].type == app::Json::Number) m_softGIProbes = std::clamp((int)lt["giProbes"].n, 1024, 16384);
        if (lt["sssStrength"].type == app::Json::Number) m_hqSSSStrength = std::clamp((float)lt["sssStrength"].n, 0.0f, 2.0f);
            m_hdriCustom.clear();
            for (const app::Json& c : lt["custom"].a) m_hdriCustom.push_back(widenU(c.str()));
        }
    }
    {   // native settings (gear button)
        const app::Json& st = p["settings"];
        if (st["uiScale"].num() >= 0.5) m_uiScaleUser = (float)st["uiScale"].num();
        if (st["ssao"].type == app::Json::Bool) m_ssaoOn = st["ssao"].b;
        if (st["autosave"].type == app::Json::Bool) m_autosaveOn = st["autosave"].b;
        // (the voxels shown is not kept from one session to the next: it shows the selected brick at 30 %, Wzx 12/11/2026)
        if (st["aa"].type == app::Json::Number) m_rs.aaQuality = std::clamp((int)st["aa"].n, 0, 2);
        if (st["stillAA"].type == app::Json::Bool) m_stillAA = st["stillAA"].b;
        if (st["touchHud"].type == app::Json::Bool) m_touchHud = st["touchHud"].b;
        if (st["selHud"].type == app::Json::Bool) m_selHud = st["selHud"].b;
        if (st["uiTheme"].type == app::Json::Number) m_uiThemePref = std::clamp((int)st["uiTheme"].n, 0, 2);
        const app::Json& tj = p["theme"];
        if (tj["id"].type == app::Json::String && theme::indexOf(tj["id"].s) >= 0) m_themeId = tj["id"].s;
        if (tj["libCards"].type == app::Json::Number) m_stCardMode = std::clamp((int)tj["libCards"].n, 0, 1);
        if (tj["hideDeco"].type == app::Json::Bool) m_hideDeco = tj["hideDeco"].b;
        if (tj["paintGroup"].type == app::Json::Bool) m_paintGroup = tj["paintGroup"].b;
        if (tj["bevel"].type == app::Json::Bool) m_rs.bevel = tj["bevel"].b;
        if (tj["ghostCol"].type == app::Json::Number) m_ghostCol = std::clamp((int)tj["ghostCol"].n, 0, 5);
        if (tj["ghostAlpha"].type == app::Json::Number) m_ghostAlphaPref = std::clamp((float)tj["ghostAlpha"].n, 0.05f, 1.0f);
        if (tj["appIcon"].type == app::Json::Number) m_appIcon = std::clamp((int)tj["appIcon"].n, 0, 1);
        if (tj["ghostGlow"].type == app::Json::Number) m_ghostGlow = std::clamp((int)tj["ghostGlow"].n, 0, 3);
        if (tj["cardScale"].type == app::Json::Number) m_stCardScale = std::clamp((float)tj["cardScale"].n, 0.6f, 2.0f);
        if (tj["highPoly"].type == app::Json::Bool) m_highPoly = tj["highPoly"].b;
        if (tj["libDock"].type == app::Json::Number) m_libDock = std::clamp((int)tj["libDock"].n, 0, 2);
        if (tj["libH"].type == app::Json::Number) m_libH = std::max(0.0f, (float)tj["libH"].n);
        if (tj["libW"].type == app::Json::Number) m_libW = std::max(0.0f, (float)tj["libW"].n);
        if (tj["libClassic"].type == app::Json::Bool) m_libClassic = tj["libClassic"].b;
        if (tj["studioUi"].type == app::Json::Bool) m_studioUi = tj["studioUi"].b;
        else m_themeId = "glass";   // (the first launch of the Studio interface: its theme, Glass)
        for (const auto& kv : tj["looks"].o)
            if (kv.second.type == app::Json::Array && theme::indexOf(kv.first) >= 0) m_themeLooks[kv.first] = {(int)kv.second[0].num(), (int)kv.second[1].num()};
        if (st["lightPreset"].type == app::Json::Number) m_lightPreset = (int)st["lightPreset"].n;
        if (st["pencilOnly"].type == app::Json::Bool) m_pencilOnly = st["pencilOnly"].b;
        if (st["renderScale"].type == app::Json::Number) m_renderScale = std::clamp((float)st["renderScale"].n, 0.5f, 1.0f);
        if (st["shadows"].type == app::Json::Number) m_rs.shadowQuality = std::clamp((int)st["shadows"].n, 0, 4);
        if (st["rc"].type == app::Json::Bool && st["rc"].b) setRC(true);
    }
    {   // vp3d-manual-w / vp3d-manual-h / vp3d-manual-pdf-page
        const app::Json& cl = p["cloud"];
        if (cl.type == app::Json::Object) {
            m_cloudId = cl["id"].str();
            m_cloudVer = (int)cl["ver"].num();
            if (cl["auto"].type == app::Json::Bool) m_cloudAuto = cl["auto"].b;
            if (cl["pending"].type == app::Json::Bool && cl["pending"].b) { m_cloudDirty = true; m_cloudDirtyAt = 0; m_cloudSendAt = 0; }
        }
        const app::Json& mn = p["manual"];
        if (mn["w"].num() >= 260) m_manW = (float)mn["w"].num();
        if (mn["h"].num() >= 160) m_manH = (float)mn["h"].num();
        if (mn["dock"].type == app::Json::Number) m_manDock = std::clamp((int)mn["dock"].n, 0, 2);
        if (mn["pdfPage"].num() >= 1) m_pdfPage = (int)mn["pdfPage"].num();
        m_pdfName = mn["pdfName"].str();
    }
    {   // brickit-dev-prefs of the web
        const app::Json& dv = p["dev"];
        if (dv["on"].type == app::Json::Bool) m_dev = dv["on"].b;
        if (dv["showVox"].type == app::Json::Bool) m_devShowVox = dv["showVox"].b;
        if (dv["showEdges"].type == app::Json::Bool) m_devShowEdges = dv["showEdges"].b;
        if (dv["useOverrides"].type == app::Json::Bool) m_devUseOverrides = dv["useOverrides"].b;
    }
    m_rs.msaaSamples = m_quality ? 4 : 1;
}

void BrickApp::autosave() {
    if (m_cornell) return;
    manualDirty();   // the steps change with the scene
    cloudNoteChange();   // (sent to the cloud a few seconds later)
    if (userSession() && m_autosaveOn && !m_autosaveGuard) writeText(m_exeDir + L"\\brickit_scene.v3d.json", m_build.toJson(m_scene.camera));
}

bool BrickApp::restoreAutosave() {
    std::string t, msg;
    // no scene of a last session: the default scene (assets\\brickit\\default_scene.v3d.json, the official house 31153)
    if (!readText(m_exeDir + L"\\brickit_scene.v3d.json", t) && !readText(m_exeDir + L"\\assets\\brickit\\default_scene.v3d.json", t)) return false;
    if (!m_build.fromJson(t, &m_scene.camera, &msg) || m_bricks.empty()) return false;
    toast(msg);
    // parts of the last session not found (the full library missing, 06/11/2026: the autosave was rewritten without
    // them): the file is kept as it is, a copy made, and the autosave stays off for this session
    if (m_build.lastSkipped > 0 && userSession()) {
        m_autosaveGuard = true;
        writeText(m_exeDir + L"\\brickit_scene.v3d.json.complet.bak", t);
        char b[200];
        snprintf(b, sizeof(b), "%d pièces de la dernière scène introuvables (bibliothèque complète absente ?) : la scène enregistrée n'est pas écrasée", m_build.lastSkipped);
        toast(b);
    }
    return true;
}

#if __has_include("brickit_version.h")
#include "brickit_version.h"
#endif
#ifndef BRICKIT_VERSION
#define BRICKIT_VERSION L"0.00"
#define BRICKIT_BRANCH L"local"
#define BRICKIT_COMMIT L"nogit"
#define BRICKIT_PLATFORM L"PC Win Local"
#endif
std::wstring BrickApp::versionString() {
    return std::wstring(L"BrickIt - ") + BRICKIT_VERSION + L" - " + BRICKIT_BRANCH + L" - " + BRICKIT_PLATFORM + L" - " + BRICKIT_COMMIT;
}

void BrickApp::writeStartLog() {
    if (!userSession()) return;
    const std::wstring dd = brickitDataDir();
    wchar_t cwd[MAX_PATH] = L"";
    GetCurrentDirectoryW(MAX_PATH, cwd);
    int full = 0;
    for (const Part& p : m_lib.parts) full += p.pack >= m_lib.technicPacks;
    std::string o;
    char b[512];
    SYSTEMTIME st;
    GetLocalTime(&st);
    const std::wstring v = versionString();
    snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d %ls\nexe: %ls\ncurrent dir: %ls\ndata: %ls\nfull library: %ls\nparts: %d (full library %d, packs %d, technic packs %d)\nscene: %d bricks%s\n",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, v.c_str(), m_exeDir.c_str(), cwd, dd.c_str(),
             m_lib.fullDir.empty() ? L"(introuvable)" : m_lib.fullDir.c_str(), (int)m_lib.parts.size(), full, m_lib.packCount, m_lib.technicPacks,
             (int)m_bricks.size(), m_autosaveGuard ? " (incomplete: autosave kept)" : "");
    o = b;
    writeText(m_exeDir + L"\\brickit_start.log", o);
    if (m_lib.fullDir.empty()) toast("Bibliothèque complète introuvable (dossier utilisateur \\BrickIt\\full) : seules les pièces de base sont là");
}

// The scene of a first launch (sampleScene of the web).
void BrickApp::sampleScene() {
    auto put = [&](const char* n, int c, float sx, float py, float sz) {
        int pi = m_lib.find(n);
        if (pi < 0 || !m_lib.get(n)) return;
        Brick b;
        b.part = pi;
        b.color = colorIndex(c);
        b.pos = float3(sx * kPitch, py * kPlate, sz * kPitch);
        m_bricks.push_back(b);
    };
    put("3001", 4, -2, 0, -1); put("3001", 1, -1, 3, -1); put("3003", 14, 3, 0, 1); put("3004", 15, 3, 3, 1);
    put("3062a", 10, 6, 0, -1); put("3062a", 10, 6, 3, -1); put("15068", 25, 1, 6, -1);
    put("2357", 15, -5, 0, 2); put("3063b", 322, 0, 0, 3); put("3009", 71, -4, 0, -4); put("2456", 272, -4, 3, -5);
    Camera& c = m_scene.camera;   // cam of the web: target (0.4, 1.2, 0.4), yaw 38, pitch 32, distance 26
    c.target = float3(0.4f, 1.2f, 0.4f);
    c.yaw = radians(38.0f);
    c.pitch = radians(32.0f);
    c.distance = 26.0f;
}

// LDraw export (sceneLdr of the web): LDraw space = BrickIt space with Y and Z flipped, units / 0.04 cm.
std::string BrickApp::sceneLdr() const {
    std::string out = "0 BrickIt scene\r\n0 Name: brickit_scene.ldr\r\n0 Author: BrickIt Builder LDraw\r\n0 BFC CERTIFY CCW\r\n\r\n";
    auto n = [](float v) {
        float r = std::round(v * 10000.0f) / 10000.0f;
        if (r == 0) return std::string("0");
        char b[32];
        snprintf(b, sizeof(b), "%g", r);
        return std::string(b);
    };
    for (const Brick& b : m_bricks) {
        const Part& p = m_lib.parts[b.part];
        float3 w = qRot(b.rot, p.ldrawOrigin) + b.pos;
        // M = C R C with C = diag(1, -1, -1)
        float3 X = qRot(b.rot, float3(1, 0, 0)), Y = qRot(b.rot, float3(0, 1, 0)), Z = qRot(b.rot, float3(0, 0, 1));
        float R[3][3] = {{X.x, Y.x, Z.x}, {X.y, Y.y, Z.y}, {X.z, Y.z, Z.z}};
        const float C[3] = {1, -1, -1};
        float M[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) M[i][j] = C[i] * R[i][j] * C[j];
        out += "1 " + std::to_string(m_lib.palette[b.color].id) + " " + n(w.x / 0.04f) + " " + n(-w.y / 0.04f) + " " + n(-w.z / 0.04f);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) out += " " + n(M[i][j]);
        out += " " + (p.ldraw.empty() ? p.name : p.ldraw) + ".dat\r\n";
    }
    return out;
}

void BrickApp::pickUpSelected() {
    if (m_selected < 0 || m_holding) return;
    m_pressPoint = m_build.worldVoxel(m_bricks[m_selected], m_build.grabOf(m_bricks[m_selected]));
    ImVec2 mp = ImGui::GetIO().MousePos;
    beginMove(m_selected, mp.x, mp.y);
}

void BrickApp::cycleGrab() {
    int b = m_holding ? m_held.brick : m_selected;
    if (b < 0) return;
    const Part& p = m_lib.parts[m_bricks[b].part];
    if (p.voxels.empty()) return;
    int g = m_bricks[b].grab;
    g = (g + 1) % (int)p.voxels.size();
    m_bricks[b].grab = g;
    if (m_holding) {
        m_held.grab = p.voxels[g];
        ImVec2 mp = ImGui::GetIO().MousePos;
        updateHeld(mp.x, mp.y);
    }
    char t[96];
    snprintf(t, sizeof(t), "Pivot d'accroche : voxel %d / %d", g + 1, (int)p.voxels.size());
    toast(t);
    autosave();
}

// the labels of the connectors set by hand (mode DEV, a right click on a card): <data>/conn_labels.json, { "3001": 4, ... }
void BrickApp::loadConnLabels() {
    std::string t;
    const std::wstring d = brickitDataDir();
    if (d.empty() || !readText(d + L"\\conn_labels.json", t)) return;
    app::Json j;
    if (!app::Json::parse(t, j)) return;
    for (const auto& [id, v] : j.o) {
        const int i = m_lib.find(id);
        if (i < 0 || v.type != app::Json::Number) continue;
        m_lib.parts[i].connSrc = (uint8_t)std::clamp((int)v.n, 0, 4);
    }
    m_connCountsDirty = true;
}
void BrickApp::saveConnLabels() {
    const std::wstring d = brickitDataDir();
    if (d.empty()) return;
    std::string j = "{";
    bool first = true;
    for (const Part& p : m_lib.parts) {
        if (p.connSrc == p.connAuto) continue;   // (only what was changed by hand)
        j += (first ? "\n  \"" : ",\n  \"") + p.name + "\": " + std::to_string(p.connSrc);
        first = false;
    }
    j += "\n}\n";
    writeText(d + L"\\conn_labels.json", j);
    m_connCountsDirty = true;
}

} // namespace brickit
