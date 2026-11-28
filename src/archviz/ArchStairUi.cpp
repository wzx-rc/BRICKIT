// ArchViz - the stairs and the roof in the plan: the treads, the walk line with its arrow and the cut line of a stair;
// the ridge / hips (solid) and the edge of the overhang (dashed) of the roof over the top level; picking a stair.
#include "ArchApp.h"
#include "ArchRoof.h"
#include "ArchStair.h"
#include "ArchUi.h"
#include <algorithm>
#include <cmath>

namespace archviz {

using namespace vp3d;

int ArchApp::stairAt(float2 p) const {
    for (int i = (int)m_b.stairs.size() - 1; i >= 0; --i) {
        const Stair& s = m_b.stairs[i];
        if (!s.alive || s.level != m_level) continue;
        StairGeom g;
        buildStair(m_b, s, g);
        if (g.footprint.size() >= 3 && pointInPoly(p, g.footprint)) return i;
    }
    return -1;
}

void ArchApp::drawStairPlan(ImDrawList* dl, const Stair& s, ImU32 col, bool ghost) {
    StairGeom g;
    buildStair(m_b, s, g);
    const float th = (ghost ? 1.2f : 1.0f) * m_ui;
    if (!ghost && g.footprint.size() >= 3) {
        std::vector<ImVec2> q;
        for (const float2& p : g.footprint) q.push_back(planToScreen(p));
        dl->AddConvexPolyFilled(q.data(), (int)q.size(), IM_COL32(255, 255, 255, 150));
    }
    for (const auto& l : g.lines) dl->AddLine(planToScreen(l.first), planToScreen(l.second), alpha(col, ghost ? 0.8f : 0.75f), th);
    // the walk line from the foot (a dot) to the top (the arrow)
    if (g.walk.size() >= 2) {
        std::vector<ImVec2> w;
        for (const float2& p : g.walk) w.push_back(planToScreen(p));
        dl->AddPolyline(w.data(), (int)w.size(), col, 0, 1.3f * m_ui);
        dl->AddCircleFilled(w.front(), 3.0f * m_ui, col);
        const ImVec2 a = w[w.size() - 2], b = w.back();
        const float l = std::max(1e-3f, std::hypot(b.x - a.x, b.y - a.y));
        const ImVec2 d((b.x - a.x) / l, (b.y - a.y) / l), n(-d.y, d.x);
        const float hs = 7.0f * m_ui;
        dl->AddTriangleFilled(ImVec2(b.x + d.x * hs, b.y + d.y * hs), ImVec2(b.x - d.x * hs * 0.3f + n.x * hs * 0.6f, b.y - d.y * hs * 0.3f + n.y * hs * 0.6f),
                              ImVec2(b.x - d.x * hs * 0.3f - n.x * hs * 0.6f, b.y - d.y * hs * 0.3f - n.y * hs * 0.6f), col);
    }
    // the cut line (the plan cuts the flight at about 1 m): a zigzag across it
    const ImVec2 c0 = planToScreen(g.cut.first), c1 = planToScreen(g.cut.second);
    const ImVec2 m((c0.x + c1.x) * 0.5f, (c0.y + c1.y) * 0.5f);
    const float l = std::max(1e-3f, std::hypot(c1.x - c0.x, c1.y - c0.y));
    const ImVec2 n(-(c1.y - c0.y) / l * 5 * m_ui, (c1.x - c0.x) / l * 5 * m_ui);
    const ImVec2 z[5] = {c0, ImVec2(m.x - (c1.x - c0.x) * 0.06f + n.x, m.y - (c1.y - c0.y) * 0.06f + n.y),
                         ImVec2(m.x + (c1.x - c0.x) * 0.06f - n.x, m.y + (c1.y - c0.y) * 0.06f - n.y), c1, c1};
    dl->AddPolyline(z, 4, col, 0, 1.2f * m_ui);
}

void ArchApp::drawRoofPlan(ImDrawList* dl) {
    const int top = roofTopLevel(m_b);
    if (top < 0 || top != m_level) return;
    RoofGeom g;
    if (!buildRoof(m_b, top, g)) return;
    const ImU32 c = IM_COL32(120, 90, 60, 170);
    for (const auto& r : g.ridges) dl->AddLine(planToScreen(r.first), planToScreen(r.second), c, 1.2f * m_ui);
    for (size_t i = 0; i < g.eaves.size(); ++i) {   // dashed
        const ImVec2 a = planToScreen(g.eaves[i]), b = planToScreen(g.eaves[(i + 1) % g.eaves.size()]);
        const float l = std::hypot(b.x - a.x, b.y - a.y), dash = 7 * m_ui;
        for (float s = 0; s < l; s += dash * 2) {
            const float e = std::min(l, s + dash);
            dl->AddLine(ImVec2(a.x + (b.x - a.x) * s / l, a.y + (b.y - a.y) * s / l), ImVec2(a.x + (b.x - a.x) * e / l, a.y + (b.y - a.y) * e / l), c, 1.0f * m_ui);
        }
    }
}

} // namespace archviz
