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
    if (warm) {   // white paper, dark ink, red and yellow accents (iPad)
        C_BG = IM_COL32(0xf4, 0xf3, 0xf1, 255);
        C_PANEL = IM_COL32(0xff, 0xff, 0xff, 246);
        C_PANEL_SOLID = IM_COL32(0xff, 0xff, 0xff, 255);
        C_CARD = IM_COL32(0xff, 0xff, 0xff, 255);
        C_CARD_HI = IM_COL32(0xf0, 0xef, 0xec, 255);
        C_LINE = IM_COL32(0xe6, 0xe4, 0xe0, 255);
        C_TEXT = IM_COL32(0x2b, 0x28, 0x26, 255);
        C_DIM = IM_COL32(0x7d, 0x79, 0x74, 255);
        C_FAINT = IM_COL32(0xaa, 0xa5, 0x9f, 255);
        C_ACCENT = IM_COL32(0xd9, 0x2b, 0x1f, 255);
        C_INK = IM_COL32(0x2b, 0x28, 0x26, 255);
        C_OVERLAY = IM_COL32(0xff, 0xff, 0xff, 238);
        C_HUD = IM_COL32(0xff, 0xff, 0xff, 214);
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
// Wzx 08/11 09:52: « moins crème, plus blanc, rouge et jaune », the mood of his reference screen (white round buttons)
static const ImU32 K_PUCK = IM_COL32(0xff, 0xff, 0xff, 238);       // white round buttons
static const ImU32 K_SHEET = IM_COL32(0xff, 0xff, 0xff, 255);      // white sheets (menus, Nuancier, cards)
static const ImU32 K_DRAWER = IM_COL32(0xf7, 0xf6, 0xf4, 255);     // the library drawer (white cards on it)
static const ImU32 K_SOFT = IM_COL32(0xf0, 0xef, 0xec, 255);       // pressed rows, chips, segments
static const ImU32 K_LINE = IM_COL32(0xe6, 0xe4, 0xe0, 255);
static const ImU32 K_INK = IM_COL32(0x2b, 0x28, 0x26, 255);        // icons and text
static const ImU32 K_INK2 = IM_COL32(0x8c, 0x88, 0x84, 255);       // secondary text
static const ImU32 K_BUBBLE = IM_COL32(0xff, 0xff, 0xff, 248);     // white bubbles around the brick
static const ImU32 K_RED = IM_COL32(0xd9, 0x2b, 0x1f, 255);
static const ImU32 K_YELLOW = IM_COL32(0xf7, 0xc8, 0x1e, 255);

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
static const char* IC_RECENT = "M3.5 12 a8.5 8.5 0 1 0 17 0 a8.5 8.5 0 1 0 -17 0 M12 7.5V12l3 2";   // recentes.svg
static const char* IC_STAR = "M12 3.5 l2.6 5.4 5.9.8-4.3 4.1 1 5.8L12 16.8l-5.2 2.8 1-5.8-4.3-4.1 5.9-.8z";   // favoris.svg
static const char* IC_GEAR = "M8.8 12 a3.2 3.2 0 1 0 6.4 0 a3.2 3.2 0 1 0 -6.4 0 M12 2.8v3M12 18.2v3M2.8 12h3M18.2 12h3M5.5 5.5l2.1 2.1M16.4 16.4l2.1 2.1M5.5 18.5l2.1-2.1M16.4 7.6l2.1-2.1";   // reglages.svg
static const char* IC_SEARCH = "M4.5 10.5 a6 6 0 1 0 12 0 a6 6 0 1 0 -12 0 M15.5 15.5 20 20";   // chercher.svg

static void puckShadow(ImDrawList* dl, ImVec2 c, float r, float s) {
    dl->AddCircleFilled(ImVec2(c.x, c.y + 3 * s), r + 1.5f * s, IM_COL32(30, 25, 20, 26), 40);
    dl->AddCircleFilled(ImVec2(c.x, c.y + 1.5f * s), r + 0.5f * s, IM_COL32(30, 25, 20, 22), 40);
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
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 3 * s), ImVec2(b.x, b.y + 3 * s), IM_COL32(30, 25, 20, 24), r);
    u.dl->AddRectFilled(a, b, fill, r);
}

// a stud seen from above (the swatches of the colours): the colour, a lighter top, a soft rim
static void stud(ImDrawList* dl, ImVec2 c, float r, const Color& col, float s, bool selected) {
    auto mix = [](float3 a, float k) { return IM_COL32((int)std::min(255.0f, a.x * 255 * k), (int)std::min(255.0f, a.y * 255 * k), (int)std::min(255.0f, a.z * 255 * k), 255); };
    dl->AddCircleFilled(ImVec2(c.x, c.y + 2.5f * s), r + 1 * s, IM_COL32(30, 25, 20, 50), 36);
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
    if (sat < 0.16f || d < 0.06f || mx < 0.24f) return 0;   // whites, greys and black
    float h = 0;
    if (mx == r) h = std::fmod((g - b) / d, 6.0f);
    else if (mx == g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h *= 60;
    if (h < 0) h += 360;
    const bool earth = h >= 10 && h < 62 && ((mx < 0.75f && sat < 0.85f) || sat < 0.55f);
    if (earth) return 6;                        // earths (browns, tans)
    if (h < 15 || h >= 330) return 1;           // reds and pinks
    if (h < 65) return 2;                       // yellows and oranges
    if (h < 170) return 3;                      // greens
    if (h < 255) return 4;                      // blues
    if (h < 330) return 5;                      // violets
    return 0;
}
// the light grey thumbnails multiplied by the active colour (a little lifted so that black stays readable)
static ImU32 partTint(const Color& c) {
    auto ch = [](float v) { return (int)std::clamp(40.0f + 215.0f * v, 0.0f, 255.0f); };
    return IM_COL32(ch(c.srgb.x), ch(c.srgb.y), ch(c.srgb.z), 255);
}
// the usual LDraw colours in French (the others keep their LDraw name)
static const char* colorFr(const std::string& n) {
    static const std::pair<const char*, const char*> t[] = {
        {"Black", "Noir"}, {"White", "Blanc"}, {"Red", "Rouge"}, {"Blue", "Bleu"}, {"Yellow", "Jaune"}, {"Green", "Vert"},
        {"Bright_Green", "Vert vif"}, {"Dark_Green", "Vert foncé"}, {"Tan", "Beige"}, {"Dark_Tan", "Beige foncé"},
        {"Light_Bluish_Grey", "Gris clair"}, {"Dark_Bluish_Grey", "Gris foncé"}, {"Light_Grey", "Gris clair ancien"}, {"Dark_Grey", "Gris foncé ancien"},
        {"Reddish_Brown", "Brun rougeâtre"}, {"Brown", "Brun"}, {"Dark_Brown", "Brun foncé"}, {"Orange", "Orange"}, {"Dark_Orange", "Orange foncé"},
        {"Bright_Light_Orange", "Orange clair"}, {"Bright_Light_Yellow", "Jaune clair"}, {"Lime", "Citron vert"}, {"Olive_Green", "Vert olive"},
        {"Sand_Green", "Vert sable"}, {"Medium_Blue", "Bleu moyen"}, {"Dark_Blue", "Bleu foncé"}, {"Dark_Azure", "Azur foncé"},
        {"Medium_Azure", "Azur moyen"}, {"Bright_Light_Blue", "Bleu clair"}, {"Sand_Blue", "Bleu sable"}, {"Dark_Turquoise", "Turquoise foncé"},
        {"Dark_Red", "Rouge foncé"}, {"Dark_Pink", "Rose foncé"}, {"Bright_Pink", "Rose vif"}, {"Magenta", "Magenta"}, {"Medium_Lavender", "Lavande"},
        {"Dark_Purple", "Violet foncé"}, {"Medium_Nougat", "Nougat moyen"}, {"Nougat", "Nougat"}, {"Light_Nougat", "Nougat clair"},
        {"Pearl_Gold", "Or perle"}, {"Flat_Silver", "Argent mat"}, {"Trans_Clear", "Transparent"}, {"Trans_Black", "Fumé"},
        {"Trans_Red", "Rouge transparent"}, {"Trans_Dark_Blue", "Bleu transparent"}, {"Trans_Light_Blue", "Bleu clair transparent"},
        {"Trans_Green", "Vert transparent"}, {"Trans_Yellow", "Jaune transparent"}, {"Trans_Orange", "Orange transparent"}, {"Chrome_Silver", "Chrome"}};
    std::string k = n;
    for (char& ch : k) if (ch == ' ') ch = '_';
    for (auto& e : t) if (k == e.first) return e.second;
    return n.c_str();
}
// a short French name of a part from its LDraw title: « Brick 4 x 4 x 1 Round with Rounded Top » -> « Brique 4×4×1 ronde »
static std::string partFr(const std::string& label) {
    static const std::pair<const char*, const char*> nouns[] = {
        {"Brick", "Brique"}, {"Plate", "Plaque"}, {"Tile", "Tuile"}, {"Slope", "Pente"}, {"Wedge", "Biseau"}, {"Arch", "Arche"},
        {"Cylinder", "Cylindre"}, {"Cone", "Cône"}, {"Dish", "Parabole"}, {"Baseplate", "Plaque de base"}, {"Bracket", "Équerre"},
        {"Panel", "Panneau"}, {"Door", "Porte"}, {"Window", "Fenêtre"}, {"Glass", "Vitre"}, {"Fence", "Clôture"}, {"Wheel", "Roue"},
        {"Tyre", "Pneu"}, {"Hinge", "Charnière"}, {"Bar", "Barre"}, {"Plant", "Plante"}, {"Ladder", "Échelle"}, {"Support", "Support"},
        {"Turntable", "Plateau tournant"}, {"Antenna", "Antenne"}, {"Flag", "Drapeau"}, {"Rock", "Rocher"}, {"Container", "Conteneur"}};
    static const std::pair<const char*, const char*> adjs[] = {
        {"Round", "ronde"}, {"Curved", "incurvée"}, {"Inverted", "inversée"}, {"Corner", "d'angle"}, {"Modified", "modifiée"},
        {"Double", "double"}, {"Triple", "triple"}};
    std::vector<std::string> w;
    for (size_t i = 0; i < label.size();) {
        size_t j = label.find(' ', i);
        if (j == std::string::npos) j = label.size();
        if (j > i) w.push_back(label.substr(i, j - i));
        i = j + 1;
    }
    if (w.empty()) return label;
    size_t k = 0;
    if (w[0] == "Technic" && w.size() > 1) k = 1;   // « Technic Beam 5 »: the noun is the second word
    std::string out;
    for (auto& n : nouns) if (w[k] == n.first) out = n.second;
    if (out.empty()) return label.size() > 30 ? label.substr(0, 29) + "…" : label;
    if (k == 1) out = "Technic " + out;
    // the dimensions: « 4 x 4 x 1 » -> « 4×4×1 »
    std::string dims;
    size_t i = k + 1;
    while (i < w.size()) {
        const bool num = !w[i].empty() && (std::isdigit((unsigned char)w[i][0]) || w[i][0] == '.');
        if (num) { dims += w[i]; ++i; }
        else if (w[i] == "x" && !dims.empty()) { dims += "×"; ++i; }
        else break;
    }
    if (!dims.empty()) out += " " + dims;
    std::string adj;
    for (size_t j = i; j < w.size() && w[j] != "with"; ++j)
        for (auto& a : adjs) if (w[j] == a.first) { adj += " "; adj += a.second; }
    return out + adj;
}
static const char* kFamilyFr[9] = {"Blancs et gris", "Rouges et roses", "Jaunes et oranges", "Verts", "Bleus", "Violets", "Terres", "Transparentes", "Métal"};

// ------------------------------------------------------------------ the whole interface of the iPad

void BrickApp::drawUiIpad() {
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // the stage: the whole screen (index 0 / 1 of m_uiRects as the PC: the top bar and the tray), the drawer excepted
    m_uiRects.push_back(ImVec4(0, 0, d.x, 0));
    if (m_libOpen) ipadDrawer();           // m_uiRects[1]
    else m_uiRects.push_back(ImVec4(0, d.y, d.x, d.y));
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

// ------------------------------------------------------------------ the library drawer (maquette: 4_bibliotheque_stylet)
// A rounded cream sheet from the bottom (40 % of the screen): illustrated tabs (Récentes, Favoris, the families), the
// search in a pill on the right, the categories in chips, then white cards tinted with the active colour. A tap puts the
// part in hand in the middle of the view, a drag up out of the drawer puts it under the finger, a long press opens its sheet.

void BrickApp::ipadDrawer() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float H = std::round(std::max(330 * s, d.y * 0.40f));
    const ImVec2 a(8 * s, d.y - H), b(d.x - 8 * s, d.y + 40 * s);
    u.dl->AddRectFilled(ImVec2(a.x, a.y - 6 * s), ImVec2(b.x, b.y), IM_COL32(30, 25, 20, 26), 30 * s, ImDrawFlags_RoundCornersTop);
    u.dl->AddRectFilled(a, b, K_DRAWER, 30 * s, ImDrawFlags_RoundCornersTop);
    m_uiRects.push_back(ImVec4(0, a.y, d.x, d.y));
    // the handle: a tap (or a drag down) closes the drawer
    u.dl->AddLine(ImVec2(d.x * 0.5f - 22 * s, a.y + 11 * s), ImVec2(d.x * 0.5f + 22 * s, a.y + 11 * s), IM_COL32(0xc9, 0xb8, 0xa2, 255), 5 * s);
    {
        const ImVec2 ga(d.x * 0.5f - 90 * s, a.y), gb(d.x * 0.5f + 90 * s, a.y + 24 * s);
        if (u.pressed && u.hover(ga, gb)) { m_libOpen = false; m_libPress = -1; return; }
    }
    const float pad = 22 * s;
    float y = a.y + 26 * s;
    // ---- tabs: an icon or a thumbnail of the family, the name under it
    struct Tab { const char* label; std::string key; const char* icon; std::vector<const char*> parts; };
    static const std::vector<Tab> tabs = {
        {"Récentes", "recent", IC_RECENT, {}}, {"Favoris", "fav", IC_STAR, {}},
        {"Basique", "Basique", nullptr, {"3001"}}, {"Formes", "Formes", nullptr, {"3040b", "3040", "3039"}},
        {"Technic", "Technic", nullptr, {"3648b", "3648a", "32524", "3700"}}, {"Mécanique", "Mécanique", nullptr, {"3937", "2429", "3680"}},
        {"Véhicules", "Véhicules", nullptr, {"4624", "30027b", "6014"}}, {"Bâtiment", "Bâtiment", nullptr, {"60594", "60592", "3853", "2377"}},
        {"Personnages", "Personnages", nullptr, {"3626b", "3626c", "973"}}, {"Nature", "Nature", nullptr, {"6064", "2423", "3741"}},
        {"Autres", "Autres gammes", nullptr, {"3437", "6474"}}};
    const float tabW = 84 * s, tabH = 66 * s;
    const float searchW = std::min(300 * s, d.x * 0.24f);
    const ImVec2 ta(a.x + pad - 8 * s, y), tb(b.x - pad - searchW - 12 * s, y + tabH);
    const float tabStart = ta.x - m_famScroll;
    float x = tabStart;
    u.dl->PushClipRect(ta, tb, true);
    for (const Tab& t : tabs) {
        const ImVec2 ca(x, y), cb(x + tabW, y + tabH);
        const bool sel = m_fam == t.key && !m_searchOpen;
        if (sel) {
            u.dl->AddRectFilled(ImVec2(ca.x, ca.y + 3 * s), ImVec2(cb.x, cb.y + 3 * s), IM_COL32(30, 25, 20, 22), 16 * s);
            u.dl->AddRectFilled(ca, cb, IM_COL32(255, 255, 255, 255), 16 * s);
        }
        const ImVec2 ic((ca.x + cb.x) * 0.5f, ca.y + 24 * s);
        if (t.icon) lucide(u.dl, t.icon, ic, 24 * s, sel ? K_INK : K_INK2, 2.0f);
        else {
            int part = -1;
            for (const char* nm : t.parts) if (part < 0) part = m_lib.find(nm);
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (part >= 0 && thumbUv(part, tex, uv0, uv1)) {
                const ImVec2 du((uv1.x - uv0.x) * 0.16f, (uv1.y - uv0.y) * 0.16f);
                const float r = 20 * s;
                u.dl->AddImage(tex, ImVec2(ic.x - r, ic.y - r), ImVec2(ic.x + r, ic.y + r), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y),
                               sel ? partTint(m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)]) : IM_COL32(0xb4, 0xb0, 0xab, 255));
            }
        }
        const float lw = u.textW(12, t.label);
        u.text(ImVec2((ca.x + cb.x) * 0.5f - lw * 0.5f, cb.y - 22 * s), 12, sel ? K_INK : K_INK2, t.label);
        if (u.released && u.hover(ca, cb) && u.hover(ta, tb) && !swipeTaken(1) && m_swipeDragged != 1) {
            m_fam = t.key; m_cat.clear(); m_sub.clear(); m_cardsScroll = 0; m_search.clear(); m_searchOpen = false;
        }
        x += tabW + 4 * s;
    }
    u.dl->PopClipRect();
    swipeRow(1, ta, tb, m_famScroll, m_famVel, std::max(0.0f, x - tabStart - (tb.x - ta.x)), true);
    // ---- search in a pill (right)
    {
        const ImVec2 sa(b.x - pad - searchW, y + 10 * s), sb(b.x - pad, y + 10 * s + 46 * s);
        u.dl->AddRectFilled(ImVec2(sa.x, sa.y + 2 * s), ImVec2(sb.x, sb.y + 2 * s), IM_COL32(30, 25, 20, 18), 23 * s);
        u.dl->AddRectFilled(sa, sb, IM_COL32(255, 255, 255, 255), 23 * s);
        lucide(u.dl, IC_SEARCH, ImVec2(sa.x + 24 * s, (sa.y + sb.y) * 0.5f), 20 * s, K_INK2, 2.2f);
        ImGui::SetNextWindowPos(ImVec2(sa.x + 42 * s, sa.y + 6 * s));
        ImGui::SetNextWindowSize(ImVec2(sb.x - sa.x - 54 * s, sb.y - sa.y - 12 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4 * s, 8 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(K_INK));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(K_INK2));
        ImGui::Begin("##ipadsearch", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s", m_search.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##q", "3001, brique 2x4…", buf, sizeof(buf))) { m_search = buf; m_searchOpen = !m_search.empty(); m_cardsScroll = 0; }
        ImGui::End();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);
    }
    y += tabH + 12 * s;
    // ---- chips: Tout + the categories of the family (the sub-families for Technic)
    const float chipH = 36 * s;
    const ImVec2 qa(a.x + pad, y), qb(b.x - pad, y + chipH);
    const float chipStart = qa.x - m_catScroll;
    x = chipStart;
    u.dl->PushClipRect(qa, qb, true);
    auto chip = [&](const char* label, bool sel) {
        const float w = u.textW(13, label) + 32 * s;
        const ImVec2 ca(x, y), cb(x + w, y + chipH);
        u.dl->AddRectFilled(ca, cb, sel ? K_RED : IM_COL32(255, 255, 255, 255), chipH * 0.5f);
        u.text(ImVec2(ca.x + 16 * s, ca.y + 9.5f * s), 13, sel ? IM_COL32(255, 255, 255, 255) : K_INK, label);
        x += w + 8 * s;
        return u.released && u.hover(ca, cb) && u.hover(qa, qb) && !swipeTaken(2) && m_swipeDragged != 2;
    };
    const Fam* fam = nullptr;
    for (const Fam& f : families()) if (m_fam == f.name) fam = &f;
    if (m_searchOpen) { std::string t = "Recherche « " + m_search + " »"; u.text(ImVec2(x + 4 * s, y + 10 * s), 13, K_INK2, t.c_str()); }
    else if (!fam) u.text(ImVec2(x + 4 * s, y + 10 * s), 13, K_INK2, m_fam == "recent" ? "Les dernières pièces posées" : m_fam == "fav" ? "Appui long sur une pièce pour l'ajouter aux favoris" : "Toutes les pièces");
    else {
        if (chip("Tout", m_cat.empty())) { m_cat.clear(); m_sub.clear(); m_cardsScroll = 0; }
        for (const char* c : fam->cats) {
            auto it = m_catCount.find(c);
            if (it == m_catCount.end() || !it->second) continue;
            if (std::string(c) == "Technic") {
                for (const char* sn : kTechSubs) {
                    auto jt = m_techSubCount.find(sn);
                    if (jt == m_techSubCount.end() || !jt->second) continue;
                    if (chip(sn, m_cat == "Technic" && m_sub == sn)) { m_cat = "Technic"; m_sub = sn; m_cardsScroll = 0; }
                }
                continue;
            }
            if (chip(catFr(c), m_cat == c)) { m_cat = c; m_sub.clear(); m_cardsScroll = 0; }
        }
    }
    u.dl->PopClipRect();
    swipeRow(2, qa, qb, m_catScroll, m_catVel, std::max(0.0f, x - chipStart - (qb.x - qa.x)), true);
    y += chipH + 14 * s;
    // ---- cards: as many rows as fit, column by column, sliding sideways
    if (m_countsFor != m_lib.parts.size()) {   // (counted by the PC tray otherwise)
        m_countsFor = m_lib.parts.size();
        m_catCount.clear();
        m_techSubCount.clear();
        m_availCount = 0;
        for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
            const Part& p = m_lib.parts[i];
            if (!p.available()) continue;
            ++m_availCount;
            ++m_catCount[partCat(i)];
            if (p.category == "Technic") ++m_techSubCount[p.sub];
        }
    }
    const std::vector<int> parts = visibleParts();
    const float cell = 104 * s, cgap = 10 * s;
    const int rows = std::max(1, (int)((d.y - y - 8 * s + cgap) / (cell + cgap)));
    const int cols = ((int)parts.size() + rows - 1) / rows;
    const ImVec2 ca(a.x + pad, y), cb(b.x - pad, d.y);
    const float contentW = cols * (cell + cgap), viewW = cb.x - ca.x;
    swipeRow(0, ca, cb, m_cardsScroll, m_cardsVel, std::max(0.0f, contentW - viewW), true);
    const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
    const ImU32 tint = partTint(cur);
    const double now = ImGui::GetTime();
    u.dl->PushClipRect(ca, cb, true);
    if (parts.empty())
        u.text(ImVec2(ca.x + 8 * s, ca.y + 30 * s), 13, K_INK2, m_searchOpen ? "Aucune pièce pour cette recherche" : m_fam == "recent" ? "Les pièces que tu poses apparaîtront ici" : m_fam == "fav" ? "Aucun favori pour l'instant" : "Aucune pièce ici");
    for (int k = 0; k < (int)parts.size(); ++k) {
        const int c = k / rows, r = k % rows;
        const ImVec2 pa(ca.x + c * (cell + cgap) - m_cardsScroll, y + r * (cell + cgap)), pb(pa.x + cell, pa.y + cell);
        if (pb.x < ca.x || pa.x > cb.x) continue;
        const int part = parts[k];
        const bool h = u.hover(pa, pb) && u.hover(ca, cb);
        const bool down = m_libPress == part;
        u.dl->AddRectFilled(ImVec2(pa.x, pa.y + 2 * s), ImVec2(pb.x, pb.y + 2 * s), IM_COL32(30, 25, 20, 16), 18 * s);
        u.dl->AddRectFilled(pa, pb, down ? K_SOFT : IM_COL32(255, 255, 255, 255), 18 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(part, tex, uv0, uv1)) {
            const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
            u.dl->AddImage(tex, ImVec2(pa.x + 6 * s, pa.y + 6 * s), ImVec2(pb.x - 6 * s, pb.y - 6 * s), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), tint);
        }
        if (std::find(m_fav.begin(), m_fav.end(), part) != m_fav.end()) lucide(u.dl, IC_STAR, ImVec2(pb.x - 14 * s, pa.y + 14 * s), 12 * s, K_YELLOW, 2.4f);
        if (k == 3) m_mtCard = ImVec2((pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f);
        if (h && u.pressed) { m_libPress = part; m_libPressPos = u.mouse; m_libPressTime = now; m_libLong = false; }
    }
    u.dl->PopClipRect();
    // press on a card: long press = sheet, drag up out of the drawer = in hand under the finger, tap = in hand in the middle
    if (m_libPress >= 0 && (swipeTaken(0) || m_swipeDragged == 0)) m_libPress = -1;
    if (m_libPress >= 0) {
        const ImVec2 dm(u.mouse.x - m_libPressPos.x, u.mouse.y - m_libPressPos.y);
        const bool moved = dm.x * dm.x + dm.y * dm.y > 49 * s * s;
        if (!m_libLong && !moved && now - m_libPressTime > 0.48) { m_libLong = true; m_infoPart = m_libPress; }
        if (moved && ImGui::IsMouseDown(ImGuiMouseButton_Left) && u.mouse.y >= a.y) {   // its thumbnail follows the finger
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (thumbUv(m_libPress, tex, uv0, uv1)) {
                ImDrawList* fg = ImGui::GetForegroundDrawList();
                const float g = 96 * s;
                const ImVec2 c(u.mouse.x, u.mouse.y - g * 0.6f);
                const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
                fg->AddImage(tex, ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), tint);
            }
        }
        if (moved && u.mouse.y < a.y) {
            const int part = m_libPress;
            m_libPress = -1;
            spawn(part, u.mouse.x, u.mouse.y);
            m_dragStarted = true;
            m_pressed = false;
        } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!m_libLong && !moved) spawn(m_libPress, d.x * 0.5f, a.y * 0.55f);
            m_libPress = -1;
        }
    }
}

// ------------------------------------------------------------------ pucks on the edges

// the column of three round buttons on the left (undo, library, settings), centred on the view above the drawer
static void ipadColumn(float s, float bottom, ImVec2 c[3], float& R) {
    R = 27 * s;
    const float M = 20 * s, gap = 16 * s, colH = 6 * R + 2 * gap;
    const float y0 = std::max(M + R, bottom * 0.5f - colH * 0.5f + R);
    for (int k = 0; k < 3; ++k) c[k] = ImVec2(M + R, y0 + k * (2 * R + gap));
}

void BrickApp::ipadPucks() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float bottom = m_libOpen ? m_uiRects[1].y : d.y;
    ImVec2 c[3];
    float R;
    ipadColumn(s, bottom, c, R);
    if (puck(u, c[0], R, IC_UNDO, !m_undo.empty() || m_holding)) undo();
    if (puck(u, c[1], R, IC_BRICK, true, m_libOpen ? K_YELLOW : K_PUCK)) { m_libOpen = !m_libOpen; m_homeOpen = false; m_libPress = -1; }
    if (puck(u, c[2], R, IC_GEAR, true, m_homeOpen ? K_YELLOW : K_PUCK)) { m_homeOpen = !m_homeOpen; m_newSceneOpen = false; }
    for (const ImVec2& p : c) m_uiRects.push_back(ImVec4(p.x - R, p.y - R, p.x + R, p.y + R));
    if (m_reading || m_libOpen) return;   // (the reader and the drawer have the bottom of the screen)
    // the bar of the last parts, each in the colour it was last laid in; a tap = in hand in the middle (and that colour),
    // a drag up = in hand under the finger
    std::vector<std::pair<int, int>> parts;   // part, colour
    auto colourOf = [&](int part) {
        for (int i = (int)m_bricks.size() - 1; i >= 0; --i) if (m_bricks[i].part == part) return m_bricks[i].color;
        return m_color;
    };
    for (int p : m_recent) if (p >= 0 && p < (int)m_lib.parts.size() && (int)parts.size() < 7) parts.push_back({p, colourOf(p)});
    if (parts.empty()) {
        static const std::pair<const char*, int> kStart[] = {{"3039", 4}, {"3010", 14}, {"60592", 1}, {"3001", 2}, {"60596", 70}, {"3185", 15}, {"3003", 4}};
        for (auto& e : kStart) { const int p = m_lib.find(e.first), c = colorIndex(e.second); if (p >= 0) parts.push_back({p, c >= 0 ? c : m_color}); }
    }
    if (parts.empty()) return;
    const int curPart = m_selected >= 0 && m_selected < (int)m_bricks.size() ? m_bricks[m_selected].part : parts[0].first;
    const float cellW = 84 * s, cellH = 70 * s, gap = 6 * s, pad = 10 * s;
    const float W = pad * 2 + parts.size() * cellW + (parts.size() - 1) * gap, H = cellH + pad * 2;
    const ImVec2 a((d.x - W) * 0.5f, d.y - 22 * s - H), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 6 * s), ImVec2(b.x, b.y + 6 * s), IM_COL32(30, 25, 20, 26), 30 * s);
    u.dl->AddRectFilled(a, b, K_PUCK, 30 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float x = a.x + pad;
    for (auto& pc : parts) {
        const ImVec2 ca(x, a.y + pad), cb(x + cellW, a.y + pad + cellH);
        const bool h = u.hover(ca, cb);
        if (pc.first == curPart || (h && ImGui::IsMouseDown(ImGuiMouseButton_Left))) u.dl->AddRectFilled(ca, cb, IM_COL32(0xec, 0xea, 0xe6, 255), 20 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(pc.first, tex, uv0, uv1)) {
            const ImVec2 du((uv1.x - uv0.x) * 0.14f, (uv1.y - uv0.y) * 0.14f);
            const float side = cellH - 8 * s;
            const ImVec2 m((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f);
            u.dl->AddImage(tex, ImVec2(m.x - side * 0.5f, m.y - side * 0.5f), ImVec2(m.x + side * 0.5f, m.y + side * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y),
                           partTint(m_lib.palette[std::clamp(pc.second, 0, (int)m_lib.palette.size() - 1)]));
        }
        if (h && u.pressed) { m_dockPress = pc.first; m_dockPressPos = u.mouse; m_color = pc.second; }
        x += cellW + gap;
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

// a flower of five petals (the colour swatches around the selection)
static void flower(ImDrawList* dl, ImVec2 c, float r, ImU32 col, float s, bool selected) {
    for (int k = 0; k < 5; ++k) {
        const float an = -1.5708f + k * 1.2566f;
        dl->AddCircleFilled(ImVec2(c.x + std::cos(an) * r * 0.48f, c.y + std::sin(an) * r * 0.48f + 2 * s), r * 0.56f, IM_COL32(30, 25, 20, 22), 24);
    }
    for (int k = 0; k < 5; ++k) {
        const float an = -1.5708f + k * 1.2566f;
        dl->AddCircleFilled(ImVec2(c.x + std::cos(an) * r * 0.48f, c.y + std::sin(an) * r * 0.48f), r * 0.54f, col, 24);
    }
    dl->AddCircleFilled(c, r * 0.5f, col, 24);
    dl->AddCircleFilled(ImVec2(c.x - r * 0.12f, c.y - r * 0.14f), r * 0.2f, IM_COL32(255, 255, 255, 70), 16);
    if (selected) dl->AddCircle(c, r * 1.12f, IM_COL32(255, 255, 255, 255), 40, 3 * s);
}

void BrickApp::ipadBubbles() {
    ImVec2 a, b;
    const bool show = !m_holding && !m_mechOn && !m_reading && !m_paletteOpen && !m_homeOpen && !m_newSceneOpen && selectionRect(a, b);
    if (!show) { m_moreOpen = false; return; }
    Ui u = ui(m_ui);
    if (m_gui->fontBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float R = 24 * s;   // white bubbles of 48 pt
    const ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    const float ring = std::clamp(std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) * 0.5f + 8 * s, 46 * s, 150 * s);
    const float off = ring + R + 6 * s;
    auto at = [&](float rad, float ang) { return ImVec2(c.x + std::cos(ang) * rad, c.y + std::sin(ang) * rad); };
    auto clampP = [&](ImVec2 p) { return ImVec2(std::clamp(p.x, R + 90 * s, d.x - R - 8 * s), std::clamp(p.y, R + 12 * s, d.y - R - 120 * s)); };
    // the colours: the current one, those of the scene, the usual ones, and « + » (the Nuancier)
    std::vector<int> cols;
    const int cur = m_selected >= 0 ? m_bricks[m_selected].color : m_color;
    cols.push_back(cur);
    {
        std::map<int, int> used;
        for (const Brick& br : m_bricks) ++used[br.color];
        std::vector<std::pair<int, int>> byUse(used.begin(), used.end());
        std::sort(byUse.begin(), byUse.end(), [](const std::pair<int, int>& x, const std::pair<int, int>& y) { return x.second > y.second; });
        static const int kUsual[] = {4, 14, 15, 1, 2, 0, 71, 70};
        for (int id : kUsual) { const int k = colorIndex(id); if ((int)cols.size() < 4 && k >= 0 && std::find(cols.begin(), cols.end(), k) == cols.end()) cols.push_back(k); }
        for (auto& kv : byUse) if ((int)cols.size() < 6 && std::find(cols.begin(), cols.end(), kv.first) == cols.end()) cols.push_back(kv.first);
    }
    // the thin ring around the brick: white on the left (the actions), the colours on the right
    const int n = (int)cols.size() + 1;
    const float fr = 19 * s, a0 = -1.75f, span = 3.0f;
    const float frad = std::max(ring + 38 * s, 2.25f * fr * (n - 1) / span);   // (the flowers never touch)
    const float step = std::min(0.5f, span / (n - 1));
    u.dl->PathArcTo(c, ring, 1.75f, 4.55f, 40);
    u.dl->PathStroke(IM_COL32(255, 255, 255, 210), 0, 2.2f * s);
    for (int k = 0; k + 1 < n; ++k) {
        const Color& col = m_lib.palette[std::clamp(cols[k], 0, (int)m_lib.palette.size() - 1)];
        u.dl->PathArcTo(c, ring, a0 + (k - 0.5f) * step, a0 + (k + 0.5f) * step, 10);
        u.dl->PathStroke(IM_COL32((int)(col.srgb.x * 255), (int)(col.srgb.y * 255), (int)(col.srgb.z * 255), 220), 0, 4 * s);
    }
    bool hit = false;
    for (int k = 0; k < n; ++k) {
        const ImVec2 p = clampP(at(frad, a0 + k * step));
        const bool h = u.hover(ImVec2(p.x - fr, p.y - fr), ImVec2(p.x + fr, p.y + fr));
        m_uiRects.push_back(ImVec4(p.x - fr, p.y - fr, p.x + fr, p.y + fr));
        if (k + 1 < n) {
            const Color& col = m_lib.palette[std::clamp(cols[k], 0, (int)m_lib.palette.size() - 1)];
            flower(u.dl, p, k == 0 ? fr * 1.1f : fr, IM_COL32((int)(col.srgb.x * 255), (int)(col.srgb.y * 255), (int)(col.srgb.z * 255), 255), s, k == 0);
            if (h && u.pressed) { applyColor(cols[k]); hit = true; }
        } else {
            flower(u.dl, p, fr, IM_COL32(255, 255, 255, 255), s, false);
            lucide(u.dl, IC_PLUS, p, fr * 1.0f, K_INK, 2.4f);
            if (h && u.pressed) { m_paletteOpen = true; hit = true; }
        }
    }
    if (hit) u.pressed = false;
    // the actions on the left: turn, duplicate, delete (white bubbles, dark icons)
    struct Bub { ImVec2 p; const char* icon; int act; };
    const Bub bubs[3] = {{clampP(at(off, -2.45f)), IC_ROTATE, 0}, {clampP(at(off, 3.1f)), IC_COPY, 1}, {clampP(at(off, 2.3f)), IC_TRASH, 2}};
    for (const Bub& q : bubs) {
        const bool h = u.hover(ImVec2(q.p.x - R, q.p.y - R), ImVec2(q.p.x + R, q.p.y + R));
        const bool down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const float r = R * (down ? 0.92f : 1.0f);
        puckShadow(u.dl, q.p, r, s);
        u.dl->AddCircleFilled(q.p, r, K_BUBBLE, 40);
        lucide(u.dl, q.icon, q.p, R * 0.92f, q.act == 2 ? K_RED : K_INK, 2.1f);
        m_uiRects.push_back(ImVec4(q.p.x - R, q.p.y - R, q.p.x + R, q.p.y + R));
        if (h && u.pressed) {
            u.pressed = false;
            switch (q.act) {
            case 0: rotateSelected(1); break;
            case 1: duplicateSelected(); break;
            case 2: deleteSelected(); return;
            }
        }
    }
    // « … » small bubble, top right
    const float r2 = 15 * s;
    const ImVec2 pm = clampP(at(off + 2 * s, 1.72f));
    puckShadow(u.dl, pm, r2, s);
    u.dl->AddCircleFilled(pm, r2, m_moreOpen ? K_YELLOW : K_BUBBLE, 32);
    for (int k = -1; k <= 1; ++k) u.dl->AddCircleFilled(ImVec2(pm.x + k * 5 * s, pm.y), 1.8f * s, K_INK, 10);
    m_uiRects.push_back(ImVec4(pm.x - r2, pm.y - r2, pm.x + r2, pm.y + r2));
    const bool hitMore = u.pressed && u.hover(ImVec2(pm.x - r2, pm.y - r2), ImVec2(pm.x + r2, pm.y + r2));
    if (hitMore) { m_moreOpen = !m_moreOpen; u.pressed = false; }
    if (!m_moreOpen) return;
    struct Item { const char* icon; const char* label; int act; };
    const Item items[5] = {{IC_MOVE, "Déplacer", 0}, {IC_GROUP, m_multi.empty() ? "Grouper (sélection multiple)" : "Grouper la sélection", 1},
                           {IC_EYE, m_focusDim ? "Ne plus isoler" : "Isoler", 2}, {IC_PIPETTE, "Prendre sa couleur", 3}, {nullptr, "Fiche de la pièce", 4}};
    const float W = 250 * s, rowH = 44 * s, pad = 10 * s;
    const float H = pad * 2 + 5 * rowH;
    ImVec2 qa(pm.x + r2 + 8 * s, pm.y - 20 * s);
    if (qa.x + W > d.x - 10 * s) qa.x = pm.x - r2 - 8 * s - W;
    qa.y = std::clamp(qa.y, 70 * s, d.y - H - 10 * s);
    const ImVec2 qb(qa.x + W, qa.y + H);
    u.dl->AddRectFilled(ImVec2(qa.x, qa.y + 6 * s), ImVec2(qb.x, qb.y + 6 * s), IM_COL32(30, 25, 20, 30), 20 * s);
    u.dl->AddRectFilled(qa, qb, K_SHEET, 20 * s);
    m_uiRects.push_back(ImVec4(qa.x, qa.y, qb.x, qb.y));
    float y = qa.y + pad;
    for (const Item& it : items) {
        const ImVec2 ia(qa.x + 6 * s, y), ib(qb.x - 6 * s, y + rowH);
        const bool h = u.hover(ia, ib);
        if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(ia, ib, C_CARD_HI, 12 * s);
        const ImVec2 ic(ia.x + 22 * s, y + rowH * 0.5f);
        if (it.icon) lucide(u.dl, it.icon, ic, 20 * s, K_INK, 2.0f);
        else lucide(u.dl, IC_INFO, ic, 20 * s, K_INK, 2.0f);
        if (it.act == 2) u.dl->AddCircle(ic, 3 * s, K_INK, 12, 2 * s);
        u.text(ImVec2(ia.x + 44 * s, y + rowH * 0.5f - 9 * s), 13.5f, K_INK, it.label);
        if (h && u.pressed) {
            switch (it.act) {
            case 0:   // in hand, at the brick; the next touch carries it, the release drops it
                if (m_selected >= 0) {
                    float sx = c.x, sy = c.y;
                    m_pressPoint = m_build.worldVoxel(m_bricks[m_selected], m_build.grabOf(m_bricks[m_selected]));
                    view().project(m_pressPoint, sx, sy);
                    beginMove(m_selected, sx, sy);
                }
                break;
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
    m_arcOpen = false;   // (the colours are flowers around the selection now: ipadBubbles)
    if (true) return;
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
    {   // the name of the colour, in a cream pill in the hollow of the fan (readable on any brick)
        const Color& col = m_lib.palette[std::clamp(cur, 0, (int)m_lib.palette.size() - 1)];
        const char* t2 = colorFr(col.name);
        const float tw = u.textW(12.5f, t2, true);
        const float px = east ? ac.x - 4 * s : ac.x + 4 * s - tw - 24 * s;
        const ImVec2 pa(px, ac.y - 14 * s), pb(px + tw + 24 * s, ac.y + 14 * s);
        u.dl->AddRectFilled(ImVec2(pa.x, pa.y + 3 * s), ImVec2(pb.x, pb.y + 3 * s), IM_COL32(30, 25, 20, 40), 14 * s);
        u.dl->AddRectFilled(pa, pb, K_SHEET, 14 * s);
        u.text(ImVec2(pa.x + 12 * s, pa.y + 6 * s), 12.5f, K_INK, t2, true);
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
    const ImVec2 a(d.x - 18 * s - W, 70 * s), b(a.x + W, std::min(a.y + H, d.y - 150 * s));   // (above the colour stud and Recadrer)
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 8 * s), ImVec2(b.x, b.y + 8 * s), IM_COL32(30, 25, 20, 34), 26 * s);
    u.dl->AddRectFilled(a, b, K_SHEET, 26 * s);
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
        u.dl->AddRectFilled(fa, fbb, K_SOFT, 20 * s);
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
    std::vector<Item> items = {{IC_REDO, "Rétablir", "3 doigts", 6}, {IC_SCAN, "Recadrer la vue", nullptr, 5}, {IC_PLUS, "Nouvelle scène", nullptr, 0}, {IC_FOLDER, "Ouvrir", "Fichiers", 1}, {IC_SAVE, "Enregistrer une copie", nullptr, 2},
                               {IC_SHARE, "Exporter en LDraw", ".ldr", 3}, {IC_BOOK, m_reading ? "Fermer la notice" : "Lire une notice", ".ldr .mpd", 4}};
    struct Seg { const char* label; std::vector<const char*> opts; int cur; int id; };
    const int q = m_renderScale > 0.95f ? 2 : m_renderScale > 0.8f ? 1 : 0;
    std::vector<Seg> segs = {{"Qualité", {"Économie", "Normale", "Belle"}, q, 0}, {"Lumière", {"Matin", "Midi", "Soir"}, m_lightPreset, 1},
                             {"Grille", {"Oui", "Non"}, m_build.gridSnap ? 0 : 1, 2}, {"Construire au stylet seulement", {"Oui", "Auto"}, m_pencilOnly ? 0 : 1, 3}};
    const float W = 400 * s, rowH = 46 * s, segH = 46 * s, pad = 12 * s;
    const float H = pad + items.size() * rowH + 14 * s + segs.size() * segH + pad;
    ImVec2 col[3];
    float CR;
    ipadColumn(s, m_libOpen ? m_uiRects[1].y : d.y, col, CR);
    const ImVec2 a(col[0].x + CR + 16 * s, std::clamp(col[2].y - H * 0.5f, 16 * s, std::max(16 * s, d.y - H - 16 * s))), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 8 * s), ImVec2(b.x, b.y + 8 * s), IM_COL32(30, 25, 20, 34), 24 * s);
    u.dl->AddRectFilled(a, b, K_SHEET, 24 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    bool acted = false;
    for (const Item& it : items) {
        const ImVec2 ia(a.x + pad, y), ib(b.x - pad, y + rowH - 4 * s);
        const bool h = u.hover(ia, ib);
        if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(ia, ib, K_SOFT, 14 * s);
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
            case 5: {
                const std::vector<int> sel = selection();
                if (sel.empty()) frameAll();
                else {   // the selection in the middle, closer
                    float3 mn(1e30f), mx(-1e30f);
                    for (int i : sel) { Box bx = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos); mn = minv(mn, bx.mn); mx = maxv(mx, bx.mx); }
                    m_scene.camera.target = (mn + mx) * 0.5f;
                    m_scene.camera.distance = std::max(6.0f, length(mx - mn) * 2.2f);
                }
                touchCamera();
                break;
            }
            case 6: redo(); break;
            }
        }
        y += rowH;
    }
    u.dl->AddLine(ImVec2(a.x + pad, y + 6 * s), ImVec2(b.x - pad, y + 6 * s), K_LINE, 1.0f);
    y += 14 * s;
    for (const Seg& sg : segs) {
        u.text(ImVec2(a.x + pad + 8 * s, y + segH * 0.5f - 9 * s), 13, K_INK, sg.label);
        float ow = 0;
        for (const char* o : sg.opts) ow += u.textW(11.5f, o) + 22 * s;
        const ImVec2 sa(b.x - pad - ow - 8 * s, y + 6 * s), sb(b.x - pad, y + segH - 6 * s);
        u.dl->AddRectFilled(sa, sb, K_SOFT, (sb.y - sa.y) * 0.5f);
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
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 10 * s), ImVec2(b.x, b.y + 10 * s), IM_COL32(30, 25, 20, 40), 28 * s);
    u.dl->AddRectFilled(a, b, K_SHEET, 28 * s);
    u.text(ImVec2(a.x + pad, a.y + pad), 21, K_INK, "Nouvelle scène", true);
    if (puck(u, ImVec2(b.x - pad - 20 * s, a.y + pad + 14 * s), 20 * s, IC_X)) { m_newSceneOpen = false; return; }
    struct Card { const char* title; const char* sub; int act; };
    const Card cards[3] = {{"Plaque verte", "32 × 32 tenons, lumière du matin", 0}, {"Sol libre", "Rien sur la scène", 1}, {"Depuis une notice", "Fichier .ldr / .mpd officiel, étape par étape", 2}};
    float x = a.x + pad;
    const float y = a.y + pad + 44 * s;
    for (const Card& c : cards) {
        const ImVec2 ca(x, y), cb(x + cardW, y + cardH);
        const bool h = u.hover(ca, cb);
        u.dl->AddRectFilled(ImVec2(ca.x, ca.y + 3 * s), ImVec2(cb.x, cb.y + 3 * s), IM_COL32(30, 25, 20, 20), 20 * s);
        u.dl->AddRectFilled(ca, cb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? C_CARD_HI : IM_COL32(255, 255, 255, 255), 20 * s);
        const ImVec2 ia(ca.x + 10 * s, ca.y + 10 * s), ib(cb.x - 10 * s, ca.y + 10 * s + cardW * 0.62f - 10 * s);
        // a drawn picture per card
        if (c.act == 0) {
            u.dl->AddRectFilledMultiColor(ia, ib, IM_COL32(0xf7, 0xf6, 0xf4, 255), IM_COL32(0xf7, 0xf6, 0xf4, 255), IM_COL32(0xe9, 0xe7, 0xe3, 255), IM_COL32(0xe9, 0xe7, 0xe3, 255));
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
            u.dl->AddRectFilledMultiColor(ia, ib, IM_COL32(0xfa, 0xfa, 0xf9, 255), IM_COL32(0xfa, 0xfa, 0xf9, 255), IM_COL32(0xee, 0xed, 0xea, 255), IM_COL32(0xee, 0xed, 0xea, 255));
        } else {
            u.dl->AddRectFilled(ia, ib, K_RED, 12 * s);
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
    // the scene in progress (a thumbnail of the view) and the official house
    const float rh = 76 * s;
    {
        const ImVec2 ra(a.x + pad, ry + 24 * s), rb(ra.x + cardW, ry + 24 * s + rh);
        const bool h0 = u.hover(ra, rb);
        u.dl->AddRectFilled(ra, rb, h0 && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? K_SOFT : IM_COL32(255, 255, 255, 255), 16 * s);
        const ImVec2 ia(ra.x + 8 * s, ra.y + 8 * s), ib(ia.x + (rh - 16 * s) * 1.33f, rb.y - 8 * s);
        const float sw = m_stage.z - m_stage.x, sh = m_stage.w - m_stage.y;
        const float aspect = (ib.x - ia.x) / (ib.y - ia.y), view = sw / std::max(1.0f, sh);
        const float cu = aspect < view ? 0.5f * (1 - aspect / view) : 0, cv = aspect < view ? 0 : 0.5f * (1 - view / aspect);
        u.dl->AddImageRounded(m_gui->viewportTexture(), ia, ib, ImVec2(cu, cv), ImVec2(1 - cu, 1 - cv), IM_COL32_WHITE, 10 * s);
        char t2[64];
        snprintf(t2, sizeof(t2), "%d pièces", (int)m_bricks.size());
        u.text(ImVec2(ib.x + 12 * s, ra.y + 18 * s), 13.5f, K_INK, "Scène en cours", true);
        u.text(ImVec2(ib.x + 12 * s, ra.y + 42 * s), 11, K_INK2, t2);
        if (h0 && u.pressed) { m_newSceneOpen = false; return; }
    }
    const ImVec2 ra(a.x + pad + cardW + 16 * s, ry + 24 * s), rb(ra.x + cardW, ry + 24 * s + rh);
    const bool h = u.hover(ra, rb);
    u.dl->AddRectFilled(ra, rb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? K_SOFT : IM_COL32(255, 255, 255, 255), 16 * s);
    {   // a little white house on a green plate
        const ImVec2 ia(ra.x + 8 * s, ra.y + 8 * s), ib(ia.x + (rh - 16 * s) * 1.33f, rb.y - 8 * s);
        u.dl->AddRectFilledMultiColor(ia, ib, IM_COL32(0xf7, 0xf6, 0xf4, 255), IM_COL32(0xf7, 0xf6, 0xf4, 255), IM_COL32(0xe9, 0xe7, 0xe3, 255), IM_COL32(0xe9, 0xe7, 0xe3, 255));
        lucide(u.dl, IC_HOUSE, ImVec2((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f), 34 * s, K_INK, 1.8f);
        u.text(ImVec2(ib.x + 12 * s, ra.y + 18 * s), 13.5f, K_INK, "Maison moderne 31153", true);
        u.text(ImVec2(ib.x + 12 * s, ra.y + 42 * s), 11, K_INK2, "Notice officielle LDraw");
    }
    if (h && u.pressed) { m_newSceneOpen = false; openOfficialHouse(); }
}
