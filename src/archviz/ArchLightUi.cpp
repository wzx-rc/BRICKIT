// ArchViz - the Light step: the fixtures in the plan (their symbols, picking, placing: a wall light goes against the
// nearest wall, facing the room), their inspector (power, colour temperature, on / off), the panel of the step (the
// sun of a clock or of the sky image, the exposure, every light on / off) and the automatic exposure.
#include "ArchApp.h"
#include "ArchLight.h"
#include "ArchMesh.h"
#include "ArchUi.h"
#include <algorithm>
#include <cmath>

namespace archviz {

using namespace vp3d;

static void uiTitle(const char* t, const char* sub, ImFont* bold) {
    if (bold) ImGui::PushFont(bold, 0.0f);
    ImGui::TextUnformatted(t);
    if (bold) ImGui::PopFont();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%s", sub);
    ImGui::Spacing();
}
static void uiSection(const char* t) { ImGui::Spacing(); ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "%s", t); }

int ArchApp::fixtureAt(float2 p) const {
    int best = -1;
    float bd = 0.35f * 0.35f;
    for (int i = 0; i < (int)m_b.fixtures.size(); ++i) {
        const Fixture& f = m_b.fixtures[i];
        if (!f.alive || f.level != m_level) continue;
        float d = len2(f.p - p);
        if (f.kind == kFixStrip) {   // the nearest point of the strip
            const float2 u(std::cos(f.yaw), std::sin(f.yaw));
            const float t = std::clamp(dot2(p - f.p, u), -f.length * 0.5f, f.length * 0.5f);
            d = len2(f.p + u * t - p);
        }
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

Fixture ArchApp::placeFixture(int kind, float2 p, bool free) const {
    Fixture f;
    f.kind = kind; f.level = m_level; f.lumens = fixtureDefaultLumens(kind);
    f.kelvin = kind == kFixStrip ? 2700.0f : 3000.0f;
    if (!free && m_snapGrid) p = float2(gridSnap(p.x), gridSnap(p.y));
    f.p = p;
    if (kind == kFixWall) {   // against the face of the nearest wall, towards the room (or the outside it stands in)
        float along = 0;
        const int w = m_b.wallAt(p, m_level, 0.8f, &along);
        if (w < 0) return f;
        const WallGeom& g = m_b.geom[w];
        const float side = dot2(p - g.a, g.n) >= 0 ? 1.0f : -1.0f;
        if (!free && m_snapGrid) along = gridSnap(along);
        f.p = g.a + g.u * along + g.n * (side * m_b.walls[w].thickness * 0.5f);
        const float2 d = g.n * side;
        f.yaw = std::atan2(d.y, d.x);
    } else if (kind == kFixStrip) {   // along the nearest wall when there is one
        float along = 0;
        const int w = m_b.wallAt(p, m_level, 0.8f, &along);
        if (w >= 0) { const float2 u = m_b.geom[w].u; f.yaw = std::atan2(u.y, u.x); }
    }
    return f;
}

void ArchApp::drawFixturePlan(ImDrawList* dl, const Fixture& f, ImU32 col) {
    const ImVec2 c = planToScreen(f.p);
    const float s = std::max(4.0f * m_ui, m_planScale * 0.14f), th = 1.4f * m_ui;
    const ImU32 fill = f.on ? IM_COL32(255, 196, 64, 200) : IM_COL32(255, 255, 255, 220);
    switch (f.kind) {
    case kFixCeiling: case kFixPendant:
        dl->AddCircleFilled(c, s, fill, 24);
        dl->AddCircle(c, s, col, 24, th);
        dl->AddLine(ImVec2(c.x - s * 0.7f, c.y - s * 0.7f), ImVec2(c.x + s * 0.7f, c.y + s * 0.7f), col, th);
        dl->AddLine(ImVec2(c.x - s * 0.7f, c.y + s * 0.7f), ImVec2(c.x + s * 0.7f, c.y - s * 0.7f), col, th);
        if (f.kind == kFixPendant) dl->AddCircle(c, s * 1.35f, col, 24, th);
        break;
    case kFixSpot:
        dl->AddCircleFilled(c, s * 0.6f, fill, 16);
        dl->AddCircle(c, s * 0.6f, col, 16, th);
        dl->AddCircleFilled(c, s * 0.18f, col, 8);
        break;
    case kFixWall: {   // a half disc on the wall, its flat side against it
        const float a = -f.yaw;   // (screen y is down)
        dl->PathArcTo(c, s, a - 1.5708f, a + 1.5708f, 16);
        dl->PathFillConvex(fill);
        dl->PathArcTo(c, s, a - 1.5708f, a + 1.5708f, 16);
        dl->PathStroke(col, ImDrawFlags_Closed, th);
        break;
    }
    case kFixStrip: {
        const float2 u(std::cos(f.yaw), std::sin(f.yaw));
        const ImVec2 a = planToScreen(f.p - u * (f.length * 0.5f)), b = planToScreen(f.p + u * (f.length * 0.5f));
        dl->AddLine(a, b, col, std::max(4.0f * m_ui, m_planScale * 0.04f) + 2 * m_ui);
        dl->AddLine(a, b, fill, std::max(2.0f * m_ui, m_planScale * 0.04f));
        break;
    }
    }
}

// ---- the inspector of the selected fixture
void ArchApp::lightInspector(float k, ImFont* bold) {
    Fixture f = m_b.fixtures[m_selFixture];
    uiTitle(fixtureName(f.kind), "Drag it to move it, Delete removes it", bold);
    bool ch = false;
    if (ImGui::Checkbox("On", &f.on)) ch = true;
    uiSection("TYPE");
    static const char* kn[] = {"Ceiling", "Pendant", "Wall", "Spot", "Strip"};
    int kind = f.kind;
    if (segButtons("fkind", kn, kFixKindCount, kind, k, bold) && kind != f.kind) {
        const Fixture p = placeFixture(kind, f.p, true);
        f.kind = kind; f.p = p.p; f.yaw = p.yaw; f.height = 0; ch = true;
    }
    uiSection("POWER");
    {
        static const char* pw[] = {"300 lm", "800 lm", "1500 lm", "3000 lm"};
        static const float pv[] = {300, 800, 1500, 3000};
        int cur = -1;
        for (int i = 0; i < 4; ++i) if (std::fabs(f.lumens - pv[i]) < 1) cur = i;
        if (segButtons("fpow", pw, 4, cur, k, bold)) { f.lumens = pv[cur]; ch = true; }
        ImGui::SetNextItemWidth(200 * k);
        float lm = f.lumens;
        if (ImGui::SliderFloat("##lm", &lm, 50.0f, 6000.0f, "%.0f lumens", ImGuiSliderFlags_Logarithmic)) { f.lumens = lm; ch = true; }
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "About %.0f W LED (%.0f W incandescent)", f.lumens / 100.0f, f.lumens / 12.0f);
    }
    uiSection("COLOUR TEMPERATURE");
    {
        static const char* kt[] = {"2700 K", "3000 K", "4000 K", "5000 K"};
        static const float kv[] = {2700, 3000, 4000, 5000};
        int cur = -1;
        for (int i = 0; i < 4; ++i) if (std::fabs(f.kelvin - kv[i]) < 1) cur = i;
        if (segButtons("fkel", kt, 4, cur, k, bold)) { f.kelvin = kv[cur]; ch = true; }
        ImGui::SetNextItemWidth(200 * k);
        float kel = f.kelvin;
        if (ImGui::SliderFloat("##kel", &kel, 1800.0f, 6500.0f, "%.0f K")) { f.kelvin = std::round(kel / 50.0f) * 50.0f; ch = true; }
        const float3 c = kelvinColor(f.kelvin);
        const float m = std::max(c.x, std::max(c.y, c.z));
        auto s = [&](float v) { return (int)std::clamp(std::pow(v / m, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f); };
        ImGui::SameLine();
        ImGui::ColorButton("##kc", ImVec4(s(c.x) / 255.0f, s(c.y) / 255.0f, s(c.z) / 255.0f, 1), ImGuiColorEditFlags_NoTooltip, ImVec2(20 * k, 20 * k));
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), f.kelvin < 2900 ? "Warm white" : f.kelvin < 3600 ? "Soft white" : f.kelvin < 4600 ? "Neutral white" : "Daylight");
    }
    uiSection("POSITION");
    {
        float h = fixtureHeight(m_b, f);
        if (fieldM(f.kind == kFixWall ? "Height" : f.kind == kFixPendant ? "Shade at" : "Height", &h, k, 0.1f, 20.0f)) { f.height = h; ch = true; }
        if (f.kind == kFixStrip) ch |= fieldM("Length", &f.length, k, 0.1f, 20.0f);
        if (f.kind == kFixStrip || f.kind == kFixWall) {
            if (ImGui::Button("-90\xc2\xb0")) { f.yaw -= kPi * 0.5f; ch = true; }
            ImGui::SameLine();
            if (ImGui::Button("+90\xc2\xb0")) { f.yaw += kPi * 0.5f; ch = true; }
        }
        if (f.height > 0.01f) { ImGui::SameLine(); if (ImGui::Button("Default height")) { f.height = 0; ch = true; } }
    }
    if (ch) { if (finishEdit()) pushUndo(); m_b.fixtures[m_selFixture] = f; edited(); }
}

// ---- the panel of the step without a selection: the sun, the exposure, the lights
void ArchApp::lightPanel(float k, ImFont* bold) {
    uiTitle("Light", "Place lights with the tools below", bold);
    uiSection("SUN");
    int mode = m_b.sunClock ? 1 : 0;
    static const char* sm[] = {"Sky image", "Clock"};
    if (segButtons("sunmode", sm, 2, mode, k, bold) && mode != (int)m_b.sunClock) { pushUndo(); m_b.sunClock = mode == 1; applySun(); touched(); }
    if (m_b.sunClock) {
        bool ch = false;
        ImGui::SetNextItemWidth(200 * k);
        float hr = m_b.sunHour;
        char hb[32];
        snprintf(hb, sizeof hb, "%02d:%02d solar time", (int)hr, (int)std::lround((hr - (int)hr) * 60) % 60);
        if (ImGui::SliderFloat("##hour", &hr, 4.0f, 22.0f, hb)) { m_b.sunHour = std::round(hr * 12.0f) / 12.0f; ch = true; }
        static const char* months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        ImGui::SetNextItemWidth(120 * k);
        int mo = std::clamp(m_b.sunMonth, 1, 12);
        if (ImGui::SliderInt("##month", &mo, 1, 12, months[mo - 1])) { m_b.sunMonth = mo; ch = true; }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(72 * k);
        int dy = std::clamp(m_b.sunDay, 1, 31);
        if (ImGui::SliderInt("##day", &dy, 1, 31, "%d")) { m_b.sunDay = dy; ch = true; }
        ImGui::SetNextItemWidth(120 * k);
        float lat = m_b.latitude;
        if (ImGui::SliderFloat("##lat", &lat, -60.0f, 70.0f, "Latitude %.1f\xc2\xb0")) { m_b.latitude = lat; ch = true; }
        static const char* cities[] = {"Paris", "Lyon", "Nice"};
        static const float cl[] = {48.86f, 45.76f, 43.70f};
        for (int i = 0; i < 3; ++i) { ImGui::SameLine(); if (ImGui::SmallButton(cities[i])) { m_b.latitude = cl[i]; ch = true; } }
        float el = 0;
        sunDirection(m_b.latitude, m_b.sunMonth, m_b.sunDay, m_b.sunHour, &el);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), el < 0 ? "The sun is down (night)" : "Sun %.0f\xc2\xb0 above the horizon", el * 180.0f / kPi);
        if (ch) { applySun(); m_r->resetAccumulation(); m_lastMove = ImGui::GetTime(); }
        if (ImGui::IsItemDeactivatedAfterEdit() || (ch && !ImGui::IsAnyItemActive())) touched();
    } else ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk3), "The sun of the sky, turned by Sun (top right).");
    uiSection("EXPOSURE");
    int am = m_autoExposure ? 0 : 1;
    static const char* em[] = {"Auto", "Manual"};
    if (segButtons("expo", em, 2, am, k, bold)) m_autoExposure = am == 0;
    ImGui::SetNextItemWidth(200 * k);
    if (m_autoExposure) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%+.1f EV (follows the view: interiors brighter)", m_ev);
    else ImGui::SliderFloat("##ev", &m_ev, -3.0f, 12.0f, "%+.1f EV");
    uiSection("LIGHTS");
    int n = 0, on = 0;
    for (const Fixture& f : m_b.fixtures) if (f.alive) { ++n; on += f.on; }
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kInk2), "%d light(s), %d on", n, on);
    if (n) {
        if (ImGui::Button("All on")) { pushUndo(); for (Fixture& f : m_b.fixtures) f.on = true; edited(); }
        ImGui::SameLine();
        if (ImGui::Button("All off")) { pushUndo(); for (Fixture& f : m_b.fixtures) f.on = false; edited(); }
    }
}

// ---- the sun: the clock moves the sun and turns the sky image so that its own sun stands where it is; at night the
// sky dims and the sun is off. Without the clock: the sky image and its sun (applyEnv).
void ArchApp::applySun() {
    applyEnv();
    m_night = false;
    if (!m_b.sunClock) return;
    float el = 0;
    const float3 d = sunDirection(m_b.latitude, m_b.sunMonth, m_b.sunDay, m_b.sunHour, &el);
    const float az = std::atan2(d.x, -d.z);   // from the north, clockwise
    Environment& e = m_scene.env;
    if (m_envHasSun[m_env]) e.hdriRotation = az - m_envSunAz[m_env];   // (its sun: phi = az)
    else e.hdriRotation = az;
    const float deg = el * 180.0f / kPi;
    // the light of the sky: about 5 EV under the day at sunset, 13 EV under at the end of the civil dusk (-6 degrees)
    const float skyEv = deg >= 20.0f ? 0.0f : deg >= 0.0f ? -5.0f * (1.0f - deg / 20.0f) : std::max(-14.0f, -5.0f + 1.4f * deg);
    e.hdriIntensity = std::exp2(skyEv);
    m_night = deg < -2.0f;
    e.hemiIntensity = 0.5f * e.hdriIntensity;                           // (the fill of the raster view with it)
    m_scene.sun.enabled = deg > -0.5f;
    m_scene.sun.direction = d.y < 0.02f ? normalize(float3(d.x, 0.02f, d.z)) : d;
    // low sun: weaker (more air on its way), warmer
    const float s = std::clamp(std::sin(std::max(el, 0.0f)), 0.0f, 1.0f);
    const float air = 1.0f / (s + 0.15f * std::pow(std::max(0.1f, deg) + 3.885f, -1.253f));
    const float tr = std::pow(0.72f, std::pow(air, 0.678f)) / std::pow(0.72f, 1.0f);
    m_scene.sun.intensity = 3.4f * std::clamp(tr, 0.0f, 1.0f) * std::clamp(deg / 3.0f + 0.2f, 0.0f, 1.0f);
    const float warm = std::clamp(1.0f - deg / 25.0f, 0.0f, 1.0f);
    m_scene.sun.color = kelvinColor(5600.0f - warm * 3000.0f) * 0.95f;
    if (m_r) m_r->resetAccumulation();
}

// ---- auto exposure: the median luminance of the view (read back from the image shown), brought up towards a mid
// grey, never darker than the default (0 EV: the exteriors stay as they were). A few steps a second, damped.
void ArchApp::autoExposure() {
    if (!m_autoExposure || m_r3.z <= m_r3.x) return;
    const bool shot = !m_opt.shotPath.empty();
    const double now = ImGui::GetTime();
    if (!shot && now - m_aeLast < 0.3) return;
    if (shot && m_frame % 2) return;
    m_aeLast = now;
    std::vector<uint8_t> img;
    uint32_t w = 0, h = 0;
    if (!m_r->readBackBuffer(img, w, h) || w == 0 || h == 0) return;
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float sx = w / std::max(1.0f, ds.x), sy = h / std::max(1.0f, ds.y);
    const float x0 = m_r3.x + (m_r3.z - m_r3.x) * 0.12f, x1 = m_r3.z - (m_r3.z - m_r3.x) * 0.12f;
    const float y0 = m_r3.y + (m_r3.w - m_r3.y) * 0.12f, y1 = m_r3.w - (m_r3.w - m_r3.y) * 0.12f;
    std::vector<float> lum;
    lum.reserve(64 * 40);
    for (int j = 0; j < 40; ++j)
        for (int i = 0; i < 64; ++i) {
            const int px = std::clamp((int)((x0 + (x1 - x0) * (i + 0.5f) / 64) * sx), 0, (int)w - 1);
            const int py = std::clamp((int)((y0 + (y1 - y0) * (j + 0.5f) / 40) * sy), 0, (int)h - 1);
            const uint8_t* p = &img[((size_t)py * w + px) * 4];
            const float r = srgbToLinear(p[0] / 255.0f), g = srgbToLinear(p[1] / 255.0f), b = srgbToLinear(p[2] / 255.0f);
            lum.push_back(0.2126f * r + 0.7152f * g + 0.0722f * b);
        }
    std::nth_element(lum.begin(), lum.begin() + lum.size() / 2, lum.end());
    const float med = std::max(lum[lum.size() / 2], 0.002f);
    // the darkness of the scene itself (EV under a lit day exterior), made up for in full inside a room (by day as by
    // night: a lit room); outside, a dusk or a night sky keeps part of its darkness (the mood, the lit windows)
    const float target = 0.14f;
    const float dark = std::max(0.0f, -std::log2(med / std::exp2(m_ev) / target));
    const float3 eye = m_scene.camera.eye();
    const float2 ep(eye.x, -eye.z);
    const Level& lv = m_b.levels[std::clamp(m_level, 0, (int)m_b.levels.size() - 1)];
    const bool inside = m_b.roomAt(ep, m_level) >= 0 && eye.y > lv.elevation && eye.y < lv.elevation + lv.height;
    const float skyDark = m_b.sunClock ? std::max(0.0f, -std::log2(std::max(1e-6f, m_scene.env.hdriIntensity))) : 0.0f;
    const float want = inside ? dark : std::max(0.0f, dark - skyDark * 0.4f);
    const float step = std::clamp(want - m_ev, -1.5f, 2.0f) * (shot ? 0.8f : 0.5f);
    const float ev = std::clamp(m_ev + step, 0.0f, 11.0f);
    if (std::fabs(ev - m_ev) > 0.03f) m_ev = ev;
}

} // namespace archviz
