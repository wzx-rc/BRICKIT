// ArchViz - the move / rotate manipulators of BrickIt (BrickGizmo.cpp) on the objects of the library, in the 3D view:
// Move = three arrows in white, grey and anthracite with their outline (x, up, y), the squares of the planes between
// them and the white centre sphere (free on the floor, flush against the walls); Rotate = the white ring around the
// vertical. The handle under the mouse in the accent yellow. Drawn over everything, sized by the distance of the eye.
#include "ArchApp.h"
#include "ArchUi.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace archviz {

using namespace vp3d;

static const ImU32 kGzMove[3] = {IM_COL32(0xf4, 0xf4, 0xf6, 245), IM_COL32(0x9a, 0x9d, 0xa4, 245), IM_COL32(0x3a, 0x3c, 0x42, 245)};
static const ImU32 kGzOutline[3] = {IM_COL32(0, 0, 0, 120), IM_COL32(0, 0, 0, 120), IM_COL32(255, 255, 255, 110)};
static const ImU32 kGzHot = IM_COL32(0xf7, 0xc5, 0x1e, 255);

static float3 axisOf(int a) { return float3(a == 0 ? 1.0f : 0.0f, a == 1 ? 1.0f : 0.0f, a == 2 ? 1.0f : 0.0f); }
static float segDist(ImVec2 p, ImVec2 a, ImVec2 b) {
    const float dx = b.x - a.x, dy = b.y - a.y, l = dx * dx + dy * dy;
    const float t = l > 0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / l, 0.0f, 1.0f) : 0;
    const float ex = a.x + dx * t - p.x, ey = a.y + dy * t - p.y;
    return std::sqrt(ex * ex + ey * ey);
}
// closest parameter along the axis line (through o, direction a) to the mouse ray
static float axisParam(const float3& ro, const float3& rd, const float3& o, const float3& a) {
    const float3 w = o - ro;
    const float b = dot(a, rd), d = dot(a, w), e = dot(rd, w);
    const float den = 1 - b * b;
    return std::fabs(den) < 1e-6f ? 0 : (b * e - d) / den;
}
static bool rayPlane(const float3& ro, const float3& rd, const float3& n, const float3& p, float3& out) {
    const float den = dot(n, rd);
    if (std::fabs(den) < 1e-6f) return false;
    const float t = dot(n, p - ro) / den;
    if (t < 0) return false;
    out = ro + rd * t;
    return true;
}

bool ArchApp::projectW(const float3& p, ImVec2& out) const {
    const float w = m_r3.z - m_r3.x, h = m_r3.w - m_r3.y;
    if (w <= 0 || h <= 0) return false;
    float2 ndc;
    float depth;
    if (!m_scene.camera.project(p, w / h, ndc, &depth) || depth <= 0.02f) return false;
    out = ImVec2(m_r3.x + (ndc.x * 0.5f + 0.5f) * w, m_r3.y + (0.5f - ndc.y * 0.5f) * h);
    return true;
}

void ArchApp::rayW(ImVec2 s, float3& o, float3& d) const {
    const float w = std::max(1.0f, m_r3.z - m_r3.x), h = std::max(1.0f, m_r3.w - m_r3.y);
    m_scene.camera.ray((s.x - m_r3.x) / w * 2 - 1, 1 - (s.y - m_r3.y) / h * 2, w / h, o, d);
}

// The object under the mouse in the 3D view: the nearest of the boxes the ray enters.
int ArchApp::itemAtRay(ImVec2 s) const {
    if (m_r3.z <= m_r3.x) return -1;
    float3 ro, rd;
    rayW(s, ro, rd);
    int best = -1;
    float bestT = 1e30f;
    for (int i = 0; i < (int)m_b.items.size(); ++i) {
        const Item& it = m_b.items[i];
        if (it.level != m_level) continue;
        const float3 c = gizmoPivot(it);
        const float cy = std::cos(it.yaw), sy = std::sin(it.yaw);
        const float3 X(cy, 0, -sy), Z(sy, 0, cy);   // the axes of the object (Affine::trs)
        const float3 lo(dot(ro - c, X), ro.y - c.y, dot(ro - c, Z)), ld(dot(rd, X), rd.y, dot(rd, Z));
        const float3 mn(-it.size.x * 0.5f, 0, -it.size.y * 0.5f), mx(it.size.x * 0.5f, it.height, it.size.y * 0.5f);
        float t0 = 0, t1 = 1e30f;
        bool hit = true;
        for (int a = 0; a < 3 && hit; ++a) {
            const float o = (&lo.x)[a], d = (&ld.x)[a], lo2 = (&mn.x)[a], hi2 = (&mx.x)[a];
            if (std::fabs(d) < 1e-7f) { if (o < lo2 || o > hi2) hit = false; continue; }
            float ta = (lo2 - o) / d, tb = (hi2 - o) / d;
            if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta);
            t1 = std::min(t1, tb);
            if (t0 > t1) hit = false;
        }
        if (hit && t0 < bestT) { bestT = t0; best = i; }
    }
    return best;
}

bool ArchApp::gizmoVisible() const {
    return (m_tool == kMoveTool || m_tool == kRotateTool) && m_step == 1 && m_selKind == 4 && m_selItem >= 0 && m_selItem < (int)m_b.items.size() &&
           m_r3.z > m_r3.x && m_b.items[m_selItem].level == m_level;
}

float3 ArchApp::gizmoPivot(const Item& it) const {
    const float e = m_b.levels[std::clamp(it.level, 0, (int)m_b.levels.size() - 1)].elevation + 0.02f + it.elev;
    return world(it.p, e);
}

float ArchApp::gizmoScale(const float3& piv) const {
    return length(m_scene.camera.eye() - piv) * 0.062f * std::tan(m_scene.camera.fovY * 0.5f) / std::tan(radians(17.5f));
}

// Screen-space hit zones as BrickIt: the sphere wins, then the squares of the planes, then the nearest arrow / the ring.
ArchApp::GizmoHandle ArchApp::pickGizmo(ImVec2 m) const {
    GizmoHandle none;
    if (!gizmoVisible() || !inRect(m, m_r3)) return none;
    const float3 piv = gizmoPivot(m_b.items[m_selItem]);
    const float k = gizmoScale(piv);
    ImVec2 c, e;
    if (!projectW(piv, c)) return none;
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    if (!projectW(piv + r * k, e)) return none;
    const float px = std::max(1.0f, std::fabs(e.x - c.x));   // pixels per gizmo unit
    if (m_tool == kMoveTool) {
        if (std::hypot(m.x - c.x, m.y - c.y) < std::max(9.0f * m_ui, 0.13f * px) + 5) return GizmoHandle{2, -1};
        for (int a = 0; a < 3; ++a) {   // the square of the plane normal to a, between the two other axes
            const float3 e1 = axisOf((a + 1) % 3), e2 = axisOf((a + 2) % 3);
            ImVec2 q[4];
            const float lo = 0.42f * k, hi = 0.68f * k;
            if (!projectW(piv + e1 * lo + e2 * lo, q[0]) || !projectW(piv + e1 * hi + e2 * lo, q[1]) || !projectW(piv + e1 * hi + e2 * hi, q[2]) ||
                !projectW(piv + e1 * lo + e2 * hi, q[3])) continue;
            auto cr = [](ImVec2 a0, ImVec2 b0, ImVec2 p) { return (b0.x - a0.x) * (p.y - a0.y) - (b0.y - a0.y) * (p.x - a0.x); };
            const float c0 = cr(q[0], q[1], m), c1 = cr(q[1], q[2], m), c2 = cr(q[2], q[3], m), c3 = cr(q[3], q[0], m);
            if ((c0 >= 0 && c1 >= 0 && c2 >= 0 && c3 >= 0) || (c0 <= 0 && c1 <= 0 && c2 <= 0 && c3 <= 0)) return GizmoHandle{3, a};
        }
        float best = 1e30f;
        GizmoHandle bh;
        for (int a = 0; a < 3; ++a) {
            ImVec2 p1;
            if (!projectW(piv + axisOf(a) * (1.25f * k), p1)) continue;
            const float d = segDist(m, c, p1);
            if (d < 0.11f * px + 4 * m_ui && d < best) { best = d; bh = GizmoHandle{0, a}; }
        }
        return bh;
    }
    // the ring around the vertical
    ImVec2 prev;
    bool hasPrev = false;
    for (int i = 0; i <= 48; ++i) {
        const float t = i * 6.2831853f / 48;
        ImVec2 q;
        const bool ok = projectW(piv + (float3(1, 0, 0) * std::cos(t) + float3(0, 0, -1) * std::sin(t)) * (0.78f * k), q);
        if (ok && hasPrev && segDist(m, prev, q) < 0.07f * px + 6 * m_ui) return GizmoHandle{1, 1};
        prev = q;
        hasPrev = ok;
    }
    return none;
}

void ArchApp::gizmoStart(const GizmoHandle& h, ImVec2 s) {
    pushUndo();
    GizmoDrag& g = m_gz;
    g = GizmoDrag();
    g.h = h;
    g.item = m_selItem;
    g.start = m_b.items[m_selItem];
    g.pivot = gizmoPivot(g.start);
    float3 ro, rd;
    rayW(s, ro, rd);
    if (h.kind == 0) g.t0 = axisParam(ro, rd, g.pivot, axisOf(h.axis));
    else if (h.kind == 2) {   // free on the floor: the offset of the grab
        float2 p;
        g.hasV0 = rayToPlan(s, p, g.pivot.y);
        g.off = g.start.p - p;
    } else {
        float3 p;
        g.hasV0 = rayPlane(ro, rd, axisOf(h.axis), g.pivot, p);
        g.v0 = h.kind == 3 ? p : p - g.pivot;
    }
    m_gzDragging = true;
}

void ArchApp::gizmoDragTo(ImVec2 s) {
    GizmoDrag& g = m_gz;
    if (g.item < 0 || g.item >= (int)m_b.items.size()) { m_gzDragging = false; return; }
    Item& it = m_b.items[g.item];
    const bool free = ImGui::GetIO().KeyShift;
    float3 ro, rd;
    rayW(s, ro, rd);
    // a move in world axes: x = the x of the plan, y = up (above the floor), z = minus the y of the plan
    auto moved = [&](float3 d) {
        if (!free) d = float3(gridSnap(d.x), gridSnap(d.y), gridSnap(d.z));
        it.p = g.start.p + float2(d.x, -d.z);
        it.elev = std::max(0.0f, g.start.elev + d.y);
    };
    if (g.h.kind == 0) {
        moved(axisOf(g.h.axis) * (axisParam(ro, rd, g.pivot, axisOf(g.h.axis)) - g.t0));
    } else if (g.h.kind == 3) {
        float3 p;
        if (!g.hasV0 || !rayPlane(ro, rd, axisOf(g.h.axis), g.pivot, p)) return;
        float3 d = p - g.v0;
        (&d.x)[g.h.axis] = 0;
        if (length(d) > 200.0f) return;   // (a ray nearly in the plane)
        moved(d);
    } else if (g.h.kind == 2) {
        float2 p;
        if (!g.hasV0 || !rayToPlan(s, p, g.pivot.y)) return;
        it.p = snapItem(it, p + g.off, free);
    } else {
        float3 p;
        const float3 a(0, 1, 0);
        if (!g.hasV0 || !rayPlane(ro, rd, a, g.pivot, p)) return;
        const float3 v = p - g.pivot;
        float turn = std::atan2(dot(a, cross(g.v0, v)), dot(g.v0, v));
        const float step = (m_rotStep > 0 && !free) ? radians((float)m_rotStep) : 0.0f;
        if (step > 0) {
            const int steps = (int)std::lround(turn / step);
            if (steps == g.steps && g.angle == steps * step) return;
            g.steps = steps;
            turn = steps * step;
        }
        g.angle = turn;
        it.yaw = g.start.yaw + turn;
    }
    placeItem(m_scene, m_map, m_b, g.item);
    m_r->resetAccumulation();
    m_lastMove = ImGui::GetTime();
}

// The mouse of the manipulator, before the tools: true when it has it (over a handle, or dragging one).
bool ArchApp::gizmoInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (m_gzDragging) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) gizmoDragTo(io.MousePos);
        else {
            m_gzDragging = false;
            const Item& a = m_gz.start;
            const Item& b = m_b.items[std::clamp(m_gz.item, 0, (int)m_b.items.size() - 1)];
            if (a.p.x == b.p.x && a.p.y == b.p.y && a.yaw == b.yaw && a.elev == b.elev) { if (!m_undo.empty()) m_undo.pop_back(); }
            else touched();
        }
        return true;
    }
    m_gzHover = GizmoHandle();
    if (!gizmoVisible() || m_uiHover || io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle)) return false;
    m_gzHover = pickGizmo(io.MousePos);
    if (m_gzHover.kind < 0) return false;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) gizmoStart(m_gzHover, io.MousePos);
    return true;
}

void ArchApp::drawGizmo(ImDrawList* dl) {
    if (!gizmoVisible()) return;
    const Item& it = m_b.items[m_selItem];
    const float3 piv = gizmoPivot(it);
    const float k = gizmoScale(piv);
    ImVec2 c, e;
    if (!projectW(piv, c)) return;
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    if (!projectW(piv + r * k, e)) return;
    const float px = std::max(1.0f, std::fabs(e.x - c.x));
    const GizmoHandle hov = m_gzDragging ? m_gz.h : m_gzHover;
    auto label = [&](ImVec2 at, const char* t) {
        const ImVec2 ts = ImGui::CalcTextSize(t);
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        fg->AddRectFilled(ImVec2(at.x - 7 * m_ui, at.y - 4 * m_ui), ImVec2(at.x + ts.x + 7 * m_ui, at.y + ts.y + 4 * m_ui), IM_COL32(31, 39, 51, 225), 7 * m_ui);
        fg->AddText(at, kWhite, t);
    };
    if (m_tool == kMoveTool) {
        int order[3] = {0, 1, 2};   // back to front (the farthest tip first)
        std::sort(order, order + 3, [&](int a, int b) { return dot(axisOf(a), f) > dot(axisOf(b), f); });
        for (int a : order) {
            const ImU32 col = (hov.kind == 0 && hov.axis == a) ? kGzHot : kGzMove[a];
            ImVec2 p0, p1, tip, b0, b1;
            const float3 d = axisOf(a);
            if (!projectW(piv + d * (0.05f * k), p0) || !projectW(piv + d * (1.03f * k), p1) || !projectW(piv + d * (1.21f * k), tip)) continue;
            dl->AddLine(p0, p1, kGzOutline[a], std::max(1.5f, 0.036f * px) + 2.0f);
            dl->AddLine(p0, p1, col, std::max(1.5f, 0.036f * px));
            float3 side = cross(d, f);
            if (length(side) < 1e-4f) side = r;
            side = normalize(side) * (0.06f * k);
            if (!projectW(piv + d * (1.03f * k) + side, b0) || !projectW(piv + d * (1.03f * k) - side, b1)) continue;
            dl->AddTriangleFilled(b0, b1, tip, col);
            dl->AddTriangle(b0, b1, tip, kGzOutline[a], 1.0f);
        }
        for (int a = 0; a < 3; ++a) {   // the squares: an L and a light fill between two axes, in the grey of the third
            const float3 e1 = axisOf((a + 1) % 3), e2 = axisOf((a + 2) % 3);
            const float lo = 0.42f * k, hi = 0.68f * k;
            ImVec2 q[4];
            if (!projectW(piv + e1 * lo + e2 * lo, q[0]) || !projectW(piv + e1 * hi + e2 * lo, q[1]) || !projectW(piv + e1 * hi + e2 * hi, q[2]) ||
                !projectW(piv + e1 * lo + e2 * hi, q[3])) continue;
            const bool on = hov.kind == 3 && hov.axis == a;
            const ImU32 base = kGzMove[a] & 0x00ffffffu;
            dl->AddQuadFilled(q[0], q[1], q[2], q[3], on ? (kGzHot & 0x00ffffffu) | (90u << 24) : (base | (60u << 24)));
            const ImVec2 L[3] = {q[1], q[2], q[3]};
            dl->AddPolyline(L, 3, kGzOutline[a], 0, std::max(1.5f, 0.03f * px) + 2.0f);
            dl->AddPolyline(L, 3, on ? kGzHot : (base | (230u << 24)), 0, std::max(1.5f, 0.03f * px));
        }
        {   // the centre sphere: free on the floor
            const bool on = hov.kind == 2;
            const float rr = std::max(7.0f * m_ui, 0.11f * px);
            dl->AddCircleFilled(ImVec2(c.x, c.y + 1.5f), rr + 1.5f, IM_COL32(0, 0, 0, 90), 32);
            dl->AddCircleFilled(c, rr, on ? kGzHot : IM_COL32(255, 255, 255, 235), 32);
        }
        if (m_gzDragging && m_gz.h.kind != 1) {
            char t[96];
            snprintf(t, sizeof t, "%+.2f  \xc2\xb7  %+.2f m   up %+.2f", it.p.x - m_gz.start.p.x, it.p.y - m_gz.start.p.y, it.elev - m_gz.start.elev);
            label(ImVec2(c.x + 0.6f * px, c.y - 0.9f * px), t);
        }
    } else {
        const bool hot = hov.kind == 1;
        std::vector<ImVec2> pts;
        for (int i = 0; i <= 64; ++i) {
            const float t = i * 6.2831853f / 64;
            ImVec2 q;
            if (projectW(piv + (float3(1, 0, 0) * std::cos(t) + float3(0, 0, -1) * std::sin(t)) * (0.78f * k), q)) pts.push_back(q);
        }
        if (pts.size() > 1) {
            const float w = std::max(2.0f, 0.045f * px);
            dl->AddPolyline(pts.data(), (int)pts.size(), IM_COL32(0, 0, 0, 90), 0, w + 2.0f);
            dl->AddPolyline(pts.data(), (int)pts.size(), hot ? kGzHot : IM_COL32(255, 255, 255, 215), 0, w);
        }
        ImVec2 q;   // where the front of the object points: a tick on the ring
        if (projectW(piv + float3(std::sin(it.yaw), 0, std::cos(it.yaw)) * (0.78f * k), q)) {
            dl->AddCircleFilled(q, std::max(4.0f * m_ui, 0.06f * px) + 1.5f, IM_COL32(0, 0, 0, 90), 20);
            dl->AddCircleFilled(q, std::max(4.0f * m_ui, 0.06f * px), hot ? kGzHot : kWhite, 20);
        }
        if (m_gzDragging && m_gz.h.kind == 1) {
            char t[32];
            snprintf(t, sizeof t, "%+.0f\xc2\xb0", degrees(m_gz.angle));
            label(ImVec2(c.x + 14 * m_ui, c.y - 34 * m_ui), t);
        }
    }
}

} // namespace archviz
