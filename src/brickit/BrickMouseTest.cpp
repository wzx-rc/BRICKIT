// BrickIt - scripted mouse tests (--mouse-test marquee | gizmo | ring | mech): real ImGui mouse events injected before
// each frame (ImGuiLayer::beforeNewFrame), so the input paths of the selection rectangle, the move / rotate gizmo and
// the drag of a moving part run exactly as with a mouse. The result goes to mouse_test.txt next to the exe.
#include "BrickApp.h"
#include <cmath>
#include <cstdio>

namespace brickit {

void BrickApp::mouseTestInput() {
    ImGuiIO& io = ImGui::GetIO();
    for (const MouseStep& s : m_mouseScript) {
        if (s.frame != m_frame) continue;
        io.AddMousePosEvent(s.x, s.y);
        if (s.btn == 0) io.AddMouseButtonEvent(0, true);
        if (s.btn == 1) io.AddMouseButtonEvent(0, false);
    }
}

// a press at a, a drag to b over 12 frames, a release
static void dragScript(std::vector<BrickApp::MouseStep>& out, int f0, ImVec2 a, ImVec2 b) {
    out.push_back({f0, a.x, a.y, -1});
    out.push_back({f0 + 1, a.x, a.y, 0});
    for (int k = 1; k <= 12; ++k) out.push_back({f0 + 1 + k, a.x + (b.x - a.x) * k / 12.0f, a.y + (b.y - a.y) * k / 12.0f, -1});
    out.push_back({f0 + 14, b.x, b.y, 1});
}

void BrickApp::mouseTestFrame() {
    const std::string& t = m_opt.mouseTest;
    if (t.empty()) return;
    const float sw = m_stage.z - m_stage.x, sh = m_stage.w - m_stage.y;
    View v = view();
    if (t == "browse") {   // the library browsed category after category (the thumbnails must keep coming, 24/11/2026)
        static const char* fams[] = {"basique", "formes", "technic", "mecanique", "vehicules", "batiment"};
        static int shown = 0;
        if (m_frame >= 10 && m_frame < 610 && m_frame % 8 == 0) {
            const int k = (m_frame - 10) / 8;
            m_stFam = fams[(k / 12) % 6];
            m_stSub = k % 12 - 1;
            m_clsGridScroll = 0;
            m_cardsScroll = 0;
        }
        if (m_frame > 10) shown = std::max(shown, (int)m_lazyThumbs.size());
        if (m_frame == 640) {
            FILE* f = nullptr;
            if (_wfopen_s(&f, (m_exeDir + L"\\mouse_test.txt").c_str(), L"w") == 0 && f) {
                fprintf(f, "browse: BrickIt textures %d, thumbnails in memory %zu (max seen %d), category %s / %d\n", m_gui->liveTextures(), m_lazyThumbs.size(), shown, m_stFam.c_str(), m_stSub);
                fclose(f);
            }
        }
        return;
    }
    if (t == "lcgizmo") {   // the gizmo of a light (with --lc-test 1): its three rings, its centre, then its target taken unselected
        static std::string rep;
        static float4 r0;
        static float3 p0, t0;
        auto quat = [](const float4& q) { char b[96]; snprintf(b, sizeof(b), "(%.3f %.3f %.3f %.3f)", q.x, q.y, q.z, q.w); return std::string(b); };
        if (m_lights.empty()) return;
        UserLight& l = m_lights[0];
        const int ringStart[3] = {20, 40, 60};
        for (int ra = 0; ra < 3; ++ra) {
            if (m_frame == ringStart[ra]) {
                m_selLight = 0; m_selCam = -1;
                float3 o, ax[3];
                lcAxes(0, 0, o, ax);
                const float k = gizmoScale(o) * 0.82f;
                const float3 eu = ax[(ra + 1) % 3], ev = ax[(ra + 2) % 3];
                ImVec2 best(0, 0), bestT(0, 0);
                float farD = -1;
                for (int j = 0; j < 24; ++j) {   // the point of the ring farthest from the centre on screen (no other handle there)
                    const float tt = j * 6.2831853f / 24;
                    float x, y, x2, y2, cx, cy;
                    if (!v.project(o + (eu * std::cos(tt) + ev * std::sin(tt)) * k, x, y) || !v.project(o + (eu * std::cos(tt + 0.25f) + ev * std::sin(tt + 0.25f)) * k, x2, y2) || !v.project(o, cx, cy)) continue;
                    const float dd = std::hypot(x - cx, y - cy);
                    if (dd > farD) { farD = dd; best = ImVec2(x, y); bestT = ImVec2(x2, y2); }
                }
                r0 = l.rot;
                m_mouseScript.clear();
                dragScript(m_mouseScript, m_frame + 1, best, ImVec2(best.x + (bestT.x - best.x) * 3, best.y + (bestT.y - best.y) * 3));
            }
            if (m_frame == ringStart[ra] + 17) {
                float3 o, ax[3];
                lcAxes(0, 0, o, ax);
                const float4 d = l.rot;
                const float dot4 = std::fabs(d.x * r0.x + d.y * r0.y + d.z * r0.z + d.w * r0.w);
                char b[200];
                snprintf(b, sizeof(b), "ring %d: turned %.1f deg %s -> %s\n", ra, 2 * std::acos(std::min(1.0f, dot4)) * 57.29578f, quat(r0).c_str(), quat(d).c_str());
                rep += b;
            }
        }
        if (m_frame == 80) {   // the centre: dragged 80 px to the right
            float cx, cy;
            v.project(l.pos, cx, cy);
            p0 = l.pos;
            m_mouseScript.clear();
            dragScript(m_mouseScript, 81, ImVec2(cx, cy), ImVec2(cx + 80, cy));
        }
        if (m_frame == 97) { char b[160]; snprintf(b, sizeof(b), "centre: moved %.2f cm\n", length(l.pos - p0)); rep += b; }
        if (m_frame == 100) {   // the target on, the light let go, its point taken and dropped on the model (its middle on screen)
            l.targetOn = true;
            float3 mn(1e30f), mx(-1e30f);
            for (const Brick& br : m_bricks) { const Part& pp = m_lib.parts[br.part]; if (pp.loaded) { mn = minv(mn, br.pos + pp.bmin); mx = maxv(mx, br.pos + pp.bmax); } }
            l.target = float3((mn.x + mx.x) * 0.5f, 0.0f, mx.z + 20.0f);   // (on the floor beside the model)
            syncLights();
            m_selLight = -1;
            t0 = l.target;
            float tx, ty, mx2, my2;
            v.project(l.target, tx, ty);
            v.project((mn + mx) * 0.5f, mx2, my2);
            m_mouseScript.clear();
            dragScript(m_mouseScript, 101, ImVec2(tx, ty), ImVec2(mx2, my2));
        }
        if (m_frame == 118) {
            float sx, sy;
            v.project(l.target, sx, sy);
            float3 hp;
            const int hit = pickBrick(sx, sy, &hp);
            char b[240];
            snprintf(b, sizeof(b), "target: selected light %d, moved %.2f cm, on a brick %s (%.2f cm from the brick surface)\n", m_selLight, length(l.target - t0), hit >= 0 ? "yes" : "no", hit >= 0 ? length(hp - l.target) : -1.0f);
            rep += b;
            FILE* f = nullptr;
            if (_wfopen_s(&f, (m_exeDir + L"\\mouse_test.txt").c_str(), L"w") == 0 && f) { fputs(rep.c_str(), f); fclose(f); }
        }
        return;
    }
    const bool swipeTest = t.rfind("swipe", 0) == 0;
    if (m_frame == (swipeTest ? 22 : 10)) {
        m_mouseScript.clear();
        if (t == "paint") {   // PEINDRE: the Pinceau held across the stage (the circle of the brush)
            m_paintMode = true; m_paintTool = true; m_pipette = false;
            const ImVec2 a(m_stage.x + sw * 0.2f, m_stage.y + sh * 0.55f), b(m_stage.x + sw * 0.8f, m_stage.y + sh * 0.5f);
            m_mouseScript.push_back({12, a.x, a.y, -1});
            m_mouseScript.push_back({13, a.x, a.y, 0});
            for (int k = 1; k <= 60; ++k) m_mouseScript.push_back({13 + k, a.x + (b.x - a.x) * k / 60.0f, a.y + (b.y - a.y) * k / 60.0f, -1});
        } else if (t == "pivot" || t == "wdrag" || t == "sphdrag") {   // W: the sphere chooses the pivot / a brick dragged comes in hand
            m_mode = 2;
            select(3);
            m_mtGrab0 = m_bricks[3].grab;
            float x, y;
            v.project(gizmoPivot(3), x, y);
            const Part& p = m_lib.parts[m_bricks[3].part];
            const Affine xf = m_scene.objects[m_firstBrickObject + 3].transform();
            int farI = 0;
            float best = -1;
            for (size_t i = 0; i < p.voxels.size(); ++i) { const float d = length(xf.point(p.voxels[i]) - gizmoPivot(3)); if (d > best) { best = d; farI = (int)i; } }
            m_mtGrabWant = farI;
            float tx, ty;
            v.project(xf.point(p.voxels[farI]), tx, ty);
            if (t == "pivot") {   // the sphere held still (long press), then a voxel clicked
                m_mouseScript.push_back({12, x, y, -1});
                m_mouseScript.push_back({13, x, y, 0});
                for (int k = 14; k < 120; ++k) m_mouseScript.push_back({k, x, y, -1});   // (held still: the choice opens; 0.35 s even at 300 fps)
                for (int k = 1; k <= 10; ++k) m_mouseScript.push_back({120 + k, x + (tx - x) * k / 10.0f, y + (ty - y) * k / 10.0f, -1});   // glided, still held
                m_mouseScript.push_back({134, tx, ty, 1});   // released on the voxel: set, and out
            } else if (t == "sphdrag") {   // the sphere dragged: the free move, the gizmo kept
                m_mouseScript.push_back({12, x, y, -1});
                m_mouseScript.push_back({13, x, y, 0});
                for (int k = 1; k <= 12; ++k) m_mouseScript.push_back({13 + k, x + k * 8.0f, y, -1});
            } else {   // a press on the brick away from the gizmo, a drag: it comes in hand
                const Box bx = m_build.aabbOf(m_bricks[3].part, m_bricks[3].rot, m_bricks[3].pos);
                float bx0, by0;
                v.project(float3(bx.mn.x + 0.1f, (bx.mn.y + bx.mx.y) * 0.5f, bx.mx.z - 0.1f), bx0, by0);
                m_mouseScript.push_back({12, bx0, by0, -1});
                m_mouseScript.push_back({13, bx0, by0, 0});
                for (int k = 1; k <= 10; ++k) m_mouseScript.push_back({13 + k, bx0 + k * 6.0f, by0, -1});
            }
        } else if (t == "hold") {   // a press held still on a brick (Studio): the ring of the colours
            m_mode = 1;
            select(-1);
            const Box bx = m_build.aabbOf(m_bricks[3].part, m_bricks[3].rot, m_bricks[3].pos);
            float x, y;
            v.project((bx.mn + bx.mx) * 0.5f, x, y);
            m_mouseScript.push_back({12, x, y, -1});
            m_mouseScript.push_back({13, x, y, 0});
            for (int k = 1; k <= 70; ++k) m_mouseScript.push_back({13 + k, x, y, -1});
        } else if (t == "marquee" || t == "marquee1") {
            m_mode = t == "marquee" ? 0 : 1;
            select(-1);
            dragScript(m_mouseScript, 12, ImVec2(m_stage.x + sw * 0.08f, m_stage.y + sh * 0.12f), ImVec2(m_stage.x + sw * 0.92f, m_stage.y + sh * 0.9f));
        } else if (t == "gizmo" || t == "ring") {
            m_mode = t == "gizmo" ? 2 : 3;
            select(3);
            m_mtBefore = m_bricks[3];
            const float3 piv = gizmoPivot(3);
            const float k = gizmoScale(piv);
            float ax, ay, bx, by;
            if (t == "gizmo") {   // the red X arrow, from near its tip, 3 studs further along it on screen
                v.project(piv + float3(0.9f * k, 0, 0), ax, ay);
                v.project(piv + float3(0.9f * k + 2.4f, 0, 0), bx, by);
                dragScript(m_mouseScript, 12, ImVec2(ax, ay), ImVec2(bx, by));
            } else {   // the red ring (around X): a quarter turn along the ring
                // from 45 degrees between +Y and -Z (a point of the X ring only: the Z ring also passes through +Y)
                auto ringPt = [&](float ang) { return piv + (float3(0, 1, 0) * std::cos(ang) + float3(0, 0, -1) * std::sin(ang)) * (0.78f * k); };
                v.project(ringPt(0.785398f), ax, ay);
                m_mouseScript.push_back({12, ax, ay, -1});
                m_mouseScript.push_back({13, ax, ay, 0});
                for (int s = 1; s <= 12; ++s) {
                    const float a = 0.785398f + s / 12.0f * 3.14159265f * 0.5f;
                    v.project(ringPt(a), bx, by);
                    m_mouseScript.push_back({13 + s, bx, by, -1});
                }
                m_mouseScript.push_back({26, bx, by, 1});
            }
        } else if (t == "swipediag" || t == "swipeghost") {
            // up and to the left, 56 degrees from straight up (within the 80 of the take: a brick, not a slide), or a
            // card pressed and moved 40 px up, held (the thumbnail following the pointer: capture)
            const ImVec4 tr = m_uiRects.size() > 1 ? m_uiRects[1] : ImVec4(0, sh, sw, sh);
            const float cy = m_mtCard.y > 0 ? m_mtCard.y : tr.w - 100 * m_ui, cx = m_mtCard.x > 0 ? m_mtCard.x : sw * 0.5f;   // (a card of the lower row)
            m_mtScroll0 = m_cardsScroll;
            if (t == "swipediag") { const float ty = m_stage.y + sh * 0.45f; dragScript(m_mouseScript, 24, ImVec2(cx, cy), ImVec2(cx - (cy - ty) * 1.5f, ty)); }
            else {
                m_mouseScript.push_back({24, cx, cy, -1});
                m_mouseScript.push_back({25, cx, cy, 0});
                for (int k = 1; k <= 10; ++k) m_mouseScript.push_back({25 + k, cx + 6.0f * k * m_ui / 10, cy - 40.0f * k * m_ui / 10, -1});
            }
        } else if (t == "swipe" || t == "swipeup") {
            // the middle of the cards: sideways 400 px (slide, inertia, no brick) or straight up out of the tray (brick)
            const ImVec4 tr = m_uiRects.size() > 1 ? m_uiRects[1] : ImVec4(0, sh, sw, sh);
            const float cy = m_mtCard.y > 0 ? m_mtCard.y : tr.w - 100 * m_ui, cx = m_mtCard.x > 0 ? m_mtCard.x : sw * 0.5f;   // (a card of the lower row)
            m_mtScroll0 = m_cardsScroll;
            if (t == "swipe") dragScript(m_mouseScript, 24, ImVec2(cx, cy), ImVec2(cx - 400 * m_ui, cy + 10 * m_ui));
            else dragScript(m_mouseScript, 24, ImVec2(cx, cy), ImVec2(cx + 10 * m_ui, m_stage.y + sh * 0.45f));
        } else if (t == "snap") {
            // a brick taken from the library (drag up out of the tray), then the mouse over the floor (frames 27-30)
            // and over the top of a placed brick (frames 31-33): the target and the snapped connector are recorded
            const ImVec4 tr = m_uiRects.size() > 1 ? m_uiRects[1] : ImVec4(0, sh, sw, sh);
            const float cy = m_mtCard.y > 0 ? m_mtCard.y : tr.w - 100 * m_ui, cx = m_mtCard.x > 0 ? m_mtCard.x : sw * 0.5f;   // (a card of the lower row)
            dragScript(m_mouseScript, 12, ImVec2(cx, cy), ImVec2(cx + 10 * m_ui, m_stage.y + sh * 0.45f));
            // the release in the stage must not drop it: the hand follows the mouse until a click
            const ImVec2 fl(m_stage.x + sw * 0.22f, m_stage.y + sh * 0.82f);
            for (int k = 0; k < 4; ++k) m_mouseScript.push_back({27 + k, fl.x, fl.y, -1});
            int top = 0;
            for (int i = 0; i < (int)m_bricks.size(); ++i) if (m_lib.parts[m_bricks[i].part].name == "3001") { top = i; break; }
            const Box bb = m_build.aabbOf(m_bricks[top].part, m_bricks[top].rot, m_bricks[top].pos);
            float ax = 0, ay = 0;
            v.project(float3((bb.mn.x + bb.mx.x) * 0.5f, bb.mx.y, (bb.mn.z + bb.mx.z) * 0.5f), ax, ay);
            m_mtBrick = top;
            for (int k = 0; k < 3; ++k) m_mouseScript.push_back({31 + k, ax, ay, -1});
        } else if (t == "mech") {
            mechEnter();
            m_mtQ = m_mm.q;
            // a visible point of a moving body (not a root): the first brick whose centre picks a brick of that body
            for (int i = 0; i < (int)m_bricks.size() && m_mouseScript.empty(); ++i) {
                bool root = false;
                for (int r : m_mm.roots) root |= m_mm.brickBody[i] == r;
                if (root) continue;
                Box bx = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
                float ax, ay;
                if (!v.project((bx.mn + bx.mx) * 0.5f, ax, ay)) continue;
                const int hit = pickBrick(ax, ay);
                if (hit < 0 || m_mm.brickBody[hit] != m_mm.brickBody[i]) continue;
                dragScript(m_mouseScript, 12, ImVec2(ax, ay), ImVec2(ax + 140 * m_ui, ay + 60 * m_ui));
                m_mtBrick = hit;
            }
        }
    }
    if (t == "snaplib" && m_frame == 12) {
        // Every part of the library with holes (pin or axle) and studs / anti-studs, taken in hand by three points
        // (the middle of a hole typed pin, a hole, a stud): over the studs on top of a 2 x 4 brick of the scene the
        // target must be a stud (the part stacks), over the floor the stud grid (x, z = 0.4 + 0.8k)
        FILE* f = nullptr;
        if (_wfopen_s(&f, (m_exeDir + L"\\snap_lib.txt").c_str(), L"w") != 0 || !f) return;
        View vv = view();
        // a free 2 x 4 brick for the test, in front of the scene (its studs uncovered)
        Box sb = m_build.aabbOf(m_bricks[0].part, m_bricks[0].rot, m_bricks[0].pos);
        for (const Brick& bk : m_bricks) { Box bb = m_build.aabbOf(bk.part, bk.rot, bk.pos); sb.mn = float3(std::min(sb.mn.x, bb.mn.x), std::min(sb.mn.y, bb.mn.y), std::min(sb.mn.z, bb.mn.z)); sb.mx = float3(std::max(sb.mx.x, bb.mx.x), std::max(sb.mx.y, bb.mx.y), std::max(sb.mx.z, bb.mx.z)); }
        Brick base;
        base.part = m_lib.find("3001");
        m_lib.get("3001");
        base.color = 0;
        base.rot = float4(0, 0, 0, 1);
        base.pos = float3(std::floor((sb.mn.x + sb.mx.x) * 0.5f / kPitch) * kPitch, 0, std::floor((sb.mx.z + 4.0f) / kPitch) * kPitch);
        m_bricks.push_back(base);
        const int top = (int)m_bricks.size() - 1;
        const Box tb = m_build.aabbOf(m_bricks[top].part, m_bricks[top].rot, m_bricks[top].pos);
        float tx = 0, ty = 0, fx = 0, fy = 0;
        vv.project(float3((tb.mn.x + tb.mx.x) * 0.5f, tb.mx.y, (tb.mn.z + tb.mx.z) * 0.5f), tx, ty);
        vv.project(float3(tb.mn.x - 9.0f, 0, tb.mx.z + 6.0f), fx, fy);   // the floor away from the bricks
        int parts = 0, cases = 0, okStud = 0, okFloor = 0;
        std::string fails;
        for (int pi = 0; pi < (int)m_lib.parts.size(); ++pi) {
            Part* pp = m_lib.get(m_lib.parts[pi].name);
            if (!pp || !pp->loaded) continue;
            bool hole = false, studs = false, anti = false;
            for (uint8_t c : pp->conn) { hole |= c == CONN_PINHOLE || c == CONN_AXLEHOLE; studs |= c == CONN_STUD || c == CONN_ANTISTUD; anti |= c == CONN_ANTISTUD; }
            if (!hole || !studs) continue;
            ++parts;
            Brick nb;
            nb.part = pi;
            nb.color = 0;
            nb.rot = float4(0, 0, 0, 1);
            nb.pos = float3(tb.mx.x + 30.0f, 0, 0);   // out of the way
            m_bricks.push_back(nb);
            const int bi = (int)m_bricks.size() - 1;
            const char* grabName[3] = {"pin (milieu du trou)", "trou", "tenon"};
            const uint8_t grabType[3] = {CONN_PIN, CONN_PINHOLE, CONN_STUD};
            for (int g = 0; g < 3; ++g) {
                int gi = -1;
                for (size_t i = 0; i < pp->voxels.size() && gi < 0; ++i)
                    if (pp->conn[i] == grabType[g] || (g == 1 && pp->conn[i] == CONN_AXLEHOLE)) gi = (int)i;
                if (gi < 0) continue;
                ++cases;
                Held H;
                m_build.beginHold(H, bi, false);
                H.grab = pp->voxels[gi];
                float3 hit((tb.mn.x + tb.mx.x) * 0.5f, tb.mx.y, (tb.mn.z + tb.mx.z) * 0.5f);   // over the top of the test brick
                m_build.computeTarget(H, vv, tx, ty, &hit);
                int ct = -1;
                for (size_t k = 0; k < H.vox.size(); ++k) if (length(H.vox[k] - H.snapVoxel) < 1e-4f) { ct = H.conn[k]; break; }
                const bool sOk = !anti || ct == CONN_STUD;   // (a part without anti-studs cannot sit on studs)
                m_build.computeTarget(H, vv, fx, fy, nullptr);
                const float gxr = (H.snapVoxel.x - 0.4f) / kPitch, gzr = (H.snapVoxel.z - 0.4f) / kPitch;
                const bool fOk = std::fabs(H.snapVoxel.y) < 1e-4f && std::fabs(gxr - std::round(gxr)) < 1e-3f && std::fabs(gzr - std::round(gzr)) < 1e-3f;
                okStud += sOk;
                okFloor += fOk;
                if ((!sOk || !fOk) && fails.size() < 4000) {
                    char b[200];
                    snprintf(b, sizeof(b), "  %s (%s), prise %s : tenon %s, grille %s\n", m_lib.parts[pi].name.c_str(), m_lib.parts[pi].label.c_str(), grabName[g], sOk ? "ok" : "RATE", fOk ? "ok" : "RATE");
                    fails += b;
                }
            }
            m_bricks.pop_back();
        }
        m_bricks.pop_back();   // (the test brick)
        fprintf(f, "snaplib: %d pieces a trous et tenons, %d prises : sur les tenons %d / %d, sur la grille %d / %d\n%s", parts, cases, okStud, cases, okFloor, cases, fails.c_str());
        fclose(f);
    }
    if (t == "snapscan" && m_frame == 12) {
        // Offline diagnostic of the snap (the scene opened with --open, its camera): every Technic brick 1 x 2 with
        // holes (32000) is taken in hand in turn (grab = its default pivot), the cursor swept over a 24 x 16 grid of the
        // view; per point: over a brick?, the connector type of the chosen voxel, floor or voxel, blocked by the safety
        FILE* f = nullptr;
        if (_wfopen_s(&f, (m_exeDir + L"\\snap_scan.txt").c_str(), L"w") != 0 || !f) return;
        View vv = view();
        const char* tn[] = {"none", "stud", "antistud", "pin", "pinhole", "axle", "axlehole"};
        for (int bi = 0; bi < (int)m_bricks.size(); ++bi) {
            if (m_lib.parts[m_bricks[bi].part].name != "32000") continue;
            Held H;
            m_build.beginHold(H, bi, false);
            const Part& hp = m_lib.parts[m_bricks[bi].part];
            int gi = -1;
            float gd = 1e9f;
            for (size_t i = 0; i < hp.voxels.size(); ++i) { float dd = length(hp.voxels[i] - H.grab); if (dd < gd) { gd = dd; gi = (int)i; } }
            fprintf(f, "brick %d (32000) at (%.2f %.2f %.2f) grab (%.2f %.2f %.2f) pivot type %s\n", bi, m_bricks[bi].pos.x, m_bricks[bi].pos.y, m_bricks[bi].pos.z,
                    H.grab.x, H.grab.y, H.grab.z, gi >= 0 && hp.conn[gi] <= 6 ? tn[hp.conn[gi]] : "?");
            int nFloor = 0, nVox = 0, nBlocked = 0, nSugg = 0, nOver = 0;
            std::map<std::string, int> kinds;
            for (int gy = 0; gy < 16; ++gy)
                for (int gx = 0; gx < 24; ++gx) {
                    const float sx = m_stage.x + (m_stage.z - m_stage.x) * (gx + 0.5f) / 24, sy = m_stage.y + (m_stage.w - m_stage.y) * (gy + 0.5f) / 16;
                    float3 hit;
                    const bool over = pickBrick(sx, sy, &hit, bi) >= 0;
                    m_build.computeTarget(H, vv, sx, sy, over ? &hit : nullptr);
                    nOver += over;
                    // the chosen voxel: one of the cache (snap) or the floor
                    int ct = -1;
                    for (size_t k = 0; k < H.vox.size(); ++k) if (length(H.vox[k] - H.snapVoxel) < 1e-4f) { ct = H.conn[k]; break; }
                    if (ct < 0) ++nFloor; else { ++nVox; kinds[std::string(over ? "over " : "near ") + (ct <= 6 ? tn[ct] : "?")]++; }
                    nBlocked += H.blocked;
                    nSugg += H.suggested;
                }
            fprintf(f, "  384 points: over a brick %d, floor %d, voxel %d, blocked %d, other pivot %d\n", nOver, nFloor, nVox, nBlocked, nSugg);
            for (auto& kv : kinds) fprintf(f, "    %s: %d\n", kv.first.c_str(), kv.second);
        }
        fclose(f);
    }
    if (m_frame == 38) m_mtScrollRel = m_cardsScroll;   // the release frame (swipes from frame 24)
    if (t == "snap" && m_frame == 26 && !m_holding) {   // the drag missed a card (tray laid out otherwise): a brick in hand
        const ImVec2 mp = ImGui::GetIO().MousePos;
        spawn(m_lib.find("3001"), mp.x, mp.y);
        m_mtDiag += " (drag missed a card: spawned)";
    }
    if (t == "snap" && m_frame == 30) {
        char b[256];
        snprintf(b, sizeof(b), "floor: holding=%d hasTarget=%d target=(%.2f %.2f %.2f) snap=(%.2f %.2f %.2f) blocked=%d", m_holding, m_held.hasTarget,
                 m_held.target.x, m_held.target.y, m_held.target.z, m_held.snapVoxel.x, m_held.snapVoxel.y, m_held.snapVoxel.z, m_held.blocked);
        m_mtDiag = std::string(b) + "\n  mid-drag: " + m_mtDiag;
    }
    if (m_frame == 16) {   // state in the middle of the drag (diagnostic)
        m_mtDiag = "dragging=" + std::to_string(m_gizmoDragging) + " kind=" + std::to_string(m_gz.h.kind) + " axis=" + std::to_string(m_gz.h.axis) +
                   " steps=" + std::to_string(m_gz.steps) + " hasV0=" + std::to_string(m_gz.hasV0) + " mechDragging=" + std::to_string(m_mechDragging) +
                   " mouse=" + std::to_string((int)ImGui::GetIO().MousePos.x) + "," + std::to_string((int)ImGui::GetIO().MousePos.y) + " down=" + std::to_string(ImGui::IsMouseDown(0)) +
                   " overUi=" + std::to_string(m_overUi) + " swipe=" + std::to_string(m_swipe.id) + "/" + std::to_string(m_swipe.dir) + " libPress=" + std::to_string(m_libPress) + " tray=" + std::to_string((int)(m_uiRects.size() > 1 ? m_uiRects[1].y : -1)) + "-" + std::to_string((int)(m_uiRects.size() > 1 ? m_uiRects[1].w : -1)) + " cardsMax?scroll=" + std::to_string(m_cardsScroll);
    }
    if (m_frame == (t.rfind("swipe", 0) == 0 ? 46 : t == "hold" ? 80 : t == "pivot" ? 160 : 34)) {   // (the swipes start at frame 24: the cards answer from ~16)
        FILE* f = nullptr;
        if (_wfopen_s(&f, (m_exeDir + L"\\mouse_test.txt").c_str(), L"w") != 0 || !f) return;
        if (t == "pivot") fprintf(f, "pivot: grab %d -> %d (expected %d), choosing %d (expected 0)\n", m_mtGrab0, m_bricks[3].grab, m_mtGrabWant, m_pivotPick ? 1 : 0);
        if (t == "sphdrag") fprintf(f, "sphdrag: brick in hand %d (expected 1), gizmo shown %d (expected 1), choosing %d (expected 0)\n", m_holding ? 1 : 0, gizmoVisible() ? 1 : 0, m_pivotPick ? 1 : 0);
        if (t == "wdrag") fprintf(f, "wdrag: brick in hand %d (expected 1), marquee %d (expected 0)\n", m_holding ? 1 : 0, m_marquee ? 1 : 0);
        if (t == "hold") fprintf(f, "hold: ring of colours open=%d (expected 1), marking menu=%d (expected 0)\n", m_crRing, m_stMenu);
        if (t == "marquee" || t == "marquee1") fprintf(f, "marquee: selected=%zu of %zu bricks (expected all)\n", selection().size(), m_bricks.size());
        else if (t == "swipe") fprintf(f, "swipe: cards scroll %.0f -> %.0f at release, %.0f now (inertia: further), velocity %.0f, brick in hand=%d (expected 0)\n", m_mtScroll0, m_mtScrollRel, m_cardsScroll, m_cardsVel, m_holding);
        else if (t == "swipediag") fprintf(f, "swipe diagonal (56 deg from up): brick in hand=%d (expected 1), cards scroll %.0f -> %.0f (expected unchanged)\n", m_holding, m_mtScroll0, m_cardsScroll);
        else if (t == "swipeup") fprintf(f, "swipe up: brick in hand=%d (expected 1), cards scroll %.0f -> %.0f (expected unchanged)\n", m_holding, m_mtScroll0, m_cardsScroll);
        else if (t == "gizmo") {
            const float3 d = m_bricks[3].pos - m_mtBefore.pos;
            fprintf(f, "gizmo move X: delta=(%.3f, %.3f, %.3f) cm, studs=%.2f (expected a whole number > 0 along X only)\n", d.x, d.y, d.z, d.x / kPitch);
        } else if (t == "ring") {
            const float4 a = m_mtBefore.rot, b = m_bricks[3].rot;
            const float c = std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
            fprintf(f, "gizmo ring X: angle=%.1f deg (expected 90)\n", 2 * std::acos(std::min(1.0f, c)) * 57.2958f);
        } else if (t == "mech") {
            double mx = 0;
            for (size_t i = 0; i < m_mm.q.size() && i < m_mtQ.size(); ++i) mx = std::max(mx, std::fabs(m_mm.q[i] - m_mtQ[i]));
            fprintf(f, "mech drag: brick=%d joints=%zu max joint change=%.1f deg (expected > 0)\n", m_mtBrick, m_mm.q.size(), mx * 57.2958);
        } else if (t == "snap") {
            const Box bb = m_build.aabbOf(m_bricks[m_mtBrick].part, m_bricks[m_mtBrick].rot, m_bricks[m_mtBrick].pos);
            fprintf(f, "snap: brick: holding=%d target=(%.2f %.2f %.2f) snap=(%.2f %.2f %.2f) blocked=%d, top of brick %d at y=%.2f\n"
                       "  (expected: floor y 0 on the stud grid x,z = 0.4 + 0.8k; brick: snap y = its top)\n",
                    m_holding, m_held.target.x, m_held.target.y, m_held.target.z, m_held.snapVoxel.x, m_held.snapVoxel.y, m_held.snapVoxel.z, m_held.blocked, m_mtBrick, bb.mx.y);
        }
        fprintf(f, "  %s\n", m_mtDiag.c_str());
        fclose(f);
    }
}

} // namespace brickit
