// BrickIt - the user's area lights and cameras (Wzx 24/11/2026: « un panneau Lighting: créer une area light, un lister,
// visibilité, la sélectionner, ses paramètres, la scaler en W et H via des poignées, la déplacer, la rotate, intensité,
// couleur, une target on/off; un panneau Caméra: créer, renommer, supprimer, déplacer, rotate, un pop-up qui montre ce
// qu'il y a dans la caméra, entrer / sortir, locker, framing, la focale »).
// Lights: AreaLight of the scene after the stage's own (the rim), seen by every renderer (soft shadows by their size in
// Ray tracing). Cameras: an orbit camera each (the view's form), looked through or previewed offscreen. In the view:
// their icons (a click selects), and on the selected one its handles: arrows (move), rings (turn), the edges of a light
// (W, H), its target.
#include "BrickApp.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace brickit {

namespace {
const ImU32 kLcCol = IM_COL32(255, 214, 120, 230), kCamCol = IM_COL32(150, 205, 255, 230), kHot = IM_COL32(0xf7, 0xc5, 0x1e, 255);
const ImU32 kAx[3] = {IM_COL32(0xe0, 0x3a, 0x2a, 240), IM_COL32(0x6c, 0xc6, 0x3a, 240), IM_COL32(0x3a, 0x7c, 0xe8, 240)};
float3 axisOf(int a) { return float3(a == 0 ? 1.0f : 0.0f, a == 1 ? 1.0f : 0.0f, a == 2 ? 1.0f : 0.0f); }
float segDist(ImVec2 p, ImVec2 a, ImVec2 b) {
    const float dx = b.x - a.x, dy = b.y - a.y, l = dx * dx + dy * dy;
    const float t = l > 0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / l, 0.0f, 1.0f) : 0;
    const float ex = a.x + dx * t - p.x, ey = a.y + dy * t - p.y;
    return std::sqrt(ex * ex + ey * ey);
}
// parameter along the line p + t a of its point nearest to the ray o + s d
float lineParam(const float3& o, const float3& d, const float3& p, const float3& a) {
    const float3 w = p - o;
    const float b = dot(a, d), dd = dot(d, d), aa = dot(a, a), e = dot(a, w), f = dot(d, w);
    const float den = aa * dd - b * b;
    if (std::fabs(den) < 1e-8f) return 0;
    return (b * f - dd * e) / den;
}
bool rayPlane(const float3& o, const float3& d, const float3& p, const float3& n, float3& hit) {
    const float dn = dot(d, n);
    if (std::fabs(dn) < 1e-6f) return false;
    const float t = dot(p - o, n) / dn;
    if (t <= 0) return false;
    hit = o + d * t;
    return true;
}
float3 lin3(const float3& c) { return float3(srgbToLinear(c.x), srgbToLinear(c.y), srgbToLinear(c.z)); }
}

float3 BrickApp::UserLight::dir() const { return float3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)); }

// ------------------------------------------------------------------ create, delete, the scene

static void modelBounds(const std::vector<Brick>& bricks, const Library& lib, float3& mn, float3& mx) {
    mn = float3(1e30f);
    mx = float3(-1e30f);
    for (const Brick& b : bricks) {
        const Part& p = lib.parts[b.part];
        if (!p.loaded) continue;
        mn = minv(mn, b.pos + p.bmin);
        mx = maxv(mx, b.pos + p.bmax);
    }
    if (mn.x > mx.x) { mn = float3(-8, 0, -8); mx = float3(8, 8, 8); }
}

void BrickApp::addLight() {
    float3 mn, mx;
    modelBounds(m_bricks, m_lib, mn, mx);
    const float3 c = (mn + mx) * 0.5f;
    const float r = std::max(8.0f, length(mx - mn) * 0.5f);
    UserLight l;
    char nm[32];
    snprintf(nm, sizeof(nm), "Area light %d", (int)m_lights.size() + 1);
    l.name = nm;
    float3 r3, u3, f3;
    m_scene.camera.basis(r3, u3, f3);   // (above the model, a little to the left of the view)
    l.pos = c + normalize(float3(0, 1.0f, 0) - r3 * 0.8f - f3 * 0.5f) * (r * 1.7f);
    const float3 d = normalize(c - l.pos);
    l.yaw = std::atan2(d.x, d.z);
    l.pitch = std::asin(std::clamp(d.y, -1.0f, 1.0f));
    l.target = c;
    l.w = l.h = std::round(std::max(8.0f, r * 0.6f));
    const float dist = length(c - l.pos);
    l.intensity = std::round(3.0f * dist * dist / (l.w * l.h));   // (about the sun's light on the model)
    m_lights.push_back(l);
    m_selLight = (int)m_lights.size() - 1;
    m_selCam = -1;
    syncLights();
}

void BrickApp::addCamera() {
    UserCam c;
    char nm[32];
    snprintf(nm, sizeof(nm), "Camera %d", (int)m_cams.size() + 1);
    c.name = nm;
    c.cam = m_inCam >= 0 ? m_perspSave : m_scene.camera;   // (where the view is)
    c.cam.dof = false;
    m_cams.push_back(c);
    m_selCam = (int)m_cams.size() - 1;
    m_selLight = -1;
}

void BrickApp::deleteLight(int i) {
    if (i < 0 || i >= (int)m_lights.size()) return;
    m_lights.erase(m_lights.begin() + i);
    if (m_selLight == i) m_selLight = -1;
    else if (m_selLight > i) --m_selLight;
    syncLights();
}

void BrickApp::deleteCamera(int i) {
    if (i < 0 || i >= (int)m_cams.size()) return;
    if (m_inCam == i) exitCamera();
    m_cams.erase(m_cams.begin() + i);
    if (m_inCam > i) --m_inCam;
    if (m_selCam == i) m_selCam = -1;
    else if (m_selCam > i) --m_selCam;
}

void BrickApp::syncLights() {
    if (m_firstUserLight < 0 || m_firstUserLight > (int)m_scene.lights.size()) m_firstUserLight = (int)m_scene.lights.size();
    m_scene.lights.resize(m_firstUserLight);
    for (const UserLight& l : m_lights) {
        AreaLight a;
        a.name = l.name;
        a.position = l.pos;
        a.target = l.targetOn ? l.target : l.pos + l.dir();
        if (length(a.target - a.position) < 1e-3f) a.target = a.position + float3(0, -1, 0);
        a.width = std::max(0.2f, l.w);
        a.height = std::max(0.2f, l.h);
        a.color = lin3(l.color);
        a.intensity = std::max(0.0f, l.intensity);
        a.visibleToCamera = false;   // (as a studio light: it lights, it does not show)
        a.enabled = l.visible;
        m_scene.lights.push_back(a);
    }
    ++m_scene.objectsVersion;
    if (m_renderer) m_renderer->resetAccumulation();
}

// ------------------------------------------------------------------ the cameras

void BrickApp::enterCamera(int i) {
    if (i < 0 || i >= (int)m_cams.size()) return;
    if (m_inCam < 0) m_perspSave = m_scene.camera;
    m_inCam = i;
    Camera& v = m_scene.camera;
    const Camera& c = m_cams[i].cam;
    v.target = c.target;
    v.yaw = c.yaw;
    v.pitch = c.pitch;
    v.distance = c.distance;
    v.fovY = c.fovY;
    m_frameAnim = false;
    m_viewAnim = false;
    m_renderer->resetAccumulation();
}

void BrickApp::exitCamera() {
    if (m_inCam < 0) return;
    m_inCam = -1;
    Camera& v = m_scene.camera;
    v.target = m_perspSave.target;
    v.yaw = m_perspSave.yaw;
    v.pitch = m_perspSave.pitch;
    v.distance = m_perspSave.distance;
    v.fovY = m_perspSave.fovY;
    m_frameAnim = false;
    m_viewAnim = false;
    m_renderer->resetAccumulation();
}

// the model framed by the camera, its direction kept (its lens: the focal length)
void BrickApp::frameCamera(int i) {
    if (i < 0 || i >= (int)m_cams.size()) return;
    float3 mn, mx;
    modelBounds(m_bricks, m_lib, mn, mx);
    Camera& c = m_cams[i].cam;
    const float r = length(mx - mn) * 0.5f;
    c.target = (mn + mx) * 0.5f;
    c.distance = std::max(4.0f, r / std::sin(c.fovY * 0.5f) * 1.02f);
    c.focusDistance = c.distance;
    if (m_inCam == i) { m_scene.camera.target = c.target; m_scene.camera.distance = c.distance; m_renderer->resetAccumulation(); }
}

// looked through: free, the camera takes the view's moves; locked, the view is held to it whatever moved it
void BrickApp::camFollowView() {
    if (m_inCam < 0 || m_inCam >= (int)m_cams.size()) { m_inCam = -1; return; }
    Camera& v = m_scene.camera;
    Camera& c = m_cams[m_inCam].cam;
    if (m_cams[m_inCam].locked) {
        if (v.target.x != c.target.x || v.target.y != c.target.y || v.target.z != c.target.z || v.yaw != c.yaw || v.pitch != c.pitch || v.distance != c.distance || v.fovY != c.fovY) {
            v.target = c.target;
            v.yaw = c.yaw;
            v.pitch = c.pitch;
            v.distance = c.distance;
            v.fovY = c.fovY;
            m_renderer->resetAccumulation();
        }
    } else {
        c.target = v.target;
        c.yaw = v.yaw;
        c.pitch = v.pitch;
        c.distance = v.distance;
        c.fovY = v.fovY;
    }
}

// ------------------------------------------------------------------ handles in the view

float3 BrickApp::lcPivot(int kind, int i) const {
    if (kind == 0 && i >= 0 && i < (int)m_lights.size()) return m_lights[i].pos;
    if (kind == 1 && i >= 0 && i < (int)m_cams.size()) return m_cams[i].cam.eye();
    return float3(0);
}

// kinds: 0 arrow (axis), 1 ring (axis 1 yaw, 0 pitch), 4 edge of a light (0 +U, 1 -U, 2 +V, 3 -V), 5 target,
// 9 the icon of light axis, 10 the icon of camera axis
BrickApp::ObjDrag BrickApp::pickObj(float sx, float sy) const {
    ObjDrag none;
    const View v = view();
    const ImVec2 m(sx, sy);
    const int kind = m_selLight >= 0 ? 0 : (m_selCam >= 0 ? 1 : -1);
    const int sel = kind == 0 ? m_selLight : m_selCam;
    if (kind >= 0 && !(kind == 1 && m_inCam == sel)) {
        const float3 piv = lcPivot(kind, sel);
        const float k = gizmoScale(piv) * 1.15f;
        float cx, cy;
        if (v.project(piv, cx, cy)) {
            if (kind == 0) {
                const UserLight& l = m_lights[sel];
                if (l.targetOn) {
                    float tx, ty;
                    if (v.project(l.target, tx, ty) && std::hypot(sx - tx, sy - ty) < 10) { ObjDrag d; d.kind = 5; return d; }
                }
                AreaLight a;
                a.position = l.pos;
                a.target = l.targetOn ? l.target : l.pos + l.dir();
                float3 U, V, N;
                a.basis(U, V, N);
                const float3 hp[4] = {l.pos + U * (l.w * 0.5f), l.pos - U * (l.w * 0.5f), l.pos + V * (l.h * 0.5f), l.pos - V * (l.h * 0.5f)};
                for (int e = 0; e < 4; ++e) {
                    float hx, hy;
                    if (v.project(hp[e], hx, hy) && std::fabs(sx - hx) < 8 && std::fabs(sy - hy) < 8) { ObjDrag d; d.kind = 4; d.axis = e; return d; }
                }
            }
            float best = 8;
            ObjDrag bh;
            for (int a = 0; a < 3; ++a) {
                float ex, ey;
                if (!v.project(piv + axisOf(a) * k, ex, ey)) continue;
                const float dd = segDist(m, ImVec2(cx, cy), ImVec2(ex, ey));
                if (dd < best && std::hypot(sx - cx, sy - cy) > 6) { best = dd; bh.kind = 0; bh.axis = a; }
            }
            const bool rings = kind == 1 || !m_lights[sel].targetOn;
            if (rings) {
                float yaw = kind == 0 ? m_lights[sel].yaw : m_cams[sel].cam.yaw;
                const float3 right(std::cos(yaw), 0, -std::sin(yaw));
                for (int ax = 0; ax < 2; ++ax) {   // 1: around Y (yaw), 0: around the object's right (pitch)
                    float3 eu, ev;
                    if (ax == 1) { eu = float3(1, 0, 0); ev = float3(0, 0, 1); }
                    else { eu = float3(0, 1, 0); ev = normalize(cross(right, float3(0, 1, 0))); }
                    ImVec2 prev;
                    bool havePrev = false;
                    for (int j = 0; j <= 40; ++j) {
                        const float t = j * 6.2831853f / 40;
                        float px, py;
                        if (!v.project(piv + (eu * std::cos(t) + ev * std::sin(t)) * (k * 0.78f), px, py)) { havePrev = false; continue; }
                        if (havePrev) {
                            const float dd = segDist(m, prev, ImVec2(px, py));
                            if (dd < best) { best = dd; bh.kind = 1; bh.axis = ax; }
                        }
                        prev = ImVec2(px, py);
                        havePrev = true;
                    }
                }
            }
            if (bh.kind >= 0) return bh;
        }
    }
    // the icons
    float best = 13;
    for (int i = 0; i < (int)m_lights.size(); ++i) {
        float x, y;
        if (v.project(m_lights[i].pos, x, y) && std::hypot(sx - x, sy - y) < best) { best = std::hypot(sx - x, sy - y); none.kind = 9; none.axis = i; }
    }
    for (int i = 0; i < (int)m_cams.size(); ++i) {
        if (i == m_inCam) continue;
        float x, y;
        if (v.project(m_cams[i].cam.eye(), x, y) && std::hypot(sx - x, sy - y) < best) { best = std::hypot(sx - x, sy - y); none.kind = 10; none.axis = i; }
    }
    return none;
}

bool BrickApp::objInput(ImVec2 mp) {
    if (m_lights.empty() && m_cams.empty()) return false;
    const View v = view();
    if (m_odDragging) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { m_odDragging = false; return true; }
        const int kind = m_selLight >= 0 ? 0 : 1, sel = kind == 0 ? m_selLight : m_selCam;
        if (sel < 0) { m_odDragging = false; return true; }
        float3 o, d;
        v.ray(mp.x, mp.y, o, d);
        const ObjDrag& g = m_od;
        if (g.kind == 0) {   // along an axis
            const float3 ax = axisOf(g.axis);
            const float t = lineParam(o, d, g.p0, ax), t0 = g.w0;
            const float3 np = g.p0 + ax * (t - t0);
            if (kind == 0) { m_lights[sel].pos = np; syncLights(); }
            else { Camera& c = m_cams[sel].cam; c.target = g.q0 + (np - g.p0); }
        } else if (g.kind == 1) {   // a ring
            const float dx = mp.x - g.m0.x, dy = mp.y - g.m0.y;
            float yaw = g.yaw0, pitch = g.pitch0;
            if (g.axis == 1) yaw = g.yaw0 - dx * 0.01f;
            else pitch = std::clamp(g.pitch0 + (kind == 0 ? -dy : dy) * 0.01f, -1.55f, 1.55f);
            if (kind == 0) { m_lights[sel].yaw = yaw; m_lights[sel].pitch = pitch; syncLights(); }
            else {
                Camera& c = m_cams[sel].cam;
                const float3 eye = g.p0;
                c.yaw = yaw;
                c.pitch = pitch;
                float3 r, u, f;
                c.basis(r, u, f);
                c.target = eye + f * c.distance;
            }
        } else if (g.kind == 4 && kind == 0) {   // the edge of a light: its width or height
            UserLight& l = m_lights[sel];
            AreaLight a;
            a.position = l.pos;
            a.target = l.targetOn ? l.target : l.pos + l.dir();
            float3 U, V, N, hit;
            a.basis(U, V, N);
            if (rayPlane(o, d, l.pos, N, hit)) {
                if (g.axis < 2) l.w = std::clamp(2.0f * std::fabs(dot(hit - l.pos, U)), 0.5f, 400.0f);
                else l.h = std::clamp(2.0f * std::fabs(dot(hit - l.pos, V)), 0.5f, 400.0f);
                syncLights();
            }
        } else if (g.kind == 5 && kind == 0) {   // the target, in the plane facing the view
            float3 r, u, f, hit;
            m_scene.camera.basis(r, u, f);
            if (rayPlane(o, d, g.q0, f, hit)) { m_lights[sel].target = hit; syncLights(); }
        }
        return true;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_odHover = pickObj(mp.x, mp.y);
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return false;
    const ObjDrag h = pickObj(mp.x, mp.y);
    if (h.kind == 9) { m_selLight = h.axis; m_selCam = -1; m_lightPanel = true; return true; }
    if (h.kind == 10) { m_selCam = h.axis; m_selLight = -1; m_camPanel = true; return true; }
    if (h.kind < 0) { m_selLight = m_selCam = -1; return false; }   // (a click elsewhere: the light / camera let go)
    const int kind = m_selLight >= 0 ? 0 : 1, sel = kind == 0 ? m_selLight : m_selCam;
    m_od = h;
    m_od.m0 = mp;
    m_od.p0 = lcPivot(kind, sel);
    if (kind == 0) {
        m_od.yaw0 = m_lights[sel].yaw;
        m_od.pitch0 = m_lights[sel].pitch;
        if (h.kind == 5) m_od.p0 = m_od.q0 = m_lights[sel].target;
    } else {
        m_od.yaw0 = m_cams[sel].cam.yaw;
        m_od.pitch0 = m_cams[sel].cam.pitch;
        m_od.q0 = m_cams[sel].cam.target;
    }
    if (h.kind == 0) {
        float3 o, d;
        v.ray(mp.x, mp.y, o, d);
        m_od.w0 = lineParam(o, d, m_od.p0, axisOf(h.axis));   // (where the arrow was taken)
    }
    m_odDragging = true;
    return true;
}

void BrickApp::drawObjOverlay() {
    if (m_lights.empty() && m_cams.empty()) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const View v = view();
    const float s = m_ui;
    auto P = [&](const float3& p, ImVec2& q) { return v.project(p, q.x, q.y); };
    auto label = [&](ImVec2 at, const std::string& t, ImU32 col) {
        const ImVec2 ts = ImGui::CalcTextSize(t.c_str());
        dl->AddRectFilled(ImVec2(at.x - 4, at.y - 2), ImVec2(at.x + ts.x + 4, at.y + ts.y + 2), IM_COL32(12, 13, 15, 170), 4);
        dl->AddText(at, col, t.c_str());
    };
    // the lights: their rectangle, where they shine, their target
    for (int i = 0; i < (int)m_lights.size(); ++i) {
        const UserLight& l = m_lights[i];
        const bool sel = i == m_selLight;
        const ImU32 col = sel ? kHot : (l.visible ? kLcCol : IM_COL32(150, 150, 150, 140));
        AreaLight a;
        a.position = l.pos;
        a.target = l.targetOn ? l.target : l.pos + l.dir();
        float3 U, V, N;
        a.basis(U, V, N);
        const float3 c4[4] = {l.pos + U * (l.w * 0.5f) + V * (l.h * 0.5f), l.pos - U * (l.w * 0.5f) + V * (l.h * 0.5f), l.pos - U * (l.w * 0.5f) - V * (l.h * 0.5f), l.pos + U * (l.w * 0.5f) - V * (l.h * 0.5f)};
        ImVec2 q[4];
        bool ok = true;
        for (int k = 0; k < 4; ++k) ok &= P(c4[k], q[k]);
        if (ok) {
            dl->AddQuadFilled(q[0], q[1], q[2], q[3], (col & 0x00FFFFFF) | (sel ? 0x30000000 : 0x18000000));
            dl->AddQuad(q[0], q[1], q[2], q[3], col, sel ? 2.0f : 1.4f);
        }
        ImVec2 c, e;
        if (P(l.pos, c)) {
            const float3 tip = l.targetOn ? l.target : l.pos + N * std::max(4.0f, std::max(l.w, l.h) * 0.8f);
            if (P(tip, e)) {
                if (l.targetOn) {   // a dashed line to the target, its point
                    const float dx = e.x - c.x, dy = e.y - c.y, len = std::sqrt(dx * dx + dy * dy);
                    for (float t = 0; t < len; t += 10) dl->AddLine(ImVec2(c.x + dx * t / len, c.y + dy * t / len), ImVec2(c.x + dx * std::min(len, t + 5) / len, c.y + dy * std::min(len, t + 5) / len), col, 1.3f);
                    const bool th = sel && (m_odDragging ? m_od.kind == 5 : m_odHover.kind == 5);
                    dl->AddCircleFilled(e, th ? 7 : 5.5f, th ? IM_COL32(255, 255, 255, 255) : col, 16);
                    dl->AddCircle(e, th ? 7 : 5.5f, IM_COL32(0, 0, 0, 140), 16, 1.0f);
                } else dl->AddLine(c, e, col, 1.5f);
            }
            dl->AddCircleFilled(c, 6, col, 16);   // the icon: a small sun
            for (int k = 0; k < 8; ++k) {
                const float t = k * 0.785398f;
                dl->AddLine(ImVec2(c.x + std::cos(t) * 8, c.y + std::sin(t) * 8), ImVec2(c.x + std::cos(t) * 11, c.y + std::sin(t) * 11), col, 1.3f);
            }
            label(ImVec2(c.x + 14, c.y - 20), l.name, sel ? kHot : IM_COL32(235, 235, 238, 220));
        }
        if (sel && ok) {   // the edge handles (W, H)
            const float3 hp[4] = {l.pos + U * (l.w * 0.5f), l.pos - U * (l.w * 0.5f), l.pos + V * (l.h * 0.5f), l.pos - V * (l.h * 0.5f)};
            for (int k = 0; k < 4; ++k) {
                ImVec2 hq;
                if (!P(hp[k], hq)) continue;
                const bool hh = m_odDragging ? (m_od.kind == 4 && m_od.axis == k) : (m_odHover.kind == 4 && m_odHover.axis == k);
                const float r = hh ? 6.5f : 5.0f;
                dl->AddRectFilled(ImVec2(hq.x - r, hq.y - r), ImVec2(hq.x + r, hq.y + r), hh ? kHot : IM_COL32(255, 255, 255, 240), 1.5f);
                dl->AddRect(ImVec2(hq.x - r, hq.y - r), ImVec2(hq.x + r, hq.y + r), IM_COL32(0, 0, 0, 150), 1.5f, 0, 1.0f);
            }
        }
    }
    // the cameras: a pyramid of the frame (16:9), a small triangle on top
    for (int i = 0; i < (int)m_cams.size(); ++i) {
        if (i == m_inCam) continue;
        const Camera& c = m_cams[i].cam;
        const bool sel = i == m_selCam;
        const ImU32 col = sel ? kHot : kCamCol;
        float3 r, u, f;
        c.basis(r, u, f);
        const float3 eye = c.eye();
        const float L = gizmoScale(eye) * 1.6f, th = std::tan(c.fovY * 0.5f) * L, tw = th * 16.0f / 9.0f;
        const float3 cs[4] = {eye + f * L + r * tw + u * th, eye + f * L - r * tw + u * th, eye + f * L - r * tw - u * th, eye + f * L + r * tw - u * th};
        ImVec2 e, q[4];
        bool ok = P(eye, e);
        for (int k = 0; k < 4; ++k) ok &= P(cs[k], q[k]);
        if (!ok) continue;
        for (int k = 0; k < 4; ++k) dl->AddLine(e, q[k], col, sel ? 1.8f : 1.3f);
        dl->AddQuad(q[0], q[1], q[2], q[3], col, sel ? 2.0f : 1.4f);
        ImVec2 t0, t1, t2;
        if (P(eye + f * L + u * (th * 1.15f) + r * (tw * 0.3f), t0) && P(eye + f * L + u * (th * 1.15f) - r * (tw * 0.3f), t1) && P(eye + f * L + u * (th * 1.55f), t2))
            dl->AddTriangleFilled(t0, t1, t2, col);
        dl->AddCircleFilled(e, 4.5f, col, 12);
        label(ImVec2(e.x + 12, e.y + 6), m_cams[i].name + (m_cams[i].locked ? "  (locked)" : ""), sel ? kHot : IM_COL32(235, 235, 238, 220));
    }
    // the handles of the selected one
    const int kind = m_selLight >= 0 ? 0 : (m_selCam >= 0 ? 1 : -1), sel = kind == 0 ? m_selLight : m_selCam;
    if (kind < 0 || (kind == 1 && m_inCam == sel)) return;
    const float3 piv = lcPivot(kind, sel);
    const float k = gizmoScale(piv) * 1.15f;
    ImVec2 c;
    if (!P(piv, c)) return;
    const ObjDrag hov = m_odDragging ? m_od : m_odHover;
    const bool rings = kind == 1 || !m_lights[sel].targetOn;
    if (rings) {
        const float yaw = kind == 0 ? m_lights[sel].yaw : m_cams[sel].cam.yaw;
        const float3 right(std::cos(yaw), 0, -std::sin(yaw));
        for (int ax = 0; ax < 2; ++ax) {
            float3 eu, ev;
            if (ax == 1) { eu = float3(1, 0, 0); ev = float3(0, 0, 1); }
            else { eu = float3(0, 1, 0); ev = normalize(cross(right, float3(0, 1, 0))); }
            const bool hh = hov.kind == 1 && hov.axis == ax;
            std::vector<ImVec2> pts;
            for (int j = 0; j <= 48; ++j) {
                const float t = j * 6.2831853f / 48;
                ImVec2 q;
                if (P(piv + (eu * std::cos(t) + ev * std::sin(t)) * (k * 0.78f), q)) pts.push_back(q);
            }
            if (pts.size() > 2) dl->AddPolyline(pts.data(), (int)pts.size(), hh ? kHot : IM_COL32(255, 255, 255, ax == 1 ? 200 : 150), 0, hh ? 2.6f : 1.6f);
        }
    }
    for (int a = 0; a < 3; ++a) {
        ImVec2 e;
        if (!P(piv + axisOf(a) * k, e)) continue;
        const bool hh = hov.kind == 0 && hov.axis == a;
        dl->AddLine(c, e, hh ? kHot : kAx[a], hh ? 3.4f : 2.4f);
        const float dx = e.x - c.x, dy = e.y - c.y, len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
        const ImVec2 n(dx / len, dy / len), pn(-n.y, n.x);
        dl->AddTriangleFilled(ImVec2(e.x + n.x * 9, e.y + n.y * 9), ImVec2(e.x + pn.x * 5, e.y + pn.y * 5), ImVec2(e.x - pn.x * 5, e.y - pn.y * 5), hh ? kHot : kAx[a]);
    }
    (void)s;
}

// ------------------------------------------------------------------ the preview of a camera (Unreal's « camera preview »)

void BrickApp::pipRender() {
    const bool want = m_camPanel && m_selCam >= 0 && m_selCam < (int)m_cams.size() && m_selCam != m_inCam && m_renderer && m_frame > 4;
    if (!want) {
        if (m_pipTex) { m_gui->releaseTexture(m_pipTex); m_pipTex = 0; }
        m_pipKey.clear();
        return;
    }
    const Camera& c = m_cams[m_selCam].cam;
    char key[256];
    snprintf(key, sizeof(key), "%d %.4f %.4f %.4f %.4f %.4f %.4f %.4f %u %d %d", m_selCam, c.target.x, c.target.y, c.target.z, c.yaw, c.pitch, c.distance, c.fovY,
             m_scene.objectsVersion, (int)m_bricks.size(), m_quality);
    const double now = ImGui::GetTime();
    if (m_pipTex && m_pipKey == key) return;
    if (m_pipTex && now - m_pipT < 0.2) return;   // (a few times a second while it moves)
    m_pipT = now;
    m_pipKey = key;
    const int W = (int)(320 * m_ui), H = W * 9 / 16;
    const Camera save = m_scene.camera;
    m_scene.camera.target = c.target;
    m_scene.camera.yaw = c.yaw;
    m_scene.camera.pitch = c.pitch;
    m_scene.camera.distance = c.distance;
    m_scene.camera.fovY = c.fovY;
    m_scene.camera.dof = false;
    RenderSettings rs = m_rs;
    rs.raster = true;   // (the preview: the real time view, whatever the main one)
    rs.idPass = false;
    rs.msaaSamples = 4;
    ViewRect vp{0, 0, W, H};
    m_renderer->render(m_scene, rs, vp, -1, false);
    std::vector<uint8_t> img;
    uint32_t w = 0, h = 0;
    const bool ok = m_renderer->readViewport(img, w, h);
    m_scene.camera = save;
    m_renderer->resetAccumulation();
    if (!ok || !w || !h) return;
    if (m_pipTex) m_gui->releaseTexture(m_pipTex);
    m_pipTex = m_gui->createTexture(img.data(), w, h);
}

} // namespace brickit
