// ------------------------------------------------------------------ Crème: the kit of the iPad, updated (Wzx 22/11/2026)
// The interface of the theme Crème is the cream touch kit (BrickUiIpad.inl), on the PC and the iPad. Its drawer takes the
// families and the categories of the library of today (stFams of BrickUiStudio.inl, as Brique and Classic).

int BrickApp::crFamCount() { return (int)stFams().size(); }
std::string BrickApp::crFamKey(int i) { return stFams()[std::clamp(i, 0, crFamCount() - 1)].key; }
const char* BrickApp::crFamLabel(int i) { return stFams()[std::clamp(i, 0, crFamCount() - 1)].label; }
int BrickApp::crFamFirst(int i) { return stFamFirst(crFamKey(i)); }

static const StFam* crFam(const std::string& key) {
    for (const StFam& f : stFams()) if (key == f.key) return &f;
    return nullptr;
}
int BrickApp::crSubCount() { const StFam* f = crFam(m_stFam); return f ? (int)f->subs.size() : 0; }
const char* BrickApp::crSubLabel(int j) { const StFam* f = crFam(m_stFam); return f && j >= 0 && j < (int)f->subs.size() ? f->subs[j].label : ""; }

// the parts of the family / category chosen (cached; the same rules as the drawer of the Studio interface)
void BrickApp::crLibParts() {
    const StFam* fam = crFam(m_stFam);
    if (!fam) { m_stFam = "basique"; m_stSub = -1; fam = crFam(m_stFam); }
    if (!fam) return;
    if (m_stSub >= (int)fam->subs.size()) m_stSub = -1;
    const StSub* sub = !m_searchOpen && m_stSub >= 0 ? &fam->subs[m_stSub] : nullptr;
    int avail = 0;
    for (const Part& p : m_lib.parts) avail += p.available() ? 1 : 0;
    const std::string key = "cr|" + m_stFam + "|" + std::to_string(m_stSub) + "|" + (m_searchOpen ? m_search : std::string()) + "|" + std::to_string(avail) + "|" +
                            std::to_string(m_recent.size()) + "|" + std::to_string(m_fav.size()) + (m_hideDeco ? "|d" : "");
    if (key == m_stKey) return;
    m_stKey = key;
    m_stParts.clear();
    if (m_searchOpen) m_stParts = visibleParts();
    else if (m_stFam == "recent") m_stParts = m_recent;
    else if (m_stFam == "fav") m_stParts = m_fav;
    else
        for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
            const Part& p = m_lib.parts[i];
            if (!p.available() || (m_hideDeco && partDeco(p)) || partDuplo(p)) continue;
            const std::string& c = partCat(i);
            bool in = false;
            if (sub) in = stInSub(*sub, c, p.sub);
            else for (const StSub& q : fam->subs) if (stInSub(q, c, p.sub)) { in = true; break; }
            if (in) m_stParts.push_back(i);
        }
}

// ================================================================== Crème A (maquette « Crème A », Wzx 22/11/2026)
// Everything of the kit stays (pebbles, brown bubbles, the pill, the yellow dock); added: the modes in a cream pebble
// beside Maison, the tools in pebbles on the left, the angle over Tourner, the Scène pebble (its groups) top right, and
// the hub: the Maison menu and « Nouvelle scène » grown into one cream sheet (the scenes of the iPad, the cloud, the demos).

// (the colours: kitPalette, BrickUiIpad.inl)

// a small segmented pill of the maquette (.pill2: 12 px, hollow, the chosen one cream with a soft shadow); top right given
static int crPill(Ui& u, ImVec2 topRight, float s, const std::vector<const char*>& opts, int cur) {
    const float px = 12 * s, pad = 3 * s, h = 28 * s;
    float W = 2 * pad;
    std::vector<float> ws;
    for (const char* o : opts) { ws.push_back(kSize(px, o, true).x + 20 * s); W += ws.back() + 2 * s; }
    W -= 2 * s;
    const ImVec2 a(topRight.x - W, topRight.y), b(topRight.x, topRight.y + h);
    u.dl->AddRectFilled(a, b, CR_HOLLOW, h * 0.5f);
    int hit = -1;
    float x = a.x + pad;
    for (int k = 0; k < (int)opts.size(); ++k) {
        const ImVec2 oa(x, a.y + pad), ob(x + ws[k], b.y - pad);
        if (k == cur) {
            u.dl->AddRectFilled(ImVec2(oa.x, oa.y + 1 * s), ImVec2(ob.x, ob.y + 1 * s), T_OMBRE_DOUCE, (ob.y - oa.y) * 0.5f);
            u.dl->AddRectFilled(oa, ob, T_BLANC, (ob.y - oa.y) * 0.5f);
        }
        kTextC(u.dl, oa.x + 10 * s, (oa.y + ob.y) * 0.5f, px, k == cur ? CR_INK : CR_INK2, opts[k], k == cur);
        if (u.pressed && u.hover(oa, ob)) hit = k;
        x += ws[k] + 2 * s;
    }
    return hit;
}

// ------------------------------------------------------------------ the modes, beside Maison

void BrickApp::ipadModes() {
    g_crModesR = 0;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const bool notice = m_reading || m_pManual.visible;
    struct M { const char* icon; const char* label; bool on; int id; };
    const M ms[4] = {{"brique", "Construire", !m_mechOn && !m_paintMode && !notice, 0}, {"pinceau", "Peindre", m_paintMode && !m_mechOn, 1},
                     {"mecanique", "Mécanique", m_mechOn, 2}, {"notice", "Notice", notice && !m_mechOn, 3}};
    const bool wide = d.x >= 1300 * s;   // (an iPad 11": the labels of the modes not chosen hidden, the scene pill keeps its room)
    const float H = 46 * s, pad = 5 * s, segH = 36 * s, px = 14 * s, ic = 18 * s, cy = 20 * s + 26 * s;
    float ws[4], W = 2 * pad;
    for (int k = 0; k < 4; ++k) { ws[k] = (ms[k].on || wide) ? 14 * s + ic + 8 * s + kSize(px, ms[k].label, true).x + 14 * s : 44 * s; W += ws[k] + 2 * s; }
    W -= 2 * s;
    const ImVec2 a(22 * s + 52 * s + 12 * s, cy - H * 0.5f), b(a.x + W, cy + H * 0.5f);
    kitShadowRect(u.dl, a, b, T_OMBRE, 4 * s, 14 * s);
    u.dl->AddRectFilled(a, b, T_PILULE, H * 0.5f);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    g_crModesR = b.x;
    float x = a.x + pad;
    for (int k = 0; k < 4; ++k) {
        const M& m = ms[k];
        const ImVec2 sa(x, cy - segH * 0.5f), sb(x + ws[k], cy + segH * 0.5f);
        const bool h = u.hover(sa, sb), down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (m.on) {
            u.dl->AddRectFilled(ImVec2(sa.x, sa.y + 2 * s), ImVec2(sb.x, sb.y + 2 * s), T_OMBRE_DOUCE, segH * 0.5f);
            u.dl->AddRectFilled(sa, sb, T_BLANC, segH * 0.5f);
        } else if (down) u.dl->AddRectFilled(sa, sb, T_CREUX, segH * 0.5f);
        const ImU32 ink = m.on ? CR_INK : CR_INK2;
        if (m.on || wide) {
            kitIcon(u.dl, m.icon, ImVec2(sa.x + 14 * s + ic * 0.5f, cy), ic, ink);
            kTextC(u.dl, sa.x + 14 * s + ic + 8 * s, cy, px, ink, m.label, true);
        } else kitIcon(u.dl, m.icon, ImVec2((sa.x + sb.x) * 0.5f, cy), ic + 2 * s, ink);
        if (h && u.pressed) {
            u.pressed = false;
            const bool wasMech = m_mechOn;
            if (m.id != 2 && m_mechOn) mechExit();
            if (m.id != 3) { if (m_reading) stopReading(); m_pManual.visible = false; }
            switch (m.id) {
            case 0: m_paintMode = false; m_paintTool = false; m_pipette = false; break;
            case 1: m_paintMode = true; m_paintTool = true; m_pipette = false; m_selected = -1; m_multi.clear(); syncObjects(); break;
            case 2: m_paintMode = false; m_paintTool = false; m_pipette = false; if (wasMech) mechExit(); else mechEnter(); break;
            case 3:
                m_paintMode = false; m_paintTool = false; m_pipette = false;
                if (notice) { if (m_reading) stopReading(); m_pManual.visible = false; }
                else if (hasSteps()) startReading(1);
                else { m_pManual.visible = true; m_pManual.collapsed = false; }
                break;
            }
            m_crScene = false;
            m_paletteOpen = false;
        }
        x += ws[k] + 2 * s;
    }
}

// ------------------------------------------------------------------ the tools, in pebbles on the left

void BrickApp::ipadTools() {
    if (m_reading || m_mechOn || m_pManual.visible || m_holding) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    {   // Recadrer and Dupliquer on the left (Wzx 22/11/2026), under the tools; the drawer open: just above it
        const float D = 44 * s, gap = 10 * s;
        const float drawerTop = m_libOpen ? d.y - std::round(std::max(200 * s, d.y * std::clamp(m_kitLibFrac, 0.2f, 0.8f))) : d.y;
        const float y1 = m_libOpen ? drawerTop - 18 * s - D * 2 - gap : d.y * 0.47f + (m_paintMode ? 3 : 5) * (D + gap) * 0.5f + 22 * s;
        const bool sel = !selection().empty();
        if (y1 > 84 * s) {
            if (kitGalet(u, ImVec2(22 * s + D * 0.5f, y1 + D * 0.5f), D, "recadrer", 0)) { u.pressed = false; crFrame(); }
            if (kitGalet(u, ImVec2(22 * s + D * 0.5f, y1 + D + gap + D * 0.5f), D, "dupliquer", sel ? 0 : 2)) { u.pressed = false; duplicateSelected(); }
        }
    }
    if (m_libOpen) return;
    struct T { const char* icon; bool on; int id; };
    std::vector<T> ts;
    if (m_paintMode) ts = {{"pinceau", m_paintTool && !m_pipette, 10}, {"pipette", m_pipette, 11}, {"grouper", m_paintGroup, 12}};
    else ts = {{m_mode == 0 ? "lasso" : "selection", (m_mode == 0 || m_mode == 1) && !m_pipette, 0}, {"deplacer", m_mode == 2 && !m_pipette, 1},
               {"tourner", m_mode == 3 && !m_pipette, 2}, {"pipette", m_pipette, 3}, {"isoler", m_focusDim, 4}};
    const float D = 44 * s, gap = 10 * s, total = ts.size() * D + (ts.size() - 1) * gap;
    float y = std::max(84 * s, d.y * 0.47f - total * 0.5f);
    for (const T& t : ts) {
        if (kitGalet(u, ImVec2(22 * s + D * 0.5f, y + D * 0.5f), D, t.icon, t.on ? 1 : 0)) {
            u.pressed = false;
            switch (t.id) {
            case 0:
                if (m_mode == 1 && !m_pipette) { m_mode = 0; toast("Sélection au cadre : glisse sur le vide pour entourer"); }
                else m_mode = 1;
                m_pipette = false;
                break;
            case 1: m_mode = m_mode == 2 ? 1 : 2; m_pipette = false; break;
            case 2: m_mode = m_mode == 3 ? 1 : 3; m_pipette = false; break;
            case 3: m_pipette = !m_pipette; break;
            case 4: m_focusDim = !m_focusDim; syncObjects(); if (m_focusDim && selection().empty()) toast("Isoler : choisis une pièce, le reste s'efface"); break;
            case 10: m_paintTool = true; m_pipette = false; break;
            case 11: m_pipette = !m_pipette; m_paintTool = !m_pipette; break;
            case 12: m_paintGroup = !m_paintGroup; toast(m_paintGroup ? "Peindre : tout le groupe de la brique" : "Peindre : la brique seule"); break;
            }
            savePrefs();
        }
        y += D + gap;
    }
}

// ------------------------------------------------------------------ the angle, over the Tourner bubble

bool BrickApp::ipadAngles(ImVec2 above) {
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    struct O { const char* label; int step; };
    const O os[5] = {{"15°", 15}, {"45°", 45}, {"90°", 90}, {"Libre", 0}, {"Remettre droit", -1}};
    const float px = 12 * s, pad = 3 * s, h = 30 * s;
    float ws[5], W = 2 * pad;
    for (int k = 0; k < 5; ++k) { ws[k] = kSize(px, os[k].label, true).x + 18 * s; W += ws[k] + 2 * s; }
    W -= 2 * s;
    ImVec2 a(above.x - W * 0.5f, above.y - 23 * s - 10 * s - h);
    a.x = std::clamp(a.x, 80 * s, std::max(80 * s, d.x - 16 * s - W));
    a.y = std::max(a.y, 80 * s);
    const ImVec2 b(a.x + W, a.y + h);
    kitShadowRect(u.dl, a, b, T_OMBRE, 3 * s, 10 * s);
    u.dl->AddRectFilled(a, b, T_PILULE, h * 0.5f);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    bool hit = false;
    float x = a.x + pad;
    for (int k = 0; k < 5; ++k) {
        const ImVec2 oa(x, a.y + pad), ob(x + ws[k], b.y - pad);
        const bool on = os[k].step == m_rotStep, hv = u.hover(oa, ob);
        if (on) u.dl->AddRectFilled(oa, ob, T_PUCE, (ob.y - oa.y) * 0.5f);
        else if (hv && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(oa, ob, T_CREUX, (ob.y - oa.y) * 0.5f);
        kTextC(u.dl, oa.x + 9 * s, (oa.y + ob.y) * 0.5f, px, on ? T_SURPUCE : os[k].step < 0 ? CR_LINK : T_BRUN, os[k].label, true);
        if (hv && u.pressed && !hit) {
            hit = true;
            if (os[k].step < 0) resetRotation();
            else { m_rotStep = os[k].step; savePrefs(); }
        }
        x += ws[k] + 2 * s;
    }
    return hit;
}

// the Tourner bubble: by the angle chosen (90°: the quarter turn on the studs; 15° / 45° / Libre: free turns of 15°)
void BrickApp::crTurn(int dir) {
    if (m_holding || m_rotStep == 90) { rotateSelected(dir); return; }
    const int n = m_rotStep == 45 ? 3 : 1;
    for (int k = 0; k < n; ++k) turnSelectedFree(dir);
}

// ------------------------------------------------------------------ the ring of colours (a long press on a brick)

// a swatch: a flat disc of the colour, a light shadow, a thin darker rim (the transparent ones lighter, the metallic ones
// with a thin light ring); the current one ringed in white then ink
void BrickApp::crSwatch(ImDrawList* dl, ImVec2 p, float r, int color, bool current, float alpha) {
    const float s = m_ui;
    const Color& col = m_lib.palette[std::clamp(color, 0, (int)m_lib.palette.size() - 1)];
    const bool trans = col.group == "transparent", metal = col.group == "metallic" || col.group == "metal" || col.group == "chrome" || col.group == "pearl";
    const float a = std::clamp(alpha, 0.0f, 1.0f);
    kitShadowCircle(dl, p, r * 2, alphaOf(IM_COL32(40, 25, 10, 70), a), 1.5f * s);
    if (current) {
        dl->AddCircleFilled(p, r + 5 * s, alphaOf(IM_COL32(255, 255, 255, 255), a), 48);
        dl->AddCircle(p, r + 5 * s, alphaOf(T_BRUN, a), 48, 1.5f * s);
    }
    const ImU32 base = kitCol(col, (trans ? 0.78f : 1.0f) * a);
    dl->AddCircleFilled(p, r, base, 48);
    const float lum = col.srgb.x * 0.3f + col.srgb.y * 0.59f + col.srgb.z * 0.11f;
    dl->AddCircle(p, r - 0.5f * s, alphaOf(lum > 0.85f ? IM_COL32(150, 140, 125, 255) : IM_COL32(0, 0, 0, 70), a), 48, 1.0f * s);
    if (metal) dl->AddCircle(p, r * 0.62f, alphaOf(IM_COL32(255, 255, 255, 90), a), 40, 1.2f * s);   // (flat: a metallic one, a thin light ring; Wzx 22/11/2026: « plus cercle plat, moins sphère »)
}
// Wzx 22/11/2026: « quand je maintiens sur une brique, apparition d'un cercle avec toutes les couleurs tout autour, avec
// la même animation d'apparition »: every colour of the Nuancier in rings around the finger, the studs appearing one
// after the other (0.85 -> 1, ease-out); the finger slid onto a colour and lifted paints the brick (or its selection);
// lifted at once, the ring stays and a tap picks; a tap elsewhere closes it.
void BrickApp::ipadColorRing() {
    if (!m_crRing) return;
    if (selection().empty()) { m_crRing = false; return; }
    kitColors();
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const double now = ImGui::GetTime();
    std::vector<int> cols;
    for (const KitColor& kc : m_kitColorList) cols.push_back(kc.index);
    if (cols.empty()) for (int i = 0; i < (int)m_lib.palette.size() && i < 60; ++i) cols.push_back(i);
    // rings of swatches 34 pt, 48 pt apart along the ring, 56 pt between the rings, from 150 pt (Wzx 22/11/2026: « met les
    // ring plus loin », no white disc under them, simple round swatches instead of the studs)
    struct At { ImVec2 p; int col; int order; };
    std::vector<At> at;
    float R = 150 * s;
    const float stud = 34 * s, step = 48 * s, ringGap = 62 * s;
    size_t k = 0;
    int ring = 0;
    while (k < cols.size()) {
        const int n = std::min(std::max(6, (int)(2 * 3.14159265f * R / step)), (int)(cols.size() - k));   // (the last ring: its swatches spread all around)
        for (int i = 0; i < n && k < cols.size(); ++i, ++k) {
            const float th = -3.14159265f * 0.5f + 2 * 3.14159265f * (i + (ring % 2) * 0.5f) / n;
            at.push_back({ImVec2(std::cos(th) * R, std::sin(th) * R), cols[k], (int)k});
        }
        R += ringGap;
        ++ring;
    }
    const float outer = R - ringGap + stud * 0.5f + 6 * s;
    const ImVec2 c(std::clamp(m_crRingAt.x, std::min(outer, d.x * 0.5f), std::max(d.x - outer, d.x * 0.5f)),
                   std::clamp(m_crRingAt.y, std::min(outer + 60 * s, d.y * 0.5f), std::max(d.y - outer - 20 * s, d.y * 0.5f)));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const int cur = m_selected >= 0 && m_selected < (int)m_bricks.size() ? m_bricks[m_selected].color : m_color;
    int hot = -1;
    for (const At& a : at) {
        const ImVec2 p(c.x + a.p.x, c.y + a.p.y);
        if (u.hover(ImVec2(p.x - stud * 0.55f, p.y - stud * 0.55f), ImVec2(p.x + stud * 0.55f, p.y + stud * 0.55f))) hot = a.col;
    }
    for (const At& a : at) {
        const ImVec2 p(c.x + a.p.x, c.y + a.p.y);
        const float e = std::clamp((float)(now - m_crRingT0 - a.order * 0.008) / 0.16f, 0.0f, 1.0f);
        if (e <= 0) continue;   // (one after the other)
        const float sc = (0.85f + 0.15f * (1.0f - (1.0f - e) * (1.0f - e) * (1.0f - e))) * (a.col == hot ? 1.22f : 1.0f);
        crSwatch(u.dl, p, stud * sc * 0.5f, a.col, a.col == cur, e);
    }
    kitLabel(u.dl, c, colorNameFr(hot >= 0 ? hot : cur));
    const bool down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    auto pick = [&](int col) {
        m_color = col;
        applyColor(col);
        m_crRing = false;
    };
    if (now - m_crRingT0 < 0.12) {}   // (the finger reaches the mouse of ImGui a frame later)
    else if (!m_crRingSticky) {   // the finger still down since the long press: lifted on a colour = that colour
        if (!down) {
            if (hot >= 0) pick(hot);
            else m_crRingSticky = true;   // (lifted elsewhere: the ring stays, a tap picks)
        }
    } else if (u.released && hot >= 0) pick(hot);
    else if (u.pressed && hot < 0) m_crRing = false;
}

// ------------------------------------------------------------------ Scène: the groups of the scene (top right)

void BrickApp::ipadScenePanel() {
    if (!m_crScene || m_libOpen) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const int G = (int)m_build.groups.size();
    std::vector<int> parent(G, -2), count(G, 0);
    int loose = 0;
    for (const Brick& br : m_bricks) {
        if (br.path.empty()) ++loose;
        for (size_t k = 0; k < br.path.size(); ++k) {
            const int g = br.path[k];
            if (g < 0 || g >= G) continue;
            ++count[g];
            if (parent[g] == -2) parent[g] = k ? br.path[k - 1] : -1;
        }
    }
    struct Row { int g, depth; };
    std::vector<Row> rows;
    std::function<void(int, int)> add = [&](int par, int depth) {
        for (int g = 0; g < G; ++g) if (parent[g] == par && count[g] > 0) { rows.push_back({g, depth}); add(g, depth + 1); }
    };
    add(-1, 0);
    const std::vector<int> sel = selection();
    const float W = 340 * s, pad = 14 * s, rowH = 48 * s;
    const float headH = 38 * s + 8 * s + 18 * s + 10 * s;
    const float footH = 8 * s + (sel.size() > 1 ? 50 * s : 0) + 50 * s + (m_isolateGroup >= 0 ? 50 * s : 0);
    const float maxH = d.y - 84 * s - 120 * s;
    const float listH = std::min(std::max(1, (int)rows.size()) * rowH, std::max(rowH, maxH - headH - footH - 2 * pad));
    const ImVec2 a(d.x - 22 * s - W, 84 * s), b(a.x + W, a.y + pad + headH + listH + footH + pad);
    kitSheet(u.dl, a, b);
    float y = a.y + pad;
    kTextC(u.dl, a.x + pad + 6 * s, y + 19 * s, 20 * s, T_BRUN, "Scène", true);
    if (kitGalet(u, ImVec2(b.x - pad - 19 * s, y + 19 * s), 38 * s, "fermer", 3)) { m_crScene = false; return; }
    y += 38 * s + 8 * s;
    char info[128];
    snprintf(info, sizeof(info), "%d pièces · %d groupe%s · %d hors groupe", (int)m_bricks.size(), (int)rows.size(), rows.size() > 1 ? "s" : "", loose);
    kTextC(u.dl, a.x + pad + 6 * s, y + 9 * s, 13 * s, T_BRUN_PALE, info);
    y += 18 * s + 10 * s;
    const ImVec2 la(a.x, y), lb(b.x, y + listH);
    if (u.hover(la, lb) && ImGui::GetIO().MouseWheel != 0) m_crSceneScroll -= ImGui::GetIO().MouseWheel * rowH;
    m_crSceneScroll = std::clamp(m_crSceneScroll, 0.0f, std::max(0.0f, rows.size() * rowH - listH));
    u.dl->PushClipRect(la, lb, true);
    const int selG = unitGroup(m_selected);
    if (rows.empty()) {
        u.dl->PushClipRect(la, ImVec2(lb.x - pad, lb.y), true);
        kTextC(u.dl, a.x + pad + 6 * s, y + rowH * 0.5f, 14 * s, T_BRUN_PALE, "Aucun groupe : des pièces choisies, « … » › Grouper");
        u.dl->PopClipRect();
    }
    for (size_t r = 0; r < rows.size(); ++r) {
        const int g = rows[r].g;
        Group& gr = m_build.groups[g];
        const ImVec2 ra(a.x + pad, y + r * rowH - m_crSceneScroll), rb(b.x - pad, ra.y + rowH - 4 * s);
        if (rb.y < la.y || ra.y > lb.y) continue;
        const bool h = u.hover(ra, rb) && u.hover(la, lb);
        if (g == selG) u.dl->AddRectFilled(ra, rb, T_BLANC, 14 * s);
        else if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(ra, rb, T_CREUX, 14 * s);
        const float cy = (ra.y + rb.y) * 0.5f, x0 = ra.x + 10 * s + rows[r].depth * 18 * s;
        kitIcon(u.dl, "grouper", ImVec2(x0 + 11 * s, cy), 22 * s, gr.hidden ? T_INACTIF : T_BRUN);
        // right: isolate (a target), shown / hidden (the eye), the count
        const ImVec2 ic(rb.x - 20 * s, cy), ec(rb.x - 56 * s, cy);
        const bool hi = u.hover(ImVec2(ic.x - 17 * s, ra.y), ImVec2(ic.x + 17 * s, rb.y)), he = u.hover(ImVec2(ec.x - 17 * s, ra.y), ImVec2(ec.x + 17 * s, rb.y));
        u.dl->AddCircle(ic, 8 * s, m_isolateGroup == g ? CR_INK : T_BRUN_PALE, 24, 1.8f * s);
        if (m_isolateGroup == g) u.dl->AddCircleFilled(ic, 3.5f * s, CR_INK, 16);
        kitIcon(u.dl, "isoler", ec, 22 * s, gr.hidden ? T_INACTIF : T_BRUN_DOUX);
        if (gr.hidden) u.dl->AddLine(ImVec2(ec.x - 9 * s, ec.y + 7 * s), ImVec2(ec.x + 9 * s, ec.y - 7 * s), T_BRUN_DOUX, 2 * s);
        const std::string cn = std::to_string(count[g]);
        const float cw = kSize(13 * s, cn.c_str()).x;
        kTextC(u.dl, ec.x - 22 * s - cw, cy, 13 * s, T_BRUN_PALE, cn.c_str());
        u.dl->PushClipRect(ImVec2(x0, ra.y), ImVec2(ec.x - 30 * s - cw, rb.y), true);
        kTextC(u.dl, x0 + 22 * s + 12 * s, cy, 16 * s, gr.hidden ? T_INACTIF : T_BRUN, gr.name.c_str(), g == selG);
        u.dl->PopClipRect();
        if (h && u.pressed) {
            u.pressed = false;
            if (he) { gr.hidden = !gr.hidden; syncObjects(); autosave(); }
            else if (hi) { m_isolateGroup = m_isolateGroup == g ? -1 : g; syncObjects(); }
            else {
                const std::vector<int> m = groupMembers(g);
                if (!m.empty()) {
                    const std::vector<int>& p = m_bricks[m[0]].path;
                    m_groupCtx.assign(p.begin(), std::find(p.begin(), p.end(), g));
                    m_selected = m[0];
                    m_multi.assign(m.begin() + 1, m.end());
                    syncObjects();
                }
            }
        }
    }
    u.dl->PopClipRect();
    y += listH;
    u.dl->AddLine(ImVec2(a.x + 26 * s, y + 4 * s), ImVec2(b.x - 18 * s, y + 4 * s), T_SEP, 1.0f * s);
    y += 8 * s;
    if (sel.size() > 1) { if (kitRow(u, ImVec2(a.x + pad, y), W - 2 * pad, "grouper", "Grouper la sélection", nullptr)) { u.pressed = false; groupSelected(); } y += 50 * s; }
    if (m_isolateGroup >= 0) { if (kitRow(u, ImVec2(a.x + pad, y), W - 2 * pad, "isoler", "Tout afficher", nullptr)) { u.pressed = false; m_isolateGroup = -1; syncObjects(); } y += 50 * s; }
    if (kitRow(u, ImVec2(a.x + pad, y), W - 2 * pad, "presets", "Presets", "groupes enregistrés")) { u.pressed = false; m_crPresets = true; m_crScene = false; m_presetsKey.clear(); return; }
}

// ------------------------------------------------------------------ Presets: the groups saved, placed in the scene

void BrickApp::ipadPresets() {
    if (!m_crPresets) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    if (m_presetsKey.empty()) { m_presets = listPresets(); m_presetsKey = "ok"; }
    const int n = (int)m_presets.size();
    const float W = 340 * s, pad = 14 * s, rowH = 50 * s;
    const int shown = std::max(1, std::min(n, (int)((d.y - 84 * s - 200 * s) / rowH)));
    const ImVec2 a(d.x - 22 * s - W, 84 * s), b(a.x + W, a.y + pad + 38 * s + 8 * s + shown * rowH + 34 * s + pad);
    kitSheet(u.dl, a, b);
    float y = a.y + pad;
    kTextC(u.dl, a.x + pad + 6 * s, y + 19 * s, 20 * s, T_BRUN, "Presets", true);
    if (kitGalet(u, ImVec2(b.x - pad - 19 * s, y + 19 * s), 38 * s, "fermer", 3)) { m_crPresets = false; return; }
    y += 38 * s + 8 * s;
    const ImVec2 la(a.x, y), lb(b.x, y + shown * rowH);
    if (n == 0) {
        u.dl->PushClipRect(la, ImVec2(lb.x - pad, lb.y), true);
        kTextC(u.dl, a.x + pad + 6 * s, y + rowH * 0.5f, 14 * s, T_BRUN_PALE, "Aucun preset : un groupe choisi, son bouton Preset");
        u.dl->PopClipRect();
    } else {
        if (u.hover(la, lb) && ImGui::GetIO().MouseWheel != 0) m_presetsScroll -= ImGui::GetIO().MouseWheel * rowH;
        m_presetsScroll = std::clamp(m_presetsScroll, 0.0f, std::max(0.0f, n * rowH - shown * rowH));
        u.dl->PushClipRect(la, lb, true);
        for (int i = 0; i < n; ++i) {
            const ImVec2 ra(a.x + pad, y + i * rowH - m_presetsScroll);
            if (ra.y + rowH < la.y || ra.y > lb.y) continue;
            if (kitRow(u, ra, W - 2 * pad, "presets", m_presets[i].first.c_str(), nullptr) && u.hover(la, lb)) {
                u.pressed = false;
                m_crPresets = false;
                placePreset(m_presets[i].second, m_presets[i].first);
                break;
            }
        }
        u.dl->PopClipRect();
    }
    const std::string dir = "Dossier : " + plat::toUtf8(presetsDir());
    u.dl->PushClipRect(ImVec2(a.x, b.y - pad - 26 * s), ImVec2(b.x - pad, b.y), true);
    kTextC(u.dl, a.x + pad + 6 * s, b.y - pad - 10 * s, 12 * s, T_BRUN_PALE, dir.c_str());
    u.dl->PopClipRect();
}

// ------------------------------------------------------------------ the hub: « Mes scènes » (Maison, or the scene pill)

void BrickApp::ipadHub() {
    if (!m_homeOpen && !m_newSceneOpen) { m_crCardMenu = -1; m_crHubRename = -1; return; }
    if (m_hubStale) hubRefresh();
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = u.dl;
    bool close = false;
    // the veil over the scene (the maquette blurs it), the sheet
    dl->AddRectFilled(ImVec2(0, 0), d, T_VOILE_HUB);
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const float W = std::min(d.x - 2 * 84 * s, 1400 * s);
    const ImVec2 a(std::max(84 * s, std::floor((d.x - W) * 0.5f)), 50 * s), b(a.x + W, d.y - 40 * s);
    kitShadowRect(dl, a, b, T_OMBRE_FEUILLE, 18 * s, 40 * s);
    dl->AddRectFilled(a, b, T_FEUILLE, 30 * s);
    const float padX = 32 * s, padY = 30 * s, colW = 300 * s;
    const float cx0 = b.x - padX - colW, L0 = a.x + padX, L1 = cx0 - 28 * s;
    const std::wstring dir = scenesDir();
    const bool inLib = !m_scenePath.empty() && m_scenePath.size() > dir.size() && m_scenePath.compare(0, dir.size(), dir) == 0;
    auto keepOpen = [&] {
        if (m_bricks.empty()) return;
        if (inLib) saveScene(m_scenePath);
        else if (m_scenePath.empty()) hubSaveHere();
    };
    const bool menuOpen = m_crCardMenu >= 0;
    Ui ub = u;   // (the sheet under the « … » menu sees no pointer)
    if (menuOpen) { ub.pressed = false; ub.released = false; ub.mouse = ImVec2(-1e9f, -1e9f); }
    // ======== the header: Mes scènes, the tabs, the search
    float y = a.y + padY;
    kTextC(dl, L0, y + 20 * s, 28 * s, CR_INK, "Mes scènes", true);
    float x = L0 + kSize(28 * s, "Mes scènes", true).x + 16 * s;
    {
        const std::string t0 = std::string(m_ipad ? "Sur l'iPad" : "Sur ce PC") + " · " + std::to_string(m_hubLocal.size());
        const std::string t1 = m_cloud.signedIn() ? "Cloud · " + std::to_string(m_cloudList.size()) : std::string("Cloud");
        const char* labs[3] = {t0.c_str(), t1.c_str(), "Démos"};
        const int secOf[3] = {0, 2, 3};
        const float th = 34 * s, pp = 4 * s;
        float w = 2 * pp;
        float tw[3];
        for (int k = 0; k < 3; ++k) { tw[k] = kSize(14 * s, labs[k], true).x + 32 * s; w += tw[k] + 2 * s; }
        w -= 2 * s;
        const ImVec2 pa(x, y + 20 * s - th * 0.5f - pp), pb(x + w, y + 20 * s + th * 0.5f + pp);
        dl->AddRectFilled(pa, pb, CR_HOLLOW, (pb.y - pa.y) * 0.5f);
        float tx = pa.x + pp;
        for (int k = 0; k < 3; ++k) {
            const ImVec2 qa(tx, pa.y + pp), qb(tx + tw[k], pb.y - pp);
            const bool on = m_hubSection == secOf[k] || (k == 0 && m_hubSection == 1);
            if (on) {
                dl->AddRectFilled(ImVec2(qa.x, qa.y + 2 * s), ImVec2(qb.x, qb.y + 2 * s), T_OMBRE_DOUCE, th * 0.5f);
                dl->AddRectFilled(qa, qb, T_BLANC, th * 0.5f);
            }
            kTextC(dl, qa.x + 16 * s, (qa.y + qb.y) * 0.5f, 14 * s, on ? CR_INK : CR_INK2, labs[k], true);
            if (ub.hover(qa, qb) && ub.pressed) { m_hubSection = secOf[k]; m_crHubScroll = 0; if (secOf[k] == 2) cloudRefreshList(); else m_hubStale = true; }
            tx += tw[k] + 2 * s;
        }
        x = pb.x + 16 * s;
    }
    {   // the search: a cream pill on the right of the scenes
        const float sw = std::min(240 * s, L1 - x);
        if (sw > 120 * s) {
            const ImVec2 sa(L1 - sw, y + 20 * s - 19 * s), sb(L1, y + 20 * s + 19 * s);
            dl->AddRectFilled(sa, sb, T_BLANC, 19 * s);
            dl->AddRect(sa, sb, CR_LINE, 19 * s, 0, 1.0f * s);
            kitIcon(dl, "chercher", ImVec2(sa.x + 22 * s, (sa.y + sb.y) * 0.5f), 18 * s, CR_PALE);
            if (!menuOpen) {
                ImGui::SetNextWindowPos(ImVec2(sa.x + 38 * s, sa.y + 2 * s));
                ImGui::SetNextWindowSize(ImVec2(sb.x - sa.x - 50 * s, sb.y - sa.y - 4 * s));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2 * s, 8 * s));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(T_BRUN));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(CR_PALE));
                ImGui::Begin("##crhubsearch", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
                ImGui::PushFont(kF(false), 14 * s * kEm);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputTextWithHint("##q", "Chercher une scène", m_crSearch, sizeof(m_crSearch))) m_crHubScroll = 0;
                ImGui::PopFont();
                ImGui::End();
                ImGui::PopStyleColor(5);
                ImGui::PopStyleVar(3);
            } else kTextC(dl, sa.x + 38 * s, (sa.y + sb.y) * 0.5f, 14 * s, m_crSearch[0] ? T_BRUN : CR_PALE, m_crSearch[0] ? m_crSearch : "Chercher une scène");
        }
    }
    y += 40 * s + 20 * s;
    // ======== the scrolled column: NOUVELLE SCÈNE, REPRENDRE
    const ImVec2 ga(L0 - 8 * s, y), gb(L1 + 8 * s, b.y - 18 * s);
    const float gap = 16 * s, Lw = L1 - L0;
    // the scenes of the tab (filtered by the search)
    struct Card { int kind; int i; };   // 0 local, 1 cloud, 2 demo, 3 the scene open (not in Mes scènes)
    std::vector<Card> cards;
    std::string q = m_crSearch;
    for (char& c : q) c = (char)std::tolower((unsigned char)c);
    auto match = [&](const std::string& n) {
        if (q.empty()) return true;
        std::string l = n;
        for (char& c : l) c = (char)std::tolower((unsigned char)c);
        return l.find(q) != std::string::npos;
    };
    if (m_hubSection <= 1) {
        if (!inLib && !m_bricks.empty() && match(m_modelName.empty() ? sceneTitle(m_scenePath) : m_modelName)) cards.push_back({3, 0});
        for (int i = 0; i < (int)m_hubLocal.size(); ++i) if (match(m_hubLocal[i].name)) cards.push_back({0, i});
    } else if (m_hubSection == 2) {
        if (m_cloud.signedIn()) for (int i = 0; i < (int)m_cloudList.size(); ++i) if (match(m_cloudList[i].name)) cards.push_back({1, i});
    } else {
        if (match("Maison (démo)")) cards.push_back({2, 0});
        if (match("Pelleteuse et mécanismes")) cards.push_back({2, 1});
    }
    const int cols = std::max(2, (int)((Lw + gap) / (140 * s + gap)));
    const float cw = std::floor((Lw - (cols - 1) * gap) / cols), thH = std::floor(std::min(110 * s, cw * 0.66f));
    const float cH = 10 * s + thH + 8 * s + 20 * s + 6 * s + 16 * s + 12 * s;
    const float ncW = std::floor((Lw - 2 * gap) / 3), ncImg = std::floor(std::min(120 * s, ncW * 0.55f));
    const float ncH = 10 * s + ncImg + 8 * s + 22 * s + 4 * s + 16 * s + 12 * s;
    const int rows = std::max(1, ((int)cards.size() + cols - 1) / cols);
    const float contentH = 16 * s + 12 * s + ncH + 24 * s + 16 * s + 12 * s + rows * (cH + gap) - gap + 10 * s;
    // scrolling: the wheel, or a finger dragged (a tap acts on release, when the finger did not move)
    const ImGuiIO& io = ImGui::GetIO();
    if (ub.hover(ga, gb) && io.MouseWheel != 0) m_crHubScroll -= io.MouseWheel * 80 * s;
    if (ub.pressed) { m_crHubPressY = ub.mouse.y; m_crHubDragging = false; }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !menuOpen && ub.hover(ga, gb) && std::fabs(ub.mouse.y - m_crHubPressY) > 8 * s) m_crHubDragging = true;
    if (m_crHubDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_crHubScroll -= io.MouseDelta.y;
    m_crHubScroll = std::clamp(m_crHubScroll, 0.0f, std::max(0.0f, contentH - (gb.y - ga.y)));
    const bool tap = ub.released && !m_crHubDragging && std::fabs(ub.mouse.y - m_crHubPressY) < 8 * s;
    const bool held = ImGui::IsMouseDown(ImGuiMouseButton_Left) && !m_crHubDragging;
    dl->PushClipRect(ga, gb, true);
    float cy = y - m_crHubScroll;
    kText(dl, ImVec2(L0, cy), 12 * s, CR_PALE, "NOUVELLE SCÈNE");
    cy += 16 * s + 12 * s;
    {   // the three cards: Plaque verte, Sol libre, Depuis une notice (their images by Recraft)
        struct NC { const char* img; ImU32 bg; const char* title; const char* sub; int act; float k; };
        const NC ncs[3] = {{"hub/new_plaque.png", T_VIGNETTE, "Plaque verte", "32 × 32 tenons, lumière du matin", 0, 1.45f},
                           {"hub/new_brique.png", T_CREUX, "Sol libre", "Rien sur la scène", 1, 0.72f},
                           {"hub/new_notice.png", IM_COL32(0x22, 0x1d, 0x1a, 255), "Depuis une notice", "Fichier .mpd officiel, étape par étape", 2, 1.1f}};
        for (int k = 0; k < 3; ++k) {
            const NC& c = ncs[k];
            const ImVec2 ca(L0 + k * (ncW + gap), cy), cb(ca.x + ncW, cy + ncH);
            const bool h = ub.hover(ca, cb) && ub.hover(ga, gb);
            kitShadowRect(dl, ca, cb, T_OMBRE_DOUCE, 2 * s, 12 * s);
            dl->AddRectFilled(ca, cb, h && held ? T_CREUX : CR_CARD, 18 * s);
            const ImVec2 ia(ca.x + 10 * s, ca.y + 10 * s), ib(cb.x - 10 * s, ca.y + 10 * s + ncImg);
            dl->AddRectFilled(ia, ib, c.bg, 12 * s);
            if (ImTextureID tex = kitTex(c.img)) {
                const float side = std::min(ib.x - ia.x, ib.y - ia.y) * c.k;
                const ImVec2 m((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f);
                dl->PushClipRect(ImVec2(std::max(ia.x, ga.x), std::max(ia.y, ga.y)), ImVec2(std::min(ib.x, gb.x), std::min(ib.y, gb.y)), true);
                dl->AddImage(tex, ImVec2(m.x - side * 0.5f, m.y - side * 0.5f), ImVec2(m.x + side * 0.5f, m.y + side * 0.5f));
                dl->PopClipRect();
            }
            kText(dl, ImVec2(ca.x + 12 * s, ib.y + 8 * s), 16 * s, CR_INK, c.title, true);
            dl->PushClipRect(ImVec2(ca.x, std::max(ib.y, ga.y)), ImVec2(cb.x - 8 * s, std::min(cb.y, gb.y)), true);
            kText(dl, ImVec2(ca.x + 12 * s, ib.y + 8 * s + 26 * s), 12 * s, CR_PALE, c.sub);
            dl->PopClipRect();
            if (h && tap) {
                keepOpen();
                if (c.act == 2) openDialog();
                else { newScene(c.act); m_modelName.clear(); }
                close = true;
            }
        }
        cy += ncH + 24 * s;
    }
    kText(dl, ImVec2(L0, cy), 12 * s, CR_PALE, m_hubSection == 3 ? "DÉMOS" : m_hubSection == 2 ? "DANS LE CLOUD" : "REPRENDRE");
    cy += 16 * s + 12 * s;
    if (cards.empty()) {
        const char* t = !q.empty() ? "Aucune scène pour cette recherche."
                      : m_hubSection <= 1 ? "Aucune scène enregistrée ici pour l'instant : tes scènes s'y rangent avec leur vignette."
                      : !m_cloud.signedIn() ? "Connecte-toi au cloud (Réglages › Cloud) pour retrouver tes scènes de l'iPad et du PC."
                      : m_cloudListBusy ? "Chargement…" : "Aucune scène dans le cloud pour l'instant.";
        dl->AddText(kF(false), 14 * s * kEm, ImVec2(L0, cy), CR_PALE, t, nullptr, Lw);
    }
    for (int k = 0; k < (int)cards.size(); ++k) {
        const Card cd = cards[k];
        const ImVec2 ca(L0 + (k % cols) * (cw + gap), cy + (k / cols) * (cH + gap)), cb(ca.x + cw, ca.y + cH);
        if (cb.y < ga.y || ca.y > gb.y) continue;
        const bool h = ub.hover(ca, cb) && ub.hover(ga, gb);
        std::string name, meta;
        ImTextureID tex = 0;
        float texAr = 1.6f;
        bool cur = false, live = false;
        ImU32 dot = CR_GREEN;
        if (cd.kind == 0) {
            const HubScene& sc = m_hubLocal[cd.i];
            name = sc.name; tex = hubThumb(sc.path); cur = sc.path == m_scenePath;
            std::string dt = hubDate(sc.mtime);   // « Aujourd'hui », « Hier », « 12/11/26 » (the card is narrow)
            if (dt.rfind("aujourd'hui", 0) == 0) dt = "Aujourd'hui";
            else if (dt.rfind("hier", 0) == 0) dt = "Hier";
            meta = cur ? dt + " · ouverte" : dt + " · " + std::to_string(sc.bricks) + " pièces";
            if (!m_cloud.signedIn()) dot = IM_COL32(0xc9, 0xb8, 0xa5, 255);
        } else if (cd.kind == 1) {
            const CloudScene& c = m_cloudList[cd.i];
            name = c.name.empty() ? std::string("Ma scène") : c.name; cur = c.id == m_cloudId;
            meta = Cloud::localDate(c.updated) + " · " + std::to_string(c.bricks) + " pièces";
        } else if (cd.kind == 2) {
            name = cd.i == 0 ? "Maison (démo)" : "Pelleteuse et mécanismes"; meta = cd.i == 0 ? "Le set 31153" : "4 moteurs, 2 vérins";
            tex = themeTex(cd.i == 0 ? "brique/hero.jpg" : "classic/banner.jpg"); texAr = cd.i == 0 ? 1.78f : 1.5f;
            dot = IM_COL32(0xc9, 0xb8, 0xa5, 255);
        } else {
            name = m_modelName.empty() ? sceneTitle(m_scenePath) : m_modelName; cur = true; live = true;
            meta = std::to_string(m_bricks.size()) + " pièces · ouverte";
            dot = IM_COL32(0xe8, 0xc0, 0x4a, 255);
        }
        kitShadowRect(dl, ca, cb, T_OMBRE_DOUCE, 2 * s, 12 * s);
        dl->AddRectFilled(ca, cb, h && held ? T_CREUX : CR_CARD, 18 * s);
        if (cur) dl->AddRect(ImVec2(ca.x - 1 * s, ca.y - 1 * s), ImVec2(cb.x + 1 * s, cb.y + 1 * s), CR_INK, 19 * s, 0, 2.5f * s);
        const ImVec2 ta(ca.x + 10 * s, ca.y + 10 * s), tb(cb.x - 10 * s, ca.y + 10 * s + thH);
        if (live) { tex = m_gui->viewportTexture(); texAr = (m_stage.z - m_stage.x) / std::max(1.0f, m_stage.w - m_stage.y); }
        if (tex) {   // cover
            const float ar = (tb.x - ta.x) / (tb.y - ta.y);
            const float cu = ar < texAr ? 0.5f * (1 - ar / texAr) : 0, cv = ar < texAr ? 0 : 0.5f * (1 - texAr / ar);
            dl->AddImageRounded(tex, ta, tb, ImVec2(cu, cv), ImVec2(1 - cu, 1 - cv), IM_COL32_WHITE, 12 * s);
        } else {
            dl->AddRectFilled(ta, tb, T_VIGNETTE, 12 * s);
            kitIcon(dl, cd.kind == 1 ? "cloud" : "brique", ImVec2((ta.x + tb.x) * 0.5f, (ta.y + tb.y) * 0.5f), 34 * s, T_PIECE);
        }
        bool act = false;
        if (cd.kind == 0 || cd.kind == 1) {   // « … »: rename, duplicate, delete
            const ImVec2 mc(tb.x - 8 * s - 14 * s, ta.y + 8 * s + 14 * s);
            const bool hm = ub.hover(ImVec2(mc.x - 18 * s, mc.y - 18 * s), ImVec2(mc.x + 18 * s, mc.y + 18 * s)) && ub.hover(ga, gb);
            dl->AddCircleFilled(mc, 14 * s, hm && held ? T_BULLE_APPUI : T_BULLE, 32);
            kitIcon(dl, "plus_actions", mc, 16 * s, T_SURBULLE);
            if (hm && tap) { m_crCardMenu = cd.kind * 100000 + cd.i; m_crCardMenuAt = mc; }
            if (hm) act = true;
        }
        const float ny = tb.y + 8 * s;
        if (cd.kind == 0 && m_crHubRename == cd.i) {   // the name, edited in place
            const ImVec2 ea(ca.x + 8 * s, ny - 2 * s), eb(cb.x - 8 * s, ny + 24 * s);
            dl->AddRectFilled(ea, eb, T_BLANC, 8 * s);
            dl->AddRect(ea, eb, CR_INK, 8 * s, 0, 1.5f * s);
            ImGui::SetNextWindowPos(ImVec2(ea.x + 6 * s, ea.y));
            ImGui::SetNextWindowSize(ImVec2(eb.x - ea.x - 12 * s, eb.y - ea.y));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 3 * s));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(CR_INK));
            ImGui::Begin("##crrename", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
            ImGui::PushFont(kF(true), 15 * s * kEm);
            ImGui::SetNextItemWidth(-1);
            if (m_hubEditFocus) { ImGui::SetKeyboardFocusHere(); m_hubEditFocus = false; }
            if (ImGui::InputText("##n", m_hubName, sizeof(m_hubName), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                const int i = m_crHubRename;
                m_crHubRename = -1;
                if (m_hubName[0]) hubRename(i, m_hubName);
            } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_crHubRename = -1;
            ImGui::PopFont();
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            act = true;
        } else {
            dl->PushClipRect(ImVec2(ca.x, std::max(ny, ga.y)), ImVec2(cb.x - 10 * s, std::min(cb.y, gb.y)), true);
            kText(dl, ImVec2(ca.x + 12 * s, ny), 15 * s, CR_INK, name.c_str(), true);
            dl->PopClipRect();
        }
        const float my = ny + 20 * s + 6 * s + 7 * s;
        dl->AddCircleFilled(ImVec2(ca.x + 12 * s + 3.5f * s, my), 3.5f * s, dot, 12);
        dl->PushClipRect(ImVec2(ca.x, std::max(ny, ga.y)), ImVec2(cb.x - 10 * s, std::min(cb.y, gb.y)), true);
        kTextC(dl, ca.x + 12 * s + 13 * s, my, 12 * s, CR_PALE, meta.c_str());
        dl->PopClipRect();
        if (act || !h || !tap) continue;
        if (cd.kind == 0) {
            const HubScene sc = m_hubLocal[cd.i];
            if (sc.path != m_scenePath) { keepOpen(); if (openScene(sc.path)) m_modelName = sc.name; }
        } else if (cd.kind == 1) {
            const CloudScene c = m_cloudList[cd.i];
            if (!cur) { keepOpen(); cloudOpen(c, false); }
        } else if (cd.kind == 2) {
            keepOpen();
            if (cd.i == 0) openOfficialHouse();
            else { newScene(1); buildExcavatorDemo(); rebuildScene(); frameAll(); m_scene.camera.pitch = radians(24.0f); m_scene.camera.yaw = radians(-30.0f); m_modelName = "Pelleteuse et mécanismes"; }
        }
        close = true;
        break;
    }
    dl->PopClipRect();
    // ======== the right column: the cloud, what was in the Maison menu, the settings of the view
    {
        dl->AddLine(ImVec2(cx0, a.y + padY), ImVec2(cx0, b.y - padY), CR_LINE, 1.0f * s);
        const float X = cx0 + 26 * s, XW = b.x - padX - X;
        float ry = a.y + padY;
        {   // the cloud
            const ImVec2 qa(X, ry), qb(X + XW, ry + 64 * s);
            const bool in = m_cloud.signedIn(), busy = m_cloudDirty || m_cloudSending;
            const bool h = ub.hover(qa, qb);
            kitShadowRect(dl, qa, qb, T_OMBRE_DOUCE, 2 * s, 12 * s);
            dl->AddRectFilled(qa, qb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? T_CREUX : CR_CARD, 18 * s);
            kitIcon(dl, "cloud", ImVec2(qa.x + 14 * s + 14 * s, (qa.y + qb.y) * 0.5f), 28 * s, T_BRUN);
            kTextC(dl, qa.x + 14 * s + 28 * s + 12 * s, qa.y + 23 * s, 15 * s, CR_INK, !in ? "Hors cloud" : busy ? "Envoi au cloud…" : "Cloud à jour", true);
            kTextC(dl, qa.x + 14 * s + 28 * s + 12 * s, qa.y + 43 * s, 12 * s, CR_PALE, !in ? "Réglages › Cloud pour te connecter" : m_ipad ? "iPad et PC synchronisés" : "PC et iPad synchronisés");
            dl->AddCircleFilled(ImVec2(qb.x - 14 * s - 5 * s, (qa.y + qb.y) * 0.5f), 5 * s, !in ? IM_COL32(0xc9, 0xb8, 0xa5, 255) : busy ? IM_COL32(0xe8, 0xc0, 0x4a, 255) : CR_GREEN, 16);
            if (h && ub.pressed) {
                if (!in) { m_popup = 5; m_setSec = kSetCloud; m_setScroll = 0; m_popupAt = ImVec2(a.x + 40 * s, 84 * s); close = true; }
                else { m_hubSection = 2; m_crHubScroll = 0; cloudRefreshList(); }
            }
            ry += 64 * s + 16 * s;
        }
        struct It { const char* icon; const char* label; const char* note; int act; };
        const It its[5] = {{"ouvrir", "Ouvrir", "Fichiers", 0}, {"enregistrer", "Enregistrer une copie", nullptr, 1}, {"image", "Partager une image", nullptr, 2},
                           {"presets", "Presets", "groupes enregistrés", 3}, {"reglages", "Réglages", nullptr, 4}};
        for (const It& it : its) {
            const ImVec2 ra(X - 8 * s, ry), rb(X + XW, ry + 42 * s);
            const bool h = ub.hover(ra, rb);
            if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) dl->AddRectFilled(ra, rb, T_BLANC, 12 * s);
            kitIcon(dl, it.icon, ImVec2(X + 10 * s, (ra.y + rb.y) * 0.5f), 20 * s, T_BRUN);
            kTextC(dl, X + 32 * s, (ra.y + rb.y) * 0.5f, 15 * s, T_BRUN, it.label);
            if (it.note) { const float nw = kSize(12 * s, it.note).x; kTextC(dl, rb.x - nw, (ra.y + rb.y) * 0.5f + 1 * s, 12 * s, CR_PALE, it.note); }
            if (h && ub.pressed) {
                switch (it.act) {
                case 0: keepOpen(); openDialog(); break;
                case 1: saveDialog(); break;
                case 2: shareImage(); break;
                case 3: m_crPresets = true; m_presetsKey.clear(); break;
                case 4: m_popup = 5; m_setScroll = 0; m_popupAt = ImVec2(a.x + 40 * s, 84 * s); break;
                }
                close = true;
            }
            ry += 46 * s;
        }
        ry += 4 * s;
        dl->AddLine(ImVec2(X, ry), ImVec2(X + XW, ry), CR_LINE, 1.0f * s);
        ry += 16 * s;
        const int qi = m_renderScale > 0.95f ? 2 : m_renderScale > 0.8f ? 1 : 0;
        const int thm = m_themeId == "brique" ? 1 : m_themeId == "classic" ? 2 : 0;
        struct Sg { const char* label; std::vector<const char*> opts; int cur; int id; };
        std::vector<Sg> sgs = {{"Qualité", {"Économie", "Normale", "Belle"}, qi, 0}, {"Lumière", {"Matin", "Midi", "Soir"}, m_lightPreset, 1},
                               {"Grille", {"Oui", "Non"}, m_build.gridSnap ? 0 : 1, 2}, {"Thème", {"Crème", "Brique", "Classique"}, thm, 3},
                               {"Interface", {"Crème", "Grey", "Black"}, 0, 5}};
        if (m_ipad) sgs.push_back({"Stylet seul", {"Oui", "Auto"}, m_pencilOnly ? 0 : 1, 4});
        for (const Sg& sg : sgs) {
            kTextC(dl, X, ry + 14 * s, 14 * s, T_BRUN, sg.label);
            const int k = crPill(ub, ImVec2(X + XW, ry), s, sg.opts, sg.cur);
            if (k >= 0) {
                switch (sg.id) {
                case 0: { static const float kS[3] = {0.67f, 0.85f, 1.0f}; m_renderScale = kS[k]; break; }
                case 1: m_lightPreset = k; applyLook(); break;
                case 2: m_build.gridSnap = k == 0; break;
                case 3: if (k != thm) m_themeId = k == 0 ? "creme" : k == 1 ? "brique" : "classic"; break;   // (the colours: the kit stays)
                case 5: if (k > 0) { m_ux = k == 1 ? 0 : 1; close = true; } break;   // the Studio interface: Grey (Brique) or Black (Classique)
                case 4: m_pencilOnly = k == 0; break;
                }
                savePrefs();
            }
            ry += 28 * s + 12 * s;
        }
        {   // at the bottom: a report (the iPad: its share sheet)
            const char* t = "Un souci ? Envoyer un rapport";
            const ImVec2 ra(X, b.y - padY - 22 * s), rb(X + kSize(12 * s, t).x, b.y - padY);
            if (ry < ra.y) {
                kTextC(dl, X, (ra.y + rb.y) * 0.5f, 12 * s, ub.hover(ra, rb) ? CR_LINK : CR_PALE, t);
                if (ub.hover(ra, rb) && ub.pressed) sendReport();
            }
        }
    }
    // ======== the « … » menu of a card
    if (m_crCardMenu >= 0 && menuOpen) {
        const int kind = m_crCardMenu / 100000, i = m_crCardMenu % 100000;
        const bool okI = kind == 0 ? i < (int)m_hubLocal.size() : i < (int)m_cloudList.size();
        if (!okI) m_crCardMenu = -1;
        else {
            struct Mi { const char* icon; const char* label; int act; };
            std::vector<Mi> mi;
            if (kind == 0) mi = {{"fiche", "Renommer", 0}, {"dupliquer", "Dupliquer", 1}, {"supprimer", "Supprimer", 2}};
            else mi = {{"supprimer", "Supprimer du cloud", 3}};
            const float MW = 250 * s, mp = 11 * s, MH = 2 * mp + mi.size() * 50 * s;
            ImVec2 ma(m_crCardMenuAt.x - 14 * s, m_crCardMenuAt.y + 20 * s);
            ma.x = std::clamp(ma.x, 16 * s, d.x - 16 * s - MW);
            ma.y = std::min(ma.y, d.y - 16 * s - MH);
            const ImVec2 mb(ma.x + MW, ma.y + MH);
            kitSheet(dl, ma, mb);
            float my = ma.y + mp;
            bool done = false;
            for (const Mi& m : mi) {
                if (!done && kitRow(u, ImVec2(ma.x + mp, my), MW - 2 * mp, m.icon, m.label, nullptr, m.act >= 2)) {
                    done = true;
                    switch (m.act) {
                    case 0: m_crHubRename = i; m_hubEditFocus = true; snprintf(m_hubName, sizeof(m_hubName), "%s", m_hubLocal[i].name.c_str()); break;
                    case 1: hubDuplicate(i); break;
                    case 2: hubDelete(i); break;
                    case 3: {
                        const CloudScene cs = m_cloudList[i];
                        plat::confirm(L"Supprimer du cloud", plat::fromUtf8("Supprimer « " + (cs.name.empty() ? std::string("Ma scène") : cs.name) + " » du cloud ?"), [this, cs] {
                            m_cloud.remove(cs.id, [this, cs](bool ok, const std::string& err) {
                                if (!ok) { toast("Cloud : " + err); return; }
                                if (cs.id == m_cloudId) cloudForget();
                                cloudRefreshList();
                            });
                        });
                        break;
                    }
                    }
                }
                my += 50 * s;
            }
            if (done || (u.pressed && !u.hover(ma, mb))) m_crCardMenu = -1;
        }
    }
    if (m_crHubRename < 0 && m_crCardMenu < 0 && ImGui::IsKeyPressed(ImGuiKey_Escape)) close = true;
    if (close) { m_homeOpen = false; m_newSceneOpen = false; m_crCardMenu = -1; m_crHubRename = -1; }
}
