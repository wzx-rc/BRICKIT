// ------------------------------------------------------------------ the « Brique » interface (Wzx 15/11/2026)
// From the mock-up « BrickIt nouveau design » (canvas JBtw2wBYiTVRTg7JuzaXc5, after Wzx's reference images): slate
// panels in relief, the yellow of the brick in a gradient on what is active, round type (Fredoka). Its own layout:
//   the header: the logo, the pills Hub / Construire / Peindre / Mécanique / Notice, round buttons on the right;
//   Construire: the library docked on the left, the 3D view inset with rounded corners, the scene and the colours in a
//   column on the right, the tools at the top of the view, the navigation on its right, the scene, the render and
//   Enregistrer in its corners;
//   the Hub: a page (a menu on the left, a banner, Mes scènes in cards, Stockage / Continuer / Outils rapides);
//   Réglages › Interface › Éditer l'interface: every block of the view can be moved, resized or hidden (the functions
//   are off meanwhile), kept in the preferences (« uiBoxes »), on the PC and the iPad.
// The rest of the Studio interface is drawn by BrickUiStudio.inl in this look (g_brique: stGlass, the tools...).

static const char* BQ_HOME = "FM12 3 2 11.5h3V21h5.5v-6h3v6H19v-9.5h3z";
static const char* BQ_BUILD = "M12 3 21 8v8l-9 5-9-5V8zM3 8l9 5 9-5M12 13v8";
static const char* BQ_PAINT = "M18 3 9 12M7 14c-2 0-3 1.5-3 3.5S3 20 3 20s3 .5 5-1.5S9 15 7 14z";
static const char* BQ_MECH = "M12 8.5a3.5 3.5 0 1 0 0 7 3.5 3.5 0 0 0 0-7zM12 2v3M12 19v3M2 12h3M19 12h3M5 5l2 2M17 17l2 2M5 19l2-2M17 7l2-2";
static const char* BQ_BOOK = "M3 5h7a2 2 0 0 1 2 2v13a2 2 0 0 0-2-2H3zM21 5h-7a2 2 0 0 0-2 2v13a2 2 0 0 1 2-2h7z";
static const char* BQ_FOLDER = "M3 7h7l2 2h9v10H3zM3 11h18";
static const char* BQ_CLOCK = "M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18zM12 7v5l3.5 2";
static const char* BQ_CLOUD = "M7 18h10a4 4 0 0 0 .6-7.95A6 6 0 0 0 6.1 9.2 4.5 4.5 0 0 0 7 18z";
static const char* BQ_DEMO = "M3 5h18v15H3zM8 11l3 3 5-6";
static const char* BQ_GRID = "M4 4h7v7H4zM13 4h7v7h-7zM4 13h7v7H4zM13 13h7v7h-7z";
static const char* BQ_PLAY = "FM7 4v16l13-8z";
static const char* BQ_CAMERA = "M4 8h4l2-3h4l2 3h4v11H4zM12 9.5a3.5 3.5 0 1 0 0 7 3.5 3.5 0 0 0 0-7z";
static const char* BQ_PALETTE = "M12 3a9 9 0 1 0 0 18c1.5 0 2-1 2-2s-1-1.5-1-2.5S14 15 15.5 15H18a3 3 0 0 0 3-3c0-5-4-9-9-9zM7.5 10.5h.01M10.5 7h.01M15.5 8h.01";
static const char* BQ_EXPORT = "M12 15V3M7 8l5-5 5 5M4 14v6h16v-6";
static const char* BQ_CHEV = "m9 6 6 6-6 6";
static const char* BQ_SAVE = "M5 4h11l3 3v13H5zM8 4v5h7V4M8 20v-6h8v6";
static const char* BQ_EYE = "M2 12c3-5 6.5-7 10-7s7 2 10 7c-3 5-6.5 7-10 7s-7-2-10-7zM12 9a3 3 0 1 0 0 6 3 3 0 0 0 0-6z";
static const char* BQ_EYEOFF = "M2 12c3-5 6.5-7 10-7s7 2 10 7c-3 5-6.5 7-10 7s-7-2-10-7zM4 4l16 16";
static const char* BQ_RESIZE = "M20 14v6h-6M20 20l-7-7";

// ------------------------------------------------------------------ the boxes of the UI editor

// the offset of a block moved in the UI editor (its position kept as a fraction of the screen)
ImVec2 BrickApp::bqOff(const char* id) {
    auto it = m_boxes.find(id);
    if (it == m_boxes.end() || it->second.x < 0) return ImVec2(0, 0);
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const UiBox& b = it->second;
    return ImVec2(b.x * d.x - b.def.x, b.y * d.y - b.def.y);
}
float BrickApp::bqK(const char* id) {
    auto it = m_boxes.find(id);
    return it == m_boxes.end() ? 1.0f : std::clamp(it->second.k, 0.5f, 2.5f);
}
ImVec2 BrickApp::bqSize(const char* id, ImVec2 def) {
    auto it = m_boxes.find(id);
    if (it == m_boxes.end() || it->second.w <= 0) return def;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    return ImVec2(it->second.w * d.x, it->second.h * d.y);
}
bool BrickApp::bqHidden(const char* id) {
    auto it = m_boxes.find(id);
    return it != m_boxes.end() && it->second.hidden && !m_editUi;
}
// a block drawn this frame: its default place (the editor's reference) and where it is (sized: w / h editable)
void BrickApp::bqReg(const char* id, const char* label, ImVec4 def, ImVec4 now, bool sized) {
    UiBox& b = m_boxes[id];
    b.def = def;
    b.last = now;
    b.label = label;
    b.sized = sized;
    if (m_editUi) m_editIds.push_back(id);
}

// ------------------------------------------------------------------ the header

void BrickApp::stHeaderBrique() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = u.dl;
    const float T = m_ipad ? kIpadTopStrip * s : 0;
    const float H = 64 * s + T, g = 12 * s;
    const float LW = bqLeftW();
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(d.x, H + g), C_BG);
    bqGrad(dl, ImVec2(0, 0), ImVec2(d.x, H), 0, IM_COL32(0x1f, 0x23, 0x2b, 255), C_BG);
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(LW, H), IM_COL32(0x1b, 0x1e, 0x26, 255), IM_COL32(0x1f, 0x23, 0x2b, 255), IM_COL32(0x1f, 0x23, 0x2b, 255), IM_COL32(0x1b, 0x1e, 0x26, 255));
    m_uiRects.push_back(ImVec4(0, 0, d.x, H + g));
    const float cy = T + 32 * s;
    const bool hubOn = m_popup == 8;
    {   // the logo: the yellow brick, Brick in white, It in yellow (a click: the Hub)
        float x = 14 * s;
        if (ImTextureID lt = themeTex("brique/logo.png")) {
            dl->AddImage(lt, ImVec2(x, cy - 21 * s), ImVec2(x + 44 * s, cy + 21 * s));
            x += 50 * s;
        }
        ImFont* f = u.bold;
        dl->AddText(f, u.fs(19), ImVec2(x, cy - u.fs(19) * 0.62f), C_TEXT, "Brick");
        const float bw = f->CalcTextSizeA(u.fs(19), 1e9f, 0, "Brick").x;
        dl->AddText(f, u.fs(19), ImVec2(x + bw, cy - u.fs(19) * 0.62f), C_ACCENT, "It");
        const ImVec2 la(0, T), lb(LW, H);
        if (u.hover(la, lb) && u.pressed) { m_popup = hubOn ? 0 : 8; m_hubStale = true; if (m_popup == 8) cloudRefreshList(); }
    }
    // the pills: Hub, Construire, Peindre, Mécanique, Notice
    const bool notice = m_pManual.visible;
    struct P { const char* ico; const char* label; bool on; int id; };
    const P ps[5] = {{BQ_HOME, "Hub", hubOn, 0}, {BQ_BUILD, "Construire", !hubOn && !m_mechOn && !m_paintMode && !notice, 1},
                     {BQ_PAINT, "Peindre", !hubOn && m_paintMode && !m_mechOn, 2}, {BQ_MECH, "Mécanique", !hubOn && m_mechOn, 3}, {BQ_BOOK, "Notice", !hubOn && notice && !m_mechOn, 4}};
    float x = LW + 16 * s;
    const float ph = 44 * s;
    for (const P& p : ps) {
        const float lw = u.textW(14, p.label, true), w = 22 * s + 22 * s + 8 * s + lw;
        const ImVec2 a(x, cy - ph * 0.5f), b(x + w, cy + ph * 0.5f);
        const bool h = u.hover(a, b);
        if (p.on) bqYellow(dl, a, b, 13 * s, s, h);
        else if (h) dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 14), 13 * s);
        const ImU32 fg = p.on ? C_INK : C_TEXT;
        stIcon(dl, p.ico, ImVec2(a.x + 22 * s + 0 * s, cy), 21 * s, p.on ? C_INK : IM_COL32(0xc9, 0xcd, 0xd6, 255), 2.0f * s);
        u.text(ImVec2(a.x + 22 * s + 19 * s, cy - u.fs(14) * 0.56f), 14, fg, p.label, true);
        if (h && u.pressed) {
            switch (p.id) {
            case 0: m_popup = hubOn ? 0 : 8; m_hubStale = true; if (m_popup == 8) cloudRefreshList(); break;
            case 1: if (m_popup == 8) m_popup = 0; if (m_mechOn) mechExit(); m_paintMode = false; m_paintTool = false; m_pipette = false; m_pManual.visible = false; break;
            case 2: if (m_popup == 8) m_popup = 0; if (m_mechOn) mechExit(); m_paintMode = true; m_paintTool = true; m_pipette = false; break;
            case 3: if (m_popup == 8) m_popup = 0; if (m_mechOn) mechExit(); else mechEnter(); m_paintMode = false; m_paintTool = false; m_pipette = false; break;
            case 4: if (m_popup == 8) m_popup = 0; m_pManual.visible = !m_pManual.visible; break;
            }
        }
        x += w + 6 * s;
    }
    // on the right: the account, Exporter, settings, more, (HD), redo, undo
    float rx = d.x - 14 * s;
    {   // the account: a yellow coin with the initial (Réglages › Cloud)
        const float r = 21 * s;
        const ImVec2 c(rx - r, cy);
        const bool h = (u.mouse.x - c.x) * (u.mouse.x - c.x) + (u.mouse.y - c.y) * (u.mouse.y - c.y) <= r * r;
        dl->AddCircleFilled(ImVec2(c.x, c.y + 2 * s), r + 2 * s, IM_COL32(0, 0, 0, 60), 32);
        dl->AddCircleFilled(c, r + 2.5f * s, IM_COL32(0x2d, 0x32, 0x3d, 255), 32);
        dl->AddCircleFilled(c, r, h ? IM_COL32(0xff, 0xdc, 0x5c, 255) : IM_COL32(0xfa, 0xc0, 0x18, 255), 32);
        std::string ini = "?";
        if (m_cloud.signedIn() && !m_cloud.email().empty()) ini = std::string(1, (char)std::toupper((unsigned char)m_cloud.email()[0]));
        const float tw = u.textW(16, ini.c_str(), true);
        u.text(ImVec2(c.x - tw * 0.5f, c.y - u.fs(16) * 0.58f), 16, C_INK, ini.c_str(), true);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", m_cloud.signedIn() ? ("Compte : " + m_cloud.email()).c_str() : "Se connecter au cloud");
        if (h && u.pressed) { m_popup = 5; m_setSec = kSetCloud; m_setScroll = 0; m_popupAt = ImVec2(d.x - 660 * s, H + 6 * s); }
        rx -= 2 * r + 12 * s;
    }
    auto roundBtn = [&](const char* icon, const char* tip, bool on, bool enabled) {
        const float w = 42 * s;
        const ImVec2 a(rx - w, cy - w * 0.5f), b(rx, cy + w * 0.5f);
        const bool h = enabled && u.hover(a, b);
        if (on) bqYellow(dl, a, b, 13 * s, s, h);
        else bqRaise(dl, a, b, 13 * s, s, h ? IM_COL32(0x38, 0x3e, 0x4a, 255) : IM_COL32(0x2d, 0x32, 0x3d, 255), 50);
        stIcon(dl, icon, ImVec2((a.x + b.x) * 0.5f, cy), 20 * s, on ? C_INK : enabled ? C_TEXT : stA(C_TEXT, 0.3f), 2.0f * s);
        if (h && tip && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", tip);
        rx -= w + 10 * s;
        return h && u.pressed;
    };
    if (roundBtn(BQ_EXPORT, "Exporter : ouvrir, enregistrer, LDraw, OBJ, notice", m_popup == 4, true)) { m_popup = m_popup == 4 ? 0 : 4; m_popupAt = ImVec2(rx - 200 * s, H + 6 * s); }
    if (roundBtn(ST_SET, "Réglages (interface, rendu, lumière, thème)", m_popup == 5, true)) { m_popup = m_popup == 5 ? 0 : 5; m_popupAt = ImVec2(d.x - 660 * s, H + 6 * s); }
    if (roundBtn(ST_MORE, "Plus : grille, auto pivot, sécurité, animations, voxels, fenêtres, DEV", m_popup == 7, true)) { m_popup = m_popup == 7 ? 0 : 7; m_popupAt = ImVec2(rx - 160 * s, H + 6 * s); }
    if (roundBtn(ST_REDO, "Rétablir (Ctrl+Y)", false, !m_redo.empty())) redo();
    if (roundBtn(ST_UNDO, "Annuler (Ctrl+Z)", false, !m_undo.empty() || m_holding)) undo();
}

// the width of the left column (the library docked on the left, the menu of the hub, the logo above them)
float BrickApp::bqLeftW() const {
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    if (m_libDock == 1 && !m_mechOn) return std::clamp(m_libW > 0 ? m_libW : 300 * s, 260 * s, d.x * 0.5f);
    return 250 * s;
}

// ------------------------------------------------------------------ Construire: the frame of the view, its corners

void BrickApp::stBriqueStage() {
    if (m_popup == 8) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui, g = 12 * s;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = u.dl;
    // the right column (Scène, Couleurs), gaps around the view, its rounded corners
    const bool col = !m_mechOn && (m_pColors.visible || m_pGroups.visible);
    const float RW = col ? 296 * s : g;
    m_bqColX = d.x - RW + g * 0.5f;
    m_bqColW = RW - g * 1.5f;
    m_dockR += RW;
    m_dockL += m_dockL > 0 ? g : g;
    if (m_uiRects.size() > 1 && m_uiRects[1].y >= d.y - 1) m_uiRects[1].y = d.y - g;
    const float top = m_uiRects[0].w, x0 = m_dockL, x1 = d.x - m_dockR, y1 = m_uiRects.size() > 1 ? m_uiRects[1].y : d.y - g;
    dl->AddRectFilled(ImVec2(x1, top), ImVec2(d.x, d.y), C_BG);
    dl->AddRectFilled(ImVec2(x0 - g, top), ImVec2(x0, d.y), C_BG);
    dl->AddRectFilled(ImVec2(x0, y1), ImVec2(x1, d.y), C_BG);
    m_uiRects.push_back(ImVec4(x1, top, d.x, d.y));
    {   // the corners of the view, rounded (the background filled outside a quarter circle)
        const float r = 16 * s;
        const ImVec2 cs[4] = {ImVec2(x0, top), ImVec2(x1, top), ImVec2(x1, y1), ImVec2(x0, y1)};
        for (int k = 0; k < 4; ++k) {
            const ImVec2 p = cs[k];
            const float sx = (k == 0 || k == 3) ? 1.0f : -1.0f, sy = (k < 2) ? 1.0f : -1.0f;
            const ImVec2 c(p.x + sx * r, p.y + sy * r);
            const float a0 = std::atan2(-sy, 0.0f), a1 = std::atan2(0.0f, -sx);
            dl->PathLineTo(p);
            float aa = a0, ab = a1;
            if (std::fabs(ab - aa) > 3.1416f) { if (ab > aa) ab -= 6.2832f; else ab += 6.2832f; }
            dl->PathArcTo(c, r, aa, ab, 10);
            dl->PathFillConcave(C_BG);
        }
    }
    m_stageB = ImVec4(x0, top, x1, y1);
}

// the scene (top left of the view), the render (bottom left), Enregistrer (bottom right)
void BrickApp::stBriqueCorners() {
    if (m_popup == 8 || m_mechOn || m_opt.noUi || m_reading) return;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const ImVec4 st = m_stageB;
    (void)d;
    // ---- the scene: its thumbnail, its name, saved / cloud; a click: the Hub
    if (!bqHidden("scene")) {
        const float k = bqK("scene"), s = m_ui * k;
        Ui u = ui(s);
        if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
        const ImVec2 off = bqOff("scene");
        const std::string nm = m_modelName.empty() ? std::string("Ma scène") : m_modelName;
        const std::wstring dir = scenesDir();
        const bool inLib = !m_scenePath.empty() && m_scenePath.size() > dir.size() && m_scenePath.compare(0, dir.size(), dir) == 0;
        std::string stt;
        ImU32 sc;
        if (m_scenePath.empty()) { stt = "Pas encore enregistrée"; sc = C_DIM; }
        else if (m_cloud.signedIn() && !m_cloudId.empty() && !m_cloudDirty) { stt = "Enregistrée · cloud à jour"; sc = IM_COL32(0x5b, 0xe0, 0x9b, 255); }
        else if (m_cloudDirty) { stt = "Modifiée · envoi au cloud"; sc = C_ACCENT; }
        else { stt = "Enregistrée"; sc = IM_COL32(0x5b, 0xe0, 0x9b, 255); }
        const float nw = std::max(u.textW(14, nm.c_str(), true), u.textW(10.5f, stt.c_str())) ;
        const float W = std::min(8 * s + 58 * s + 12 * s + nw + 34 * s, 360 * s), Hh = 54 * s;
        const ImVec2 a0(st.x + 14 * m_ui, st.y + 14 * m_ui), a(a0.x + off.x, a0.y + off.y), b(a.x + W, a.y + Hh);
        bqRaise(u.dl, a, b, 15 * s, s, C_HUD, 60);
        m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
        const ImVec2 ta(a.x + 8 * s, a.y + 8 * s), tb(ta.x + 58 * s, b.y - 8 * s);
        ImTextureID tex = inLib ? hubThumb(m_scenePath) : 0;
        if (tex) u.dl->AddImageRounded(tex, ta, tb, ImVec2(0.1f, 0.05f), ImVec2(0.9f, 0.95f), IM_COL32_WHITE, 8 * s);
        else { u.dl->AddRectFilled(ta, tb, C_CARD, 8 * s); stPicto(u.dl, "brick", 2, 4, ImVec2((ta.x + tb.x) * 0.5f, (ta.y + tb.y) * 0.5f), 26 * s, C_DIM, 1.2f * s); }
        const float tx = tb.x + 12 * s;
        u.text(ImVec2(tx, a.y + 9 * s), 14, C_TEXT, stFit(u, 14, nm, W - (tx - a.x) - 30 * s, true).c_str(), true);
        u.dl->AddCircleFilled(ImVec2(tx + 4 * s, a.y + 37 * s), 3.5f * s, sc, 12);
        u.text(ImVec2(tx + 12 * s, a.y + 30 * s), 10.5f, sc, stt.c_str());
        stIcon(u.dl, "FM5 8h14l-7 9z", ImVec2(b.x - 16 * s, (a.y + b.y) * 0.5f), 11 * s, C_DIM, 1.0f);
        const bool h = u.hover(a, b);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Mes scènes : enregistrer, renommer, ouvrir, cloud");
        if (h && u.pressed) { m_popup = 8; m_hubStale = true; cloudRefreshList(); }
        m_bqSceneY = b.y;
        bqReg("scene", "Scène", ImVec4(a0.x, a0.y, a0.x + W, a0.y + Hh), ImVec4(a.x, a.y, b.x, b.y), false);
    } else m_bqSceneY = st.y;
    // ---- the render: Rapide / HQ / Ray tracing, HD (PC)
    if (!m_ipad && !bqHidden("render")) {
        const float k = bqK("render"), s = m_ui * k;
        Ui u = ui(s);
        if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
        const ImVec2 off = bqOff("render");
        struct R { const char* label; bool on; int id; const char* tip; };
        const R rs[4] = {{"Rapide", m_rs.raster && m_quality == 0 && !m_rcMode, 0, "Temps réel rapide"}, {"HQ", m_rs.raster && m_quality == 1 && !m_rcMode, 1, "Temps réel haute qualité : ombres, GI douce"},
                         {"Ray tracing", pathTraced(), 2, "Rendu progressif réaliste (path tracing)"}, {"HD", m_highPoly, 3, "Pièces HD : arrondis lisses"}};
        float W = 10 * s;
        for (const R& r : rs) W += u.textW(12, r.label, true) + 24 * s + 2 * s;
        const float Hh = 44 * s;
        const ImVec2 a0(st.x + 14 * m_ui, st.w - 14 * m_ui - Hh), a(a0.x + off.x, a0.y + off.y), b(a.x + W, a.y + Hh);
        bqRaise(u.dl, a, b, 14 * s, s, C_HUD, 60);
        m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
        float xx = a.x + 5 * s;
        for (const R& r : rs) {
            const float bw = u.textW(12, r.label, true) + 24 * s;
            const ImVec2 ba(xx, a.y + 5 * s), bb(xx + bw, b.y - 5 * s);
            const bool h = u.hover(ba, bb);
            if (r.id == 3) u.dl->AddLine(ImVec2(ba.x - 2 * s, ba.y + 6 * s), ImVec2(ba.x - 2 * s, bb.y - 6 * s), IM_COL32(255, 255, 255, 30), 1.0f);
            if (r.on) bqYellow(u.dl, ba, bb, 10 * s, s, h);
            else if (h) u.dl->AddRectFilled(ba, bb, IM_COL32(255, 255, 255, 16), 10 * s);
            u.text(ImVec2(ba.x + 12 * s, (ba.y + bb.y) * 0.5f - u.fs(12) * 0.56f), 12, r.on ? C_INK : C_TEXT, r.label, true);
            if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", r.tip);
            if (h && u.pressed) {
                if (r.id == 3) setHighPoly(!m_highPoly);
                else if (!r.on) {
                    if (r.id == 2) { setRC(false); m_rs.raster = false; applyLook(); }
                    else { setRC(false); m_quality = r.id; m_rs.msaaSamples = m_quality ? 4 : 1; if (!m_rs.raster) { m_rs.raster = true; applyLook(); } }
                    m_renderer->resetAccumulation();
                    savePrefs();
                }
            }
            xx += bw + 2 * s;
        }
        bqReg("render", "Rendu", ImVec4(a0.x, a0.y, a0.x + W, a0.y + Hh), ImVec4(a.x, a.y, b.x, b.y), false);
    }
    // ---- Enregistrer
    if (!bqHidden("save")) {
        const float k = bqK("save"), s = m_ui * k;
        Ui u = ui(s);
        if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
        const ImVec2 off = bqOff("save");
        const char* lab = "Enregistrer";
        const float W = u.textW(15, lab, true) + 64 * s, Hh = 48 * s;
        const ImVec2 a0(st.z - 14 * m_ui - W, st.w - 14 * m_ui - Hh), a(a0.x + off.x, a0.y + off.y), b(a.x + W, a.y + Hh);
        const bool h = u.hover(a, b);
        bqYellow(u.dl, a, b, 14 * s, s, h);
        stIcon(u.dl, BQ_SAVE, ImVec2(a.x + 24 * s, (a.y + b.y) * 0.5f - 1 * s), 20 * s, C_INK, 2.0f * s);
        u.text(ImVec2(a.x + 42 * s, (a.y + b.y) * 0.5f - u.fs(15) * 0.6f), 15, C_INK, lab, true);
        m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Enregistrer dans Mes scènes (Ctrl+S)");
        if (h && u.pressed) saveNow();
        bqReg("save", "Enregistrer", ImVec4(a0.x, a0.y, a0.x + W, a0.y + Hh), ImVec4(a.x, a.y, b.x, b.y), false);
    }
}

// ------------------------------------------------------------------ the UI editor (Réglages › Interface)

void BrickApp::stEditUi() {
    if (!m_editUi) { m_editIds.clear(); return; }
    Ui u = ui(m_ui);
    u.mouse = ImGui::GetIO().MousePos;
    u.pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = u.dl;
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));   // (nothing of the scene answers meanwhile)
    dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(8, 10, 14, 70));
    const bool down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool clicked = false;
    // the drag in progress
    if (!m_editDrag.empty()) {
        UiBox& b = m_boxes[m_editDrag];
        const ImVec2 dm(u.mouse.x - m_editP0.x, u.mouse.y - m_editP0.y);
        if (down) {
            if (m_editMode == 0) {
                const float w = b.last.z - b.last.x, h = b.last.w - b.last.y;
                b.x = std::clamp(m_editB0.last.x + dm.x, -w * 0.5f, d.x - w * 0.5f) / d.x;
                b.y = std::clamp(m_editB0.last.y + dm.y, 0.0f, d.y - 20 * s) / d.y;
            } else if (b.sized) {
                if (b.x < 0) { b.x = m_editB0.last.x / d.x; b.y = m_editB0.last.y / d.y; }
                b.w = std::max(160 * s, m_editB0.last.z - m_editB0.last.x + dm.x) / d.x;
                b.h = std::max(120 * s, m_editB0.last.w - m_editB0.last.y + dm.y) / d.y;
            } else {
                if (b.x < 0) { b.x = m_editB0.last.x / d.x; b.y = m_editB0.last.y / d.y; }
                const float w0 = std::max(1.0f, m_editB0.last.z - m_editB0.last.x);
                b.k = std::clamp(m_editB0.k * (w0 + dm.x) / w0, 0.5f, 2.5f);
            }
        } else m_editDrag.clear();
    }
    // the blocks: a dashed yellow frame, their name, move (drag), resize (the corner), hide (the eye)
    std::vector<std::string> ids = m_editIds;
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    for (const std::string& id : ids) {
        UiBox& b = m_boxes[id];
        const ImVec2 a(b.last.x, b.last.y), c(b.last.z, b.last.w);
        const bool on = m_editDrag == id;
        const bool h = u.hover(a, c) && m_editDrag.empty();
        dl->AddRectFilled(a, c, b.hidden ? IM_COL32(20, 22, 28, 190) : IM_COL32(255, 201, 60, on || h ? 34 : 14), 12 * s);
        for (float t = 0; t < 1; t += 0.025f) {   // the dashed frame
            const float per = 2 * ((c.x - a.x) + (c.y - a.y));
            auto P = [&](float q) {
                float l = q * per;
                if (l < c.x - a.x) return ImVec2(a.x + l, a.y);
                l -= c.x - a.x;
                if (l < c.y - a.y) return ImVec2(c.x, a.y + l);
                l -= c.y - a.y;
                if (l < c.x - a.x) return ImVec2(c.x - l, c.y);
                l -= c.x - a.x;
                return ImVec2(a.x, c.y - l);
            };
            dl->AddLine(P(t), P(t + 0.0125f), IM_COL32(255, 201, 60, on || h ? 255 : 170), 2.0f * s);
        }
        // its name
        const std::string lab = b.hidden ? b.label + " (masqué)" : b.label;
        const float lw = u.textW(11, lab.c_str(), true) + 16 * s;
        const ImVec2 na(a.x, a.y - 24 * s < 0 ? a.y + 4 * s : a.y - 24 * s), nb(na.x + lw, na.y + 20 * s);
        dl->AddRectFilled(na, nb, IM_COL32(255, 201, 60, 255), 10 * s);
        u.text(ImVec2(na.x + 8 * s, na.y + 10 * s - u.fs(11) * 0.56f), 11, IM_COL32(0x23, 0x18, 0, 255), lab.c_str(), true);
        // hide / show
        const ImVec2 ec(c.x - 16 * s, a.y + 16 * s);
        const bool he = (u.mouse.x - ec.x) * (u.mouse.x - ec.x) + (u.mouse.y - ec.y) * (u.mouse.y - ec.y) < 13 * s * 13 * s;
        dl->AddCircleFilled(ec, 13 * s, he ? IM_COL32(255, 201, 60, 255) : IM_COL32(24, 27, 34, 235), 24);
        stIcon(dl, b.hidden ? BQ_EYEOFF : BQ_EYE, ec, 15 * s, he ? IM_COL32(0x23, 0x18, 0, 255) : C_TEXT, 1.6f * s);
        // resize: the bottom right corner
        const ImVec2 rc(c.x - 2 * s, c.y - 2 * s);
        const ImVec2 ra(rc.x - 20 * s, rc.y - 20 * s);
        const bool hr = u.hover(ra, ImVec2(rc.x + 6 * s, rc.y + 6 * s));
        dl->AddRectFilled(ra, rc, hr ? IM_COL32(255, 201, 60, 255) : IM_COL32(24, 27, 34, 235), 6 * s);
        stIcon(dl, BQ_RESIZE, ImVec2((ra.x + rc.x) * 0.5f, (ra.y + rc.y) * 0.5f), 15 * s, hr ? IM_COL32(0x23, 0x18, 0, 255) : C_TEXT, 1.8f * s);
        if (u.pressed && m_editDrag.empty()) {
            if (he) { b.hidden = !b.hidden; clicked = true; }
            else if (hr) { m_editDrag = id; m_editMode = 1; m_editP0 = u.mouse; m_editB0 = b; clicked = true; }
            else if (h) { m_editDrag = id; m_editMode = 0; m_editP0 = u.mouse; m_editB0 = b; clicked = true; }
        }
        if (hr) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        else if (h) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
    // the banner: what it is, Réinitialiser, Terminer
    {
        const char* t1 = "Édition de l'interface";
        const char* t2 = "Glisse un bloc pour le déplacer, son coin pour le redimensionner, l'œil pour le masquer.";
        const float W = std::max(u.textW(16, t1, true), u.textW(11.5f, t2)) + 300 * s, Hh = 64 * s;
        const ImVec2 a((d.x - W) * 0.5f, std::floor(d.y * 0.62f)), b(a.x + W, a.y + Hh);   // (above the corners of the view)
        bqRaise(dl, a, b, 18 * s, s, IM_COL32(0x28, 0x2d, 0x37, 250), 90);
        u.text(ImVec2(a.x + 20 * s, a.y + 12 * s), 16, C_TEXT, t1, true);
        u.text(ImVec2(a.x + 20 * s, a.y + 36 * s), 11.5f, C_DIM, t2);
        const ImVec2 fa(b.x - 132 * s, a.y + 12 * s), fb(b.x - 14 * s, b.y - 12 * s);
        const bool hf = u.hover(fa, fb);
        bqYellow(dl, fa, fb, 12 * s, s, hf);
        u.text(ImVec2(fa.x + (fb.x - fa.x - u.textW(14, "Terminer", true)) * 0.5f, (fa.y + fb.y) * 0.5f - u.fs(14) * 0.58f), 14, C_INK, "Terminer", true);
        const ImVec2 ra(fa.x - 140 * s, fa.y), rb(fa.x - 10 * s, fb.y);
        const bool hr = u.hover(ra, rb);
        bqRaise(dl, ra, rb, 12 * s, s, hr ? C_CARD_HI : C_CARD, 40);
        u.text(ImVec2(ra.x + (rb.x - ra.x - u.textW(13, "Réinitialiser", true)) * 0.5f, (ra.y + rb.y) * 0.5f - u.fs(13) * 0.58f), 13, C_TEXT, "Réinitialiser", true);
        if (hf && u.pressed) { m_editUi = false; savePrefs(); toast("Interface enregistrée"); }
        if (hr && u.pressed) { m_boxes.clear(); toast("Disposition par défaut"); }
        (void)clicked;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { m_editUi = false; savePrefs(); }
    m_editIds.clear();
}

// ------------------------------------------------------------------ the Hub, as a page (Wzx's reference image)

void BrickApp::stHubPage() {
    if (m_hubStale) hubRefresh();
    Ui u = ui(m_ui);
    u.mouse = ImGui::GetIO().MousePos;
    u.pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = u.dl;
    const double now = ImGui::GetTime();
    const float top = m_uiRects.empty() ? 64 * s : m_uiRects[0].w - 12 * s;
    const bool editing = m_hubEdit != -1;
    bool close = false;
    m_uiRects.push_back(ImVec4(0, top, d.x, d.y));
    dl->AddRectFilled(ImVec2(0, top), d, C_BG);
    const std::wstring dir = scenesDir();
    const bool inLib = !m_scenePath.empty() && m_scenePath.size() > dir.size() && m_scenePath.compare(0, dir.size(), dir) == 0;
    auto keepOpen = [&] {
        if (m_bricks.empty()) return;
        if (inLib) saveScene(m_scenePath);
        else if (m_scenePath.empty()) hubSaveHere();
    };
    auto demo = [&](int i) {
        keepOpen();
        if (i == 0) openOfficialHouse();
        else { newScene(); buildExcavatorDemo(); rebuildScene(); frameAll(); m_scene.camera.pitch = radians(24.0f); m_scene.camera.yaw = radians(-30.0f); m_modelName = "Pelleteuse et mécanismes"; }
        close = true;
    };

    // ======== the menu on the left
    const float LW = bqLeftW();
    for (int i = 0; i < 12; ++i)   // its soft shadow on the page
        dl->AddRectFilled(ImVec2(LW + i * 1.5f * s, top), ImVec2(LW + (i + 1) * 1.5f * s, d.y), IM_COL32(0, 0, 0, (int)(50 * (1 - i / 12.0f))));
    dl->AddRectFilledMultiColor(ImVec2(0, top), ImVec2(LW, d.y), IM_COL32(0x1b, 0x1e, 0x26, 255), IM_COL32(0x20, 0x24, 0x2c, 255), IM_COL32(0x20, 0x24, 0x2c, 255), IM_COL32(0x1b, 0x1e, 0x26, 255));
    if (ImTextureID bt = themeTex("brique/bricks.png")) {   // the yellow bricks, at the bottom of the menu
        const float bs = 210 * s;
        dl->PushClipRect(ImVec2(0, top), ImVec2(LW, d.y), true);
        dl->AddImage(bt, ImVec2(-26 * s, d.y - bs * 0.78f), ImVec2(-26 * s + bs, d.y - bs * 0.78f + bs));
        dl->PopClipRect();
    }
    float y = top + 16 * s;
    {   // Nouvelle scène
        const ImVec2 a(18 * s, y), b(LW - 18 * s, y + 52 * s);
        const bool h = u.hover(a, b) && !editing;
        bqYellow(dl, a, b, 14 * s, s, h);
        stIcon(dl, ST_NEW, ImVec2(a.x + 28 * s, (a.y + b.y) * 0.5f - 1 * s), 22 * s, C_INK, 3.0f * s);
        u.text(ImVec2(a.x + 50 * s, (a.y + b.y) * 0.5f - u.fs(16) * 0.6f), 16, C_INK, "Nouvelle scène", true);
        if (h && u.pressed) { keepOpen(); newScene(); close = true; }
        y += 52 * s + 18 * s;
    }
    struct N { const char* ico; const char* label; int sec; int count; };
    const int nCloud = m_cloud.signedIn() ? (int)m_cloudList.size() : -1;
    const N ns[4] = {{BQ_FOLDER, "Mes scènes", 0, (int)m_hubLocal.size()}, {BQ_CLOCK, "Récentes", 1, -1}, {BQ_CLOUD, "Cloud", 2, nCloud}, {BQ_DEMO, "Démos", 3, -1}};
    for (const N& n : ns) {
        const ImVec2 a(18 * s, y), b(LW - 18 * s, y + 44 * s);
        const bool on = m_hubSection == n.sec, h = u.hover(a, b) && !editing;
        if (on) {
            dl->AddRectFilled(a, b, IM_COL32(0x2c, 0x31, 0x3c, 255), 12 * s);
            dl->AddRectFilled(ImVec2(0, a.y + 6 * s), ImVec2(4 * s, b.y - 6 * s), C_ACCENT, 2 * s);
        } else if (h) dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 12 * s);
        stIcon(dl, n.ico, ImVec2(a.x + 24 * s, (a.y + b.y) * 0.5f), 21 * s, C_TEXT, 2.0f * s);
        u.text(ImVec2(a.x + 46 * s, (a.y + b.y) * 0.5f - u.fs(14.5f) * 0.58f), 14.5f, C_TEXT, n.label, true);
        if (n.count >= 0) {
            const std::string c = std::to_string(n.count);
            const float cw = std::max(26 * s, u.textW(11, c.c_str(), true) + 14 * s);
            const ImVec2 ca(b.x - 10 * s - cw, (a.y + b.y) * 0.5f - 12 * s), cb(b.x - 10 * s, (a.y + b.y) * 0.5f + 12 * s);
            dl->AddRectFilled(ca, cb, IM_COL32(0x3a, 0x40, 0x4c, 255), 12 * s);
            u.text(ImVec2((ca.x + cb.x - u.textW(11, c.c_str(), true)) * 0.5f, (ca.y + cb.y) * 0.5f - u.fs(11) * 0.58f), 11, C_TEXT, c.c_str(), true);
        }
        if (h && u.pressed) { m_hubSection = n.sec; m_hubScroll = 0; if (n.sec == 2) cloudRefreshList(); if (n.sec <= 1) m_hubStale = true; }
        y += 48 * s;
    }
    dl->AddLine(ImVec2(34 * s, y + 8 * s), ImVec2(LW - 34 * s, y + 8 * s), IM_COL32(255, 255, 255, 22), 1.0f);
    y += 22 * s;
    {
        struct M { const char* ico; const char* label; int id; };
        const M ms[3] = {{ST_PRESET, "Presets", 0}, {ST_SET, "Réglages", 1}, {BQ_GRID, "Éditer l'interface", 2}};
        for (const M& m : ms) {
            const ImVec2 a(18 * s, y), b(LW - 18 * s, y + 44 * s);
            const bool h = u.hover(a, b) && !editing;
            if (h) dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 12 * s);
            stIcon(dl, m.ico, ImVec2(a.x + 24 * s, (a.y + b.y) * 0.5f), 21 * s, C_TEXT, 2.0f * s);
            u.text(ImVec2(a.x + 46 * s, (a.y + b.y) * 0.5f - u.fs(14.5f) * 0.58f), 14.5f, C_TEXT, m.label, true);
            if (h && u.pressed) {
                if (m.id == 0) { m_popup = 10; m_presetsKey.clear(); m_popupAt = ImVec2(LW + 10 * s, top + 10 * s); }
                else if (m.id == 1) { m_popup = 5; m_popupAt = ImVec2(d.x - 660 * s, top + 6 * s); }
                else { m_popup = 0; m_editUi = true; }
                return;
            }
            y += 48 * s;
        }
    }

    // ======== the right column: Stockage, Continuer, Outils rapides
    const float RW = 290 * s, rx0 = d.x - RW - 4 * s, rx1 = d.x - 20 * s;
    float ry = top + 16 * s;
    {   // Stockage: Mes scènes here, the cloud
        long long bytes = 0;
        for (const HubScene& h : m_hubLocal) {
            WIN32_FIND_DATAW fd;
            (void)fd;
            FILE* f = nullptr;
            if (_wfopen_s(&f, h.path.c_str(), L"rb") == 0 && f) { fseek(f, 0, SEEK_END); bytes += ftell(f); fclose(f); }
        }
        const float ch = 128 * s;
        const ImVec2 a(rx0, ry), b(rx1, ry + ch);
        bqRaise(dl, a, b, 18 * s, s, C_PANEL, 60);
        stIcon(dl, BQ_CLOUD, ImVec2(a.x + 34 * s, a.y + 34 * s), 34 * s, C_TEXT, 1.8f * s);
        u.text(ImVec2(a.x + 64 * s, a.y + 16 * s), 15, C_TEXT, "Stockage", true);
        char b1[96];
        snprintf(b1, sizeof(b1), "%d scènes · %.1f Mo sur cet appareil", (int)m_hubLocal.size(), bytes / 1048576.0);
        u.text(ImVec2(a.x + 64 * s, a.y + 38 * s), 11, C_DIM, b1);
        const bool in = m_cloud.signedIn();
        std::string cl = !in ? std::string("Cloud : non connecté") : m_cloudSending ? std::string("Cloud : envoi…") : m_cloudDirty ? std::string("Cloud : envoi dans la minute") : "Cloud : " + std::to_string(m_cloudList.size()) + " scènes · à jour";
        const ImU32 dot = !in ? C_FAINT : (m_cloudSending || m_cloudDirty) ? C_ACCENT : IM_COL32(0x5b, 0xe0, 0x9b, 255);
        dl->AddCircleFilled(ImVec2(a.x + 22 * s, a.y + 80 * s), 4 * s, dot, 12);
        u.text(ImVec2(a.x + 34 * s, a.y + 72 * s), 11.5f, C_TEXT, cl.c_str(), true);
        {   // Synchroniser / Se connecter
            const char* lab = in ? "Synchroniser" : "Se connecter";
            const ImVec2 ba(a.x + 16 * s, b.y - 38 * s), bb(b.x - 16 * s, b.y - 10 * s);
            const bool h = u.hover(ba, bb) && !editing && !m_cloudSending;
            bqRaise(dl, ba, bb, 9 * s, s, h ? C_CARD_HI : C_CARD, 30);
            stIcon(dl, ST_SYNC, ImVec2(ba.x + 18 * s, (ba.y + bb.y) * 0.5f), 14 * s, m_cloudSending ? C_ACCENT : C_TEXT, 1.6f * s);
            u.text(ImVec2(ba.x + 34 * s, (ba.y + bb.y) * 0.5f - u.fs(11.5f) * 0.58f), 11.5f, C_TEXT, lab, true);
            if (h && u.pressed) {
                if (in) { m_cloudSentKey.clear(); cloudSend(); cloudRefreshList(); }
                else { m_popup = 5; m_setSec = kSetCloud; m_setScroll = 0; m_popupAt = ImVec2(d.x - 660 * s, top + 6 * s); return; }
            }
        }
        ry += ch + 16 * s;
    }
    {   // Continuer: the scene open (rename it, its thumbnail, save it)
        const float ch = 150 * s;
        const ImVec2 a(rx0, ry), b(rx1, ry + ch);
        bqRaise(dl, a, b, 18 * s, s, C_PANEL, 60);
        u.text(ImVec2(a.x + 18 * s, a.y + 14 * s), 17, C_TEXT, "Continuer", true);
        const ImVec2 ta(a.x + 18 * s, a.y + 48 * s), tb(ta.x + 84 * s, ta.y + 60 * s);
        ImTextureID tex = inLib ? hubThumb(m_scenePath) : 0;
        if (tex) dl->AddImageRounded(tex, ta, tb, ImVec2(0.05f, 0), ImVec2(0.95f, 1), IM_COL32_WHITE, 10 * s);
        else { dl->AddRectFilled(ta, tb, C_CARD, 10 * s); stPicto(dl, "brick", 2, 4, ImVec2((ta.x + tb.x) * 0.5f, (ta.y + tb.y) * 0.5f), 30 * s, C_DIM, 1.2f * s); }
        const std::string nm = m_modelName.empty() ? std::string("Ma scène") : m_modelName;
        const ImVec2 na(tb.x + 10 * s, ta.y), nb(b.x - 56 * s, ta.y + 28 * s);
        if (m_hubEdit == -2) {
            if (int r = bqEditField(u, na, nb, 13)) { const int i = m_hubEdit; m_hubEdit = -1; if (r == 1 && m_hubName[0]) hubRename(i, m_hubName); }
        } else {
            const bool hn = u.hover(na, nb) && !editing;
            if (hn) dl->AddRectFilled(na, nb, IM_COL32(255, 255, 255, 10), 7 * s);
            u.text(ImVec2(na.x + 4 * s, na.y + 4 * s), 14, C_TEXT, stFit(u, 14, nm, nb.x - na.x - 8 * s, true).c_str(), true);
            if (hn && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Renommer");
            if (hn && u.pressed) { m_hubEdit = -2; m_hubEditFocus = true; snprintf(m_hubName, sizeof(m_hubName), "%s", nm.c_str()); }
        }
        char info[64];
        snprintf(info, sizeof(info), "%d pièces", (int)m_bricks.size());
        u.text(ImVec2(na.x + 4 * s, na.y + 30 * s), 11, C_DIM, inLib ? (std::string(info) + " · " + hubDate(hubFileTime(m_scenePath))).c_str() : info);
        {   // the yellow play: back to the scene
            const ImVec2 pa(b.x - 52 * s, ta.y + 10 * s), pb(b.x - 14 * s, ta.y + 48 * s);
            const bool h = u.hover(pa, pb) && !editing;
            bqYellow(dl, pa, pb, 10 * s, s, h);
            stIcon(dl, BQ_PLAY, ImVec2((pa.x + pb.x) * 0.5f + 1 * s, (pa.y + pb.y) * 0.5f - 1 * s), 18 * s, C_INK, 1.0f);
            if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Reprendre la construction");
            if (h && u.pressed) close = true;
        }
        {   // Enregistrer, Vignette
            const float bw = (b.x - a.x - 36 * s - 8 * s) * 0.5f;
            const ImVec2 sa(a.x + 18 * s, b.y - 34 * s), sb(sa.x + bw, b.y - 8 * s);
            const ImVec2 va(sb.x + 8 * s, sa.y), vb(va.x + bw, sb.y);
            const bool hs = u.hover(sa, sb) && !editing, hv = u.hover(va, vb) && !editing;
            bqRaise(dl, sa, sb, 9 * s, s, hs ? C_CARD_HI : C_CARD, 30);
            bqRaise(dl, va, vb, 9 * s, s, hv ? C_CARD_HI : C_CARD, 30);
            stIcon(dl, BQ_SAVE, ImVec2(sa.x + 16 * s, (sa.y + sb.y) * 0.5f), 14 * s, C_TEXT, 1.6f * s);
            u.text(ImVec2(sa.x + 30 * s, (sa.y + sb.y) * 0.5f - u.fs(11) * 0.58f), 11, C_TEXT, "Enregistrer", true);
            stIcon(dl, BQ_CAMERA, ImVec2(va.x + 16 * s, (va.y + vb.y) * 0.5f), 14 * s, C_TEXT, 1.6f * s);
            u.text(ImVec2(va.x + 30 * s, (va.y + vb.y) * 0.5f - u.fs(11) * 0.58f), 11, C_TEXT, "Vignette", true);
            if (hs && u.pressed) { if (m_scenePath.empty() || inLib) hubSaveHere(); else saveNow(); }
            if (hv && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("La vue 3D actuelle devient la vignette de la scène");
            if (hv && u.pressed) { if (inLib) m_thumbReq = m_scenePath; else hubSaveHere(); }
        }
        ry += ch + 16 * s;
    }
    {   // Outils rapides
        const ImVec2 a(rx0, ry), b(rx1, d.y - 18 * s);
        bqRaise(dl, a, b, 18 * s, s, C_PANEL, 60);
        u.text(ImVec2(a.x + 18 * s, a.y + 14 * s), 17, C_TEXT, "Outils rapides", true);
        struct Q { const char* ico; const char* label; int id; };
        const Q qs[5] = {{BQ_CAMERA, m_ipad ? "Rendu HQ" : "Rendu photo", 0}, {BQ_BUILD, "Pièces", 1}, {BQ_PALETTE, "Couleurs", 2}, {BQ_BOOK, "Notice", 3}, {BQ_EXPORT, "Exporter", 4}};
        float qy = a.y + 48 * s;
        for (const Q& q : qs) {
            if (qy + 46 * s > b.y - 10 * s) break;
            const ImVec2 qa(a.x + 14 * s, qy), qb(b.x - 14 * s, qy + 46 * s);
            const bool h = u.hover(qa, qb) && !editing;
            bqRaise(dl, qa, qb, 12 * s, s, h ? C_CARD_HI : C_CARD, 40);
            stIcon(dl, q.ico, ImVec2(qa.x + 24 * s, (qa.y + qb.y) * 0.5f), 21 * s, C_TEXT, 2.0f * s);
            u.text(ImVec2(qa.x + 48 * s, (qa.y + qb.y) * 0.5f - u.fs(13.5f) * 0.58f), 13.5f, C_TEXT, q.label, true);
            stIcon(dl, BQ_CHEV, ImVec2(qb.x - 18 * s, (qa.y + qb.y) * 0.5f), 16 * s, C_TEXT, 2.4f * s);
            if (h && u.pressed) {
                switch (q.id) {
                case 0: if (m_ipad) { m_quality = 1; } else { setRC(false); m_rs.raster = false; applyLook(); } m_renderer->resetAccumulation(); close = true; break;
                case 1: m_paintMode = false; m_paintTool = false; close = true; break;
                case 2: m_paintMode = true; m_paintTool = true; m_pColors.visible = true; m_pColors.collapsed = false; close = true; break;
                case 3: m_pManual.visible = true; close = true; break;
                case 4: m_popup = 4; m_popupAt = ImVec2(d.x - 300 * s, top + 6 * s); return;
                }
            }
            qy += 54 * s;
        }
    }

    // ======== the middle: the banner, Mes scènes
    const float mx0 = LW + 24 * s, mx1 = rx0 - 18 * s;
    float my = top + 16 * s;
    {   // the banner: a brick castle on the right, the welcome on the left, three glass buttons
        const float bh = std::clamp((mx1 - mx0) * 0.25f, 170 * s, 230 * s);
        const ImVec2 a(mx0, my), b(mx1, my + bh);
        dl->AddRectFilled(ImVec2(a.x, a.y + 5 * s), ImVec2(b.x, b.y + 7 * s), IM_COL32(0, 0, 0, 70), 18 * s);
        dl->AddRectFilled(a, b, IM_COL32(0x34, 0x3b, 0x4c, 255), 18 * s);
        if (ImTextureID ht = themeTex("brique/hero.jpg")) {
            const float iw = (b.x - a.x) * 0.66f, ar = 720.0f / 1280.0f;
            const float vh = (b.y - a.y) / iw / ar;   // (the part of the image that fills the height)
            const float v0 = std::max(0.0f, (1 - vh) * 0.45f);
            dl->AddImageRounded(ht, ImVec2(b.x - iw, a.y), b, ImVec2(0, v0), ImVec2(1, std::min(1.0f, v0 + vh)), IM_COL32_WHITE, 18 * s, ImDrawFlags_RoundCornersRight);
            const float fx0 = b.x - iw, fx1 = fx0 + iw * 0.45f;
            dl->AddRectFilledMultiColor(ImVec2(fx0, a.y), ImVec2(fx1, b.y), IM_COL32(0x34, 0x3b, 0x4c, 255), IM_COL32(0x34, 0x3b, 0x4c, 0), IM_COL32(0x34, 0x3b, 0x4c, 0), IM_COL32(0x34, 0x3b, 0x4c, 255));
        }
        for (int i = 0; i < 10; ++i) {   // a warm glow at the bottom left
            const float rr = (10 - i) * 16 * s;
            dl->PushClipRect(a, b, true);
            dl->AddCircleFilled(ImVec2(a.x + 30 * s, b.y + 20 * s), rr, IM_COL32(255, 160, 40, 9), 40);
            dl->PopClipRect();
        }
        ImFont* f = u.bold;
        const float px = 26;
        float tx = a.x + 34 * s;
        const float ty = a.y + 26 * s;
        dl->AddText(f, u.fs(px), ImVec2(tx, ty), C_TEXT, "Bienvenue dans ");
        tx += f->CalcTextSizeA(u.fs(px), 1e9f, 0, "Bienvenue dans ").x;
        dl->AddText(f, u.fs(px), ImVec2(tx, ty), C_ACCENT, "BrickIt");
        u.text(ImVec2(a.x + 36 * s, ty + u.fs(px) * 1.25f), 13.5f, IM_COL32(0xe2, 0xe5, 0xec, 255), "Construire · Imaginer · Assembler · Rendre", true);
        struct B { const char* ico; const char* label; int id; };
        const B bs[3] = {{ST_NEW, "Nouvelle scène", 0}, {BQ_DEMO, "Démos", 1}, {ST_OPEN, "Ouvrir…", 2}};
        float bx = a.x + 34 * s;
        const float bw = 118 * s, bhh = 68 * s, by = b.y - bhh - 20 * s;
        for (const B& bb0 : bs) {
            const ImVec2 qa(bx, by), qb(bx + bw, by + bhh);
            const bool h = u.hover(qa, qb) && !editing;
            dl->AddRectFilled(qa, qb, IM_COL32(255, 255, 255, h ? 40 : 22), 14 * s);
            dl->AddRect(qa, qb, IM_COL32(255, 255, 255, 52), 14 * s, 0, 1.0f);
            stIcon(dl, bb0.ico, ImVec2((qa.x + qb.x) * 0.5f, qa.y + 24 * s), 24 * s, bb0.id == 0 ? C_ACCENT : C_TEXT, bb0.id == 0 ? 3.0f * s : 2.0f * s);
            u.text(ImVec2((qa.x + qb.x - u.textW(12, bb0.label, true)) * 0.5f, qb.y - 26 * s), 12, C_TEXT, bb0.label, true);
            if (h && u.pressed) {
                if (bb0.id == 0) { keepOpen(); newScene(); close = true; }
                else if (bb0.id == 1) { m_hubSection = 3; m_hubScroll = 0; }
                else { keepOpen(); openDialog(); close = true; }
            }
            bx += bw + 12 * s;
        }
        my += bh + 18 * s;
    }
    {   // Mes scènes / Récentes / Cloud / Démos, in cards
        const ImVec2 a(mx0, my), b(mx1, d.y - 18 * s);
        bqRaise(dl, a, b, 18 * s, s, C_PANEL, 60);
        const char* titles[4] = {"Mes scènes", "Récentes", "Cloud", "Démos"};
        stIcon(dl, m_hubSection == 2 ? BQ_CLOUD : m_hubSection == 3 ? BQ_DEMO : BQ_FOLDER, ImVec2(a.x + 30 * s, a.y + 30 * s), 24 * s, C_TEXT, 2.0f * s);
        u.text(ImVec2(a.x + 52 * s, a.y + 30 * s - u.fs(18) * 0.6f), 18, C_TEXT, titles[m_hubSection], true);
        if (m_hubSection == 2 && m_cloud.signedIn()) {   // refresh
            const ImVec2 c(b.x - 34 * s, a.y + 30 * s);
            const bool h = u.hover(ImVec2(c.x - 16 * s, c.y - 16 * s), ImVec2(c.x + 16 * s, c.y + 16 * s)) && !editing;
            bqRaise(dl, ImVec2(c.x - 16 * s, c.y - 16 * s), ImVec2(c.x + 16 * s, c.y + 16 * s), 10 * s, s, h ? C_CARD_HI : C_CARD, 30);
            stIcon(dl, ST_SYNC, c, 16 * s, m_cloudListBusy ? C_ACCENT : C_TEXT, 1.6f * s);
            if (h && u.pressed) cloudRefreshList();
        }
        // the cards
        struct Card { int kind; int i; };   // 0 here, 1 the cloud, 2 a demo
        std::vector<Card> cards;
        if (m_hubSection == 0) for (int i = 0; i < (int)m_hubLocal.size(); ++i) cards.push_back({0, i});
        else if (m_hubSection == 1) for (int i = 0; i < (int)m_hubLocal.size() && i < 8; ++i) cards.push_back({0, i});
        else if (m_hubSection == 2) { if (m_cloud.signedIn()) for (int i = 0; i < (int)m_cloudList.size(); ++i) cards.push_back({1, i}); }
        else { cards.push_back({2, 0}); cards.push_back({2, 1}); }
        const ImVec2 ga(a.x + 18 * s, a.y + 58 * s), gb(b.x - 18 * s, b.y - 16 * s);
        const float gap = 14 * s, gw = gb.x - ga.x;
        const int cols = std::max(2, (int)((gw + gap) / (210 * s + gap)));
        const float cw = std::floor((gw - (cols - 1) * gap) / cols), thH = std::floor(cw * 0.55f), cH = thH + 58 * s;
        const int rows = ((int)cards.size() + cols - 1) / cols;
        const float contentH = rows * (cH + gap) - gap;
        if (u.hover(ga, gb) && ImGui::GetIO().MouseWheel != 0) m_hubScroll -= ImGui::GetIO().MouseWheel * (cH + gap) * 0.5f;
        m_hubScroll = std::clamp(m_hubScroll, 0.0f, std::max(0.0f, contentH - (gb.y - ga.y)));
        dl->PushClipRect(ga, gb, true);
        const bool inGrid = u.hover(ga, gb);
        for (int k = 0; k < (int)cards.size(); ++k) {
            const Card cd = cards[k];
            const ImVec2 ca(ga.x + (k % cols) * (cw + gap), ga.y + (k / cols) * (cH + gap) - m_hubScroll), cb(ca.x + cw, ca.y + cH);
            if (cb.y < ga.y || ca.y > gb.y) continue;
            const ImVec2 ta(ca.x, ca.y), tb(cb.x, ca.y + thH);
            const bool h = inGrid && u.hover(ca, cb) && !editing;
            std::string name, sub;
            ImTextureID tex = 0;
            bool cur = false, cloudMark = false;
            const char* ico = nullptr;
            if (cd.kind == 0) {
                const HubScene& sc = m_hubLocal[cd.i];
                name = sc.name;
                sub = hubDate(sc.mtime) + " · " + std::to_string(sc.bricks) + " pièces";
                tex = hubThumb(sc.path);
                cur = sc.path == m_scenePath;
                cloudMark = cur && !m_cloudId.empty();
            } else if (cd.kind == 1) {
                const CloudScene& c = m_cloudList[cd.i];
                name = c.name.empty() ? std::string("Ma scène") : c.name;
                sub = Cloud::localDate(c.updated) + " · " + std::to_string(c.bricks) + " pièces";
                cur = c.id == m_cloudId;
                cloudMark = true;
                ico = BQ_CLOUD;
            } else {
                name = cd.i == 0 ? "Maison (démo)" : "Pelleteuse et mécanismes";
                sub = cd.i == 0 ? "Le set 31153 et ses figurines" : "4 moteurs, 2 vérins";
                ico = cd.i == 0 ? "M4 11 12 4l8 7v9H4zM10 20v-5h4v5" : ST_MECH;
            }
            bqRaise(dl, ca, cb, 14 * s, s, h ? IM_COL32(0x3a, 0x40, 0x4d, 255) : C_CARD, 50);
            if (tex) dl->AddImageRounded(tex, ta, tb, ImVec2(0, 0.05f), ImVec2(1, 0.95f), IM_COL32_WHITE, 14 * s, ImDrawFlags_RoundCornersTop);
            else {
                bqGrad(dl, ta, tb, 14 * s, IM_COL32(0x3c, 0x43, 0x52, 255), IM_COL32(0x2c, 0x31, 0x3c, 255), ImDrawFlags_RoundCornersTop);
                if (ico) stIcon(dl, ico, ImVec2((ta.x + tb.x) * 0.5f, (ta.y + tb.y) * 0.5f), thH * 0.36f, IM_COL32(0x9a, 0xa0, 0xac, 255), 1.6f * s);
                else stPicto(dl, "brick", 2, 4, ImVec2((ta.x + tb.x) * 0.5f, (ta.y + tb.y) * 0.5f), thH * 0.45f, IM_COL32(0x9a, 0xa0, 0xac, 255), 1.2f * s);
            }
            if (cur) dl->AddRect(ImVec2(ca.x - 1 * s, ca.y - 1 * s), ImVec2(cb.x + 1 * s, cb.y + 1 * s), C_ACCENT, 15 * s, 0, 3.0f * s);
            if (cur) {   // OUVERTE
                const char* t = "OUVERTE";
                const float w = stTrackW(u, 7, t, 1.2f * s, true) + 16 * s;
                const ImVec2 pa(ta.x + 10 * s, ta.y + 10 * s), pb(pa.x + w, pa.y + 20 * s);
                bqYellow(dl, pa, pb, 8 * s, s);
                stTrack(u, ImVec2(pa.x + 8 * s, (pa.y + pb.y) * 0.5f - u.fs(7) * 0.55f), 7, C_INK, t, 1.2f * s);
            }
            const float ny = tb.y + 9 * s;
            if (cd.kind == 0 && m_hubEdit == cd.i) {
                if (int r = bqEditField(u, ImVec2(ca.x + 8 * s, ny - 2 * s), ImVec2(cb.x - 8 * s, ny + 22 * s), 13)) { const int i = m_hubEdit; m_hubEdit = -1; if (r == 1 && m_hubName[0]) hubRename(i, m_hubName); }
            } else u.text(ImVec2(ca.x + 14 * s, ny), 14, C_TEXT, stFit(u, 14, name, cw - 70 * s, true).c_str(), true);
            u.text(ImVec2(ca.x + 14 * s, ny + 22 * s), 11, C_DIM, stFit(u, 11, sub, cw - 60 * s, false).c_str());
            if (cloudMark) stIcon(dl, BQ_CLOUD, ImVec2(cb.x - 22 * s, ny + 18 * s), 19 * s, cur ? IM_COL32(0x5b, 0xe0, 0x9b, 255) : C_TEXT, 1.6f * s);
            // the actions (hover; always on the iPad): rename, duplicate, delete
            bool act = false;
            if ((h || m_ipad) && cd.kind != 2) {
                float bx = tb.x - 22 * s;
                const float by = ta.y + 22 * s, r = 15 * s;
                auto rb = [&](const char* icon, const char* tip, bool danger) {
                    const ImVec2 c(bx, by);
                    const bool hb = (u.mouse.x - c.x) * (u.mouse.x - c.x) + (u.mouse.y - c.y) * (u.mouse.y - c.y) < r * r && !editing;
                    dl->AddCircleFilled(c, r, hb ? (danger ? IM_COL32(200, 50, 45, 235) : IM_COL32(0x3a, 0x40, 0x4d, 245)) : IM_COL32(20, 23, 30, 215), 24);
                    stIcon(dl, icon, c, r * 1.05f, IM_COL32(240, 242, 246, 255), 1.6f * s);
                    if (hb && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", tip);
                    bx -= 2 * r + 6 * s;
                    return hb && u.pressed;
                };
                if (rb(ST_DEL, cd.kind == 1 ? "Supprimer du cloud" : "Supprimer", true)) {
                    act = true;
                    if (cd.kind == 0) hubDelete(cd.i);
                    else {
                        const CloudScene cs = m_cloudList[cd.i];
                        plat::confirm(L"Supprimer du cloud", plat::fromUtf8("Supprimer « " + (cs.name.empty() ? std::string("Ma scène") : cs.name) + " » du cloud ?"), [this, cs] {
                            m_cloud.remove(cs.id, [this, cs](bool ok, const std::string& err) {
                                if (!ok) { toast("Cloud : " + err); return; }
                                if (cs.id == m_cloudId) cloudForget();
                                toast("Cloud : « " + cs.name + " » supprimée");
                                cloudRefreshList();
                            });
                        });
                    }
                }
                if (cd.kind == 0 && rb(ST_DUP, "Dupliquer", false)) { act = true; hubDuplicate(cd.i); }
                if (cd.kind == 0 && rb(ST_PEN, "Renommer", false)) { act = true; m_hubEdit = cd.i; m_hubEditFocus = true; snprintf(m_hubName, sizeof(m_hubName), "%s", name.c_str()); }
            }
            if (act) continue;
            if (h && u.pressed) {   // open it
                if (cd.kind == 0) {
                    const HubScene sc = m_hubLocal[cd.i];
                    if (sc.path != m_scenePath) { keepOpen(); if (openScene(sc.path)) m_modelName = sc.name; }
                    close = true;
                } else if (cd.kind == 1) {
                    const CloudScene c = m_cloudList[cd.i];
                    if (!cur) { keepOpen(); cloudOpen(c, false); }
                    close = true;
                } else demo(cd.i);
            }
        }
        dl->PopClipRect();
        if (cards.empty()) {
            const char* t = m_hubSection <= 1 ? "Aucune scène enregistrée ici pour l'instant : « Enregistrer » range la scène ouverte dans Mes scènes, avec sa vignette."
                          : !m_cloud.signedIn() ? "Connecte-toi (Stockage, à droite) pour retrouver tes scènes de l'iPad et du PC."
                          : m_cloudListBusy ? "Chargement…" : !m_cloudListErr.empty() ? m_cloudListErr.c_str() : "Aucune scène dans le cloud pour l'instant.";
            u.text(ImVec2(ga.x + 4 * s, ga.y + 8 * s), 12, C_DIM, t);
        }
        if (contentH > gb.y - ga.y) {
            const float vh = gb.y - ga.y, bh = std::max(30 * s, vh * vh / contentH), by = ga.y + (vh - bh) * (m_hubScroll / (contentH - vh));
            dl->AddRectFilled(ImVec2(gb.x + 6 * s, by), ImVec2(gb.x + 10 * s, by + bh), IM_COL32(255, 255, 255, 60), 2 * s);
        }
    }
    (void)now;
    if (!editing && ImGui::IsKeyPressed(ImGuiKey_Escape)) close = true;
    if (close && m_popup == 8) m_popup = 0;
}

// an edit field (rename) over a rectangle: 1 Entrée, -1 Échap, 0 typing
int BrickApp::bqEditField(Ui& u, ImVec2 ea, ImVec2 eb, float px) {
    const float s = u.s;
    u.dl->AddRectFilled(ea, eb, IM_COL32(255, 255, 255, 14), 8 * s);
    u.dl->AddRect(ea, eb, stA(C_ACCENT, 0.9f), 8 * s, 0, 1.5f * s);
    ImGui::SetNextWindowPos(ImVec2(ea.x + 8 * s, ea.y));
    ImGui::SetNextWindowSize(ImVec2(eb.x - ea.x - 12 * s, eb.y - ea.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, std::max(0.0f, (eb.y - ea.y - u.fs(px)) * 0.5f)));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C_TEXT));
    ImGui::Begin("##bqedit", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImGui::PushFont(u.bold, u.fs(px));
    ImGui::SetNextItemWidth(-1);
    if (m_hubEditFocus) { ImGui::SetKeyboardFocusHere(); m_hubEditFocus = false; }
    const bool enter = ImGui::InputText("##n", m_hubName, sizeof(m_hubName), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    const bool lost = !ImGui::IsItemActive() && !ImGui::IsItemFocused() && !enter;
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) return -1;
    if (enter || (lost && u.pressed && !u.hover(ea, eb))) return 1;
    return 0;
}
