// BrickIt - the interface of the iPad (included at the end of BrickUi.cpp: the same drawing helpers and colours).
// UI kit 2.0 validated by Wzx (08/11/2026, « Exactement ça », docs/brickit/ui_kit/KIT.md on the kit branch): the « menu
// compact » screen of maquette v5: a translucent cream HUD, brown bubbles around the selection, Fredoka.
//   - top: Maison (files, quality, light, grid, Pencil only), the pill (scene or selected part), undo / redo;
//   - around the selection: a dashed frame, Tourner / Dupliquer / Couleur / Déplacer and a small « … » (delete, group,
//     isolate, take its colour, part sheet); Couleur opens an arc of studs, « + » the Nuancier;
//   - bottom: the dock (yellow button = the library drawer, the last parts), the compass, the active colour, Recadrer;
//   - Nouvelle scène: three cards and the scenes to resume.
// The PC keeps its interface (m_ipad false), the editing underneath is the same code.

static void uiTheme(bool warm) {
    if (warm) {   // the kit's cream and brown (the panels shared with the PC: reader, part sheet)
        C_BG = IM_COL32(0xf9, 0xf5, 0xeb, 255);
        C_PANEL = IM_COL32(251, 246, 237, 245);
        C_PANEL_SOLID = IM_COL32(0xf9, 0xf5, 0xeb, 255);
        C_CARD = IM_COL32(0xff, 0xff, 0xff, 255);
        C_CARD_HI = IM_COL32(0xef, 0xe5, 0xd4, 255);
        C_LINE = IM_COL32(0xe3, 0xd7, 0xc3, 255);
        C_TEXT = IM_COL32(0x57, 0x46, 0x38, 255);
        C_DIM = IM_COL32(0x96, 0x86, 0x72, 255);
        C_FAINT = IM_COL32(0xb6, 0xa6, 0x90, 255);
        C_ACCENT = IM_COL32(0xc9, 0x45, 0x2f, 255);
        C_INK = IM_COL32(0x57, 0x46, 0x38, 255);
        C_OVERLAY = IM_COL32(251, 246, 237, 238);
        C_HUD = IM_COL32(255, 249, 241, 199);
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
    C_PROGRESS = C_ACCENT;   // (the sliders: the accent, as before the themes)
}

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

// ================================================================== UI kit 2.0 (docs/brickit/ui_kit/KIT.md, 08/11/2026)
// The « menu compact » screen of maquette v5 validated by Wzx (« Exactement ça »): a translucent HUD (cream pebbles 78 %,
// brown bubbles 48 % with white icons), Fredoka, the top bar (Maison, the scene pill, undo / redo), the compact cross
// around a dashed frame, the arc of studs, the cream dock with its yellow button, the active colour bottom right.
// Every size is in iPad points (x m_ui). Icons and shapes: the kit's white masks (assets/brickit/ui_kit), drawn tinted.

// ---- tokens (tokens.json > color, measured on the validated image)
static const ImU32 T_BRUN = IM_COL32(0x57, 0x46, 0x38, 255);
static const ImU32 T_BRUN_DOUX = IM_COL32(0x8d, 0x7a, 0x64, 255);
static const ImU32 T_BRUN_PALE = IM_COL32(0x96, 0x86, 0x72, 255);
static const ImU32 T_INACTIF = IM_COL32(0xb6, 0xa6, 0x90, 255);
static const ImU32 T_ACTIF = IM_COL32(0x31, 0x2e, 0x27, 255);
static const ImU32 T_GALET = IM_COL32(255, 249, 241, 199);
static const ImU32 T_BULLE = IM_COL32(64, 46, 32, 122);
static const ImU32 T_BULLE_VISEE = IM_COL32(255, 255, 255, 235);
static const ImU32 T_OMBRE_ICONE = IM_COL32(30, 18, 8, 140);
static const ImU32 T_FEUILLE = IM_COL32(251, 246, 237, 245);
static const ImU32 T_TIROIR = IM_COL32(250, 244, 233, 240);
static const ImU32 T_CREUX = IM_COL32(0xef, 0xe5, 0xd4, 255);
static const ImU32 T_SEP = IM_COL32(0xe3, 0xd7, 0xc3, 255);
static const ImU32 T_POIGNEE = IM_COL32(0xcd, 0xbc, 0xa4, 255);
static const ImU32 T_BLANC = IM_COL32(255, 255, 255, 255);
static const ImU32 T_CARTE_DOCK = IM_COL32(255, 255, 255, 140);
static const ImU32 T_JAUNE = IM_COL32(0xf5, 0xc3, 0x2c, 255);
static const ImU32 T_ROUGE = IM_COL32(0xc9, 0x45, 0x2f, 255);
static const ImU32 T_VERT = IM_COL32(0x8b, 0xbe, 0x68, 255);
static const ImU32 T_ETIQUETTE = IM_COL32(64, 46, 32, 140);
static const ImU32 T_MESSAGE = IM_COL32(64, 46, 32, 184);
static const ImU32 T_VOILE = IM_COL32(60, 42, 28, 71);
static const ImU32 T_SELECTION = IM_COL32(255, 255, 255, 242);
static const ImU32 T_SELECTION_OMBRE = IM_COL32(40, 25, 10, 153);
static const ImU32 T_OMBRE = IM_COL32(70, 45, 20, 89);
static const ImU32 T_OMBRE_FEUILLE = IM_COL32(60, 40, 20, 128);
static const ImU32 T_OMBRE_DOUCE = IM_COL32(70, 45, 20, 46);

static ImU32 kitCol(const Color& c, float a = 1.0f) {
    return IM_COL32((int)(std::clamp(c.srgb.x, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.y, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.z, 0.0f, 1.0f) * 255), (int)(a * 255));
}

// ---- Fredoka (Gfx fontKit / fontKitMed; the default font on the PC)
static ImVec2 g_morePos(0, 0);
static ImFont* g_kf = nullptr;
static ImFont* g_kfm = nullptr;
static ImFont* kF(bool med) { ImFont* f = med ? g_kfm : g_kf; return f ? f : ImGui::GetFont(); }
// The kit's sizes are CSS sizes (em); ImGui sizes a font by its ascent - descent, 1.21 em for Fredoka (974 + 236 / 1000)
static const float kEm = 1.21f;
static ImVec2 kSize(float px, const char* t, bool med = false) { return kF(med)->CalcTextSizeA(px * kEm, 1e9f, 0, t); }
static void kText(ImDrawList* dl, ImVec2 p, float px, ImU32 col, const char* t, bool med = false) { dl->AddText(kF(med), px * kEm, p, col, t); }
static void kTextC(ImDrawList* dl, float x, float cy, float px, ImU32 col, const char* t, bool med = false) {   // vertically centred on cy
    kText(dl, ImVec2(x, cy - px * kEm * 0.52f), px, col, t, med);
}
static std::string upperFr(std::string t) {   // ASCII + the accents of the families (É È Ê À)
    for (size_t i = 0; i < t.size(); ++i) {
        unsigned char c = (unsigned char)t[i];
        if (c == 0xC3 && i + 1 < t.size()) { unsigned char n = (unsigned char)t[i + 1]; if (n >= 0xA0 && n <= 0xBE) t[i + 1] = (char)(n - 0x20); ++i; }
        else t[i] = (char)std::toupper(c);
    }
    return t;
}

// ---- the kit's images (white masks), loaded on first use
ImTextureID BrickApp::kitTex(const std::string& name) {
    auto it = m_kitTex.find(name);
    if (it != m_kitTex.end()) return it->second;
    ImTextureID tex = 0;
    std::wstring path = m_exeDir + L"\\assets\\brickit\\ui_kit\\";
    for (char ch : name) path += ch == '/' ? L'\\' : (wchar_t)ch;
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") == 0 && f) {
        std::vector<uint8_t> data;
        uint8_t buf[65536];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
        fclose(f);
        std::vector<uint8_t> rgba;
        int w = 0, h = 0;
        if (image::decode(data.data(), data.size(), rgba, w, h) && w > 0 && h > 0) tex = m_gui->createTexture(rgba.data(), (uint32_t)w, (uint32_t)h);
    }
    m_kitTex[name] = tex;
    return tex;
}

// couleurs.json: the colours of the Nuancier, French names and families (sorted by hand)
void BrickApp::kitColors() {
    if (m_kitColorsLoaded) return;
    m_kitColorsLoaded = true;
    std::wstring path = m_exeDir + L"\\assets\\brickit\\ui_kit\\couleurs.json";
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) return;
    std::string text;
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
    fclose(f);
    app::Json j;
    if (!app::Json::parse(text, j) || j.type != app::Json::Array) return;
    for (const app::Json& e : j.a) {
        KitColor kc;
        kc.ldraw = (int)e["ldraw"].n;
        kc.nom = e["nom"].s;
        kc.famille = e["famille"].s;
        kc.index = colorIndex(kc.ldraw);
        if (kc.index >= 0 && kc.index < (int)m_lib.palette.size() && m_lib.palette[kc.index].id == kc.ldraw) m_kitColorList.push_back(kc);
    }
}

std::string BrickApp::colorNameFr(int index) {
    kitColors();
    for (const KitColor& kc : m_kitColorList) if (kc.index == index) return kc.nom;
    if (index < 0 || index >= (int)m_lib.palette.size()) return "";
    return colorFr(m_lib.palette[index].name);
}

// ---- drawing components
static void kitImage(ImDrawList* dl, ImTextureID tex, ImVec2 c, float size, ImU32 tint) {
    if (!tex) return;
    dl->AddImage(tex, ImVec2(c.x - size * 0.5f, c.y - size * 0.5f), ImVec2(c.x + size * 0.5f, c.y + size * 0.5f), ImVec2(0, 0), ImVec2(1, 1), tint);
}

// an icon (white mask, tinted); shadow: the white icons of the bubbles carry a drop shadow (+1 pt)
void BrickApp::kitIcon(ImDrawList* dl, const char* name, ImVec2 c, float size, ImU32 tint, bool shadow) {
    ImTextureID tex = kitTex(std::string("icons/") + name + ".png");
    if (shadow) kitImage(dl, tex, ImVec2(c.x, c.y + 1 * m_ui), size, T_OMBRE_ICONE);
    kitImage(dl, tex, c, size, tint);
}

// the soft round shadow under a disc of diameter `diam` (the useful disc is half the texture)
void BrickApp::kitShadowCircle(ImDrawList* dl, ImVec2 c, float diam, ImU32 tint, float dy) {
    kitImage(dl, kitTex("shapes/shadow_circle.png"), ImVec2(c.x, c.y + dy), diam * 2, tint);
}

// the shadow of a rounded rectangle: 9 slices of 48 px, the 50 % edge on the slice line; the CSS spread ~ -0.35 blur
void BrickApp::kitShadowRect(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 tint, float dy, float blur) {
    ImTextureID tex = kitTex("shapes/shadow_rrect_9slice.png");
    if (!tex) return;
    const float m = blur * 0.5f, in = blur * 0.35f;
    a = ImVec2(a.x + in, a.y + in + dy);
    b = ImVec2(b.x - in, b.y - in + dy);
    const float xs[4] = {a.x - m, a.x + m, b.x - m, b.x + m}, ys[4] = {a.y - m, a.y + m, b.y - m, b.y + m};
    const float us[4] = {0, 0.25f, 0.75f, 1};
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k) {
            if (xs[i + 1] <= xs[i] || ys[k + 1] <= ys[k]) continue;
            dl->AddImage(tex, ImVec2(xs[i], ys[k]), ImVec2(xs[i + 1], ys[k + 1]), ImVec2(us[i], us[k]), ImVec2(us[i + 1], us[k + 1]), tint);
        }
}

// 6. a colour stud: shadow (in the scene), LEGO colour disc, the relief untinted; current: white ring (scene) / brown (sheet)
void BrickApp::kitStud(ImDrawList* dl, ImVec2 c, float size, int color, int where, bool current) {
    const float s = m_ui, r = size * 0.5f;
    const Color& col = m_lib.palette[std::clamp(color, 0, (int)m_lib.palette.size() - 1)];
    const bool trans = col.group == "transparent", metal = col.group == "metallic" || col.group == "metal" || col.group == "chrome" || col.group == "pearl";
    if (where == 0) kitShadowCircle(dl, c, size, IM_COL32(40, 25, 10, 115), 3 * s);
    if (current && where == 0) dl->AddCircleFilled(c, r + 6 * s, T_BLANC, 48);
    if (current && where == 1) dl->AddCircle(c, r + 3.5f * s, T_BRUN, 48, 3 * s);
    dl->AddCircleFilled(c, r, kitCol(col, trans ? 0.88f : 1.0f), 48);
    if (trans) {   // a slanted gradient towards white 60 %
        dl->AddCircleFilled(ImVec2(c.x - r * 0.25f, c.y - r * 0.25f), r * 0.7f, IM_COL32(255, 255, 255, 60), 40);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.4f, c.y - r * 0.4f), r * 0.4f, IM_COL32(255, 255, 255, 70), 32);
    }
    if (metal) {
        dl->AddCircleFilled(ImVec2(c.x - r * 0.3f, c.y - r * 0.3f), r * 0.5f, IM_COL32(255, 255, 255, 80), 32);
        dl->AddCircleFilled(ImVec2(c.x + r * 0.35f, c.y + r * 0.35f), r * 0.45f, IM_COL32(0, 0, 0, 50), 32);
    }
    kitImage(dl, kitTex("shapes/tenon_relief.png"), c, size, T_BLANC);
    if (where == 1 && (col.srgb.x + col.srgb.y + col.srgb.z) > 2.7f) dl->AddCircle(c, r, IM_COL32(0xd9, 0xcb, 0xb4, 255), 48, 1 * s);
}

// 1. a pebble (« galet »): shadow, translucent cream disc, light rim on top, brown icon.
// state: 0 normal, 1 active (dark, white icon), 2 inactive (pale icon, no tap), 3 white 38 (sheet header, no shadow)
bool BrickApp::kitGalet(Ui& u, ImVec2 c, float diam, const char* icon, int state) {
    const float s = m_ui, r = diam * 0.5f;
    const bool h = u.hover(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
    const bool down = state != 2 && h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const float rr = r * (down ? 0.94f : 1.0f);
    if (state != 3) kitShadowCircle(u.dl, c, rr * 2, T_OMBRE, 4 * s);
    const ImU32 fill = state == 1 ? T_ACTIF : down ? T_CREUX : state == 3 ? T_BLANC : T_GALET;
    u.dl->AddCircleFilled(c, rr, fill, 64);
    if (state == 0 || state == 2) {   // the light rim on top
        u.dl->PathArcTo(c, rr - 0.5f * s, 3.14159265f * 1.12f, 3.14159265f * 1.88f, 24);
        u.dl->PathStroke(IM_COL32(255, 255, 255, 204), 0, 1 * s);
    }
    if (icon) kitIcon(u.dl, icon, c, (state == 3 ? 22 : diam >= 50 * s ? 24 : 22) * s, state == 1 ? T_BLANC : state == 2 ? T_INACTIF : T_BRUN);
    m_uiRects.push_back(ImVec4(c.x - r, c.y - r, c.x + r, c.y + r));
    return state != 2 && h && u.pressed;
}

// 2. an action bubble: translucent brown disc, white icon with its drop shadow; pressed 0.94 and a little darker
bool BrickApp::kitBubble(Ui& u, ImVec2 c, float diam, const char* icon, float scale, bool red) {
    const float s = m_ui, r = diam * 0.5f;
    const bool h = u.hover(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
    const bool down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const float rr = r * scale * (down ? 0.94f : 1.0f);
    kitShadowCircle(u.dl, c, rr * 2, IM_COL32(40, 25, 10, 128), 3 * s);
    u.dl->AddCircleFilled(c, rr, down ? IM_COL32(64, 46, 32, 148) : T_BULLE, 48);
    kitIcon(u.dl, icon, c, (diam >= 44 * s ? 24 : 18) * s * scale, red ? T_ROUGE : T_BLANC, true);
    m_uiRects.push_back(ImVec4(c.x - r, c.y - r, c.x + r, c.y + r));
    return h && u.pressed;
}

// 11. a sheet: cream 96 %, radius 26, its 9-slice shadow
void BrickApp::kitSheet(ImDrawList* dl, ImVec2 a, ImVec2 b, ImDrawFlags flags) {
    kitShadowRect(dl, a, b, T_OMBRE_FEUILLE, 18 * m_ui, 40 * m_ui);
    dl->AddRectFilled(a, b, T_FEUILLE, 26 * m_ui, flags);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
}

// 11. segmented control: a hollow pill, the active option white with a soft shadow. a = top left (top right if alignRight);
// fullW > 0: that width, the options shared equally. Returns the option tapped or -1.
static float g_segFullW = 0;
int BrickApp::kitSegmented(Ui& u, ImVec2 a, float h, const std::vector<const char*>& opts, int cur, bool alignRight) {
    const float s = m_ui, pad = 3 * s, px = 14 * s;
    std::vector<float> ws;
    float W = 2 * pad;
    for (const char* o : opts) { ws.push_back(kSize(px, o).x + 22 * s); W += ws.back(); }
    if (g_segFullW > 0) { const float k = (g_segFullW - 2 * pad) / (W - 2 * pad); for (float& w : ws) w *= k; W = g_segFullW; }
    if (alignRight) a.x -= W;
    const ImVec2 b(a.x + W, a.y + h);
    u.dl->AddRectFilled(a, b, T_CREUX, h * 0.5f);
    int hit = -1;
    float x = a.x + pad;
    for (int k = 0; k < (int)opts.size(); ++k) {
        const ImVec2 oa(x, a.y + pad), ob(x + ws[k], b.y - pad);
        if (k == cur) {
            u.dl->AddRectFilled(ImVec2(oa.x, oa.y + 1.5f * s), ImVec2(ob.x, ob.y + 1.5f * s), T_OMBRE_DOUCE, (ob.y - oa.y) * 0.5f);
            u.dl->AddRectFilled(oa, ob, T_BLANC, (ob.y - oa.y) * 0.5f);
        }
        const float tw = kSize(px, opts[k]).x;
        kTextC(u.dl, (oa.x + ob.x) * 0.5f - tw * 0.5f, (oa.y + ob.y) * 0.5f, px, k == cur ? T_BRUN : T_BRUN_DOUX, opts[k]);
        if (u.pressed && u.hover(oa, ob)) hit = k;
        x += ws[k];
    }
    g_segFullW = 0;
    return hit;
}

// 11. a menu row: padding 11 x 14, icon 24 + label 17, detail 14 on the right; pressed: white, radius 14
bool BrickApp::kitRow(Ui& u, ImVec2 a, float w, const char* icon, const char* label, const char* note, bool red) {
    const float s = m_ui, H = 50 * s;
    const ImVec2 b(a.x + w, a.y + H);
    const bool h = u.hover(a, b);
    if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRectFilled(a, b, T_BLANC, 14 * s);
    const ImU32 ink = red ? T_ROUGE : T_BRUN;
    if (icon) kitIcon(u.dl, icon, ImVec2(a.x + 14 * s + 12 * s, a.y + H * 0.5f), 24 * s, ink);
    kTextC(u.dl, a.x + 14 * s + 24 * s + 14 * s, a.y + H * 0.5f, 17 * s, ink, label);
    if (note) { const float tw = kSize(14 * s, note).x; kTextC(u.dl, b.x - 14 * s - tw, a.y + H * 0.5f, 14 * s, T_BRUN_PALE, note); }
    return h && u.pressed;
}

// 12. a label: dark translucent pill, white 13/500, padding 5 x 9
void BrickApp::kitLabel(ImDrawList* dl, ImVec2 c, const std::string& t) {
    const float s = m_ui, px = 13 * s;
    const ImVec2 ts = kSize(px, t.c_str(), true);
    const ImVec2 a(c.x - ts.x * 0.5f - 9 * s, c.y - px * 0.6f - 5 * s), b(c.x + ts.x * 0.5f + 9 * s, c.y + px * 0.6f + 5 * s);
    dl->AddRectFilled(a, b, T_ETIQUETTE, (b.y - a.y) * 0.5f);
    kTextC(dl, a.x + 9 * s, c.y, px, T_BLANC, t.c_str(), true);
}

// a dashed rounded rectangle (the selection frame)
static void dashedRRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float rad, ImU32 col, float thick, float dash, float gap) {
    dl->PathRect(a, b, rad);
    std::vector<ImVec2> pts(dl->_Path.Data, dl->_Path.Data + dl->_Path.Size);
    dl->PathClear();
    if (pts.size() < 2) return;
    pts.push_back(pts[0]);
    float t = 0;   // position in the dash pattern
    bool on = true;
    std::vector<ImVec2> cur;
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        ImVec2 p = pts[i];
        const ImVec2 q = pts[i + 1];
        float len = std::sqrt((q.x - p.x) * (q.x - p.x) + (q.y - p.y) * (q.y - p.y));
        if (len <= 0) continue;
        const ImVec2 dir((q.x - p.x) / len, (q.y - p.y) / len);
        if (on && cur.empty()) cur.push_back(p);
        while (len > 0) {
            const float left = (on ? dash : gap) - t, step = std::min(left, len);
            p = ImVec2(p.x + dir.x * step, p.y + dir.y * step);
            len -= step;
            t += step;
            if (on) cur.push_back(p);
            if (t >= (on ? dash : gap) - 1e-4f) {
                if (on && cur.size() >= 2) dl->AddPolyline(cur.data(), (int)cur.size(), col, 0, thick);
                cur.clear();
                on = !on;
                t = 0;
                if (on) cur.push_back(p);
            }
        }
    }
    if (on && cur.size() >= 2) dl->AddPolyline(cur.data(), (int)cur.size(), col, 0, thick);
}

// the scene name for the pill (the file name, « Ma scène » for a new one)
static std::string sceneTitle(const std::wstring& path) {
    if (path.empty()) return "Ma scène";
    std::wstring n = path.substr(path.find_last_of(L"\\/") + 1);
    for (const wchar_t* ext : {L".v3d.json", L".json", L".ldr", L".mpd"}) {
        const size_t l = wcslen(ext);
        if (n.size() > l && n.compare(n.size() - l, l, ext) == 0) { n.resize(n.size() - l); break; }
    }
    std::string o = plat::toUtf8(n);
    for (char& c : o) if (c == '_') c = ' ';
    if (!o.empty()) o[0] = (char)std::toupper((unsigned char)o[0]);
    return o;
}

// ------------------------------------------------------------------ the whole interface of the iPad

void BrickApp::drawUiIpad() {
    g_kf = m_gui->fontKit;
    g_kfm = m_gui->fontKitMed ? m_gui->fontKitMed : m_gui->fontKit;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // the stage: the whole screen (index 0 / 1 of m_uiRects as the PC: the top bar and the tray), the drawer excepted
    m_uiRects.push_back(ImVec4(0, 0, d.x, 0));
    if (m_libOpen) ipadDrawer();           // m_uiRects[1]
    else m_uiRects.push_back(ImVec4(0, d.y, d.x, d.y));
    ipadBubbles();
    ipadTopBar();
    ipadCorners();
    ipadDock();
    if (m_moreOpen && m_selKey >= 0) ipadMoreMenu(g_morePos);
    uiReader();
    uiManual();
    uiInfoSheet();
    ipadNuancier();
    ipadHomeMenu();
    ipadNewScene();
}

void BrickApp::ipadPucks() {}
void BrickApp::ipadColorArc() {}

// ------------------------------------------------------------------ 5. the top bar: Maison, the pill, undo / redo

void BrickApp::ipadTopBar() {
    if (m_newSceneOpen) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    if (kitGalet(u, ImVec2(22 * s + 26 * s, 20 * s + 26 * s), 52 * s, "maison", m_homeOpen ? 1 : 0)) {
        m_homeOpen = !m_homeOpen; m_paletteOpen = false; m_arcOpen = false; m_moreOpen = false;
        u.pressed = false;
    }
    // the pill: « Maison moderne · 214 pièces · enregistrée », or the selected part « Brique 1 × 6 · Bleu · 3009 »
    if (!m_homeOpen && !m_paletteOpen && !m_libOpen) {
        std::string name, detail;
        const std::vector<int> sel = selection();
        if (sel.size() == 1 && sel[0] >= 0 && sel[0] < (int)m_bricks.size()) {
            const Brick& br = m_bricks[sel[0]];
            name = partFr(m_lib.parts[br.part].label);
            for (size_t k = 1; k + 1 < name.size(); ++k)   // « 1x6 » -> « 1 × 6 »
                if ((name[k] == 'x' || name[k] == 'X') && std::isdigit((unsigned char)name[k - 1]) && std::isdigit((unsigned char)name[k + 1])) { name.replace(k, 1, " \xC3\x97 "); k += 3; }
            detail = "· " + colorNameFr(br.color) + " · " + m_lib.parts[br.part].name;
        } else if (sel.size() > 1) {
            name = std::to_string(sel.size()) + " pièces";
            detail = "· sélection";
        } else {
            name = sceneTitle(m_scenePath);
            detail = "· " + std::to_string(m_bricks.size()) + " pièces · enregistrée";
        }
        const float pn = 16 * s, pd = 14 * s;
        const float wn = kSize(pn, name.c_str()).x, wd = kSize(pd, detail.c_str()).x;
        const float W = 18 * s + 8 * s + 10 * s + wn + 7 * s + wd + 18 * s, H = 42 * s;
        const ImVec2 a((d.x - W) * 0.5f, 25 * s), b(a.x + W, a.y + H);
        kitShadowRect(u.dl, a, b, T_OMBRE, 4 * s, 14 * s);
        u.dl->AddRectFilled(a, b, T_GALET, H * 0.5f);
        const float cy = (a.y + b.y) * 0.5f;
        u.dl->AddCircleFilled(ImVec2(a.x + 18 * s + 4 * s, cy), 4 * s, T_VERT, 16);
        kTextC(u.dl, a.x + 18 * s + 8 * s + 10 * s, cy, pn, T_BRUN, name.c_str());
        kTextC(u.dl, a.x + 18 * s + 8 * s + 10 * s + wn + 7 * s, cy + 1 * s, pd, T_BRUN_PALE, detail.c_str());
        m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    }
    if (m_paletteOpen) return;   // (the Nuancier sits there)
    const bool canUndo = !m_undo.empty() || m_holding, canRedo = !m_redo.empty();
    if (kitGalet(u, ImVec2(d.x - 76 * s - 22 * s, 24 * s + 22 * s), 44 * s, "annuler", canUndo ? 0 : 2)) undo();
    if (kitGalet(u, ImVec2(d.x - 24 * s - 22 * s, 24 * s + 22 * s), 44 * s, "retablir", canRedo ? 0 : 2)) redo();
}

// ------------------------------------------------------------------ 9. the corners: compass, Recadrer, the active colour

void BrickApp::ipadCorners() {
    if (m_libOpen || m_newSceneOpen || m_reading) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // bottom left: the compass, turning with the camera; a tap = the nearest 45° view
    {
        const ImVec2 c(24 * s + 22 * s, d.y - 24 * s - 22 * s);
        if (kitGalet(u, c, 44 * s, nullptr, 0)) {
            const float q = 3.14159265f * 0.25f;
            m_scene.camera.yaw = std::round(m_scene.camera.yaw / q) * q;
            touchCamera();
        }
        ImTextureID tex = kitTex("icons/boussole.png");
        if (tex) {
            const float an = -m_scene.camera.yaw, h = 11 * s, cs = std::cos(an), sn = std::sin(an);
            auto rot = [&](float x, float y) { return ImVec2(c.x + x * cs - y * sn, c.y + x * sn + y * cs); };
            u.dl->AddImageQuad(tex, rot(-h, -h), rot(h, -h), rot(h, h), rot(-h, h), ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), T_BRUN);
        }
    }
    // bottom right: the active colour (stud 60), a tap = the arc (a selection) or the Nuancier; Recadrer above
    const ImVec2 cc(d.x - 22 * s - 30 * s, d.y - 22 * s - 30 * s);
    {
        const bool h = u.hover(ImVec2(cc.x - 30 * s, cc.y - 30 * s), ImVec2(cc.x + 30 * s, cc.y + 30 * s));
        kitStud(u.dl, cc, (h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? 56 : 60) * s, m_color, 0, false);
        m_uiRects.push_back(ImVec4(cc.x - 30 * s, cc.y - 30 * s, cc.x + 30 * s, cc.y + 30 * s));
        if (h && u.pressed) {
            u.pressed = false;
            if (!selection().empty() && !m_arcOpen) { m_arcOpen = true; m_moreOpen = false; }
            else { m_paletteOpen = !m_paletteOpen; m_homeOpen = false; }
        }
    }
    if (kitGalet(u, ImVec2(cc.x, cc.y - 30 * s - 12 * s - 22 * s), 44 * s, "recadrer", 0)) {
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
}

// ------------------------------------------------------------------ 8. the dock

void BrickApp::ipadDock() {
    if (m_reading || m_libOpen || m_newSceneOpen) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // the last parts, the most recent on the left, tinted with the active colour; a tap = in hand in the middle of the
    // view, a drag up = in hand under the finger
    std::vector<int> parts;
    for (int p : m_recent) if (p >= 0 && p < (int)m_lib.parts.size() && (int)parts.size() < 8) parts.push_back(p);
    if (parts.size() < 8)
        for (const char* nm : {"3001", "3003", "3010", "3004", "3020", "3062b", "3040b", "3022"}) {
            const int p = m_lib.find(nm);
            if (p >= 0 && (int)parts.size() < 8 && std::find(parts.begin(), parts.end(), p) == parts.end()) parts.push_back(p);
        }
    const float bw = 56 * s, cw = 70 * s, ch = 56 * s, gap = 6 * s, padX = 10 * s, padY = 8 * s;
    const float W = padX * 2 + bw + parts.size() * (cw + gap), H = ch + padY * 2;
    const ImVec2 a((d.x - W) * 0.5f, d.y - 18 * s - H), b(a.x + W, a.y + H);
    kitShadowRect(u.dl, a, b, T_OMBRE, 8 * s, 24 * s);
    u.dl->AddRectFilled(a, b, T_GALET, 28 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    // the handle 46 x 5, 12 pt above: a tap (or the yellow button) opens the drawer
    const ImVec2 ha(d.x * 0.5f - 23 * s, a.y - 12 * s - 5 * s), hb(d.x * 0.5f + 23 * s, a.y - 12 * s);
    u.dl->AddRectFilled(ha, hb, T_POIGNEE, 2.5f * s);
    m_uiRects.push_back(ImVec4(ha.x - 20 * s, ha.y - 12 * s, hb.x + 20 * s, a.y));
    if (u.pressed && u.hover(ImVec2(ha.x - 20 * s, ha.y - 12 * s), ImVec2(hb.x + 20 * s, a.y))) { m_libOpen = true; m_homeOpen = false; m_libPress = -1; return; }
    {   // the yellow button: all the parts
        const ImVec2 ya(a.x + padX, a.y + padY), yb(ya.x + bw, ya.y + ch);
        const bool h = u.hover(ya, yb), down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        u.dl->AddRectFilled(ya, yb, down ? IM_COL32(0xe6, 0xb2, 0x1f, 255) : T_JAUNE, 16 * s);
        kitIcon(u.dl, "brique", ImVec2((ya.x + yb.x) * 0.5f, (ya.y + yb.y) * 0.5f), 26 * s, T_BRUN);
        if (h && u.pressed) { m_libOpen = true; m_homeOpen = false; m_paletteOpen = false; m_libPress = -1; return; }
    }
    const ImU32 tint = kitCol(m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)]);
    float x = a.x + padX + bw + gap;
    for (int p : parts) {
        const ImVec2 ca(x, a.y + padY), cb(x + cw, a.y + padY + ch);
        const bool h = u.hover(ca, cb);
        u.dl->AddRectFilled(ca, cb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? T_BLANC : T_CARTE_DOCK, 16 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(p, tex, uv0, uv1)) {
            const ImVec2 du((uv1.x - uv0.x) * 0.14f, (uv1.y - uv0.y) * 0.14f);
            const float side = ch - 8 * s;
            const ImVec2 m((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f);
            u.dl->AddImage(tex, ImVec2(m.x - side * 0.5f, m.y - side * 0.5f), ImVec2(m.x + side * 0.5f, m.y + side * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), tint);
        }
        if (h && u.pressed) { m_dockPress = p; m_dockPressPos = u.mouse; }
        x += cw + gap;
    }
    if (m_dockPress >= 0) {
        const float dx = u.mouse.x - m_dockPressPos.x, dy = u.mouse.y - m_dockPressPos.y;
        // dragged up within the 160° arc above: in hand under the finger
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && u.mouse.y < a.y - 6 * s && dx * dx + dy * dy > 49 * s * s && -dy > std::abs(dx) * 0.18f) {
            m_libSel = m_dockPress;
            spawn(m_dockPress, u.mouse.x, u.mouse.y);
            m_dragStarted = true;   // dropped on release (editInput)
            m_dockPress = -1;
        } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (dx * dx + dy * dy < 100 * s * s) { m_libSel = m_dockPress; spawn(m_dockPress, d.x * 0.5f, d.y * 0.45f); }   // a tap: in hand in the middle
            m_dockPress = -1;
        }
    }
}

// ------------------------------------------------------------------ 3. the compact menu, 7. the arc of colours

void BrickApp::ipadBubbles() {
    ImVec2 a, b;
    const bool show = !m_holding && !m_mechOn && !m_reading && !m_homeOpen && !m_newSceneOpen && selectionRect(a, b);
    const int key = m_selected * 7 + (int)m_multi.size();
    if (!show) { m_selKey = -1; m_moreOpen = false; m_arcOpen = false; return; }
    const double now = ImGui::GetTime();
    if (key != m_selKey) { m_selKey = key; m_selT0 = now; m_moreOpen = false; m_arcOpen = false; }
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    // the frame: at least 60 x 36 pt, 4 pt around the part
    a = ImVec2(std::min(a.x - 4 * s, c.x - 30 * s), std::min(a.y - 4 * s, c.y - 18 * s));
    b = ImVec2(std::max(b.x + 4 * s, c.x + 30 * s), std::max(b.y + 4 * s, c.y + 18 * s));
    dashedRRect(u.dl, ImVec2(a.x, a.y + 0.75f * s), ImVec2(b.x, b.y + 0.75f * s), 8 * s, T_SELECTION_OMBRE, 3.5f * s, 6 * s, 4 * s);
    dashedRRect(u.dl, a, b, 8 * s, T_SELECTION, 2 * s, 6 * s, 4 * s);
    const int cur = m_selected >= 0 && m_selected < (int)m_bricks.size() ? m_bricks[m_selected].color : m_color;
    auto appear = [&](int order) {   // 0.85 -> 1 in 160 ms, 25 ms apart, ease-out cubic
        const float e = std::clamp((float)(now - m_selT0 - order * 0.025) / 0.16f, 0.0f, 1.0f);
        return 0.85f + 0.15f * (1.0f - (1.0f - e) * (1.0f - e) * (1.0f - e));
    };
    if (m_arcOpen) {
        // 7. the arc: the current colour, then the most present in the scene; « + » at the end opens the Nuancier
        std::vector<int> cols;
        cols.push_back(cur);
        std::map<int, int> used;
        for (const Brick& br : m_bricks) ++used[br.color];
        std::vector<std::pair<int, int>> byUse(used.begin(), used.end());
        std::sort(byUse.begin(), byUse.end(), [](const std::pair<int, int>& x, const std::pair<int, int>& y) { return x.second > y.second; });
        for (auto& kv : byUse) if ((int)cols.size() < 9 && std::find(cols.begin(), cols.end(), kv.first) == cols.end()) cols.push_back(kv.first);
        static const int kUsual[] = {15, 71, 4, 14, 2, 19, 0, 70, 1, 72};
        for (int id : kUsual) { const int k = colorIndex(id); if ((int)cols.size() < 9 && k >= 0 && std::find(cols.begin(), cols.end(), k) == cols.end()) cols.push_back(k); }
        auto pos = [&](int i) {
            const float th = (-84.0f + 168.0f * i / 9.0f) * 3.14159265f / 180.0f;
            return ImVec2(c.x + 40 * s + 0.8f * 150 * s * std::cos(th), c.y + 150 * s * std::sin(th));
        };
        bool hit = false;
        for (int i = (int)cols.size() - 1; i >= 0; --i) {   // (the current one on top)
            const ImVec2 p = pos(i);
            const bool h = u.hover(ImVec2(p.x - 20 * s, p.y - 20 * s), ImVec2(p.x + 20 * s, p.y + 20 * s));
            kitStud(u.dl, p, 40 * s * appear(i) * (h ? 1.15f : 1.0f), cols[i], 0, cols[i] == cur);
            m_uiRects.push_back(ImVec4(p.x - 20 * s, p.y - 20 * s, p.x + 20 * s, p.y + 20 * s));
            if (h && u.pressed && !hit) { hit = true; m_color = cols[i]; applyColor(cols[i]); m_arcOpen = false; }
        }
        {
            const ImVec2 p = pos(9);
            const float r = 20 * s * appear(9);
            const bool h = u.hover(ImVec2(p.x - 20 * s, p.y - 20 * s), ImVec2(p.x + 20 * s, p.y + 20 * s));
            kitShadowCircle(u.dl, p, r * 2, T_OMBRE, 3 * s);
            u.dl->AddCircleFilled(p, r, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? T_CREUX : IM_COL32(255, 249, 239, 230), 48);
            kitIcon(u.dl, "ajouter", p, 22 * s, T_BRUN);
            m_uiRects.push_back(ImVec4(p.x - 20 * s, p.y - 20 * s, p.x + 20 * s, p.y + 20 * s));
            if (h && u.pressed && !hit) { hit = true; m_paletteOpen = true; m_homeOpen = false; }
        }
        const std::vector<int> sel = selection();
        kitLabel(u.dl, ImVec2(c.x, std::max(c.y + 50 * s, b.y + 22 * s)), colorNameFr(cur) + " · " + std::to_string(std::max<size_t>(1, sel.size())));
        return;
    }
    // 3. the cross: Tourner above, Déplacer below, Dupliquer left, Couleur right, « … » small in the top right corner
    const ImVec2 pT((a.x + b.x) * 0.5f, a.y - 36 * s), pB((a.x + b.x) * 0.5f, b.y + 38 * s), pL(a.x - 42 * s, (a.y + b.y) * 0.5f),
        pR(b.x + 42 * s, (a.y + b.y) * 0.5f), pM(b.x + 24 * s, a.y - 24 * s);
    if (kitBubble(u, pT, 46 * s, "tourner", appear(0))) { u.pressed = false; rotateSelected(1); }
    if (kitBubble(u, pR, 46 * s, "couleur", appear(1))) { u.pressed = false; m_arcOpen = true; m_moreOpen = false; }
    if (kitBubble(u, pB, 46 * s, "deplacer", appear(2))) {   // in hand, at the brick; the finger still down carries it
        u.pressed = false;
        if (m_selected >= 0) {
            float sx = c.x, sy = c.y;
            m_pressPoint = m_build.worldVoxel(m_bricks[m_selected], m_build.grabOf(m_bricks[m_selected]));
            view().project(m_pressPoint, sx, sy);
            beginMove(m_selected, sx, sy);
            m_dragStarted = true;
        }
        return;
    }
    if (kitBubble(u, pL, 46 * s, "dupliquer", appear(3))) { u.pressed = false; duplicateSelected(); }
    if (kitBubble(u, pM, 36 * s, "plus_actions", appear(4))) { u.pressed = false; m_moreOpen = !m_moreOpen; }
    (void)d;
    g_morePos = pM;   // the « … » sheet is drawn above the top bar (drawUiIpad)
}

// « … »: a sheet above the bubble, on its right: Supprimer (red), Grouper la sélection, Isoler, Prendre sa couleur, Fiche
void BrickApp::ipadMoreMenu(ImVec2 bubble) {
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    struct Item { const char* icon; const char* label; int act; };
    const Item items[5] = {{"supprimer", "Supprimer", 0}, {"grouper", "Grouper la sélection", 1}, {"isoler", m_focusDim ? "Ne plus isoler" : "Isoler", 2},
                           {"pipette", "Prendre sa couleur", 3}, {"fiche", "Fiche de la pièce", 4}};
    const float W = 282 * s, pad = 11 * s, H = 2 * pad + 5 * 50 * s;
    ImVec2 a(bubble.x - 4 * s, bubble.y + 32 * s - H);
    a.x = std::min(a.x, d.x - 16 * s - W);
    a.y = std::clamp(a.y, 80 * s, d.y - 16 * s - H);   // (under the top bar)
    const ImVec2 b(a.x + W, a.y + H);
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));   // a tap outside closes
    kitSheet(u.dl, a, b);
    float y = a.y + pad;
    for (const Item& it : items) {
        if (kitRow(u, ImVec2(a.x + pad, y), W - 2 * pad, it.icon, it.label, nullptr, it.act == 0)) {
            u.pressed = false;
            switch (it.act) {
            case 0: deleteSelected(); break;
            case 1: groupSelected(); break;
            case 2: m_focusDim = !m_focusDim; syncObjects(); break;
            case 3: if (m_selected >= 0) { m_color = m_bricks[m_selected].color; toast("Couleur prise : " + colorNameFr(m_color)); } break;
            case 4: if (m_selected >= 0) m_infoPart = m_bricks[m_selected].part; break;
            }
            m_moreOpen = false;
            return;
        }
        y += 50 * s;
    }
    if (u.pressed && !u.hover(a, b) && !u.hover(ImVec2(bubble.x - 18 * s, bubble.y - 18 * s), ImVec2(bubble.x + 18 * s, bubble.y + 18 * s))) m_moreOpen = false;
}

// ------------------------------------------------------------------ 10. the library drawer

struct KitTab { const char* label; const char* key; const char* icon; std::vector<const char*> cats; std::vector<const char*> parts; };
static const std::vector<KitTab>& kitTabs() {
    static const std::vector<KitTab> t = {
        {"Récentes", "recent", "recentes", {}, {}}, {"Favoris", "fav", "favoris", {}, {}},
        {"Briques", "Briques", nullptr, {"Brick"}, {"3001"}}, {"Plaques", "Plaques", nullptr, {"Plate", "Tile", "Baseplate"}, {"3020", "3022"}},
        {"Pentes", "Pentes", nullptr, {"Slope", "Wedge"}, {"3040b", "3039", "3040"}},
        {"Rondes", "Rondes", nullptr, {"Cylinder", "Cone", "Dish", "Sphere", "Arch"}, {"3062b", "6143", "3941"}},
        {"Technic", "Technic", "technic", {"Technic", "Electric", "Znap", "Constraction", "Constraction Accessory"}, {}},
        {"Fenêtres", "Fenêtres", "fenetre", {"Window", "Door", "Glass"}, {}},
        {"Nature", "Nature", "nature", {"Plant", "Animal", "Rock"}, {}},
        {"Mécanique", "Mécanique", nullptr, {"Hinge", "Turntable", "Bar", "Lever", "Winch", "Crane", "Grab", "Magnet", "Conveyor", "Screw", "Rack", "Hose", "Tap"}, {"3937", "2429"}},
        {"Véhicules", "Véhicules", nullptr, {"Wheel", "Tyre", "Car", "Vehicle", "Train", "Monorail", "Boat", "Plane", "Wing", "Tail", "Propeller", "Cockpit", "Windscreen", "Exhaust", "Tractor", "Trailer", "Tipper", "Forklift"}, {"4624", "6014"}},
        {"Bâtiment", "Bâtiment", nullptr, {"Bracket", "Panel", "Fence", "Ladder", "Staircase", "Support", "Platform", "Garage", "Container", "Roadsign", "Flag", "Antenna"}, {"3185", "4215b", "87552"}},
        {"Personnages", "Personnages", nullptr, {"Minifig", "Minifig Accessory", "Minifig Footwear", "Minifig Headwear", "Minifig Hipwear", "Minifig Neckwear", "Figure", "Figure Accessory", "Arm"}, {"3626b", "3626c", "973"}},
        {"Autres", "Autres", nullptr, {"Duplo", "Scala", "Belville", "Clikits", "Homemaker", "Freestyle", "Sticker", "Sheet Cardboard", "Sheet Fabric", "Sheet Plastic", "Obsolete", "Pov-RAY"}, {"3437", "6474"}}};
    return t;
}

void BrickApp::ipadDrawer() {
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float H = std::round(std::max(318 * s, d.y * 0.36f));
    const ImVec2 a(0, d.y - H), b(d.x, d.y + 40 * s);
    kitShadowRect(u.dl, a, b, T_OMBRE, -10 * s, 30 * s);
    u.dl->AddRectFilled(a, b, T_TIROIR, 30 * s, ImDrawFlags_RoundCornersTop);
    m_uiRects.push_back(ImVec4(0, a.y + 30 * s, d.x, d.y));   // (the view goes on under the rounded corners)
    m_uiRects.push_back(ImVec4(0, a.y, d.x, a.y + 30 * s));
    // the handle 46 x 5 at 10 pt: a tap closes
    u.dl->AddRectFilled(ImVec2(d.x * 0.5f - 23 * s, a.y + 10 * s), ImVec2(d.x * 0.5f + 23 * s, a.y + 15 * s), T_POIGNEE, 2.5f * s);
    if (u.pressed && u.hover(ImVec2(d.x * 0.5f - 90 * s, a.y), ImVec2(d.x * 0.5f + 90 * s, a.y + 26 * s))) { m_libOpen = false; m_libPress = -1; return; }
    {   // a tab of the kit (the PC's families, e.g. « Basique » from the preferences, are not tabs here)
        bool known = false;
        for (const KitTab& t : kitTabs()) known |= m_fam == t.key;
        if (!known) { m_fam = "Briques"; m_cat.clear(); m_sub.clear(); }
    }
    const float M = 22 * s;
    // ---- tabs at 28 pt, margin 24: 78 wide, icon or thumbnail 46 x 34 + label 13; active: white with a soft shadow
    const float searchW = 220 * s;
    float y = a.y + 28 * s;
    const float tw = 78 * s, th = 72 * s;
    const ImVec2 ta(24 * s, y - 6 * s), tb(d.x - M - searchW - 16 * s, y + th + 6 * s);
    const float tabStart = ta.x - m_famScroll;
    float x = tabStart;
    const KitTab* curTab = nullptr;
    const ImU32 thumbTint = IM_COL32(0xa8, 0x96, 0x80, 255);
    u.dl->PushClipRect(ta, tb, true);
    for (const KitTab& t : kitTabs()) {
        const ImVec2 ca(x, y), cb(x + tw, y + th);
        const bool sel = m_fam == t.key && !m_searchOpen;
        if (m_fam == t.key) curTab = &t;
        if (sel) {
            kitShadowRect(u.dl, ca, cb, T_OMBRE_DOUCE, 2 * s, 10 * s);
            u.dl->AddRectFilled(ca, cb, T_BLANC, 16 * s);
        }
        const ImVec2 ic((ca.x + cb.x) * 0.5f, ca.y + 25 * s);
        if (t.icon) kitIcon(u.dl, t.icon, ic, 26 * s, T_BRUN_DOUX);
        else {
            int part = -1;
            for (const char* nm : t.parts) if (part < 0) part = m_lib.find(nm);
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (part >= 0 && thumbUv(part, tex, uv0, uv1)) {
                const ImVec2 du((uv1.x - uv0.x) * 0.12f, (uv1.y - uv0.y) * 0.2f);
                u.dl->AddImage(tex, ImVec2(ic.x - 23 * s, ic.y - 17 * s), ImVec2(ic.x + 23 * s, ic.y + 17 * s), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), thumbTint);
            }
        }
        const float lw = kSize(13 * s, t.label).x;
        kTextC(u.dl, (ca.x + cb.x) * 0.5f - lw * 0.5f, cb.y - 15 * s, 13 * s, sel ? T_BRUN : T_BRUN_DOUX, t.label);
        if (u.released && u.hover(ca, cb) && u.hover(ta, tb) && !swipeTaken(1) && m_swipeDragged != 1) {
            m_fam = t.key; m_cat.clear(); m_sub.clear(); m_cardsScroll = 0; m_search.clear(); m_searchOpen = false;
        }
        x += tw + 6 * s;
    }
    u.dl->PopClipRect();
    swipeRow(1, ta, tb, m_famScroll, m_famVel, std::max(0.0f, x - tabStart - (tb.x - ta.x)), true);
    // ---- search: a white pill 220 on the right, icon chercher 20
    {
        const ImVec2 sa(d.x - M - searchW, a.y + 43 * s), sb(d.x - M, a.y + 43 * s + 40 * s);
        u.dl->AddRectFilled(sa, sb, T_BLANC, 20 * s);
        kitIcon(u.dl, "chercher", ImVec2(sa.x + 22 * s, (sa.y + sb.y) * 0.5f), 20 * s, IM_COL32(0xa3, 0x90, 0x79, 255));
        ImGui::SetNextWindowPos(ImVec2(sa.x + 40 * s, sa.y + 3 * s));
        ImGui::SetNextWindowSize(ImVec2(sb.x - sa.x - 52 * s, sb.y - sa.y - 6 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2 * s, 8 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(T_BRUN));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(T_BRUN_PALE));
        ImGui::Begin("##ipadsearch", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        ImGui::PushFont(kF(false), 15 * s * kEm);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s", m_search.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##q", "3001, brique 2x4…", buf, sizeof(buf))) { m_search = buf; m_searchOpen = !m_search.empty(); m_cardsScroll = 0; }
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);
        m_uiRects.push_back(ImVec4(sa.x, sa.y, sb.x, sb.y));
    }
    // the counts per category (again when parts become available: the full library loads in the background)
    int avail = 0;
    for (const Part& p : m_lib.parts) avail += p.available() ? 1 : 0;
    if (m_countsFor != m_lib.parts.size() || avail != m_availCount) {
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
    // ---- chips at 116 pt: 14 pt on hollow cream, active brown with white text
    y = a.y + 116 * s;
    const float chipH = 30 * s;
    const ImVec2 qa(M, y), qb(d.x - M, y + chipH);
    const float chipStart = qa.x - m_catScroll;
    x = chipStart;
    u.dl->PushClipRect(qa, qb, true);
    auto chip = [&](const char* label, bool sel) {
        const float w = kSize(14 * s, label).x + 26 * s;
        const ImVec2 ca(x, y), cb(x + w, y + chipH);
        u.dl->AddRectFilled(ca, cb, sel ? T_BRUN : T_CREUX, chipH * 0.5f);
        kTextC(u.dl, ca.x + 13 * s, (ca.y + cb.y) * 0.5f, 14 * s, sel ? T_BLANC : T_BRUN, label);
        x += w + 6 * s;
        return u.released && u.hover(ca, cb) && u.hover(qa, qb) && !swipeTaken(2) && m_swipeDragged != 2;
    };
    if (m_searchOpen) { std::string t = "Recherche « " + m_search + " »"; kTextC(u.dl, x + 4 * s, y + chipH * 0.5f, 14 * s, T_BRUN_DOUX, t.c_str()); }
    else if (!curTab || curTab->cats.size() > 0) {
        if (chip("Tout", m_cat.empty() && m_sub.empty())) { m_cat.clear(); m_sub.clear(); m_cardsScroll = 0; }
        if (curTab) {
            for (const char* cat : curTab->cats) {
                auto it = m_catCount.find(cat);
                if (it == m_catCount.end() || !it->second) continue;
                if (std::string(cat) == "Technic") {
                    for (const char* sn : kTechSubs) {
                        auto jt = m_techSubCount.find(sn);
                        if (jt == m_techSubCount.end() || !jt->second) continue;
                        if (chip(sn, m_cat == "Technic" && m_sub == sn)) { m_cat = "Technic"; m_sub = sn; m_cardsScroll = 0; }
                    }
                    continue;
                }
                if (curTab->cats.size() > 1 && chip(catFr(cat), m_cat == cat)) { m_cat = cat; m_sub.clear(); m_cardsScroll = 0; }
            }
        }
    } else kTextC(u.dl, x + 4 * s, y + chipH * 0.5f, 14 * s, T_BRUN_PALE, m_fam == "recent" ? "Les dernières pièces posées" : "Appui long sur une pièce pour l'ajouter aux favoris");
    u.dl->PopClipRect();
    swipeRow(2, qa, qb, m_catScroll, m_catVel, std::max(0.0f, x - chipStart - (qb.x - qa.x)), true);
    // ---- the parts of the tab (cached: the full library has 22 000 parts)
    {
        const std::string key = m_fam + "|" + m_cat + "|" + m_sub + "|" + (m_searchOpen ? m_search : std::string()) + "|" + std::to_string(avail) + "|" +
                                std::to_string(m_recent.size()) + "|" + std::to_string(m_fav.size());
        if (key != m_drawerKey) {
            m_drawerKey = key;
            m_drawerParts.clear();
            if (m_searchOpen) m_drawerParts = visibleParts();
            else if (m_fam == "recent") m_drawerParts = m_recent;
            else if (m_fam == "fav") m_drawerParts = m_fav;
            else if (curTab) {
                for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
                    const Part& p = m_lib.parts[i];
                    if (!p.available()) continue;
                    const std::string& c = partCat(i);
                    const bool in = m_cat.empty() ? std::find_if(curTab->cats.begin(), curTab->cats.end(), [&](const char* k) { return c == k; }) != curTab->cats.end() : c == m_cat;
                    if (in && (m_sub.empty() || p.sub == m_sub)) m_drawerParts.push_back(i);
                }
            }
        }
    }
    const std::vector<int>& parts = m_drawerParts;
    // ---- the grid at 156 pt: 9 columns of white cards 72 pt, radius 16, gap 10; sliding sideways; the part taken 35 %
    y = a.y + 156 * s;
    const float cgap = 10 * s, cw = std::floor((d.x - 2 * M - 8 * cgap) / 9), ch = 72 * s;
    const int rows = std::max(1, (int)((d.y - y - 6 * s + cgap) / (ch + cgap)));
    const int cols = ((int)parts.size() + rows - 1) / rows;
    const ImVec2 ca(M, y), cb(d.x - M, d.y);
    swipeRow(0, ca, cb, m_cardsScroll, m_cardsVel, std::max(0.0f, cols * (cw + cgap) - cgap - (cb.x - ca.x)), true);
    const ImU32 tint = kitCol(m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)]);
    const double now = ImGui::GetTime();
    const int taken = m_libPress >= 0 ? m_libPress : (m_holding ? m_libSel : -1);
    u.dl->PushClipRect(ca, cb, true);
    if (parts.empty())
        kTextC(u.dl, ca.x + 4 * s, ca.y + 24 * s, 15 * s, T_BRUN_PALE, m_searchOpen ? "Aucune pièce pour cette recherche" : m_fam == "recent" ? "Les pièces que tu poses apparaîtront ici" : m_fam == "fav" ? "Aucun favori pour l'instant" : "Aucune pièce ici");
    for (int k = 0; k < (int)parts.size(); ++k) {
        const int c = k / rows, r = k % rows;
        const ImVec2 pa(ca.x + c * (cw + cgap) - m_cardsScroll, y + r * (ch + cgap)), pb(pa.x + cw, pa.y + ch);
        if (pb.x < ca.x || pa.x > cb.x) continue;
        const int part = parts[k];
        const float al = part == taken ? 0.35f : 1.0f;
        u.dl->AddRectFilled(pa, pb, alphaOf(T_BLANC, al), 16 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(part, tex, uv0, uv1)) {
            const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
            const float side = ch - 8 * s;
            const ImVec2 m((pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f);
            u.dl->AddImage(tex, ImVec2(m.x - side * 0.5f, m.y - side * 0.5f), ImVec2(m.x + side * 0.5f, m.y + side * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), alphaOf(tint, al));
        }
        if (std::find(m_fav.begin(), m_fav.end(), part) != m_fav.end()) kitIcon(u.dl, "favoris", ImVec2(pb.x - 13 * s, pa.y + 13 * s), 13 * s, T_JAUNE);
        if (k == 3) m_mtCard = ImVec2((pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f);
        if (u.hover(pa, pb) && u.hover(ca, cb) && u.pressed) { m_libPress = part; m_libPressPos = u.mouse; m_libPressTime = now; m_libLong = false; }
    }
    u.dl->PopClipRect();
    // a press on a card: long = its sheet, dragged up out of the drawer = in hand under the finger, tap = in hand in the middle
    if (m_libPress >= 0 && (swipeTaken(0) || m_swipeDragged == 0)) m_libPress = -1;
    if (m_libPress >= 0) {
        const ImVec2 dm(u.mouse.x - m_libPressPos.x, u.mouse.y - m_libPressPos.y);
        const bool moved = dm.x * dm.x + dm.y * dm.y > 49 * s * s;
        if (!m_libLong && !moved && now - m_libPressTime > 0.48) { m_libLong = true; m_infoPart = m_libPress; }
        if (moved && ImGui::IsMouseDown(ImGuiMouseButton_Left) && u.mouse.y >= a.y) {   // its thumbnail follows the finger
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (thumbUv(m_libPress, tex, uv0, uv1)) {
                const float g = 96 * s;
                const ImVec2 c(u.mouse.x, u.mouse.y - g * 0.6f);
                const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
                ImGui::GetForegroundDrawList()->AddImage(tex, ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), tint);
            }
        }
        if (moved && u.mouse.y < a.y) {
            const int part = m_libPress;
            m_libPress = -1;
            m_libSel = part;
            spawn(part, u.mouse.x, u.mouse.y);
            m_dragStarted = true;
            m_pressed = false;
        } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!m_libLong && !moved) { m_libSel = m_libPress; spawn(m_libPress, d.x * 0.5f, a.y * 0.55f); }
            m_libPress = -1;
        }
    }
    // the part in hand: « Brique 2 × 4 · s'emboîte ici » under it
    if (m_holding && m_libSel >= 0 && m_libSel < (int)m_lib.parts.size() && m_holdPos.y < a.y - 40 * s)
        kitLabel(ImGui::GetForegroundDrawList(), ImVec2(m_holdPos.x, std::min(m_holdPos.y + 46 * s, a.y - 18 * s)), partFr(m_lib.parts[m_libSel].label) + (m_blockedMaterial >= 0 ? " · ne tient pas ici" : " · s'emboîte ici"));
}

// ------------------------------------------------------------------ 11. the Nuancier (328 pt on the right)

void BrickApp::ipadNuancier() {
    if (!m_paletteOpen) return;
    kitColors();
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const int fb = focusBrick() >= 0 ? focusBrick() : m_selected;
    const int fpart = fb >= 0 && fb < (int)m_bricks.size() ? m_bricks[fb].part : -1;
    const int cur = fb >= 0 && fb < (int)m_bricks.size() ? m_bricks[fb].color : m_color;
    // the colours of the filter, by family (the kit's order and names; the others by hue)
    std::vector<std::pair<std::string, std::vector<int>>> fams;
    auto add = [&](const std::string& fam, int idx) {
        for (auto& f : fams) if (f.first == fam) { if (std::find(f.second.begin(), f.second.end(), idx) == f.second.end()) f.second.push_back(idx); return; }
        fams.push_back({fam, {idx}});
    };
    if (m_paletteFilter == 1) {
        std::set<int> inScene;
        for (const Brick& br : m_bricks) inScene.insert(br.color);
        for (const KitColor& kc : m_kitColorList) if (inScene.count(kc.index)) add(kc.famille, kc.index);
        for (int i : inScene) {
            bool known = false;
            for (const KitColor& kc : m_kitColorList) known |= kc.index == i;
            if (!known && i >= 0 && i < (int)m_lib.palette.size()) add(kFamilyFr[colorFamily(m_lib.palette[i])], i);
        }
    } else
        for (const KitColor& kc : m_kitColorList)
            if (m_paletteFilter == 0 || fpart < 0 || !m_lib.hasColorTable(fpart) || m_lib.colorExists(fpart, kc.index)) add(kc.famille, kc.index);
    const float W = 328 * s, pad = 18 * s, sw = 34 * s, gapS = 8 * s, famH = 26 * s;
    const int perRow = std::max(1, (int)((W - 2 * pad + gapS) / (sw + gapS)));
    // the families laid out in lines; two small families share a line (Transparentes · Métal)
    struct Placed { int fam; int col0; float y; };
    std::vector<Placed> placed;
    float yy = 0;
    for (size_t i = 0; i < fams.size();) {
        const int n0 = (int)fams[i].second.size();
        const int rows0 = (n0 + perRow - 1) / perRow;
        if (i + 1 < fams.size() && n0 + 1 + (int)fams[i + 1].second.size() <= perRow) {
            placed.push_back({(int)i, 0, yy});
            placed.push_back({(int)i + 1, n0 + 1, yy});
            yy += famH + sw + gapS;
            i += 2;
        } else {
            placed.push_back({(int)i, 0, yy});
            yy += famH + rows0 * (sw + gapS);
            ++i;
        }
    }
    const float H = pad + 38 * s + 14 * s + 38 * s + 12 * s + yy + pad - gapS;
    const ImVec2 a(d.x - 58 * s - W, 22 * s), b(a.x + W, std::min(a.y + H, d.y - 22 * s));
    kitSheet(u.dl, a, b);
    float y = a.y + pad;
    kTextC(u.dl, a.x + pad, y + 19 * s, 20 * s, T_BRUN, "Nuancier", true);
    {   // close and the eyedropper (the colour of the selected brick)
        const float D = 38 * s;
        const ImVec2 cx(b.x - pad - D * 0.5f, y + D * 0.5f), cp(cx.x - D - 6 * s, y + D * 0.5f);
        if (kitGalet(u, cx, D, "fermer", 3)) { m_paletteOpen = false; m_arcOpen = false; return; }
        if (kitGalet(u, cp, D, "pipette", 3) && m_selected >= 0) m_color = m_bricks[m_selected].color;
    }
    y += 38 * s + 14 * s;
    {
        g_segFullW = W - 2 * pad;
        const int k = kitSegmented(u, ImVec2(a.x + pad, y), 38 * s, {"Toutes", "Dans la scène", "Pour la pièce"}, m_paletteFilter, false);
        if (k >= 0) m_paletteFilter = k;
    }
    y += 38 * s + 12 * s;
    u.dl->PushClipRect(ImVec2(a.x, y), ImVec2(b.x, b.y - 8 * s), true);
    bool picked = false;
    for (const Placed& pl : placed) {
        const auto& f = fams[pl.fam];
        const float fx = a.x + pad + pl.col0 * (sw + gapS);
        kText(u.dl, ImVec2(fx, y + pl.y), 13 * s, T_BRUN_PALE, upperFr(f.first).c_str());
        const float y0 = y + pl.y + famH;
        for (size_t i = 0; i < f.second.size(); ++i) {
            const int col = (int)(i % perRow), row = (int)(i / perRow);
            const ImVec2 c(fx + col * (sw + gapS) + sw * 0.5f, y0 + row * (sw + gapS) + sw * 0.5f);
            const bool h = u.hover(ImVec2(c.x - sw * 0.5f, c.y - sw * 0.5f), ImVec2(c.x + sw * 0.5f, c.y + sw * 0.5f));
            kitStud(u.dl, c, h ? sw * 1.15f : sw, f.second[i], 1, f.second[i] == cur);
            if (h && u.pressed && !picked) {
                picked = true;
                m_color = f.second[i];
                if (m_selected >= 0) applyColor(f.second[i]);
            }
        }
    }
    u.dl->PopClipRect();
}

// ------------------------------------------------------------------ 11. the Maison menu (380 pt under the Maison pebble)

void BrickApp::ipadHomeMenu() {
    if (!m_homeOpen) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    struct Item { const char* icon; const char* label; const char* note; int act; };
    const std::vector<Item> items = {{"ajouter", "Nouvelle scène", nullptr, 0}, {"ouvrir", "Ouvrir", "Fichiers", 1}, {"enregistrer", "Enregistrer une copie", nullptr, 2},
                                     {"image", "Partager une image", nullptr, 5}, {"notice", m_reading ? "Fermer la notice" : "Lire une notice", ".ldr .mpd", 4},
                                     {"couleur", "Thème Studio", "comme sur PC", 6}};
    struct Seg { const char* label; std::vector<const char*> opts; int cur; int id; };
    const int q = m_renderScale > 0.95f ? 2 : m_renderScale > 0.8f ? 1 : 0;
    const std::vector<Seg> segs = {{"Qualité", {"Économie", "Normale", "Belle"}, q, 0}, {"Lumière", {"Matin", "Midi", "Soir"}, m_lightPreset, 1},
                                   {"Grille", {"Oui", "Non"}, m_build.gridSnap ? 0 : 1, 2}, {"Construire au stylet\nseulement", {"Oui", "Auto"}, m_pencilOnly ? 0 : 1, 3}};
    const float W = 380 * s, pad = 12 * s, segH = 50 * s;
    const float H = pad + items.size() * 50 * s + 17 * s + (segs.size() - 1) * segH + 62 * s + 10 * s;
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));   // a tap outside closes
    const ImVec2 a(22 * s, 84 * s), b(a.x + W, a.y + H);
    kitSheet(u.dl, a, b);
    float y = a.y + pad;
    bool acted = false;
    for (const Item& it : items) {
        if (kitRow(u, ImVec2(a.x + pad, y), W - 2 * pad, it.icon, it.label, it.note)) {
            acted = true;
            switch (it.act) {
            case 0: m_newSceneOpen = true; break;
            case 1: openDialog(); break;
            case 2: saveDialog(); break;
            case 4: if (m_reading) stopReading(); else if (hasSteps()) startReading(1); else openDialog(); break;
            case 5: shareImage(); break;
            case 6: m_ipadCreme = false; m_homeOpen = false; savePrefs(); return;   // back to the Studio interface (Réglages › Thème)
            }
        }
        y += 50 * s;
    }
    u.dl->AddLine(ImVec2(a.x + 26 * s, y + 8 * s), ImVec2(b.x - 18 * s, y + 8 * s), T_SEP, 1.0f * s);
    y += 17 * s;
    for (size_t i = 0; i < segs.size(); ++i) {
        const Seg& sg = segs[i];
        const float h = i + 1 == segs.size() ? 62 * s : segH;
        kTextC(u.dl, a.x + 27 * s, y + h * 0.5f - (i + 1 == segs.size() ? 12 * s : 0), 16 * s, T_BRUN, sg.label);
        const int k = kitSegmented(u, ImVec2(b.x - 24 * s, y + h * 0.5f - 18 * s), 36 * s, sg.opts, sg.cur, true);
        if (k >= 0) {
            acted = true;
            switch (sg.id) {
            case 0: { static const float kS[3] = {0.67f, 0.85f, 1.0f}; m_renderScale = kS[k]; break; }
            case 1: m_lightPreset = k; applyLook(); break;
            case 2: m_build.gridSnap = k == 0; break;
            case 3: m_pencilOnly = k == 0; break;
            }
            savePrefs();
        }
        y += h;
    }
    if (acted && (m_newSceneOpen || ImGui::GetIO().WantTextInput)) m_homeOpen = false;
    const ImVec2 hc(22 * s + 26 * s, 20 * s + 26 * s);
    if (u.pressed && !acted && !u.hover(a, b) && !u.hover(ImVec2(hc.x - 26 * s, hc.y - 26 * s), ImVec2(hc.x + 26 * s, hc.y + 26 * s))) m_homeOpen = false;
}

void BrickApp::shareImage() {
    std::vector<uint8_t> img;
    uint32_t w = 0, h = 0;
    if (!m_renderer->readViewport(img, w, h) || !w || !h) { toast("Image indisponible"); return; }
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t name[64];
    swprintf(name, 64, L"BrickIt %04d-%02d-%02d %02dh%02d.png", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    plat::saveFileDialog(L"Image PNG (*.png)\0*.png\0", L"png", name, [this, img, w, h](const std::wstring& p) {
        if (!p.empty() && savePng(p, img, w, h)) toast("Image enregistrée");
    });
}

// ------------------------------------------------------------------ 12. the message: dark pill 118 pt above the bottom, 2.5 s

void BrickApp::ipadMessage() {
    const double t = ImGui::GetTime() - m_toastTime;
    if (m_toast.empty() || t > 2.5) return;
    g_kf = m_gui->fontKit;
    g_kfm = m_gui->fontKitMed ? m_gui->fontKitMed : m_gui->fontKit;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float al = (float)std::clamp(std::min(t / 0.16, (2.5 - t) / 0.3), 0.0, 1.0);
    const float px = 15 * s, tw = kSize(px, m_toast.c_str()).x;
    const float W = 16 * s + 18 * s + 10 * s + tw + 20 * s, H = 42 * s;
    const ImVec2 a((d.x - W) * 0.5f, d.y - 118 * s - H * 0.5f), b(a.x + W, a.y + H);
    dl->AddRectFilled(a, b, alphaOf(T_MESSAGE, al), H * 0.5f);
    kitIcon(dl, "coche", ImVec2(a.x + 16 * s + 9 * s, (a.y + b.y) * 0.5f), 18 * s, alphaOf(T_BLANC, al));
    kTextC(dl, a.x + 16 * s + 18 * s + 10 * s, (a.y + b.y) * 0.5f, px, alphaOf(T_BLANC, al), m_toast.c_str());
}

// ------------------------------------------------------------------ 11. Nouvelle scène (a centred sheet 880 pt over a veil)

void BrickApp::ipadNewScene() {
    if (!m_newSceneOpen) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    u.dl->AddRectFilled(ImVec2(0, 0), d, T_VOILE);
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const float W = std::min(880 * s, d.x - 60 * s), pad = 28 * s;
    const float cardW = (W - 2 * pad - 2 * 18 * s) / 3, imgH = cardW * 0.57f, cardH = imgH + 12 * s + 80 * s;
    const float thW = (W - 2 * pad - 3 * 16 * s) / 4, thH = thW * 0.5f;
    const float H = pad + 44 * s + 16 * s + cardH + 30 * s + 22 * s + thH + 34 * s + pad - 8 * s;
    const ImVec2 a((d.x - W) * 0.5f, std::max(20 * s, (d.y - H) * 0.42f)), b(a.x + W, a.y + H);
    kitSheet(u.dl, a, b);
    kTextC(u.dl, a.x + pad, a.y + pad + 22 * s, 28 * s, T_BRUN, "Nouvelle scène", true);
    if (kitGalet(u, ImVec2(b.x - pad - 19 * s, a.y + pad + 22 * s), 38 * s, "fermer", 3)) { m_newSceneOpen = false; return; }
    struct Card { const char* title; const char* sub; int act; };
    const Card cards[3] = {{"Plaque verte", "32 × 32 tenons, lumière du matin", 0}, {"Sol libre", "Rien sur la scène", 1}, {"Depuis une notice", "Fichier .mpd officiel, étape par étape", 2}};
    float y = a.y + pad + 44 * s + 16 * s, x = a.x + pad;
    for (const Card& c : cards) {
        const ImVec2 ca(x, y), cb(x + cardW, y + cardH);
        const bool h = u.hover(ca, cb);
        kitShadowRect(u.dl, ca, cb, T_OMBRE_DOUCE, 3 * s, 12 * s);
        u.dl->AddRectFilled(ca, cb, h && ImGui::IsMouseDown(ImGuiMouseButton_Left) ? T_CREUX : T_BLANC, 18 * s);
        const ImVec2 ia(ca.x + 12 * s, ca.y + 12 * s), ib(cb.x - 12 * s, ca.y + 12 * s + imgH);
        if (c.act == 0) {   // a green plate on the cream floor
            u.dl->AddRectFilled(ia, ib, IM_COL32(0xf1, 0xe4, 0xd1, 255), 12 * s);
            const ImVec2 m((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f + 8 * s);
            const float k = (ib.x - ia.x) * 0.34f;
            const ImVec2 qd[4] = {ImVec2(m.x - k, m.y), ImVec2(m.x, m.y - k * 0.45f), ImVec2(m.x + k, m.y), ImVec2(m.x, m.y + k * 0.45f)};
            u.dl->AddQuadFilled(ImVec2(qd[0].x, qd[0].y + 5 * s), ImVec2(qd[1].x, qd[1].y + 5 * s), ImVec2(qd[2].x, qd[2].y + 5 * s), ImVec2(qd[3].x, qd[3].y + 5 * s), IM_COL32(0x2f, 0x7a, 0x2f, 255));
            u.dl->AddQuadFilled(qd[0], qd[1], qd[2], qd[3], IM_COL32(0x4b, 0x9f, 0x4a, 255));
            for (int i = 1; i < 8; ++i)
                for (int j = 1; j < 8; ++j) {
                    const float fu = i / 8.0f, fv = j / 8.0f;
                    const ImVec2 p(qd[0].x + (qd[1].x - qd[0].x) * fu + (qd[3].x - qd[0].x) * fv, qd[0].y + (qd[1].y - qd[0].y) * fu + (qd[3].y - qd[0].y) * fv);
                    u.dl->AddCircleFilled(p, 2.2f * s, IM_COL32(0x6c, 0xc4, 0x60, 255), 10);
                }
        } else if (c.act == 1)
            u.dl->AddRectFilled(ia, ib, IM_COL32(0xec, 0xe2, 0xd2, 255), 12 * s);
        else {
            u.dl->AddRectFilled(ia, ib, IM_COL32(0x1e, 0x1b, 0x18, 255), 12 * s);
            kitIcon(u.dl, "notice", ImVec2((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f), 48 * s, IM_COL32(0xef, 0xe5, 0xd4, 255));
        }
        kText(u.dl, ImVec2(ca.x + 12 * s, ib.y + 14 * s), 18 * s, T_BRUN, c.title, true);
        u.dl->PushClipRect(ImVec2(ca.x, ib.y), ImVec2(cb.x - 8 * s, cb.y), true);
        kText(u.dl, ImVec2(ca.x + 12 * s, ib.y + 46 * s), 14 * s, T_BRUN_PALE, c.sub);
        u.dl->PopClipRect();
        if (h && u.pressed) {
            m_newSceneOpen = false;
            if (c.act == 2) openDialog();
            else newScene(c.act);
            return;
        }
        x += cardW + 18 * s;
    }
    // Reprendre: the scene in progress, the official house
    y += cardH + 30 * s;
    kText(u.dl, ImVec2(a.x + pad, y), 13 * s, T_BRUN_PALE, "REPRENDRE");
    y += 22 * s;
    x = a.x + pad;
    for (int k = 0; k < 2; ++k) {
        const ImVec2 ra(x, y), rb(x + thW, y + thH);
        const bool h = u.hover(ra, rb);
        if (k == 0) {
            const float sw = m_stage.z - m_stage.x, sh = m_stage.w - m_stage.y;
            const float aspect = (rb.x - ra.x) / (rb.y - ra.y), view = sw / std::max(1.0f, sh);
            const float cu = aspect < view ? 0.5f * (1 - aspect / view) : 0, cv = aspect < view ? 0 : 0.5f * (1 - view / aspect);
            u.dl->AddImageRounded(m_gui->viewportTexture(), ra, rb, ImVec2(cu, cv), ImVec2(1 - cu, 1 - cv), IM_COL32_WHITE, 12 * s);
        } else {
            u.dl->AddRectFilled(ra, rb, IM_COL32(0x1e, 0x1b, 0x18, 255), 12 * s);
            kitIcon(u.dl, "maison", ImVec2((ra.x + rb.x) * 0.5f, (ra.y + rb.y) * 0.5f), 34 * s, IM_COL32(0xef, 0xe5, 0xd4, 255));
        }
        if (h && ImGui::IsMouseDown(ImGuiMouseButton_Left)) u.dl->AddRect(ra, rb, T_BRUN, 12 * s, 0, 2 * s);
        const std::string t = k == 0 ? sceneTitle(m_scenePath) : "Modern House 31153";
        kText(u.dl, ImVec2(ra.x, rb.y + 8 * s), 14 * s, T_BRUN, t.c_str());
        if (h && u.pressed) { m_newSceneOpen = false; if (k == 1) openOfficialHouse(); return; }
        x += thW + 16 * s;
    }
}

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

