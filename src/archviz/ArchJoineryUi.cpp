// ArchViz - the open / close handles of the joinery (Wzx 28/11/2026: « un léger HUD / flèche pour les ouvrir / fermer,
// une baie vitrée c'est une flèche blanche que l'on fait glisser »). The opening selected shows one handle per leaf,
// anchored on it in the 3D view and in the plan: a white arrow along the path the leaf would take (straight for the
// sliding panels, an arc around the hinge for the doors and casements, short for a tilt or an awning), the dark
// outline and a soft shadow, the accent under the mouse. Dragging it moves the leaf 1:1 (the point of its path nearest
// to the mouse), a click without moving opens / closes it, animated; a double-click on a leaf in the 3D view too.
// Only the transforms of the leaf objects change (placeJoinery): no geometry rebuilt.
#include "ArchApp.h"
#include "ArchJoinery.h"
#include "ArchUi.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace archviz {

using namespace vp3d;

namespace {
const ImU32 kHShadow = IM_COL32(0, 0, 0, 55), kHOutline = IM_COL32(25, 30, 40, 200);
float len2D(ImVec2 a, ImVec2 b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }
}

// where the leaf is (its open value) and the range of its handle: turn / slide 0..1, tilt 0..-1
static float leafValue(const Building& B, const SceneMap::JoineryLeaf& L) { return B.walls[L.wall].ops[L.op].leafOpen[L.leaf]; }

bool ArchApp::leafHandleScreen(int li, bool tilt, bool plan, float t, ImVec2& out) const {
    if (li < 0 || li >= (int)m_map.leaves.size()) return false;
    const SceneMap::JoineryLeaf& L = m_map.leaves[li];
    const float3 w = leafPoint(L, t, tilt ? L.anchor2 : L.anchor);
    if (plan) {
        if (m_rp.z <= m_rp.x) return false;
        out = planToScreen(float2(w.x, -w.z));
        return true;
    }
    return projectW(w, out);
}

// the handles shown: the leaves of the selected opening (turn handle when not tilted, tilt handle when not turned)
template <class F> static void forHandles(const ArchApp* app, const SceneMap& map, const Building& B, int selKind, int wall, int op, F f) {
    if (selKind != 2) return;
    for (int i = 0; i < (int)map.leaves.size(); ++i) {
        const SceneMap::JoineryLeaf& L = map.leaves[i];
        if (L.wall != wall || L.op != op) continue;
        if (L.wall >= (int)B.walls.size() || L.op >= (int)B.walls[L.wall].ops.size()) continue;
        const float v = leafValue(B, L);
        if (v >= -0.001f) f(i, false);
        if (L.hasTilt && v <= 0.001f) f(i, true);
    }
    (void)app;
}

ArchApp::LeafHandle ArchApp::pickLeafHandle(ImVec2 s) const {
    LeafHandle best;
    float bd = 18.0f * m_ui;
    const bool inPlan = inRect(s, m_rp), in3D = inRect(s, m_r3);
    if (!inPlan && !in3D) return best;
    forHandles(this, m_map, m_b, m_selKind, m_selWall, m_selOp, [&](int li, bool tilt) {
        ImVec2 p;
        const float v = leafValue(m_b, m_map.leaves[li]);
        if (!leafHandleScreen(li, tilt, inPlan, v, p)) return;
        const float d = len2D(p, s);
        if (d < bd) { bd = d; best.leaf = li; best.tilt = tilt; best.plan = inPlan; }
    });
    return best;
}

// The leaf under the mouse in the 3D view: the nearest box of a leaf (in its own frame) the ray enters.
int ArchApp::leafAtRay(ImVec2 s) const {
    if (m_r3.z <= m_r3.x) return -1;
    float3 ro, rd;
    rayW(s, ro, rd);
    int best = -1;
    float bt = 1e30f;
    for (int i = 0; i < (int)m_map.leaves.size(); ++i) {
        const SceneMap::JoineryLeaf& L = m_map.leaves[i];
        if (L.wall >= (int)m_b.walls.size() || L.op >= (int)m_b.walls[L.wall].ops.size()) continue;
        float3 pos;
        float4 rot;
        leafTransform(L, leafValue(m_b, L), pos, rot);
        const float4 inv(-rot.x, -rot.y, -rot.z, rot.w);
        const float3 o = L.pivot + qRot(inv, ro - pos), d = qRot(inv, rd);
        float t0 = 0, t1 = 1e30f;
        bool hit = true;
        for (int a = 0; a < 3 && hit; ++a) {
            const float lo = (&L.bmin.x)[a] - 0.01f, hi = (&L.bmax.x)[a] + 0.01f, oa = (&o.x)[a], da = (&d.x)[a];
            if (std::fabs(da) < 1e-9f) { if (oa < lo || oa > hi) hit = false; continue; }
            float ta = (lo - oa) / da, tb = (hi - oa) / da;
            if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb);
            if (t0 > t1) hit = false;
        }
        if (hit && t0 < bt) { bt = t0; best = i; }
    }
    return best;
}

// The leaf follows the mouse: the value whose handle point is nearest to it on screen (sampled, then refined).
void ArchApp::leafDragTo(ImVec2 s) {
    const LeafHandle& h = m_leafDrag.h;
    if (h.leaf < 0 || h.leaf >= (int)m_map.leaves.size()) return;
    const SceneMap::JoineryLeaf& L = m_map.leaves[h.leaf];
    const float lo = h.tilt ? -1.0f : 0.0f, hi = h.tilt ? 0.0f : 1.0f;
    const int N = 64;
    float bestT = leafValue(m_b, L), bestD = 1e30f;
    ImVec2 prev;
    bool hasPrev = false;
    for (int k = 0; k <= N; ++k) {
        const float t = lo + (hi - lo) * k / N;
        ImVec2 p;
        if (!leafHandleScreen(h.leaf, h.tilt, h.plan, t, p)) { hasPrev = false; continue; }
        if (hasPrev) {   // the nearest point of the segment prev..p
            const float dx = p.x - prev.x, dy = p.y - prev.y, l = dx * dx + dy * dy;
            const float u = l > 0 ? std::clamp(((s.x - prev.x) * dx + (s.y - prev.y) * dy) / l, 0.0f, 1.0f) : 0.0f;
            const float ex = prev.x + dx * u - s.x, ey = prev.y + dy * u - s.y, d = ex * ex + ey * ey;
            if (d < bestD) { bestD = d; bestT = lo + (hi - lo) * (k - 1 + u) / N; }
        }
        prev = p;
        hasPrev = true;
    }
    Opening& o = m_b.walls[L.wall].ops[L.op];
    o.leafOpen[L.leaf] = std::clamp(bestT, lo, hi);
    joinerySyncOpen(o);
    placeJoinery(m_scene, m_map, m_b, L.wall, L.op);
    m_r->resetAccumulation();
    m_lastMove = ImGui::GetTime();
}

bool ArchApp::joineryInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (m_leafDrag.active) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (len2D(io.MousePos, m_leafDrag.press) > 4.0f * m_ui) m_leafDrag.moved = true;
            if (m_leafDrag.moved) leafDragTo(io.MousePos);
        } else {
            m_leafDrag.active = false;
            const SceneMap::JoineryLeaf& L = m_map.leaves[std::clamp(m_leafDrag.h.leaf, 0, (int)m_map.leaves.size() - 1)];
            if (!m_leafDrag.moved) {   // a click: open / close it, animated
                if (!m_undo.empty()) m_undo.pop_back();
                toggleOpening(L.wall, L.op, L.leaf, m_leafDrag.h.tilt);
            } else if (leafValue(m_b, L) == m_leafDrag.start) {
                if (!m_undo.empty()) m_undo.pop_back();
            } else if (m_opt.shotPath.empty()) save(m_savePath);
        }
        return true;
    }
    m_leafHover = LeafHandle();
    if (m_uiHover || io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle)) return false;
    // a double-click on a leaf in the 3D view: opened / closed (whatever the tool)
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && inRect(io.MousePos, m_r3)) {
        const int li = leafAtRay(io.MousePos);
        if (li >= 0) {
            const SceneMap::JoineryLeaf& L = m_map.leaves[li];
            toggleOpening(L.wall, L.op, L.leaf, false);
            return true;
        }
    }
    m_leafHover = pickLeafHandle(io.MousePos);
    if (m_leafHover.leaf < 0) return false;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        pushUndo();
        m_leafDrag.active = true;
        m_leafDrag.moved = false;
        m_leafDrag.h = m_leafHover;
        m_leafDrag.press = io.MousePos;
        m_leafDrag.start = leafValue(m_b, m_map.leaves[m_leafHover.leaf]);
    }
    return true;
}

// The handles: an arrow along the path of the leaf from where it is, towards open (or back when it is open).
void ArchApp::drawLeafHandles(ImDrawList* dl, bool plan) {
    const float u = m_ui;
    forHandles(this, m_map, m_b, m_selKind, m_selWall, m_selOp, [&](int li, bool tilt) {
        const SceneMap::JoineryLeaf& L = m_map.leaves[li];
        const float v = leafValue(m_b, L);
        const float lo = tilt ? -1.0f : 0.0f, hi = tilt ? 0.0f : 1.0f;
        const float openEnd = tilt ? lo : hi, closedEnd = tilt ? hi : lo;
        const float frac = (v - closedEnd) / (openEnd - closedEnd);
        const float target = frac < 0.5f ? openEnd : closedEnd;
        ImVec2 p0;
        if (!leafHandleScreen(li, tilt, plan, v, p0)) return;
        std::vector<ImVec2> pts{p0};
        float acc = 0;
        const float want = (L.handleKind == 0 && !tilt ? 52.0f : 42.0f) * u;
        for (int k = 1; k <= 60 && acc < want; ++k) {
            const float t = v + (target - v) * k / 60.0f;
            ImVec2 p;
            if (!leafHandleScreen(li, tilt, plan, t, p)) break;
            acc += len2D(p, pts.back());
            pts.push_back(p);
        }
        if (acc < 14.0f * u) {   // the path is too short on screen from here (seen edge-on): a plain dot
            pts.resize(1);
        }
        const bool hot = (m_leafDrag.active && m_leafDrag.h.leaf == li && m_leafDrag.h.tilt == tilt) ||
                         (!m_leafDrag.active && m_leafHover.leaf == li && m_leafHover.tilt == tilt && m_leafHover.plan == plan);
        const ImU32 fill = hot ? kAccent : kWhite;
        auto path = [&](float th, ImU32 c, ImVec2 off) {
            for (size_t k = 1; k < pts.size(); ++k)
                dl->AddLine(ImVec2(pts[k - 1].x + off.x, pts[k - 1].y + off.y), ImVec2(pts[k].x + off.x, pts[k].y + off.y), c, th);
        };
        // the arrow head at the end of the path
        ImVec2 tip = pts.back(), dir(0, 0);
        if (pts.size() >= 2) {
            const ImVec2 b = pts[pts.size() >= 4 ? pts.size() - 4 : 0];
            const float l = std::max(1e-3f, len2D(tip, b));
            dir = ImVec2((tip.x - b.x) / l, (tip.y - b.y) / l);
        }
        const ImVec2 nrm(-dir.y, dir.x);
        const float hs = 9.0f * u;
        const ImVec2 h0(tip.x + dir.x * hs, tip.y + dir.y * hs), h1(tip.x - dir.x * hs * 0.4f + nrm.x * hs * 0.8f, tip.y - dir.y * hs * 0.4f + nrm.y * hs * 0.8f),
            h2(tip.x - dir.x * hs * 0.4f - nrm.x * hs * 0.8f, tip.y - dir.y * hs * 0.4f - nrm.y * hs * 0.8f);
        const ImVec2 sh(1.5f * u, 2.0f * u);
        path(7.0f * u, kHShadow, sh);
        path(5.5f * u, kHOutline, ImVec2(0, 0));
        if (pts.size() >= 2) {
            dl->AddTriangleFilled(ImVec2(h0.x + sh.x, h0.y + sh.y), ImVec2(h1.x + sh.x, h1.y + sh.y), ImVec2(h2.x + sh.x, h2.y + sh.y), kHShadow);
            dl->AddTriangleFilled(h0, h1, h2, kHOutline);
            const ImVec2 c((h0.x + h1.x + h2.x) / 3, (h0.y + h1.y + h2.y) / 3);
            auto shrink = [&](ImVec2 p) { return ImVec2(c.x + (p.x - c.x) * 0.62f, c.y + (p.y - c.y) * 0.62f); };
            dl->AddTriangleFilled(shrink(h0), shrink(h1), shrink(h2), fill);
        }
        path(3.0f * u, fill, ImVec2(0, 0));
        // the grab point
        dl->AddCircleFilled(ImVec2(p0.x + sh.x, p0.y + sh.y), 7.5f * u, kHShadow);
        dl->AddCircleFilled(p0, 7.0f * u, kHOutline);
        dl->AddCircleFilled(p0, 5.2f * u, fill);
        if (m_leafDrag.active && m_leafDrag.moved && m_leafDrag.h.leaf == li && m_leafDrag.h.tilt == tilt) {
            char b[24];
            snprintf(b, sizeof b, "%.0f %%", std::fabs(v) * 100.0f);
            const ImVec2 ts = ImGui::CalcTextSize(b);
            const ImVec2 at(p0.x + 12 * u, p0.y - 26 * u);
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            fg->AddRectFilled(ImVec2(at.x - 6 * u, at.y - 3 * u), ImVec2(at.x + ts.x + 6 * u, at.y + ts.y + 3 * u), IM_COL32(31, 39, 51, 220), 6 * u);
            fg->AddText(at, kWhite, b);
        }
    });
}

} // namespace archviz
