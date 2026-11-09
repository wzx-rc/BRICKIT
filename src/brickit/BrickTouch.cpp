// BrickIt - the fingers and the Apple Pencil on the iPad: the touch gestures of the web builder (canvas pointer events
// of BrickIt_Builder.html), on the same editing functions as the mouse of the PC.
//   In the 3D view:
//     one finger on empty space          orbit (a tap without moving deselects)
//     two fingers                        pinch = zoom, both moving = pan; a quick tap of two fingers = Annuler,
//                                        of three fingers = Rétablir
//     one finger on a brick (Viseur)     a tap selects it, a drag of 8 px takes it in hand, release drops it
//     a brick in hand                    the finger moves it, release drops it; a second finger: tap = turn 90 degrees,
//                                        swipe left / right = turn, up / down = tilt (every 60 px)
//     Sélection mode                     a brick touched is selected, a drag on empty space draws the rectangle
//     Déplacer / Tourner modes           a tap selects, a drag orbits; the arrows / rings of the gizmo follow the finger
//     the Pencil                         the same as the first finger (iPadOS may cancel it when a hand lands: the
//                                        brick stays in hand, the next touch carries it)
//   On a panel the touch is the mouse of the PC (buttons, library, sliders, the HUD around the selection); a card
//   dragged out of the library comes into the hand, and a second finger turns it there too.
#include "BrickApp.h"
#include "imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace brickit {

static float dist2(float ax, float ay, float bx, float by) { return std::hypot(ax - bx, ay - by); }

bool BrickApp::touchOverUi(float x, float y) const {
    for (const ImVec4& r : m_uiRects)
        if (x >= r.x && y >= r.y && x < r.z && y < r.w) return true;
    if (m_libPress >= 0 || m_sheetOpen || m_popup != 0) return true;
    // the ImGui windows (part sheet, settings sheets, popups...): every one that takes input
    ImGuiContext& g = *GImGui;
    for (ImGuiWindow* w : g.Windows) {
        if (!(w->Active || w->WasActive) || w->Hidden || (w->Flags & ImGuiWindowFlags_NoInputs)) continue;
        if (std::strncmp(w->Name, "Debug##", 7) == 0) continue;
        if (w->Rect().Contains(ImVec2(x, y))) return true;
    }
    return false;
}

void BrickApp::touchCamera() {
    m_scene.camera.focusDistance = m_scene.camera.distance;
    m_renderer->resetAccumulation();
    m_lastMove = ImGui::GetTime();
}

void BrickApp::touchPump() {
    if (m_tg.kept && !m_holding) m_tg = TouchGesture();   // the brick kept in hand was placed or cancelled meanwhile
    std::vector<TouchEvent> q;
    q.swap(m_touchQueue);
    for (const TouchEvent& e : q) {
        if (e.type == 0) touchDown(e);
        else if (e.type == 1) touchMove(e);
        else touchUp(e);
    }
    // Studio: a finger held still 0.45 s on a brick opens the marking menu around it; sliding then lifting picks
    // an action or a colour (stSelHud reads the ImGui mouse, fed here like a pressed button)
    TouchGesture& g = m_tg;
    if (g.type == TG_PRESS && m_studioUi && !m_ipadCreme && !m_mechOn && !m_reading && m_touches.count(g.pid) &&
        ImGui::GetTime() - m_touches[g.pid].t > 0.45 && m_frame - g.f0 > 12) {
        const TouchPt& p = m_touches[g.pid];
        select(g.hit);
        ImGuiIO& io = ImGui::GetIO();
        const float s = m_ui, R = 190 * s;   // the ring and the arc of colours stay on screen
        const float cx = std::clamp(p.x, std::min(R, io.DisplaySize.x * 0.5f), std::max(io.DisplaySize.x - R, io.DisplaySize.x * 0.5f));
        const float cy = std::clamp(p.y, std::min(R, io.DisplaySize.y * 0.5f), std::max(io.DisplaySize.y - 120 * s, io.DisplaySize.y * 0.5f));
        m_stMenu = true;
        m_stMenuAt = ImVec2(cx, cy);
        m_stMenuBtn = 0;
        m_stMenuSticky = false;
        m_stMenuWait = true;
        m_stMenuT0 = ImGui::GetTime();
        const uint64_t pid = g.pid;
        g = TouchGesture();
        g.type = TG_MENU;
        g.pid = pid;
        g.hasPid = true;
        io.AddMouseSourceEvent(p.pen ? ImGuiMouseSource_Pen : ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent(p.x, p.y);
        io.AddMouseButtonEvent(0, true);
    }
}

// --------------------------------------------------------------------------- pointerdown

void BrickApp::touchDown(const TouchEvent& e) {
    ImGuiIO& io = ImGui::GetIO();
    const double now = ImGui::GetTime();
    m_touches[e.id] = {e.x, e.y, e.x, e.y, now, e.pen};
    TouchGesture& g = m_tg;

    // a finger on a panel: the mouse of the PC. A second finger while that finger carries a brick from the library
    // turns it (the web: libDrag + another finger)
    if (g.type == TG_MOUSE) {
        if (m_holding && !g.second) { g.second = true; g.secondId = e.id; g.ax = e.x; g.ay = e.y; g.rotated = false; }
        return;
    }
    if (m_touches.size() == 1 && g.type == TG_NONE && touchOverUi(e.x, e.y)) {
        g = TouchGesture();
        g.type = TG_MOUSE;
        g.pid = e.id;
        g.hasPid = true;
        io.AddMouseSourceEvent(e.pen ? ImGuiMouseSource_Pen : ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent(e.x, e.y);
        io.AddMouseButtonEvent(0, true);
        return;
    }

    // the stylus pressed a brick but has not moved yet: a second finger picks it up now and acts as the turning finger.
    // (Two fingers landing together on the model are a pinch: only a press of the Pencil, or one held 0.3 s, is taken.)
    if (g.type == TG_PRESS && e.id != g.pid && (m_touches[g.pid].pen || now - m_touches[g.pid].t > 0.3)) {
        m_pressPoint = g.hitPoint;
        const TouchPt& p = m_touches[g.pid];
        beginMove(g.hit, p.x, p.y);
        const uint64_t pid = g.pid;
        g = TouchGesture();
        g.type = TG_HOLD;
        g.pid = pid;
        g.hasPid = true;
    }
    if (g.type == TG_HOLD && g.kept) {   // a brick kept in hand (Pencil cancelled): this touch carries it
        g.pid = e.id;
        g.hasPid = true;
        g.kept = false;
        updateHeld(e.x, e.y);
        return;
    }
    if (g.type == TG_HOLD) {
        if (!g.second) { g.second = true; g.secondId = e.id; g.ax = e.x; g.ay = e.y; g.rotated = false; }
        return;
    }
    if (g.type == TG_GIZMO || g.type == TG_GROUP || g.type == TG_AIM || g.type == TG_MENU) return;   // one finger at a time on those

    if (m_touches.size() >= 2) {   // two fingers (or three): camera, or a tap that undoes
        if (g.type == TG_MARQUEE) m_marquee = false;
        if (g.type != TG_TWO) {
            g = TouchGesture();
            g.type = TG_TWO;
            g.t0 = now;
            for (auto& t : m_touches) { t.second.sx = t.second.x; t.second.sy = t.second.y; }
        }
        g.fingers = std::max(g.fingers, (int)m_touches.size());
        g.d = 0;
        return;
    }

    g = TouchGesture();
    g.pid = e.id;
    g.hasPid = true;
    g.sx = e.x;
    g.sy = e.y;
    if (m_mechOn) {   // Mechanical mode: a moving part follows the finger (mechUpdate, as the mouse), else orbit
        float3 hit;
        const int b = pickBrick(e.x, e.y, &hit);
        bool movable = false;
        if (b >= 0 && b < (int)m_mm.brickBody.size()) {
            const int body = m_mm.brickBody[b];
            movable = std::find(m_mm.roots.begin(), m_mm.roots.end(), body) == m_mm.roots.end();
        }
        if (movable) {
            g.type = TG_MOUSE;
            io.AddMouseSourceEvent(e.pen ? ImGuiMouseSource_Pen : ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(e.x, e.y);
            io.AddMouseButtonEvent(0, true);
        } else g.type = TG_ORBIT;
        return;
    }
    if (m_reading) { g.type = TG_ORBIT; return; }   // reading the instructions: no editing
    if (!m_holding && !m_pivotEdit) {
        GizmoHandle gh = pickGizmo(e.x, e.y);
        if (gh.kind == 2) {   // centre sphere: free move, the brick goes in hand and drops on release
            m_pressPoint = gizmoPivot(m_selected);
            const bool ap = m_autoPivot;
            m_autoPivot = false;
            beginMove(m_selected, e.x, e.y);
            m_autoPivot = ap;
            g.type = TG_HOLD;
            return;
        }
        if (gh.kind >= 0) { gizmoStart(gh, e.x, e.y); g.type = TG_GIZMO; return; }
    }
    if (m_pivotEdit && focusBrick() >= 0) { aimPivot(e.x, e.y); g.type = TG_AIM; return; }
    if (m_holding && !m_held.dropping) { updateHeld(e.x, e.y); g.type = TG_HOLD; return; }   // the finger moves the brick, release drops it

    float3 pt;
    const int hit = pickBrick(e.x, e.y, &pt);
    if (m_mode == 0) {   // Sélection: a brick touched is selected, a drag on empty space draws the rectangle
        if (hit >= 0) { select(hit); g.type = TG_ORBIT; g.tap = false; }
        else { g.type = TG_MARQUEE; m_marquee = true; m_marquee0 = m_marquee1 = ImVec2(e.x, e.y); }
        return;
    }
    if (m_mode != 1) {   // Déplacer / Tourner: a tap selects (Auto Pivot moves the pivot), dragging orbits
        if (hit >= 0) {
            if (m_autoPivot) {
                const Part& p = m_lib.parts[m_bricks[hit].part];
                float best = 1e30f;
                for (size_t v = 0; v < p.voxels.size(); ++v) {
                    const float d = length(m_build.worldVoxel(m_bricks[hit], p.voxels[v]) - pt);
                    if (d < best) { best = d; m_bricks[hit].grab = (int)v; }
                }
            }
            select(hit);
        }
        g.type = TG_ORBIT;
        g.tap = hit < 0;
        return;
    }
    if (hit >= 0 && m_pencilOnly && !e.pen) {   // « Construire au stylet seulement »: a finger selects, never takes
        select(hit);
        g.type = TG_ORBIT;
        return;
    }
    if (hit >= 0) { g.type = TG_PRESS; g.hit = hit; g.hitPoint = pt; g.f0 = m_frame; return; }   // Viseur: tap selects, drag takes
    g.type = TG_ORBIT;
    g.tap = true;
}

// --------------------------------------------------------------------------- pointermove

void BrickApp::touchMove(const TouchEvent& e) {
    auto it = m_touches.find(e.id);
    if (it == m_touches.end()) return;
    TouchPt& t = it->second;
    const float dx = e.x - t.x, dy = e.y - t.y;
    t.x = e.x;
    t.y = e.y;
    TouchGesture& g = m_tg;
    const float k = 1.0f / std::max(0.5f, m_dpi);   // pixels -> points
    Camera& cam = m_scene.camera;

    // second finger while a brick is held: swipe right / left turns it, up / down tilts it (every 60 points)
    if (g.second && e.id == g.secondId) {
        const float ddx = (e.x - g.ax) * k, ddy = (e.y - g.ay) * k;
        if (std::max(std::fabs(ddx), std::fabs(ddy)) > 60 && m_holding) {
            if (std::fabs(ddx) > std::fabs(ddy)) rotateSelected(ddx > 0 ? 1 : -1);
            else tiltSelected(ddy < 0 ? 1 : -1);
            g.ax = e.x;
            g.ay = e.y;
            g.rotated = true;
        }
        return;
    }
    switch (g.type) {
    case TG_MOUSE:
    case TG_MENU:
        if (e.id == g.pid) ImGui::GetIO().AddMousePosEvent(e.x, e.y);
        break;
    case TG_PRESS:
        if (e.id == g.pid && dist2(e.x, e.y, g.sx, g.sy) * k > 8) {
            const int hit = g.hit;
            const uint64_t pid = g.pid;
            if (unitOf(hit).size() > 1) {   // a group: the whole group moves as one block
                fitGroupContext(hit);
                select(hit);
                m_pressBrick = hit;
                beginGroupDrag(unitOf(hit), e.x, e.y);
                g = TouchGesture();
                g.type = TG_GROUP;
            } else {
                m_pressPoint = g.hitPoint;
                beginMove(hit, e.x, e.y);   // a placed brick comes into the hand
                g = TouchGesture();
                g.type = TG_HOLD;
            }
            g.pid = pid;
            g.hasPid = true;
        }
        break;
    case TG_HOLD:
        if (g.hasPid && e.id == g.pid) updateHeld(e.x, e.y);
        break;
    case TG_GROUP:
        if (e.id == g.pid) updateGroupDrag(e.x, e.y);
        break;
    case TG_GIZMO:
        if (e.id == g.pid) gizmoDragTo(e.x, e.y);
        break;
    case TG_AIM:
        if (e.id == g.pid) aimPivot(e.x, e.y);
        break;
    case TG_MARQUEE:
        if (e.id == g.pid) m_marquee1 = ImVec2(e.x, e.y);
        break;
    case TG_ORBIT:
        if (e.id != g.pid) break;
        g.moved += (std::fabs(dx) + std::fabs(dy)) * k;
        cam.yaw -= dx * k * 0.0065f;
        cam.pitch = clampf(cam.pitch + dy * k * 0.0065f, radians(-5.0f), radians(89.0f));
        touchCamera();
        break;
    case TG_TWO: {
        if (dist2(t.x, t.y, t.sx, t.sy) * k > 10) g.still = false;
        if (m_touches.size() != 2) break;
        auto a = m_touches.begin();
        auto b = std::next(a);
        const float d = dist2(a->second.x, a->second.y, b->second.x, b->second.y);
        const float mx = (a->second.x + b->second.x) * 0.5f, my = (a->second.y + b->second.y) * 0.5f;
        if (g.d > 1 && d > 1) {
            cam.distance = clampf(cam.distance * (g.d / d), 1.0f, 1000.0f);
            float3 r, u, f;
            cam.basis(r, u, f);
            const float kp = cam.distance * std::tan(cam.fovY * 0.5f) * 2.0f / std::max(1.0f, m_stage.w - m_stage.y);
            cam.target = cam.target - r * ((mx - g.mx) * kp) + u * ((my - g.my) * kp);
            touchCamera();
        }
        g.d = d;
        g.mx = mx;
        g.my = my;
        break;
    }
    default: break;
    }
}

// --------------------------------------------------------------------------- pointerup / pointercancel

void BrickApp::touchUp(const TouchEvent& e) {
    auto it = m_touches.find(e.id);
    if (it == m_touches.end()) return;
    const TouchPt t = it->second;
    m_touches.erase(it);
    TouchGesture& g = m_tg;
    ImGuiIO& io = ImGui::GetIO();
    const bool cancelled = e.type == 3;
    const double now = ImGui::GetTime();

    // the second finger of a brick in hand: a tap without a swipe turns it 90 degrees
    if (g.second && e.id == g.secondId) {
        if (!g.rotated && !cancelled && m_holding) rotateSelected(1);
        g.second = false;
        if (m_touches.empty() && !g.kept) g = TouchGesture();
        return;
    }
    switch (g.type) {
    case TG_MOUSE:
    case TG_MENU:
        if (e.id == g.pid && g.type == TG_MENU && cancelled) m_stMenu = false;   // cancelled by iPadOS: no action
        if (e.id == g.pid) {
            io.AddMousePosEvent(e.x, e.y);
            io.AddMouseButtonEvent(0, false);
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);   // a finger does not hover once lifted
            g.hasPid = false;
        }
        break;
    case TG_PRESS:
        if (e.id == g.pid) {
            const int hit = g.hit;
            // a second tap on a group within 0.4 s: inside the group (the double click of the PC)
            if (hit == m_lastTapBrick && now - m_lastTapTime < 0.4 && unitGroup(hit) >= 0) { enterGroup(hit); m_lastTapBrick = -1; }
            else { select(hit); m_lastTapBrick = hit; m_lastTapTime = now; }
            g = TouchGesture();
        }
        break;
    case TG_HOLD:
        if (g.hasPid && e.id == g.pid) {
            // iPadOS may cancel the stylus when a finger lands: the brick stays in hand (the next touch carries it)
            if (cancelled && m_holding) { g.hasPid = false; g.kept = true; return; }
            if (m_holding && !m_pivotEdit) dropHeld();
            g.hasPid = false;
        }
        break;
    case TG_GROUP:
        if (e.id == g.pid) { endGroupDrag(); g.hasPid = false; }
        break;
    case TG_GIZMO:
        if (e.id == g.pid) { m_gizmoDragging = false; autosave(); g.hasPid = false; }
        break;
    case TG_AIM:
        if (e.id == g.pid) g.hasPid = false;
        break;
    case TG_MARQUEE:
        if (e.id == g.pid) {
            m_marquee1 = ImVec2(e.x, e.y);
            if (cancelled) m_marquee = false;
            else finishMarquee(false);
            g = TouchGesture();
        }
        break;
    case TG_ORBIT:
        if (e.id == g.pid && g.tap && g.moved < 6 && !cancelled && m_touches.empty()) {
            if (!leaveGroup()) select(-1);
        }
        break;
    case TG_TWO:
        if (dist2(t.x, t.y, t.sx, t.sy) / std::max(0.5f, m_dpi) > 10) g.still = false;
        if (m_touches.empty() && g.still && !cancelled && now - g.t0 < 0.35) {   // a quick tap of the fingers
            if (g.fingers >= 3) { redo(); toast("Rétablir"); }
            else { undo(); toast("Annuler"); }
        }
        break;
    default: break;
    }
    if (m_touches.empty() && !g.kept) g = TouchGesture();
}

// --------------------------------------------------------------------------- --touch-test (Linux test build)

void BrickApp::touchTestFrame() {
    const std::string& t = m_opt.touchTest;
    if (t.empty()) return;
    static FILE* out = nullptr;
    static float yaw0 = 0, dist0 = 0;
    static size_t count0 = 0;
    static int target = -1;
    static float3 pos0;
    static float4 rot0;
    const float sw = m_stage.z - m_stage.x, sh = m_stage.w - m_stage.y;
    const float cx = m_stage.x + sw * 0.5f, cy = m_stage.y + sh * 0.5f;
    const int f = m_frame;
    auto line = [&](const std::string& s) {
        if (!out) _wfopen_s(&out, (m_exeDir + L"\\touch_test.txt").c_str(), L"w");
        if (out) { fprintf(out, "%s\n", s.c_str()); fflush(out); }
        printf("touch-test: %s\n", s.c_str());
    };
    // an empty point of the view (orbit / marquee): the first of a grid of candidates that hits no brick
    auto emptyPoint = [&]() {
        for (int j = 1; j < 8; ++j)
            for (int i = 1; i < 8; ++i) {
                const float x = m_stage.x + sw * i / 8.0f, y = m_stage.y + sh * j / 8.0f;
                if (pickBrick(x, y) < 0 && !touchOverUi(x, y)) return ImVec2(x, y);
            }
        return ImVec2(m_stage.x + 20, m_stage.y + 20);
    };
    auto brickPoint = [&](int& hit) {   // a point of the view on a brick (the brick that point hits)
        View v = view();
        for (int i = 0; i < (int)m_bricks.size(); ++i) {
            Box bx = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
            float sx, sy;
            if (!v.project((bx.mn + bx.mx) * 0.5f, sx, sy) || touchOverUi(sx, sy)) continue;
            hit = pickBrick(sx, sy);
            if (hit >= 0 && unitOf(hit).size() == 1) return ImVec2(sx, sy);
        }
        hit = -1;
        return ImVec2(cx, cy);
    };
    static ImVec2 a;
    if (f == 20) {
        yaw0 = m_scene.camera.yaw;
        dist0 = m_scene.camera.distance;
        count0 = m_bricks.size();
        m_mode = 1;
        select(-1);
        if (t == "orbit") { a = emptyPoint(); touchBegan(1, a.x, a.y, false); }
        if (t == "pinch") { touchBegan(1, cx - 60, cy, false); touchBegan(2, cx + 60, cy, false); }
        if (t == "undo") {
            int hit;
            brickPoint(hit);
            select(hit);
            deleteSelected();
            line("deleted 1 brick: " + std::to_string(count0) + " -> " + std::to_string(m_bricks.size()));
        }
        if (t == "menu") { a = brickPoint(target); touchBegan(1, a.x, a.y, false); }   // a finger held still: the marking menu
        if (t == "pick" || t == "hold2") {
            a = brickPoint(target);
            pos0 = m_bricks[target].pos;
            rot0 = m_bricks[target].rot;
            touchBegan(1, a.x, a.y, true);   // (the Pencil)
        }
    }
    static ImVec2 mc;
    if (t == "menu" && f > 20 && f < 200 && m_tg.type == TG_PRESS) touchMoved(1, a.x + 1, a.y);   // (a finger trembles a little)
    if (t == "menu" && m_tg.type == TG_MENU && mc.x == 0) {
        mc = m_stMenuAt;
        line(std::string("menu: open after a still press ") + (m_stMenu ? "OK" : "FAIL") + " (frame " + std::to_string(f) + ")");
        const float r = 60 * 0.0174533f, R = 74 * m_ui;   // Dupliquer, at 60 degrees
        touchMoved(1, mc.x + std::cos(r) * R * 0.5f, mc.y + std::sin(r) * R * 0.5f);
        touchMoved(1, mc.x + std::cos(r) * R, mc.y + std::sin(r) * R);
    }
    static int endF = 0;
    if (t == "menu" && mc.x != 0 && !endF && m_tg.type == TG_MENU) { endF = f + 6; }
    if (t == "menu" && endF && f == endF) touchEnded(1, mc.x + std::cos(1.0472f) * 74 * m_ui, mc.y + std::sin(1.0472f) * 74 * m_ui, false);
    if (t == "menu" && endF && f == endF + 6) line(std::string("menu: slide to Dupliquer then lift, bricks ") + std::to_string(count0) + " -> " + std::to_string(m_bricks.size()) + (m_bricks.size() == count0 + 1 && !m_stMenu ? " OK" : " FAIL"));
    if (t == "orbit" && f > 20 && f <= 32) touchMoved(1, a.x + (f - 20) * 15.0f, a.y);
    if (t == "orbit" && f == 33) {
        touchEnded(1, a.x + 180, a.y, false);
        line("orbit: yaw " + std::to_string(yaw0) + " -> " + std::to_string(m_scene.camera.yaw) + (m_scene.camera.yaw < yaw0 ? " OK" : " FAIL"));
    }
    if (t == "pinch" && f > 20 && f <= 32) { touchMoved(1, cx - 60 - (f - 20) * 10.0f, cy); touchMoved(2, cx + 60 + (f - 20) * 10.0f, cy); }
    if (t == "pinch" && f == 33) {
        touchEnded(1, cx - 180, cy, false);
        touchEnded(2, cx + 180, cy, false);
        line("pinch: distance " + std::to_string(dist0) + " -> " + std::to_string(m_scene.camera.distance) + (m_scene.camera.distance < dist0 * 0.6f ? " OK" : " FAIL"));
    }
    if (t == "undo" && f == 24) { touchBegan(1, cx - 40, cy, false); touchBegan(2, cx + 40, cy, false); }
    if (t == "undo" && f == 26) { touchEnded(1, cx - 40, cy, false); touchEnded(2, cx + 40, cy, false); }
    if (t == "undo" && f == 28) line("two-finger tap: bricks " + std::to_string(m_bricks.size()) + (m_bricks.size() == count0 ? " OK" : " FAIL"));
    if ((t == "pick" || t == "hold2") && f > 20 && f <= 32) touchMoved(1, a.x + (f - 20) * 12.0f, a.y - (f - 20) * 6.0f);
    if (t == "pick" && f == 26) line(std::string("pick: in hand ") + (m_holding && m_held.brick == target ? "OK" : "FAIL"));
    if (t == "hold2" && f == 26) touchBegan(2, cx + 200, cy + 100, false);
    if (t == "hold2" && f == 28) touchEnded(2, cx + 200, cy + 100, false);
    if ((t == "pick" || t == "hold2") && f == 34) touchEnded(1, a.x + 144, a.y - 72, false);
    if ((t == "pick" || t == "hold2") && f == 110) {
        const Brick& b = m_bricks[target];
        const bool moved = length(b.pos - pos0) > 0.5f;
        const bool turned = std::fabs(b.rot.x - rot0.x) + std::fabs(b.rot.y - rot0.y) + std::fabs(b.rot.z - rot0.z) + std::fabs(b.rot.w - rot0.w) > 0.1f;
        line(t + ": dropped " + (!m_holding ? "OK" : std::string("FAIL (blocked ") + std::to_string(m_held.blocked) + " dropping " + std::to_string(m_held.dropping) + ")") + ", moved " + (moved ? "OK" : "FAIL") +
             (t == "hold2" ? std::string(", turned by the second finger ") + (turned ? "OK" : "FAIL") : std::string()));
    }
}

} // namespace brickit
