// BrickIt - viewport editing: the brick in hand (typed snap), selection, duplicate, rotate, colours, undo, files.
#include "BrickApp.h"
#include <algorithm>
#include "Platform.h"
#include <cstdio>

namespace brickit {

View BrickApp::view() const {
    View v;
    v.cam = m_scene.camera;
    v.ox = m_stage.x;
    v.oy = m_stage.y;
    v.w = std::max(1.0f, m_stage.z - m_stage.x);
    v.h = std::max(1.0f, m_stage.w - m_stage.y);
    return v;
}

// Ray picking for tens of thousands of bricks: world boxes cached per scene revision, slab test on every box (about
// 0.3 ms at 50k), then the triangles of the boxes hit, nearest box first, stopping once a box starts beyond the best hit.
static bool rayTriangle(const float3& o, const float3& d, const float3& a, const float3& b, const float3& c, float& t) {
    float3 e1 = b - a, e2 = c - a, p = cross(d, e2);
    float det = dot(e1, p);
    if (std::fabs(det) < 1e-12f) return false;
    float inv = 1.0f / det;
    float3 s = o - a;
    float u = dot(s, p) * inv;
    if (u < 0 || u > 1) return false;
    float3 q = cross(s, e1);
    float v = dot(d, q) * inv;
    if (v < 0 || u + v > 1) return false;
    float tt = dot(e2, q) * inv;
    if (tt <= 1e-5f || tt >= t) return false;
    t = tt;
    return true;
}

int BrickApp::pickBrick(float sx, float sy, float3* point, int exclude) {
    float3 o, d;
    view().ray(sx, sy, o, d);
    const int n = (int)m_bricks.size();
    if (m_pickVersion != m_scene.objectsVersion || (int)m_pickBoxes.size() != n) {
        m_pickVersion = m_scene.objectsVersion;
        m_pickBoxes.resize(n);
        for (int i = 0; i < n; ++i) {
            const Object& ob = m_scene.objects[m_firstBrickObject + i];
            const Part& p = m_lib.parts[m_bricks[i].part];
            Affine xf = ob.transform();
            Box bx{float3(1e30f), float3(-1e30f)};
            for (int c = 0; c < 8; ++c) {
                float3 w = xf.point(float3((c & 1) ? p.bmax.x : p.bmin.x, (c & 2) ? p.bmax.y : p.bmin.y, (c & 4) ? p.bmax.z : p.bmin.z));
                bx.mn = minv(bx.mn, w);
                bx.mx = maxv(bx.mx, w);
            }
            m_pickBoxes[i] = bx;
        }
    }
    const float3 inv(1.0f / (std::fabs(d.x) > 1e-12f ? d.x : 1e-12f), 1.0f / (std::fabs(d.y) > 1e-12f ? d.y : 1e-12f), 1.0f / (std::fabs(d.z) > 1e-12f ? d.z : 1e-12f));
    std::vector<std::pair<float, int>> hits;
    for (int i = 0; i < n; ++i) {
        if (i == exclude || !m_scene.objects[m_firstBrickObject + i].visible) continue;
        const Box& b = m_pickBoxes[i];
        float t0 = 0, t1 = 1e30f;
        for (int k = 0; k < 3; ++k) {
            float a = ((&b.mn.x)[k] - (&o.x)[k]) * (&inv.x)[k], c = ((&b.mx.x)[k] - (&o.x)[k]) * (&inv.x)[k];
            if (a > c) std::swap(a, c);
            t0 = std::max(t0, a);
            t1 = std::min(t1, c);
        }
        if (t0 <= t1) hits.push_back({t0, i});
    }
    std::sort(hits.begin(), hits.end());
    float best = 1e30f;
    int bestI = -1;
    for (auto& h : hits) {
        if (h.first > best) break;
        const int i = h.second;
        const Part& p = m_lib.parts[m_bricks[i].part];
        Affine xi = m_scene.objects[m_firstBrickObject + i].transform().inverse();
        float3 lo = xi.point(o), ld = xi.vector(d);   // t stays in world units (rigid transforms)
        const std::vector<Vertex>& V = p.mesh.vertices;
        const std::vector<uint32_t>& I = p.mesh.indices;
        for (size_t t = 0; t + 2 < I.size(); t += 3)
            if (rayTriangle(lo, ld, V[I[t]].pos, V[I[t + 1]].pos, V[I[t + 2]].pos, best)) bestI = i;
    }
    if (bestI >= 0 && point) *point = o + d * best;
    return bestI;
}

// --------------------------------------------------------------------------- brick in hand

void BrickApp::spawn(int part, float sx, float sy) {
    if (part < 0 || !m_lib.get(m_lib.parts[part].name)) return;
    if (m_holding) cancelHeld();
    pushUndo();
    Brick b;
    b.part = part;
    b.color = m_color;
    b.pos = m_scene.camera.target;
    m_bricks.push_back(b);
    rebuildScene();
    m_build.beginHold(m_held, (int)m_bricks.size() - 1, true);
    m_holding = true;
    m_selected = (int)m_bricks.size() - 1;
    m_multi.clear();
    noteRecent(part);
    syncObjects();   // focus mode: the rest dims
    updateHeld(sx, sy);
    m_held.vpos = m_held.target + float3(0, 1.2f, 0);
    m_held.vrot = m_held.rot;
    m_held.squash = 0.55f;
    toast("Brique en main : clic pour poser, R tourner, T basculer, clic droit annuler");
}

void BrickApp::beginMove(int brick, float sx, float sy) {
    if (m_holding) cancelHeld();
    pushUndo();
    m_build.beginHold(m_held, brick, false);
    // Auto Pivot: the voxel of the brick nearest to where it was grabbed becomes its pivot
    const Part& p = m_lib.parts[m_bricks[brick].part];
    float best = m_autoPivot ? 1e30f : -1.0f;
    for (size_t v = 0; v < p.voxels.size(); ++v) {
        float d = length(m_build.worldVoxel(m_bricks[brick], p.voxels[v]) - m_pressPoint);
        if (d < best) { best = d; m_held.grab = p.voxels[v]; m_bricks[brick].grab = (int)v; }
    }
    m_holding = true;
    m_selected = brick;
    m_multi.clear();
    m_held.vpos = m_bricks[brick].pos;
    m_held.vrot = m_bricks[brick].rot;
    m_held.vvel = float3(0, 8, 0);   // a placed brick jumps into the hand
    m_held.squash = 1.12f;
    syncObjects();
    updateHeld(sx, sy);
}

void BrickApp::updateHeld(float sx, float sy) {
    if (!m_holding) return;
    m_holdPos = ImVec2(sx, sy);
    float3 hit;
    const bool over = pickBrick(sx, sy, &hit, m_held.brick) >= 0;
    const bool wasBlocked = m_held.blocked;
    m_build.computeTarget(m_held, view(), sx, sy, over ? &hit : nullptr);
    Brick& b = m_bricks[m_held.brick];
    b.pos = m_held.target;
    b.rot = m_held.rot;
    if (m_held.snapVoxel.x != m_held.lastSnap.x || m_held.snapVoxel.y != m_held.lastSnap.y || m_held.snapVoxel.z != m_held.lastSnap.z) {
        m_held.snapT = 0;   // new voxel: replay the highlight
        m_held.lastSnap = m_held.snapVoxel;
    }
    if (wasBlocked != m_held.blocked) syncObjects();
    m_lastMove = ImGui::GetTime();
}

void BrickApp::dropHeld(bool keepCopy) {
    if (!m_holding) return;
    if (m_held.blocked) {
        toast("Placement impossible : plus de 50 % de la brique est dans une autre");
        return;
    }
    if (m_held.dropping) return;
    m_held.dropping = true;   // drop animation (0.15 s), finishDrop at its end
    m_held.dropT = 0;
    m_held.dropFrom = m_held.vpos;
    m_held.dropRotFrom = m_held.vrot;
    m_held.keepCopy = keepCopy;
    if (!m_animOn) finishDrop();
}

void BrickApp::cancelHeld() {
    if (!m_holding) return;
    m_holding = false;
    if (m_held.isNew) {
        m_bricks.erase(m_bricks.begin() + m_held.brick);
        m_selected = -1;
        rebuildScene();
    } else {
        m_bricks[m_held.brick] = m_held.original;
        syncObjects();
    }
    if (!m_undo.empty()) m_undo.pop_back();
}

// --------------------------------------------------------------------------- selection and edits

std::vector<int> BrickApp::selection() const {
    std::vector<int> s;
    if (m_selected >= 0) s.push_back(m_selected);
    for (int i : m_multi)
        if (i != m_selected) s.push_back(i);
    return s;
}

void BrickApp::select(int brick, bool add) {
    // a brick of a group selects the whole group at the level entered (double click enters it)
    if (brick >= 0 && !add) fitGroupContext(brick);
    const std::vector<int> unit = unitOf(brick);
    if (add && brick >= 0) {
        std::vector<int> s = selection();
        const bool in = std::find(s.begin(), s.end(), brick) != s.end();
        for (int i : unit) {
            auto it = std::find(s.begin(), s.end(), i);
            if (in && it != s.end()) s.erase(it);
            else if (!in && it == s.end()) s.push_back(i);
        }
        m_selected = s.empty() ? -1 : s[0];
        m_multi.assign(s.size() > 1 ? s.begin() + 1 : s.end(), s.end());
    } else {
        m_selected = brick;
        m_multi.clear();
        for (int i : unit)
            if (i != brick) m_multi.push_back(i);
    }
    syncObjects();
}

void BrickApp::deleteSelected() {
    std::vector<int> s = selection();
    if (s.empty()) return;
    pushUndo();
    std::sort(s.begin(), s.end(), std::greater<int>());
    for (int i : s) m_bricks.erase(m_bricks.begin() + i);
    m_selected = -1;
    m_multi.clear();
    rebuildScene();
}

// Ctrl+D: a copy stacked right above (first free place), selected and in place; the original stays.
void BrickApp::duplicateSelected() {
    if (m_holding) return;
    std::vector<int> s = selection();
    if (s.empty()) return;
    if (unitGroup(m_selected) >= 0) {   // a group: one copy of the whole group, above it
        pushUndo();
        if (!duplicateGroup(s)) m_undo.pop_back();
        return;
    }
    pushUndo();
    int last = -1;
    for (int i : s) {
        int c = m_build.stackedCopy(i);
        if (c >= 0) last = c;
    }
    if (last < 0) { m_undo.pop_back(); toast("Pas de place libre au-dessus de cette brique"); return; }
    m_selected = last;
    m_multi.clear();
    rebuildScene();
}

void BrickApp::rotateSelected(int dir) {
    if (m_holding) {
        m_build.rotateHeld(m_held, float3(0, 1, 0), dir);
        updateHeld(m_holdPos.x, m_holdPos.y);   // (the finger or the cursor that aims it)
        return;
    }
    std::vector<int> s = selection();
    if (s.empty()) return;
    pushUndo();
    if (unitGroup(m_selected) >= 0) rotateGroup(s, dir);   // a group turns as one block
    else for (int i : s) m_build.rotatePlaced(i, dir);
    syncObjects();
}

// Radial menu: tilt towards the viewer (around the camera right axis snapped to world X or Z), about the pivot.
void BrickApp::tiltSelected(int dir) {
    float3 r, u, f;
    m_scene.camera.basis(r, u, f);
    float3 axis = std::fabs(r.x) > std::fabs(r.z) ? float3(r.x > 0 ? 1.0f : -1.0f, 0, 0) : float3(0, 0, r.z > 0 ? 1.0f : -1.0f);
    if (m_holding) {
        m_build.rotateHeld(m_held, axis, -dir);
        updateHeld(m_holdPos.x, m_holdPos.y);
        syncObjects();
        return;
    }
    std::vector<int> sel = selection();
    if (sel.empty()) return;
    pushUndo();
    for (int i : sel) m_build.rotatePlacedAxis(i, axis, -dir);
    syncObjects();
}

// Touch HUD (as Tiny Glade): 15 degrees about the vertical axis through the centre of the selection (a group as one
// block), the bricks kept where they are. Off the quarter turns they are free (no stud snapping, the exact angle saved);
// back on a quarter turn they snap to the stud lattice again. Safety on: a turn more than 50 % into another brick is refused.
void BrickApp::turnSelectedFree(int dir) {
    if (m_holding) return;
    std::vector<int> s = selection();
    if (s.empty()) return;
    float3 mn(1e30f), mx(-1e30f);
    for (int i : s) {
        const Box b = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
        mn = minv(mn, b.mn);
        mx = maxv(mx, b.mx);
    }
    const float3 c = (mn + mx) * 0.5f;
    pushUndo();
    const float4 q = qAxisAngle(float3(0, 1, 0), dir * kPi / 12.0f);
    for (int i : s) {
        Brick& b = m_bricks[i];
        b.rot = qNormalize(qMul(q, b.rot));
        b.pos = c + qRot(q, b.pos - c);
    }
    const float4 r0 = m_bricks[s[0]].rot, sq = snapQuat(r0);
    if (std::fabs(r0.x * sq.x + r0.y * sq.y + r0.z * sq.z + r0.w * sq.w) > 0.9999f) {   // a quarter turn: on the lattice again
        for (int i : s) m_bricks[i].rot = snapQuat(m_bricks[i].rot);
        const Brick& b0 = m_bricks[s[0]];
        const float3 w = m_build.worldVoxel(b0, m_build.grabOf(b0));
        const float3 t(std::round((w.x - 0.4f) / kPitch) * kPitch + 0.4f, w.y, std::round((w.z - 0.4f) / kPitch) * kPitch + 0.4f);
        for (int i : s) { m_bricks[i].pos = m_bricks[i].pos + (t - w); m_bricks[i].free = false; }
    } else for (int i : s) m_bricks[i].free = true;
    if (m_build.safety) {
        BoxHash others;
        for (int j = 0; j < (int)m_bricks.size(); ++j)
            if (std::find(s.begin(), s.end(), j) == s.end()) others.insert(j, m_build.bodyBox(m_bricks[j].part, m_bricks[j].rot, m_bricks[j].pos));
        float worst = 0;
        for (int i : s) worst = std::max(worst, others.worstOverlap(m_build.bodyBox(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos)));
        if (worst > 0.5f) {
            m_bricks = m_undo.back();
            m_undo.pop_back();
            toast("Rotation refusée : la brique entrerait dans une autre (Sécurité)");
            return;
        }
    }
    syncObjects();
}

// Voxels shown as dots: studs and anti-studs yellow, pins and pin holes cyan, axles and axle holes magenta.
void BrickApp::drawVoxels() {
    if (!m_showVox) return;
    static const ImU32 col[3] = {IM_COL32(0xff, 0xc2, 0x1a, 230), IM_COL32(0x38, 0xd0, 0xe8, 230), IM_COL32(0xe0, 0x5a, 0xd7, 230)};
    View v = view();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float r = 2.5f * m_ui;
    int drawn = 0;
    const int fb = focusBrick();   // as the web: the voxels of the selected / held brick only
    for (size_t i = 0; i < m_bricks.size() && drawn < 20000; ++i) {
        if ((int)i != fb) continue;
        const Object& o = m_scene.objects[m_firstBrickObject + i];
        Affine xf = o.transform();
        const Part& p = m_lib.parts[m_bricks[i].part];
        for (size_t k = 0; k < p.voxels.size(); ++k) {
            float sx, sy;
            if (!v.project(xf.point(p.voxels[k]), sx, sy)) continue;
            uint8_t c = p.conn[k];
            int t = (c == CONN_PIN || c == CONN_PINHOLE) ? 1 : (c == CONN_AXLE || c == CONN_AXLEHOLE) ? 2 : 0;
            dl->AddCircleFilled(ImVec2(sx, sy), r, col[t], 8);
            ++drawn;
        }
    }
}

void BrickApp::applyColor(int color) {
    m_color = color;
    m_recentColors.erase(std::remove(m_recentColors.begin(), m_recentColors.end(), color), m_recentColors.end());
    m_recentColors.insert(m_recentColors.begin(), color);
    if (m_recentColors.size() > 12) m_recentColors.resize(12);
    std::vector<int> s = selection();
    if (m_holding) s.push_back(m_held.brick);
    if (s.empty()) return;
    if (!m_holding) pushUndo();
    for (int i : s) m_bricks[i].color = color;
    syncObjects();
}

// the selection moved by dx / dz studs and dy plates (the arrows of the keyboard), kept above the floor
void BrickApp::nudgeSelected(int dx, int dy, int dz) {
    const std::vector<int> s = selection();
    if (s.empty()) return;
    pushUndo();
    const float3 d(dx * kPitch, dy * kPlate, dz * kPitch);
    float low = 1e30f;
    for (int i : s) low = std::min(low, m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos + d).mn.y);
    const float3 dd = low < -1e-3f ? float3(d.x, d.y - low, d.z) : d;
    for (int i : s) m_bricks[i].pos = m_bricks[i].pos + dd;
    syncObjects();
    autosave();
}

void BrickApp::pushUndo() {
    m_undo.push_back(m_bricks);
    if (m_undo.size() > 100) m_undo.erase(m_undo.begin());
    m_redo.clear();
}

void BrickApp::restore(const std::vector<Brick>& snap) {
    m_bricks = snap;
    m_selected = -1;
    m_multi.clear();
    rebuildScene();
}

void BrickApp::undo() {
    if (m_holding) { cancelHeld(); return; }
    if (m_undo.empty()) return;
    m_redo.push_back(m_bricks);
    restore(m_undo.back());
    m_undo.pop_back();
}

void BrickApp::redo() {
    if (m_holding || m_redo.empty()) return;
    m_undo.push_back(m_bricks);
    restore(m_redo.back());
    m_redo.pop_back();
}

void BrickApp::noteRecent(int part) {
    m_recent.erase(std::remove(m_recent.begin(), m_recent.end(), part), m_recent.end());
    m_recent.insert(m_recent.begin(), part);
    if (m_recent.size() > 24) m_recent.resize(24);
}

// --------------------------------------------------------------------------- files

bool BrickApp::saveScene(const std::wstring& path) {
    std::string text = m_build.toJson(m_scene.camera);
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) { toast("Impossible d'écrire le fichier"); return false; }
    fwrite(text.data(), 1, text.size(), f);
    fclose(f);
    m_scenePath = path;
    toast("Scène enregistrée");
    return true;
}

bool BrickApp::openScene(const std::wstring& path) {
    std::wstring ext = path.size() > 4 ? path.substr(path.size() - 4) : L"";
    for (auto& c : ext) c = (wchar_t)towlower(c);
    if (ext == L".ldr" || ext == L".mpd" || ext == L".dat") return importLDraw(path);   // an LDraw model (OMR)
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) { toast("Fichier introuvable"); return false; }
    std::string text;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
    fclose(f);
    std::vector<Brick> before = m_bricks;
    std::string msg;
    if (!m_build.fromJson(text, &m_scene.camera, &msg)) { toast(msg); return false; }
    m_undo.push_back(before);
    m_redo.clear();
    m_selected = -1;
    m_multi.clear();
    m_scenePath = path;
    rebuildScene();
    toast(msg);
    return true;
}

void BrickApp::saveDialog() {
    std::wstring suggested = m_scenePath;
    if (suggested.empty() && m_ipad) {   // the iPad saves without a dialog (Documents): a new name for every new scene
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t name[64];
        swprintf(name, 64, L"BrickIt %04d-%02d-%02d %02dh%02d.v3d.json", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
        suggested = name;
    }
    plat::saveFileDialog(L"Scène BrickIt (*.v3d.json)\0*.v3d.json;*.json\0Tous les fichiers\0*.*\0", L"v3d.json", suggested,
                         [this](const std::wstring& p) { if (!p.empty()) saveScene(p); });
}

void BrickApp::openDialog() {
    plat::openFileDialog(L"Scène BrickIt ou modèle LDraw (*.v3d.json, *.ldr, *.mpd)\0*.v3d.json;*.json;*.ldr;*.mpd;*.dat\0Tous les fichiers\0*.*\0",
                         L"v3d.json", [this](const std::wstring& p) { if (!p.empty()) openScene(p); }, m_scenePath);
}

// --------------------------------------------------------------------------- input

// the selection rectangle released (mode Sélection): bricks whose box centre falls inside, add = Shift / Ctrl
void BrickApp::finishMarquee(bool add) {
    m_marquee = false;
    const float x0 = std::min(m_marquee0.x, m_marquee1.x), x1 = std::max(m_marquee0.x, m_marquee1.x);
    const float y0 = std::min(m_marquee0.y, m_marquee1.y), y1 = std::max(m_marquee0.y, m_marquee1.y);
    if (x1 - x0 < 4 && y1 - y0 < 4) { if (!add) select(-1); return; }
    std::vector<int> all = add ? selection() : std::vector<int>();
    View v = view();
    int hits = 0;
    for (int i = 0; i < (int)m_bricks.size(); ++i) {
        Box bx = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
        float sx, sy;
        if (!v.project((bx.mn + bx.mx) * 0.5f, sx, sy)) continue;
        if (sx >= x0 && sx <= x1 && sy >= y0 && sy <= y1) { ++hits; if (std::find(all.begin(), all.end(), i) == all.end()) all.push_back(i); }
    }
    m_selected = all.empty() ? -1 : all[0];
    m_multi.assign(all.size() > 1 ? all.begin() + 1 : all.end(), all.end());
    syncObjects();
    char t[64];
    snprintf(t, sizeof(t), "%d briques sélectionnées", (int)all.size());
    if (!all.empty()) toast(t);
}


void BrickApp::editInput() {
    ImGuiIO& io = ImGui::GetIO();
    const bool keys = !io.WantTextInput;
    if (m_reading) {   // reading the instructions: the pages, Échap closes; no editing (the camera still orbits)
        readInput();
        if (keys && ImGui::IsKeyPressed(ImGuiKey_Escape)) stopReading();
        return;
    }
    if (keys) {
        const bool ctrl = io.KeyCtrl;
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) undo();
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) redo();
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D)) duplicateSelected();
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_G) && !m_holding) { if (io.KeyShift) ungroupSelected(); else groupSelected(); }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) { if (io.KeyShift) saveDialog(); else saveNow(); }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N)) newScene();
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) openDialog();
        // keys of the web: R or E = turn +90, Q = -90, C = pivot voxel, V = voxels, P = pivot edit, A = animations,
        // F = focus mode, Space = take the selected brick in hand
        if (!ctrl && (ImGui::IsKeyPressed(ImGuiKey_R) || ImGui::IsKeyPressed(ImGuiKey_E))) rotateSelected(1);
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_Q)) rotateSelected(-1);
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) rotateSelected(1);   // (Q / S: Wzx 09/11/2026)
        if (!m_reading && !m_holding) {   // the arrows: the selection moves by a stud, along the view; Page up / down: a plate
            int ax = 0, az = 0, ay = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) ax = -1;
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) ax = 1;
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) az = 1;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) az = -1;
            if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) ay = 1;
            if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) ay = -1;
            if (ax || az || ay) {
                float3 r, u, f;
                m_scene.camera.basis(r, u, f);
                // the right and the forward of the view, snapped to the axes of the grid
                const float3 fw(f.x, 0, f.z);
                const bool rx = std::fabs(r.x) >= std::fabs(r.z);
                const int rs = rx ? (r.x >= 0 ? 1 : -1) : (r.z >= 0 ? 1 : -1);
                const int fs = rx ? (fw.z >= 0 ? 1 : -1) : (fw.x >= 0 ? 1 : -1);
                int dx = 0, dz = 0;
                if (rx) { dx = ax * rs; dz = az * fs; } else { dz = ax * rs; dx = az * fs; }
                nudgeSelected(dx, ay, dz);
            }
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_C)) cycleGrab();
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_V)) { m_showVox = !m_showVox; syncObjects(); toast(m_showVox ? "Voxels : affichés (tenons jaunes, pins cyan, axles magenta)" : "Voxels : masqués"); }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_P)) m_pivotEdit = !m_pivotEdit;
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_A)) m_animOn = !m_animOn;
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_F)) { m_focusDim = !m_focusDim; syncObjects(); toast(m_focusDim ? "Mode focus : les autres briques à 50 %" : "Mode focus désactivé"); }
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) pickUpSelected();
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_T) && m_holding) {   // tilt towards the viewer
            float3 r, u, f;
            m_scene.camera.basis(r, u, f);
            float3 axis = std::fabs(r.x) > std::fabs(r.z) ? float3(r.x > 0 ? 1.0f : -1.0f, 0, 0) : float3(0, 0, r.z > 0 ? 1.0f : -1.0f);
            m_build.rotateHeld(m_held, axis, io.KeyShift ? 1 : -1);
            updateHeld(io.MousePos.x, io.MousePos.y);
            syncObjects();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            if (m_holding) { bool isNew = m_held.isNew; cancelHeld(); if (!isNew) deleteSelected(); }
            else deleteSelected();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            if (m_holding) cancelHeld();
            else if (!leaveGroup()) select(-1);   // inside a group: Échap leaves it first
        }
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_G)) { m_build.gridSnap = !m_build.gridSnap; toast(m_build.gridSnap ? "Grille : oui" : "Grille : non"); }
        if (m_studioUi && !m_ipad && !ctrl) {   // Studio: the tools on 1 to 6 (Sélection, Construire, Pinceau, Pipette, Déplacer, Tourner)
            static const ImGuiKey kKeys[6] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6};
            for (int k = 0; k < 6; ++k) if (ImGui::IsKeyPressed(kKeys[k])) stTool(k);
        }
    }
    if (m_stMenuEat == ImGui::GetFrameCount()) { m_pressed = false; return; }   // the click that closed a sticky marking menu
    if (m_stMenu) return;   // the marking menu of the Studio interface has the mouse until it is released (stSelHud)

    if (m_tg.type >= TG_ORBIT) return;   // iPad: a finger gesture in the view (BrickTouch.cpp) has the bricks
    const ImVec2 mp = io.MousePos;
    const bool inView = !io.WantCaptureMouse && !m_overUi;
    // the brick in hand follows the cursor
    // pivot edit: the cursor aims a voxel of the focus brick (no exact click needed); a click does not drop
    if (m_pivotEdit && focusBrick() >= 0 && inView) {
        if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0 || ImGui::IsMouseClicked(ImGuiMouseButton_Left)) aimPivot(mp.x, mp.y);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Left)) return;
    }
    if (m_holding && inView && m_opt.spawnPart.empty() && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)) updateHeld(mp.x, mp.y);

    // right button: a click without moving cancels the brick in hand (a drag orbits the camera)
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { m_rmbPos = mp; m_rmbMoved = false; }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && (std::fabs(mp.x - m_rmbPos.x) + std::fabs(mp.y - m_rmbPos.y)) > 4) m_rmbMoved = true;
    // Studio: the right button on a brick opens the marking menu at once (Wzx 09/11/2026, as Maya): released on an
    // action it acts, a quick flick in a direction acts at once (stSelHud), released in place it stays open and the
    // next click acts. Off a brick, the right drag orbits the camera.
    if (m_studioUi && !m_ipad && !m_holding && !m_mechOn && !m_reading) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { m_rmbBrick = inView ? pickBrick(mp.x, mp.y) : -1; m_rmbT = ImGui::GetTime(); }
        if (m_rmbBrick >= 0) {
            const bool sticky = false;
            if (m_selected != m_rmbBrick) select(m_rmbBrick);
            m_stMenu = true;
            m_stMenuT0 = ImGui::GetTime();
            m_stMenuAt = m_rmbPos;
            m_stMenuBtn = 1;
            m_stMenuSticky = sticky;
            m_rmbBrick = -1;
            m_pressed = false;
            return;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) || m_rmbMoved) m_rmbBrick = -1;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !m_rmbMoved && inView) {
        if (m_holding) cancelHeld();
        else if (m_dev) { const int b = pickBrick(mp.x, mp.y); if (b >= 0) { m_ctxPart = m_bricks[b].part; m_ctxPos = mp; } }   // DEV: Edit thumbnail
    }
    if (!inView) return;
    if (io.KeyAlt) return;   // Alt + left drag orbits

    if (!m_holding && !m_gizmoDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_gizmoHover = pickGizmo(mp.x, mp.y);
    if (m_gizmoDragging) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { m_gizmoDragging = false; autosave(); }
        else gizmoDragTo(mp.x, mp.y);
        return;
    }
    if (m_pipette && ImGui::IsKeyPressed(ImGuiKey_Escape)) m_pipette = false;
    if (m_paintStroke) {   // the Pinceau held: held a moment (or moved), a circle of rays paints all it covers
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !m_paintTool) {
            m_paintStroke = false;
            m_paintCircle = false;
            if (m_paintUndo) autosave();
            m_paintUndo = false;
        } else {
            const double now = ImGui::GetTime();
            if (!m_paintCircle && (now - m_paintStrokeT0 > 0.18 || std::fabs(mp.x - m_paintStrokeAt.x) + std::fabs(mp.y - m_paintStrokeAt.y) > 6 * m_ui)) {
                m_paintCircle = true;
                m_paintSampleT = -1;
            }
            if (m_paintCircle && (std::fabs(mp.x - m_paintSampleAt.x) + std::fabs(mp.y - m_paintSampleAt.y) > 3 || now - m_paintSampleT > 0.08)) {
                m_paintSampleAt = mp;
                m_paintSampleT = now;
                const float R = m_paintRadius * m_ui;
                bool changed = false;
                auto ray = [&](float x, float y) { const int b = pickBrick(x, y); if (b >= 0) changed |= paintHit(b, ImVec2(x, y), true); };
                ray(mp.x, mp.y);
                for (int k = 0; k < 6; ++k) ray(mp.x + std::cos(k * 1.0472f) * R * 0.5f, mp.y + std::sin(k * 1.0472f) * R * 0.5f);
                for (int k = 0; k < 12; ++k) ray(mp.x + std::cos(k * 0.5236f + 0.26f) * R * 0.92f, mp.y + std::sin(k * 0.5236f + 0.26f) * R * 0.92f);
                if (changed) syncObjects();
            }
            return;   // (the stroke has the mouse)
        }
    }
    if (m_pipette && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_holding) {   // the pipette of the HUD: the colour of the brick clicked
        const int b = pickBrick(mp.x, mp.y);
        if (b >= 0) {
            m_color = m_bricks[b].color;
            m_pipette = false;
            m_paintFx.push_back({mp, m_color, ImGui::GetTime(), 0});   // (a ring, a drop to the active colour)
            m_paintSwatchPop = ImGui::GetTime() + 0.58;
            if (m_paintMode) m_paintTool = true;   // (PEINDRE: back to the Pinceau with the colour taken)
            if (!m_paintMode) toast(std::string("Couleur prise : ") + m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)].name + " (pinceau pour l'appliquer)");
        }
        return;
    }
    if (m_paintTool && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_holding) {   // Studio, Pinceau: the colour on the brick clicked
        const int b = pickBrick(mp.x, mp.y);
        m_paintStroke = true;   // (held: the circle; editInput above)
        m_paintCircle = false;
        m_paintUndo = false;
        m_paintStrokeT0 = ImGui::GetTime();
        m_paintStrokeAt = m_paintSampleAt = mp;
        m_paintDone.clear();
        if (b >= 0 && paintHit(b, mp, true)) syncObjects();
        return;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (m_holding) { dropHeld(io.KeyCtrl); return; }
        if (!m_pivotEdit) {
            GizmoHandle gh = pickGizmo(mp.x, mp.y);
            if (gh.kind == 2) {   // centre sphere: free move, the brick goes in hand and drops on release
                m_pressPoint = gizmoPivot(m_selected);
                const bool ap = m_autoPivot;
                m_autoPivot = false;
                beginMove(m_selected, mp.x, mp.y);
                m_autoPivot = ap;
                m_dragStarted = true;
                return;
            }
            if (gh.kind >= 0) { gizmoStart(gh, mp.x, mp.y); return; }
        }
        if (m_mode == 2 || m_mode == 3) {   // Move / Rotate: a tap selects (Auto Pivot moves the pivot), dragging orbits
            float3 pt;
            int hit = pickBrick(mp.x, mp.y, &pt);
            if (hit >= 0) {
                if (m_autoPivot) {
                    const Part& p = m_lib.parts[m_bricks[hit].part];
                    float best = 1e30f;
                    for (size_t v = 0; v < p.voxels.size(); ++v) {
                        float d = length(m_build.worldVoxel(m_bricks[hit], p.voxels[v]) - pt);
                        if (d < best) { best = d; m_bricks[hit].grab = (int)v; }
                    }
                }
                select(hit);
            }
            m_leftOrbit = true;
            return;
        }
        m_pressed = true;
        m_dragStarted = false;
        m_pressPos = mp;
        m_pressT = ImGui::GetTime();
        m_pressBrick = pickBrick(mp.x, mp.y, &m_pressPoint);
        if (m_mode == 0 && m_pressBrick < 0) { m_marquee = true; m_marquee0 = m_marquee1 = mp; m_pressed = false; }
        if (m_mode == 1 && m_pressBrick < 0) m_leftOrbit = true;   // Visor: dragging the empty space orbits
    }
    if (m_marquee) {   // selection rectangle (mode Sélection): bricks whose box centre falls inside, Shift / Ctrl adds
        m_marquee1 = mp;
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) finishMarquee(io.KeyShift || io.KeyCtrl);
        return;
    }
    if (m_groupDrag) {   // a group dragged: it follows the cursor until the button is released
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) { if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0) updateGroupDrag(mp.x, mp.y); }
        else endGroupDrag();
        return;
    }
    if (m_studioUi && !m_ipad && m_pressed && m_pressBrick >= 0 && !m_dragStarted && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        (std::fabs(mp.x - m_pressPos.x) + std::fabs(mp.y - m_pressPos.y)) <= 6 && ImGui::GetTime() - m_pressT > 0.26) {
        m_pressed = false;   // Studio: a press held still on a brick opens the marking menu where it is held
        if (m_selected != m_pressBrick) select(m_pressBrick);
        m_stMenu = true;
        m_stMenuT0 = ImGui::GetTime();
        m_stMenuAt = m_pressPos;
        m_stMenuBtn = 0;
        m_stMenuSticky = false;
        return;
    }
    if (m_pressed && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && m_pressBrick >= 0 && unitGroup(m_pressBrick) >= 0) {
        m_pressed = false;   // double click on a group: inside it
        enterGroup(m_pressBrick);
        return;
    }
    if (m_pressed && (m_mode == 0 || m_mode == 1) && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !m_dragStarted && m_pressBrick >= 0 &&
        unitOf(m_pressBrick).size() > 1 && (std::fabs(mp.x - m_pressPos.x) + std::fabs(mp.y - m_pressPos.y)) > 6) {
        m_pressed = false;   // drag a group: the whole group moves as one block
        fitGroupContext(m_pressBrick);
        select(m_pressBrick);
        beginGroupDrag(unitOf(m_pressBrick), mp.x, mp.y);
        return;
    }
    if (m_pressed && m_mode == 1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !m_dragStarted && m_pressBrick >= 0 &&
        (std::fabs(mp.x - m_pressPos.x) + std::fabs(mp.y - m_pressPos.y)) > 6) {
        m_dragStarted = true;
        m_pressed = false;
        beginMove(m_pressBrick, mp.x, mp.y);   // drag a placed brick: it comes into the hand
    }
    if (m_pressed && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_pressed = false;
        const bool moved = (std::fabs(mp.x - m_pressPos.x) + std::fabs(mp.y - m_pressPos.y)) > 6;
        if (!(m_leftOrbit && moved)) select(m_pressBrick, io.KeyShift || io.KeyCtrl);
    }
    // the brick dragged into the hand drops where the button is released
    if (m_holding && m_dragStarted && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_dragStarted = false;
        dropHeld();
    }
}

} // namespace brickit
