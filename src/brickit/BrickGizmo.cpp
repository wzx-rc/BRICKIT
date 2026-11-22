// BrickIt - move / rotate manipulators of the web builder: thin LEGO-coloured arrows (red X, yellow Y, blue Z) move the
// selection by studs / plates, rings turn it by 90 degrees around its pivot, the white centre sphere takes it in hand
// (free move). Drawn over everything (depthTest false), sized 0.07 x camera distance, white under the mouse.
#include "BrickApp.h"
#include <algorithm>
#include <cmath>

namespace brickit {

static const ImU32 kGz[3] = {IM_COL32(0xc9, 0x1a, 0x09, 242), IM_COL32(0xf2, 0xcd, 0x37, 242), IM_COL32(0x00, 0x55, 0xbf, 242)};
static float4 qNorm(const float4& q) { const float l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); return l > 0 ? float4(q.x / l, q.y / l, q.z / l, q.w / l) : float4(0, 0, 0, 1); }
static float3 axisOf(int a) { return float3(a == 0 ? 1.0f : 0.0f, a == 1 ? 1.0f : 0.0f, a == 2 ? 1.0f : 0.0f); }
// two unit vectors spanning the plane of a ring around axis a
static void ringBasis(int a, float3& u, float3& v) {
    if (a == 0) { u = float3(0, 1, 0); v = float3(0, 0, -1); }
    else if (a == 1) { u = float3(1, 0, 0); v = float3(0, 0, -1); }
    else { u = float3(1, 0, 0); v = float3(0, 1, 0); }
}
static float segDist(ImVec2 p, ImVec2 a, ImVec2 b) {
    float dx = b.x - a.x, dy = b.y - a.y, l = dx * dx + dy * dy;
    float t = l > 0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / l, 0.0f, 1.0f) : 0;
    float ex = a.x + dx * t - p.x, ey = a.y + dy * t - p.y;
    return std::sqrt(ex * ex + ey * ey);
}

bool BrickApp::gizmoVisible() const {
    // (+ Réglages › Gizmo de rotation permanent: the rings, light, on any selected brick in Sélection and Construire)
    return m_selected >= 0 && m_selected < (int)m_bricks.size() && !m_holding && !m_mechOn && !m_paintMode && !m_paintTool && !m_pipette &&
           (m_mode == 2 || m_mode == 3 || (m_permRotate && !m_ipadCreme && (m_mode == 0 || m_mode == 1)));   // (the kit: only when asked, its Tourner / Déplacer pebbles, Wzx 22/11/2026)
}
float3 BrickApp::gizmoPivot(int b) const { return m_build.worldVoxel(m_bricks[b], m_build.grabOf(m_bricks[b])); }
float BrickApp::gizmoScale(const float3& piv) const {
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    float3 eye = m_scene.camera.target - f * m_scene.camera.distance;
    return length(eye - piv) * 0.07f;
}

// Screen-space version of the web's invisible hit meshes (cylinders 0.11, tori 0.07, sphere 0.3); the sphere wins.
BrickApp::GizmoHandle BrickApp::pickGizmo(float sx, float sy) const {
    GizmoHandle none;
    if (!gizmoVisible()) return none;
    View v = view();
    const float3 piv = gizmoPivot(m_selected);
    const float k = gizmoScale(piv);
    float cx, cy;
    if (!v.project(piv, cx, cy)) return none;
    float ex, ey;
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    v.project(piv + r * k, ex, ey);
    const float px = std::max(1.0f, std::fabs(ex - cx));   // pixels per gizmo unit
    const ImVec2 m(sx, sy);
    // (no centre sphere any more, Wzx 14/11/2026: the plane handles, two axes at once, instead)
    float best = 1e30f;
    GizmoHandle bh;
    if (m_mode == 2)
        for (int a = 0; a < 3; ++a) {   // the square of the plane normal to a, between the two other axes
            const float3 e1 = axisOf((a + 1) % 3), e2 = axisOf((a + 2) % 3);
            ImVec2 q[4];
            bool ok = true;
            const float lo = 0.42f * k, hi = 0.68f * k;   // (further from the centre, Wzx 14/11/2026)
            ok &= v.project(piv + e1 * lo + e2 * lo, q[0].x, q[0].y);
            ok &= v.project(piv + e1 * hi + e2 * lo, q[1].x, q[1].y);
            ok &= v.project(piv + e1 * hi + e2 * hi, q[2].x, q[2].y);
            ok &= v.project(piv + e1 * lo + e2 * hi, q[3].x, q[3].y);
            if (!ok) continue;
            auto cr = [](ImVec2 a0, ImVec2 b0, ImVec2 p) { return (b0.x - a0.x) * (p.y - a0.y) - (b0.y - a0.y) * (p.x - a0.x); };
            const float c0 = cr(q[0], q[1], m), c1 = cr(q[1], q[2], m), c2 = cr(q[2], q[3], m), c3 = cr(q[3], q[0], m);
            if ((c0 >= 0 && c1 >= 0 && c2 >= 0 && c3 >= 0) || (c0 <= 0 && c1 <= 0 && c2 <= 0 && c3 <= 0)) return GizmoHandle{3, a};
        }
    for (int a = 0; a < 3; ++a) {
        if (m_mode == 2) {
            ImVec2 p0, p1;
            if (!v.project(piv, p0.x, p0.y) || !v.project(piv + axisOf(a) * (1.25f * k), p1.x, p1.y)) continue;
            float d = segDist(m, p0, p1);
            if (d < 0.11f * px + 4 && d < best) { best = d; bh = GizmoHandle{0, a}; }
        } else {
            float3 ru, rv;
            ringBasis(a, ru, rv);
            ImVec2 prev;
            bool hasPrev = false;
            for (int i = 0; i <= 48; ++i) {
                float t = i * 6.2831853f / 48;
                ImVec2 q;
                bool ok = v.project(piv + (ru * std::cos(t) + rv * std::sin(t)) * (0.78f * k), q.x, q.y);
                if (ok && hasPrev) {
                    float d = segDist(m, prev, q);
                    if (d < 0.07f * px + 6 && d < best) { best = d; bh = GizmoHandle{1, a}; }
                }
                prev = q;
                hasPrev = ok;
            }
        }
    }
    return bh;
}

// closest parameter along the axis line (through o, direction a) to the mouse ray
static float axisParam(const float3& ro, const float3& rd, const float3& o, const float3& a) {
    float3 w = o - ro;
    float b = dot(a, rd), d = dot(a, w), e = dot(rd, w);
    float den = 1 - b * b;
    return std::fabs(den) < 1e-6f ? 0 : (b * e - d) / den;
}
static bool rayPlane(const float3& ro, const float3& rd, const float3& n, const float3& p, float3& out) {
    float den = dot(n, rd);
    if (std::fabs(den) < 1e-6f) return false;
    float t = dot(n, p - ro) / den;
    if (t < 0) return false;
    out = ro + rd * t;
    return true;
}

void BrickApp::gizmoStart(const GizmoHandle& h, float sx, float sy) {
    pushUndo();
    GizmoDrag& g = m_gz;
    g = GizmoDrag();
    g.h = h;
    g.brick = m_selected;
    g.startPos = m_bricks[g.brick].pos;
    g.startRot = m_bricks[g.brick].rot;
    g.pivot = gizmoPivot(g.brick);
    g.others = m_multi;
    for (int o : m_multi) g.othersStart.push_back(m_bricks[o]);
    float3 ro, rd;
    view().ray(sx, sy, ro, rd);
    const float3 a = axisOf(h.axis);
    if (h.kind == 0) g.t0 = axisParam(ro, rd, g.pivot, a);
    else if (h.kind == 3) {
        float3 p;
        g.hasV0 = rayPlane(ro, rd, a, g.pivot, p);
        g.v0 = p;
    } else {
        float3 p;
        g.hasV0 = rayPlane(ro, rd, a, g.pivot, p);
        g.v0 = p - g.pivot;
    }
    m_gizmoDragging = true;
}

void BrickApp::gizmoDragTo(float sx, float sy) {
    GizmoDrag& g = m_gz;
    if (g.brick < 0 || g.brick >= (int)m_bricks.size()) { m_gizmoDragging = false; return; }
    Brick& b = m_bricks[g.brick];
    const float3 a = axisOf(g.h.axis);
    float3 ro, rd;
    view().ray(sx, sy, ro, rd);
    if (g.h.kind == 0) {
        float d = axisParam(ro, rd, g.pivot, a) - g.t0;
        if (m_build.gridSnap) { float step = g.h.axis == 1 ? kPlate : kPitch; d = std::round(d / step) * step; }
        b.pos = g.startPos + a * d;
        const float floorMin = m_build.aabbOf(b.part, b.rot, float3(0)).mn.y;
        if (g.h.axis == 1 && b.pos.y + floorMin < 0) b.pos.y = -floorMin;
        const float3 delta = b.pos - g.startPos;
        for (size_t i = 0; i < g.others.size(); ++i) m_bricks[g.others[i]].pos = g.othersStart[i].pos + delta;
    } else if (g.h.kind == 3) {   // two axes at once, in the plane of the « équerre »: by studs (and plates upwards)
        float3 p;
        if (!g.hasV0 || !rayPlane(ro, rd, a, g.pivot, p)) return;
        float3 d = p - g.v0;
        (&d.x)[g.h.axis] = 0;
        if (m_build.gridSnap) { d.x = std::round(d.x / kPitch) * kPitch; d.z = std::round(d.z / kPitch) * kPitch; d.y = std::round(d.y / kPlate) * kPlate; }
        b.pos = g.startPos + d;
        const float floorMin = m_build.aabbOf(b.part, b.rot, float3(0)).mn.y;
        if (b.pos.y + floorMin < 0) b.pos.y = -floorMin;
        const float3 delta = b.pos - g.startPos;
        for (size_t i = 0; i < g.others.size(); ++i) m_bricks[g.others[i]].pos = g.othersStart[i].pos + delta;
    } else {
        float3 p;
        if (!g.hasV0 || !rayPlane(ro, rd, a, g.pivot, p)) return;
        const float3 v = p - g.pivot;
        const float ang = std::atan2(dot(a, cross(g.v0, v)), dot(g.v0, v));
        // the step of Tourner (Wzx 14/11/2026: 15° by default, 45°, 90°, free); the quarter turns stay exact
        const float step = m_rotStep > 0 ? radians((float)m_rotStep) : 0.0f;
        float turn = ang;
        if (step > 0) {
            const int steps = (int)std::lround(ang / step);
            if (steps == g.steps) return;
            g.steps = steps;
            turn = steps * step;
        }
        g.angle = turn;
        const bool quarter = std::fabs(std::remainder(turn, kPi * 0.5f)) < 1e-4f;
        const float4 dq = qAxisAngle(a, turn);
        b.rot = quarter ? snapQuat(qMul(dq, g.startRot)) : qNorm(qMul(dq, g.startRot));
        b.pos = g.pivot + qRot(dq, g.startPos - g.pivot);
        for (size_t i = 0; i < g.others.size(); ++i) {
            Brick& o = m_bricks[g.others[i]];
            o.rot = quarter ? snapQuat(qMul(dq, g.othersStart[i].rot)) : qNorm(qMul(dq, g.othersStart[i].rot));
            o.pos = g.pivot + qRot(dq, g.othersStart[i].pos - g.pivot);
        }
        // (no settle bounce on release: Wzx 14/11/2026)
    }
    syncObjects();
}

void BrickApp::drawGizmo() {
    if (gizmoVisible() && m_gizmoDragging && m_gz.h.kind == 1) {   // the angle while it turns
        float cx0, cy0;
        if (view().project(gizmoPivot(m_selected), cx0, cy0)) {
            char t[32];
            snprintf(t, sizeof(t), "%+.0f°", m_gz.angle * 57.29578f);
            const ImVec2 ts = ImGui::CalcTextSize(t), tp(cx0 + 14, cy0 - 34);
            ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(tp.x - 6, tp.y - 3), ImVec2(tp.x + ts.x + 6, tp.y + ts.y + 3), IM_COL32(16, 17, 19, 200), 6);
            ImGui::GetForegroundDrawList()->AddText(tp, IM_COL32(255, 255, 255, 235), t);
        }
    }
    if (!gizmoVisible()) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    View v = view();
    const float3 piv = gizmoPivot(m_selected);
    const float k = gizmoScale(piv);
    float cx, cy;
    if (!v.project(piv, cx, cy)) return;
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    float ex, ey;
    v.project(piv + r * k, ex, ey);
    const float px = std::max(1.0f, std::fabs(ex - cx));
    const GizmoHandle hov = m_gizmoDragging ? m_gz.h : m_gizmoHover;
    const ImU32 white = IM_COL32(255, 255, 255, 242);
    if (m_mode == 2) {
        // arrows drawn back to front (farthest axis tip first)
        int order[3] = {0, 1, 2};
        std::sort(order, order + 3, [&](int a, int b) { return dot(axisOf(a), f) > dot(axisOf(b), f); });
        for (int a : order) {
            const ImU32 col = (hov.kind == 0 && hov.axis == a) ? white : kGz[a];
            ImVec2 p0, p1, tip;
            const float3 d = axisOf(a);
            if (!v.project(piv + d * (0.05f * k), p0.x, p0.y) || !v.project(piv + d * (1.03f * k), p1.x, p1.y) || !v.project(piv + d * (1.21f * k), tip.x, tip.y)) continue;
            dl->AddLine(p0, p1, col, std::max(1.5f, 0.036f * px));
            // cone (radius 0.06, length 0.18) seen from the camera: triangle from its base to the tip
            float3 side = cross(d, f);
            if (length(side) < 1e-4f) side = r;
            side = normalize(side) * (0.06f * k);
            ImVec2 b0, b1;
            v.project(piv + d * (1.03f * k) + side, b0.x, b0.y);
            v.project(piv + d * (1.03f * k) - side, b1.x, b1.y);
            dl->AddTriangleFilled(b0, b1, tip, col);
        }
        // the « équerres » (Wzx 14/11/2026): an L and a light square between two axes, in the colour of the third (its normal)
        for (int a = 0; a < 3; ++a) {
            const float3 e1 = axisOf((a + 1) % 3), e2 = axisOf((a + 2) % 3);
            const float lo = 0.42f * k, hi = 0.68f * k;   // (further from the centre, Wzx 14/11/2026)
            ImVec2 q[4], c1, c2;
            bool ok = true;
            ok &= v.project(piv + e1 * lo + e2 * lo, q[0].x, q[0].y);
            ok &= v.project(piv + e1 * hi + e2 * lo, q[1].x, q[1].y);
            ok &= v.project(piv + e1 * hi + e2 * hi, q[2].x, q[2].y);
            ok &= v.project(piv + e1 * lo + e2 * hi, q[3].x, q[3].y);
            ok &= v.project(piv + e1 * hi + e2 * hi, c1.x, c1.y);
            if (!ok) continue;
            const bool on = hov.kind == 3 && hov.axis == a;
            const ImU32 base = kGz[a] & 0x00ffffffu;
            dl->AddQuadFilled(q[0], q[1], q[2], q[3], on ? IM_COL32(255, 255, 255, 90) : (base | (60u << 24)));
            // the L: along the far edges of the square, from one axis to the other
            const ImVec2 L[3] = {q[1], q[2], q[3]};
            dl->AddPolyline(L, 3, on ? white : (base | (230u << 24)), 0, std::max(1.5f, 0.03f * px));
            (void)c2;
        }
        dl->AddCircleFilled(ImVec2(cx, cy), std::max(2.5f, 0.05f * px), IM_COL32(255, 255, 255, 220), 16);   // (the pivot, a small dot)
        if (m_gizmoDragging && (m_gz.h.kind == 0 || m_gz.h.kind == 3) && m_gz.brick >= 0 && m_gz.brick < (int)m_bricks.size()) {
            // how far it moved: in studs on the floor, in plates upwards
            const float3 dd = m_bricks[m_gz.brick].pos - m_gz.startPos;
            char t[96];
            snprintf(t, sizeof(t), "%+d · %+d tenons · %+d plaques", (int)std::lround(dd.x / kPitch), (int)std::lround(dd.z / kPitch), (int)std::lround(dd.y / kPlate));
            const ImVec2 tp(cx + 0.6f * px, cy - 0.9f * px);
            const ImVec2 ts = ImGui::CalcTextSize(t);
            dl->AddRectFilled(ImVec2(tp.x - 6, tp.y - 3), ImVec2(tp.x + ts.x + 6, tp.y + ts.y + 3), IM_COL32(16, 17, 19, 200), 6);
            dl->AddText(tp, IM_COL32(255, 255, 255, 235), t);
        }
    } else {
        for (int a = 0; a < 3; ++a) {
            // (Wzx 15/11/2026: in the style of the arrows of Déplacer, no ticks; the permanent rings: white, light)
            const bool hot = hov.kind == 1 && hov.axis == a, light = m_mode != 3;
            const ImU32 col = hot ? white : light ? IM_COL32(255, 255, 255, 75) : kGz[a];
            float3 ru, rv;
            ringBasis(a, ru, rv);
            std::vector<ImVec2> pts;
            for (int i = 0; i <= 64; ++i) {
                float t = i * 6.2831853f / 64;
                ImVec2 q;
                if (v.project(piv + (ru * std::cos(t) + rv * std::sin(t)) * (0.78f * k), q.x, q.y)) pts.push_back(q);
            }
            if (pts.size() > 1) dl->AddPolyline(pts.data(), (int)pts.size(), col, 0, light && !hot ? std::max(1.5f, 0.03f * px) : std::max(2.0f, 0.045f * px));
        }
    }
}

// --------------------------------------------------------------------------- pivot edit (web: aimPivot, refreshOverlays)

// Nearest voxel of the focus brick to the cursor, on screen.
void BrickApp::aimPivot(float sx, float sy) {
    const int b = focusBrick();
    if (b < 0 || b >= (int)m_bricks.size()) return;
    const Part& p = m_lib.parts[m_bricks[b].part];
    Affine xf = m_scene.objects[m_firstBrickObject + b].transform();
    View v = view();
    float best = 1e30f;
    int bi = -1;
    for (size_t i = 0; i < p.voxels.size(); ++i) {
        float x, y;
        if (!v.project(xf.point(p.voxels[i]), x, y)) continue;
        float d = (x - sx) * (x - sx) + (y - sy) * (y - sy);
        if (d < best) { best = d; bi = (int)i; }
    }
    if (bi < 0 || bi == m_bricks[b].grab) return;
    m_bricks[b].grab = bi;
    if (m_holding && m_held.brick == b) {
        m_held.grab = p.voxels[bi];
        m_held.cacheValid = false;
        updateHeld(sx, sy);
    }
}

// Spheres of radius 0.2 on every voxel (stud yellow, pin cyan, axle magenta, 90 %), the current pivot white and 1.5x,
// over everything (depthTest false).
void BrickApp::drawPivots() {
    if (!m_pivotEdit) return;
    const int b = focusBrick();
    if (b < 0 || b >= (int)m_bricks.size()) return;
    static const ImU32 col[3] = {IM_COL32(0xff, 0xc2, 0x1a, 230), IM_COL32(0x38, 0xd0, 0xe8, 230), IM_COL32(0xe0, 0x5a, 0xd7, 230)};
    const Part& p = m_lib.parts[m_bricks[b].part];
    Affine xf = m_scene.objects[m_firstBrickObject + b].transform();
    View v = view();
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    const float3 cur = m_holding && m_held.brick == b ? m_held.grab : m_build.grabOf(m_bricks[b]);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    int curIdx = -1;
    for (size_t i = 0; i < p.voxels.size(); ++i) {
        const float3& vx = p.voxels[i];
        if (std::fabs(vx.x - cur.x) < 1e-3f && std::fabs(vx.y - cur.y) < 1e-3f && std::fabs(vx.z - cur.z) < 1e-3f) { curIdx = (int)i; continue; }
        float3 w = xf.point(vx);
        float x, y, ex, ey;
        if (!v.project(w, x, y) || !v.project(w + r * 0.2f, ex, ey)) continue;
        uint8_t c = p.conn[i];
        int t = (c == CONN_PIN || c == CONN_PINHOLE) ? 1 : (c == CONN_AXLE || c == CONN_AXLEHOLE) ? 2 : 0;
        dl->AddCircleFilled(ImVec2(x, y), std::max(2.0f, std::fabs(ex - x)), col[t], 18);
    }
    if (curIdx >= 0) {   // drawn last, on top
        float3 w = xf.point(p.voxels[curIdx]);
        float x, y, ex, ey;
        if (v.project(w, x, y) && v.project(w + r * 0.3f, ex, ey)) dl->AddCircleFilled(ImVec2(x, y), std::max(3.0f, std::fabs(ex - x)), IM_COL32(255, 255, 255, 255), 24);
    }
}

} // namespace brickit
