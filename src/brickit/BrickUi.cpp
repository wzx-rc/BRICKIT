// BrickIt - the interface of the web builder v26 (docs/brickit/web/BrickIt_Builder.html), element by element: the CSS
// colours (--bg, --panel, --card, --line, --accent ...), the compact sizes (top bar 24 px buttons, tray, 84 px cards),
// the SVG icons (Svg.cpp), the French texts. Everything is drawn with ImDrawList in one window behind the ImGui popups;
// clicks are hit-tested here. m_ui scales the whole interface (4K: 2).
#include "BrickApp.h"
#include "CrashLog.h"
#include "Platform.h"
#include "Svg.h"
#include "app/Theme.h"
#include "app/Json.h"
#include <array>
#include <unordered_set>
#include "Image.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <cctype>
#include <cmath>
#include <cstring>

namespace brickit {

static void uiTheme(bool warm);   // BrickUiIpad.inl
// ------------------------------------------------------------------ web CSS colours
static ImU32 C_BG = IM_COL32(0x13, 0x14, 0x17, 255);
static ImU32 C_PANEL = IM_COL32(24, 25, 29, 235);           // rgba(24, 25, 29, 0.92)
static ImU32 C_PANEL_SOLID = IM_COL32(0x1b, 0x1c, 0x20, 255);
static ImU32 C_CARD = IM_COL32(0x25, 0x26, 0x2b, 255);
static ImU32 C_CARD_HI = IM_COL32(0x30, 0x32, 0x3a, 255);
static ImU32 C_LINE = IM_COL32(0x2f, 0x31, 0x37, 255);
static ImU32 C_TEXT = IM_COL32(0xec, 0xec, 0xee, 255);
static ImU32 C_DIM = IM_COL32(0x9a, 0x9c, 0xa4, 255);
static ImU32 C_FAINT = IM_COL32(0x67, 0x6a, 0x72, 255);
static ImU32 C_ACCENT = IM_COL32(0xf7, 0xc5, 0x1e, 255);
static ImU32 C_ACCENT_TXT = IM_COL32(0xf7, 0xc5, 0x1e, 255);
static ImU32 C_THUMB = IM_COL32_WHITE;   // tint of the library thumbnails (anthracite #474B52 on the light theme, as the mock-up)
static ImU32 C_PROGRESS = IM_COL32(0xf7, 0xc5, 0x1e, 255);  // sliders (the accent; Sommet: its blue while the pill marks what is active)
static ImU32 C_INK = IM_COL32(0x17, 0x17, 0x1a, 255);
static ImU32 C_LYEL = IM_COL32(0xf2, 0xcd, 0x37, 255);
static ImU32 C_LBLU = IM_COL32(0x00, 0x55, 0xbf, 255);
static ImU32 C_OVERLAY = IM_COL32(15, 16, 19, 219);        // rgba(15, 16, 19, 0.86)
static ImU32 C_HUD = IM_COL32(15, 16, 19, 184);            // rgba(15, 16, 19, 0.72)
static ImU32 C_DEV = IM_COL32(0x7f, 0xe3, 0xff, 255);

static ImU32 alphaOf(ImU32 c, float a) { return (c & 0x00FFFFFF) | ((ImU32)(((c >> 24) & 0xFF) * a) << 24); }

// the theme (BrickTheme.h, BrickApp::applyTheme): its fonts, the scale of its text, the shape of its buttons
static ImFont* g_thFont = nullptr;    // regular (nullptr: Segoe UI)
static ImFont* g_thBold = nullptr;    // semibold
static ImFont* g_thHead = nullptr;    // titles of the panels (Minimal: Manrope Light)
static ImFont* g_fInstr[2] = {}, *g_fGeist[2] = {}, *g_fManrope = nullptr;
static float g_fontK = 1.22f;         // Segoe UI in ImGui reads smaller than in the browser at the same px size
static float g_rk = 1.0f;             // radii of the panels and buttons
static bool g_pill = false, g_lines = false;
static int g_hudStyle = 0;            // theme::HUD_LEGO / HUD_FLAT / HUD_PILL
static ImTextureID g_studTex = 0;     // Glass, Red: the swatches as a 1x1 brick seen from above (white, tinted by the colour)
static float rr(float r) { return r * g_rk; }
// the radius of a button of height h: the theme's, or a pill
static float rb(float r, float h) { return g_pill ? h * 0.5f : r * g_rk; }

// LDraw families of the web (line 1 of the library) and their categories (line 2, empty ones dashed)
struct Fam { const char* name; std::vector<const char*> cats; };
static const std::vector<Fam>& families() {
    static const std::vector<Fam> f = {
        {"Basique", {"Brick", "Plate", "Tile", "Baseplate", "Bracket", "Panel"}},
        {"Formes", {"Slope", "Wedge", "Arch", "Cylinder", "Cone", "Dish", "Sphere"}},
        {"Technic", {"Technic", "Electric", "Znap", "Constraction", "Constraction Accessory"}},
        {"Mécanique", {"Hinge", "Turntable", "Bar", "Lever", "Winch", "Crane", "Grab", "Magnet", "Conveyor", "Screw", "Rack", "Hose", "Tap"}},
        {"Véhicules", {"Wheel", "Tyre", "Car", "Vehicle", "Train", "Monorail", "Boat", "Plane", "Wing", "Tail", "Propeller", "Cockpit", "Windscreen", "Exhaust", "Tractor", "Trailer", "Tipper", "Forklift"}},
        {"Bâtiment", {"Door", "Window", "Glass", "Fence", "Ladder", "Staircase", "Support", "Platform", "Garage", "Container", "Roadsign", "Flag", "Antenna"}},
        {"Personnages", {"Minifig", "Minifig Accessory", "Minifig Footwear", "Minifig Headwear", "Minifig Hipwear", "Minifig Neckwear", "Figure", "Figure Accessory", "Arm"}},
        {"Nature", {"Animal", "Plant", "Rock"}},
        {"Autres gammes", {"Duplo", "Scala", "Belville", "Clikits", "Homemaker", "Freestyle", "Sticker", "Sheet Cardboard", "Sheet Fabric", "Sheet Plastic", "Obsolete", "Pov-RAY"}},
    };
    return f;
}
static const char* catFr(const std::string& c) {
    static const std::pair<const char*, const char*> t[] = {
        {"Brick", "Briques"}, {"Plate", "Plaques"}, {"Tile", "Tuiles"}, {"Baseplate", "Plaques de base"}, {"Bracket", "Équerres"},
        {"Panel", "Panneaux"}, {"Slope", "Pentes"}, {"Wedge", "Biseaux"}, {"Arch", "Arches"}, {"Cylinder", "Cylindres"}, {"Cone", "Cônes"},
        {"Dish", "Paraboles"}, {"Sphere", "Sphères"}, {"Electric", "Électrique"}, {"Hinge", "Charnières"}, {"Turntable", "Plateaux tournants"},
        {"Bar", "Barres"}, {"Lever", "Leviers"}, {"Winch", "Treuils"}, {"Crane", "Grues"}, {"Grab", "Pinces"}, {"Magnet", "Aimants"},
        {"Conveyor", "Convoyeurs"}, {"Screw", "Vis"}, {"Rack", "Crémaillères"}, {"Hose", "Tuyaux"}, {"Tap", "Robinets"}, {"Wheel", "Roues"},
        {"Tyre", "Pneus"}, {"Car", "Voiture"}, {"Vehicle", "Véhicule"}, {"Train", "Train"}, {"Boat", "Bateau"}, {"Plane", "Avion"},
        {"Wing", "Ailes"}, {"Tail", "Dérives"}, {"Propeller", "Hélices"}, {"Cockpit", "Cockpits"}, {"Windscreen", "Pare-brise"},
        {"Door", "Portes"}, {"Window", "Fenêtres"}, {"Glass", "Vitres"}, {"Fence", "Clôtures"}, {"Ladder", "Échelles"},
        {"Staircase", "Escaliers"}, {"Support", "Supports"}, {"Platform", "Plateformes"}, {"Container", "Conteneurs"},
        {"Roadsign", "Panneaux routiers"}, {"Flag", "Drapeaux"}, {"Antenna", "Antennes"}, {"Minifig", "Minifigs"}, {"Animal", "Animaux"},
        {"Plant", "Plantes"}, {"Rock", "Rochers"}};
    for (auto& e : t)
        if (c == e.first) return e.second;
    return c.c_str();
}
static const char* kTechSubs[] = {"Poutres", "Axes", "Pins", "Connecteurs", "Engrenages", "Briques", "Panneaux", "Direction", "Pneumatique",
                                  "Chenilles", "Moteurs", "Rotation", "Flex", "Bionicle", "Divers"};

const std::string& BrickApp::partCat(int part) const {
    static const std::string other = "Autre";
    const Part& p = m_lib.parts[part];
    return p.category.empty() ? other : p.category;
}

void BrickApp::buildNav() {
    // Technic parts of the base set get their sub-family from the second word of their title, as the web (techSub)
    static const std::pair<const char*, std::vector<const char*>> subs[] = {
        {"Poutres", {"Beam", "Liftarm"}}, {"Axes", {"Axle"}}, {"Pins", {"Pin"}}, {"Connecteurs", {"Connector", "Cross", "Angle", "Bush", "Link"}},
        {"Engrenages", {"Gear", "Worm", "Differential", "Gearbox", "Sprocket", "Clutch", "Changeover", "Transmission", "Cam"}},
        {"Briques", {"Brick", "Plate", "Tile"}}, {"Panneaux", {"Panel", "Fairing", "Dome", "Wedge", "Bumper", "Grille", "Windscreen", "Wing"}},
        {"Direction", {"Steering", "Suspension", "Shock", "Universal", "Ball"}}, {"Pneumatique", {"Pneumatic"}},
        {"Chenilles", {"Chain", "Tread", "Belt", "Rubber", "Track", "Wheel", "Pulley"}}, {"Moteurs", {"Engine", "Linear", "Piston", "Cylinder"}},
        {"Rotation", {"Turntable", "Rotor", "Rotation"}}, {"Flex", {"Flex-System", "Cable"}}, {"Bionicle", {"Bionicle", "Disc", "Sword"}}};
    for (Part& p : m_lib.parts) {
        if (p.category != "Technic" || !p.sub.empty()) continue;
        std::string w;
        size_t a = p.label.find(' ');
        if (a != std::string::npos) { size_t b = p.label.find(' ', a + 1); w = p.label.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1); }
        p.sub = "Divers";
        for (auto& sbs : subs)
            for (const char* k : sbs.second)
                if (w == k) p.sub = sbs.first;
    }
    // --cat N (scripted tests): 0 recents, 1 favourites, then the families
    m_nav.clear();
    m_nav.push_back({"Récentes", 0, "recent"});
    m_nav.push_back({"Favoris", 1, "fav"});
    for (const Fam& f : families()) m_nav.push_back({f.name, 2, f.name});
    for (const char* s : kTechSubs) m_nav.push_back({s, 3, s, 2});
}

// a decorated part: a sticker or a printed pattern (« ... Pattern », « ... Sticker » in its LDraw title)
ImU32 stConnCol(int src);          // (BrickUiStudio.inl: mode DEV, the source of the connectors)
const char* stConnName(int src);
static bool partDeco(const Part& p) {
    std::string t = p.label;
    for (char& c : t) c = (char)std::tolower((unsigned char)c);
    return t.find("pattern") != std::string::npos || t.find("sticker") != std::string::npos;
}
// a Duplo part (and the Hubelino ones, Duplo compatible): never in the library (Wzx 09/11/2026)
static bool partDuplo(const Part& p) {
    std::string t = p.label;
    for (char& c : t) c = (char)std::tolower((unsigned char)c);
    return p.category == "Duplo" || t.find("duplo") != std::string::npos || t.find("hubelino") != std::string::npos;
}

std::vector<int> BrickApp::visibleParts() const {
    std::vector<int> out;
    auto lower = [](std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; };
    if (m_searchOpen && !m_search.empty()) {
        const std::string q = lower(m_search);
        for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
            const Part& p = m_lib.parts[i];
            if (!p.available()) continue;
            if ((m_hideDeco && partDeco(p)) || partDuplo(p)) continue;
            const std::string& c = partCat(i);
            if (lower(p.name).find(q) != std::string::npos || lower(p.label).find(q) != std::string::npos || lower(catFr(c)).find(q) != std::string::npos ||
                lower(c).find(q) != std::string::npos)
                out.push_back(i);
        }
        return out;
    }
    if (m_fam == "fav") return m_fav;
    if (m_fam == "recent") return m_recent;
    const Fam* fam = nullptr;
    for (const Fam& f : families()) if (m_fam == f.name) fam = &f;
    for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
        const Part& p = m_lib.parts[i];
        if (!p.available()) continue;
        if ((m_hideDeco && partDeco(p)) || partDuplo(p)) continue;
        if (m_fam == "all") { out.push_back(i); continue; }
        if (!fam) continue;
        const std::string& c = partCat(i);
        bool in = m_cat.empty() ? std::find_if(fam->cats.begin(), fam->cats.end(), [&](const char* x) { return c == x; }) != fam->cats.end() : c == m_cat;
        if (in && (m_sub.empty() || p.sub == m_sub)) out.push_back(i);
    }
    return out;
}

// ------------------------------------------------------------------ drawing helpers

struct Ui {
    ImDrawList* dl;
    float s;
    ImVec2 mouse;
    bool pressed, released;
    ImFont* font;
    ImFont* bold;
    float fs(float px) const { return px * s * g_fontK; }   // (g_fontK: 1.22 for Segoe UI, the theme's font otherwise)
    bool hover(ImVec2 a, ImVec2 b) const { return mouse.x >= a.x && mouse.y >= a.y && mouse.x < b.x && mouse.y < b.y; }
    ImVec2 text(ImVec2 p, float px, ImU32 col, const char* t, bool b = false) {
        ImFont* f = b ? bold : font;
        dl->AddText(f, fs(px), p, col, t);
        return f->CalcTextSizeA(fs(px), 1e9f, 0, t);
    }
    float textW(float px, const char* t, bool b = false) const { return (b ? bold : font)->CalcTextSizeA(fs(px), 1e9f, 0, t).x; }
    void grip(ImVec2 p, float w, const char* label, bool collapsed, bool chev = true) {   // dots + LABEL + chevron
        for (int r = 0; r < 2; ++r)
            for (int c = 0; c < 6; ++c) dl->AddCircleFilled(ImVec2(p.x + (3 + c * 6) * s, p.y + (3 + r * 4) * s), 1.1f * s, C_FAINT, 6);
        if (label) text(ImVec2(p.x + 42 * s, p.y - 1 * s), 10, C_FAINT, label);
        if (chev) {
            ImVec2 c(p.x + w - 6 * s, p.y + 5 * s);
            float k = 3.5f * s;
            if (!collapsed) dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - k, c.y - k * 0.5f), c, ImVec2(c.x + k, c.y - k * 0.5f)}.data(), 3, C_FAINT, 0, 2 * s);
            else dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - k, c.y + k * 0.5f), ImVec2(c.x, c.y - k * 0.5f), ImVec2(c.x + k, c.y + k * 0.5f)}.data(), 3, C_FAINT, 0, 2 * s);
        }
    }
};

static Ui ui(float s) {
    Ui u;
    u.dl = ImGui::GetWindowDrawList();
    u.s = s;
    u.mouse = ImGui::GetIO().MousePos;
    u.pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    u.released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    u.font = g_thFont ? g_thFont : ImGui::GetFont();
    u.bold = g_thBold ? g_thBold : u.font;
    return u;
}

// ------------------------------------------------------------------ top bar

void BrickApp::uiHeader() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    const float H = 30 * s;
    u.dl->AddRectFilled(ImVec2(0, 0), ImVec2(d.x, H), C_PANEL);
    u.dl->AddLine(ImVec2(0, H - 0.5f), ImVec2(d.x, H - 0.5f), C_LINE, 1.0f);
    m_uiRects.push_back(ImVec4(0, 0, d.x, H));
    // logo: the app icon (A / B, Réglages) + BrickIt Builder (the old yellow « 3D » square when the image is missing)
    ImVec2 l(10 * s, 5 * s);
    if (ImTextureID lt = kitTex(m_appIcon == 1 ? "logo/brickit_b.png" : "logo/brickit_a.png"))
        u.dl->AddImageRounded(lt, l, ImVec2(l.x + 20 * s, l.y + 20 * s), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 4.5f * s);
    else {
        u.dl->AddRectFilled(l, ImVec2(l.x + 20 * s, l.y + 20 * s), C_ACCENT, 5 * s);
        float tw = u.textW(9, "3D", true);
        u.text(ImVec2(l.x + 10 * s - tw * 0.5f, l.y + 4.5f * s), 9, C_INK, "3D", true);
    }
    u.text(ImVec2(l.x + 26 * s, l.y + 3 * s), 12, C_TEXT, "BrickIt Builder", true);

    // buttons, right aligned, in the order of the web
    struct B { int kind; svg::Icon icon; const char* label; const char* tip; bool on, enabled; int id; };   // kind 0 icon, 1 select, 2 icon+text, 3 text, 4 sep
    const bool sel = m_selected >= 0 || !m_multi.empty();
    const char* tmNames[4] = {"ACES", "AgX", "AgX", "Std"};
    std::vector<B> bs = {
        {0, svg::I_UNDO, nullptr, "Annuler (Ctrl+Z)", false, !m_undo.empty() || m_holding, 1},
        {0, svg::I_REDO, nullptr, "Rétablir", false, !m_redo.empty(), 2},
        {4, svg::I_UNDO, nullptr, nullptr, false, true, 0},
        {0, svg::I_SELECT, nullptr, "Mode Sélection : touche une brique, ou trace un rectangle pour en sélectionner plusieurs", m_mode == 0, true, 3},
        {0, svg::I_VISOR, nullptr, "Mode Viseur : toucher une brique la prend en main", m_mode == 1, true, 4},
        {0, svg::I_MOVE, nullptr, "Mode Déplacer : flèches sur la brique sélectionnée", m_mode == 2, true, 5},
        {0, svg::I_ROTATE, nullptr, "Mode Tourner : anneaux sur la brique sélectionnée", m_mode == 3, true, 6},
        {4, svg::I_UNDO, nullptr, nullptr, false, true, 0},
        {0, svg::I_GRID, nullptr, "Grille ON : snap sur la grille du sol · OFF : pas de sol, snap sur le voxel le plus proche", m_build.gridSnap, true, 7},
        {0, svg::I_PIVOT, nullptr, "Auto Pivot : le point touché devient le pivot", m_autoPivot, true, 8},
        {0, svg::I_SAFETY, nullptr, "Sécurité : refuse de poser si plus de 50 % de la brique est dans une autre", m_build.safety, true, 9},
        {0, svg::I_FOCUS, nullptr, "Mode focus : la brique sélectionnée reste claire, les autres passent à 50 % (F)", m_focusDim, true, 10},
        {0, svg::I_ANIM, nullptr, "Animations des briques (A)", m_animOn, true, 11},
        {4, svg::I_UNDO, nullptr, nullptr, false, true, 0},
        {0, svg::I_VOXELS, nullptr, "Voir les voxels : tenons jaunes, pins cyan, axles magenta (V)", m_showVox, true, 12},
        {0, svg::I_DELETE, nullptr, "Supprimer la sélection", false, sel && !m_holding, 13},
        {1, svg::I_UNDO, m_rcMode ? "Radiance C." : !m_rs.raster ? "Path tracing" : m_quality ? "HQ" : "Rapide", "Qualité", false, true, 14},
        {1, svg::I_UNDO, tmNames[std::clamp(m_rs.tonemapper, 0, 3)], "Transformation de vue", false, true, 15},
        {4, svg::I_UNDO, nullptr, nullptr, false, true, 0},
        {2, svg::I_MECH, "Mécanique", "Mode Mécanique : animer pivots, engrenages et axes (cinématique)", m_mechOn, true, 16},
        {3, svg::I_UNDO, "DEV", "Mode développeur : connecteurs LDraw, export / réimport Maya des pièces", m_dev, true, 17},
        {0, svg::I_WINDOWS, nullptr, "Fenêtres : afficher / masquer les panneaux", m_popup == 3, true, 18},
        {0, svg::I_FILE, nullptr, "Scène : ouvrir / enregistrer (.v3d.json)", m_popup == 4, true, 19},
        {3, svg::I_UNDO, "Bac", "Bac à briques temporaire : garde des pièces sous la main (glisse-les depuis la bibliothèque)", m_bacOpen, true, 21},
        {0, svg::I_SETTINGS, nullptr, "Réglages : échelle de l'interface, miniatures, qualité, transformation de vue…", m_popup == 5, true, 20},
    };
    if (m_ipad) {   // the iPad: no render modes, no view transform, no DEV mode (Maya), no tray of bricks
        bs.erase(std::remove_if(bs.begin(), bs.end(), [](const B& b) { return b.id == 14 || b.id == 15 || b.id == 17 || b.id == 21; }), bs.end());
        for (size_t i = 1; i < bs.size(); ++i)   // (no two separators in a row)
            if (bs[i].kind == 4 && bs[i - 1].kind == 4) bs.erase(bs.begin() + i--);
    }
    auto width = [&](const B& b) {
        switch (b.kind) {
        case 0: return 26 * s;
        case 1: return u.textW(11, b.label) + 24 * s;
        case 2: return u.textW(11, b.label) + 32 * s;
        case 3: return u.textW(12, b.label, true) + 12 * s;
        default: return 5 * s;
        }
    };
    float total = 0;
    for (const B& b : bs) total += width(b) + 4 * s;
    float x = d.x - 10 * s - total + 4 * s;
    const float y0 = 3 * s, y1 = 27 * s;
    for (const B& b : bs) {
        const float w = width(b);
        ImVec2 a(x, y0), c(x + w, y1);
        x += w + 4 * s;
        if (b.kind == 4) { u.dl->AddLine(ImVec2(a.x + 2.5f * s, 8 * s), ImVec2(a.x + 2.5f * s, 22 * s), C_LINE, 1.0f); continue; }
        const bool h = b.enabled && u.hover(a, c) && m_popup == 0;
        ImU32 bg = b.on ? (b.id == 17 ? C_DEV : C_ACCENT) : (h ? C_CARD_HI : C_CARD);
        ImU32 fg = b.on ? (b.id == 17 ? IM_COL32(0x0d, 0x1a, 0x1f, 255) : C_INK) : C_TEXT;
        if (!b.enabled) { bg = alphaOf(bg, 0.4f); fg = alphaOf(fg, 0.4f); }
        u.dl->AddRectFilled(a, c, bg, rb(6 * s, c.y - a.y));
        const float cy = (y0 + y1) * 0.5f;
        if (b.kind == 0) drawIcon(u.dl, b.icon, ImVec2((a.x + c.x) * 0.5f, cy), 14 * s, fg);
        else if (b.kind == 1) {
            u.text(ImVec2(a.x + 6 * s, cy - 7 * s), 11, fg, b.label);
            ImVec2 k(c.x - 10 * s, cy);
            u.dl->AddTriangleFilled(ImVec2(k.x - 3 * s, k.y - 1.5f * s), ImVec2(k.x + 3 * s, k.y - 1.5f * s), ImVec2(k.x, k.y + 2 * s), C_DIM);
        } else if (b.kind == 2) {
            drawIcon(u.dl, b.icon, ImVec2(a.x + 13 * s, cy), 14 * s, fg);
            u.text(ImVec2(a.x + 24 * s, cy - 7 * s), 11, fg, b.label, b.on);
        } else u.text(ImVec2(a.x + 6 * s, cy - 8 * s), 12, fg, b.label, true);
        if (h && b.tip && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", b.tip);
        if (!(h && u.pressed)) continue;
        switch (b.id) {
        case 1: undo(); break;
        case 2: redo(); break;
        case 3: case 4: case 5: case 6:
            m_mode = b.id - 3;
            if (m_mode >= 2) toast(m_mode == 2 ? "Mode Déplacer : les flèches arrivent bientôt dans la version native" : "Mode Tourner : les anneaux arrivent bientôt dans la version native");
            break;
        case 7: m_build.gridSnap = !m_build.gridSnap; toast(m_build.gridSnap ? "Grille ON : snap sur la grille du sol" : "Grille OFF : snap sur le voxel le plus proche"); break;
        case 8: m_autoPivot = !m_autoPivot; toast(m_autoPivot ? "Auto Pivot : le point touché devient le pivot" : "Auto Pivot désactivé"); break;
        case 9: m_build.safety = !m_build.safety; toast(m_build.safety ? "Sécurité ON : refus au-delà de 50 % de recouvrement" : "Sécurité OFF"); break;
        case 10: m_focusDim = !m_focusDim; syncObjects(); break;
        case 11: m_animOn = !m_animOn; break;
        case 12: m_showVox = !m_showVox; syncObjects(); toast(m_showVox ? "Voxels : affichés (tenons jaunes, pins cyan, axles magenta)" : "Voxels : masqués"); break;
        case 13: deleteSelected(); break;
        case 14: m_popup = m_popup == 1 ? 0 : 1; m_popupAt = ImVec2(a.x, y1 + 4 * s); break;
        case 15: m_popup = m_popup == 2 ? 0 : 2; m_popupAt = ImVec2(a.x, y1 + 4 * s); break;
        case 16: if (m_mechOn) mechExit(); else mechEnter(); break;
        case 17: devSetOn(!m_dev); break;
        case 18: m_popup = m_popup == 3 ? 0 : 3; m_popupAt = ImVec2(d.x - 220 * s, y1 + 4 * s); break;
        case 19:
            if (m_ipad) { m_popup = m_popup == 4 ? 0 : 4; m_popupAt = ImVec2(a.x, y1 + 4 * s); break; }   // iPad: the simple scene menu
            m_popup = 0; m_sheetOpen = true; m_sheetText = m_build.toJson(m_scene.camera); break;
        case 21: m_bacOpen = !m_bacOpen; m_bacDirty = true; break;
        case 20: m_popup = m_popup == 5 ? 0 : 5; m_popupAt = ImVec2(d.x - 330 * s, y1 + 4 * s); break;
        }
    }
}

// ------------------------------------------------------------------ left-hand radial menu

void BrickApp::uiRadial() {
    if (!m_pOpts.visible || m_mechOn) return;
    if (m_ipad) { uiToolDock(); return; }
    if (!m_touchHud || !m_ipad) return;   // (the touch HUD: the iPad only)
    Ui u = ui(m_ui);
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    const float trayTop = m_uiRects.size() > 1 ? m_uiRects[1].y : d.y;   // the tray is drawn before (index 1)
    const float size = 200 * s, k = size / 220.0f;   // the web svg is 220 x 220 (viewBox -110..110)
    const float titleH = 30 * s;
    ImVec2 p0(m_optsRight ? d.x - 8 * s - size : 10 * s, trayTop - 14 * s - size - titleH - (m_holding ? 40 * s : 0));
    if (m_pOpts.floating) p0 = ImVec2(std::max(0.0f, std::min(m_pOpts.x, d.x - 80 * s)), std::max(0.0f, std::min(m_pOpts.y, d.y - 34 * s)));
    ImVec2 c(p0.x + size * 0.5f, p0.y + titleH + size * 0.5f);
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p0.x + size, p0.y + titleH + size + (m_holding ? 40 * s : 0)));
    // grip (56 px) + title
    u.grip(ImVec2(c.x - 18 * s, p0.y + 2 * s), 36 * s, nullptr, m_pOpts.collapsed, false);
    if (gripPanel(1, m_pOpts, ImVec2(c.x - 28 * s, p0.y), ImVec2(c.x + 28 * s, p0.y + 14 * s), p0)) return;
    std::string title = "AUCUNE BRIQUE";
    int focus = m_holding ? m_held.brick : m_selected;
    if (focus >= 0) title = m_lib.parts[m_bricks[focus].part].label;
    std::string up = title;
    for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
    if (up.size() > 28) up = up.substr(0, 27) + "…";
    float tw = u.textW(10.5f, up.c_str());
    u.text(ImVec2(c.x - tw * 0.5f, p0.y + 14 * s), 10.5f, C_DIM, up.c_str());
    if (m_pOpts.collapsed) return;
    const bool active = focus >= 0;
    const float alpha = active ? 1.0f : 0.45f;   // .opts.radial.idle #radial { opacity: 0.45 }
    const float RIN = 58 * k, ROUT = 100 * k, RDUP = 50 * k;
    struct Wedge { const char* act; float a; const char* icon; const char* tip; };
    static const Wedge W[4] = {{"rotU", -90, "M-9 4 L0 -5 L9 4", "Basculer vers le haut"},
                               {"rotR", 0, "M-6 -7 A9 9 0 1 1 -8 3 M-6 -7 L-6 -1 M-6 -7 L0 -7", "Tourner à droite 90°"},
                               {"rotD", 90, "M-9 -4 L0 5 L9 -4", "Basculer vers le bas"},
                               {"rotL", 180, "M6 -7 A9 9 0 1 0 8 3 M6 -7 L6 -1 M6 -7 L0 -7", "Tourner à gauche 90°"}};
    const float dx = u.mouse.x - c.x, dy = u.mouse.y - c.y, dist = std::sqrt(dx * dx + dy * dy);
    int hot = -1;   // 0..3 wedges, 4 duplicate
    if (dist <= RDUP + 4 * k) hot = 4;
    else if (dist >= RIN - 3 * k && dist <= ROUT + 6 * k) {
        float a = std::atan2(dy, dx) * 180.0f / 3.14159265f, bd = 360;
        for (int i = 0; i < 4; ++i) { float diff = std::fabs(std::fmod(a - W[i].a + 540.0f, 360.0f) - 180.0f); if (diff < bd) { bd = diff; hot = i; } }
    }
    const float half = 41.0f * 3.14159265f / 180.0f;
    for (int i = 0; i < 4; ++i) {
        float a = W[i].a * 3.14159265f / 180.0f;
        u.dl->PathArcTo(c, ROUT, a - half, a + half, 16);
        u.dl->PathArcTo(c, RIN, a + half, a - half, 12);
        const bool on = hot == i && active;
        u.dl->PathFillConvex(alphaOf(on ? C_CARD_HI : IM_COL32(27, 28, 32, 240), alpha));
        u.dl->PathArcTo(c, ROUT, a - half, a + half, 16);
        u.dl->PathArcTo(c, RIN, a + half, a - half, 12);
        u.dl->PathStroke(alphaOf(on ? C_LYEL : C_LINE, alpha), ImDrawFlags_Closed, 1.5f * k);
        ImVec2 ic(c.x + std::cos(a) * (RIN + ROUT) * 0.5f, c.y + std::sin(a) * (RIN + ROUT) * 0.5f);
        svg::path(u.dl, W[i].icon, ic, k, alphaOf(C_TEXT, alpha), 2.4f * k);
        if (hot == i) ImGui::SetTooltip("%s", W[i].tip);
    }
    const bool onDup = hot == 4 && active;
    u.dl->AddCircleFilled(c, RDUP, alphaOf(onDup ? C_LYEL : IM_COL32(27, 28, 32, 245), alpha), 48);
    u.dl->AddCircle(c, RDUP, alphaOf(C_LYEL, alpha), 48, 2 * k);
    svg::path(u.dl, "M-14 -10 h16 a3 3 0 0 1 3 3 v16 a3 3 0 0 1 -3 3 h-16 a3 3 0 0 1 -3 -3 v-16 a3 3 0 0 1 3 -3 z M5 -16 h11 a3 3 0 0 1 3 3 v13",
              c, k, alphaOf(onDup ? IM_COL32(0x1b, 0x15, 0x00, 255) : C_TEXT, alpha), 2.4f * k);
    float lw = u.textW(8 * k / s, "Dupliquer");
    u.text(ImVec2(c.x - lw * 0.5f, c.y + 22 * k), 8 * k / s, alphaOf(C_DIM, alpha), "Dupliquer");
    if (hot == 4) ImGui::SetTooltip("Dupliquer la brique");
    if (u.pressed && active && hot >= 0) {
        if (hot == 4) duplicateSelected();
        else if (hot == 0) tiltSelected(-1);
        else if (hot == 2) tiltSelected(1);
        else rotateSelected(hot == 1 ? -1 : 1);
    }
    // pills while a brick is in hand: Poser / Annuler
    if (m_holding) {
        float py = p0.y + titleH + size + 4 * s;
        const char* labels[2] = {"Poser", "Annuler"};
        float x = c.x - 70 * s;
        for (int i = 0; i < 2; ++i) {
            ImVec2 a(x, py), b(x + 66 * s, py + 34 * s);
            bool h = u.hover(a, b);
            u.dl->AddRectFilled(a, b, i == 0 ? C_LBLU : (h ? C_CARD_HI : IM_COL32(27, 28, 32, 240)), 17 * s);
            float w = u.textW(12.5f, labels[i], i == 0);
            u.text(ImVec2((a.x + b.x) * 0.5f - w * 0.5f, py + 9 * s), 12.5f, C_TEXT, labels[i], i == 0);
            if (h && u.pressed) { if (i == 0) dropHeld(); else cancelHeld(); }
            x += 74 * s;
        }
    }
}

// ------------------------------------------------------------------ themes (Réglages › Thème, Wzx 09/11/2026)
// The theme module (BrickTheme.h) resolves the theme, its mode, its tint and its accent into tokens; they become the
// colours of the web CSS here (C_PANEL, C_TEXT, C_ACCENT…), with the theme's fonts, radii and HUD bubbles.
//   Classic: the original dark LEGO interface. Crème: PC theme 2.0, the language of the iPad « entre-deux » mock-up
//   (white translucent pebbles, LEGO colour accents, light and dark), sober and dense for the mouse and 4K, with the
//   fine icon set (assets/brickit/ui_pc, tools/brickit/pc_icons.py). Minimal and Sommet: PLUM (plum-themes-kit).

// the fonts of the themes (assets/fonts, from the woff2 of the kit, tools/brickit/theme_fonts.py), loaded once
void BrickApp::loadThemeFonts() {
    ImGuiIO& io = ImGui::GetIO();
    const std::wstring dir = m_exeDir + L"\\assets\\fonts\\";
    auto add = [&](const wchar_t* file) -> ImFont* {
        const std::string path = plat::toUtf8(dir + file);
        FILE* f = nullptr;
        if (_wfopen_s(&f, (dir + file).c_str(), L"rb") != 0 || !f) return nullptr;
        fclose(f);
        ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 15.0f * m_ui);
        if (!font) return nullptr;
#ifdef _WIN32
        ImFontConfig sc;   // the symbols the kit's fonts do not have (★ ☆ ✎ ◀ ▶…): Segoe UI Symbol
        sc.MergeMode = true;
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisym.ttf", 15.0f * m_ui, &sc);
#endif
        return font;
    };
    g_fInstr[0] = add(L"InstrumentSans-Regular.ttf");
    g_fInstr[1] = add(L"InstrumentSans-SemiBold.ttf");
    g_fManrope = add(L"Manrope-Light.ttf");
    g_fGeist[0] = add(L"GeistMono-Regular.ttf");
    g_fGeist[1] = add(L"GeistMono-SemiBold.ttf");
}

// an image of a theme (assets/brickit/ui_pc/themes), loaded on first use
ImTextureID BrickApp::themeTex(const char* file) {
    auto it = m_themeTex.find(file);
    if (it != m_themeTex.end()) return it->second;
    ImTextureID tex = 0;
    std::wstring path = m_exeDir + L"\\assets\\brickit\\ui_pc\\themes\\";
    for (const char* c = file; *c; ++c) path += *c == '/' ? L'\\' : (wchar_t)*c;
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") == 0 && f) {
        std::vector<uint8_t> data, rgba;
        fseek(f, 0, SEEK_END);
        data.resize((size_t)ftell(f));
        fseek(f, 0, SEEK_SET);
        const bool ok = fread(data.data(), 1, data.size(), f) == data.size();
        fclose(f);
        int w = 0, h = 0;
        if (ok && decodeImage(data.data(), data.size(), rgba, w, h)) tex = m_gui->createTexture(rgba.data(), (uint32_t)w, (uint32_t)h);
    }
    m_themeTex[file] = tex;
    return tex;
}

// the tokens of the theme onto the colours of the interface
static void setTheme(const theme::Tokens& t) {
    C_BG = t.bg;
    C_PANEL = t.panel;
    C_PANEL_SOLID = t.panelSolid;
    C_CARD = t.card;
    C_CARD_HI = t.cardHi;
    C_LINE = t.line;
    C_TEXT = t.text;
    C_DIM = t.dim;
    C_FAINT = t.faint;
    C_ACCENT = t.accent;
    C_ACCENT_TXT = t.accentTxt;
    C_PROGRESS = t.progress;
    C_THUMB = t.thumb;
    C_INK = t.onAccent;   // (the text on the accent)
    C_OVERLAY = t.overlay;
    C_HUD = t.hud;
    g_rk = t.radius;
    g_pill = t.pill;
    g_lines = t.lines;
    g_hudStyle = t.hudStyle;
    // the fonts: em sizes matched to Segoe UI (ImGui sizes a font by ascent - descent: 1.33 em for Segoe UI, 1.22 em
    // for Instrument Sans, 1.30 em for Geist Mono, which is wide: a little smaller so that the texts keep their room)
    g_thFont = g_thBold = g_thHead = nullptr;
    g_fontK = 1.22f;
    if (t.font == theme::FONT_INSTRUMENT && g_fInstr[0]) {
        g_thFont = g_fInstr[0];
        g_thBold = g_fInstr[1] ? g_fInstr[1] : g_fInstr[0];
        g_thHead = g_fManrope ? g_fManrope : g_thBold;
        g_fontK = 1.22f * 1.22f / 1.33f * 1.02f;
    } else if (t.font == theme::FONT_GEIST && g_fGeist[0]) {
        g_thFont = g_fGeist[0];
        g_thBold = g_fGeist[1] ? g_fGeist[1] : g_fGeist[0];
        g_thHead = g_thBold;
        g_fontK = 1.22f * 1.30f / 1.33f * 0.86f;
    }
}

// the theme of this frame: resolved again when the theme, the mode, the tint or the accent change
bool BrickApp::applyTheme() {
    int ti = theme::indexOf(m_themeId);
    if (ti < 0) { m_themeId = "creme"; ti = theme::indexOf(m_themeId); }
    const theme::Def& d = theme::themes()[ti];
    const bool dark = d.mode >= 0 ? d.mode == 1 : pcDarkNow();
    const ThemeLook lk = m_themeLooks.count(m_themeId) ? m_themeLooks[m_themeId] : ThemeLook{};
    char key[96];
    snprintf(key, sizeof(key), "%s|%d|%d|%d", m_themeId.c_str(), dark ? 1 : 0, lk.tint, lk.accent);
    bool changed = false;
    if (m_thKey != key) {
        m_thKey = key;
        m_th = theme::resolve(m_themeId, dark, lk.tint, lk.accent);
        changed = true;
    }
    m_darkUi = m_th.dark;
    g_studTex = m_th.brickSwatch ? themeTex("glass/stud1x1.png") : 0;
    setTheme(m_th);
    return changed;
}

// Windows: dark apps (Paramètres › Personnalisation › Couleurs), read every 2 s
bool BrickApp::pcDarkNow() {
    if (m_opt.theme == 1 || (m_opt.theme < 0 && m_uiThemePref == 1)) return false;
    if (m_opt.theme == 2 || (m_opt.theme < 0 && m_uiThemePref == 2)) return true;
#ifdef _WIN32
    const double now = ImGui::GetTime();
    if (now - m_sysThemeT > 2.0 || m_sysThemeT < 0) {
        m_sysThemeT = now;
        DWORD v = 1, n = sizeof(v);
        if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &v, &n) == ERROR_SUCCESS)
            m_sysDark = v == 0;
    }
#endif
    return m_sysDark;
}

// an icon of the PC set (128 px PNG, loaded on first use): white (on dark, on the coloured bubbles) or ink
ImTextureID BrickApp::pcIcon(const char* name, bool white) {
    const std::string key = std::string(name) + (white ? "_blanc" : "_encre");
    auto it = m_pcIcons.find(key);
    if (it != m_pcIcons.end()) return it->second;
    ImTextureID tex = 0;
    std::vector<uint8_t> data, rgba;
    int w = 0, h = 0;
    const std::wstring path = m_exeDir + L"\\assets\\brickit\\ui_pc\\icons\\" + std::wstring(key.begin(), key.end()) + L".png";
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") == 0 && f) {
        fseek(f, 0, SEEK_END);
        data.resize((size_t)ftell(f));
        fseek(f, 0, SEEK_SET);
        const bool ok = fread(data.data(), 1, data.size(), f) == data.size();
        fclose(f);
        if (ok && decodeImage(data.data(), data.size(), rgba, w, h)) tex = m_gui->createTexture(rgba.data(), (uint32_t)w, (uint32_t)h);
    }
    m_pcIcons[key] = tex;
    return tex;
}

// the icons of the top bar and the library in the PC set when there is one (else the line icons of the web)
static const char* pcIconName(svg::Icon i) {
    switch (i) {
    case svg::I_UNDO: return "annuler";
    case svg::I_REDO: return "retablir";
    case svg::I_MOVE: return "deplacer";
    case svg::I_ROTATE: return "tourner";
    case svg::I_DELETE: return "supprimer";
    case svg::I_WINDOWS: return "fenetre";
    case svg::I_FILE: return "ouvrir";
    case svg::I_SETTINGS: return "reglages";
    case svg::I_SEARCH: return "chercher";
    case svg::I_FAV: return "favoris";
    case svg::I_RECENT: return "recentes";
    case svg::I_FOCUS: return "isoler";
    case svg::I_CLOSE: return "fermer";
    default: return nullptr;
    }
}

void BrickApp::drawIcon(ImDrawList* dl, svg::Icon i, ImVec2 c, float size, ImU32 col) {
    const char* n = pcIconName(i);
    ImTextureID t = n ? pcIcon(n, ((col >> 0) & 0xff) * 0.3f + ((col >> 8) & 0xff) * 0.59f + ((col >> 16) & 0xff) * 0.11f > 128) : 0;
    if (!t) { svg::icon(dl, i, c, size, col); return; }
    const float h = size * 0.62f * 1.25f;   // (the 24 grid: the glyph spans ~20 of it)
    dl->AddImage(t, ImVec2(c.x - h, c.y - h), ImVec2(c.x + h, c.y + h), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, (col >> 24) & 0xff));
}

// a bubble of the selection HUD: a vertical gradient (the colour mixed with 30 % white at the top), a 1 px white rim
// at 32 %, a very soft shadow
static void pcBubble(ImDrawList* dl, ImVec2 c, float r, ImU32 col, float al, float s) {
    if (r < 0.5f || al <= 0.01f) return;
    if (g_hudStyle != theme::HUD_LEGO) {   // Minimal: flat discs with a fine line; Sommet: flat pills, no rim
        for (int k = 2; k >= 1; --k) dl->AddCircleFilled(ImVec2(c.x, c.y + 1.0f * s + k * 0.5f * s), r + k * 0.9f * s, IM_COL32(12, 14, 18, (int)(14 * al)), 40);
        const float ca = ((col >> 24) & 0xff) / 255.0f;   // (Glass, Red: translucent smoked discs)
        dl->AddCircleFilled(c, r, (col & 0x00ffffff) | ((ImU32)(255 * al * ca) << 24), 48);
        const float lum = ((col >> 0) & 0xff) * 0.3f + ((col >> 8) & 0xff) * 0.59f + ((col >> 16) & 0xff) * 0.11f;
        if (g_hudStyle == theme::HUD_FLAT && (lum > 200 || ca < 0.95f)) dl->AddCircle(c, r, alphaOf(ca < 0.95f ? IM_COL32(255, 255, 255, 60) : C_LINE, al), 48, 1.0f * s);
        return;
    }
    for (int k = 3; k >= 1; --k) dl->AddCircleFilled(ImVec2(c.x, c.y + 1.2f * s + k * 0.5f * s), r + k * 1.1f * s, IM_COL32(12, 18, 30, (int)(16 * al)), 40);
    const float cr = ((col >> 0) & 0xff), cg = ((col >> 8) & 0xff), cb = ((col >> 16) & 0xff);
    auto mix = [&](float t) {   // t 0 top .. 1 bottom
        const float w = 0.30f * (1.0f - t);
        return IM_COL32((int)(cr + (255 - cr) * w), (int)(cg + (255 - cg) * w), (int)(cb + (255 - cb) * w), (int)(255 * al));
    };
    const int n = 48;
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    dl->PrimReserve(n * 3, n + 1);
    const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
    dl->PrimWriteVtx(c, uv, mix(0.5f));
    for (int i = 0; i < n; ++i) {
        const float a = i * 6.2831853f / n;
        const ImVec2 p(c.x + std::cos(a) * r, c.y + std::sin(a) * r);
        dl->PrimWriteVtx(p, uv, mix((p.y - (c.y - r)) / (2 * r)));
    }
    for (int i = 0; i < n; ++i) {
        dl->PrimWriteIdx(base);
        dl->PrimWriteIdx((ImDrawIdx)(base + 1 + i));
        dl->PrimWriteIdx((ImDrawIdx)(base + 1 + (i + 1) % n));
    }
    dl->AddCircle(c, r, mix(1.0f) & 0x00ffffff | ((ImU32)(160 * al) << 24), n, 1.0f * s);   // (the edge smoothed)
    dl->AddCircle(c, r - 0.8f * s, IM_COL32(255, 255, 255, (int)(82 * al)), n, 1.0f * s);   // liseré
}

// ------------------------------------------------------------------ touch HUD around the selection (Tiny Glade)
// Wzx (05/11/2026), cosy as Tiny Glade: bold rounded white icons with a soft shadow (assets/brickit/hud, drawn by
// tools/brickit/hud_icons.py), no panel, no frame (Wzx), 40 % transparent but under the cursor, around the projected
// bounds of the selection, fixed screen size. Above: the colour button; left: « … » (tilt up / down, delete); right: duplicate; below: turn 15° left, move
// (press and drag: the brick in hand, dropped on release), turn 15° right. Everything eases in (scale + fade, slight
// overshoot), follows the selection smoothly and grows on hover. The colour button opens an arc of round swatches
// (the real colours of the part, 12 per page) arriving one after another; a click (or press, slide, release) picks,
// the chosen swatch pops and the arc folds back.

static float easeOutBack(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float c1 = 1.70158f, c3 = c1 + 1.0f, u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}
static float easeOut(float t) { t = std::clamp(t, 0.0f, 1.0f); return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }

void BrickApp::loadHudIcons() {
    static const wchar_t* kNames[HUD_ICONS] = {L"turn_left", L"turn_right", L"duplicate", L"move", L"more", L"colour", L"hexagon"};
    for (int i = 0; i < HUD_ICONS; ++i) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, (m_exeDir + L"\\assets\\brickit\\hud\\" + kNames[i] + L".png").c_str(), L"rb") != 0 || !f) continue;
        std::vector<uint8_t> data;
        fseek(f, 0, SEEK_END);
        data.resize((size_t)ftell(f));
        fseek(f, 0, SEEK_SET);
        const bool ok = fread(data.data(), 1, data.size(), f) == data.size();
        fclose(f);
        std::vector<uint8_t> rgba;
        int w = 0, h = 0;
        if (ok && decodeImage(data.data(), data.size(), rgba, w, h)) m_hudTex[i] = m_gui->createTexture(rgba.data(), (uint32_t)w, (uint32_t)h);
    }
}

void BrickApp::uiSelHud() {
    const std::vector<int> sel = selection();
    const float dt = std::min(0.05f, ImGui::GetIO().DeltaTime);
    const bool shot = !m_opt.shotPath.empty();
    if (!m_selHud || sel.empty() || m_holding || m_mechOn || m_reading || (m_pressed && m_dragStarted)) {
        m_arcOpen = m_arcDrag = m_moreOpen = false;
        m_hudKey = -1;
        return;
    }
    if (m_opt.colorArc) m_arcOpen = true;   // (captures)
    // (the iPad: the whole HUD and its colour arc at 72 %, Wzx 08/11/2026: too big on the iPad screen)
    const float s = m_ui * (m_ipad ? 0.72f : 1.0f);
    Ui u = ui(s);
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const double now = ImGui::GetTime();
    // screen bounds of the selection
    float3 mn(1e30f), mx(-1e30f);
    for (int i : sel) {
        const Box b = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
        mn = minv(mn, b.mn);
        mx = maxv(mx, b.mx);
    }
    const View v = view();
    ImVec2 a(1e30f, 1e30f), b(-1e30f, -1e30f);
    for (int k = 0; k < 8; ++k) {
        float x, y;
        if (!v.project(float3((k & 1) ? mx.x : mn.x, (k & 2) ? mx.y : mn.y, (k & 4) ? mx.z : mn.z), x, y)) { m_arcOpen = m_moreOpen = false; return; }
        a = ImVec2(std::min(a.x, x), std::min(a.y, y));
        b = ImVec2(std::max(b.x, x), std::max(b.y, y));
    }
    const float top = 30 * s;   // under the header
    if (b.x < 0 || a.x > d.x || b.y < top || a.y > d.y) { m_arcOpen = m_moreOpen = false; return; }
    a = ImVec2(a.x - 10 * s, a.y - 10 * s);
    b = ImVec2(b.x + 10 * s, b.y + 10 * s);
    // a new selection: the HUD eases in; the frame follows smoothly (no lag in captures)
    if (sel[0] != m_hudKey) { m_hudKey = sel[0]; m_hudT0 = now; m_hudA = a; m_hudB = b; m_arcOpen = m_moreOpen = false; }
    const float follow = shot ? 1.0f : 1.0f - std::exp(-dt * 22.0f);
    m_hudA = ImVec2(m_hudA.x + (a.x - m_hudA.x) * follow, m_hudA.y + (a.y - m_hudA.y) * follow);
    m_hudB = ImVec2(m_hudB.x + (b.x - m_hudB.x) * follow, m_hudB.y + (b.y - m_hudB.y) * follow);
    a = m_hudA;
    b = m_hudB;
    const ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    const float appear = shot ? 1.0f : (float)((now - m_hudT0) / 0.22);
    const float fade = easeOut(appear);
    auto alpha = [&](ImU32 col, float k) { return (col & 0x00FFFFFF) | ((ImU32)(((col >> 24) & 0xFF) * std::clamp(k, 0.0f, 1.0f)) << 24); };
    // PC theme 2.0 (Wzx 09/11/2026, the iPad « entre-deux » mock-up, sober): bubbles in LEGO colours with a vertical
    // gradient and a fine white icon. Above: Couleur (yellow) between the pipette (take the colour of a brick) and the
    // brush (the current colour on the selection); the sides: turn 15° left / right (blue); below: duplicate (green);
    // bottom right: « … » (part sheet, tilt, delete). No move bubble: the brick itself is dragged.
    const float R = 15 * s, r2 = 11 * s, g = R + 14 * s, M = R + 6 * s;
    const bool darkUi = m_darkUi;
    auto place = [&](ImVec2 p) { return ImVec2(std::clamp(p.x, M, d.x - M), std::clamp(p.y, top + M, d.y - M)); };
    auto hot = [&](ImVec2 p, float r) { return (u.mouse.x - p.x) * (u.mouse.x - p.x) + (u.mouse.y - p.y) * (u.mouse.y - p.y) <= r * r; };
    // a bubble: eased in (delay), grown on hover; returns true on press
    auto bubble = [&](int slot, ImVec2 p, float rad, ImU32 col, const char* icon, bool whiteIcon, bool flip, float delay, const char* tip) {
        const float t = shot ? 1.0f : (float)((now - m_hudT0 - delay) / 0.22);
        const float sc = easeOutBack(t), al = easeOut(t * 1.4f);
        const bool h = hot(p, rad + 2 * s) && !m_arcOpen;
        float& hv = m_hudHover[slot];
        hv += ((h ? 1.0f : 0.0f) - hv) * (shot ? 1.0f : 1.0f - std::exp(-dt * 18.0f));
        const float r = rad * sc * (1.0f + 0.1f * hv);
        pcBubble(u.dl, p, r, col, al, s);
        if (ImTextureID tx = icon ? pcIcon(icon, whiteIcon) : 0) {
            const float q = r * 0.72f;
            u.dl->AddImage(tx, ImVec2(p.x - q, p.y - q), ImVec2(p.x + q, p.y + q), ImVec2(flip ? 1.0f : 0.0f, 0), ImVec2(flip ? 0.0f : 1.0f, 1), IM_COL32(255, 255, 255, (int)(255 * al)));
        }
        m_uiRects.push_back(ImVec4(p.x - rad, p.y - rad, p.x + rad, p.y + rad));
        if (h && tip) ImGui::SetTooltip("%s", tip);
        return h && u.pressed;
    };
    const int part = m_bricks[sel[0]].part;
    const int curColor = m_bricks[sel[0]].color;
    const ImVec2 pSw = place(ImVec2(c.x, a.y - g));
    {   // Couleur: the arc of colours
        const bool legoHud = m_th.hudStyle == theme::HUD_LEGO;   // (the colour bubble: LEGO yellow, or the theme's accent)
        const ImU32 cCol = legoHud ? IM_COL32(0xf2, 0xcd, 0x37, 255) : C_PROGRESS;
        const bool wCol = !legoHud && (((cCol >> 0) & 0xff) * 0.3f + ((cCol >> 8) & 0xff) * 0.59f + ((cCol >> 16) & 0xff) * 0.11f) < 170;
        const bool on = bubble(0, pSw, R, cCol, "couleur", wCol, false, 0.0f, m_arcOpen ? nullptr : "Couleur");
        if (on) {
            m_arcOpen = !m_arcOpen;
            m_arcDrag = m_arcOpen;
            m_arcPress = u.mouse;
            m_arcT0 = now;
            m_arcPick = -1;
            m_moreOpen = false;
            u.pressed = false;
        }
    }
    if (m_arcOpen) {
        std::vector<int> cols;
        const bool filter = !m_allColors && m_lib.hasColorTable(part);
        for (int i = 0; i < (int)m_lib.palette.size(); ++i)
            if (!filter || m_lib.colorExists(part, i)) cols.push_back(i);
        const int per = 12, pages = std::max(1, ((int)cols.size() + per - 1) / per);
        m_arcPage = std::clamp(m_arcPage, 0, pages - 1);
        const int first = m_arcPage * per, n = std::min(per, (int)cols.size() - first);
        const int slots = n + (pages > 1 ? 1 : 0);   // (+ the next page)
        const float rs = 15 * s;                    // swatches of 30 px (Wzx: twice smaller, a bit more spaced)
        const float rad = std::max(84 * s, slots * 44 * s / 6.2832f);
        const float step = std::min(44 * s / rad, 6.2832f / std::max(1, slots));
        // the arc around the colour button, opening downwards over the selection (a loose ring, the button at its top)
        const ImVec2 ac(std::clamp(pSw.x, rad + rs + 4 * s, std::max(rad + rs + 4 * s, d.x - rad - rs - 4 * s)),
                        std::clamp(pSw.y + rad * 0.55f, top + rad + rs + 4 * s, std::max(top + rad + rs + 4 * s, d.y - rad - rs - 4 * s)));
        m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));   // open: a click outside closes it, nothing else
        const float picked = m_arcPick >= 0 ? (float)((now - m_arcPickT) / 0.12) : 0.0f;   // after a pick: pop, then fold back
        int hit = -1;
        std::vector<ImVec2> at(slots);
        std::vector<float> sc(slots);
        for (int k = 0; k < slots; ++k) {
            const float ang = -1.5708f - 0.9f + k * step;   // from upper left, clockwise
            float t = shot && m_opt.arcPhase >= 0 ? (m_opt.arcPhase * 0.24f - k * 0.0125f) / 0.09f : shot ? 1.0f : (float)((now - m_arcT0 - k * 0.0125) / 0.09);   // (twice faster)
            float e = easeOutBack(t);
            if (m_arcPick >= 0 && k != m_arcPick) e *= 1.0f - easeOut(picked * 1.6f);   // the others fold back
            const float rr = rad * (0.55f + 0.45f * e);
            at[k] = ImVec2(ac.x + std::cos(ang) * rr, ac.y + std::sin(ang) * rr);
            sc[k] = std::max(0.0f, e);
            if (m_arcPick < 0 && sc[k] > 0.6f && hot(at[k], rs + 6 * s)) hit = k;
        }
        for (int k = 0; k < slots; ++k) {
            float& hv = m_arcHover[std::min(k, 63)];
            hv += ((k == hit ? 1.0f : 0.0f) - hv) * (shot ? 1.0f : 1.0f - std::exp(-dt * 16.0f));
            float grow = 1.0f + 0.12f * hv;
            if (k == m_arcPick) grow *= 1.0f + 0.35f * std::sin(std::clamp(picked, 0.0f, 1.0f) * 3.14159f);   // the pick pops
            const float r = rs * sc[k] * grow;
            if (r < 0.5f) continue;
            const float al = std::clamp(sc[k] * 1.3f, 0.0f, 1.0f) * (k == m_arcPick ? 1.0f - easeOut(picked - 0.6f) : 1.0f);
            const ImVec2 p = at[k];
            u.dl->AddCircleFilled(ImVec2(p.x, p.y + 2.5f * s), r + 1.5f * s, alpha(IM_COL32(10, 12, 18, 70), al), 32);
            if (k == n) {   // next page
                u.dl->AddCircleFilled(p, r, alpha(IM_COL32(0xee, 0xec, 0xe8, 240), al), 48);
                char t[16];
                snprintf(t, sizeof(t), "%d/%d", m_arcPage + 1, pages);
                const float tw = u.textW(8.5f, t, true);
                u.text(ImVec2(p.x - tw * 0.5f, p.y - 6 * s), 8.5f, alpha(IM_COL32(0x55, 0x58, 0x66, 255), al), t, true);
                if (k == hit) ImGui::SetTooltip("Autres couleurs");
                continue;
            }
            const Color& col = m_lib.palette[cols[first + k]];
            const bool trans = col.group == "transparent";
            const ImU32 fill = IM_COL32((int)(col.srgb.x * 255), (int)(col.srgb.y * 255), (int)(col.srgb.z * 255), trans ? 180 : 255);
            const ImU32 pale = IM_COL32((int)(150 + col.srgb.x * 105), (int)(150 + col.srgb.y * 105), (int)(150 + col.srgb.z * 105), 235);   // a soft pale ring, tinted
            if (g_studTex) {   // Glass, Red: the 1x1 brick, tinted
                const float q = r * 0.92f;
                u.dl->AddImage(g_studTex, ImVec2(p.x - q, p.y - q), ImVec2(p.x + q, p.y + q), ImVec2(0, 0), ImVec2(1, 1), alpha(fill, al));
                if (cols[first + k] == curColor) u.dl->AddRect(ImVec2(p.x - q - 3 * s, p.y - q - 3 * s), ImVec2(p.x + q + 3 * s, p.y + q + 3 * s), alpha(C_ACCENT, al), 4 * s, 0, 1.5f * s);
            } else {
            u.dl->AddCircleFilled(p, r, alpha(fill, al), 48);
            if (trans || col.group == "metallic") u.dl->AddCircleFilled(ImVec2(p.x - r * 0.32f, p.y - r * 0.32f), r * 0.3f, alpha(IM_COL32(255, 255, 255, 90), al), 24);
            if (cols[first + k] == curColor) u.dl->AddCircle(p, r + 1.5f * s, alpha(IM_COL32(255, 255, 255, 255), al), 32, 3.0f * s);
            else u.dl->AddCircle(p, r - 1.0f * s, alpha(pale, al), 32, 2.0f * s);
            }
        }
        if (hit >= 0 && hit < n) {   // the name of the colour under the cursor, in the middle of the ring
            const Color& col = m_lib.palette[cols[first + hit]];
            const float tw = u.textW(13, col.name.c_str(), true);
            const ImVec2 tp(ac.x - tw * 0.5f, ac.y - 9 * s);
            u.text(ImVec2(tp.x, tp.y + 2 * s), 13, IM_COL32(0, 0, 0, 150), col.name.c_str(), true);
            u.text(tp, 13, IM_COL32_WHITE, col.name.c_str(), true);
        }
        if (m_arcPick >= 0) {   // the pick done: apply once popped, close once folded
            if (picked >= 0.35f && !m_arcApplied) { applyColor(cols[first + m_arcPick]); m_arcApplied = true; }
            if (picked >= 1.0f) { m_arcOpen = false; m_arcPick = -1; }
            return;
        }
        const bool slid = (u.mouse.x - m_arcPress.x) * (u.mouse.x - m_arcPress.x) + (u.mouse.y - m_arcPress.y) * (u.mouse.y - m_arcPress.y) > 64 * s * s;
        auto choose = [&](int k) {
            if (k == n) { m_arcPage = (m_arcPage + 1) % pages; m_arcT0 = now; return; }
            m_arcPick = k;
            m_arcPickT = now;
            m_arcApplied = false;
            if (shot) { applyColor(cols[first + k]); m_arcOpen = false; m_arcPick = -1; }
        };
        if (u.pressed) {
            if (hit >= 0) choose(hit);
            else if (!hot(pSw, R)) m_arcOpen = false;
        }
        if (u.released && m_arcDrag) {   // marking menu: pressed on the button, released on a colour
            m_arcDrag = false;
            if (slid && hit >= 0) choose(hit);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_arcOpen = false;
        return;   // (the other buttons hidden while the arc is open)
    }
    // the bubbles around the selection
    const ImVec2 pL = place(ImVec2(a.x - g, c.y)), pR = place(ImVec2(b.x + g, c.y)), pDup = place(ImVec2(c.x, b.y + g));
    const ImVec2 pMore = place(ImVec2(b.x + g * 0.72f, b.y + g * 0.72f));
    const ImVec2 pPip = place(ImVec2(pSw.x - R - r2 - 6 * s, pSw.y + 4 * s)), pBru = place(ImVec2(pSw.x + R + r2 + 6 * s, pSw.y + 4 * s));
    const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
    const ImU32 curCol = IM_COL32((int)(cur.srgb.x * 255), (int)(cur.srgb.y * 255), (int)(cur.srgb.z * 255), 255);
    const bool curLight = cur.srgb.x * 0.3f + cur.srgb.y * 0.59f + cur.srgb.z * 0.11f > 0.62f;
    // the colours of the bubbles: LEGO (Classic, Crème), or the theme's own (Minimal: white discs, Sommet: the pill)
    const bool lego = m_th.hudStyle == theme::HUD_LEGO;
    const ImU32 cTurn = lego ? IM_COL32(0x00, 0x55, 0xbf, 255) : m_th.hudBtn, cDup = lego ? IM_COL32(0x23, 0x78, 0x41, 255) : m_th.hudBtn;
    const ImU32 cMore = lego ? (darkUi ? IM_COL32(0x5d, 0x65, 0x73, 255) : IM_COL32(0x3a, 0x42, 0x50, 255)) : m_th.hudBtn;
    const ImU32 cPip = lego ? IM_COL32(0xfe, 0x8a, 0x18, 255) : m_th.hudBtn;
    const bool wBtn = lego || m_th.hudBtnWhiteIcon;
    if (bubble(1, pL, R, cTurn, "tourner", wBtn, true, 0.03f, "Tourner de 15° à gauche")) turnSelectedFree(1);
    if (bubble(2, pR, R, cTurn, "tourner", wBtn, false, 0.06f, "Tourner de 15° à droite")) turnSelectedFree(-1);
    if (bubble(3, pDup, R, cDup, "dupliquer", wBtn, false, 0.09f, "Dupliquer")) duplicateSelected();
    if (bubble(4, pMore, r2, cMore, "plus_actions", wBtn, false, 0.12f, "Plus d'actions")) { m_moreOpen = !m_moreOpen; m_moreT0 = now; u.pressed = false; }
    if (bubble(5, pPip, r2, cPip, "pipette", wBtn, false, 0.15f, m_pipette ? "Pipette : clique une brique pour prendre sa couleur" : "Pipette : prendre la couleur d'une brique")) { m_pipette = !m_pipette; u.pressed = false; }
    if (m_pipette) u.dl->AddCircle(pPip, r2 + 3 * s, lego ? IM_COL32(0xfe, 0x8a, 0x18, 220) : C_PROGRESS, 40, 1.6f * s);
    {
        char tip[120];
        snprintf(tip, sizeof(tip), "Pinceau : appliquer %s à la sélection", cur.name.c_str());
        if (bubble(6, pBru, r2, curCol, "pinceau", !curLight, false, 0.18f, tip)) applyColor(m_color);
    }
    if (m_bricks[sel[0]].free) {   // the free angle, next to the brush
        const float3 f = qRot(m_bricks[sel[0]].rot, float3(1, 0, 0));
        float deg = std::fmod(std::atan2(-f.z, f.x) * 57.29578f + 360.0f, 90.0f);
        if (deg > 45) deg -= 90;
        char t[16];
        snprintf(t, sizeof(t), "%+d°", (int)std::lround(deg));
        u.text(ImVec2(pBru.x + r2 + 6 * s, pBru.y - 8 * s), 11, alpha(C_TEXT, fade), t, true);
    }
    // « … »: a small menu next to it (part sheet, tilt up / down, delete)
    if (m_moreOpen) {
        static const char* kItems[4] = {"Fiche de la pièce", "Basculer vers le haut", "Basculer vers le bas", "Supprimer"};
        const float iw = 168 * s, ih = 26 * s, pad = 5 * s;
        const float t = shot ? 1.0f : (float)((now - m_moreT0) / 0.16);
        const float e = easeOut(t);
        ImVec2 p0(pMore.x + r2 + 6 * s, pMore.y - ih * 0.5f - pad);
        if (p0.x + iw > d.x - 4 * s) p0.x = pMore.x - r2 - 6 * s - iw;
        if (p0.y + 4 * ih + 2 * pad > d.y - 4 * s) p0.y = d.y - 4 * s - 4 * ih - 2 * pad;
        const ImVec2 p1(p0.x + iw, p0.y + 4 * ih + 2 * pad);
        u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 3 * s), ImVec2(p1.x, p1.y + 4 * s), IM_COL32(12, 18, 30, (int)(28 * e)), 9 * s);
        u.dl->AddRectFilled(p0, p1, alpha(C_PANEL_SOLID, e), 9 * s);
        u.dl->AddRect(p0, p1, alpha(C_LINE, e), 9 * s, 0, 1.0f * s);
        m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
        for (int k = 0; k < 4; ++k) {
            const ImVec2 ia(p0.x + pad, p0.y + pad + k * ih), ib(p1.x - pad, ia.y + ih);
            const bool h = u.hover(ia, ib);
            if (h) u.dl->AddRectFilled(ia, ib, alpha(C_CARD, e), 6 * s);
            u.text(ImVec2(ia.x + 8 * s, ia.y + 5 * s), 11.5f, alpha(k == 3 ? IM_COL32(0xc9, 0x1a, 0x09, 255) : C_TEXT, e), kItems[k]);
            if (h && u.pressed) {
                m_moreOpen = false;
                u.pressed = false;
                if (k == 0) m_infoPart = part;
                else if (k == 1) tiltSelected(-1);
                else if (k == 2) tiltSelected(1);
                else { deleteSelected(); return; }
            }
        }
        if (u.pressed && !u.hover(p0, p1) && !hot(pMore, r2)) m_moreOpen = false;
    }
    (void)curColor;
}

// ------------------------------------------------------------------ temporary brick tray (BrickBac.cpp)
// A glass window: the view behind it shows through (a dark veil at half opacity, a thin light border, round corners;
// no blur, it would cost a copy of the view). Its title bar moves it, its corner resizes it. Inside, the thumbnails
// placed freely: drag one to move it (out of the window: the part in hand, in its colour), click one to take it, right
// click or Suppr on one removes it; the wheel zooms around the cursor, a right / middle drag on the empty space pans.
// A card of the library (the brick in hand) released over it is added.

void BrickApp::uiBac() {
    if (!m_bacOpen || m_ipad) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImGuiIO& io = ImGui::GetIO();
    if (m_bacRect.z <= 0) m_bacRect = ImVec4(d.x - 470 * s, 120 * s, 340 * s, 300 * s);   // first opening: right, under the colours
    m_bacRect.z = std::clamp(m_bacRect.z, 220 * s, d.x - 20 * s);
    m_bacRect.w = std::clamp(m_bacRect.w, 160 * s, d.y - 60 * s);
    m_bacRect.x = std::clamp(m_bacRect.x, 0.0f, d.x - 80 * s);
    m_bacRect.y = std::clamp(m_bacRect.y, 30 * s, d.y - 40 * s);
    const ImVec2 p0(m_bacRect.x, m_bacRect.y), p1(p0.x + m_bacRect.z, p0.y + m_bacRect.w);
    const float hb = 34 * s, rr = 16 * s;
    // glass
    u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 6 * s), ImVec2(p1.x, p1.y + 8 * s), IM_COL32(0, 0, 0, 40), rr);   // soft shadow
    u.dl->AddRectFilled(p0, p1, IM_COL32(24, 26, 32, 128), rr);
    u.dl->AddRectFilledMultiColor(ImVec2(p0.x + 1, p0.y + 1), ImVec2(p1.x - 1, p0.y + hb), IM_COL32(255, 255, 255, 22), IM_COL32(255, 255, 255, 22), IM_COL32(255, 255, 255, 6), IM_COL32(255, 255, 255, 6));
    u.dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 60), rr, 0, 1.2f * s);
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    const bool overWin = u.hover(p0, p1);
    // a library card released over it (the new brick in hand): added in its colour where it is dropped
    if (m_holding && m_held.isNew && overWin && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        const int part = m_bricks[m_held.brick].part, color = m_bricks[m_held.brick].color;
        cancelHeld();
        bacAdd(part, color, (u.mouse.x - p0.x - m_bacPan.x) / m_bacZoom - 42, (u.mouse.y - p0.y - hb - m_bacPan.y) / m_bacZoom - 42);
        m_dragStarted = false;
        m_pressed = false;
    }
    // title bar: grip (move), title, « Ajouter la sélection », close
    u.text(ImVec2(p0.x + 14 * s, p0.y + 9 * s), 12, IM_COL32(236, 236, 238, 235), "Bac à briques", true);
    {
        const char* lab = "+ Ajouter la sélection";
        const float bw = u.textW(11, lab) + 18 * s;
        ImVec2 ba(p1.x - 36 * s - bw, p0.y + 6 * s), bb(p1.x - 36 * s, p0.y + 28 * s);
        const bool h = u.hover(ba, bb);
        u.dl->AddRectFilled(ba, bb, h ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 28), 11 * s);
        u.text(ImVec2(ba.x + 9 * s, ba.y + 4 * s), 11, IM_COL32(245, 245, 247, 255), lab);
        if (h) ImGui::SetTooltip("Une vignette par brique sélectionnée, avec sa couleur");
        if (h && u.pressed) { bacAddSelection(); u.pressed = false; }
        ImVec2 ca(p1.x - 30 * s, p0.y + 6 * s), cb(p1.x - 8 * s, p0.y + 28 * s);
        const bool hc = u.hover(ca, cb);
        if (hc) u.dl->AddCircleFilled(ImVec2((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f), 11 * s, IM_COL32(255, 255, 255, 40), 24);
        svg::path(u.dl, "M-5 -5 L5 5 M5 -5 L-5 5", ImVec2((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f), s, IM_COL32(240, 240, 242, 230), 2.0f * s);
        if (hc && u.pressed) { m_bacOpen = false; m_bacDirty = true; u.pressed = false; }
        const bool onBar = u.hover(p0, ImVec2(ba.x - 4 * s, p0.y + hb));
        if (onBar && u.pressed) { m_bacMove = 1; m_bacGrab = ImVec2(u.mouse.x - p0.x, u.mouse.y - p0.y); }
    }
    // resize corner
    {
        const ImVec2 ka(p1.x - 16 * s, p1.y - 16 * s);
        for (int k = 0; k < 3; ++k) u.dl->AddLine(ImVec2(p1.x - 5 * s - k * 4 * s, p1.y - 5 * s), ImVec2(p1.x - 5 * s, p1.y - 5 * s - k * 4 * s), IM_COL32(255, 255, 255, 70), 1.2f * s);
        if (u.hover(ka, p1) && u.pressed) { m_bacMove = 2; m_bacGrab = ImVec2(p1.x - u.mouse.x, p1.y - u.mouse.y); u.pressed = false; }
    }
    if (m_bacMove && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) { m_bacMove = 0; m_bacDirty = true; }
    if (m_bacMove == 1) { m_bacRect.x = u.mouse.x - m_bacGrab.x; m_bacRect.y = u.mouse.y - m_bacGrab.y; }
    if (m_bacMove == 2) { m_bacRect.z = u.mouse.x + m_bacGrab.x - p0.x; m_bacRect.w = u.mouse.y + m_bacGrab.y - p0.y; }
    // content
    const ImVec2 c0(p0.x + 6 * s, p0.y + hb), c1(p1.x - 6 * s, p1.y - 6 * s);
    const bool overContent = u.hover(c0, c1) && !m_bacMove;
    u.dl->PushClipRect(c0, c1, true);
    const float cs = 84 * m_bacZoom * s;   // card size
    auto cardAt = [&](const BacItem& it) { return ImVec2(c0.x + m_bacPan.x + it.x * m_bacZoom * s, c0.y + m_bacPan.y + it.y * m_bacZoom * s); };
    int hover = -1;
    for (int i = (int)m_bac.size() - 1; i >= 0 && overContent; --i) {
        const ImVec2 a = cardAt(m_bac[i]);
        if (u.hover(a, ImVec2(a.x + cs, a.y + cs))) { hover = i; break; }
    }
    if (m_bac.empty()) {
        const char* t1 = "Glisse ici une pièce de la bibliothèque,";
        const char* t2 = "ou « Ajouter la sélection »";
        u.text(ImVec2((c0.x + c1.x) * 0.5f - u.textW(11, t1) * 0.5f, (c0.y + c1.y) * 0.5f - 16 * s), 11, IM_COL32(255, 255, 255, 140), t1);
        u.text(ImVec2((c0.x + c1.x) * 0.5f - u.textW(11, t2) * 0.5f, (c0.y + c1.y) * 0.5f + 2 * s), 11, IM_COL32(255, 255, 255, 140), t2);
    }
    for (int i = 0; i < (int)m_bac.size(); ++i) {
        const BacItem& it = m_bac[i];
        const ImVec2 a = cardAt(it), b(a.x + cs, a.y + cs);
        if (b.x < c0.x || a.x > c1.x || b.y < c0.y || a.y > c1.y) continue;
        const bool h = i == hover || i == m_bacDrag;
        u.dl->AddRectFilled(a, b, h ? IM_COL32(255, 255, 255, 34) : IM_COL32(255, 255, 255, 14), 12 * s * m_bacZoom);
        if (it.tex && it.tex != (ImTextureID)1) u.dl->AddImage(it.tex, ImVec2(a.x + 3 * s, a.y + 3 * s), ImVec2(b.x - 3 * s, b.y - 3 * s));
        else if (!it.tex) u.text(ImVec2(a.x + cs * 0.5f - 6 * s, a.y + cs * 0.5f - 8 * s), 12, IM_COL32(255, 255, 255, 110), "…");
        if (h) u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 120), 12 * s * m_bacZoom, 0, 1.2f * s);
    }
    u.dl->PopClipRect();
    if (hover >= 0 && m_bacDrag < 0) {
        const BacItem& it = m_bac[hover];
        ImGui::SetTooltip("%s · %s", m_lib.parts[it.part].label.c_str(), m_lib.palette[it.color].name.c_str());
    }
    // a card: press, then move (in the window: placed; out of it: the part in hand) or release (taken)
    if (hover >= 0 && u.pressed && !m_bacMove) { m_bacDrag = hover; m_bacPress = u.mouse; m_bacDragMoved = false; }
    if (m_bacDrag >= 0) {
        BacItem& it = m_bac[m_bacDrag];
        const ImVec2 dm(u.mouse.x - m_bacPress.x, u.mouse.y - m_bacPress.y);
        if (dm.x * dm.x + dm.y * dm.y > 36 * s * s) m_bacDragMoved = true;
        if (m_bacDragMoved && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (overWin) { it.x += io.MouseDelta.x / (m_bacZoom * s); it.y += io.MouseDelta.y / (m_bacZoom * s); m_bacDirty = true; }
            else {   // out of the tray: the part in hand, its colour (drop on release, as from the library)
                const int part = it.part;
                m_color = it.color;
                m_bacDrag = -1;
                spawn(part, u.mouse.x, u.mouse.y);
                m_dragStarted = true;
                m_pressed = false;
            }
        } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!m_bacDragMoved) {   // a click: the part in hand in the middle of the view
                m_color = it.color;
                spawn(it.part, (m_stage.x + m_stage.z) * 0.5f, (m_stage.y + m_stage.w) * 0.5f);
            } else {   // dropped: on top of the others
                BacItem moved = it;
                m_bac.erase(m_bac.begin() + m_bacDrag);
                m_bac.push_back(moved);
            }
            m_bacDrag = -1;
        }
    }
    // remove: right click or Suppr on a card
    if (hover >= 0 && m_bacDrag < 0 && (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !m_bacPanned || ImGui::IsKeyPressed(ImGuiKey_Delete))) {
        if (m_bac[hover].tex && m_bac[hover].tex != (ImTextureID)1) m_gui->releaseTexture(m_bac[hover].tex);
        m_bac.erase(m_bac.begin() + hover);
        m_bacDirty = true;
    }
    // zoom around the cursor, pan with a right / middle drag
    if (overContent && io.MouseWheel != 0) {
        const float z0 = m_bacZoom, z1 = std::clamp(z0 * std::pow(1.12f, io.MouseWheel), 0.4f, 3.0f);
        const ImVec2 m(u.mouse.x - c0.x, u.mouse.y - c0.y);
        m_bacPan = ImVec2(m.x - (m.x - m_bacPan.x) * z1 / z0, m.y - (m.y - m_bacPan.y) * z1 / z0);
        m_bacZoom = z1;
        m_bacDirty = true;
    }
    if (overContent && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) { m_bacPanning = true; m_bacPanned = false; }
    if (m_bacPanning) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            if (std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y) > 0) { m_bacPan.x += io.MouseDelta.x; m_bacPan.y += io.MouseDelta.y; m_bacPanned = true; m_bacDirty = true; }
        } else m_bacPanning = false;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseReleased(ImGuiMouseButton_Right)) m_bacPanned = false;
    if (m_bacDirty && !m_bacMove && m_bacDrag < 0 && !m_bacPanning) saveBac();
}

// ------------------------------------------------------------------ colours panel (right)

// ------------------------------------------------------------------ iPad: colours (right) and tools (left)
// Wzx (08/11/2026): the colour column of the PC is too long for the iPad and the radial pad reads badly. The iPad gets
// a thin column of round swatches (the current colour, then the last ones used; a tap on the current one opens the
// whole palette beside it) and a dock of big labelled buttons on the left (turn, tilt, duplicate, delete).

static ImU32 colU32(const Color& c, int a = 255) { return IM_COL32((int)(c.srgb.x * 255), (int)(c.srgb.y * 255), (int)(c.srgb.z * 255), a); }

static void swatch(ImDrawList* dl, ImVec2 c, float r, const Color& col, float s) {
    dl->AddCircleFilled(ImVec2(c.x, c.y + 1.5f * s), r + 1 * s, IM_COL32(0, 0, 0, 70), 32);
    dl->AddCircleFilled(c, r, colU32(col), 32);
    if (col.group == "transparent") dl->AddCircleFilled(ImVec2(c.x - r * 0.3f, c.y - r * 0.3f), r * 0.35f, IM_COL32(255, 255, 255, 90), 20);
    if (col.group == "metallic") dl->AddCircle(c, r * 0.62f, IM_COL32(255, 255, 255, 80), 24, 1.5f * s);
    dl->AddCircle(c, r, IM_COL32(255, 255, 255, 34), 32, 1.0f * s);
}

void BrickApp::uiColorsTouch() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const int fbp = focusBrick();
    const int fpart = fbp >= 0 && fbp < (int)m_bricks.size() ? m_bricks[fbp].part : -1;
    const bool filter = !m_allColors && fpart >= 0 && m_lib.hasColorTable(fpart);
    // the last colours used (7), the usual ones while there are fewer
    std::vector<int> recent;
    for (int c : m_recentColors) if (c != m_color && (int)recent.size() < 7) recent.push_back(c);
    static const int kUsual[] = {15, 0, 4, 1, 14, 2, 71, 72, 19, 25};
    for (int id : kUsual) {
        const int c = colorIndex(id);
        if ((int)recent.size() >= 7) break;
        if (c >= 0 && c != m_color && std::find(recent.begin(), recent.end(), c) == recent.end()) recent.push_back(c);
    }
    const float W = 52 * s, rCur = 18 * s, r = 14 * s, step = 36 * s, pad = 10 * s;
    const float H = pad + 2 * rCur + 14 * s + 10 * s + recent.size() * step + pad - 6 * s;
    const float top = m_stage.y, bot = m_stage.w;
    const ImVec2 p0(d.x - 12 * s - W, std::max(top + 12 * s, (top + bot - H) * 0.5f)), p1(d.x - 12 * s, p0.y + H);
    u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 6 * s), ImVec2(p1.x, p1.y + 6 * s), IM_COL32(0, 0, 0, 50), W * 0.5f);
    u.dl->AddRectFilled(p0, p1, C_OVERLAY, W * 0.5f);
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    const float cx = (p0.x + p1.x) * 0.5f;
    // the current colour: a bigger swatch ringed in yellow, a tap opens the palette
    const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
    ImVec2 cc(cx, p0.y + pad + rCur);
    swatch(u.dl, cc, rCur, cur, s);
    u.dl->AddCircle(cc, rCur + 3.5f * s, m_paletteOpen ? C_ACCENT : IM_COL32(255, 255, 255, 120), 40, 2.0f * s);
    {   // « + » badge: the whole palette
        const ImVec2 b(cc.x + rCur * 0.72f, cc.y + rCur * 0.72f);
        u.dl->AddCircleFilled(b, 7 * s, C_ACCENT, 20);
        u.dl->AddLine(ImVec2(b.x - 3.5f * s, b.y), ImVec2(b.x + 3.5f * s, b.y), C_INK, 1.8f * s);
        u.dl->AddLine(ImVec2(b.x, b.y - 3.5f * s), ImVec2(b.x, b.y + 3.5f * s), C_INK, 1.8f * s);
    }
    const bool hitCur = u.pressed && u.hover(ImVec2(cc.x - rCur - 6 * s, cc.y - rCur - 6 * s), ImVec2(cc.x + rCur + 6 * s, cc.y + rCur + 6 * s));
    if (hitCur) m_paletteOpen = !m_paletteOpen;
    float y = cc.y + rCur + 14 * s;
    u.dl->AddLine(ImVec2(p0.x + 12 * s, y - 4 * s), ImVec2(p1.x - 12 * s, y - 4 * s), C_LINE, 1.0f);
    y += 6 * s + r;
    for (int c : recent) {
        const ImVec2 pc(cx, y);
        const bool exists = !filter || m_lib.colorExists(fpart, c);
        swatch(u.dl, pc, r, m_lib.palette[c], s);
        if (!exists) u.dl->AddCircleFilled(pc, r, IM_COL32(0x13, 0x14, 0x17, 170), 32);
        if (exists && u.pressed && u.hover(ImVec2(p0.x, y - step * 0.5f), ImVec2(p1.x, y + step * 0.5f))) applyColor(c);
        y += step;
    }
    if (!m_paletteOpen) return;
    // the whole palette, beside the column: groups of round swatches, « Toutes les couleurs » for the impossible ones
    const int cols = 7;
    const float sr = 15 * s, sg = 38 * s, ppad = 14 * s, labH = 20 * s;
    struct G { const char* title; const char* key; std::vector<int> idx; };
    G groups[3] = {{"Unies", "standard", {}}, {"Transparentes", "transparent", {}}, {"Métal", "metallic", {}}};
    for (int i = 0; i < (int)m_lib.palette.size(); ++i)
        for (G& g : groups) if (m_lib.palette[i].group == g.key) g.idx.push_back(i);
    float PH = ppad + 22 * s + 28 * s;
    for (G& g : groups) if (!g.idx.empty()) PH += labH + ((g.idx.size() + cols - 1) / cols) * sg;
    PH += ppad - 6 * s;
    const float PW = ppad * 2 + cols * sg - (sg - 2 * sr);
    ImVec2 q0(p0.x - 10 * s - PW, std::clamp((p0.y + p1.y - PH) * 0.5f, top + 8 * s, std::max(top + 8 * s, bot - 8 * s - PH))), q1(q0.x + PW, q0.y + PH);
    u.dl->AddRectFilled(ImVec2(q0.x, q0.y + 8 * s), ImVec2(q1.x, q1.y + 8 * s), IM_COL32(0, 0, 0, 60), 18 * s);
    u.dl->AddRectFilled(q0, q1, C_PANEL_SOLID, 18 * s);
    u.dl->AddRect(q0, q1, C_LINE, 18 * s);
    m_uiRects.push_back(ImVec4(q0.x, q0.y, q1.x, q1.y));
    float py = q0.y + ppad;
    char nm[96];
    snprintf(nm, sizeof(nm), "%s · %d", cur.name.c_str(), cur.id);
    u.text(ImVec2(q0.x + ppad, py), 13, C_TEXT, nm, true);
    py += 22 * s;
    {   // toggle « Toutes les couleurs »
        ImVec2 ca(q0.x + ppad, py + 3 * s), cb(ca.x + 16 * s, ca.y + 16 * s);
        if (m_allColors) {
            u.dl->AddRectFilled(ca, cb, C_ACCENT, 4 * s);
            u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(ca.x + 4 * s, ca.y + 8 * s), ImVec2(ca.x + 7 * s, ca.y + 12 * s), ImVec2(ca.x + 12.5f * s, ca.y + 4 * s)}.data(), 3, C_INK, 0, 2 * s);
        } else u.dl->AddRect(ca, cb, C_FAINT, 4 * s, 0, 1.5f * s);
        u.text(ImVec2(cb.x + 8 * s, py + 2 * s), 11, C_DIM, filter || m_allColors ? "Toutes les couleurs (même inexistantes pour cette pièce)" : "Toutes les couleurs");
        if (u.pressed && u.hover(ImVec2(q0.x, py), ImVec2(q1.x, py + 22 * s))) m_allColors = !m_allColors;
        py += 28 * s;
    }
    bool picked = false;
    for (G& g : groups) {
        if (g.idx.empty()) continue;
        u.text(ImVec2(q0.x + ppad, py), 10, C_FAINT, g.title, true);
        py += labH;
        for (size_t k = 0; k < g.idx.size(); ++k) {
            const int i = g.idx[k];
            const ImVec2 pc(q0.x + ppad + sr + (k % cols) * sg, py + sr + (k / cols) * sg);
            const bool exists = !filter || m_lib.colorExists(fpart, i);
            swatch(u.dl, pc, sr, m_lib.palette[i], s);
            if (!exists) {
                u.dl->AddCircleFilled(pc, sr, IM_COL32(0x13, 0x14, 0x17, 180), 32);
                u.dl->AddLine(ImVec2(pc.x - sr * 0.5f, pc.y + sr * 0.5f), ImVec2(pc.x + sr * 0.5f, pc.y - sr * 0.5f), IM_COL32(255, 255, 255, 70), 1.5f * s);
            }
            if (i == m_color) u.dl->AddCircle(pc, sr + 3.5f * s, C_ACCENT, 32, 2.2f * s);
            if (exists && u.pressed && u.hover(ImVec2(pc.x - sg * 0.5f, pc.y - sg * 0.5f), ImVec2(pc.x + sg * 0.5f, pc.y + sg * 0.5f))) { applyColor(i); picked = true; }
        }
        py += ((g.idx.size() + cols - 1) / cols) * sg;
    }
    if (picked || (u.pressed && !hitCur && !u.hover(q0, q1) && !u.hover(p0, p1))) m_paletteOpen = false;
}

// the tools of the selection, left (iPad): big buttons with a word, grouped; dimmed while nothing is selected
static void dockIcon(ImDrawList* dl, int kind, ImVec2 c, float s, ImU32 col) {
    const float t = 2.2f * s;
    auto arrowHead = [&](ImVec2 tip, ImVec2 dir) {   // a filled head pointing along dir
        const float l = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir = ImVec2(dir.x / l, dir.y / l);
        const ImVec2 n(-dir.y, dir.x);
        const float h = 5.5f * s, w = 4.5f * s;
        dl->AddTriangleFilled(ImVec2(tip.x + dir.x * 1.5f * s, tip.y + dir.y * 1.5f * s), ImVec2(tip.x - dir.x * h + n.x * w, tip.y - dir.y * h + n.y * w),
                              ImVec2(tip.x - dir.x * h - n.x * w, tip.y - dir.y * h - n.y * w), col);
    };
    switch (kind) {
    case 0: case 1: {   // turn left / right: a flat brick seen from above circled by an arrow
        dl->AddRect(ImVec2(c.x - 5 * s, c.y - 3.5f * s), ImVec2(c.x + 5 * s, c.y + 3.5f * s), col, 2 * s, 0, 1.6f * s);
        const float R = 9.5f * s, dir = kind == 0 ? -1.0f : 1.0f;
        const float a0 = -1.5708f - dir * 0.35f, a1 = a0 + dir * 4.9f;
        dl->PathArcTo(c, R, a0, a1, 28);
        dl->PathStroke(col, 0, t);
        const ImVec2 tip(c.x + std::cos(a1) * R, c.y + std::sin(a1) * R);
        arrowHead(tip, ImVec2(-std::sin(a1) * dir, std::cos(a1) * dir));
        break;
    }
    case 2: case 3: {   // tilt up / down: a brick seen from the side, an arrow arcing over (or under) it
        const float dy = kind == 2 ? 1.0f : -1.0f;
        dl->AddRect(ImVec2(c.x - 7 * s, c.y + dy * 1 * s - 3 * s), ImVec2(c.x + 7 * s, c.y + dy * 1 * s + 3 * s), col, 1.5f * s, 0, 1.6f * s);
        for (int k = -1; k <= 1; k += 2) dl->AddRectFilled(ImVec2(c.x + k * 3.5f * s - 1.5f * s, c.y + dy * 1 * s - 3 * s - 2 * s), ImVec2(c.x + k * 3.5f * s + 1.5f * s, c.y + dy * 1 * s - 3 * s), col);
        const ImVec2 ac(c.x, c.y + dy * 6 * s);
        const float R = 9 * s;
        const float a0 = dy > 0 ? 3.4f : -3.4f + 6.2832f, a1 = dy > 0 ? 3.4f + 2.6f : 6.2832f - 3.4f - 2.6f;
        dl->PathArcTo(ac, R, kind == 2 ? 3.6f : 2.68f, kind == 2 ? 5.82f : 0.46f, 20);
        dl->PathStroke(col, 0, t);
        (void)a0; (void)a1;
        const float ae = kind == 2 ? 5.82f : 0.46f;
        const ImVec2 tip(ac.x + std::cos(ae) * R, ac.y + std::sin(ae) * R);
        arrowHead(tip, kind == 2 ? ImVec2(-std::sin(ae), std::cos(ae)) : ImVec2(std::sin(ae), -std::cos(ae)));
        break;
    }
    case 4:   // duplicate: two rounded squares, the front one with a +
        dl->AddRect(ImVec2(c.x - 9 * s, c.y - 9 * s), ImVec2(c.x + 3 * s, c.y + 3 * s), col, 3 * s, 0, 1.8f * s);
        dl->AddRectFilled(ImVec2(c.x - 3 * s, c.y - 3 * s), ImVec2(c.x + 9 * s, c.y + 9 * s), col, 3 * s);
        dl->AddLine(ImVec2(c.x + 3 * s, c.y), ImVec2(c.x + 3 * s, c.y + 6 * s), IM_COL32(0x1b, 0x1c, 0x20, 255), 1.8f * s);
        dl->AddLine(ImVec2(c.x, c.y + 3 * s), ImVec2(c.x + 6 * s, c.y + 3 * s), IM_COL32(0x1b, 0x1c, 0x20, 255), 1.8f * s);
        break;
    case 5:   // delete: a bin
        dl->AddLine(ImVec2(c.x - 9 * s, c.y - 6 * s), ImVec2(c.x + 9 * s, c.y - 6 * s), col, t);
        dl->AddLine(ImVec2(c.x - 3 * s, c.y - 9 * s), ImVec2(c.x + 3 * s, c.y - 9 * s), col, t);
        dl->AddQuad(ImVec2(c.x - 7 * s, c.y - 6 * s), ImVec2(c.x + 7 * s, c.y - 6 * s), ImVec2(c.x + 5.5f * s, c.y + 9 * s), ImVec2(c.x - 5.5f * s, c.y + 9 * s), col, 1.8f * s);
        dl->AddLine(ImVec2(c.x - 2 * s, c.y - 2 * s), ImVec2(c.x - 2 * s, c.y + 5 * s), col, 1.6f * s);
        dl->AddLine(ImVec2(c.x + 2 * s, c.y - 2 * s), ImVec2(c.x + 2 * s, c.y + 5 * s), col, 1.6f * s);
        break;
    case 6:   // place: a check
        dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - 8 * s, c.y), ImVec2(c.x - 2.5f * s, c.y + 6 * s), ImVec2(c.x + 9 * s, c.y - 7 * s)}.data(), 3, col, 0, 2.8f * s);
        break;
    case 7:   // cancel: a cross
        dl->AddLine(ImVec2(c.x - 7 * s, c.y - 7 * s), ImVec2(c.x + 7 * s, c.y + 7 * s), col, 2.6f * s);
        dl->AddLine(ImVec2(c.x + 7 * s, c.y - 7 * s), ImVec2(c.x - 7 * s, c.y + 7 * s), col, 2.6f * s);
        break;
    }
}

void BrickApp::uiToolDock() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const int focus = m_holding ? m_held.brick : m_selected;
    const bool active = focus >= 0 || !m_multi.empty();
    struct T { int icon; const char* label; int act; };   // act: 0..3 turn L / R, tilt up / down, 4 duplicate, 5 delete, 6 place, 7 cancel
    std::vector<std::vector<T>> groups;
    if (m_holding) groups = {{{6, "Poser", 6}, {7, "Annuler", 7}}, {{0, "Tourner", 0}, {1, "Tourner", 1}}, {{2, "Basculer", 2}, {3, "Basculer", 3}}};
    else groups = {{{0, "Tourner", 0}, {1, "Tourner", 1}}, {{2, "Basculer", 2}, {3, "Basculer", 3}}, {{4, "Dupliquer", 4}, {5, "Supprimer", 5}}};
    const float B = 54 * s, gap = 6 * s, sep = 14 * s, pad = 8 * s;
    int n = 0;
    for (auto& g : groups) n += (int)g.size();
    const float H = pad * 2 + n * B + (n - (int)groups.size()) * gap + ((int)groups.size() - 1) * sep;
    const float top = m_stage.y, bot = m_stage.w;
    const ImVec2 p0(12 * s, std::max(top + 12 * s, (top + bot - H) * 0.5f)), p1(p0.x + B + 2 * pad, p0.y + H);
    u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 6 * s), ImVec2(p1.x, p1.y + 6 * s), IM_COL32(0, 0, 0, 50), 20 * s);
    u.dl->AddRectFilled(p0, p1, C_OVERLAY, 20 * s);
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    float y = p0.y + pad;
    for (size_t gi = 0; gi < groups.size(); ++gi) {
        if (gi) {
            u.dl->AddLine(ImVec2(p0.x + 14 * s, y + sep * 0.5f - gap), ImVec2(p1.x - 14 * s, y + sep * 0.5f - gap), C_LINE, 1.0f);
            y += sep - gap;
        }
        for (const T& t : groups[gi]) {
            const ImVec2 a(p0.x + pad, y), b(a.x + B, y + B);
            const bool on = active || t.act >= 6;
            const bool h = on && u.hover(a, b) && ImGui::IsMouseDown(ImGuiMouseButton_Left);
            const ImU32 bg = t.act == 6 ? C_ACCENT : (h ? C_CARD_HI : IM_COL32(255, 255, 255, 10));
            u.dl->AddRectFilled(a, b, bg, 14 * s);
            const ImU32 fg = t.act == 6 ? C_INK : (t.act == 5 ? IM_COL32(0xff, 0x8a, 0x80, 255) : C_TEXT);
            const ImU32 fgA = on ? fg : alphaOf(fg, 0.3f);
            dockIcon(u.dl, t.icon, ImVec2((a.x + b.x) * 0.5f, a.y + 21 * s), s, fgA);
            const float tw = u.textW(8.5f, t.label, t.act == 6);
            u.text(ImVec2((a.x + b.x) * 0.5f - tw * 0.5f, b.y - 16 * s), 8.5f, on ? (t.act == 6 ? C_INK : C_DIM) : alphaOf(C_DIM, 0.4f), t.label, t.act == 6);
            if (on && u.pressed && u.hover(a, b)) {
                switch (t.act) {
                case 0: rotateSelected(1); break;
                case 1: rotateSelected(-1); break;
                case 2: tiltSelected(-1); break;
                case 3: tiltSelected(1); break;
                case 4: duplicateSelected(); break;
                case 5: deleteSelected(); break;
                case 6: dropHeld(); break;
                case 7: cancelHeld(); break;
                }
            }
            y += B + gap;
        }
    }
}

void BrickApp::uiColors() {
    if (!m_pColors.visible || m_mechOn) return;
    if (m_ipad) { uiColorsTouch(); return; }
    Ui u = ui(m_ui);
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    const float W = 104 * s, pad = 6 * s, sw = 28 * s, gap = 4 * s;
    // height of the content
    int nStd = 0, nTr = 0, nMe = 0;
    for (const Color& c : m_lib.palette) (c.group == "transparent" ? nTr : c.group == "metallic" ? nMe : nStd)++;
    auto rows = [](int n) { return (n + 2) / 3; };
    const float h6 = 16 * s;
    float H = pad + 14 * s + 6 * s;
    if (!m_pColors.collapsed) H += 14 * s + 6 * s + 18 * s + 3 * h6 + (rows(nStd) + rows(nTr) + rows(nMe)) * (sw + gap) + pad;
    // the colours that exist for real for the part selected or in hand (Rebrickable data); the others greyed out
    const int fbp = focusBrick();
    const int fpart = fbp >= 0 && fbp < (int)m_bricks.size() ? m_bricks[fbp].part : -1;
    const bool filter = !m_allColors && fpart >= 0 && m_lib.hasColorTable(fpart);
    const float headerH = 30 * s, trayTop = m_uiRects.size() > 1 ? m_uiRects[1].y : d.y;
    float y = std::max(headerH + 12 * s, headerH + (trayTop - headerH - H) * 0.5f);
    ImVec2 p0(d.x - 10 * s - W, y), p1(d.x - 10 * s, y + H);
    u.dl->AddRectFilled(p0, p1, C_OVERLAY, rr(14 * s));
    if (g_lines) u.dl->AddRect(p0, p1, C_LINE, rr(14 * s));
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    u.grip(ImVec2(p0.x + pad, p0.y + pad), W - 2 * pad, "COULEURS", m_pColors.collapsed);
    if (u.hover(p0, ImVec2(p1.x, p0.y + pad + 14 * s)) && u.pressed) m_pColors.collapsed = !m_pColors.collapsed;
    if (m_pColors.collapsed) return;
    float cy = p0.y + pad + 14 * s + 6 * s;
    const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
    ImU32 curCol = IM_COL32((int)(cur.srgb.x * 255), (int)(cur.srgb.y * 255), (int)(cur.srgb.z * 255), 255);
    u.dl->AddRectFilled(ImVec2(p0.x + pad, cy + 1 * s), ImVec2(p0.x + pad + 12 * s, cy + 13 * s), curCol, 3 * s);
    u.dl->AddRect(ImVec2(p0.x + pad, cy + 1 * s), ImVec2(p0.x + pad + 12 * s, cy + 13 * s), IM_COL32(255, 255, 255, 51), 3 * s);
    char buf[96];
    snprintf(buf, sizeof(buf), "%s · %d", cur.name.c_str(), cur.id);
    u.dl->PushClipRect(ImVec2(p0.x, cy), ImVec2(p1.x - pad, cy + 16 * s), true);
    u.text(ImVec2(p0.x + pad + 18 * s, cy), 10.5f, C_DIM, buf);
    u.dl->PopClipRect();
    cy += 14 * s + 6 * s;
    {   // toggle: every colour (bypass the real colours of the part)
        ImVec2 ca(p0.x + pad + 2 * s, cy + 1 * s), cb(ca.x + 12 * s, ca.y + 12 * s);
        const bool h = u.hover(ImVec2(p0.x, cy), ImVec2(p1.x, cy + 16 * s));
        if (m_allColors) {
            u.dl->AddRectFilled(ca, cb, C_ACCENT, 3 * s);
            u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(ca.x + 3 * s, ca.y + 6 * s), ImVec2(ca.x + 5 * s, ca.y + 9 * s), ImVec2(ca.x + 9.5f * s, ca.y + 3 * s)}.data(), 3, C_INK, 0, 1.6f * s);
        } else u.dl->AddRect(ca, cb, h ? C_TEXT : C_FAINT, 3 * s, 0, 1.3f * s);
        u.text(ImVec2(cb.x + 5 * s, cy), 9.5f, h ? C_TEXT : C_DIM, "Toutes les couleurs");
        if (h) ImGui::SetTooltip("Non coché : seules les couleurs qui existent pour cette pièce (données Rebrickable)");
        if (h && u.pressed) m_allColors = !m_allColors;
        cy += 18 * s;
    }
    auto group = [&](const char* title, const char* key) {
        u.text(ImVec2(p0.x + pad + 2 * s, cy + 3 * s), 9.5f, C_FAINT, title, true);
        cy += h6;
        int k = 0;
        for (int i = 0; i < (int)m_lib.palette.size(); ++i) {
            const Color& c = m_lib.palette[i];
            if (c.group != key) continue;
            ImVec2 a(p0.x + pad + 2 * s + (k % 3) * (sw + gap), cy + (k / 3) * (sw + gap)), b(a.x + sw, a.y + sw);
            ImU32 col = IM_COL32((int)(c.srgb.x * 255), (int)(c.srgb.y * 255), (int)(c.srgb.z * 255), 255);
            if (g_studTex) {   // Glass, Red: the brick seen from above, multiplied by the colour (transparent ones see-through)
                u.dl->AddImage(g_studTex, a, b, ImVec2(0, 0), ImVec2(1, 1), (col & 0x00ffffff) | ((ImU32)(c.group == "transparent" ? 165 : 255) << 24));
                if (c.group == "metallic") u.dl->AddRectFilledMultiColor(ImVec2(a.x + 3 * s, a.y + 3 * s), ImVec2(b.x - 6 * s, (a.y + b.y) * 0.5f), IM_COL32(255, 255, 255, 70), IM_COL32(255, 255, 255, 70), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
            } else {
            u.dl->AddRectFilled(a, b, col, 7 * s);
            if (c.group == "transparent")   // .sw.trans::after: light diagonal
                u.dl->AddRectFilledMultiColor(ImVec2(a.x + 4 * s, a.y + 4 * s), ImVec2(b.x - 4 * s, b.y - 4 * s), IM_COL32(255, 255, 255, 115), IM_COL32(255, 255, 255, 20), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 20));
            if (c.group == "metallic")   // .sw.metal::after: vertical sheen
                u.dl->AddRectFilledMultiColor(a, ImVec2(b.x, (a.y + b.y) * 0.5f), IM_COL32(255, 255, 255, 115), IM_COL32(255, 255, 255, 115), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
            u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 26), 7 * s);
            }
            const bool exists = !filter || m_lib.colorExists(fpart, i);
            if (!exists) {   // does not exist for this part: veiled and struck through
                u.dl->AddRectFilled(a, b, IM_COL32(0x13, 0x14, 0x17, 190), 7 * s);
                u.dl->AddLine(ImVec2(a.x + 5 * s, b.y - 5 * s), ImVec2(b.x - 5 * s, a.y + 5 * s), IM_COL32(255, 255, 255, 70), 1.5f * s);
            }
            if (i == m_color) u.dl->AddRect(ImVec2(a.x - 3 * s, a.y - 3 * s), ImVec2(b.x + 3 * s, b.y + 3 * s), C_ACCENT, (g_studTex ? 4 : 9) * s, 0, (g_studTex ? 1.5f : 2) * s);
            if (u.hover(a, b)) {
                if (exists) ImGui::SetTooltip("%s · %d", c.name.c_str(), c.id);
                else ImGui::SetTooltip("%s · %d : n'existe pas pour cette pièce (coche « Toutes les couleurs »)", c.name.c_str(), c.id);
                if (u.pressed && exists) applyColor(i);
            }
            ++k;
        }
        cy += rows(k) * (sw + gap);
    };
    group("UNIES", "standard");
    group("TRANSPARENTES", "transparent");
    group("MÉTAL", "metallic");
}

// ------------------------------------------------------------------ library (tray, bottom)

void BrickApp::uiTray() {
    if (!m_pTray.visible) { m_uiRects.push_back(ImVec4(0, ImGui::GetIO().DisplaySize.y, 0, ImGui::GetIO().DisplaySize.y)); return; }
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    static const float kTh[3] = {64, 84, 112};
    const float th = kTh[std::clamp(m_thumbSize, 0, 2)] * s;
    const float padX = 12 * s, padT = 4 * s, padB = 6 * s, gap = 4 * s;
    const float gripH = 14 * s, famH = 30 * s, catH = 26 * s, cardsH = th * 2 + 6 * s + 6 * s;
    const float H = padT + gripH + (m_pTray.collapsed ? 0 : gap + famH + gap + catH + gap + cardsH) + padB;
    const bool fl = m_pTray.floating;
    const float TW = fl ? std::min(780 * s, d.x * 0.94f) : d.x;
    const float TX = fl ? std::max(0.0f, std::min(m_pTray.x, d.x - 80 * s)) : 0.0f;
    const float TY = fl ? std::max(0.0f, std::min(m_pTray.y, d.y - 34 * s)) : d.y - H;
    ImVec2 p0(TX, TY), p1(TX + TW, TY + H);
    const float alpha = m_mechOn ? 0.45f : 1.0f;   // #app.mech-on .tray { opacity: .45 }
    if (fl) {   // .tray.floating: accent border .55, radius 12, shadow
        m_uiRects.push_back(ImVec4(0, d.y, 0, d.y));   // (index 1: the stage reaches the bottom)
        u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 14 * s), ImVec2(p1.x, p1.y + 14 * s), IM_COL32(0, 0, 0, 70), 12 * s);
        u.dl->AddRectFilled(p0, p1, C_PANEL, rr(12 * s));
        u.dl->AddRect(p0, p1, IM_COL32(247, 197, 30, 140), 12 * s);
    } else {
        u.dl->AddRectFilled(p0, p1, C_PANEL);
        u.dl->AddLine(ImVec2(0, p0.y + 0.5f), ImVec2(d.x, p0.y + 0.5f), C_LINE, 1.0f);
    }
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    float y = p0.y + padT;
    u.grip(ImVec2(TX + padX, y), TW - 2 * padX, "BIBLIOTHÈQUE", m_pTray.collapsed);
    if (gripPanel(0, m_pTray, ImVec2(TX, y - 2 * s), ImVec2(TX + TW, y + gripH), p0)) return;
    if (m_pTray.collapsed) return;
    const bool live = !m_mechOn;
    y += gripH + gap;
    // ---- row 1: recent / favourites icons, families with counts, then search and size tools on the right
    // .fscroll: the tabs scroll sideways (scrollbar hidden) left of the tools (.ftools: search field, magnifier, size)
    const float toolsW = (m_searchOpen ? 194 * s : 0) + 36 * s + 32 * s;
    const ImVec2 fa(TX + padX, y), fb(TX + TW - padX - toolsW - 6 * s, y + famH);
    const float famStart = TX + padX - m_famScroll;
    float x = famStart;
    u.dl->PushClipRect(fa, fb, true);
    auto famTab = [&](const char* label, int n, bool selected, svg::Icon icon, bool isIcon, float& xx) {
        char cnt[16];
        snprintf(cnt, sizeof(cnt), "%d", n);
        float w = isIcon ? 32 * s : 12 * s + u.textW(13, label, selected) + (n >= 0 ? 4 * s + u.textW(11, cnt) : 0) + 12 * s;
        ImVec2 a(xx, y), b(xx + w, y + famH);
        bool h = live && u.hover(a, b) && u.hover(fa, fb);
        if (selected) u.dl->AddRectFilled(a, b, alphaOf(C_CARD_HI, alpha), 15 * s);
        ImU32 col = alphaOf(selected ? C_ACCENT_TXT : (h ? C_TEXT : C_DIM), alpha);
        if (isIcon) drawIcon(u.dl, icon, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), 16 * s, col);
        else {
            ImVec2 ts = u.text(ImVec2(a.x + 12 * s, a.y + 7 * s), 13, col, label, selected);
            if (n >= 0) u.text(ImVec2(a.x + 12 * s + ts.x + 4 * s, a.y + 9 * s), 11, alphaOf(col, 0.55f), cnt);
        }
        xx += w + 4 * s;
        return h && u.released && !swipeTaken(1) && m_swipeDragged != 1;
    };
    if (famTab("", -1, m_fam == "recent" && !m_searchOpen, svg::I_RECENT, true, x)) { m_fam = "recent"; m_cat.clear(); m_sub.clear(); m_searchOpen = false; m_cardsScroll = 0; }
    if (u.hover(ImVec2(x - 36 * s, y), ImVec2(x - 4 * s, y + famH))) ImGui::SetTooltip("Récentes");
    if (famTab("", -1, m_fam == "fav" && !m_searchOpen, svg::I_FAV, true, x)) { m_fam = "fav"; m_cat.clear(); m_sub.clear(); m_searchOpen = false; m_cardsScroll = 0; }
    if (u.hover(ImVec2(x - 36 * s, y), ImVec2(x - 4 * s, y + famH))) ImGui::SetTooltip("Favoris (appui long sur une pièce pour l'ajouter)");
    // parts per family, per category, per Technic sub-family: counted once (the full library has 22 000 parts)
    if (m_countsFor != m_lib.parts.size()) {
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
    const int loaded = m_availCount;
    if (famTab("Toutes", loaded, m_fam == "all" && !m_searchOpen, svg::I_RECENT, false, x)) { m_fam = "all"; m_cat.clear(); m_sub.clear(); m_searchOpen = false; m_cardsScroll = 0; }
    for (const Fam& f : families()) {
        int n = 0;
        for (const char* k : f.cats) { auto it = m_catCount.find(k); if (it != m_catCount.end()) n += it->second; }
        if (famTab(f.name, n, m_fam == f.name && !m_searchOpen, svg::I_RECENT, false, x)) { m_fam = f.name; m_cat.clear(); m_sub.clear(); m_searchOpen = false; m_cardsScroll = 0; }
    }
    u.dl->PopClipRect();
    {
        const float content = x - famStart, view = fb.x - fa.x;
        if (live && u.hover(fa, fb)) m_famScroll -= (ImGui::GetIO().MouseWheel + ImGui::GetIO().MouseWheelH) * 60 * s;
        swipeRow(1, fa, fb, m_famScroll, m_famVel, std::max(0.0f, content - view), live);
    }
    // tools on the right: [search field] search, size
    float tx = TX + TW - padX - 32 * s;
    {
        ImVec2 a(tx, y), b(tx + 32 * s, y + famH);
        bool h = live && u.hover(a, b);
        drawIcon(u.dl, svg::I_SIZE, ImVec2(a.x + 16 * s, a.y + 15 * s), 16 * s, alphaOf(h ? C_TEXT : C_DIM, alpha));
        static const char* sizes[3] = {"petites", "moyennes", "grandes"};
        if (h) ImGui::SetTooltip("Taille des miniatures (%s)", sizes[std::clamp(m_thumbSize, 0, 2)]);
        if (h && u.pressed) m_thumbSize = (m_thumbSize + 1) % 3;
        tx -= 36 * s;
        a = ImVec2(tx, y); b = ImVec2(tx + 32 * s, y + famH);
        h = live && u.hover(a, b);
        if (m_searchOpen) u.dl->AddRectFilled(a, b, C_CARD_HI, 15 * s);
        drawIcon(u.dl, svg::I_SEARCH, ImVec2(a.x + 16 * s, a.y + 15 * s), 16 * s, alphaOf(m_searchOpen ? C_ACCENT_TXT : (h ? C_TEXT : C_DIM), alpha));
        if (h) ImGui::SetTooltip("Chercher une pièce");
        if (h && u.pressed) { m_searchOpen = !m_searchOpen; if (!m_searchOpen) m_search.clear(); }
        if (m_searchOpen) {   // input.search: 190 px, 28 px high, round, card background
            ImVec2 sa(tx - 194 * s, y + 1 * s);
            ImGui::SetNextWindowPos(sa);
            ImGui::SetNextWindowSize(ImVec2(190 * s, 28 * s));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14 * s);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12 * s, 6 * s));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(C_CARD));
            ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(C_LINE));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::Begin("##search", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
            char buf[128];
            snprintf(buf, sizeof(buf), "%s", m_search.c_str());
            ImGui::SetNextItemWidth(190 * s);
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            if (ImGui::InputTextWithHint("##q", "Nom ou réf. LDraw…", buf, sizeof(buf))) { m_search = buf; m_cardsScroll = 0; }
            if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape)) { m_search.clear(); m_searchOpen = false; }
            ImGui::End();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(4);
        }
    }
    y += famH + gap;
    // ---- row 2: categories (pills) or a note; .cats scrolls sideways too
    const ImVec2 qa(TX + padX, y), qb(TX + TW - padX, y + catH);
    const float catStart = TX + padX - m_catScroll;
    x = catStart;
    u.dl->PushClipRect(qa, qb, true);
    auto catPill = [&](const char* label, int n, bool selected, bool empty) {
        char cnt[16];
        snprintf(cnt, sizeof(cnt), "%d", n);
        float w = 10 * s + u.textW(12, label) + (n > 0 ? 4 * s + u.textW(10.5f, cnt) : 0) + 10 * s;
        ImVec2 a(x, y + 1 * s), b(x + w, y + 25 * s);
        bool h = live && !empty && u.hover(a, b) && u.hover(qa, qb);
        float al = (empty ? 0.38f : 1.0f) * alpha;
        ImU32 border = selected ? C_ACCENT : C_LINE;
        if (empty) {   // dashed border
            for (float dx = a.x + 8 * s; dx < b.x - 8 * s; dx += 6 * s) {
                u.dl->AddLine(ImVec2(dx, a.y), ImVec2(dx + 3 * s, a.y), alphaOf(border, al), 1.0f);
                u.dl->AddLine(ImVec2(dx, b.y), ImVec2(dx + 3 * s, b.y), alphaOf(border, al), 1.0f);
            }
        } else u.dl->AddRect(a, b, alphaOf(border, al), 12 * s, 0, 1.0f);
        ImU32 col = alphaOf(selected ? C_ACCENT_TXT : (h ? C_TEXT : C_DIM), al);
        ImVec2 ts = u.text(ImVec2(a.x + 10 * s, a.y + 4.5f * s), 12, col, label);
        if (n > 0) u.text(ImVec2(a.x + 10 * s + ts.x + 4 * s, a.y + 6 * s), 10.5f, alphaOf(col, 0.55f), cnt);
        x += w + 4 * s;
        return h && u.released && !swipeTaken(2) && m_swipeDragged != 2;
    };
    const Fam* fam = nullptr;
    for (const Fam& f : families()) if (m_fam == f.name) fam = &f;
    if (m_searchOpen && !m_search.empty()) { std::string t = "Recherche « " + m_search + " »"; u.text(ImVec2(x + 4 * s, y + 6 * s), 11.5f, C_FAINT, t.c_str()); }
    else if (!fam) {
        const char* t = m_fam == "recent" ? "Récentes : les 24 dernières pièces posées" : m_fam == "fav" ? "Favoris : appui long sur une pièce > Ajouter aux favoris" : "Toutes les pièces importées";
        u.text(ImVec2(x + 4 * s, y + 6 * s), 11.5f, C_FAINT, t);
    } else {
        if (catPill("Tout", 0, m_cat.empty(), false)) { m_cat.clear(); m_sub.clear(); m_cardsScroll = 0; }
        std::vector<std::pair<const char*, int>> cs;
        for (const char* c : fam->cats) {
            auto it = m_catCount.find(c);
            cs.push_back({c, it == m_catCount.end() ? 0 : it->second});
        }
        std::stable_sort(cs.begin(), cs.end(), [](const std::pair<const char*, int>& a, const std::pair<const char*, int>& b) { return (a.second > 0) > (b.second > 0); });
        for (auto& c : cs) {
            if (std::string(c.first) == "Technic" && c.second) {   // 700+ Technic parts: one chip per sub-family
                for (const char* sn : kTechSubs) {
                    auto it = m_techSubCount.find(sn);
                    const int k = it == m_techSubCount.end() ? 0 : it->second;
                    if (!k) continue;
                    if (catPill(sn, k, m_cat == "Technic" && m_sub == sn, false)) { m_cat = "Technic"; m_sub = sn; m_cardsScroll = 0; }
                }
                continue;
            }
            if (catPill(catFr(c.first), c.second, m_cat == c.first, c.second == 0)) { m_cat = c.first; m_sub.clear(); m_cardsScroll = 0; }
        }
    }
    u.dl->PopClipRect();
    {
        const float content = x - catStart, view = qb.x - qa.x;
        if (live && u.hover(qa, qb)) m_catScroll -= (ImGui::GetIO().MouseWheel + ImGui::GetIO().MouseWheelH) * 60 * s;
        swipeRow(2, qa, qb, m_catScroll, m_catVel, std::max(0.0f, content - view), live);
    }
    y += catH + gap;
    // ---- cards on two rows (column by column), horizontal scroll with the wheel
    std::vector<int> parts = visibleParts();
    const float cgap = 6 * s;
    const int cols = (int)(parts.size() + 1) / 2;
    const float contentW = cols * (th + cgap), viewW = TW - 2 * padX;
    ImVec2 ca(TX + padX, y), cb(TX + TW - padX, y + cardsH);
    if (live && u.hover(ca, cb) && ImGui::GetIO().MouseWheel != 0) m_cardsScroll -= ImGui::GetIO().MouseWheel * th;
    // click + drag sideways = slide the cards (inertia); upwards = the brick goes in hand (below)
    swipeRow(0, ca, cb, m_cardsScroll, m_cardsVel, std::max(0.0f, contentW - viewW), live);
    u.dl->PushClipRect(ca, cb, true);
    if (parts.empty()) {
        const char* msg = (m_searchOpen && !m_search.empty()) ? "Aucune pièce pour cette recherche" : m_fam == "recent" ? "Les pièces que tu poses apparaîtront ici"
                        : m_fam == "fav" ? "Aucun favori : appui long sur une pièce, puis Ajouter aux favoris" : "Aucune pièce importée dans cette catégorie";
        u.text(ImVec2(TX + padX + 8 * s, y + th - 8 * s), 12, C_FAINT, msg);
    }
    const double now = ImGui::GetTime();
    for (int k = 0; k < (int)parts.size(); ++k) {
        int c = k / 2, r = k % 2;
        ImVec2 a(TX + padX + c * (th + cgap) - m_cardsScroll, y + r * (th + cgap)), b(a.x + th, a.y + th);
        if (b.x < ca.x || a.x > cb.x) continue;
        const int part = parts[k];
        if (k == 3) m_mtCard = ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);   // (mouse tests: a card on the lower row)
        bool h = live && u.hover(a, b) && u.hover(ca, cb);
        u.dl->AddRectFilled(a, b, alphaOf(h ? C_CARD_HI : C_CARD, alpha), 10 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(part, tex, uv0, uv1)) {   // img: object-fit contain, scale(1.4): the middle of the thumbnail, zoomed
            ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
            u.dl->AddImageRounded(tex, ImVec2(a.x + 1.5f * s, a.y + 1.5f * s), ImVec2(b.x - 1.5f * s, b.y - 1.5f * s), ImVec2(uv0.x + du.x, uv0.y + du.y),
                                  ImVec2(uv1.x - du.x, uv1.y - du.y), alphaOf(m_ipad ? IM_COL32_WHITE : C_THUMB, alpha), 9 * s);
        }
        u.dl->AddRect(a, b, alphaOf(h || m_libPress == part ? C_ACCENT : C_LINE, alpha), 10 * s, 0, 1.5f * s);
        if (std::find(m_fav.begin(), m_fav.end(), part) != m_fav.end()) u.text(ImVec2(b.x - 14 * s, a.y + 2 * s), 11, C_ACCENT_TXT, "★");
        if (m_dev && m_devUseOverrides && m_devOverrides.count(devId(part))) {   // .card.devmod::after: "M" badge
            ImVec2 ma(b.x - 6 * s - 18 * s, a.y + 6 * s), mb(b.x - 6 * s, a.y + 24 * s);
            u.dl->AddRectFilled(ma, mb, C_DEV, 5 * s);
            float mw = u.textW(11, "M", true);
            u.text(ImVec2((ma.x + mb.x) * 0.5f - mw * 0.5f, ma.y + 2 * s), 11, IM_COL32(0x0d, 0x1a, 0x1f, 255), "M", true);
        }
        if (h) {
            char tip[200];
            snprintf(tip, sizeof(tip), "%s · %s", m_lib.parts[part].label.c_str(), m_lib.parts[part].name.c_str());
            if (m_libPress < 0) ImGui::SetTooltip("%s", tip);
            if (u.pressed) { m_libPress = part; m_libPressPos = u.mouse; m_libPressTime = now; m_libLong = false; }
            if (m_dev && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { m_ctxPart = part; m_ctxPos = u.mouse; }   // DEV: Edit thumbnail
        }
    }
    u.dl->PopClipRect();
    // scrollbar (thin)
    if (contentW > viewW) {
        float bw = viewW * viewW / contentW, bx = TX + padX + (viewW - bw) * (m_cardsScroll / (contentW - viewW));
        u.dl->AddRectFilled(ImVec2(bx, cb.y - 3 * s), ImVec2(bx + bw, cb.y - 1 * s), C_LINE, 2 * s);
    }
    // press on a card: long press (480 ms, still) = part sheet; drag out of the tray = brick in hand following the
    // cursor (drop on release); click = brick in hand in the middle of the view
    if (m_libPress >= 0 && (swipeTaken(0) || m_swipeDragged == 0)) m_libPress = -1;   // a side drag: no brick
    if (m_libPress >= 0) {
        ImVec2 dm(u.mouse.x - m_libPressPos.x, u.mouse.y - m_libPressPos.y);
        const bool moved = dm.x * dm.x + dm.y * dm.y > 49 * s * s;
        if (!m_libLong && !moved && now - m_libPressTime > 0.48) { m_libLong = true; m_infoPart = m_libPress; }
        if (moved && ImGui::IsMouseDown(ImGuiMouseButton_Left) && u.mouse.y >= p0.y) {   // taken (upwards): its thumbnail follows the pointer
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (thumbUv(m_libPress, tex, uv0, uv1)) {
                ImDrawList* fg = ImGui::GetForegroundDrawList();
                const float g = 84 * s;
                const ImVec2 c(u.mouse.x, u.mouse.y - g * 0.35f);
                fg->AddRectFilled(ImVec2(c.x - g * 0.5f, c.y - g * 0.5f + 6 * s), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f + 6 * s), IM_COL32(0, 0, 0, 50), 14 * s);
                fg->AddRectFilled(ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), IM_COL32(255, 255, 255, 40), 14 * s);
                const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);   // (the cards zoom x 1.4)
                fg->AddImage(tex, ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), IM_COL32(255, 255, 255, 235));
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (moved && u.mouse.y < p0.y) {   // out of the tray: drag and drop
            int part = m_libPress;
            m_libPress = -1;
            spawn(part, u.mouse.x, u.mouse.y);
            m_dragStarted = true;   // released over the view = dropped (editInput)
            m_pressed = false;
        } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!m_libLong && !moved) spawn(m_libPress, d.x * 0.5f, (30 * s + p0.y) * 0.55f);
            m_libPress = -1;
        }
    }
}

// ------------------------------------------------------------------ Mechanical panel (right)

void BrickApp::uiMechPanel() {
    if (!m_mechOn) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    const float W = 280 * s, pad = 12 * s;
    ImVec2 p0(d.x - 16 * s - W, 30 * s + 12 * s);
    // content height first (two passes would be cleaner; the layout is linear)
    float H = pad + 20 * s + 38 * s + 38 * s + 44 * s + m_mm.warnings.size() * 18 * s + m_mm.cmds.size() * 78 * s + 30 * s + pad;
    if (m_mm.cmds.empty()) H += 34 * s;
    ImVec2 p1(p0.x + W, p0.y + H);
    u.dl->AddRectFilled(p0, p1, C_PANEL, rr(12 * s));
    u.dl->AddRect(p0, p1, C_LINE, rr(12 * s));
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    float y = p0.y + pad, x = p0.x + pad;
    u.text(ImVec2(x, y), 14, C_TEXT, "Mécanique", true);
    {   // Quitter: back to building
        float qw = u.textW(12, "Quitter") + 20 * s;
        ImVec2 a(p1.x - pad - qw, y - 6 * s), b(p1.x - pad, y + 24 * s);
        bool h = u.hover(a, b);
        u.dl->AddRectFilled(a, b, h ? C_CARD_HI : C_CARD, 7 * s);
        u.text(ImVec2(a.x + 10 * s, a.y + 8 * s), 12, C_TEXT, "Quitter");
        if (h) ImGui::SetTooltip("Revenir au montage (Échap)");
        if (h && u.pressed) { mechExit(); return; }
    }
    y += 26 * s;
    auto smallBtn = [&](const char* label, bool on, float& bx, float by) {
        float w = u.textW(12, label, on) + 20 * s;
        ImVec2 a(bx, by), b(bx + w, by + 30 * s);
        bool h = u.hover(a, b);
        u.dl->AddRectFilled(a, b, on ? C_ACCENT : (h ? C_CARD_HI : C_CARD), 7 * s);
        u.text(ImVec2(a.x + 10 * s, a.y + 8 * s), 12, on ? C_INK : C_TEXT, label, on);
        bx += w + 6 * s;
        return h && u.pressed;
    };
    float bx = x;
    if (smallBtn("Pose de montage", false, bx, y)) {
        for (MechCmd& c : m_mm.cmds) c.value = 0;
        std::fill(m_mm.q.begin(), m_mm.q.end(), 0.0);
        m_mech.solve(m_mm);
        mechApplyPose();
    }
    y += 38 * s;
    bx = x;
    if (smallBtn("Liaisons", m_mechJoints, bx, y)) m_mechJoints = !m_mechJoints;
    if (smallBtn("Couleurs par corps", m_mechTint, bx, y)) { m_mechTint = !m_mechTint; syncObjects(); mechApplyPose(); }
    y += 38 * s;
    // stats
    int nRev = 0, nSlide = 0;
    for (const MechTreeJ& t : m_mm.tree) (t.j.type == 1 ? nSlide : nRev)++;
    for (const MechJoint& j : m_mm.closures) (j.type == 1 ? nSlide : nRev)++;
    char line[200];
    snprintf(line, sizeof(line), "%d corps (%d mobiles) · %d pivots · %d vérins", m_mm.bodyCount, m_mm.bodyCount - (int)m_mm.roots.size(), nRev, nSlide);
    u.text(ImVec2(x, y), 11.5f, C_DIM, line);
    snprintf(line, sizeof(line), "%d engrenages · %d boucles · %d DDL pour %d commandes", (int)m_mm.meshes.size(), (int)m_mm.closures.size(), m_mm.dof, (int)m_mm.cmds.size());
    u.text(ImVec2(x, y + 20 * s), 11.5f, C_DIM, line);
    y += 44 * s;
    for (const std::string& w : m_mm.warnings) { u.text(ImVec2(x, y), 12, IM_COL32(0xff, 0x8a, 0x80, 255), w.c_str()); y += 18 * s; }
    if (m_mm.cmds.empty()) {
        u.text(ImVec2(x, y), 12, C_DIM, "Aucune commande : ajoute un vérin ou un train");
        u.text(ImVec2(x, y + 16 * s), 12, C_DIM, "d'engrenages, ou attrape une pièce mobile.");
        y += 34 * s;
    }
    ImGuiIO& io = ImGui::GetIO();
    for (size_t k = 0; k < m_mm.cmds.size(); ++k) {
        MechCmd& c = m_mm.cmds[k];
        u.dl->AddLine(ImVec2(x, y), ImVec2(p1.x - pad, y), C_LINE, 1.0f);
        y += 9 * s;
        ImVec2 ns = u.text(ImVec2(x, y), 12.5f, C_TEXT, c.name.c_str(), true);
        std::string ratio;
        for (auto& r : c.ratio) {
            char b[24];
            float v = std::fabs(1.0f / r.second);
            snprintf(b, sizeof(b), "%s1:%.1f", ratio.empty() ? "" : ", ", v);
            std::string t(b);
            size_t z = t.find(".0");
            if (z != std::string::npos && z + 2 == t.size()) t.erase(z);
            ratio += t;
        }
        if (!ratio.empty()) u.text(ImVec2(x + ns.x + 6 * s, y + 1 * s), 11.5f, C_FAINT, ratio.c_str());
        if (c.type == 1) snprintf(line, sizeof(line), "%.1f cm", c.value);
        else snprintf(line, sizeof(line), "%d°", (int)std::lround(c.value * 180.0f / 3.14159265f));
        float vw = u.textW(12, line);
        u.text(ImVec2(p1.x - pad - vw, y), 12, C_ACCENT_TXT, line);
        y += 20 * s;
        // range -720..720 degrees, accent colour
        ImVec2 ra(x, y + 6 * s), rb(p1.x - pad, y + 10 * s);
        u.dl->AddRectFilled(ra, rb, C_CARD_HI, 2 * s);
        float deg = std::clamp(c.value * 180.0f / 3.14159265f, -720.0f, 720.0f), t = (deg + 720.0f) / 1440.0f;
        if (c.type == 1) t = c.hi > c.lo ? std::clamp((c.value - c.lo) / (c.hi - c.lo), 0.0f, 1.0f) : 0.5f;
        float kx = ra.x + (rb.x - ra.x) * t;
        u.dl->AddRectFilled(ra, ImVec2(kx, rb.y), C_ACCENT, 2 * s);
        u.dl->AddCircleFilled(ImVec2(kx, (ra.y + rb.y) * 0.5f), 7 * s, C_ACCENT, 20);
        static int dragCmd = -1;
        if (u.hover(ImVec2(ra.x, y), ImVec2(rb.x, y + 16 * s)) && u.pressed) dragCmd = (int)k;
        if (dragCmd == (int)k) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) dragCmd = -1;
            else if (c.type == 1) c.value = c.lo + (c.hi - c.lo) * std::clamp((u.mouse.x - ra.x) / (rb.x - ra.x), 0.0f, 1.0f);
            else c.value = (-720.0f + 1440.0f * std::clamp((u.mouse.x - ra.x) / (rb.x - ra.x), 0.0f, 1.0f)) * 3.14159265f / 180.0f;
        }
        y += 20 * s;
        // keys: ◀ Q / A ▶ (hold buttons)
        auto hold = [&](const char* glyph, float hx, int dir) {
            ImVec2 a(hx, y), b(hx + 34 * s, y + 28 * s);
            bool h = u.hover(a, b), down = h && ImGui::IsMouseDown(ImGuiMouseButton_Left);
            u.dl->AddRectFilled(a, b, down ? C_ACCENT : C_CARD, 7 * s);
            (void)glyph;   // ◀ / ▶ drawn as triangles (the UI font has no such glyphs)
            ImVec2 m((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
            const float t = 5 * s;
            if (dir < 0) u.dl->AddTriangleFilled(ImVec2(m.x - t, m.y), ImVec2(m.x + t * 0.8f, m.y - t), ImVec2(m.x + t * 0.8f, m.y + t), down ? C_INK : C_TEXT);
            else u.dl->AddTriangleFilled(ImVec2(m.x + t, m.y), ImVec2(m.x - t * 0.8f, m.y - t), ImVec2(m.x - t * 0.8f, m.y + t), down ? C_INK : C_TEXT);
            if (down) c.value += dir * c.speed * io.DeltaTime;
        };
        hold("◀", x, -1);
        float kx2 = x + 40 * s;
        auto kbd = [&](const char* t) {
            float w = u.textW(11.5f, t) + 12 * s;
            u.dl->AddRectFilled(ImVec2(kx2, y + 5 * s), ImVec2(kx2 + w, y + 23 * s), C_CARD, 4 * s);
            u.text(ImVec2(kx2 + 6 * s, y + 7 * s), 11.5f, C_TEXT, t);
            kx2 += w + 4 * s;
        };
        kbd(c.keyMinus ? ImGui::GetKeyName((ImGuiKey)c.keyMinus) : "");
        u.text(ImVec2(kx2, y + 6 * s), 12, C_DIM, "/");
        kx2 += 10 * s;
        kbd(c.keyPlus ? ImGui::GetKeyName((ImGuiKey)c.keyPlus) : "");
        hold("▶", kx2 + 2 * s, 1);
        y += 36 * s;
    }
    u.text(ImVec2(x, y + 4 * s), 10.5f, C_DIM, "Glisse une pièce mobile : tout ce qui y est relié suit.");
    if (!m_mechFlash.empty() && ImGui::GetTime() - m_mechFlashTime < 0.9) {
        ImVec2 a(x, y + 24 * s), b(p1.x - pad, y + 50 * s);
        u.dl->AddRectFilled(a, b, IM_COL32(229, 72, 77, 46), 8 * s);
        u.text(ImVec2(a.x + 8 * s, a.y + 6 * s), 12, IM_COL32(0xff, 0xb3, 0xb5, 255), m_mechFlash.c_str());
    }
}

// ------------------------------------------------------------------ HUD (Fenêtres > Infos), part sheet, popups, toast

void BrickApp::uiHud() {
    if (!m_pHud.visible) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    char l1[160], l2[200];
    snprintf(l1, sizeof(l1), "%d pièces · %d fps · GPU %.2f ms", (int)m_bricks.size(), (int)std::lround(m_frameMs > 0 ? 1000.0 / m_frameMs : 0.0), m_renderer->gpuFrameMs());
    const char* st = m_mechOn ? "Mode Mécanique : touches du panneau ou glisse une pièce mobile"
                   : m_holding ? (m_held.suggested ? "Pivot voisin valide proposé" : m_held.blocked ? "Placement refusé (> 50 %)" : "Brique en main")
                   : m_selected >= 0 ? m_lib.parts[m_bricks[m_selected].part].label.c_str() : "Touche une brique pour la prendre";
    snprintf(l2, sizeof(l2), "%s", st);
    float w = std::max(u.textW(11, l1), u.textW(11, l2)) + 16 * s;
    ImVec2 a(10 * s, 30 * s + 8 * s), b(a.x + w, a.y + 40 * s);
    u.dl->AddRectFilled(a, b, C_HUD, 10 * s);
    u.text(ImVec2(a.x + 8 * s, a.y + 4 * s), 11, C_DIM, l1);
    u.text(ImVec2(a.x + 8 * s, a.y + 20 * s), 11, C_DIM, l2);
}

// Part sheet of the normal mode (web: showPartSimple): turntable 3D view ("Glisse pour tourner la pièce"), name,
// family › category, ☆ Favori, Fermer, Ajouter à la scène. Box min(440 px, 92 vw), padding 18, gap 8.
void BrickApp::uiInfoSheet() {
    if (m_infoPart < 0) return;
    if (m_dev) { uiDevSheet(); return; }   // full sheet (metadata, connectors, Maya export / import)
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(0, 0, 0, 128));   // .sheet rgba(0, 0, 0, .5)
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const Part& p = m_lib.parts[m_infoPart];
    if (m_pvPart != m_infoPart) { m_pvPart = m_infoPart; m_pvYaw = m_pvPitch = 0; }
    const float W = std::min(440 * s, d.x * 0.92f), pad = 18 * s, gap = 8 * s;
    const float vw = W - 2 * pad, vh = vw * 4 / 5;   // canvas aspect 5 / 4
    const float H = pad + vh + gap + 4 * s + 24 * s + gap + 17 * s + gap + 28 * s + pad;
    ImVec2 p0((d.x - W) * 0.5f, (d.y - H) * 0.5f), p1(p0.x + W, p0.y + H);
    u.dl->AddRectFilled(p0, p1, C_PANEL_SOLID, rr(14 * s));
    u.dl->AddRect(p0, p1, C_LINE, rr(14 * s));
    // viewer
    ImVec2 va(p0.x + pad, p0.y + pad), vb(va.x + vw, va.y + vh);
    u.dl->AddRectFilled(va, vb, C_CARD, 12 * s);
    m_pvRect = ImVec4(va.x, va.y, vb.x, vb.y);
    if (m_frozenTex) u.dl->AddImageRounded(m_gui->viewportTexture(), va, vb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 12 * s);
    u.text(ImVec2(va.x + 10 * s, vb.y - 8 * s - 13 * s), 10.5f, C_FAINT, "Glisse pour tourner la pièce");
    ImGuiIO& io = ImGui::GetIO();
    const bool overView = u.hover(va, vb);
    if (overView) ImGui::SetMouseCursor(m_pvDrag ? ImGuiMouseCursor_ResizeAll : ImGuiMouseCursor_Hand);   // cursor: grab
    if (overView && u.pressed) m_pvDrag = true;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_pvDrag = false;
    if (m_pvDrag) {
        m_pvYaw += io.MouseDelta.x * 0.012f;
        m_pvPitch = std::clamp(m_pvPitch + io.MouseDelta.y * 0.01f, -1.4f, 1.4f);
    }
    // name, family › category
    float y = vb.y + gap + 4 * s;
    u.text(ImVec2(p0.x + pad, y), 18, C_TEXT, p.label.c_str(), true);
    y += 24 * s + gap;
    const std::string cat = partCat(m_infoPart);
    const char* fam = "Autres gammes";
    for (const Fam& f : families())
        for (const char* c : f.cats)
            if (cat == c) fam = f.name;
    std::string path = std::string(fam) + " › " + catFr(cat);
    u.text(ImVec2(p0.x + pad, y), 12.5f, C_DIM, path.c_str());
    y += 17 * s + gap;
    // row: ☆ Favori · grow · Fermer · Ajouter à la scène (.btn compact: 28 px, 12 px, padding 10, radius 7)
    auto button = [&](float x, const char* label, bool accent, ImU32 textCol, bool& clicked) {
        float w = u.textW(12, label) + 20 * s;
        ImVec2 a(x, y), b(x + w, y + 28 * s);
        bool h = u.hover(a, b);
        u.dl->AddRectFilled(a, b, accent ? C_ACCENT : (h ? C_CARD_HI : C_CARD), 7 * s);
        u.text(ImVec2(a.x + 10 * s, a.y + 7 * s), 12, accent ? C_INK : textCol, label, accent);
        clicked = h && u.pressed;
        return w;
    };
    const bool fav = std::find(m_fav.begin(), m_fav.end(), m_infoPart) != m_fav.end();
    bool cFav = false, cClose = false, cAdd = false;
    button(p0.x + pad, fav ? "★ Favori" : "☆ Favori", false, fav ? C_ACCENT_TXT : C_TEXT, cFav);
    const float addW = u.textW(12, "Ajouter à la scène") + 20 * s, closeW = u.textW(12, "Fermer") + 20 * s;
    button(p1.x - pad - addW, "Ajouter à la scène", true, C_INK, cAdd);
    button(p1.x - pad - addW - gap - closeW, "Fermer", false, C_TEXT, cClose);
    if (cFav) {
        if (fav) m_fav.erase(std::find(m_fav.begin(), m_fav.end(), m_infoPart));
        else m_fav.push_back(m_infoPart);
        savePrefs();
    }
    const int part = m_infoPart;
    if (cClose || ImGui::IsKeyPressed(ImGuiKey_Escape) || (u.pressed && !u.hover(p0, p1))) m_infoPart = -1;
    if (cAdd) {
        m_infoPart = -1;
        ImVec2 c((m_stage.x + m_stage.z) * 0.5f, (m_stage.y + m_stage.w) * 0.5f);
        spawn(part, c.x, c.y);
    }
}

void BrickApp::uiPopups() {
    if (!m_popup || m_popup == 7 || m_popup == 8) return;   // (7: « … » of the Studio interface, stMore; 8: its scene menu, stFile)
    if (m_popup == 5) { uiSettings(); return; }
    if (m_popup == 9) { stCloudList(); return; }   // Scène › Scènes du cloud
    Ui u = ui(m_ui);
    const float s = m_ui;
    struct Item { const char* label; bool on; int id; };
    std::vector<Item> items;
    if (m_popup == 1) items = {{"Rapide", m_rs.raster && m_quality == 0, 0}, {"HQ", m_rs.raster && m_quality == 1, 1}, {"Path tracing", pathTraced(), 2}, {"Radiance Cascades", m_rcMode, 3}};
    else if (m_popup == 2) items = {{"ACES", m_rs.tonemapper == 0, 0}, {"AgX", m_rs.tonemapper == 1, 1}, {"Std", m_rs.tonemapper == 3, 3}};
    else if (m_popup == 3) items = {{"Bibliothèque", m_pTray.visible, 0}, {"Manuel d'instructions", m_pManual.visible, 4}, {"Infos (pièces, fps)", m_pHud.visible, 3},
                                    {"Couleurs", m_pColors.visible, 2}, {"Actions de la brique", m_pOpts.visible, 1}, {"Groupes", m_pGroups.visible, 6},
                                    {"Réinitialiser la disposition", false, 5}};
    else if (m_ipad) {   // the iPad: the scene in a few big words (files in « Sur mon iPad › BrickIt »)
        items = {{"Nouvelle scène", false, 0}, {"Ouvrir une scène…", false, 1}, {"Enregistrer", false, 2}, {"Exporter en LDraw (.ldr)", false, 6},
                 {"Ouvrir la maison officielle (31153)", false, 5}};
        if (hasSteps()) items.push_back({m_reading ? "Fermer la notice" : "Lire la notice (étapes du modèle)", false, 4});
    } else {
        items = {{"Nouvelle scène", false, 0}, {"Ouvrir… (.v3d.json, .ldr, .mpd)", false, 1}, {"Enregistrer… (.v3d.json)", false, 2}, {"Scènes du cloud…", false, 8}, {"Ouvrir la maison test (GI)", false, 3},
                 {"Ouvrir la maison officielle (31153)", false, 5}};
        if (hasSteps()) items.push_back({m_reading ? "Fermer la notice" : "Lire la notice (étapes du modèle)", false, 4});
    }
    const bool checks = m_popup == 3;
    float w = 0;
    for (const Item& it : items) w = std::max(w, u.textW(12.5f, it.label));
    w += (checks ? 44 : 28) * s;
    w = std::max(w, (m_popup >= 3 ? 210.0f : 90.0f) * s);
    ImVec2 d = ImGui::GetIO().DisplaySize;
    ImVec2 p0(std::min(m_popupAt.x, d.x - w - 10 * s), m_popupAt.y), p1(p0.x + w, p0.y + 12 * s + items.size() * 32 * s);
    u.dl->AddRectFilled(p0, p1, C_PANEL_SOLID, 10 * s);
    u.dl->AddRect(p0, p1, C_LINE, 10 * s);
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    float y = p0.y + 6 * s;
    bool clickedItem = false;
    for (const Item& it : items) {
        ImVec2 a(p0.x + 6 * s, y), b(p1.x - 6 * s, y + 30 * s);
        bool h = u.hover(a, b);
        if (h) u.dl->AddRectFilled(a, b, C_CARD_HI, 7 * s);
        float tx = a.x + 10 * s;
        if (checks && it.id != 5) {
            ImVec2 ca(tx, a.y + 8 * s), cb(tx + 14 * s, a.y + 22 * s);
            if (it.on) {
                u.dl->AddRectFilled(ca, cb, C_ACCENT, 4 * s);
                u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(ca.x + 3.5f * s, ca.y + 7 * s), ImVec2(ca.x + 6 * s, ca.y + 10 * s), ImVec2(ca.x + 11 * s, ca.y + 4 * s)}.data(), 3, C_INK, 0, 2 * s);
            } else u.dl->AddRect(ca, cb, C_FAINT, 4 * s, 0, 1.5f * s);
            tx += 24 * s;
        }
        u.text(ImVec2(tx, a.y + 7 * s), 12.5f, (it.on && !checks) ? C_ACCENT_TXT : C_TEXT, it.label);
        if (h && u.pressed) {
            clickedItem = true;
            if (m_popup == 1) {
                if (it.id == 3) { setRC(true); applyLook(); }
                else if (it.id == 2) { setRC(false); m_rs.raster = false; applyLook(); }
                else { setRC(false); m_quality = it.id; m_rs.msaaSamples = m_quality ? 4 : 1; if (!m_rs.raster) { m_rs.raster = true; applyLook(); } }
            }
            else if (m_popup == 2) { m_rs.tonemapper = it.id; m_renderer->resetAccumulation(); }
            else if (m_popup == 3) {
                PanelState* ps[5] = {&m_pTray, &m_pOpts, &m_pColors, &m_pHud, &m_pManual};
                if (it.id < 4) ps[it.id]->visible = !ps[it.id]->visible;
                else if (it.id == 4) { m_pManual.visible = !m_pManual.visible; if (m_pManual.visible) m_pManual.collapsed = false; }
                else if (it.id == 6) m_pGroups.visible = !m_pGroups.visible;
                else { m_pTray = m_pOpts = m_pColors = PanelState(); m_pHud = m_pManual = PanelState{false, false}; m_optsRight = false; toast("Disposition réinitialisée"); }
            } else {
                if (it.id == 0) { pushUndo(); m_bricks.clear(); m_selected = -1; m_multi.clear(); cloudForget(); rebuildScene(); toast("Nouvelle scène"); }
                else if (it.id == 8) { clickedItem = false; m_popup = 9; cloudRefreshList(); }
                else if (it.id == 1) openDialog();
                else if (it.id == 3) openTestHouse();
                else if (it.id == 4) { if (m_reading) stopReading(); else startReading(1); }
                else if (it.id == 5) openOfficialHouse();
                else if (it.id == 6) exportLdrDialog();
                else saveDialog();
            }
        }
        y += 32 * s;
    }
    if (u.pressed && !clickedItem && !u.hover(p0, p1) && u.mouse.y > 30 * s) m_popup = 0;
    if (clickedItem && m_popup != 3 && m_popup != 9) m_popup = 0;
}

// « Scène partagée PC · Mac · iPad » (#sheet): the .v3d.json text, copy, load, open, exports, new scene.
void BrickApp::uiSceneSheet() {
    if (!m_sheetOpen) return;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    ImGui::GetForegroundDrawList();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(d);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
    ImGui::Begin("##sheetbg", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
    ImGui::End();
    ImGui::PopStyleColor();
    const float W = std::min(640 * s, d.x - 32 * s);
    ImGui::SetNextWindowPos(ImVec2((d.x - W) * 0.5f, d.y * 0.5f - 190 * s));
    ImGui::SetNextWindowSize(ImVec2(W, 0));
    ImGui::SetNextWindowFocus();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18 * s, 18 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14 * s);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * s, 12 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7 * s);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * s, 6 * s));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(C_PANEL_SOLID));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(C_LINE));
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(C_CARD));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(C_CARD_HI));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0x0f / 255.0f, 0x10 / 255.0f, 0x12 / 255.0f, 1));
    ImGui::Begin("##sheet", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    if (m_gui->fontBold) ImGui::PushFont(m_gui->fontBold);
    ImGui::TextUnformatted("Scène partagée PC · Mac · iPad");
    if (m_gui->fontBold) ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C_DIM));
    ImGui::TextWrapped("Même format que l'application bureau (.v3d.json). Copie ce texte dans un fichier pour l'ouvrir sur PC (Ctrl+O) et lancer le rendu path tracing, ou colle ici une scène exportée du PC.");
    ImGui::PopStyleColor();
    static std::vector<char> buf;
    if (buf.size() < m_sheetText.size() + 65536) buf.assign(m_sheetText.size() + 65536, 0);
    if (ImGui::IsWindowAppearing() || std::strncmp(buf.data(), m_sheetText.c_str(), buf.size()) != 0) {
        std::memcpy(buf.data(), m_sheetText.c_str(), m_sheetText.size() + 1);
    }
    if (ImGui::InputTextMultiline("##scenetext", buf.data(), buf.size(), ImVec2(-1, 180 * s))) m_sheetText = buf.data();
    if (ImGui::Button("Copier")) { ImGui::SetClipboardText(m_sheetText.c_str()); toast("Scène copiée dans le presse-papiers"); }
    ImGui::SameLine();
    if (ImGui::Button("Charger ce texte")) {
        std::string msg;
        std::vector<Brick> before = m_bricks;
        if (m_build.fromJson(m_sheetText, &m_scene.camera, &msg)) { m_undo.push_back(before); m_selected = -1; m_multi.clear(); rebuildScene(); m_sheetOpen = false; }
        toast(msg);
    }
    ImGui::SameLine();
    if (ImGui::Button("Ouvrir un fichier…")) { openDialog(); m_sheetText = m_build.toJson(m_scene.camera); }
    ImGui::SameLine();
    float right = 0;
    const char* rb[4] = {"Exporter .ldr", "Exporter vers Maya", "Nouvelle scène", "Fermer"};
    for (const char* t : rb) right += ImGui::CalcTextSize(t).x + 20 * s + 8 * s;
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - right + 8 * s));
    if (ImGui::Button("Exporter .ldr")) { m_sheetText = sceneLdr(); exportLdrDialog(); }
    ImGui::SameLine();
    if (ImGui::Button("Exporter vers Maya")) mayaExportScene();
    ImGui::SameLine();
    if (ImGui::Button("Nouvelle scène")) { pushUndo(); m_bricks.clear(); m_selected = -1; m_multi.clear(); rebuildScene(); m_sheetText = m_build.toJson(m_scene.camera); toast("Nouvelle scène"); }
    ImGui::SameLine();
    if (ImGui::Button("Fermer") || ImGui::IsKeyPressed(ImGuiKey_Escape)) m_sheetOpen = false;
    ImGui::End();
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(5);
    m_overUi = true;
}

void BrickApp::exportLdrDialog() {
    plat::saveFileDialog(L"LDraw (*.ldr)\0*.ldr\0", L"ldr", L"brickit_scene.ldr", [this](const std::wstring& file) {
        const std::string text = sceneLdr();
        FILE* f = nullptr;
        if (file.empty() || _wfopen_s(&f, file.c_str(), L"wb") != 0 || !f) return;
        fwrite(text.data(), 1, text.size(), f);
        fclose(f);
        char t[96];
        snprintf(t, sizeof(t), "Scène exportée en .ldr (%d pièces)", (int)m_bricks.size());
        toast(t);
    });
}

void BrickApp::drawToast() {
    // .toast: top 8 px of the view, round, dark, thin yellow border
    if (m_ipad) { ipadMessage(); return; }
    if (m_toast.empty() || ImGui::GetTime() - m_toastTime > 3.2) return;
    const float s = m_ui;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* f = ImGui::GetFont();
    ImVec2 ts = f->CalcTextSizeA(12 * s, 1e9f, 0, m_toast.c_str());
    ImVec2 d = ImGui::GetIO().DisplaySize;
    float a = (float)std::clamp(3.2 - (ImGui::GetTime() - m_toastTime), 0.0, 0.4) / 0.4f;
    ImVec2 p0(d.x * 0.5f - ts.x * 0.5f - 12 * s, m_vePart >= 0 ? d.y - 190 * s : (m_ipad ? 20 * s : 30 * s + 8 * s)), p1(d.x * 0.5f + ts.x * 0.5f + 12 * s, p0.y + ts.y + 10 * s);
    dl->AddRectFilled(p0, p1, alphaOf(IM_COL32(15, 16, 19, 224), a), 18 * s);
    dl->AddRect(p0, p1, alphaOf(IM_COL32(247, 197, 30, 115), a), 18 * s);
    dl->AddText(f, 12 * s, ImVec2(p0.x + 12 * s, p0.y + 5 * s), alphaOf(C_TEXT, a), m_toast.c_str());
}

// ------------------------------------------------------------------ frame of the interface

// ------------------------------------------------------------------ groups (outliner)
// The tree of the groups: a row per group (unfold, name, number of pieces, shown / hidden, isolated), a click selects
// the group, a double click on its name renames it. Affichage › Groupes, or the panel opens on the first Ctrl+G.
void BrickApp::uiGroups() {
    if (!m_pGroups.visible) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const int G = (int)m_build.groups.size();
    // parent of each group and its number of pieces, from the paths of the bricks
    std::vector<int> parent(G, -2), count(G, 0);
    int loose = 0;
    for (const Brick& b : m_bricks) {
        if (b.path.empty()) ++loose;
        for (size_t k = 0; k < b.path.size(); ++k) {
            const int g = b.path[k];
            if (g < 0 || g >= G) continue;
            ++count[g];
            if (parent[g] == -2) parent[g] = k ? b.path[k - 1] : -1;
        }
    }
    struct Row { int g, depth; };
    std::vector<Row> rows;
    std::function<void(int, int)> add = [&](int par, int depth) {
        for (int g = 0; g < G; ++g) {
            if (parent[g] != par || count[g] == 0) continue;
            rows.push_back({g, depth});
            if (std::find(m_groupsOpen.begin(), m_groupsOpen.end(), g) != m_groupsOpen.end()) add(g, depth + 1);
        }
    };
    add(-1, 0);
    const float W = 270 * s, pad = 10 * s, rowH = 24 * s;
    const int shown = std::min((int)rows.size(), 30);
    const float H = pad + 20 * s + 6 * s + (shown ? shown * rowH : 18 * s) + 6 * s + 16 * s + pad;
    ImVec2 a(10 * s, (m_pHud.visible ? 86 : 44) * s), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(a, b, C_PANEL_SOLID, 10 * s);
    u.dl->AddRect(a, b, C_LINE, 10 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    u.text(ImVec2(a.x + pad, a.y + pad - 2 * s), 13, C_TEXT, "Groupes", true);
    if (m_isolateGroup >= 0) {   // "Tout afficher" while a group is isolated
        const char* lab = "Tout afficher";
        const float tw = u.textW(11, lab);
        ImVec2 ba(b.x - pad - tw - 12 * s, a.y + pad - 3 * s), bb(b.x - pad, a.y + pad + 15 * s);
        const bool h = u.hover(ba, bb);
        u.dl->AddRectFilled(ba, bb, h ? C_CARD_HI : C_CARD, 6 * s);
        u.text(ImVec2(ba.x + 6 * s, ba.y + 2 * s), 11, C_TEXT, lab);
        if (h && u.pressed) { m_isolateGroup = -1; syncObjects(); }
    }
    float y = a.y + pad + 20 * s + 6 * s;
    if (rows.empty()) {
        u.text(ImVec2(a.x + pad, y), 11, C_DIM, "Aucun groupe : sélectionne des pièces puis Ctrl+G");
        y += 18 * s;
    }
    const int sel = unitGroup(m_selected);
    for (int r = 0; r < shown; ++r) {
        const Row& row = rows[r];
        const int g = row.g;
        Group& gr = m_build.groups[g];
        ImVec2 ra(a.x + 4 * s, y), rb(b.x - 4 * s, y + rowH - 2 * s);
        const bool h = u.hover(ra, rb);
        if (g == sel) u.dl->AddRectFilled(ra, rb, C_ACCENT, 6 * s);
        else if (h) u.dl->AddRectFilled(ra, rb, C_CARD_HI, 6 * s);
        float x = ra.x + 6 * s + row.depth * 14 * s;
        // unfold chevron (when it has sub-groups)
        bool hasKids = false;
        for (int k = 0; k < G; ++k) if (parent[k] == g && count[k] > 0) hasKids = true;
        const bool open = std::find(m_groupsOpen.begin(), m_groupsOpen.end(), g) != m_groupsOpen.end();
        ImVec2 ca(x, y + 4 * s), cb(x + 14 * s, y + rowH - 6 * s);
        if (hasKids) {
            const ImVec2 c((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f);
            const float k = 3.5f * s;
            if (open) u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - k, c.y - k * 0.5f), ImVec2(c.x, c.y + k * 0.5f), ImVec2(c.x + k, c.y - k * 0.5f)}.data(), 3, C_DIM, 0, 1.8f * s);
            else u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - k * 0.5f, c.y - k), ImVec2(c.x + k * 0.5f, c.y), ImVec2(c.x - k * 0.5f, c.y + k)}.data(), 3, C_DIM, 0, 1.8f * s);
        }
        const bool overChev = hasKids && u.hover(ca, cb);
        x += 18 * s;
        char nm[96];
        snprintf(nm, sizeof(nm), "%s", gr.name.c_str());
        u.text(ImVec2(x, y + 3 * s), 12, gr.hidden ? C_FAINT : (g == sel ? C_INK : C_TEXT), nm, g == sel);
        char cn[24];
        snprintf(cn, sizeof(cn), "%d", count[g]);
        // right: number of pieces, shown / hidden (eye), isolate
        const float bx2 = rb.x - 4 * s;
        ImVec2 ia(bx2 - 20 * s, y + 2 * s), ib(bx2, y + rowH - 4 * s);            // isolate
        ImVec2 ea(ia.x - 24 * s, y + 2 * s), eb(ia.x - 4 * s, y + rowH - 4 * s);   // eye
        u.text(ImVec2(ea.x - 6 * s - u.textW(11, cn), y + 4 * s), 11, g == sel ? C_INK : C_DIM, cn);
        const bool he = u.hover(ea, eb), hi = u.hover(ia, ib);
        {   // eye: an almond with its pupil, struck through when hidden
            const ImVec2 c((ea.x + eb.x) * 0.5f, (ea.y + eb.y) * 0.5f);
            const ImU32 col = he ? C_TEXT : (g == sel ? C_INK : C_DIM);
            u.dl->AddEllipse(c, ImVec2(7 * s, 4 * s), col, 0, 0, 1.4f * s);
            if (!gr.hidden) u.dl->AddCircleFilled(c, 2.2f * s, col);
            else u.dl->AddLine(ImVec2(c.x - 7 * s, c.y + 5 * s), ImVec2(c.x + 7 * s, c.y - 5 * s), col, 1.4f * s);
        }
        {   // isolate: a target, filled when this group is the one isolated
            const ImVec2 c((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f);
            const ImU32 col = m_isolateGroup == g ? C_ACCENT_TXT : (hi ? C_TEXT : (g == sel ? C_INK : C_DIM));
            u.dl->AddCircle(c, 5.5f * s, col, 0, 1.4f * s);
            if (m_isolateGroup == g) u.dl->AddCircleFilled(c, 2.5f * s, col);
        }
        if (h && u.pressed) {
            if (he) { gr.hidden = !gr.hidden; syncObjects(); autosave(); }
            else if (hi) { m_isolateGroup = m_isolateGroup == g ? -1 : g; syncObjects(); }
            else if (overChev) {
                auto it = std::find(m_groupsOpen.begin(), m_groupsOpen.end(), g);
                if (it != m_groupsOpen.end()) m_groupsOpen.erase(it); else m_groupsOpen.push_back(g);
            } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_renameGroup = g;
                snprintf(m_renameBuf, sizeof(m_renameBuf), "%s", gr.name.c_str());
            } else {   // select the group: the level of its parent is entered
                std::vector<int> m = groupMembers(g);
                if (!m.empty()) {
                    const std::vector<int>& p = m_bricks[m[0]].path;
                    const auto at = std::find(p.begin(), p.end(), g);
                    m_groupCtx.assign(p.begin(), at);
                    m_selected = m[0];
                    m_multi.assign(m.begin() + 1, m.end());
                    syncObjects();
                }
            }
        }
        if (m_renameGroup == g) {   // rename: a text field over the row, Entrée to keep, Échap to cancel
            ImGui::SetNextWindowPos(ImVec2(x - 4 * s, y));
            ImGui::SetNextWindowSize(ImVec2(rb.x - x - 70 * s, rowH));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2 * s, 1 * s));
            ImGui::Begin("##renamegroup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##name", m_renameBuf, sizeof(m_renameBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                if (m_renameBuf[0]) gr.name = m_renameBuf;
                m_renameGroup = -1;
                autosave();
            } else if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (!ImGui::IsWindowFocused() && !ImGui::IsWindowAppearing())) m_renameGroup = -1;
            ImGui::End();
            ImGui::PopStyleVar();
        }
        y += rowH;
    }
    char lb[96];
    snprintf(lb, sizeof(lb), "%d pièces hors groupe · Ctrl+G grouper · Ctrl+Maj+G dégrouper", loose);
    u.text(ImVec2(a.x + pad, b.y - pad - 13 * s), 9.5f, C_FAINT, lb);
}

// ------------------------------------------------------------------ reading the instructions of a model
// The page of the booklet over the 3D view: model, step n / N, the submodel built, previous / next, close; under it
// the parts of the step (thumbnail, colour, x count), as the boxes of the official booklets.
void BrickApp::uiReader() {
    if (!m_reading) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // the parts of the step, grouped by part and colour
    std::map<std::pair<int, int>, int> cnt;
    for (int i : readStepBricks()) ++cnt[{m_bricks[i].part, m_bricks[i].color}];
    std::vector<std::pair<std::pair<int, int>, int>> items(cnt.begin(), cnt.end());
    std::sort(items.begin(), items.end(), [](auto& a, auto& b) { return a.second > b.second; });
    const float cell = 58 * s, gap = 6 * s, pad = 12 * s, headH = 30 * s;
    const int maxCells = std::max(1, (int)((std::min(980 * s, d.x - 40 * s) - 2 * pad + gap) / (cell + gap)));
    const int shown = std::min((int)items.size(), maxCells);
    const float W = std::min(980 * s, d.x - 40 * s), H = pad + headH + gap + cell + 16 * s + pad;
    ImVec2 a((d.x - W) * 0.5f, 44 * s), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(a, b, C_PANEL_SOLID, 12 * s);
    u.dl->AddRect(a, b, C_LINE, 12 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float x = a.x + pad;
    const float y = a.y + pad;
    x += u.text(ImVec2(x, y + 4 * s), 14, C_TEXT, m_modelName.empty() ? "Notice" : m_modelName.c_str(), true).x + 14 * s;
    char st[64];
    snprintf(st, sizeof(st), "Étape %d / %d", m_readStep, m_readSteps);
    x += u.text(ImVec2(x, y + 4 * s), 14, C_ACCENT_TXT, st, true).x + 14 * s;
    const std::string& sub = m_readStep - 1 < (int)m_readTitles.size() ? m_readTitles[m_readStep - 1] : std::string();
    if (!sub.empty()) {
        const std::string t = "Sous-modèle : " + sub;
        u.text(ImVec2(x, y + 6 * s), 12, C_DIM, t.c_str());
    }
    // buttons: previous, next, close
    auto button = [&](float bx, const char* lab, bool enabled) {
        const float tw = u.textW(12, lab, true);
        ImVec2 p0(bx - tw - 22 * s, y), p1(bx, y + headH);
        const bool h = enabled && u.hover(p0, p1);
        u.dl->AddRectFilled(p0, p1, h ? C_ACCENT : C_CARD, 8 * s);
        u.text(ImVec2(p0.x + 11 * s, y + (headH - u.fs(12)) * 0.5f), 12, h ? C_INK : (enabled ? C_TEXT : C_FAINT), lab, true);
        return std::pair<bool, float>(h && u.pressed, p0.x - 6 * s);
    };
    auto [close, x1] = button(b.x - pad, "Fermer", true);
    auto [next, x2] = button(x1, "Suivante  ›", m_readStep < m_readSteps);
    auto [prev, x3] = button(x2, "‹  Précédente", m_readStep > 1);
    (void)x3;
    if (close) stopReading();
    if (next) readStepTo(m_readStep + 1);
    if (prev) readStepTo(m_readStep - 1);
    // the parts of the step
    float cx = a.x + pad;
    const float cy = y + headH + gap;
    for (int k = 0; k < shown; ++k) {
        const int part = items[k].first.first, col = items[k].first.second;
        ImVec2 c0(cx, cy), c1(cx + cell, cy + cell);
        u.dl->AddRectFilled(c0, c1, C_CARD, 8 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(part, tex, uv0, uv1)) u.dl->AddImage(tex, ImVec2(c0.x + 3 * s, c0.y + 3 * s), ImVec2(c1.x - 3 * s, c1.y - 3 * s), uv0, uv1);
        const Color& cc = m_lib.palette[std::clamp(col, 0, (int)m_lib.palette.size() - 1)];
        u.dl->AddCircleFilled(ImVec2(c0.x + 9 * s, c1.y - 9 * s), 5 * s, IM_COL32((int)(cc.srgb.x * 255), (int)(cc.srgb.y * 255), (int)(cc.srgb.z * 255), 255));
        u.dl->AddCircle(ImVec2(c0.x + 9 * s, c1.y - 9 * s), 5 * s, IM_COL32(255, 255, 255, 60), 0, 1 * s);
        char nb[16];
        snprintf(nb, sizeof(nb), "%d×", items[k].second);
        u.text(ImVec2(c0.x + 4 * s, c1.y + 1 * s), 10.5f, C_TEXT, nb, true);
        if (u.hover(c0, c1)) ImGui::SetTooltip("%s · %s", m_lib.parts[part].label.c_str(), cc.name.c_str());
        cx += cell + gap;
    }
    if ((int)items.size() > shown) {
        char more[32];
        snprintf(more, sizeof(more), "+ %d", (int)items.size() - shown);
        u.text(ImVec2(cx, cy + cell * 0.4f), 12, C_DIM, more);
    }
}

void BrickApp::drawUi() {
    m_lazyThumbBudget = 12;   // thumbnails of the full library decoded this frame at most
    ImVec2 d = ImGui::GetIO().DisplaySize;
    m_uiRects.clear();
    // one window covering the screen, behind every ImGui window, holding the draw list of the panels
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(d);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("##webui", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    const bool creme = m_ipad && m_ipadCreme;   // iPad: the cream touch kit (Thème › Crème iPad), else the Studio interface
    if (creme) uiTheme(true);
    else if (applyTheme()) applyLook();   // Réglages › Thème: the colours, the fonts; the 3D stage follows
    if (m_ipad && m_ipadUiWas != (int)creme) { m_ipadUiWas = (int)creme; applyLook(); }
    if (creme) {
        drawUiIpad();
        uiCtxMenu();
        drawDropzone();
        ImGui::End();
        ImGui::PopStyleVar(2);
        drawToast();
        const ImVec2 mp = ImGui::GetIO().MousePos;
        m_overUi = m_libPress >= 0 || ImGui::GetIO().WantCaptureMouse;
        for (const ImVec4& r : m_uiRects)
            if (mp.x >= r.x && mp.y >= r.y && mp.x < r.z && mp.y < r.w) m_overUi = true;
        return;
    }
    if (m_ipad) m_studioUi = true;   // iPad: Studio or the cream kit, never the Classique interface of the PC
    if (m_studioUi) drawUiStudio();   // the Studio interface (Maquette 4.0, BrickUiStudio.inl)
    else {
    uiHeader();      // m_uiRects[0]
    uiTray();        // m_uiRects[1]
    if (m_focusReticle && m_focusMode == 0 && m_rs.dof > 0 && !m_opt.noUi && !m_ipad) {   // the focus reticle (DOF Auto): a thin white circle in the middle of the view
        const ImVec2 c((m_stage.x + m_stage.z) * 0.5f, (m_stage.y + m_stage.w) * 0.5f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddCircle(ImVec2(c.x, c.y + 1), 9 * m_ui, IM_COL32(0, 0, 0, 60), 32, 2.4f * m_ui);
        dl->AddCircle(c, 9 * m_ui, IM_COL32(255, 255, 255, 150), 32, 1.2f * m_ui);
        dl->AddCircleFilled(c, 1.4f * m_ui, IM_COL32(255, 255, 255, 170), 8);
    }
    uiRadial();
    uiSelHud();
    uiColors();
    uiMechPanel();
    uiHud();
    uiGroups();
    uiReader();
    uiPopups();
    uiManual();
    uiDevPanel();
    uiBac();
    uiInfoSheet();
    uiThumbEditor();
    uiCtxMenu();
    uiDevReview();
    uiManualBig();
    drawDropzone();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    uiSceneSheet();
    drawToast();
    const ImVec2 mp = ImGui::GetIO().MousePos;
    m_overUi = m_libPress >= 0 || ImGui::GetIO().WantCaptureMouse;
    for (const ImVec4& r : m_uiRects)
        if (mp.x >= r.x && mp.y >= r.y && mp.x < r.z && mp.y < r.w) m_overUi = true;
}

// --------------------------------------------------------------------------- DEV mode (web: devRenderPanel, showInfo
// in DEV, devInfoActions, devReview)

static const ImU32 C_DEVLINE = IM_COL32(0x2b, 0x4b, 0x55, 255);
static const ImU32 C_DEVINK = IM_COL32(0x0d, 0x1a, 0x1f, 255);
static const char* kConnFr[] = {"Tenons", "Anti-tenons", "Pins", "Trous de pin", "Axe", "Trous d'axe"};   // CONN_FR
static const char* kDevTypeFr[] = {"Tenon", "Anti-tenon", "Pin", "Trou de pin", "Axe", "Trou d'axe"};
static const ImU32 kDevCol[] = {IM_COL32(0xf7, 0xc5, 0x1e, 255), IM_COL32(0xff, 0x8a, 0x3d, 255), IM_COL32(0x7f, 0xe3, 0xff, 255),
                                IM_COL32(0x4a, 0xa3, 0xff, 255), IM_COL32(0x5f, 0xd0, 0x68, 255), IM_COL32(0xc7, 0x7d, 0xff, 255)};
// legend order of the web (stud, antistud, pinhole, pin, axle, axlehole) -> Conn
static const int kDevLegend[] = {CONN_STUD, CONN_ANTISTUD, CONN_PINHOLE, CONN_PIN, CONN_AXLE, CONN_AXLEHOLE};

// toLocaleString("fr-FR") of an integer: narrow no-break space between thousands
static std::string frInt(long long v) {
    std::string d = std::to_string(v < 0 ? -v : v), o;
    for (size_t i = 0; i < d.size(); ++i) {
        if (i && (d.size() - i) % 3 == 0) o += "\xE2\x80\xAF";
        o.push_back(d[i]);
    }
    return (v < 0 ? "-" : "") + o;
}
// fmt: (Math.round(x * 100) / 100).toLocaleString("fr-FR")
static std::string frNum(double x) {
    double r = std::round(x * 100) / 100;
    char b[48];
    snprintf(b, sizeof(b), "%.2f", std::fabs(r));
    std::string s = b;
    while (s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
    for (char& c : s) if (c == '.') c = ',';
    return (r < 0 ? "-" : "") + s;
}
static float wrapText(Ui& u, ImVec2 p, float px, ImU32 col, const char* t, float wrap, bool draw = true) {
    ImFont* f = u.font;
    if (draw) u.dl->AddText(f, u.fs(px), p, col, t, nullptr, wrap);
    return f->CalcTextSizeA(u.fs(px), 1e9f, wrap, t).y;
}
// .btn of the web (compact: 28 px, 12 px text, padding 10, radius 7); sm: 28 px, 11 px, padding 8
static bool webButton(Ui& u, ImVec2 a, const char* label, float& w, bool pressed = false, ImU32 onBg = C_ACCENT, ImU32 onInk = C_INK,
                      bool disabled = false, float h = 28, float px = 12, float pad = 10) {
    const float s = u.s;
    w = u.textW(px, label, pressed) + 2 * pad * s;
    ImVec2 b(a.x + w, a.y + h * s);
    const bool hv = !disabled && u.hover(a, b);
    const float al = disabled ? 0.4f : 1.0f;
    u.dl->AddRectFilled(a, b, alphaOf(pressed ? onBg : (hv ? C_CARD_HI : C_CARD), al), 7 * s);
    u.text(ImVec2(a.x + pad * s, a.y + (h * s - u.fs(px)) * 0.5f), px, alphaOf(pressed ? onInk : C_TEXT, al), label, pressed);
    return hv && u.pressed;
}

void BrickApp::uiDevPanel() {
    if (!m_dev || m_mechOn) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const float W = std::min(340 * s, (m_stage.z - m_stage.x) - 32 * s), pad = 12 * s, gap = 10 * s;
    const float x0 = m_stage.x + 16 * s, inner = W - 2 * pad;
    // layout first (heights), then drawing from the bottom (bottom: 12 px)
    const char* store = "stocké sur cet appareil";
    std::vector<std::string> ids;
    for (auto& [id, o] : m_devOverrides) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    const float headH = 18 * s, toolsH = 30 * s, legendH = 2 * 16 * s, itemH = 44 * s;
    const char* empty = "Aucun remplacement. Appui long sur une pièce de la bibliothèque, puis « Exporter pour Maya ».";
    const float listH = ids.empty() ? wrapText(u, ImVec2(0, 0), 12, C_DIM, empty, inner, false) * 1.15f : ids.size() * itemH + (ids.size() - 1) * 6 * s;
    int ok = 0, stale = 0, missing = 0;
    for (auto& id : ids) {
        int p = devPartById(id);
        if (p < 0) ++missing;
        else if (!m_devOverrides[id].baseRelease.empty() && !m_lib.parts[p].release.empty() && m_devOverrides[id].baseRelease != m_lib.parts[p].release) ++stale;
        else ++ok;
    }
    std::string sum;
    if (!ids.empty()) {
        sum = std::to_string(ids.size()) + " remplacement" + (ids.size() > 1 ? "s" : "") + " · " + std::to_string(ok) + " à jour";
        if (stale) sum += " · " + std::to_string(stale) + " à refaire (LDraw modifié)";
        if (missing) sum += " · " + std::to_string(missing) + " pièce" + (missing > 1 ? "s" : "") + " absente" + (missing > 1 ? "s" : "");
    }
    const float sumH = sum.empty() ? 0 : wrapText(u, ImVec2(0, 0), 12, C_DIM, sum.c_str(), inner, false);
    const float H = pad + headH + gap + toolsH + gap + legendH + gap + listH + gap + (sum.empty() ? 0 : sumH + gap) + 28 * s + pad;
    ImVec2 p0(x0, m_stage.w - 12 * s - H), p1(x0 + W, m_stage.w - 12 * s);
    u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 8 * s), ImVec2(p1.x, p1.y + 8 * s), IM_COL32(0, 0, 0, 40), 12 * s);   // shadow
    u.dl->AddRectFilled(p0, p1, C_PANEL, rr(12 * s));
    u.dl->AddRect(p0, p1, C_DEVLINE, 12 * s);
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    float y = p0.y + pad, x = p0.x + pad;
    u.text(ImVec2(x, y), 13, C_DEV, "Mode développeur", true);
    u.text(ImVec2(p1.x - pad - u.textW(12, store), y + 1 * s), 12, C_FAINT, store);
    y += headH + gap;
    // toggles
    struct Tog { const char* label; bool* v; };
    Tog togs[] = {{"Connecteurs", &m_devShowVox}, {"Contour LDraw", &m_devShowEdges}, {"Remplacements", &m_devUseOverrides}};
    float tx = x;
    for (int i = 0; i < 3; ++i) {
        float w;
        if (webButton(u, ImVec2(tx, y), togs[i].label, w, *togs[i].v, C_DEV, C_DEVINK, false, 30)) {
            *togs[i].v = !*togs[i].v;
            savePrefs();
            if (i == 2) devApplyAll();
        }
        tx += w + 6 * s;
    }
    y += toolsH + gap;
    // legend (wraps on two lines in 340 px)
    float lx = x, ly = y;
    for (int k : kDevLegend) {
        const float w = 12 * s + u.textW(12, kDevTypeFr[k]);
        if (lx + w > x + inner) { lx = x; ly += 16 * s; }
        u.dl->AddCircleFilled(ImVec2(lx + 4 * s, ly + 8 * s), 4 * s, kDevCol[k], 12);
        u.text(ImVec2(lx + 12 * s, ly), 12, C_DIM, kDevTypeFr[k]);
        lx += w + 10 * s;
    }
    y += legendH + gap;
    // list
    if (ids.empty()) wrapText(u, ImVec2(x, y), 12, C_DIM, empty, inner);
    std::string removeId;
    for (const std::string& id : ids) {
        const DevOverride& o = m_devOverrides[id];
        const int part = devPartById(id);
        ImVec2 a(x, y), b(x + inner, y + itemH);
        u.dl->AddRectFilled(a, b, C_CARD, 8 * s);
        float bw1 = u.textW(11, "Retirer") + 16 * s, bw2 = u.textW(11, "Exporter") + 16 * s;
        float bx = b.x - 8 * s - bw1;
        float w;
        if (webButton(u, ImVec2(bx, y + (itemH - 28 * s) * 0.5f), "Retirer", w, false, C_ACCENT, C_INK, false, 28, 11, 8)) removeId = id;
        bx -= 6 * s + bw2;
        if (webButton(u, ImVec2(bx, y + (itemH - 28 * s) * 0.5f), "Exporter", w, false, C_ACCENT, C_INK, part < 0, 28, 11, 8) && part >= 0) devExport(part);
        const bool isStale = part >= 0 && !o.baseRelease.empty() && !m_lib.parts[part].release.empty() && o.baseRelease != m_lib.parts[part].release;
        const char* chip = part < 0 ? "absente" : isStale ? "LDraw modifié" : "à jour";
        const ImU32 cb = part < 0 ? IM_COL32(0x4a, 0x1c, 0x1e, 255) : isStale ? IM_COL32(0x4a, 0x38, 0x12, 255) : IM_COL32(0x1f, 0x3b, 0x2a, 255);
        const ImU32 cf = part < 0 ? IM_COL32(0xff, 0x9a, 0x9d, 255) : isStale ? IM_COL32(0xff, 0xd2, 0x7a, 255) : IM_COL32(0x7e, 0xe2, 0xa0, 255);
        const float cw = u.textW(11, chip) + 14 * s;
        ImVec2 ca(bx - 6 * s - cw, y + itemH * 0.5f - 9 * s), cbb(bx - 6 * s, y + itemH * 0.5f + 9 * s);
        u.dl->AddRectFilled(ca, cbb, cb, 9 * s);
        u.text(ImVec2(ca.x + 7 * s, ca.y + 2 * s), 11, cf, chip);
        if (u.hover(ca, cbb)) ImGui::SetTooltip("%s · base %s", part < 0 ? "Pièce absente de la bibliothèque" : isStale ? ("LDraw mis à jour : " + m_lib.parts[part].release).c_str() : "À jour", o.baseRelease.empty() ? "?" : o.baseRelease.c_str());
        u.dl->PushClipRect(a, ImVec2(ca.x - 6 * s, b.y), true);
        u.text(ImVec2(x + 8 * s, y + 6 * s), 12, C_TEXT, id.c_str(), true);
        std::string sub = o.label + " · " + frInt(o.triangles) + " tri";
        u.text(ImVec2(x + 8 * s, y + 23 * s), 12, C_DIM, sub.c_str());
        u.dl->PopClipRect();
        y += itemH + 6 * s;
    }
    if (!ids.empty()) y -= 6 * s;
    y = (ids.empty() ? y + listH : y) + gap;
    if (!sum.empty()) { wrapText(u, ImVec2(x, y), 12, C_DIM, sum.c_str(), inner); y += sumH + gap; }
    float w;
    if (webButton(u, ImVec2(x, y), "Importer un maillage… (.obj / .fbx)", w)) devImport(-1);
    if (!removeId.empty()) {
        std::wstring q = L"Retirer le remplacement de " + std::wstring(removeId.begin(), removeId.end()) + L" et revenir à LDraw ?";
        plat::confirm(L"BrickIt Builder", q, [this, removeId] {
            const int part = devPartById(removeId);
            if (part >= 0) devRevert(part);
            else { devDelete(removeId); m_devOverrides.erase(removeId); }
        });
    }
}

// Full part sheet of the DEV mode (web: showInfo + devInfoActions). Box min(640 px), padding 18, gap 12.
void BrickApp::uiDevSheet() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(0, 0, 0, 128));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const int part = m_infoPart;
    const Part& p = m_lib.parts[part];
    const std::string id = devId(part);
    const float W = std::min(640 * s, d.x - 32 * s), pad = 18 * s, gap = 12 * s, inner = W - 2 * pad;
    // rows of the dl
    std::vector<std::pair<std::string, std::string>> rows;
    auto row = [&](const char* k, const std::string& v) { if (!v.empty()) rows.push_back({k, v}); };
    row("Famille", p.family);
    row("Référence LDraw", p.ldraw.empty() ? "" : p.ldraw + ".dat");
    row("Catégorie LDraw", p.category);
    if (p.hasSizeStuds) row("Encombrement", frNum(p.sizeStuds[0]) + " × " + frNum(p.sizeStuds[1]) + " tenons · " + std::to_string(p.heightPlates) + " plate" + (p.heightPlates > 1 ? "s" : "") + " de haut");
    if (p.hasSizeCm) row("Dimensions", frNum(p.sizeCm[0]) + " × " + frNum(p.sizeCm[2]) + " × " + frNum(p.sizeCm[1]) + " cm (L × P × H, tenons compris)");
    row("Points de connexion", std::to_string(p.voxels.size()) + " voxels");
    row("Maillage", frInt(p.triangles) + " triangles · " + frInt(p.vertices) + " sommets");
    row("Auteur LDraw", p.author);
    row("Licence", p.license);
    row("Version LDraw", p.release);
    if (p.history) row("Historique", std::to_string(p.history) + " révision" + (p.history > 1 ? "s" : ""));
    row("Mots-clés", p.keywords);
    if (p.hasOrigin) row("Origine LDraw", frNum(p.ldrawOrigin.x) + " · " + frNum(p.ldrawOrigin.y) + " · " + frNum(p.ldrawOrigin.z) + " cm (coin min.)");
    float dtW = 0;
    for (auto& r : rows) dtW = std::max(dtW, u.textW(13, r.first.c_str()));
    const float ddX = dtW + 14 * s, ddW = inner - ddX;
    float rowsH = 0;
    std::vector<float> rh;
    for (auto& r : rows) { float h = wrapText(u, ImVec2(0, 0), 13, C_TEXT, r.second.c_str(), ddW, false); rh.push_back(h); rowsH += h + 6 * s; }
    const char* credit = "Géométrie : bibliothèque LDraw (ldraw.org, CC BY). Connexions : LDraw + LDCad shadow library, converties automatiquement. Couleurs réelles : données Rebrickable (rebrickable.com).";
    const float creditH = wrapText(u, ImVec2(0, 0), 11, C_FAINT, credit, inner, false) * 1.1f;
    auto ov = m_devOverrides.find(id);
    std::string note;
    if (ov != m_devOverrides.end()) {
        const DevOverride& o = ov->second;
        const bool stale = !o.baseRelease.empty() && !p.release.empty() && o.baseRelease != p.release;
        note = "Mode DEV · remplacement actif (" + frInt(o.triangles) + " triangles, " + o.fileName + ") · base " + (o.baseRelease.empty() ? "?" : o.baseRelease) + " · " + (stale ? "LDraw mis à jour : " + p.release : std::string("À jour"));
    }
    const float noteH = note.empty() ? 0 : wrapText(u, ImVec2(0, 0), 11, C_FAINT, note.c_str(), inner, false) * 1.1f;
    const float headH = 192 * s;   // image 240 x 192 (aspect 5 / 4)
    const float H = pad + headH + gap + rowsH - 6 * s + gap + creditH + gap + (note.empty() ? 0 : noteH + gap) + 8 * s + 28 * s + gap + 28 * s + pad;
    ImVec2 p0((d.x - W) * 0.5f, std::max(8 * s, (d.y - H) * 0.5f)), p1(p0.x + W, p0.y + H);
    u.dl->AddRectFilled(p0, p1, C_PANEL_SOLID, rr(14 * s));
    u.dl->AddRect(p0, p1, C_LINE, rr(14 * s));
    float x = p0.x + pad, y = p0.y + pad;
    // head: image (radial gradient #34363d -> #1d1e22) and title + chips
    {
        ImVec2 ia(x, y), ib(x + 240 * s, y + headH);
        u.dl->AddRectFilled(ia, ib, IM_COL32(0x1d, 0x1e, 0x22, 255), 10 * s);
        const ImVec2 c((ia.x + ib.x) * 0.5f, ia.y + headH * 0.45f);
        for (int k = 12; k >= 1; --k) {
            const float t = k / 12.0f;
            const int r = (int)(0x34 + (0x1d - 0x34) * t), g = (int)(0x36 + (0x1e - 0x36) * t), b = (int)(0x3d + (0x22 - 0x3d) * t);
            u.dl->PushClipRect(ia, ib, true);
            u.dl->AddCircleFilled(c, 150 * s * t, IM_COL32(r, g, b, 255), 48);
            u.dl->PopClipRect();
        }
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(part, tex, uv0, uv1)) u.dl->AddImage(tex, ImVec2(c.x - headH * 0.5f, ia.y), ImVec2(c.x + headH * 0.5f, ib.y), uv0, uv1);
        const float rx = ib.x + 12 * s, rw = p1.x - pad - rx;
        float ry = y;
        ry += u.text(ImVec2(rx, ry), 17, C_TEXT, p.label.c_str(), true).y + 2 * s;   // h2
        std::string t2;
        for (char ch : p.title) { if (std::isspace((unsigned char)ch)) { if (t2.empty() || t2.back() != ' ') t2.push_back(' '); } else t2.push_back(ch); }
        ry += wrapText(u, ImVec2(rx, ry), 12, C_DIM, t2.c_str(), rw) + 10 * s;
        float cx = rx;
        for (auto& [k, n] : p.connectors) {
            int ci = -1;
            for (int q = 0; q < CONN_COUNT; ++q) if (k == std::string(q == 0 ? "stud" : q == 1 ? "antistud" : q == 2 ? "pin" : q == 3 ? "pinhole" : q == 4 ? "axle" : "axlehole")) ci = q;
            const std::string num = std::to_string(n), lab = ci >= 0 ? kConnFr[ci] : k;
            const float w = 16 * s + u.textW(12, num.c_str(), true) + 4 * s + u.textW(12, lab.c_str());
            if (cx + w > p1.x - pad) { cx = rx; ry += 26 * s; }
            u.dl->AddRectFilled(ImVec2(cx, ry), ImVec2(cx + w, ry + 22 * s), C_CARD, 11 * s);
            float tw = u.text(ImVec2(cx + 8 * s, ry + 3 * s), 12, C_ACCENT_TXT, num.c_str(), true).x;
            u.text(ImVec2(cx + 8 * s + tw + 4 * s, ry + 3 * s), 12, C_TEXT, lab.c_str());
            cx += w + 6 * s;
        }
    }
    y += headH + gap;
    for (size_t i = 0; i < rows.size(); ++i) {
        u.text(ImVec2(x, y), 13, C_DIM, rows[i].first.c_str());
        wrapText(u, ImVec2(x + ddX, y), 13, C_TEXT, rows[i].second.c_str(), ddW);
        y += rh[i] + 6 * s;
    }
    y += gap - 6 * s;
    wrapText(u, ImVec2(x, y), 11, C_FAINT, credit, inner);
    y += creditH + gap;
    if (!note.empty()) { wrapText(u, ImVec2(x, y), 11, C_FAINT, note.c_str(), inner); y += noteH + gap; }
    // DEV row (dashed top border)
    for (float dx = x; dx < p1.x - pad; dx += 6 * s) u.dl->AddLine(ImVec2(dx, y), ImVec2(std::min(dx + 3 * s, p1.x - pad), y), C_DEVLINE, 1);
    y += 8 * s;
    {
        const float tw = u.textW(11, "DEV", true) + 12 * s;
        u.dl->AddRectFilled(ImVec2(x, y + 5 * s), ImVec2(x + tw, y + 23 * s), C_DEV, 6 * s);
        u.text(ImVec2(x + 6 * s, y + 7 * s), 11, C_DEVINK, "DEV", true);
        float bx = x + tw + 8 * s, w;
        if (webButton(u, ImVec2(bx, y), "Exporter pour Maya", w)) devExport(part);
        bx += w + 8 * s;
        if (webButton(u, ImVec2(bx, y), "Importer le maillage modifié", w)) { m_infoPart = -1; devImport(part); return; }
        bx += w + 8 * s;
        if (ov != m_devOverrides.end() && webButton(u, ImVec2(bx, y), "Revenir à LDraw", w)) {
            devRevert(part);
            m_infoPart = -1;
            toast(id + " : géométrie LDraw rétablie");
            return;
        }
    }
    y += 28 * s + gap;
    // actions (right): Fermer, favourites, Ajouter à la scène
    const bool fav = std::find(m_fav.begin(), m_fav.end(), part) != m_fav.end();
    const char* fl = fav ? "★ Retirer des favoris" : "☆ Ajouter aux favoris";
    float wClose = u.textW(12, "Fermer") + 20 * s, wFav = u.textW(12, fl) + 20 * s, wAdd = u.textW(12, "Ajouter à la scène", true) + 20 * s, w;
    float bx = p1.x - pad - wAdd - 8 * s - wFav - 8 * s - wClose;
    const bool cClose = webButton(u, ImVec2(bx, y), "Fermer", w);
    bx += wClose + 8 * s;
    if (webButton(u, ImVec2(bx, y), fl, w)) {
        if (fav) m_fav.erase(std::find(m_fav.begin(), m_fav.end(), part));
        else m_fav.push_back(part);
        savePrefs();
    }
    bx += wFav + 8 * s;
    const bool cAdd = webButton(u, ImVec2(bx, y), "Ajouter à la scène", w, true);
    if (cClose || ImGui::IsKeyPressed(ImGuiKey_Escape) || (u.pressed && !u.hover(p0, p1))) m_infoPart = -1;
    if (cAdd) {
        m_infoPart = -1;
        spawn(part, (m_stage.x + m_stage.z) * 0.5f, (m_stage.y + m_stage.w) * 0.5f);
    }
}

// Review of a re-imported mesh (web: devReview): live preview in the scene, rows, Annuler / Recaler / Garder.
void BrickApp::uiDevReview() {
    if (!m_devReview.open) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    ImVec2 d = ImGui::GetIO().DisplaySize;
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(0, 0, 0, 128));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    DevReview& R = m_devReview;
    const Part& p = m_lib.parts[R.part];
    const std::string id = devId(R.part);
    const float W = std::min(640 * s, d.x - 32 * s), pad = 18 * s, gap = 12 * s, inner = W - 2 * pad;
    const int tri = (int)(R.g.indices.size() / 3), ltri = (int)((p.hasLdrawMesh ? p.ldrawMesh : R.prev).indices.size() / 3);
    struct Row { std::string k, v; bool warn; };
    std::vector<Row> rows;
    rows.push_back({"Pièce", id + " · " + p.label, false});
    rows.push_back({"Fichier", R.fileName, false});
    rows.push_back({"Triangles", frInt(tri) + " (LDraw : " + frInt(ltri) + ")", false});
    rows.push_back({"Dimensions", frNum(R.size.x) + " × " + frNum(R.size.z) + " × " + frNum(R.size.y) + " cm (LDraw : " + frNum(R.ref.x) + " × " + frNum(R.ref.z) + " × " + frNum(R.ref.y) + ")", false});
    const float offLen = length(R.offset);
    rows.push_back({"Décalage du centre", offLen < 0.02f ? std::string("aligné (< 0,2 mm)") : frNum(R.offset.x) + " · " + frNum(R.offset.y) + " · " + frNum(R.offset.z) + " cm", offLen >= 0.08f});
    if (!R.note.empty()) rows.push_back({"Échelle", R.note, true});
    if (R.swapped) rows.push_back({"Axes", "Proportions différentes de LDraw : Z vers le haut dans Maya, ou pièce tournée ?", true});
    rows.push_back({"Connecteurs", std::to_string(p.voxels.size()) + " points, toujours calculés depuis LDraw", false});
    rows.push_back({"Base LDraw", p.release.empty() ? std::string("inconnue") : p.release, false});
    float dtW = 0;
    for (auto& r : rows) dtW = std::max(dtW, u.textW(13, r.k.c_str()));
    const float ddX = dtW + 14 * s, ddW = inner - ddX;
    std::vector<float> rh;
    float rowsH = 0;
    for (auto& r : rows) { float h = wrapText(u, ImVec2(0, 0), 13, C_TEXT, r.v.c_str(), ddW, false); rh.push_back(h); rowsH += h + 6 * s; }
    const char* intro = "Aperçu en direct dans la scène, avec les connecteurs LDraw et le contour LDraw par-dessus.";
    const float introH = wrapText(u, ImVec2(0, 0), 13, C_DIM, intro, inner, false) * 1.15f;
    const float H = pad + 22 * s + gap + introH + gap + rowsH - 6 * s + gap + 28 * s + pad;
    ImVec2 p0((d.x - W) * 0.5f, (d.y - H) * 0.5f), p1(p0.x + W, p0.y + H);
    u.dl->AddRectFilled(p0, p1, C_PANEL_SOLID, rr(14 * s));
    u.dl->AddRect(p0, p1, C_LINE, rr(14 * s));
    float x = p0.x + pad, y = p0.y + pad;
    u.text(ImVec2(x, y), 17, C_TEXT, ("Réimport de " + id).c_str(), true);
    y += 22 * s + gap;
    wrapText(u, ImVec2(x, y), 13, C_DIM, intro, inner);
    y += introH + gap;
    for (size_t i = 0; i < rows.size(); ++i) {
        u.text(ImVec2(x, y), 13, C_DIM, rows[i].k.c_str());
        wrapText(u, ImVec2(x + ddX, y), 13, rows[i].warn ? IM_COL32(0xff, 0xd2, 0x7a, 255) : C_TEXT, rows[i].v.c_str(), ddW);
        y += rh[i] + 6 * s;
    }
    y += gap - 6 * s;
    float w;
    if (webButton(u, ImVec2(x, y), "Annuler", w)) { devReviewCancel(); return; }
    const float wKeep = u.textW(12, "Garder ce remplacement", true) + 20 * s, wRe = u.textW(12, "Recaler sur le centre LDraw") + 20 * s;
    const bool keep = webButton(u, ImVec2(p1.x - pad - wKeep, y), "Garder ce remplacement", w, true);
    if (offLen >= 0.02f && webButton(u, ImVec2(p1.x - pad - wKeep - 8 * s - wRe, y), "Recaler sur le centre LDraw", w)) devReviewRecenter();
    if (keep) devReviewKeep();
}

// --------------------------------------------------------------------------- instruction manual (web: #manual, #mbig,
// #pdfbig): steps of the scene read vertically, imported PDF read vertically, resizable from the bottom left corner

static const ImU32 C_PAPER = IM_COL32(0xfb, 0xfa, 0xf6, 255), C_PAPER_INK = IM_COL32(0x22, 0x25, 0x2d, 255), C_PAPER_CANVAS = IM_COL32(0xf4, 0xf2, 0xea, 255);

// One page of the scene manual (.mp): step number, part to add (thumbnail + "1×"), step image (8 / 5), "p. n".
// big: the #mbig layout (padding 20 24 24, gap 12, number 44 px, thumbnail 48 x 38, 13 px, no page number).
float BrickApp::manualPage(Ui& u, ImVec2 a, float w, int n, int corner, bool big, bool draw) {
    const float s = u.s;
    const float padT = (big ? 20 : 8) * s, padX = (big ? 24 : 10) * s, padB = (big ? 24 : 10) * s, gap = (big ? 12 : 5) * s;
    const float numPx = big ? 44 : 26;
    const float inner = w - 2 * padX, ch = inner * 5 / 8;
    const float H = padT + u.fs(numPx) + gap + ch + padB;
    if (!draw) return H;
    const float r = 8 * s;
    ImDrawFlags fl = corner == 0 ? (ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft) : (ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomRight);
    u.dl->AddRectFilled(a, ImVec2(a.x + w, a.y + H), C_PAPER, big ? 0 : r, big ? 0 : fl);
    const bool valid = n >= 1 && n <= (int)m_bricks.size();
    float y = a.y + padT;
    if (valid) {
        const std::string sn = std::to_string(n);
        u.text(ImVec2(a.x + padX, y), numPx, C_PAPER_INK, sn.c_str(), true);
        // .mneed: thumbnail (34 x 26, #e9e6dc, radius 4) + "1×", at the right
        const float iw = (big ? 48 : 34) * s, ih = (big ? 38 : 26) * s, tp = big ? 13 : 11;
        const float tw = u.textW(tp, "1×", true);
        ImVec2 ib(a.x + w - padX - tw - 5 * s - iw, y + (u.fs(numPx) - ih) * 0.5f);
        u.dl->AddRectFilled(ib, ImVec2(ib.x + iw, ib.y + ih), IM_COL32(0xe9, 0xe6, 0xdc, 255), 4 * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        const int part = m_bricks[n - 1].part;
        if (thumbUv(part, tex, uv0, uv1)) u.dl->AddImage(tex, ImVec2(ib.x + (iw - ih) * 0.5f, ib.y), ImVec2(ib.x + (iw + ih) * 0.5f, ib.y + ih), uv0, uv1);
        u.text(ImVec2(ib.x + iw + 5 * s, y + (u.fs(numPx) - u.fs(tp)) * 0.5f), tp, C_PAPER_INK, "1×", true);
        if (u.hover(ib, ImVec2(ib.x + iw + 5 * s + tw, ib.y + ih))) ImGui::SetTooltip("%s", m_lib.parts[part].label.c_str());
    }
    y += u.fs(numPx) + gap;
    ImVec2 ca(a.x + padX, y), cb(a.x + padX + inner, y + ch);
    u.dl->AddRectFilled(ca, cb, C_PAPER_CANVAS, 6 * s);
    if (valid) {
        ImTextureID t = stepImage(n);
        if (t) u.dl->AddImageRounded(t, ca, cb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 6 * s);
    }
    u.dl->AddRect(ca, cb, C_PAPER_INK, 6 * s, 0, 1.5f * s);
    if (!big && n >= 1) {
        const std::string pn = "p. " + std::to_string(n);
        u.text(ImVec2(a.x + w - 8 * s - u.textW(9, pn.c_str()), a.y + H - 2 * s - u.fs(9)), 9, IM_COL32(0x7a, 0x7e, 0x88, 255), pn.c_str());
    }
    return H;
}

void BrickApp::dragScroll(Ui& u, ImVec2 a, ImVec2 b, float& scroll, int id) {
    if (!m_ipad) return;
    ImGuiIO& io = ImGui::GetIO();
    const float dt = std::max(1e-3f, io.DeltaTime);
    if (u.pressed && u.hover(a, b)) { m_dsId = id; m_dsVel = 0; if (m_dsFling == id) m_dsFling = -1; }
    if (m_dsId == id) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float dy = io.MouseDelta.y;
            scroll -= dy;
            m_dsVel = m_dsVel * 0.5f + (-dy / dt) * 0.5f;
        } else { m_dsId = -1; m_dsFling = std::fabs(m_dsVel) > 60 * u.s ? id : -1; }
    } else if (m_dsFling == id) {   // thrown: it glides, slowing down
        scroll += m_dsVel * dt;
        m_dsVel *= std::pow(0.04f, dt);
        if (std::fabs(m_dsVel) < 20 * u.s) m_dsFling = -1;
    }
}

// Vertical PDF view (.pdfview): white pages one under the other, gap 10, rendered when they come near the view.
void BrickApp::pdfView(Ui& u, ImVec2 a, ImVec2 b, float& scroll, bool main) {
    const float s = u.s;
    const float w = b.x - a.x - 4 * s;
    if (m_pdfSizes.empty()) {
        const char* msg = m_pdfMsg.empty() ? "Ouverture du PDF…" : m_pdfMsg.c_str();
        u.text(ImVec2((a.x + b.x) * 0.5f - u.textW(12, msg) * 0.5f, a.y + 16 * s), 12, C_DIM, msg);
        return;
    }
    std::vector<float> top(m_pdfSizes.size());
    float y = 0;
    for (size_t i = 0; i < m_pdfSizes.size(); ++i) {
        top[i] = y;
        y += w * m_pdfSizes[i].second / std::max(1.0f, m_pdfSizes[i].first) + 10 * s;
    }
    const float content = y - 10 * s + 4 * s, viewH = b.y - a.y;
    // scroll requests (‹ ›, start)
    if (main && m_pdfScrollTo > 0) {
        const int n = std::clamp(m_pdfScrollTo, 1, (int)m_pdfSizes.size());
        m_pdfScrollGoal = top[n - 1];
        if (!m_pdfScrollSmooth) scroll = m_pdfScrollGoal;
        m_pdfScrollTo = 0;
        m_pdfLock = ImGui::GetTime() + 0.7;
    }
    if (!main && m_bigScrollTo > 0) {
        scroll = top[std::clamp(m_bigScrollTo, 1, (int)m_pdfSizes.size()) - 1];
        m_bigScrollTo = 0;
    }
    const bool over = u.hover(a, b);
    if (over && ImGui::GetIO().MouseWheel != 0) { scroll -= ImGui::GetIO().MouseWheel * 90 * s; if (main) m_pdfScrollGoal = -1; }
    {
        const float before = scroll;
        dragScroll(u, a, b, scroll, main ? 1 : 2);
        if (main && scroll != before) { m_pdfScrollGoal = -1; m_pdfLock = 0; }
    }
    if (main && m_pdfScrollGoal >= 0) {
        scroll += (m_pdfScrollGoal - scroll) * std::min(1.0f, ImGui::GetIO().DeltaTime * 12);   // behavior: "smooth"
        if (std::fabs(m_pdfScrollGoal - scroll) < 0.5f) { scroll = m_pdfScrollGoal; m_pdfScrollGoal = -1; }
    }
    scroll = std::clamp(scroll, 0.0f, std::max(0.0f, content - viewH));
    u.dl->PushClipRect(a, b, true);
    const int want = (int)std::round(w);
    for (size_t i = 0; i < m_pdfSizes.size(); ++i) {
        const float h = w * m_pdfSizes[i].second / std::max(1.0f, m_pdfSizes[i].first);
        ImVec2 pa(a.x + 2 * s, a.y + 2 * s + top[i] - scroll), pb(pa.x + w, pa.y + h);
        const bool inRange = pb.y > a.y - 600 * s && pa.y < b.y + 600 * s;   // rootMargin: 600px
        PdfTex& t = m_pdfTex[i];
        if (inRange && (!t.tex || std::abs(t.w - want) > 2) && !t.asked && m_pdf) { m_pdf->request((int)i, want); t.asked = true; }
        if (!inRange && t.tex && std::abs((int)i + 1 - m_pdfPage) > 6) { m_gui->releaseTexture(t.tex); t = PdfTex(); }
        if (pb.y < a.y || pa.y > b.y) continue;
        u.dl->AddRectFilled(ImVec2(pa.x, pa.y + 4 * s), ImVec2(pb.x, pb.y + 4 * s), IM_COL32(0, 0, 0, 60), 4 * s);
        u.dl->AddRectFilled(pa, pb, IM_COL32_WHITE, 4 * s);
        if (t.tex) u.dl->AddImageRounded(t.tex, pa, pb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 4 * s);
        const std::string lab = std::to_string(i + 1) + " / " + std::to_string(m_pdfSizes.size());
        const float lw = u.textW(10, lab.c_str()) + 10 * s;
        ImVec2 la(pb.x - 6 * s - lw, pb.y - 4 * s - u.fs(10) - 2 * s);
        u.dl->AddRectFilled(la, ImVec2(la.x + lw, pb.y - 4 * s), IM_COL32(255, 255, 255, 204), 6 * s);
        u.text(ImVec2(la.x + 5 * s, la.y + 1 * s), 10, IM_COL32(0x55, 0x55, 0x55, 255), lab.c_str());
    }
    u.dl->PopClipRect();
    if (main && ImGui::GetTime() >= m_pdfLock) {   // updatePdfPage: the last page whose top is above the view top + 30
        int n = 1;
        for (size_t i = 0; i < top.size(); ++i)
            if (a.y + 2 * s + top[i] - scroll <= a.y + 30 * s) n = (int)i + 1;
            else break;
        if (n != m_pdfPage) { m_pdfPage = n; m_prefsDirty = true; }
    }
}

float BrickApp::manualWidth() const {
    const float s = m_ui;
    const float dsx = ImGui::GetIO().DisplaySize.x;
    if (m_manDock) return std::clamp(m_manW >= 260 ? m_manW : 380 * s, 260 * s, dsx * 0.5f);
    const float stageW = m_stage.z - m_stage.x;
    const float W = m_manW >= 260 ? m_manW : std::min(stageW * 0.34f, 500 * s);
    return std::min(W, std::max(300 * s, stageW - 40 * s));
}

void BrickApp::manualReserveDock() {
    if (!m_pManual.visible || m_mechOn || !m_manDock || m_pManual.floating) return;
    const float s = m_ui, W = manualWidth(), dsx = ImGui::GetIO().DisplaySize.x;
    if (m_manDock == 1) { m_manDockX = m_dockL + 8 * s; m_dockL += W + 12 * s; }
    else { m_manDockX = dsx - m_dockR - 8 * s - W; m_dockR += W + 12 * s; }
}

// The Notice (Wzx 11/11/2026): big arrows on the left and the right of the pages, the two pages side by side when the
// panel is wider than tall (one under the other otherwise), dockable in the dock spaces on the left / the right
void BrickApp::uiManual() {
    if (!m_pManual.visible || m_mechOn) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const float stageW = m_stage.z - m_stage.x;
    const bool docked = m_manDock != 0 && !m_pManual.floating;
    const float W = manualWidth();
    const float padT = 8 * s, padX = 10 * s, padB = 10 * s, gap = 8 * s, headH = 28 * s;
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float libTop = m_uiRects.size() > 1 ? m_uiRects[1].y : ds.y;
    float x1 = m_stage.z - 14 * s, x0 = x1 - W, y0 = m_stage.y + 12 * s;
    if (docked) { x0 = m_manDockX; x1 = x0 + W; y0 = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 8 * s; }
    else if (m_pManual.floating) {   // moved by its grip (Wzx 09/11/2026: « le grip notice ne fonctionne pas »)
        x0 = std::clamp(m_pManual.x, 0.0f, std::max(0.0f, ds.x - W));
        y0 = std::clamp(m_pManual.y, 0.0f, std::max(0.0f, ds.y - 60 * s));
        x1 = x0 + W;
    }
    const bool folded = m_pManual.collapsed && !docked;
    const float innerW = W - 2 * padX;
    const int total = (int)m_bricks.size();
    m_manRight = std::clamp(m_manRight, 2, std::max(2, total));
    const int l = m_manRight - 1, r = m_manRight;
    const float pgap = 8 * s;
    auto pagesHeight = [&](bool side) {
        if (side) {
            const float pw = (innerW - pgap) * 0.5f;
            return std::max(manualPage(u, ImVec2(0, 0), pw, l, 0, false, false), manualPage(u, ImVec2(0, 0), pw, r, 1, false, false));
        }
        return manualPage(u, ImVec2(0, 0), innerW, l, 0, false, false) + pgap + manualPage(u, ImVec2(0, 0), innerW, r, 1, false, false);
    };
    const float maxBody = (docked ? libTop - 8 * s : m_stage.w - 8 * s) - y0 - padT - headH - gap - padB;
    float bodyH;
    bool side = false;
    if (docked) bodyH = maxBody;
    else if (m_manH >= 160) bodyH = m_manH - padT - headH - gap - padB;
    else if (m_pdfOn) bodyH = std::min(ds.y * 0.72f, 920 * s);
    else bodyH = std::min(pagesHeight(false), std::min(ds.y * 0.70f, 900 * s));
    bodyH = std::max(40 * s, std::min(bodyH, maxBody));
    if (!m_pdfOn) {
        side = innerW > bodyH * 1.25f;   // wider than tall: the two pages side by side
        if (side && !docked && m_manH < 160) bodyH = std::max(40 * s, std::min(pagesHeight(true), maxBody));
    }
    const float pagesH = pagesHeight(side);
    const float H = padT + headH + (folded ? 0 : gap + bodyH) + padB;
    ImVec2 p0(x0, y0), p1(x1, y0 + H);
    u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 16 * s), ImVec2(p1.x, p1.y + 16 * s), IM_COL32(0, 0, 0, 40), 16 * s);   // shadow
    u.dl->AddRectFilled(p0, p1, (C_PANEL & 0x00FFFFFFu) | (std::max<ImU32>(C_PANEL >> 24, 248u) << 24), rr(16 * s));   // nearly opaque: it floats over the tools / colours
    u.dl->AddRect(p0, p1, C_LINE, rr(16 * s));
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    if (gripPanel(3, m_pManual, p0, ImVec2(p0.x + 34 * s + 2 * padX, p0.y + padT + headH), p0)) return;
    if (u.hover(p0, ImVec2(p0.x + 34 * s + 2 * padX, p0.y + padT + headH)) || m_grip.id == 3) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    // header: grip dots, Manuel, the steps / the page, then its buttons (drawn icons: the fonts of the iPad lack ⤢)
    float x = p0.x + padX, y = p0.y + padT;
    for (int rw = 0; rw < 2; ++rw)
        for (int cc = 0; cc < 5; ++cc) u.dl->AddCircleFilled(ImVec2(x + (3 + cc * 6) * s, y + headH * 0.5f - 2 * s + rw * 4 * s), 1.1f * s, C_FAINT, 6);
    x += 26 * s + 6 * s;
    float tw = u.text(ImVec2(x, y + (headH - u.fs(13)) * 0.5f), 13, C_TEXT, "Notice", true).x;
    x += tw + 6 * s;
    char step[96];
    if (m_pdfOn && !m_pdfSizes.empty()) snprintf(step, sizeof(step), "PDF p. %d / %d", m_pdfPage, (int)m_pdfSizes.size());
    else if (total) snprintf(step, sizeof(step), "Étapes %d–%d / %d", l, std::min(r, total), total);
    else snprintf(step, sizeof(step), "Aucune brique");
    u.text(ImVec2(x, y + (headH - u.fs(11)) * 0.5f), 11, C_DIM, step);
    struct Btn { const char* label; int icon; int id; const char* tip; };   // icon: 0 text, 1 fold, 2 enlarge
    std::vector<Btn> btns;
    if (m_pdf && (!m_pdfSizes.empty() || m_pdfOpening)) btns.push_back({m_pdfOn ? "Scène" : "PDF", 0, 0, m_pdfOn ? "Revenir au manuel de ta scène" : "Revenir au PDF importé"});
    btns.push_back({"PDF", 0, 1, "Importer un PDF d'instructions"});
    if (!docked) btns.push_back({"", 1, 4, "Réduire"});
    btns.push_back({"", 2, 5, "Agrandir"});
    float bx = p1.x - padX;
    int clicked = -1;
    for (int i = (int)btns.size() - 1; i >= 0; --i) {
        const Btn& bt = btns[i];
        const float bw = bt.icon == 0 ? u.textW(11, bt.label, true) + 16 * s : 28 * s;
        bx -= bw;
        ImVec2 a(bx, y), b(bx + bw, y + 28 * s);
        const bool h = u.hover(a, b);
        u.dl->AddRectFilled(a, b, h ? C_LYEL : C_CARD, 8 * s);
        const ImU32 fg = h ? IM_COL32(0x1b, 0x15, 0x00, 255) : C_TEXT;
        const ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
        if (bt.icon == 0) {
            const float lw = u.textW(11, bt.label, true);
            u.text(ImVec2(c.x - lw * 0.5f, a.y + (28 * s - u.fs(11)) * 0.5f), 11, fg, bt.label, true);
        } else if (bt.icon == 1) u.dl->AddLine(ImVec2(c.x - 6 * s, c.y), ImVec2(c.x + 6 * s, c.y), fg, 1.6f * s);
        else {   // enlarge: two corners going out
            const float k = 6 * s, t = 3.5f * s;
            u.dl->AddLine(ImVec2(c.x - k, c.y + k), ImVec2(c.x + k, c.y - k), fg, 1.4f * s);
            u.dl->AddLine(ImVec2(c.x + k - t, c.y - k), ImVec2(c.x + k, c.y - k), fg, 1.4f * s);
            u.dl->AddLine(ImVec2(c.x + k, c.y - k), ImVec2(c.x + k, c.y - k + t), fg, 1.4f * s);
            u.dl->AddLine(ImVec2(c.x - k, c.y + k - t), ImVec2(c.x - k, c.y + k), fg, 1.4f * s);
            u.dl->AddLine(ImVec2(c.x - k, c.y + k), ImVec2(c.x - k + t, c.y + k), fg, 1.4f * s);
        }
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", bt.tip);
        if (h && u.pressed) clicked = bt.id;
        bx -= 4 * s;
    }
    if (clicked == 0) { m_pdfOn = !m_pdfOn; if (m_pdfOn) m_pdfScrollTo = m_pdfPage, m_pdfScrollSmooth = false; }
    if (clicked == 1) pdfDialog();
    if (clicked == 4) { m_pManual.collapsed = !m_pManual.collapsed; savePrefs(); }
    if (clicked == 5) { if (m_pdfOn) { m_pdfBig = true; m_bigScrollTo = m_pdfPage; } else { m_manBig = true; m_bigScroll = 0; } }
    if (folded) return;
    // body
    ImVec2 ba(p0.x + padX, p0.y + padT + headH + gap), bb(p1.x - padX, ba.y + bodyH);
    // the arrows: round, on the left and the right edges of the pages, half over them (drawn after the pages)
    const float ar = 19 * s;
    const ImVec2 aL(ba.x + ar * 0.6f, (ba.y + bb.y) * 0.5f), aR(bb.x - ar * 0.6f, (ba.y + bb.y) * 0.5f);
    const bool hasPrev = m_pdfOn ? m_pdfPage > 1 : m_manRight > 2;
    const bool hasNext = m_pdfOn ? m_pdfPage < (int)m_pdfSizes.size() : m_manRight < total;
    auto inArrow = [&](ImVec2 c) { const float dx = u.mouse.x - c.x, dy = u.mouse.y - c.y; return dx * dx + dy * dy <= (ar + 4 * s) * (ar + 4 * s); };
    const bool overArrow = inArrow(aL) || inArrow(aR);
    if (m_pdfOn) pdfView(u, ba, bb, m_pdfScroll, true);
    else {
        const bool over = u.hover(ba, bb) && !overArrow;
        if (over && ImGui::GetIO().MouseWheel != 0) m_manScroll -= ImGui::GetIO().MouseWheel * 60 * s;
        if (!overArrow) dragScroll(u, ba, bb, m_manScroll, 3);
        m_manScroll = std::clamp(m_manScroll, 0.0f, std::max(0.0f, pagesH - bodyH));
        u.dl->PushClipRect(ba, bb, true);
        float py = ba.y - m_manScroll;
        if (side) {
            const float pw = (innerW - pgap) * 0.5f;
            manualPage(u, ImVec2(ba.x, py), pw, l, 0, false, true);
            manualPage(u, ImVec2(ba.x + pw + pgap, py), pw, r, 1, false, true);
        } else {
            py += manualPage(u, ImVec2(ba.x, py), innerW, l, 0, false, true) + pgap;
            manualPage(u, ImVec2(ba.x, py), innerW, r, 1, false, true);
        }
        u.dl->PopClipRect();
        if (over) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);   // cursor: zoom-in
        if (over && u.released && ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x == 0 && ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y == 0 && !m_manResizing) { m_manBig = true; m_bigScroll = 0; }
    }
    for (int k = 0; k < 2; ++k) {
        const ImVec2 c = k == 0 ? aL : aR;
        const bool on = k == 0 ? hasPrev : hasNext;
        const bool h = on && inArrow(c);
        u.dl->AddCircleFilled(ImVec2(c.x, c.y + 2 * s), ar, IM_COL32(0, 0, 0, 60), 32);
        u.dl->AddCircleFilled(c, ar, h ? C_LYEL : alphaOf(C_PANEL_SOLID, on ? 0.92f : 0.5f), 32);
        u.dl->AddCircle(c, ar, alphaOf(C_LINE, on ? 1.0f : 0.5f), 32, 1.0f);
        const float d = k == 0 ? -1.0f : 1.0f, q = 5.5f * s;
        const ImU32 fg = h ? IM_COL32(0x1b, 0x15, 0x00, 255) : alphaOf(C_TEXT, on ? 1.0f : 0.3f);
        u.dl->AddLine(ImVec2(c.x - d * q * 0.5f, c.y - q), ImVec2(c.x + d * q * 0.5f, c.y), fg, 2.2f * s);
        u.dl->AddLine(ImVec2(c.x + d * q * 0.5f, c.y), ImVec2(c.x - d * q * 0.5f, c.y + q), fg, 2.2f * s);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", k == 0 ? "Pages précédentes" : "Pages suivantes");
        if (h && u.pressed) {   // manualStep
            const int dd = k == 0 ? -1 : 1;
            if (m_pdfOn && !m_pdfSizes.empty()) { m_pdfScrollTo = std::clamp(m_pdfPage + dd, 1, (int)m_pdfSizes.size()); m_pdfScrollSmooth = true; m_pdfPage = m_pdfScrollTo; }
            else { m_manRight += dd; m_manScroll = 0; }
        }
    }
    // the resize handle in the inner bottom corner: free, the width and the height (anchored at the top, at the outer
    // side); docked, the width alone. Wider than tall, the pages go side by side.
    const bool handleRight = m_manDock == 1 && docked;
    ImVec2 ra = handleRight ? ImVec2(p1.x - 27 * s, p1.y - 27 * s) : ImVec2(p0.x + 3 * s, p1.y - 27 * s), rb(ra.x + 24 * s, ra.y + 24 * s);
    const bool rh = u.hover(ra, rb);
    if (rh && u.pressed) { m_manResizing = true; m_manResize0 = u.mouse; m_manW0 = W; m_manH0 = H; }
    if (m_manResizing) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float maxW = docked ? ds.x * 0.5f : std::max(300 * s, stageW - 40 * s);
            const float dx = handleRight ? u.mouse.x - m_manResize0.x : m_manResize0.x - u.mouse.x;
            m_manW = std::clamp(m_manW0 + dx, 260 * s, std::min(docked ? ds.x * 0.5f : 1400 * s, maxW));
            if (!docked) m_manH = std::round(std::clamp(m_manH0 + (u.mouse.y - m_manResize0.y), 160 * s, m_stage.w - y0 - 8 * s));
        } else { m_manResizing = false; savePrefs(); }
    }
    u.dl->AddRectFilled(ra, rb, (rh || m_manResizing) ? C_LYEL : IM_COL32(0, 0, 0, 89), 8 * s);
    if (rh || m_manResizing) ImGui::SetMouseCursor(handleRight ? ImGuiMouseCursor_ResizeNWSE : ImGuiMouseCursor_ResizeNESW);
    const ImU32 hc = (rh || m_manResizing) ? IM_COL32(0x1b, 0x15, 0x00, 255) : C_DIM;
    if (handleRight) svg::path(u.dl, "M14 14 L8 8 M14 14 L14 9 M14 14 L9 14 M2 2 L8 8", ImVec2(ra.x + 4 * s, ra.y + 4 * s), s, hc, 1.8f * s);
    else svg::path(u.dl, "M2 14 L8 8 M2 14 L2 9 M2 14 L7 14 M14 2 L8 8", ImVec2(ra.x + 4 * s, ra.y + 4 * s), s, hc, 1.8f * s);
}

// #mbig (scene pages, vertical book) and #pdfbig (the PDF, large)
void BrickApp::uiManualBig() {
    if (!m_manBig && !m_pdfBig) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    auto roundBtn = [&](ImVec2 c, float r, const char* label, float px) {
        ImVec2 a(c.x - r, c.y - r), b(c.x + r, c.y + r);
        const bool h = u.hover(a, b);
        u.dl->AddCircleFilled(c, r, h ? C_CARD_HI : C_PANEL_SOLID, 32);
        u.dl->AddCircle(c, r, C_LINE, 32, 1.0f);
        const float lw = u.textW(px, label);
        u.text(ImVec2(c.x - lw * 0.5f, c.y - u.fs(px) * 0.5f), px, C_TEXT, label);
        return h && u.pressed;
    };
    if (m_pdfBig) {
        u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(8, 10, 14, 209));
        const float w = std::min(1000 * s, d.x * 0.94f);
        ImVec2 a((d.x - w) * 0.5f, 16 * s), b(a.x + w, d.y - 40 * s);
        pdfView(u, a, b, m_bigScroll, false);
        if (roundBtn(ImVec2(d.x - 14 * s - 19 * s, 12 * s + 19 * s), 19 * s, "✕", 14) || ImGui::IsKeyPressed(ImGuiKey_Escape)) m_pdfBig = false;
        return;
    }
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(8, 10, 14, 140));
    const float w = std::min(760 * s, d.x * 0.92f);
    const int total = (int)m_bricks.size();
    m_manRight = std::clamp(m_manRight, 2, std::max(2, total));
    const int l = m_manRight - 1, r = m_manRight;
    const float h1 = manualPage(u, ImVec2(0, 0), w, l, 0, true, false), h2 = manualPage(u, ImVec2(0, 0), w, r, 1, true, false);
    const float content = h1 + 12 * s + h2, viewH = std::min(content, d.y * 0.92f);
    ImVec2 a((d.x - w) * 0.5f, (d.y - viewH) * 0.5f), b(a.x + w, a.y + viewH);
    if (u.hover(a, b) && ImGui::GetIO().MouseWheel != 0) m_bigScroll -= ImGui::GetIO().MouseWheel * 80 * s;
    dragScroll(u, a, b, m_bigScroll, 4);
    m_bigScroll = std::clamp(m_bigScroll, 0.0f, std::max(0.0f, content - viewH));
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 24 * s), ImVec2(b.x, b.y + 24 * s), IM_COL32(0, 0, 0, 60), 8 * s);   // drop-shadow
    u.dl->PushClipRect(a, b, true);
    manualPage(u, ImVec2(a.x, a.y - m_bigScroll), w, l, 0, true, true);
    manualPage(u, ImVec2(a.x, a.y - m_bigScroll + h1 + 12 * s), w, r, 1, true, true);
    u.dl->PopClipRect();
    bool close = false;
    if (roundBtn(ImVec2(b.x + 10 * s - 17 * s, a.y - 14 * s + 17 * s), 17 * s, "✕", 13)) close = true;
    if (roundBtn(ImVec2(a.x - 22 * s + 22 * s, (a.y + b.y) * 0.5f), 22 * s, "‹", 22)) { m_manRight -= 1; return; }
    if (roundBtn(ImVec2(b.x + 22 * s - 22 * s, (a.y + b.y) * 0.5f), 22 * s, "›", 22)) { m_manRight += 1; return; }
    if (close || ImGui::IsKeyPressed(ImGuiKey_Escape) || (u.pressed && !u.hover(a, b) && !close)) m_manBig = false;
}


// --------------------------------------------------------------------------- dockable panels (web: grip pointer events)
// Returns true while the panel is being dragged out (the caller stops drawing its content this frame).
bool BrickApp::gripPanel(int id, PanelState& P, ImVec2 ga, ImVec2 gb, ImVec2 panelTopLeft) {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 m = io.MousePos;
    const float s = m_ui;
    if (m_grip.id < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m.x >= ga.x && m.y >= ga.y && m.x < gb.x && m.y < gb.y) {
        m_grip = GripDrag();
        m_grip.id = id;
        m_grip.start = m;
        m_grip.ox = m.x - panelTopLeft.x;
        m_grip.oy = m.y - panelTopLeft.y;
    }
    if (m_grip.id != id) return false;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (!m_grip.moved && std::hypot(m.x - m_grip.start.x, m.y - m_grip.start.y) > 6) {
            m_grip.moved = true;
            if (id == 3) m_manDock = 0;   // (the Notice leaves its dock)
            if (!P.floating) {
                m_grip.ox = std::min(m.x - panelTopLeft.x, 120 * s);
                m_grip.oy = 8 * s;
                P.floating = true;
                P.collapsed = false;
            }
        }
        if (m_grip.moved) { P.x = m.x - m_grip.ox; P.y = m.y - m_grip.oy; }
        return false;
    }
    // released: a click folds, a drop on the dock zone docks
    const ImVec2 d = io.DisplaySize;
    if (!m_grip.moved) P.collapsed = !P.collapsed;
    else {
        const char* t = id == 0 ? (m.y > d.y - 80 * s ? "bottom" : nullptr) : (m.x < 90 * s ? "left" : m.x > d.x - 90 * s ? "right" : nullptr);
        if (t) { P.floating = false; if (id == 1) m_optsRight = std::string(t) == "right"; if (id == 3) m_manDock = std::string(t) == "right" ? 2 : 1; }
    }
    m_grip = GripDrag();
    savePrefs();
    return false;
}

void BrickApp::drawDropzone() {
    if (m_grip.id < 0 || !m_grip.moved) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 d = ImGui::GetIO().DisplaySize, m = ImGui::GetIO().MousePos;
    const float s = m_ui;
    const char* t = m_grip.id == 0 ? (m.y > d.y - 80 * s ? "bottom" : nullptr) : (m.x < 90 * s ? "left" : m.x > d.x - 90 * s ? "right" : nullptr);
    ImVec2 a, b;
    if (m_grip.id == 0) { a = ImVec2(8 * s, d.y - 74 * s); b = ImVec2(d.x - 8 * s, d.y - 8 * s); }
    else {
        const bool right = t && std::string(t) == "right";
        a = ImVec2(right ? d.x - 96 * s : 6 * s, 60 * s);
        b = ImVec2(a.x + 90 * s, d.y - 80 * s);
    }
    const float op = t ? 1.0f : 0.45f;
    dl->AddRectFilled(a, b, IM_COL32(247, 197, 30, (int)(36 * op)), 10 * s);
    // 2 px dashed accent border
    auto dash = [&](ImVec2 p, ImVec2 q) {
        const float len = std::hypot(q.x - p.x, q.y - p.y);
        for (float u0 = 0; u0 < len; u0 += 10 * s) {
            const float u1 = std::min(len, u0 + 6 * s);
            dl->AddLine(ImVec2(p.x + (q.x - p.x) * u0 / len, p.y + (q.y - p.y) * u0 / len), ImVec2(p.x + (q.x - p.x) * u1 / len, p.y + (q.y - p.y) * u1 / len), IM_COL32(247, 197, 30, (int)(255 * op)), 2 * s);
        }
    };
    dash(a, ImVec2(b.x, a.y));
    dash(ImVec2(b.x, a.y), b);
    dash(b, ImVec2(a.x, b.y));
    dash(ImVec2(a.x, b.y), a);
}


// --------------------------------------------------------------------------- drag scrolling of the library rows
// A press in the row starts a gesture; after ~7 px its direction decides, locked until the release. The cards (row 0,
// Wzx 06/11/2026): upwards within 160 degrees (80 either side of straight up) = the card is taken (its thumbnail follows
// the pointer at once, the brick in hand when it leaves the tray), any other direction = the row slides with the
// pointer and keeps its speed on release (decelerating, as the touch scrolling of the web). The other rows: the
// dominant direction (sideways = slide). Returns true while the row is being dragged.
bool BrickApp::swipeRow(int id, ImVec2 a, ImVec2 b, float& scroll, float& vel, float maxScroll, bool live, bool vertical) {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 m = io.MousePos;
    const float s = m_ui, dt = std::max(1e-3f, io.DeltaTime);
    if (m_swipeDragged == id && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) m_swipeDragged = -1;
    if (live && m_swipe.id < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m.x >= a.x && m.y >= a.y && m.x < b.x && m.y < b.y) {
        m_swipe = Swipe();
        m_swipe.id = id;
        m_swipe.start = m_swipe.last = m;
        vel = 0;
    }
    bool dragging = false;
    if (m_swipe.id == id) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float dx = m.x - m_swipe.start.x, dy = m.y - m_swipe.start.y;
            if (m_swipe.dir == 0 && dx * dx + dy * dy > 49 * s * s) {
                if (id == 0 && vertical) m_swipe.dir = std::fabs(dx) > std::fabs(dy) ? 2 : 1;   // (sideways: the part toward the view; up / down: the scroll)
                else if (id == 0) m_swipe.dir = (dy < 0 && std::fabs(dx) <= -dy * 5.671f) ? 2 : 1;   // tan(80) = 5.671
                else m_swipe.dir = std::fabs(dx) > std::fabs(dy) ? 1 : 2;
            }
            if (m_swipe.dir == 1) {
                const float step = vertical ? m.y - m_swipe.last.y : m.x - m_swipe.last.x;
                scroll -= step;
                vel = vel * 0.6f + (-step / dt) * 0.4f;   // smoothed speed (px / s)
                dragging = true;
                ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            }
            m_swipe.last = m;
        } else {
            if (m_swipe.dir != 1) vel = 0;
            else m_swipeDragged = id;   // (the release of this gesture is not a click)
            m_swipe = Swipe();
        }
    } else if (vel != 0) {   // inertia
        scroll += vel * dt;
        vel *= std::exp(-dt * 4.0f);
        if (std::fabs(vel) < 20 * s) vel = 0;
    }
    if (scroll < 0) { scroll = 0; vel = 0; }
    if (scroll > maxScroll) { scroll = maxScroll; vel = 0; }
    return dragging;
}

// --------------------------------------------------------------------------- Réglages › Thème (BrickTheme.h)
// As PLUM's Réglages › UI (plum-themes-kit README § 6): a card per theme with a small preview drawn with its own tokens,
// Clair / Sombre / Système, the « couleur du thème » (tint of the greys) and the accent, remembered per theme.

// the tabs of the settings: Réglages | Thème (a segmented track, the active one in the accent, Sommet's pill)
bool BrickApp::settingsTabs(Ui& u, ImVec2 p) {
    const float s = u.s, h = 24 * s;
    static const char* kTabs[2] = {"Réglages", "Thème"};
    float w[2], total = 0;
    for (int i = 0; i < 2; ++i) { w[i] = u.textW(12.5f, kTabs[i], true) + 26 * s; total += w[i]; }
    u.dl->AddRectFilled(p, ImVec2(p.x + total + 4 * s, p.y + h + 4 * s), C_CARD, rb(8 * s, h + 4 * s));
    float x = p.x + 2 * s;
    bool clicked = false;
    for (int i = 0; i < 2; ++i) {
        const ImVec2 a(x, p.y + 2 * s), b(x + w[i], p.y + 2 * s + h);
        const bool on = m_setTab == i, hv = u.hover(a, b);
        if (on) u.dl->AddRectFilled(a, b, C_ACCENT, rb(7 * s, h));
        else if (hv) u.dl->AddRectFilled(a, b, C_CARD_HI, rb(7 * s, h));
        const float tw = u.textW(12.5f, kTabs[i], true);
        u.text(ImVec2((a.x + b.x - tw) * 0.5f, a.y + (h - u.fs(12.5f)) * 0.5f), 12.5f, on ? C_INK : C_TEXT, kTabs[i], true);
        if (hv && u.pressed) {
            if (!on) { m_setTab = i; savePrefs(); }
            clicked = true;
            m_tabClicked = true;
        }
        x += w[i];
    }
    return clicked;
}

// the small preview of a theme: the top bar, the 3D stage (Sommet: its painted mountains), the HUD bubbles, the library
static void themePreview(ImDrawList* dl, float s, const theme::Tokens& t, ImVec2 a, ImVec2 b, ImTextureID banner) {
    const float W = b.x - a.x, H = b.y - a.y;
    dl->PushClipRect(a, b, true);
    const float barH = H * 0.13f, libY = a.y + H * 0.70f;
    const ImU32 opaquePanel = (t.panel & 0x00ffffff) | 0xff000000;
    // the stage
    if (t.stage.light) {
        const ImU32 top = theme::col(t.stage.top), hor = theme::col(t.stage.horizon), flo = theme::col(t.stage.bottom);
        const float hy = a.y + H * 0.42f;
        dl->AddRectFilledMultiColor(a, ImVec2(b.x, hy), top, top, hor, hor);
        dl->AddRectFilledMultiColor(ImVec2(a.x, hy), ImVec2(b.x, libY), hor, hor, flo, flo);
    } else {
        dl->AddRectFilled(a, ImVec2(b.x, libY), IM_COL32(0x11, 0x12, 0x14, 255));
        for (int k = 1; k < 7; ++k) {   // the dark floor and its grid
            const float y = a.y + H * 0.42f + k * k * 1.4f * s;
            if (y < libY) dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), IM_COL32(0x9a, 0xa3, 0xb8, 40), 1.0f);
        }
    }
    if (banner) {   // Sommet: the painted mountains (banner-wide, 2:1, the figure on the right, a white fade on the left)
        const float ih = libY - a.y, iw = ih * 2.0f;
        dl->AddImage(banner, ImVec2(b.x - iw, a.y), ImVec2(b.x, libY), ImVec2(0, 0), ImVec2(1, 1), t.dark ? IM_COL32(200, 200, 205, 255) : IM_COL32_WHITE);
    }
    // a few bricks on the stage (LEGO red, yellow, white)
    const ImVec2 c(a.x + W * 0.36f, a.y + H * 0.50f);
    dl->AddRectFilled(ImVec2(c.x - 16 * s, c.y - 2 * s), ImVec2(c.x + 4 * s, c.y + 10 * s), IM_COL32(0xc9, 0x1a, 0x09, 255), 1.5f * s);
    dl->AddRectFilled(ImVec2(c.x - 2 * s, c.y - 13 * s), ImVec2(c.x + 18 * s, c.y - 2 * s), IM_COL32(0xf2, 0xcd, 0x37, 255), 1.5f * s);
    dl->AddRectFilled(ImVec2(c.x + 6 * s, c.y - 2 * s), ImVec2(c.x + 22 * s, c.y + 10 * s), IM_COL32(0xf4, 0xf4, 0xf4, 255), 1.5f * s);
    dl->AddRect(ImVec2(c.x - 3 * s, c.y - 14 * s), ImVec2(c.x + 19 * s, c.y - 1 * s), IM_COL32(255, 255, 255, 200), 2 * s, 0, 1.0f * s);
    // the bubbles of the selection HUD
    const float br = 4.2f * s;
    const ImVec2 bp[3] = {ImVec2(c.x + 8 * s, c.y - 24 * s), ImVec2(c.x - 14 * s, c.y - 8 * s), ImVec2(c.x + 30 * s, c.y - 8 * s)};
    for (int k = 0; k < 3; ++k) {
        const ImU32 bc = t.hudStyle == theme::HUD_LEGO ? (k == 0 ? IM_COL32(0xf2, 0xcd, 0x37, 255) : IM_COL32(0x00, 0x55, 0xbf, 255)) : (k == 0 ? t.progress : t.hudBtn);
        dl->AddCircleFilled(bp[k], br, bc, 16);
        if (t.hudStyle == theme::HUD_FLAT && k) dl->AddCircle(bp[k], br, t.line, 16, 1.0f);
    }
    // the top bar: the logo, a few buttons, one active
    dl->AddRectFilled(a, ImVec2(b.x, a.y + barH), opaquePanel);
    dl->AddLine(ImVec2(a.x, a.y + barH), ImVec2(b.x, a.y + barH), t.line, 1.0f);
    const float bh = barH * 0.6f, by = a.y + barH * 0.2f;
    dl->AddRectFilled(ImVec2(a.x + 4 * s, by), ImVec2(a.x + 4 * s + bh, by + bh), t.accent, 1.5f * s);
    dl->AddRectFilled(ImVec2(a.x + 8 * s + bh, by + bh * 0.3f), ImVec2(a.x + 8 * s + bh + 22 * s, by + bh * 0.7f), t.text, 1.0f * s);
    for (int k = 0; k < 5; ++k) {
        const float x = b.x - 6 * s - (k + 1) * (bh + 3 * s);
        dl->AddRectFilled(ImVec2(x, by), ImVec2(x + bh, by + bh), k == 3 ? t.accent : t.card, t.pill ? bh * 0.5f : 1.5f * s);
    }
    // the library: tabs, thumbnails
    dl->AddRectFilled(ImVec2(a.x, libY), b, opaquePanel);
    dl->AddLine(ImVec2(a.x, libY), ImVec2(b.x, libY), t.line, 1.0f);
    float tx = a.x + 6 * s;
    for (int k = 0; k < 4; ++k) {
        const float tw = (k == 1 ? 18 : 14) * s;
        if (k == 1) dl->AddRectFilled(ImVec2(tx - 3 * s, libY + 3 * s), ImVec2(tx + tw + 3 * s, libY + 10 * s), t.card, 3.5f * s);
        dl->AddRectFilled(ImVec2(tx, libY + 5.5f * s), ImVec2(tx + tw, libY + 7.5f * s), k == 1 ? t.accentTxt : t.dim, 1.0f * s);
        tx += tw + 8 * s;
    }
    const float ty = libY + 14 * s, tsz = std::max(8 * s, b.y - ty - 5 * s);
    for (int k = 0; k < 10; ++k) {
        const float x = a.x + 6 * s + k * (tsz + 4 * s);
        if (x > b.x) break;
        dl->AddRectFilled(ImVec2(x, ty), ImVec2(x + tsz, ty + tsz), t.card, 2 * s);
        const ImU32 brick = t.dark ? IM_COL32(0xee, 0xee, 0xee, 255) : t.thumb;
        dl->AddRectFilled(ImVec2(x + tsz * 0.22f, ty + tsz * 0.35f), ImVec2(x + tsz * 0.78f, ty + tsz * 0.72f), brick, 1.0f * s);
    }
    dl->PopClipRect();
}

// a small word wrap for the descriptions of the cards
static void textWrap(Ui& u, ImVec2 p, float w, float px, ImU32 col, const char* t, int maxLines) {
    const std::string all = t;
    std::string line;
    float y = p.y;
    int lines = 0;
    size_t i = 0;
    while (lines < maxLines) {
        const size_t sp = all.find(' ', i);
        const std::string word = all.substr(i, sp == std::string::npos ? std::string::npos : sp - i);
        const std::string tryLine = line.empty() ? word : line + " " + word;
        if (!line.empty() && u.textW(px, tryLine.c_str()) > w) {
            u.text(ImVec2(p.x, y), px, col, line.c_str());
            y += u.fs(px) * 1.15f;
            ++lines;
            line = word;
        } else line = tryLine;
        if (sp == std::string::npos) break;
        i = sp + 1;
    }
    if (lines < maxLines && !line.empty()) u.text(ImVec2(p.x, y), px, col, line.c_str());
}

static float fontKOf(int font) { return font == theme::FONT_INSTRUMENT ? 1.22f * 1.22f / 1.33f * 1.02f : font == theme::FONT_GEIST ? 1.22f * 1.30f / 1.33f * 0.86f : 1.22f; }

void BrickApp::uiThemeTab(Ui& u, ImVec2 p0, ImVec2 p1) {
    const float s = u.s, pad = 14 * s, gap = 10 * s, rowH = 30 * s;
    const auto& all = theme::themes();
    float y = p0.y + pad + 20 * s + gap;
    bool changed = false;
    // ---- Thème: one card per theme
    u.text(ImVec2(p0.x + pad, y), 11, C_DIM, "Thème");
    y += 16 * s;
    const int n = (int)all.size();
    const int perRow = n > 4 ? 3 : n, nRows = (n + perRow - 1) / perRow;   // (6 themes: 2 rows of 3)
    const float cg = 8 * s, cw = (p1.x - p0.x - 2 * pad - (perRow - 1) * cg) / perRow, pvH = cw * (n > 4 ? 0.42f : 0.62f), ch = pvH + 54 * s;
    for (int k = 0; k < n; ++k) {
        const theme::Def& d = all[k];
        const ImVec2 a(p0.x + pad + (k % perRow) * (cw + cg), y + (k / perRow) * (ch + cg)), b(a.x + cw, a.y + ch);
        const bool on = m_themeId == d.id, hv = u.hover(a, b);
        const ThemeLook lk = m_themeLooks.count(d.id) ? m_themeLooks[d.id] : ThemeLook{};
        const bool dk = d.mode >= 0 ? d.mode == 1 : pcDarkNow();
        const theme::Tokens t = on ? m_th : theme::resolve(d.id, dk, lk.tint, lk.accent);
        u.dl->AddRectFilled(a, b, hv && !on ? C_CARD_HI : C_CARD, rr(10 * s));
        const ImVec2 pa(a.x + 5 * s, a.y + 5 * s), pb(b.x - 5 * s, a.y + 5 * s + pvH);
        themePreview(u.dl, s, t, pa, pb, std::string(d.id) == "sommet" ? themeTex("sommet/banner-wide.png") : 0);
        u.dl->AddRect(pa, pb, C_LINE, 0, 0, 1.0f);
        // the name in the theme's own font
        ImFont* f = m_gui->fontBold ? m_gui->fontBold : u.bold;
        if (t.font == theme::FONT_INSTRUMENT && g_fInstr[1]) f = g_fInstr[1];
        if (t.font == theme::FONT_GEIST && g_fGeist[1]) f = g_fGeist[1];
        u.dl->AddText(f, 13 * s * fontKOf(t.font), ImVec2(a.x + 8 * s, pb.y + 6 * s), C_TEXT, d.label);
        textWrap(u, ImVec2(a.x + 8 * s, pb.y + 25 * s), cw - 14 * s, 9.5f, C_DIM, d.desc, 2);
        if (on) u.dl->AddRect(ImVec2(a.x - 1.5f * s, a.y - 1.5f * s), ImVec2(b.x + 1.5f * s, b.y + 1.5f * s), C_ACCENT_TXT, rr(11 * s), 0, 2.0f * s);
        if (hv && u.pressed && (!on || m_ipadCreme)) { m_themeId = d.id; changed = true; if (m_ipad) { m_ipadCreme = false; savePrefs(); } }
    }
    y += nRows * ch + (nRows - 1) * cg + gap + 4 * s;
    if (m_ipad) {   // iPad: the cream touch kit 2.0 (galets crème, bulles brunes, Fredoka), kept as a theme of its own
        const ImVec2 a(p0.x + pad, y - gap + 4 * s), b(p1.x - pad, y - gap + 4 * s + 58 * s);
        const bool hv = u.hover(a, b);
        u.dl->AddRectFilled(a, b, hv ? IM_COL32(0xef, 0xe5, 0xd4, 255) : IM_COL32(0xf9, 0xf5, 0xeb, 255), rr(10 * s));
        for (int k = 0; k < 3; ++k) {   // three pebbles: the look in a glance
            const ImVec2 c(a.x + 24 * s + k * 26 * s, (a.y + b.y) * 0.5f);
            u.dl->AddCircleFilled(c, 10 * s, k == 1 ? IM_COL32(64, 46, 32, 140) : IM_COL32(255, 249, 241, 255), 24);
            u.dl->AddCircle(c, 10 * s, IM_COL32(0xd9, 0xcb, 0xb4, 255), 24, 1.0f);
        }
        u.dl->AddText(m_gui->fontBold ? m_gui->fontBold : u.bold, 13 * s * 1.22f, ImVec2(a.x + 100 * s, a.y + 10 * s), IM_COL32(0x57, 0x46, 0x38, 255), "Crème iPad");
        u.dl->AddText(u.font, 9.5f * s * 1.22f, ImVec2(a.x + 100 * s, a.y + 31 * s), IM_COL32(0x96, 0x86, 0x72, 255), "Le kit tactile crème : galets, bulles autour de la brique, arc de tenons, dock");
        if (m_ipadCreme) u.dl->AddRect(ImVec2(a.x - 1.5f * s, a.y - 1.5f * s), ImVec2(b.x + 1.5f * s, b.y + 1.5f * s), C_ACCENT_TXT, rr(11 * s), 0, 2.0f * s);
        if (hv && u.pressed) { m_ipadCreme = true; m_popup = 0; savePrefs(); return; }
        y += 58 * s + gap;
    }
    const int ti = std::max(0, theme::indexOf(m_themeId));
    const theme::Def& cur = all[ti];
    ThemeLook& look = m_themeLooks[m_themeId];
    const float colW = (p1.x - p0.x - 2 * pad - 2 * gap) / 2;
    {   // ---- Clair ou sombre (Système follows Windows) | Aperçu
        const float x0 = p0.x + pad;
        u.text(ImVec2(x0, y), 11, C_DIM, "Clair ou sombre");
        if (cur.mode >= 0) u.text(ImVec2(x0 + u.textW(11, "Clair ou sombre") + 6 * s, y), 11, C_FAINT, cur.mode == 1 ? "·  ce thème reste sombre" : "·  ce thème reste clair");
        const float x1 = p0.x + pad + colW + gap;
        u.text(ImVec2(x1, y), 11, C_DIM, "Aperçu");
        y += 16 * s;
        static const char* kModes[3] = {"Système", "Clair", "Sombre"};
        const float bw = (colW - 2 * 4 * s) / 3;
        for (int k = 0; k < 3; ++k) {
            const ImVec2 a(x0 + k * (bw + 4 * s), y), b(a.x + bw, y + rowH);
            const bool enabled = cur.mode < 0, on = enabled ? m_uiThemePref == k : (k == (cur.mode == 1 ? 2 : 1));
            const bool hv = enabled && u.hover(a, b);
            ImU32 bg = on ? C_ACCENT : (hv ? C_CARD_HI : C_CARD), fg = on ? C_INK : C_TEXT;
            if (!enabled) { bg = alphaOf(bg, 0.45f); fg = alphaOf(fg, 0.45f); }
            u.dl->AddRectFilled(a, b, bg, rb(7 * s, rowH));
            const float tw = u.textW(12, kModes[k], on);
            u.text(ImVec2((a.x + b.x - tw) * 0.5f, a.y + (rowH - u.fs(12)) * 0.5f), 12, fg, kModes[k], on);
            if (hv && u.pressed && !on) { m_uiThemePref = k; changed = true; }
        }
        // the active colours, as PLUM's « Couleurs actives »: the main button, a secondary one, a slider, the texts
        float bx = x1;
        const char* l1 = "Principal";
        const float w1 = u.textW(12, l1, true) + 24 * s;
        u.dl->AddRectFilled(ImVec2(bx, y), ImVec2(bx + w1, y + rowH), C_ACCENT, rb(7 * s, rowH));
        u.text(ImVec2(bx + 12 * s, y + (rowH - u.fs(12)) * 0.5f), 12, C_INK, l1, true);
        bx += w1 + 6 * s;
        const char* l2 = "Annuler";
        const float w2 = u.textW(12, l2) + 24 * s;
        u.dl->AddRectFilled(ImVec2(bx, y), ImVec2(bx + w2, y + rowH), C_CARD, rb(7 * s, rowH));
        if (g_lines) u.dl->AddRect(ImVec2(bx, y), ImVec2(bx + w2, y + rowH), C_LINE, rb(7 * s, rowH));
        u.text(ImVec2(bx + 12 * s, y + (rowH - u.fs(12)) * 0.5f), 12, C_TEXT, l2);
        bx += w2 + 12 * s;
        const float cy = y + rowH * 0.5f, ex = p1.x - pad - 8 * s;
        if (ex - bx > 40 * s) {
            u.dl->AddRectFilled(ImVec2(bx, cy - 3 * s), ImVec2(ex, cy + 3 * s), C_CARD, 3 * s);
            u.dl->AddRectFilled(ImVec2(bx, cy - 3 * s), ImVec2(bx + (ex - bx) * 0.62f, cy + 3 * s), C_PROGRESS, 3 * s);
            u.dl->AddCircleFilled(ImVec2(bx + (ex - bx) * 0.62f, cy), 7 * s, C_PROGRESS);
        }
        y += rowH + 6 * s;
        float tx = x1;
        tx += u.text(ImVec2(tx, y), 11, C_TEXT, "Texte").x + 10 * s;
        tx += u.text(ImVec2(tx, y), 11, C_DIM, "secondaire").x + 10 * s;
        tx += u.text(ImVec2(tx, y), 11, C_FAINT, "discret").x + 10 * s;
        u.text(ImVec2(tx, y), 11, C_ACCENT_TXT, "accent", true);
    }
    y += 16 * s + gap;
    // ---- Couleur du thème (the tint of the greys) | Couleur d'accent
    auto chips = [&](float x0, float yy, const char* title, int count, const std::function<const char*(int)>& name,
                     const std::function<uint32_t(int)>& rgb, int& sel) {
        u.text(ImVec2(x0, yy), 11, C_DIM, title);
        yy += 16 * s;
        float x = x0;
        const float chH = 26 * s;
        for (int k = 0; k < count; ++k) {
            const char* nm = name(k);
            const float w = 22 * s + u.textW(11.5f, nm, sel == k) + 10 * s;
            if (x + w > x0 + colW && x > x0) { x = x0; yy += chH + 5 * s; }
            const ImVec2 a(x, yy), b(x + w, yy + chH);
            const bool on = sel == k, hv = u.hover(a, b);
            u.dl->AddRectFilled(a, b, on ? C_PANEL_SOLID : (hv ? C_CARD_HI : C_CARD), chH * 0.5f);
            if (on) u.dl->AddRect(a, b, C_TEXT, chH * 0.5f, 0, 1.5f * s);
            u.dl->AddCircleFilled(ImVec2(a.x + 12 * s, (a.y + b.y) * 0.5f), 5 * s, theme::col(rgb(k)), 20);
            u.text(ImVec2(a.x + 21 * s, a.y + (chH - u.fs(11.5f)) * 0.5f), 11.5f, C_TEXT, nm, on);
            if (hv && u.pressed && !on) { sel = k; changed = true; }
            x += w + 5 * s;
        }
        return yy + chH;
    };
    const float yA = chips(p0.x + pad, y, "Couleur du thème", (int)cur.tints.size(), [&](int k) { return cur.tints[k].name; },
                           [&](int k) { return cur.tints[k].swatch; }, look.tint);
    const float yB = chips(p0.x + pad + colW + gap, y, "Couleur d'accent", (int)cur.accents.size(), [&](int k) { return cur.accents[k].name; },
                           [&](int k) { return cur.accents[k].rgb; }, look.accent);
    y = std::max(yA, yB) + gap;
    u.text(ImVec2(p0.x + pad, y), 10, C_FAINT, "Chaque thème garde sa couleur et son accent. Minimal et Sommet viennent du kit PLUM.");
    y += 14 * s + pad;
    m_themeTabH = y - p0.y;
    if (changed) {
        m_thKey.clear();   // (resolved again next frame; the 3D stage follows)
        savePrefs();
    }
}

// --------------------------------------------------------------------------- settings (native addition, gear button)
// UI scale (text and panels, 4K), thumbnail size, real time quality / path tracing, ambient occlusion of the HQ mode,
// tone mapping, voxels, animations, automatic save. Saved in brickit_prefs.json.

// a text field of the cloud panel (an ImGui input on a card of the theme)
static bool cloudField(Ui& u, const char* id, ImVec2 a, ImVec2 b, char* buf, size_t n, const char* hint, bool password, float s, ImU32 card, ImU32 text, ImU32 faint) {
    u.dl->AddRectFilled(a, b, card, 7 * s);
    ImGui::SetNextWindowPos(ImVec2(a.x + 10 * s, a.y + 2 * s));
    ImGui::SetNextWindowSize(ImVec2(b.x - a.x - 20 * s, b.y - a.y - 4 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2 * s, 5 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(text));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(faint));
    ImGui::Begin(id, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImGui::SetNextItemWidth(-1);
    const bool enter = ImGui::InputTextWithHint("##f", hint, buf, n, ImGuiInputTextFlags_EnterReturnsTrue | (password ? ImGuiInputTextFlags_Password : 0));
    ImGui::End();
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
    return enter;
}


void BrickApp::uiSettings() {
    // Réglages, rangés par sections (Wzx 13/11/2026: « revoit tout le panneau des settings, pour mieux organiser et
    // synthétiser tous les paramètres, fais une UX claire »): the sections on the left, the active one on the right with
    // its groups (a small title and a line), each setting a row of segmented buttons or a slider.
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float pad = 14 * s, rowH = 30 * s, gap = 10 * s;
    enum { S_UI, S_LIB, S_BUILD, S_RENDER, S_CAM, S_LIGHT, S_THEME, S_FILES, S_CLOUD, S_N };
    static_assert(S_CLOUD == kSetCloud, "Réglages › Cloud");
    struct Sec { const char* name; const char* desc; bool pcOnly; };
    static const Sec kSec[S_N] = {
        {"Interface", "Taille, style et aides à l'écran", false},
        {"Bibliothèque", "Les cartes des pièces et leurs miniatures", false},
        {"Construction", "La brique en main et la sélection", false},
        {"Rendu", "Moteur, qualité et image", true},
        {"Caméra", "Profondeur de champ et mise au point", true},
        {"Lumière", "HDRI, illumination globale, grille", true},
        {"Thème", "Couleurs et style de l'application", false},   // (iPad too: the Crème iPad kit is one of its themes)
        {"Fichiers", "Sauvegarde et réglages par défaut", false},
        {"Cloud", "Tes scènes entre l'iPad et le PC", false},
    };
    if (m_setTab == 1) { m_setSec = S_THEME; m_setTab = 0; }   // (--settings-tab 1, the former Thème tab)
    if (m_setSec < 0 || m_setSec >= S_N || (m_ipad && kSec[m_setSec].pcOnly)) m_setSec = S_UI;
    const int sec = m_setSec;
    const float sideW = 156 * s, CW = (sec == S_THEME ? 640 : 380) * s;
    const float W2 = sideW + CW;
    // the height: the sections column, or the content of the active one as drawn last frame (scrolled when taller)
    int nSec = 0;
    for (int i = 0; i < S_N; ++i) if (!(m_ipad && kSec[i].pcOnly)) ++nSec;
    const float itemH = 34 * s, sideH = pad + 24 * s + gap + nSec * (itemH + 2 * s) + pad;
    const float maxH = d.y - 8 * s - m_popupAt.y;
    const float contentH = m_setSecH[sec] > 0 ? m_setSecH[sec] : 420 * s;
    const float H = std::min(maxH, std::max(sideH, contentH));
    ImVec2 p0(std::max(4 * s, std::min(m_popupAt.x, d.x - W2 - 10 * s)), m_popupAt.y), p1(p0.x + W2, p0.y + H);
    u.dl->AddRectFilled(ImVec2(p0.x, p0.y + 10 * s), ImVec2(p1.x, p1.y + 10 * s), m_th.shadow, rr(12 * s));
    u.dl->AddRectFilled(p0, p1, C_PANEL_SOLID, rr(12 * s));
    u.dl->AddRect(p0, p1, C_LINE, rr(12 * s));
    m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    const bool inPanel = u.hover(p0, p1);
    // ---- the sections
    {
        u.dl->AddRectFilled(p0, ImVec2(p0.x + sideW, p1.y), C_CARD & 0x55ffffff, rr(12 * s), ImDrawFlags_RoundCornersLeft);
        u.text(ImVec2(p0.x + pad, p0.y + pad), 15, C_TEXT, "Réglages", true);
        float y = p0.y + pad + 24 * s + gap;
        for (int i = 0; i < S_N; ++i) {
            if (m_ipad && kSec[i].pcOnly) continue;
            const ImVec2 a(p0.x + 8 * s, y), b(p0.x + sideW - 8 * s, y + itemH);
            const bool on = sec == i, hv = u.hover(a, b);
            if (on) u.dl->AddRectFilled(a, b, C_CARD_HI, rb(8 * s, itemH));
            else if (hv) u.dl->AddRectFilled(a, b, C_CARD, rb(8 * s, itemH));
            if (on) u.dl->AddRectFilled(ImVec2(a.x + 4 * s, a.y + 9 * s), ImVec2(a.x + 7 * s, b.y - 9 * s), C_ACCENT, 2 * s);
            u.text(ImVec2(a.x + 16 * s, a.y + (itemH - u.fs(12.5f)) * 0.5f), 12.5f, on ? C_TEXT : C_DIM, kSec[i].name, on);
            if (hv && u.pressed && !on) { m_setSec = i; m_setScroll = 0; m_tabClicked = true; }
            y += itemH + 2 * s;
        }
        u.dl->AddLine(ImVec2(p0.x + sideW, p0.y), ImVec2(p0.x + sideW, p1.y), C_LINE, 1.0f);
    }
    const float x0 = p0.x + sideW, x1 = p1.x, IW = CW - 2 * pad;
    // scrolling (a content taller than the screen)
    const float scrollMax = std::max(0.0f, contentH - H);
    if (inPanel && scrollMax > 0 && ImGui::GetIO().MouseWheel != 0) m_setScroll -= ImGui::GetIO().MouseWheel * 60 * s;
    m_setScroll = std::clamp(m_setScroll, 0.0f, scrollMax);
    u.dl->PushClipRect(ImVec2(x0, p0.y + 1), ImVec2(x1, p1.y - 1), true);
    const float top = p0.y - m_setScroll;
    if (sec == S_THEME) {
        u.text(ImVec2(x0 + pad, top + pad), 15, C_TEXT, kSec[sec].name, true);
        uiThemeTab(u, ImVec2(x0, top), ImVec2(x1, p1.y));
        m_setSecH[sec] = m_themeTabH;
        u.dl->PopClipRect();
        if (u.pressed && !inPanel && u.mouse.y > (m_studioUi ? 46 : 30) * s && !m_tabClicked) m_popup = 0;
        m_tabClicked = false;
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_popup = 0;
        return;
    }
    float y = top + pad;
    u.text(ImVec2(x0 + pad, y), 15, C_TEXT, kSec[sec].name, true);
    {
        const float tw = u.textW(15, kSec[sec].name, true);
        u.text(ImVec2(x0 + pad + tw + 10 * s, y + 3 * s), 10.5f, C_DIM, kSec[sec].desc);
    }
    y += 20 * s + gap;
    auto hov = [&](ImVec2 a, ImVec2 b) { return inPanel && u.hover(a, b); };
    // a group: its small title and a line
    bool firstGroup = true;
    auto group = [&](const char* name) {
        if (!firstGroup) y += 6 * s;
        firstGroup = false;
        u.text(ImVec2(x0 + pad, y), 10.5f, C_ACCENT_TXT, name, true);
        const float tw = u.textW(10.5f, name, true);
        u.dl->AddLine(ImVec2(x0 + pad + tw + 8 * s, y + u.fs(10.5f) * 0.55f), ImVec2(x1 - pad, y + u.fs(10.5f) * 0.55f), C_LINE, 1.0f);
        y += 20 * s;
    };
    // a row of segmented buttons: its index clicked, -1
    auto seg = [&](const char* title, std::initializer_list<const char*> labs, int on, float fs = 12) -> int {
        u.text(ImVec2(x0 + pad, y), 11, C_DIM, title);
        y += 16 * s;
        const int n = (int)labs.size();
        const float bw = (IW - (n - 1) * 4 * s) / n;
        int k = 0, hit = -1;
        for (const char* lab : labs) {
            ImVec2 a(x0 + pad + k * (bw + 4 * s), y), b(a.x + bw, y + rowH);
            const bool o = k == on, h = hov(a, b);
            u.dl->AddRectFilled(a, b, o ? C_ACCENT : (h ? C_CARD_HI : C_CARD), rb(7 * s, rowH));
            const float tw = u.textW(fs, lab, o);
            u.text(ImVec2((a.x + b.x) * 0.5f - tw * 0.5f, a.y + (rowH - u.fs(fs)) * 0.5f), fs, o ? C_INK : C_TEXT, lab, o);
            if (h && u.pressed) hit = k;
            ++k;
        }
        y += rowH + gap;
        return hit;
    };
    // the rows of the former panel: their click handled below (clickRow, clickOpt), by their id
    int clickRow = -1, clickOpt = -1;
    auto row = [&](const char* title, std::initializer_list<const char*> labs, int on, int id) {
        const int k = seg(title, labs, on);
        if (k >= 0 && k != on) { clickRow = id; clickOpt = k; }
    };
    auto yesNo = [&](const char* title, bool v, int id) { row(title, {"Oui", "Non"}, v ? 0 : 1, id); };
    auto slider = [&](int id, const char* label, float& v, float lo, float hi, const char* fmt, float shown) {
        char vb[32];
        snprintf(vb, sizeof(vb), fmt, shown);
        u.text(ImVec2(x0 + pad, y), 11, C_DIM, label);
        const float vw = u.textW(11, vb, false);
        u.text(ImVec2(x1 - pad - vw, y), 11, C_TEXT, vb);
        y += 16 * s;
        const float sh = 22 * s;   // (a slider: lower than a row of buttons)
        ImVec2 a(x0 + pad, y), b(x1 - pad, y + sh);
        const bool h = hov(a, b);
        if (h && u.pressed) m_setDrag = id;
        bool changed = false;
        if (m_setDrag == id) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                const float t = std::clamp((u.mouse.x - a.x) / std::max(1.0f, b.x - a.x), 0.0f, 1.0f);
                const float nv = lo + t * (hi - lo);
                if (std::fabs(nv - v) > 1e-5f) { v = nv; changed = true; }
            } else { m_setDrag = -1; savePrefs(); }
        }
        const float t = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f), cy = y + sh * 0.5f;
        u.dl->AddRectFilled(ImVec2(a.x, cy - 3 * s), ImVec2(b.x, cy + 3 * s), C_CARD, 3 * s);
        u.dl->AddRectFilled(ImVec2(a.x, cy - 3 * s), ImVec2(a.x + (b.x - a.x) * t, cy + 3 * s), C_PROGRESS, 3 * s);
        u.dl->AddCircleFilled(ImVec2(a.x + (b.x - a.x) * t, cy), (h || m_setDrag == id ? 9 : 8) * s, C_PROGRESS);
        y += sh + gap;
        return changed;
    };
    auto note = [&](const char* txt) {   // a short explanation under a row
        y -= gap - 2 * s;
        u.text(ImVec2(x0 + pad, y), 10, C_FAINT, txt);
        y += 14 * s + gap;
    };
    bool lookChanged = false;
    static const float kScales[5] = {0.75f, 1.0f, 1.25f, 1.5f, 2.0f};
    switch (sec) {
    case S_UI: {
        group("Affichage");
        int sc = 1;
        for (int i = 0; i < 5; ++i) if (std::fabs(m_uiScaleUser - kScales[i]) < 0.01f) sc = i;
        row("Échelle de l'interface", {"75 %", "100 %", "125 %", "150 %", "200 %"}, sc, 0);
        row("Style de l'interface", {"Studio", "Classique"}, m_studioUi ? 0 : 1, 18);
        row("Icône de l'app", {"A : viseur", "B : brique seule"}, m_appIcon, 25);
        group("Aides");
        yesNo("HUD de sélection", m_selHud, 16);
        if (m_ipad) yesNo("Anneau de la main gauche (tourner, basculer)", m_touchHud, 14);
        yesNo("Animations des briques", m_animOn, 7);
        break;
    }
    case S_LIB:
        group("Cartes");
        row("Présentation des cartes", {"Image", "Fiche"}, m_stCardMode, 19);
        row("Taille des miniatures", {"Petites", "Moyennes", "Grandes"}, m_thumbSize, 1);
        group("Filtre");
        row("Pièces décorées (stickers, motifs)", {"Masquées", "Affichées"}, m_hideDeco ? 0 : 1, 20);
        break;
    case S_BUILD: {
        group("Brique en main");
        note("L'aperçu translucide de l'endroit où la brique va se poser.");
        row("Couleur de l'aperçu", {"Jaune", "Blanc", "Bleu", "Vert", "Rose", "Brique"}, m_ghostCol, 21);
        const int ga = m_ghostAlphaPref < 0.2f ? 0 : m_ghostAlphaPref < 0.4f ? 1 : m_ghostAlphaPref < 0.6f ? 2 : 3;
        row("Opacité de l'aperçu", {"15 %", "32 %", "50 %", "75 %"}, ga, 22);
        row("Lueur de l'aperçu", {"Aucune", "Douce", "Normale", "Forte"}, m_ghostGlow, 23);
        if (!m_ipad) {
            group("Sélection");
            slider(106, "Contour de sélection", m_selOutline, 0.0f, 1.0f, "%.2f", m_selOutline);
            slider(107, "Voile de sélection", m_selFill, 0.0f, 0.5f, "%.2f", m_selFill);
        }
        group("Connecteurs");
        row("Voxels (V)", {"Affichés", "Masqués"}, m_showVox ? 0 : 1, 6);
        note("Tenons jaunes, pins cyan, axes magenta.");
        break;
    }
    case S_RENDER: {
        group("Moteur");
        row("Rendu", {"Rapide", "HQ", "PT", "RC"}, m_rcMode ? 3 : pathTraced() ? 2 : m_quality == 1 ? 1 : 0, 2);
        note("Rapide : raster simple · HQ : GI, SSS · PT : path tracing · RC : Radiance Cascades");
        group("Qualité");
        const int rs = m_renderScale > 0.95f ? 0 : std::fabs(m_renderScale - 0.85f) < 0.01f ? 1 : std::fabs(m_renderScale - 0.75f) < 0.01f ? 2 : 3;
        row("Échelle de rendu", {"100 %", "85 %", "75 %", "67 %"}, rs, 11);
        yesNo("Anti-aliasing au repos (HQ)", m_stillAA, 12);
        yesNo("Occlusion HDRI (HQ)", m_ssaoOn, 3);
        yesNo("Arêtes arrondies (biseau)", m_rs.bevel, 24);
        group("Radiance Cascades");
        row("Anti-aliasing", {"MSAA 4x", "MSAA 8x", "AAA"}, m_rs.aaQuality, 9);
        row("Ombres", {"Basse", "Moyenne", "Haute", "AAA"}, m_rs.shadowQuality <= 1 ? 0 : m_rs.shadowQuality - 1, 10);
        group("Image");
        row("Transformation de vue", {"ACES", "AgX", "Std"}, m_rs.tonemapper == 3 ? 2 : m_rs.tonemapper, 4);
        slider(109, "Bloom", m_bloom, 0.0f, 1.0f, "%.2f", m_bloom);
        break;
    }
    case S_CAM: {
        group("Objectif (HQ)");
        slider(108, "Profondeur de champ", m_dof, 0.0f, 1.0f, "%.2f", m_dof);
        const int k = seg("Mise au point", {"Auto (viseur)", "Sélection", "Manuel"}, m_focusMode, 11);
        if (k >= 0 && k != m_focusMode) {
            m_focusMode = k;
            if (k == 2) m_focusManual = std::clamp(m_focusCur > 0 ? m_focusCur : m_scene.camera.distance, 2.0f, 400.0f);
            savePrefs();
        }
        if (m_focusMode == 2) slider(110, "Distance de mise au point", m_focusManual, 2.0f, 400.0f, "%.0f cm", m_focusManual);
        if (m_focusMode == 0) {
            const int r = seg("Viseur de mise au point", {"Affiché", "Masqué"}, m_focusReticle ? 0 : 1);
            if (r >= 0 && (r == 0) != m_focusReticle) { m_focusReticle = r == 0; savePrefs(); }
        }
        break;
    }
    case S_LIGHT: {
        group("Environnement");
        u.text(ImVec2(x0 + pad, y), 11, C_DIM, "HDRI");
        y += 16 * s;
        const float bw = (IW - 4 * s) / 2;
        for (size_t i = 0; i < m_hdriPaths.size(); ++i) {
            const float bx = x0 + pad + (i % 2) * (bw + 4 * s);
            ImVec2 a(bx, y), b(bx + bw, y + rowH);
            const bool on = m_hdriPaths[i] == m_hdriPath, h = hov(a, b);
            u.dl->AddRectFilled(a, b, on ? C_ACCENT : (h ? C_CARD_HI : C_CARD), rb(7 * s, rowH));
            std::string name = "Ciel procédural";
            if (!m_hdriPaths[i].empty()) {
                std::wstring f = m_hdriPaths[i].substr(m_hdriPaths[i].find_last_of(L"\\/") + 1);
                const size_t dot = f.find_last_of(L'.');
                std::wstring ext = dot != std::wstring::npos ? f.substr(dot + 1) : L"";
                if (dot != std::wstring::npos) f = f.substr(0, dot);
                for (const wchar_t* cut : {L"_2k", L"_4k", L"_1k", L"_8k"}) { const size_t k = f.find(cut); if (k != std::wstring::npos) f = f.substr(0, k); }
                for (auto& c : f) if (c == L'_') c = L' ';
                if (f.size() > 18) f = f.substr(0, 18);
                if (ext == L"exr") f += L" (exr)";
                name.assign(f.begin(), f.end());   // (file names of the HDRI: plain ASCII)
            }
            const float tw = u.textW(11, name.c_str(), on);
            u.text(ImVec2((a.x + b.x) * 0.5f - tw * 0.5f, a.y + (rowH - u.fs(11)) * 0.5f), 11, on ? C_INK : C_TEXT, name.c_str(), on);
            if (h && u.pressed && !on) { setHdri(m_hdriPaths[i]); savePrefs(); }
            if (i % 2 == 1 || i + 1 == m_hdriPaths.size()) y += rowH + 4 * s;
        }
        y += gap - 4 * s;
        {   // load a custom HDRI
            ImVec2 a(x0 + pad, y), b(x1 - pad, y + rowH);
            const bool h = hov(a, b);
            u.dl->AddRectFilled(a, b, h ? C_CARD_HI : C_CARD, rb(7 * s, rowH));
            const char* lab = "Charger une HDRI…  (.hdr, .exr)";
            const float tw = u.textW(12, lab, false);
            u.text(ImVec2((a.x + b.x) * 0.5f - tw * 0.5f, a.y + (rowH - u.fs(12)) * 0.5f), 12, C_TEXT, lab);
            if (h && u.pressed) loadHdriDialog();
            y += rowH + gap;
        }
        if (slider(101, "Rotation de la HDRI", m_hdriRotDeg, 0.0f, 360.0f, "%.0f°", m_hdriRotDeg)) lookChanged = true;
        if (slider(102, "Intensité de la HDRI", m_hdriInt, 0.0f, 3.0f, "%.2f", m_hdriInt)) lookChanged = true;
        {
            char ib[96];
            snprintf(ib, sizeof(ib), "Le soleil suit la HDRI · chargée en %.0f ms", m_hdriLoadMs);
            note(ib);
        }
        group("Illumination globale (HQ)");
        {
            const int k = seg("GI douce", {"Oui", "Non"}, m_softGI ? 0 : 1);
            if (k >= 0 && (k == 0) != m_softGI) { m_softGI = k == 0; m_renderer->resetAccumulation(); savePrefs(); }
        }
        if (m_softGI) {
            slider(104, "Intensité GI", m_softGIStrength, 0.0f, 2.0f, "%.2f", m_softGIStrength);   // (live, probes kept)
            static const int kBudgets[3] = {4096, 8192, 16384};   // (16384 at most: lit::placeProbeGrid)
            const int cur = m_softGIProbes == kBudgets[0] ? 0 : m_softGIProbes == kBudgets[1] ? 1 : m_softGIProbes == kBudgets[2] ? 2 : -1;
            const int k = seg("Qualité des sondes", {"Basse", "Moyenne", "Haute"}, cur);
            if (k >= 0 && k != cur) { m_softGIProbes = kBudgets[k]; m_renderer->resetAccumulation(); savePrefs(); }
            const int f = seg("Figer la GI pendant le déplacement", {"Oui", "Non"}, m_softGIHold ? 0 : 1);
            if (f >= 0 && (f == 0) != m_softGIHold) { m_softGIHold = f == 0; savePrefs(); }
            const int sp = seg("Afficher les sondes", {"Oui", "Non"}, m_showProbes ? 0 : 1);
            if (sp >= 0 && (sp == 0) != m_showProbes) { m_showProbes = sp == 0; savePrefs(); }
        }
        {
            const int k = seg("Lookdev GI (gris uni)", {"Oui", "Non"}, m_giLookdev ? 0 : 1);
            if (k >= 0 && (k == 0) != m_giLookdev) setLookdev(k == 0);
        }
        group("Matière");
        slider(105, "SSS (lumière sous la surface) · 0 = coupé", m_hqSSSStrength, 0.0f, 2.0f, "%.2f", m_hqSSSStrength);
        group("Grille du sol");
        u.text(ImVec2(x0 + pad, y), 11, C_DIM, "Couleur");
        y += 16 * s;
        {
            static const uint32_t sw[7] = {0x9aa3b8, 0xffffff, 0xf2c230, 0x4fc3f7, 0xe53935, 0x43a047, 0x7e57c2};
            const float cw = (IW - 7 * 4 * s) / 8;
            for (int k = 0; k < 8; ++k) {
                const float bx = x0 + pad + k * (cw + 4 * s);
                ImVec2 a(bx, y), b(bx + cw, y + rowH);
                const bool h = hov(a, b);
                if (k == 0) {
                    const bool on = m_gridAlpha <= 0;
                    u.dl->AddRectFilled(a, b, on ? C_ACCENT : (h ? C_CARD_HI : C_CARD), rb(7 * s, rowH));
                    const float tw = u.textW(10, "Défaut", on);
                    u.text(ImVec2((a.x + b.x) * 0.5f - tw * 0.5f, a.y + (rowH - u.fs(10)) * 0.5f), 10, on ? C_INK : C_TEXT, "Défaut", on);
                    if (h && u.pressed) { m_gridAlpha = 0; lookChanged = true; savePrefs(); }
                } else {
                    const uint32_t c = sw[k - 1];
                    const float3 rgb(((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f);
                    const bool on = m_gridAlpha > 0 && length(rgb - m_gridRgb) < 0.01f;
                    u.dl->AddRectFilled(a, b, IM_COL32((c >> 16) & 255, (c >> 8) & 255, c & 255, 255), rb(7 * s, rowH));
                    if (on || h) u.dl->AddRect(a, b, on ? C_ACCENT : C_TEXT, 7 * s, 0, (on ? 3.0f : 1.5f) * s);
                    if (h && u.pressed) { m_gridRgb = rgb; if (m_gridAlpha <= 0) m_gridAlpha = 0.6f; lookChanged = true; savePrefs(); }
                }
            }
            y += rowH + gap;
        }
        if (slider(103, "Opacité de la grille", m_gridAlpha, 0.0f, 1.0f, "%.0f %%", m_gridAlpha * 100.0f)) lookChanged = true;
        break;
    }
    case S_CLOUD: {   // the account of the cloud and the sync of the scenes (BrickCloud.cpp)
        if (!m_cloud.signedIn()) {
            group("Compte");
            note("Le même compte sur l'iPad et le PC : tes scènes passent de l'un à l'autre.");
            u.text(ImVec2(x0 + pad, y), 11, C_DIM, "E-mail");
            y += 16 * s;
            bool go = cloudField(u, "##cloudmail", ImVec2(x0 + pad, y), ImVec2(x1 - pad, y + rowH), m_cloudEmail, sizeof(m_cloudEmail), "toi@exemple.fr", false, s, C_CARD, C_TEXT, C_FAINT);
            y += rowH + gap;
            u.text(ImVec2(x0 + pad, y), 11, C_DIM, "Mot de passe");
            y += 16 * s;
            go |= cloudField(u, "##cloudpw", ImVec2(x0 + pad, y), ImVec2(x1 - pad, y + rowH), m_cloudPw, sizeof(m_cloudPw), "6 caractères au moins", true, s, C_CARD, C_TEXT, C_FAINT);
            y += rowH + gap;
            int k = seg(m_cloud.pending() ? "Connexion…" : "", {"Se connecter", "Créer un compte"}, -1);
            if (go) k = 0;
            if (k >= 0 && !m_cloud.pending()) {
                const std::string em = m_cloudEmail, pw = m_cloudPw;
                if (em.find('@') == std::string::npos || pw.size() < 6) m_cloudAuthMsg = "un e-mail et un mot de passe de 6 caractères au moins";
                else {
                    m_cloudAuthMsg = k == 0 ? "connexion…" : "création du compte…";
                    auto done = [this](bool ok, const std::string& msg) {
                        m_cloudAuthMsg = msg;
                        if (!ok) return;
                        memset(m_cloudPw, 0, sizeof(m_cloudPw));
                        m_cloudPollAt = 0;
                        if (m_cloudId.empty() && !m_bricks.empty()) { m_cloudDirty = true; m_cloudDirtyAt = 0; m_cloudSendAt = 0; }   // (this scene sent at once)
                        toast("Cloud : connecté (" + m_cloud.email() + ")");
                    };
                    if (k == 0) m_cloud.signIn(em, pw, done);
                    else m_cloud.signUp(em, pw, done);
                }
            }
            if (!m_cloudAuthMsg.empty()) note(m_cloudAuthMsg.c_str());
        } else {
            group("Compte");
            {
                const std::string who = "Connecté : " + m_cloud.email();
                u.text(ImVec2(x0 + pad, y), 12, C_TEXT, who.c_str());
                y += 22 * s;
            }
            if (seg("", {"Se déconnecter"}, -1) == 0) {
                m_cloud.signOut();
                m_cloudAuthMsg = "déconnecté";
                m_cloudDirty = false;
                m_cloudList.clear();
            }
            group("Synchro");
            {
                const int k = seg("Synchro automatique (à chaque modification)", {"Oui", "Non"}, m_cloudAuto ? 0 : 1);
                if (k >= 0 && (k == 0) != m_cloudAuto) { m_cloudAuto = k == 0; savePrefs(); }
            }
            {
                const int k = seg("", {"Synchroniser maintenant", "Scènes du cloud…"}, -1);
                if (k == 0) {
                    if (m_cloudDirty || m_cloudId.empty() || m_cloudSentKey.empty()) { m_cloudDirty = true; m_cloudDirtyAt = 0; m_cloudSendAt = 0; m_cloudSentKey.clear(); }
                    else m_cloudPollAt = 0;   // (nothing to send: its version in the cloud read now)
                }
                if (k == 1) { m_popup = 9; cloudRefreshList(); }
            }
            {   // the state of this scene
                std::string st = "Cette scène : ";
                if (m_cloudSending) st += "envoi…";
                else if (m_cloudDirty) { const int w = (int)std::ceil(std::max(2.5 - (ImGui::GetTime() - m_cloudDirtyAt), m_cloudSendAt - ImGui::GetTime())); st += w > 1 ? "modifiée, envoi dans " + std::to_string(w) + " s" : std::string("modifiée, envoi dans un instant"); }
                else if (m_cloudId.empty()) st += "pas encore dans le cloud";
                else st += m_cloudMsg.empty() ? std::string("à jour") : m_cloudMsg;
                if (!m_cloudId.empty() && m_cloudVer > 0) st += " · version " + std::to_string(m_cloudVer);
                note(st.c_str());
            }
        }
        break;
    }
    case S_FILES: {
        group("Scène");
        yesNo("Sauvegarde automatique", m_autosaveOn, 8);
        group("Réglages par défaut");
        {   // two actions (not a choice): no active button
            const int k = seg("Tous ces réglages", {"Enregistrer comme défaut", "Rétablir le défaut"}, -1);
            if (k >= 0) { clickRow = 13; clickOpt = k; }
        }
        group("À propos");
        const std::wstring vw = versionString();
        const std::string vs(vw.begin(), vw.end());
        u.text(ImVec2(x0 + pad, y), 11, C_DIM, vs.c_str());
        y += 18 * s;
        break;
    }
    }
    m_setSecH[sec] = y + m_setScroll - gap + pad - p0.y;
    u.dl->PopClipRect();
    if (scrollMax > 0) {   // the scroll bar
        const float trackH = H - 2 * pad, barH = std::max(30 * s, trackH * H / contentH);
        const float by = p0.y + pad + (trackH - barH) * (m_setScroll / scrollMax);
        u.dl->AddRectFilled(ImVec2(x1 - 6 * s, by), ImVec2(x1 - 3 * s, by + barH), C_FAINT, 2 * s);
    }
    if (lookChanged) {   // live: the environment and the grid of every renderer (accumulations reset)
        m_scene.env.hdriRotation = radians(m_hdriRotDeg);
        if (!m_ipad) m_scene.sun.direction = hdriSunDir();   // (the sun turns with the HDRI)
        m_scene.env.hdriIntensity = m_hdriInt;
        m_scene.gridLine = gridLineValue();
        m_renderer->resetAccumulation();
    }
    if (clickRow >= 0) {
        switch (clickRow) {
        case 0: m_uiScaleUser = kScales[clickOpt]; m_ui = (m_opt.uiScale > 0 ? m_opt.uiScale : m_dpi) * m_uiScaleUser; app::th::apply(m_ui); break;
        case 1: m_thumbSize = clickOpt; break;
        case 2:
            if (clickOpt == 3) { setRC(true); applyLook(); }
            else if (clickOpt == 2) { setRC(false); m_rs.raster = false; applyLook(); }
            else { setRC(false); m_quality = clickOpt; m_rs.msaaSamples = m_quality ? 4 : 1; if (!m_rs.raster) { m_rs.raster = true; applyLook(); } }
            break;
        case 3: m_ssaoOn = clickOpt == 0; break;
        case 4: m_rs.tonemapper = clickOpt == 2 ? 3 : clickOpt; break;
        case 6: m_showVox = clickOpt == 0; syncObjects(); break;
        case 7: m_animOn = clickOpt == 0; break;
        case 8: m_autosaveOn = clickOpt == 0; break;
        case 9: m_rs.aaQuality = clickOpt; break;
        case 10: m_rs.shadowQuality = clickOpt == 0 ? 0 : clickOpt + 1; break;
        case 12: m_stillAA = clickOpt == 0; break;
        case 14: m_touchHud = clickOpt == 0; m_arcOpen = false; break;
        case 16: m_selHud = clickOpt == 0; m_arcOpen = false; break;
        case 18: m_studioUi = clickOpt == 0; m_stMenu = false; break;
        case 19: m_stCardMode = clickOpt; m_cardsScroll = 0; break;
        case 24: m_rs.bevel = clickOpt == 0; m_renderer->resetAccumulation(); break;
        case 25: if (m_appIcon != clickOpt) { m_appIcon = clickOpt; plat::setAppIcon(m_appIcon); } break;
        case 21: m_ghostCol = clickOpt; applyGhostLook(); break;
        case 22: { static const float k[4] = {0.15f, 0.32f, 0.5f, 0.75f}; m_ghostAlphaPref = k[std::clamp(clickOpt, 0, 3)]; break; }
        case 23: m_ghostGlow = clickOpt; applyGhostLook(); break;
        case 20: m_hideDeco = clickOpt == 0; m_stKey.clear(); m_cardsScroll = 0; break;
        case 13: if (clickOpt == 0) saveDefaultPrefs(); else resetToDefaultPrefs(); break;
        case 11: { static const float k[4] = {1.0f, 0.85f, 0.75f, 0.67f}; m_renderScale = k[std::clamp(clickOpt, 0, 3)]; break; }
        }
        m_renderer->resetAccumulation();
        savePrefs();
    }
    if (u.pressed && clickRow < 0 && !inPanel && u.mouse.y > (m_studioUi ? 46 : 30) * s && !m_tabClicked) m_popup = 0;
    m_tabClicked = false;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_popup = 0;
}

// ------------------------------------------------------------------ Edit thumbnail (DEV, BrickThumbEdit.cpp)

// DEV: the menu of a right click (scene brick or library card)
void BrickApp::uiCtxMenu() {
    if (m_ctxPart < 0) return;
    Ui u = ui(m_ui);
    const float s = m_ui;
    const char* item = "Modifier la miniature";
    // + the source of its connectors, to set by hand (mode DEV, Wzx 10/11/2026)
    static const int kConn[4] = {4, 1, 2, 3};
    const int nConn = m_studioUi ? 4 : 0;
    const float w = std::max(u.textW(12, item), u.textW(11, "Échec, aucun connecteur") + 30 * s) + 28 * s, h = 30 * s, rh = 24 * s;
    const float veH = m_studioUi ? h : 0;   // « Éditer les voxels » (Voxel Editor)
    ImVec2 a(m_ctxPos.x, m_ctxPos.y), b(a.x + w, a.y + h + veH + 22 * s + (nConn ? 18 * s + nConn * rh + 4 * s : 0));
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    if (b.x > d.x) { a.x -= b.x - d.x; b.x = d.x; }
    if (b.y > d.y) { a.y -= b.y - d.y; b.y = d.y; }
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 4 * s), ImVec2(b.x, b.y + 4 * s), IM_COL32(0, 0, 0, 70), 9 * s);
    u.dl->AddRectFilled(a, b, IM_COL32(0x1d, 0x1e, 0x22, 250), 9 * s);
    u.dl->AddRect(a, b, IM_COL32(0x3a, 0x3b, 0x42, 255), 9 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    std::string nm = m_lib.parts[m_ctxPart].name;
    u.text(ImVec2(a.x + 12 * s, a.y + 5 * s), 10, IM_COL32(0x8a, 0x8c, 0x96, 255), nm.c_str());
    ImVec2 ia(a.x + 4 * s, a.y + 22 * s), ib(b.x - 4 * s, ia.y + h - 4 * s);
    const bool hov = u.hover(ia, ib);
    if (hov) u.dl->AddRectFilled(ia, ib, IM_COL32(0x2e, 0x30, 0x36, 255), 6 * s);
    u.text(ImVec2(ia.x + 10 * s, ia.y + 5 * s), 12, IM_COL32(0xec, 0xec, 0xee, 255), item);
    bool veHov = false;
    if (veH > 0) {
        const ImVec2 ea(ia.x, ib.y + 2 * s), eb(ib.x, ib.y + 2 * s + h - 4 * s);
        veHov = u.hover(ea, eb);
        if (veHov) u.dl->AddRectFilled(ea, eb, IM_COL32(0x2e, 0x30, 0x36, 255), 6 * s);
        u.text(ImVec2(ea.x + 10 * s, ea.y + 5 * s), 12, IM_COL32(0xec, 0xec, 0xee, 255), "Éditer les voxels");
    }
    int connHit = -1;
    if (nConn) {
        Part& P = m_lib.parts[m_ctxPart];
        float y = ib.y + veH + 6 * s;
        u.text(ImVec2(a.x + 12 * s, y), 9, IM_COL32(0x8a, 0x8c, 0x96, 255), "CONNECTEURS");
        y += 14 * s;
        for (int k = 0; k < nConn; ++k) {
            const int src = kConn[k];
            const ImVec2 ra(a.x + 4 * s, y), rb(b.x - 4 * s, y + rh - 2 * s);
            const bool hv = u.hover(ra, rb);
            if (hv) { u.dl->AddRectFilled(ra, rb, IM_COL32(0x2e, 0x30, 0x36, 255), 6 * s); connHit = src; }
            u.dl->AddRect(ImVec2(ra.x + 8 * s, ra.y + 5 * s), ImVec2(ra.x + 22 * s, rb.y - 5 * s), stConnCol(src), 3 * s, 0, 2.0f * s);
            if (P.connSrc == src) u.dl->AddRectFilled(ImVec2(ra.x + 12 * s, ra.y + 9 * s), ImVec2(ra.x + 18 * s, rb.y - 9 * s), stConnCol(src), 1 * s);
            std::string t = stConnName(src);
            if (src == P.connAuto && src != 4) t += "  (conversion)";
            u.text(ImVec2(ra.x + 30 * s, ra.y + 4 * s), 11, P.connSrc == src ? IM_COL32(0xff, 0xff, 0xff, 255) : IM_COL32(0xb0, 0xb2, 0xb8, 255), t.c_str());
            y += rh;
        }
    }
    if (u.pressed) {
        if (hov) openThumbEditor(m_ctxPart);
        if (veHov) openVoxelEditor(m_ctxPart);
        if (connHit >= 0) {
            Part& P = m_lib.parts[m_ctxPart];
            P.connSrc = (uint8_t)(P.connSrc == connHit && connHit == 4 ? P.connAuto : connHit);   // (Validé again: back to the conversion)
            saveConnLabels();
            toast(std::string("Connecteurs de ") + P.name + " : " + stConnName(P.connSrc));
        }
        m_ctxPart = -1;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_ctxPart = -1;
}

// the editor: viewer (orbit / zoom / pan), the 85 % frame drawn, Réinitialiser, Annuler, Enregistrer comme miniature
void BrickApp::uiThumbEditor() {
    if (m_teditPart < 0) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(0, 0, 0, 128));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const Part& p = m_lib.parts[m_teditPart];
    const float pad = 18 * s, vw = std::min(420 * s, d.y * 0.6f);
    const float W = vw + 2 * pad, H = pad + 24 * s + vw + 10 * s + 16 * s + 12 * s + 30 * s + 34 * s + pad;
    ImVec2 p0((d.x - W) * 0.5f, (d.y - H) * 0.5f), p1(p0.x + W, p0.y + H);
    u.dl->AddRectFilled(p0, p1, IM_COL32(0x1d, 0x1e, 0x22, 255), 14 * s);
    u.dl->AddRect(p0, p1, IM_COL32(0x3a, 0x3b, 0x42, 255), 14 * s);
    std::string title = "Miniature · " + p.label;
    if (title.size() > 52) title = title.substr(0, 51) + "…";
    u.text(ImVec2(p0.x + pad, p0.y + pad - 2 * s), 14, IM_COL32(0xec, 0xec, 0xee, 255), title.c_str(), true);
    ImVec2 va(p0.x + pad, p0.y + pad + 24 * s), vb(va.x + vw, va.y + vw);
    m_teRect = ImVec4(va.x, va.y, vb.x, vb.y);
    if (m_frozenTex) u.dl->AddImageRounded(m_gui->viewportTexture(), va, vb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 12 * s);
    ImGuiIO& io = ImGui::GetIO();
    const bool over = u.hover(va, vb);
    if (over && !m_teLightDrag && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) m_teDrag = true;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) m_teDrag = false;
    if (m_teDrag) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_teView.yaw -= io.MouseDelta.x * 0.01f;
            m_teView.pitch = std::clamp(m_teView.pitch + io.MouseDelta.y * 0.01f, radians(-89.0f), radians(89.0f));
        } else {
            m_teView.panX -= io.MouseDelta.x / (vw * 0.5f) / m_teView.zoom;
            m_teView.panY += io.MouseDelta.y / (vw * 0.5f) / m_teView.zoom;
        }
    }
    if (over && io.MouseWheel != 0) m_teView.zoom = std::clamp(m_teView.zoom * std::pow(1.1f, io.MouseWheel), 0.3f, 6.0f);
    char hint[96];
    snprintf(hint, sizeof(hint), "Glisser : tourner · clic droit : déplacer · molette : zoom (x%.2f)", m_teView.zoom);
    u.text(ImVec2(va.x, vb.y + 8 * s), 10.5f, IM_COL32(0x8a, 0x8c, 0x96, 255), hint);
    {   // the lights turned around the part, 0 to 360° (Wzx 09/11/2026)
        const float ly = vb.y + 10 * s + 16 * s + 10 * s;
        u.text(ImVec2(va.x, ly), 11, IM_COL32(0xc8, 0xca, 0xd2, 255), "Lumière");
        char deg[16];
        snprintf(deg, sizeof(deg), "%d°", (int)std::lround(m_teView.light));
        const float dw = u.textW(11, deg);
        u.text(ImVec2(vb.x - dw, ly), 11, IM_COL32(0xc8, 0xca, 0xd2, 255), deg);
        const float tx0 = va.x + 70 * s, tx1 = vb.x - 48 * s, ty = ly + u.fs(11) * 0.55f;
        const float k = std::clamp(m_teView.light / 360.0f, 0.0f, 1.0f), kx = tx0 + (tx1 - tx0) * k;
        u.dl->AddLine(ImVec2(tx0, ty), ImVec2(tx1, ty), IM_COL32(255, 255, 255, 40), 3 * s);
        u.dl->AddLine(ImVec2(tx0, ty), ImVec2(kx, ty), C_ACCENT, 3 * s);
        const bool ho = u.hover(ImVec2(tx0 - 8 * s, ty - 10 * s), ImVec2(tx1 + 8 * s, ty + 10 * s));
        if (ho && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) m_teLightDrag = true;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_teLightDrag = false;
        if (m_teLightDrag) m_teView.light = std::clamp((u.mouse.x - tx0) / (tx1 - tx0), 0.0f, 1.0f) * 360.0f;
        u.dl->AddCircleFilled(ImVec2(kx, ty), (ho || m_teLightDrag ? 8 : 7) * s, IM_COL32(0xf4, 0xf4, 0xf6, 255), 20);
    }
    float y = vb.y + 10 * s + 16 * s + 12 * s + 34 * s, bw = 0;
    const ImVec2 bp(p0.x + pad, y);
    if (webButton(u, bp, "Réinitialiser", bw)) m_teView = defaultThumbView();
    float sw = u.textW(12, "Enregistrer comme miniature", true) + 20 * s, cw = u.textW(12, "Annuler") + 20 * s;
    if (webButton(u, ImVec2(p1.x - pad - sw, y), "Enregistrer comme miniature", sw, true)) m_teSave = true;
    if (webButton(u, ImVec2(p1.x - pad - sw - 8 * s - cw, y), "Annuler", cw) || ImGui::IsKeyPressed(ImGuiKey_Escape)) m_teditPart = -1;
}

#include "BrickUiIpad.inl"
#include "BrickUiStudio.inl"
#include "BrickVoxelEditor.inl"

} // namespace brickit
