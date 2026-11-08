// BrickIt - the interface of the iPad (included at the end of BrickUi.cpp: the same drawing helpers and colours).
// Maquette v5 « Tiny Glade » validated by Wzx (08/11/2026, docs: /mnt/project-files/brickit_ipad/maquette_tinyglade):
//   - a bright warm stage (cream floor, morning light), no bars: round cream « pucks » on the edges of the screen:
//     Maison (top left: files, quality, light, grid, Pencil only), the name of the scene (top middle), undo / redo (top
//     right), the dock of the 8 last parts (bottom middle, the first button opens the library drawer), the colour stud
//     and Recadrer (bottom right), the compass (bottom left);
//   - around the selection, 4 translucent brown bubbles: N turn, W duplicate, E colour, S move, and a small « … »
//     (delete, group, isolate, take its colour, part sheet);
//   - colour: an arc of stud swatches around the brick (the current one, the colours of the scene), « + » opens the
//     Nuancier (families, filters Toutes / Dans la scène / Pour la pièce);
//   - the library: a drawer from the bottom (the tray of the PC in the warm theme, large cards);
//   - Nouvelle scène: three cards (green baseplate, free floor, from instructions) and the scenes to resume.
// The PC keeps its interface (m_ipad false), the editing underneath is the same code.

static void uiTheme(bool warm) {
    if (warm) {   // cream paper, brown ink (maquette v5)
        C_BG = IM_COL32(0xf3, 0xea, 0xdb, 255);
        C_PANEL = IM_COL32(0xfa, 0xf4, 0xea, 246);
        C_PANEL_SOLID = IM_COL32(0xfa, 0xf4, 0xea, 255);
        C_CARD = IM_COL32(0xff, 0xff, 0xff, 255);
        C_CARD_HI = IM_COL32(0xf0, 0xe6, 0xd4, 255);
        C_LINE = IM_COL32(0xe6, 0xda, 0xc6, 255);
        C_TEXT = IM_COL32(0x4a, 0x3a, 0x2c, 255);
        C_DIM = IM_COL32(0x8a, 0x77, 0x64, 255);
        C_FAINT = IM_COL32(0xb0, 0x9f, 0x8a, 255);
        C_ACCENT = IM_COL32(0xf2, 0xb8, 0x1c, 255);
        C_INK = IM_COL32(0x3d, 0x2c, 0x1c, 255);
        C_OVERLAY = IM_COL32(0xfa, 0xf4, 0xea, 236);
        C_HUD = IM_COL32(0xfa, 0xf4, 0xea, 210);
    } else {
        C_BG = IM_COL32(0x13, 0x14, 0x17, 255);
        C_PANEL = IM_COL32(24, 25, 29, 235);
        C_PANEL_SOLID = IM_COL32(0x1b, 0x1c, 0x20, 255);
        C_CARD = IM_COL32(0x25, 0x26, 0x2b, 255);
        C_CARD_HI = IM_COL32(0x30, 0x32, 0x3a, 255);
        C_LINE = IM_COL32(0x2f, 0x31, 0x37, 255);
        C_TEXT = IM_COL32(0xec, 0xec, 0xee, 255);
        C_DIM = IM_COL32(0x9a, 0x9c, 0xa4, 255);
        C_FAINT = IM_COL32(0x67, 0x6a, 0x72, 255);
        C_ACCENT = IM_COL32(0xf7, 0xc5, 0x1e, 255);
        C_INK = IM_COL32(0x17, 0x17, 0x1a, 255);
        C_OVERLAY = IM_COL32(15, 16, 19, 219);
        C_HUD = IM_COL32(15, 16, 19, 184);
    }
}

// ------------------------------------------------------------------ drawing kit of the iPad
static const ImU32 K_PUCK = IM_COL32(0xfb, 0xf6, 0xee, 244);       // cream puck
static const ImU32 K_INK = IM_COL32(0x4a, 0x3a, 0x2c, 255);        // brown icons and text
static const ImU32 K_INK2 = IM_COL32(0x9a, 0x86, 0x70, 255);       // secondary text
static const ImU32 K_BUBBLE = IM_COL32(0x6b, 0x55, 0x40, 178);     // translucent brown bubble
static const ImU32 K_YELLOW = IM_COL32(0xf5, 0xc5, 0x2a, 255);

// the icons of the maquette v5 (docs/brickit/maquette_ipad/icones_maquette, viewBox 24, round stroke 2.6) centred on c, `size` pixels wide
static void lucide(ImDrawList* dl, const char* d, ImVec2 c, float size, ImU32 col, float stroke = 2.0f) {
    svg::path(dl, d, ImVec2(c.x - size * 0.5f, c.y - size * 0.5f), size / 24.0f, col, stroke * 1.2f * size / 24.0f);   // 2.1 -> the 2.6 of the maquette at 52 pt
}
static const char* IC_HOUSE = "M3.5 11 12 4l8.5 7 M6 9.5V20h12V9.5 M10 20v-5h4v5";   // maison.svg
static const char* IC_UNDO = "M9 5 4 10l5 5 M4 10h10a6 6 0 0 1 0 12h-3";   // annuler.svg
static const char* IC_REDO = "M15 5l5 5-5 5 M20 10H10a6 6 0 0 0 0 12h3";   // retablir.svg
static const char* IC_ROTATE = "M20 12a8 8 0 1 1-2.4-5.7 M19.5 3.5v5h-5";   // tourner.svg
static const char* IC_COPY = "M11.5 3 H18 a3 3 0 0 1 3 3 V12.5 a3 3 0 0 1 -3 3 H11.5 a3 3 0 0 1 -3 -3 V6 a3 3 0 0 1 3 -3 Z M5 8.5v9.5a3 3 0 0 0 3 3h9.5 M14.75 6.5v6M11.75 9.25h6";   // dupliquer.svg
static const char* IC_PALETTE = "M12 3a9 9 0 1 0 0 18c1.6 0 2.2-1 2.2-2c0 -1 -1 -1.6 -1 -2.6c0 -1 1 -1.8 2 -1.8h2a4 4 0 0 0 4-4C21.2 6.2 17.2 3 12 3z M6.9 11 a.6 .6 0 1 0 1.2 0 a.6 .6 0 1 0 -1.2 0 M9.9 7.2 a.6 .6 0 1 0 1.2 0 a.6 .6 0 1 0 -1.2 0 M14.4 7.5 a.6 .6 0 1 0 1.2 0 a.6 .6 0 1 0 -1.2 0";   // couleur.svg (s -> c)   // couleur.svg
static const char* IC_MOVE = "M12 3v18M3 12h18M9 6l3-3 3 3M9 18l3 3 3-3M6 9l-3 3 3 3M18 9l3 3-3 3";   // deplacer.svg
static const char* IC_TRASH = "M4 7h16M9.5 7V4h5v3M6.5 7l1 13h9l1-13";   // supprimer.svg
static const char* IC_GROUP = "M5.5 3.5 H10.5 a2 2 0 0 1 2 2 V10.5 a2 2 0 0 1 -2 2 H5.5 a2 2 0 0 1 -2 -2 V5.5 a2 2 0 0 1 2 -2 Z M13.5 11.5 H18.5 a2 2 0 0 1 2 2 V18.5 a2 2 0 0 1 -2 2 H13.5 a2 2 0 0 1 -2 -2 V13.5 a2 2 0 0 1 2 -2 Z";   // grouper.svg
static const char* IC_EYE = "M2.5 12 C2.5 12 6 5.5 12 5.5 C18 5.5 21.5 12 21.5 12 C21.5 12 18 18.5 12 18.5 C6 18.5 2.5 12 2.5 12 Z M9.4 12 a2.6 2.6 0 1 0 5.2 0 a2.6 2.6 0 1 0 -5.2 0";   // isoler.svg
static const char* IC_PIPETTE = "m14 4 6 6 M17 7 7.5 16.5 4 20l3.5-3.5 M15.5 5.5 l3 3";   // pipette.svg
static const char* IC_INFO = "M3.5 12 a8.5 8.5 0 1 0 17 0 a8.5 8.5 0 1 0 -17 0 M12 11v5.5 M11.5 7.8 a0.5 0.5 0 1 0 1 0 a0.5 0.5 0 1 0 -1 0";   // fiche.svg
static const char* IC_PLUS = "M12 5v14M5 12h14";   // ajouter.svg
static const char* IC_X = "M6 6l12 12M18 6 6 18";   // fermer.svg
static const char* IC_FOLDER = "M3 7.5a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2V18a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z";   // ouvrir.svg
static const char* IC_SAVE = "M12 4v11M7.5 10.5 12 15l4.5-4.5 M4.5 16v3.5h15V16";   // enregistrer.svg
static const char* IC_SHARE = "M12 15 V3 M8 7 L12 3 L16 7 M5 12 V19 a2 2 0 0 0 2 2 H17 a2 2 0 0 0 2 -2 V12";
static const char* IC_BOOK = "M12 6.5C10 4.8 7 4.5 3.5 5v13.5c3.5-.5 6.5-.2 8.5 1.5 2-1.7 5-2 8.5-1.5V5c-3.5-.5-6.5-.2-8.5 1.5zM12 6.5V20";   // notice.svg
static const char* IC_SCAN = "M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5 M10 12 a2 2 0 1 0 4 0 a2 2 0 1 0 -4 0";   // recadrer.svg
static const char* IC_BRICK = "M3 11h18v8.5H3z M6 11V7.5h4V11M14 11V7.5h4V11";   // bibliotheque.svg
static const char* IC_MORE = "M3.8 12 a1.2 1.2 0 1 0 2.4 0 a1.2 1.2 0 1 0 -2.4 0 M10.8 12 a1.2 1.2 0 1 0 2.4 0 a1.2 1.2 0 1 0 -2.4 0 M17.8 12 a1.2 1.2 0 1 0 2.4 0 a1.2 1.2 0 1 0 -2.4 0";   // plus_actions.svg
static const char* IC_SEARCH = "M4.5 10.5 a6 6 0 1 0 12 0 a6 6 0 1 0 -12 0 M15.5 15.5 20 20";   // chercher.svg

static void puckShadow(ImDrawList* dl, ImVec2 c, float r, float s) {
    dl->AddCircleFilled(ImVec2(c.x, c.y + 3 * s), r + 1.5f * s, IM_COL32(70, 45, 20, 26), 40);
    dl->AddCircleFilled(ImVec2(c.x, c.y + 1.5f * s), r + 0.5f * s, IM_COL32(70, 45, 20, 22), 40);
}

// a round cream button; true when tapped
static bool puck(Ui& u, ImVec2 c, float r, const char* icon, bool enabled = true, ImU32 fill = K_PUCK, ImU32 ink = K_INK) {
    const float s = u.s;
    const bool h = enabled && u.hover(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
    const bool down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const float rr = r * (down ? 0.94f : 1.0f);
    puckShadow(u.dl, c, rr, s);
    u.dl->AddCircleFilled(c, rr, fill, 48);
    if (icon) lucide(u.dl, icon, c, r * 0.95f, enabled ? ink : alphaOf(ink, 0.35f), 2.1f);
    return h && u.pressed;
}

static void pill(Ui& u, ImVec2 a, ImVec2 b, ImU32 fill = K_PUCK) {
    const float s = u.s, r = (b.y - a.y) * 0.5f;
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 3 * s), ImVec2(b.x, b.y + 3 * s), IM_COL32(70, 45, 20, 24), r);
    u.dl->AddRectFilled(a, b, fill, r);
}

// a stud seen from above (the swatches of the colours): the colour, a lighter top, a soft rim
static void stud(ImDrawList* dl, ImVec2 c, float r, const Color& col, float s, bool selected) {
    auto mix = [](float3 a, float k) { return IM_COL32((int)std::min(255.0f, a.x * 255 * k), (int)std::min(255.0f, a.y * 255 * k), (int)std::min(255.0f, a.z * 255 * k), 255); };
    dl->AddCircleFilled(ImVec2(c.x, c.y + 2.5f * s), r + 1 * s, IM_COL32(60, 40, 20, 50), 36);
    dl->AddCircleFilled(c, r, mix(col.srgb, 0.82f), 36);
    dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.08f), r * 0.82f, mix(col.srgb, 1.0f), 36);
    dl->AddCircle(ImVec2(c.x, c.y - r * 0.08f), r * 0.5f, mix(col.srgb, 0.9f), 32, 1.6f * s);
    if (col.group == "transparent") dl->AddCircleFilled(ImVec2(c.x - r * 0.32f, c.y - r * 0.35f), r * 0.22f, IM_COL32(255, 255, 255, 120), 16);
    if (col.group == "metallic") dl->AddCircleFilled(ImVec2(c.x - r * 0.3f, c.y - r * 0.38f), r * 0.18f, IM_COL32(255, 255, 255, 150), 16);
    if (selected) dl->AddCircle(c, r + 4 * s, IM_COL32(255, 255, 255, 255), 40, 3.2f * s);
}

// colour families of the Nuancier (by hue of the sRGB colour)
static int colorFamily(const Color& c) {
    if (c.group == "transparent") return 7;
    if (c.group == "metallic") return 8;
    const float r = c.srgb.x, g = c.srgb.y, b = c.srgb.z;
    const float mx = std::max(r, std::max(g, b)), mn = std::min(r, std::min(g, b)), d = mx - mn;
    const float sat = mx > 0 ? d / mx : 0;
    if (sat < 0.16f || d < 0.06f) return 0;   // whites and greys
    float h = 0;
    if (mx == r) h = std::fmod((g - b) / d, 6.0f);
    else if (mx == g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h *= 60;
    if (h < 0) h += 360;
    const bool earth = (h >= 15 && h < 50 && (mx < 0.62f || sat < 0.45f));
    if (earth) return 6;                        // earths (browns, tans)
    if (h < 15 || h >= 330) return 1;           // reds and pinks
    if (h < 65) return 2;                       // yellows and oranges
    if (h < 170) return 3;                      // greens
    if (h < 255) return 4;                      // blues
    if (h < 330) return 5;                      // violets
    return 0;
}
static const char* kFamilyFr[9] = {"Blancs et gris", "Rouges et roses", "Jaunes et oranges", "Verts", "Bleus", "Violets", "Terres", "Transparentes", "Métal"};

// ------------------------------------------------------------------ the whole interface of the iPad

void BrickApp::drawUiIpad() {
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // the stage: the whole screen (index 0 / 1 of m_uiRects as the PC: the top bar and the tray), the drawer excepted
    m_uiRects.push_back(ImVec4(0, 0, d.x, 0));
    if (m_libOpen) {
        const int ts = m_thumbSize;
        m_thumbSize = 2;                   // large cards
        m_pTray.visible = true;
        m_pTray.floating = false;
        uiTray();                          // m_uiRects[1]
        m_thumbSize = ts;
        if (m_pTray.collapsed) { m_pTray.collapsed = false; m_libOpen = false; }   // the grip closes the drawer
    } else m_uiRects.push_back(ImVec4(0, d.y, d.x, d.y));
    ipadBubbles();
    ipadColorArc();
    ipadPucks();
    uiReader();
    uiManual();
    uiInfoSheet();
    ipadNuancier();
    ipadHomeMenu();
    ipadNewScene();
}

// ------------------------------------------------------------------ pucks on the edges

void BrickApp::ipadPucks() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float R = 24 * s, M = 18 * s;
    const float bottom = m_libOpen ? m_uiRects[1].y : d.y;
    // Maison
    {
        const ImVec2 c(M + R, M + R);
        if (puck(u, c, R, IC_HOUSE)) { m_homeOpen = !m_homeOpen; m_newSceneOpen = false; }
        m_uiRects.push_back(ImVec4(c.x - R, c.y - R, c.x + R, c.y + R));
    }
    // the name of the scene (or of the selection)
    {
        std::string name = "Sans titre";
        if (!m_scenePath.empty()) {
            name = plat::toUtf8(m_scenePath.substr(m_scenePath.find_last_of(L"\\/") + 1));
            for (const char* ext : {".v3d.json", ".json", ".ldr", ".mpd"})
                if (name.size() > strlen(ext) && name.compare(name.size() - strlen(ext), strlen(ext), ext) == 0) { name.resize(name.size() - strlen(ext)); break; }
        }
        char info[96];
        const int focus = m_holding ? m_held.brick : m_selected;
        if (focus >= 0 && focus < (int)m_bricks.size()) {
            const Color& col = m_lib.palette[std::clamp(m_bricks[focus].color, 0, (int)m_lib.palette.size() - 1)];
            name = m_lib.parts[m_bricks[focus].part].label;
            snprintf(info, sizeof(info), "  ·  %s  ·  %s", col.name.c_str(), m_lib.parts[m_bricks[focus].part].name.c_str());
        } else snprintf(info, sizeof(info), "  ·  %d pièces%s", (int)m_bricks.size(), m_autosaveOn ? "  ·  enregistrée" : "");
        if (name.size() > 40) name = name.substr(0, 39) + "…";
        const float w1 = u.textW(13.5f, name.c_str(), true), w2 = u.textW(12, info);
        const float W = 36 * s + w1 + w2 + 18 * s, H = 36 * s;
        const ImVec2 a((d.x - W) * 0.5f, M + R - H * 0.5f), b(a.x + W, a.y + H);
        pill(u, a, b);
        u.dl->AddCircleFilled(ImVec2(a.x + 18 * s, (a.y + b.y) * 0.5f), 4 * s, IM_COL32(0x5c, 0xb8, 0x4f, 255), 16);
        u.text(ImVec2(a.x + 30 * s, a.y + 8.5f * s), 13.5f, K_INK, name.c_str(), true);
        u.text(ImVec2(a.x + 30 * s + w1, a.y + 10 * s), 12, K_INK2, info);
        m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    }
    // undo / redo
    {
        const ImVec2 cr(d.x - M - R, M + R), cu(cr.x - 2 * R - 10 * s, cr.y);
        if (puck(u, cu, R, IC_UNDO, !m_undo.empty() || m_holding)) undo();
        if (puck(u, cr, R, IC_REDO, !m_redo.empty())) redo();
        m_uiRects.push_back(ImVec4(cu.x - R, cu.y - R, cr.x + R, cr.y + R));
    }
    if (m_reading) return;   // reading the instructions: the reader has the bottom of the screen
    // compass (bottom left): the needle turns with the view; a tap turns the view back to the nearest diagonal
    if (!m_libOpen) {
        const ImVec2 c(M + R, bottom - M - R);
        puck(u, c, R, nullptr);
        const float a = -m_scene.camera.yaw;
        const ImVec2 dir(std::sin(a), -std::cos(a)), nrm(-dir.y, dir.x);
        const float L = R * 0.62f, Wd = R * 0.22f;
        u.dl->AddTriangleFilled(ImVec2(c.x + dir.x * L, c.y + dir.y * L), ImVec2(c.x + nrm.x * Wd, c.y + nrm.y * Wd), ImVec2(c.x - nrm.x * Wd, c.y - nrm.y * Wd), IM_COL32(0xd9, 0x4a, 0x38, 255));
        u.dl->AddTriangleFilled(ImVec2(c.x - dir.x * L, c.y - dir.y * L), ImVec2(c.x - nrm.x * Wd, c.y - nrm.y * Wd), ImVec2(c.x + nrm.x * Wd, c.y + nrm.y * Wd), K_INK2);
        u.dl->AddCircleFilled(c, 2.5f * s, K_PUCK, 12);
        if (u.pressed && u.hover(ImVec2(c.x - R, c.y - R), ImVec2(c.x + R, c.y + R))) {
            const float q = 3.14159265f * 0.5f;
            m_scene.camera.yaw = std::round((m_scene.camera.yaw - q * 0.5f) / q) * q + q * 0.5f;
            m_scene.camera.pitch = radians(32.0f);
            touchCamera();
        }
        m_uiRects.push_back(ImVec4(c.x - R, c.y - R, c.x + R, c.y + R));
    }
    // the colour stud and Recadrer (bottom right)
    {
        const float RS = 28 * s;
        const ImVec2 c(d.x - M - RS, bottom - M - RS);
        const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
        stud(u.dl, c, RS, cur, s, false);
        if (u.pressed && u.hover(ImVec2(c.x - RS, c.y - RS), ImVec2(c.x + RS, c.y + RS))) { m_paletteOpen = !m_paletteOpen; m_arcOpen = false; }
        m_uiRects.push_back(ImVec4(c.x - RS, c.y - RS, c.x + RS, c.y + RS));
        const float r2 = 20 * s;
        const ImVec2 c2(c.x, c.y - RS - 14 * s - r2);
        if (puck(u, c2, r2, IC_SCAN)) {
            const std::vector<int> sel = selection();
            if (sel.empty()) frameAll();
            else {   // the selection in the middle, closer
                float3 mn(1e30f), mx(-1e30f);
                for (int i : sel) { Box bx = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos); mn = minv(mn, bx.mn); mx = maxv(mx, bx.mx); }
                m_scene.camera.target = (mn + mx) * 0.5f;
                m_scene.camera.distance = std::max(6.0f, length(mx - mn) * 2.2f);
            }
            touchCamera();
        }
        u.dl->AddCircleFilled(c2, 2.2f * s, K_INK, 12);
        m_uiRects.push_back(ImVec4(c2.x - r2, c2.y - r2, c2.x + r2, c2.y + r2));
    }
    // the dock: the library button, then the 8 last parts (a tap = in hand in the middle, drag up = in hand under the finger)
    if (!m_libOpen) {
        const float cell = 58 * s, gap = 6 * s, pad = 7 * s;
        std::vector<int> parts;
        for (int p : m_recent) if (p >= 0 && p < (int)m_lib.parts.size() && (int)parts.size() < 8) parts.push_back(p);
        if (parts.empty())
            for (const char* nm : {"3001", "3003", "3004", "3010", "3020", "3022", "3062b", "3040"}) { const int p = m_lib.find(nm); if (p >= 0) parts.push_back(p); }
        const float W = pad * 2 + (parts.size() + 1) * cell + parts.size() * gap, H = cell + pad * 2;
        const ImVec2 a((d.x - W) * 0.5f, bottom - M - H), b(a.x + W, a.y + H);
        pill(u, a, b);
        u.dl->AddRectFilled(a, b, K_PUCK, 22 * s);
        u.dl->AddLine(ImVec2(d.x * 0.5f - 18 * s, a.y - 8 * s), ImVec2(d.x * 0.5f + 18 * s, a.y - 8 * s), IM_COL32(120, 100, 80, 90), 4 * s);   // (handle: the drawer)
        m_uiRects.push_back(ImVec4(a.x, a.y - 16 * s, b.x, b.y));
        float x = a.x + pad;
        {   // library
            const ImVec2 ca(x, a.y + pad), cb(x + cell, a.y + pad + cell);
            u.dl->AddRectFilled(ca, cb, K_YELLOW, 16 * s);
            lucide(u.dl, IC_BRICK, ImVec2((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f), 28 * s, K_INK, 2.2f);
            if (u.pressed && u.hover(ca, cb)) { m_libOpen = true; m_homeOpen = false; }
            x += cell + gap;
        }
        // (a press on the handle or a swipe up from the dock also opens the drawer)
        if (u.pressed && u.hover(ImVec2(d.x * 0.5f - 40 * s, a.y - 16 * s), ImVec2(d.x * 0.5f + 40 * s, a.y))) m_libOpen = true;
        for (int p : parts) {
            const ImVec2 ca(x, a.y + pad), cb(x + cell, a.y + pad + cell);
            const bool h = u.hover(ca, cb);
            u.dl->AddRectFilled(ca, cb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? C_CARD_HI : IM_COL32(255, 255, 255, 255), 16 * s);
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (thumbUv(p, tex, uv0, uv1)) {
                const ImVec2 du((uv1.x - uv0.x) * 0.14f, (uv1.y - uv0.y) * 0.14f);
                const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
                // the cards tinted with the colour of the stud (the thumbnails are light grey: multiplied)
                const ImU32 tint = IM_COL32((int)(155 + 100 * cur.srgb.x), (int)(155 + 100 * cur.srgb.y), (int)(155 + 100 * cur.srgb.z), 255);
                u.dl->AddImage(tex, ImVec2(ca.x + 4 * s, ca.y + 4 * s), ImVec2(cb.x - 4 * s, cb.y - 4 * s), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), tint);
            }
            if (h && u.pressed) { m_dockPress = p; m_dockPressPos = u.mouse; }
            x += cell + gap;
        }
        if (m_dockPress >= 0) {
            const float dx = u.mouse.x - m_dockPressPos.x, dy = u.mouse.y - m_dockPressPos.y;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && u.mouse.y < a.y - 6 * s && dx * dx + dy * dy > 49 * s * s) {   // dragged out: in hand under the finger
                spawn(m_dockPress, u.mouse.x, u.mouse.y);
                m_dragStarted = true;   // dropped on release (editInput)
                m_dockPress = -1;
            } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (dx * dx + dy * dy < 100 * s * s) spawn(m_dockPress, d.x * 0.5f, d.y * 0.45f);   // a tap: in hand in the middle
                m_dockPress = -1;
            }
        }
    }
}

// ------------------------------------------------------------------ bubbles around the selection

bool BrickApp::selectionRect(ImVec2& a, ImVec2& b) const {
    const std::vector<int> sel = selection();
    if (sel.empty()) return false;
    View v = view();
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
    for (int i : sel) {
        if (i < 0 || i >= (int)m_bricks.size()) continue;
        Box bx = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
        for (int k = 0; k < 8; ++k) {
            float3 p((k & 1) ? bx.mx.x : bx.mn.x, (k & 2) ? bx.mx.y : bx.mn.y, (k & 4) ? bx.mx.z : bx.mn.z);
            float sx, sy;
            if (!v.project(p, sx, sy)) continue;
            x0 = std::min(x0, sx); y0 = std::min(y0, sy); x1 = std::max(x1, sx); y1 = std::max(y1, sy);
        }
    }
    if (x0 > x1) return false;
    a = ImVec2(x0, y0);
    b = ImVec2(x1, y1);
    return true;
}

void BrickApp::ipadBubbles() {
    ImVec2 a, b;
    const bool show = !m_holding && !m_mechOn && !m_reading && !m_arcOpen && selectionRect(a, b);
    if (!show) { m_moreOpen = false; return; }
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float R = 23 * s, off = R + 10 * s;   // bubbles of 46 pt
    const ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    // a dashed frame around the selection
    {
        const ImVec2 fa(a.x - 6 * s, a.y - 6 * s), fb(b.x + 6 * s, b.y + 6 * s);
        const ImU32 col = IM_COL32(255, 255, 255, 220);
        auto dash = [&](ImVec2 p, ImVec2 q) {
            const float L = std::sqrt((q.x - p.x) * (q.x - p.x) + (q.y - p.y) * (q.y - p.y));
            for (float t = 0; t < L; t += 9 * s) {
                const float t1 = std::min(L, t + 5 * s);
                u.dl->AddLine(ImVec2(p.x + (q.x - p.x) * t / L, p.y + (q.y - p.y) * t / L), ImVec2(p.x + (q.x - p.x) * t1 / L, p.y + (q.y - p.y) * t1 / L), col, 2 * s);
            }
        };
        dash(fa, ImVec2(fb.x, fa.y)); dash(ImVec2(fb.x, fa.y), fb); dash(fb, ImVec2(fa.x, fb.y)); dash(ImVec2(fa.x, fb.y), fa);
    }
    auto clampP = [&](ImVec2 p) { return ImVec2(std::clamp(p.x, R + 8 * s, d.x - R - 8 * s), std::clamp(p.y, R + 70 * s, d.y - R - 100 * s)); };
    struct Bub { ImVec2 p; const char* icon; int act; };
    const Bub bubs[4] = {{clampP(ImVec2(c.x, a.y - off)), IC_ROTATE, 0}, {clampP(ImVec2(a.x - off, c.y)), IC_COPY, 1},
                         {clampP(ImVec2(b.x + off, c.y)), IC_PALETTE, 2}, {clampP(ImVec2(c.x, b.y + off)), IC_MOVE, 3}};
    for (const Bub& q : bubs) {
        const bool h = u.hover(ImVec2(q.p.x - R, q.p.y - R), ImVec2(q.p.x + R, q.p.y + R));
        const bool down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const float r = R * (down ? 0.92f : 1.0f);
        u.dl->AddCircleFilled(ImVec2(q.p.x, q.p.y + 2.5f * s), r + 1 * s, IM_COL32(40, 25, 10, 40), 40);
        u.dl->AddCircleFilled(q.p, r, K_BUBBLE, 40);
        lucide(u.dl, q.icon, q.p, R * 0.95f, IM_COL32(255, 255, 255, 245), 2.3f);
        m_uiRects.push_back(ImVec4(q.p.x - R, q.p.y - R, q.p.x + R, q.p.y + R));
        if (h && u.pressed) {
            u.pressed = false;
            switch (q.act) {
            case 0: rotateSelected(1); break;
            case 1: duplicateSelected(); break;
            case 2: m_arcOpen = true; m_arcT0 = ImGui::GetTime(); break;
            case 3:   // in hand, at the brick; the finger still down carries it, the release drops it
                if (m_selected >= 0) {
                    float sx = c.x, sy = c.y;
                    m_pressPoint = m_build.worldVoxel(m_bricks[m_selected], m_build.grabOf(m_bricks[m_selected]));
                    view().project(m_pressPoint, sx, sy);
                    beginMove(m_selected, sx, sy);
                    m_dragStarted = true;
                }
                break;
            }
        }
    }
    // « … » small bubble, top right
    const float r2 = 15 * s;
    const ImVec2 pm = clampP(ImVec2(b.x + off * 0.72f, a.y - off * 0.72f));
    u.dl->AddCircleFilled(ImVec2(pm.x, pm.y + 2 * s), r2 + 1 * s, IM_COL32(40, 25, 10, 40), 32);
    u.dl->AddCircleFilled(pm, r2, m_moreOpen ? IM_COL32(0x4a, 0x3a, 0x2c, 230) : K_BUBBLE, 32);
    for (int k = -1; k <= 1; ++k) u.dl->AddCircleFilled(ImVec2(pm.x + k * 5 * s, pm.y), 1.8f * s, IM_COL32(255, 255, 255, 240), 10);
    m_uiRects.push_back(ImVec4(pm.x - r2, pm.y - r2, pm.x + r2, pm.y + r2));
    const bool hitMore = u.pressed && u.hover(ImVec2(pm.x - r2, pm.y - r2), ImVec2(pm.x + r2, pm.y + r2));
    if (hitMore) { m_moreOpen = !m_moreOpen; u.pressed = false; }
    if (!m_moreOpen) return;
    struct Item { const char* icon; const char* label; int act; };
    const Item items[5] = {{IC_TRASH, "Supprimer", 0}, {IC_GROUP, m_multi.empty() ? "Grouper (sélection multiple)" : "Grouper la sélection", 1},
                           {IC_EYE, m_focusDim ? "Ne plus isoler" : "Isoler", 2}, {IC_PIPETTE, "Prendre sa couleur", 3}, {nullptr, "Fiche de la pièce", 4}};
    const float W = 250 * s, rowH = 44 * s, pad = 10 * s;
    const float H = pad * 2 + 5 * rowH;
    ImVec2 qa(pm.x + r2 + 8 * s, pm.y - 20 * s);
    if (qa.x + W > d.x - 10 * s) qa.x = pm.x - r2 - 8 * s - W;
    qa.y = std::clamp(qa.y, 70 * s, d.y - H - 10 * s);
    const ImVec2 qb(qa.x + W, qa.y + H);
    u.dl->AddRectFilled(ImVec2(qa.x, qa.y + 6 * s), ImVec2(qb.x, qb.y + 6 * s), IM_COL32(70, 45, 20, 30), 20 * s);
    u.dl->AddRectFilled(qa, qb, K_PUCK, 20 * s);
    m_uiRects.push_back(ImVec4(qa.x, qa.y, qb.x, qb.y));
    float y = qa.y + pad;
    for (const Item& it : items) {
        const ImVec2 ia(qa.x + 6 * s, y), ib(qb.x - 6 * s, y + rowH);
        const bool h = u.hover(ia, ib);
        if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(ia, ib, C_CARD_HI, 12 * s);
        const ImVec2 ic(ia.x + 22 * s, y + rowH * 0.5f);
        if (it.icon) lucide(u.dl, it.icon, ic, 20 * s, it.act == 0 ? IM_COL32(0xc0, 0x3a, 0x2a, 255) : K_INK, 2.0f);
        else lucide(u.dl, IC_INFO, ic, 20 * s, K_INK, 2.0f);
        if (it.act == 2) u.dl->AddCircle(ic, 3 * s, K_INK, 12, 2 * s);
        u.text(ImVec2(ia.x + 44 * s, y + rowH * 0.5f - 9 * s), 13.5f, it.act == 0 ? IM_COL32(0xc0, 0x3a, 0x2a, 255) : K_INK, it.label);
        if (h && u.pressed) {
            switch (it.act) {
            case 0: deleteSelected(); break;
            case 1: groupSelected(); break;
            case 2: m_focusDim = !m_focusDim; syncObjects(); break;
            case 3: if (m_selected >= 0) { m_color = m_bricks[m_selected].color; toast("Couleur prise : " + m_lib.palette[m_color].name); } break;
            case 4: if (m_selected >= 0) m_infoPart = m_bricks[m_selected].part; break;
            }
            m_moreOpen = false;
        }
        y += rowH;
    }
    if (u.pressed && !hitMore && !u.hover(qa, qb)) m_moreOpen = false;
}

// ------------------------------------------------------------------ the colour arc around the brick

void BrickApp::ipadColorArc() {
    ImVec2 a, b;
    if (!m_arcOpen || m_holding || !selectionRect(a, b)) { m_arcOpen = false; return; }
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // the current colour, then the colours of the scene (most used first), the usual ones to fill: 9 studs and « + »
    std::vector<int> cols;
    const int cur = m_selected >= 0 ? m_bricks[m_selected].color : m_color;
    cols.push_back(cur);
    std::map<int, int> used;
    for (const Brick& br : m_bricks) ++used[br.color];
    std::vector<std::pair<int, int>> byUse(used.begin(), used.end());
    std::sort(byUse.begin(), byUse.end(), [](const std::pair<int, int>& x, const std::pair<int, int>& y) { return x.second > y.second; });
    for (auto& kv : byUse) if ((int)cols.size() < 9 && std::find(cols.begin(), cols.end(), kv.first) == cols.end()) cols.push_back(kv.first);
    static const int kUsual[] = {15, 71, 4, 14, 2, 19, 0, 70, 1, 25};
    for (int id : kUsual) { const int c = colorIndex(id); if ((int)cols.size() < 9 && c >= 0 && std::find(cols.begin(), cols.end(), c) == cols.end()) cols.push_back(c); }
    const float rs = 17 * s;
    const ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    // a compact fan around the colour bubble (east of the brick; west when the screen edge is near), 100 pt of radius
    const float rad = 100 * s, off = 23 * s + 10 * s;
    const bool east = b.x + off + rad + rs + 8 * s < d.x || a.x - off - rad - rs < 8 * s;
    const ImVec2 ac(std::clamp(east ? b.x + off : a.x - off, rs + 8 * s, d.x - rs - 8 * s), std::clamp(c.y, 70 * s + rad, std::max(70 * s + rad, d.y - 100 * s - rad)));
    const double t = ImGui::GetTime() - m_arcT0;
    const int n = (int)cols.size() + 1;
    const float step = 3.4f / (n - 1);   // ~195 degrees
    bool hit = false;
    for (int k = 0; k < n; ++k) {
        const float e = std::clamp((float)(t - k * 0.02) / 0.16f, 0.0f, 1.0f);
        const float ease = 1.0f - (1.0f - e) * (1.0f - e) * (1.0f - e);
        const float ang = east ? -1.7f + k * step : -1.7f - k * step + 3.14159f;   // from the top, around the bubble
        const ImVec2 p(ac.x + std::cos(ang) * rad * (0.6f + 0.4f * ease), ac.y + std::sin(ang) * rad * (0.6f + 0.4f * ease));
        const float r = rs * ease;
        if (r < 1) continue;
        const bool h = u.hover(ImVec2(p.x - rs - 3 * s, p.y - rs - 3 * s), ImVec2(p.x + rs + 3 * s, p.y + rs + 3 * s));
        m_uiRects.push_back(ImVec4(p.x - rs, p.y - rs, p.x + rs, p.y + rs));
        if (k < (int)cols.size()) {
            stud(u.dl, p, k == 0 ? r * 1.15f : r, m_lib.palette[cols[k]], s, k == 0);
            if (h && u.pressed) { applyColor(cols[k]); hit = true; }
        } else {   // « + »: the Nuancier
            puckShadow(u.dl, p, r, s);
            u.dl->AddCircleFilled(p, r, K_PUCK, 36);
            lucide(u.dl, IC_PLUS, p, r * 1.1f, K_INK, 2.4f);
            if (h && u.pressed) { m_paletteOpen = true; hit = true; }
        }
    }
    {   // the name of the colour under the brick
        const Color& col = m_lib.palette[std::clamp(cur, 0, (int)m_lib.palette.size() - 1)];
        char t2[64];
        snprintf(t2, sizeof(t2), "%s · %d", col.name.c_str(), col.id);
        const float tw = u.textW(12, t2, true);
        const ImVec2 pa(c.x - tw * 0.5f - 12 * s, b.y + 14 * s), pb(c.x + tw * 0.5f + 12 * s, b.y + 40 * s);
        u.dl->AddRectFilled(pa, pb, IM_COL32(0x6b, 0x55, 0x40, 200), 13 * s);
        u.text(ImVec2(pa.x + 12 * s, pa.y + 5 * s), 12, IM_COL32(255, 255, 255, 245), t2, true);
    }
    if (hit) { m_arcOpen = false; u.pressed = false; }
    else if (u.pressed && m_arcT0 < ImGui::GetTime() - 0.05) {   // a tap elsewhere closes the arc
        bool inside = false;
        for (const ImVec4& r : m_uiRects) if (u.mouse.x >= r.x && u.mouse.y >= r.y && u.mouse.x < r.z && u.mouse.y < r.w) inside = true;
        if (!inside) m_arcOpen = false;
    }
}

// ------------------------------------------------------------------ the Nuancier (every colour, by family)

void BrickApp::ipadNuancier() {
    if (!m_paletteOpen) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const int fb = focusBrick() >= 0 ? focusBrick() : m_selected;
    const int fpart = fb >= 0 && fb < (int)m_bricks.size() ? m_bricks[fb].part : -1;
    std::set<int> inScene;
    for (const Brick& br : m_bricks) inScene.insert(br.color);
    std::vector<int> fam[9];
    for (int i = 0; i < (int)m_lib.palette.size(); ++i) {
        if (m_paletteFilter == 1 && !inScene.count(i)) continue;
        if (m_paletteFilter == 2 && fpart >= 0 && m_lib.hasColorTable(fpart) && !m_lib.colorExists(fpart, i)) continue;
        fam[colorFamily(m_lib.palette[i])].push_back(i);
    }
    const int cols = 7;
    const float W = 420 * s, pad = 22 * s, sw = 44 * s, sr = 17 * s, labH = 24 * s;
    float H = pad + 40 * s + 12 * s + 40 * s + 10 * s;
    for (auto& f : fam) if (!f.empty()) H += labH + ((f.size() + cols - 1) / cols) * sw;
    H += pad;
    const ImVec2 a(d.x - 18 * s - W, std::max(70 * s, std::min(70 * s, d.y - H - 18 * s))), b(a.x + W, std::min(a.y + H, d.y - 18 * s));
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 8 * s), ImVec2(b.x, b.y + 8 * s), IM_COL32(70, 45, 20, 34), 26 * s);
    u.dl->AddRectFilled(a, b, K_PUCK, 26 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    u.text(ImVec2(a.x + pad, y + 4 * s), 19, K_INK, "Nuancier", true);
    {   // close and the eyedropper (the colour of the selected brick)
        const float r = 19 * s;
        const ImVec2 cx(b.x - pad - r, y + r), cp(cx.x - 2 * r - 10 * s, y + r);
        if (puck(u, cx, r, IC_X)) { m_paletteOpen = false; return; }
        if (puck(u, cp, r, IC_PIPETTE, m_selected >= 0)) m_color = m_bricks[m_selected].color;
    }
    y += 40 * s + 12 * s;
    {   // filters
        const char* labs[3] = {"Toutes", "Dans la scène", "Pour la pièce"};
        const ImVec2 fa(a.x + pad, y), fbb(b.x - pad, y + 40 * s);
        u.dl->AddRectFilled(fa, fbb, IM_COL32(0xef, 0xe5, 0xd4, 255), 20 * s);
        const float w = (fbb.x - fa.x - 8 * s) / 3;
        for (int k = 0; k < 3; ++k) {
            const ImVec2 ka(fa.x + 4 * s + k * w, fa.y + 4 * s), kb(ka.x + w, fbb.y - 4 * s);
            if (m_paletteFilter == k) u.dl->AddRectFilled(ka, kb, IM_COL32(255, 255, 255, 255), 16 * s);
            const float tw = u.textW(12.5f, labs[k], m_paletteFilter == k);
            u.text(ImVec2((ka.x + kb.x - tw) * 0.5f, ka.y + 7 * s), 12.5f, m_paletteFilter == k ? K_INK : K_INK2, labs[k], m_paletteFilter == k);
            if (u.pressed && u.hover(ka, kb)) m_paletteFilter = k;
        }
        y += 40 * s + 10 * s;
    }
    u.dl->PushClipRect(ImVec2(a.x, y), ImVec2(b.x, b.y - 6 * s), true);
    const int cur = m_selected >= 0 ? m_bricks[m_selected].color : m_color;
    for (int f = 0; f < 9; ++f) {
        if (fam[f].empty()) continue;
        u.text(ImVec2(a.x + pad, y + 4 * s), 10.5f, K_INK2, kFamilyFr[f], true);
        y += labH;
        for (size_t k = 0; k < fam[f].size(); ++k) {
            const int i = fam[f][k];
            const ImVec2 p(a.x + pad + sr + 2 * s + (k % cols) * sw, y + sr + 4 * s + (k / cols) * sw);
            stud(u.dl, p, sr, m_lib.palette[i], s, false);
            if (i == cur) u.dl->AddCircle(p, sr + 4 * s, K_INK, 36, 2.5f * s);
            if (u.pressed && u.hover(ImVec2(p.x - sw * 0.5f, p.y - sw * 0.5f), ImVec2(p.x + sw * 0.5f, p.y + sw * 0.5f))) {
                applyColor(i);
                u.pressed = false;
            }
        }
        y += ((fam[f].size() + cols - 1) / cols) * sw;
    }
    u.dl->PopClipRect();
}

// ------------------------------------------------------------------ Maison: files and the four settings

void BrickApp::ipadHomeMenu() {
    if (!m_homeOpen) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    struct Item { const char* icon; const char* label; const char* note; int act; };
    std::vector<Item> items = {{IC_PLUS, "Nouvelle scène", nullptr, 0}, {IC_FOLDER, "Ouvrir", "Fichiers", 1}, {IC_SAVE, "Enregistrer une copie", nullptr, 2},
                               {IC_SHARE, "Exporter en LDraw", ".ldr", 3}, {IC_BOOK, m_reading ? "Fermer la notice" : "Lire une notice", ".ldr .mpd", 4}};
    struct Seg { const char* label; std::vector<const char*> opts; int cur; int id; };
    const int q = m_renderScale > 0.95f ? 2 : m_renderScale > 0.8f ? 1 : 0;
    std::vector<Seg> segs = {{"Qualité", {"Économie", "Normale", "Belle"}, q, 0}, {"Lumière", {"Matin", "Midi", "Soir"}, m_lightPreset, 1},
                             {"Grille", {"Oui", "Non"}, m_build.gridSnap ? 0 : 1, 2}, {"Construire au stylet seulement", {"Oui", "Auto"}, m_pencilOnly ? 0 : 1, 3}};
    const float W = 400 * s, rowH = 46 * s, segH = 46 * s, pad = 12 * s;
    const float H = pad + items.size() * rowH + 14 * s + segs.size() * segH + pad;
    const ImVec2 a(18 * s, 18 * s + 48 * s + 10 * s), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 8 * s), ImVec2(b.x, b.y + 8 * s), IM_COL32(70, 45, 20, 34), 24 * s);
    u.dl->AddRectFilled(a, b, K_PUCK, 24 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    bool acted = false;
    for (const Item& it : items) {
        const ImVec2 ia(a.x + pad, y), ib(b.x - pad, y + rowH - 4 * s);
        const bool h = u.hover(ia, ib);
        if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(ia, ib, IM_COL32(255, 255, 255, 255), 14 * s);
        lucide(u.dl, it.icon, ImVec2(ia.x + 22 * s, (ia.y + ib.y) * 0.5f), 21 * s, K_INK, 2.0f);
        u.text(ImVec2(ia.x + 48 * s, (ia.y + ib.y) * 0.5f - 9.5f * s), 14, K_INK, it.label);
        if (it.note) { const float tw = u.textW(11, it.note); u.text(ImVec2(ib.x - 10 * s - tw, (ia.y + ib.y) * 0.5f - 7 * s), 11, K_INK2, it.note); }
        if (h && u.pressed) {
            acted = true;
            switch (it.act) {
            case 0: m_newSceneOpen = true; break;
            case 1: openDialog(); break;
            case 2: saveDialog(); break;
            case 3: exportLdrDialog(); break;
            case 4: if (m_reading) stopReading(); else if (hasSteps()) startReading(1); else openDialog(); break;
            }
        }
        y += rowH;
    }
    u.dl->AddLine(ImVec2(a.x + pad, y + 6 * s), ImVec2(b.x - pad, y + 6 * s), IM_COL32(0xe0, 0xd2, 0xbc, 255), 1.0f);
    y += 14 * s;
    for (const Seg& sg : segs) {
        u.text(ImVec2(a.x + pad + 8 * s, y + segH * 0.5f - 9 * s), 13, K_INK, sg.label);
        float ow = 0;
        for (const char* o : sg.opts) ow += u.textW(11.5f, o) + 22 * s;
        const ImVec2 sa(b.x - pad - ow - 8 * s, y + 6 * s), sb(b.x - pad, y + segH - 6 * s);
        u.dl->AddRectFilled(sa, sb, IM_COL32(0xef, 0xe5, 0xd4, 255), (sb.y - sa.y) * 0.5f);
        float x = sa.x + 4 * s;
        for (int k = 0; k < (int)sg.opts.size(); ++k) {
            const float w = u.textW(11.5f, sg.opts[k]) + 22 * s;
            const ImVec2 oa(x, sa.y + 3 * s), ob(x + w, sb.y - 3 * s);
            if (sg.cur == k) u.dl->AddRectFilled(oa, ob, IM_COL32(255, 255, 255, 255), (ob.y - oa.y) * 0.5f);
            u.text(ImVec2(oa.x + 11 * s, oa.y + 6 * s), 11.5f, sg.cur == k ? K_INK : K_INK2, sg.opts[k]);
            if (u.pressed && u.hover(oa, ob)) {
                acted = true;
                switch (sg.id) {
                case 0: { static const float kS[3] = {0.67f, 0.85f, 1.0f}; m_renderScale = kS[k]; break; }
                case 1: m_lightPreset = k; applyLook(); break;
                case 2: m_build.gridSnap = k == 0; break;
                case 3: m_pencilOnly = k == 0; break;
                }
                savePrefs();
            }
            x += w;
        }
        y += segH;
    }
    if (acted && (m_newSceneOpen || ImGui::GetIO().WantTextInput)) m_homeOpen = false;
    // a tap outside closes it (the Maison puck toggles it itself)
    if (u.pressed && !u.hover(a, b) && !u.hover(ImVec2(18 * s, 18 * s), ImVec2(66 * s, 66 * s))) m_homeOpen = false;
    (void)d;
}

// ------------------------------------------------------------------ Nouvelle scène

void BrickApp::newScene(int kind) {
    pushUndo();
    m_bricks.clear();
    m_selected = -1;
    m_multi.clear();
    m_scenePath.clear();
    if (kind == 0) {   // a green 32 x 32 baseplate in the middle
        const int p = m_lib.find("3811");
        if (p >= 0 && m_lib.get(m_lib.parts[p].name)) {
            const Part& part = m_lib.parts[p];
            Brick b;
            b.part = p;
            b.color = colorIndex(2) >= 0 ? colorIndex(2) : m_color;
            b.pos = float3(-(part.bmin.x + part.bmax.x) * 0.5f, -part.bmin.y, -(part.bmin.z + part.bmax.z) * 0.5f);
            m_bricks.push_back(b);
        }
    }
    rebuildScene();
    frameAll();
    if (m_bricks.empty()) { m_scene.camera.target = float3(0, 0, 0); m_scene.camera.distance = 60; }
    m_scene.camera.pitch = radians(32.0f);
    touchCamera();
    toast("Nouvelle scène");
}

void BrickApp::ipadNewScene() {
    if (!m_newSceneOpen) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(60, 45, 30, 70));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const float W = std::min(880 * s, d.x - 60 * s), pad = 26 * s;
    const float cardW = (W - 2 * pad - 2 * 16 * s) / 3, cardH = cardW * 0.62f + 70 * s;
    const float H = pad + 44 * s + cardH + 30 * s + 24 * s + 90 * s + pad;
    const ImVec2 a((d.x - W) * 0.5f, (d.y - H) * 0.5f), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 10 * s), ImVec2(b.x, b.y + 10 * s), IM_COL32(60, 40, 20, 40), 28 * s);
    u.dl->AddRectFilled(a, b, K_PUCK, 28 * s);
    u.text(ImVec2(a.x + pad, a.y + pad), 21, K_INK, "Nouvelle scène", true);
    if (puck(u, ImVec2(b.x - pad - 20 * s, a.y + pad + 14 * s), 20 * s, IC_X)) { m_newSceneOpen = false; return; }
    struct Card { const char* title; const char* sub; int act; };
    const Card cards[3] = {{"Plaque verte", "32 × 32 tenons, lumière du matin", 0}, {"Sol libre", "Rien sur la scène", 1}, {"Depuis une notice", "Fichier .ldr / .mpd officiel, étape par étape", 2}};
    float x = a.x + pad;
    const float y = a.y + pad + 44 * s;
    for (const Card& c : cards) {
        const ImVec2 ca(x, y), cb(x + cardW, y + cardH);
        const bool h = u.hover(ca, cb);
        u.dl->AddRectFilled(ImVec2(ca.x, ca.y + 3 * s), ImVec2(cb.x, cb.y + 3 * s), IM_COL32(70, 45, 20, 20), 20 * s);
        u.dl->AddRectFilled(ca, cb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? C_CARD_HI : IM_COL32(255, 255, 255, 255), 20 * s);
        const ImVec2 ia(ca.x + 10 * s, ca.y + 10 * s), ib(cb.x - 10 * s, ca.y + 10 * s + cardW * 0.62f - 10 * s);
        // a drawn picture per card
        if (c.act == 0) {
            u.dl->AddRectFilledMultiColor(ia, ib, IM_COL32(0xf6, 0xe7, 0xd2, 255), IM_COL32(0xf6, 0xe7, 0xd2, 255), IM_COL32(0xe8, 0xd6, 0xbb, 255), IM_COL32(0xe8, 0xd6, 0xbb, 255));
            const ImVec2 m((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f + 8 * s);
            const float k = (ib.x - ia.x) * 0.32f;
            const ImVec2 q[4] = {ImVec2(m.x - k, m.y), ImVec2(m.x, m.y - k * 0.45f), ImVec2(m.x + k, m.y), ImVec2(m.x, m.y + k * 0.45f)};
            u.dl->AddQuadFilled(ImVec2(q[0].x, q[0].y + 5 * s), ImVec2(q[1].x, q[1].y + 5 * s), ImVec2(q[2].x, q[2].y + 5 * s), ImVec2(q[3].x, q[3].y + 5 * s), IM_COL32(0x2f, 0x7a, 0x2f, 255));
            u.dl->AddQuadFilled(q[0], q[1], q[2], q[3], IM_COL32(0x4c, 0xa8, 0x45, 255));
            for (int i = 1; i < 6; ++i)
                for (int j = 1; j < 6; ++j) {
                    const float fu = i / 6.0f, fv = j / 6.0f;
                    const ImVec2 p(q[0].x + (q[1].x - q[0].x) * fu + (q[3].x - q[0].x) * fv, q[0].y + (q[1].y - q[0].y) * fu + (q[3].y - q[0].y) * fv);
                    u.dl->AddCircleFilled(p, 2.4f * s, IM_COL32(0x6c, 0xc4, 0x60, 255), 10);
                }
        } else if (c.act == 1) {
            u.dl->AddRectFilledMultiColor(ia, ib, IM_COL32(0xf3, 0xea, 0xdb, 255), IM_COL32(0xf3, 0xea, 0xdb, 255), IM_COL32(0xe6, 0xd8, 0xc0, 255), IM_COL32(0xe6, 0xd8, 0xc0, 255));
        } else {
            u.dl->AddRectFilled(ia, ib, IM_COL32(0x2a, 0x24, 0x1f, 255), 12 * s);
            lucide(u.dl, IC_BOOK, ImVec2((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f), 54 * s, IM_COL32(0xfb, 0xf6, 0xee, 255), 1.6f);
        }
        u.text(ImVec2(ca.x + 14 * s, ib.y + 12 * s), 15, K_INK, c.title, true);
        u.dl->PushClipRect(ImVec2(ca.x, ib.y), ImVec2(cb.x - 8 * s, cb.y), true);
        u.text(ImVec2(ca.x + 14 * s, ib.y + 36 * s), 11, K_INK2, c.sub);
        u.dl->PopClipRect();
        if (h && u.pressed) {
            m_newSceneOpen = false;
            if (c.act == 2) openDialog();
            else newScene(c.act);
            return;
        }
        x += cardW + 16 * s;
    }
    // resume: the official house (and the autosave of the last session stays the scene at start)
    const float ry = y + cardH + 30 * s;
    u.text(ImVec2(a.x + pad, ry), 11, K_INK2, "REPRENDRE", true);
    const ImVec2 ra(a.x + pad, ry + 24 * s), rb(ra.x + cardW, ry + 24 * s + 76 * s);
    const bool h = u.hover(ra, rb);
    u.dl->AddRectFilled(ra, rb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? C_CARD_HI : IM_COL32(255, 255, 255, 255), 16 * s);
    lucide(u.dl, IC_HOUSE, ImVec2(ra.x + 34 * s, (ra.y + rb.y) * 0.5f), 30 * s, K_INK, 1.8f);
    u.text(ImVec2(ra.x + 62 * s, ra.y + 18 * s), 13.5f, K_INK, "Modern House 31153", true);
    u.text(ImVec2(ra.x + 62 * s, ra.y + 42 * s), 11, K_INK2, "Maison officielle LDraw (OMR)");
    if (h && u.pressed) { m_newSceneOpen = false; openOfficialHouse(); }
}
