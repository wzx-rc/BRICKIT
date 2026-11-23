// BrickIt PC: the theme module (BrickTheme.h). Portable (no Windows API): compiled on every platform, used on the PC.
#include "BrickTheme.h"
#include <algorithm>
#include <cmath>

namespace brickit::theme {

// ------------------------------------------------------------------ colour helpers

static float srgbOf(float x) { return x <= 0.0031308f ? 12.92f * x : 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f; }
static float linOf(float x) { return x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f); }
static uint32_t pack(float r, float g, float b) {
    auto c = [](float v) { return (uint32_t)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f); };
    return (c(r) << 16) | (c(g) << 8) | c(b);
}
static void unpack(uint32_t rgb, float& r, float& g, float& b) { r = ((rgb >> 16) & 255) / 255.0f; g = ((rgb >> 8) & 255) / 255.0f; b = (rgb & 255) / 255.0f; }

// CSS Color 4: oklch -> oklab -> linear sRGB -> sRGB
uint32_t oklch(float L, float C, float h) {
    const float hr = h * 3.14159265f / 180.0f, a = C * std::cos(hr), b = C * std::sin(hr);
    const float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
    const float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
    const float s_ = L - 0.0894841775f * a - 1.2914855480f * b;
    const float l = l_ * l_ * l_, m = m_ * m_ * m_, s = s_ * s_ * s_;
    const float R = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    const float G = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    const float B = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
    return pack(srgbOf(std::clamp(R, 0.0f, 1.0f)), srgbOf(std::clamp(G, 0.0f, 1.0f)), srgbOf(std::clamp(B, 0.0f, 1.0f)));
}

static float luma(uint32_t rgb) {   // relative luminance (WCAG)
    float r, g, b;
    unpack(rgb, r, g, b);
    return 0.2126f * linOf(r) + 0.7152f * linOf(g) + 0.0722f * linOf(b);
}
// color-mix(in srgb, a k, b)
static uint32_t mix(uint32_t a, uint32_t b, float k) {
    float r1, g1, b1, r2, g2, b2;
    unpack(a, r1, g1, b1);
    unpack(b, r2, g2, b2);
    return pack(r1 * k + r2 * (1 - k), g1 * k + g2 * (1 - k), b1 * k + b2 * (1 - k));
}
// PLUM onAccent(): white, or almost black on a light accent
static uint32_t onAccent(uint32_t acc) { return luma(acc) > 0.42f ? 0x15161a : 0xffffff; }

// ------------------------------------------------------------------ the themes

static const std::vector<Accent> kLego = {
    {"Jaune LEGO", 0xf2cd37}, {"Orange", 0xfe8a18}, {"Rouge", 0xc91a09}, {"Bleu", 0x0055bf}, {"Vert", 0x237841}, {"Azur", 0x36aebf}};

const std::vector<Def>& themes() {
    static std::vector<Def> d = {
        {"classic", "Noir", "Noir, jaune LEGO et blanc.", 1,
         {{"anthracite", "Anthracite", 0, 0, 0x25262b}},
         {{"Jaune", 0xf7c51e}, {"Orange", 0xfe8a18}, {"Rouge", 0xc91a09}, {"Bleu", 0x0055bf}, {"Vert", 0x237841}, {"Azur", 0x36aebf}}},
        // the « Brique » interface (Wzx 15/11/2026, canvas JBtw2wBYiTVRTg7JuzaXc5): slate panels in relief, the yellow of
        // the brick in gradient on what is active, round type (Fredoka); its own layout (BrickUiBrique.inl)
        {"brique", "Ardoise", "Ardoise bleutée et jaune brique.", 1,
         {{"ardoise", "Ardoise", 0, 0, 0x282d37}},
         {{"Jaune", 0xffc93c}, {"Orange", 0xff9f1c}, {"Bleu", 0x5b8def}, {"Vert", 0x3dd68c}}},
        // Crème (Wzx 22/11/2026): the cream touch kit of the iPad, rebuilt on the layout of Brique (all its functions, the
        // library of today); Galet, the same a little less cream (warm greys)
        {"creme", "Crème", "Crème chaud et brun doux.", 0,
         {{"creme", "Crème", 0, 0, 0xf1e7d8}},
         {{"Espresso", 0x4a3b30}, {"Jaune", 0xe2a300}, {"Terracotta", 0xc0603a}, {"Sauge", 0x5c8a68}, {"Bleu", 0x3d6fb6}}},
    };
    static bool swatches = false;
    if (!swatches) {   // the swatch of an oklch tint: its grey at mid lightness, a little more saturated so that it shows
        swatches = true;
        for (Def& t : d)
            for (Tint& ti : t.tints)
                if (!ti.swatch) ti.swatch = oklch(0.62f, std::max(0.02f, ti.c * 4.0f), ti.h);
    }
    return d;
}

int indexOf(const std::string& id) {
    const auto& t = themes();
    for (size_t i = 0; i < t.size(); ++i)
        if (id == t[i].id) return (int)i;
    return -1;
}

bool isDark(const Def& d, int mode, bool sysDark) {
    if (d.mode >= 0) return d.mode == 1;
    if (mode == 1) return false;
    if (mode == 2) return true;
    return sysDark;
}

// ------------------------------------------------------------------ resolve

static void accentOf(Tokens& t, const Def& d, int accent, uint32_t defLight, uint32_t def2Light, uint32_t defDark, uint32_t def2Dark) {
    accent = std::clamp(accent, 0, (int)d.accents.size() - 1);
    uint32_t a, a2;
    if (accent == 0) { a = t.dark ? defDark : defLight; a2 = t.dark ? def2Dark : def2Light; }
    else {   // PLUM data-accent="custom": the secondary is the accent 86 % with black (light) or 88 % with white (dark)
        a = d.accents[accent].rgb;
        a2 = t.dark ? mix(a, 0xffffff, 0.88f) : mix(a, 0x000000, 0.86f);
    }
    t.accent = col(a);
    t.accent2 = col(a2);
    t.onAccent = col(onAccent(a));
    t.progress = t.accent;
    // the accent as a text colour: a light accent (yellow) does not read on a light panel
    t.accentTxt = (!t.dark && luma(a) > 0.45f) ? col(mix(a, 0x000000, 0.55f)) : (!t.dark ? col(a2) : col(a));
}

Tokens resolve(const std::string& id, bool dark, int tint, int accent) {
    const auto& all = themes();
    int ti = indexOf(id);
    if (ti < 0) ti = indexOf("creme");
    const Def& d = all[ti];
    Tokens t;
    t.id = d.id;
    t.dark = dark;
    tint = std::clamp(tint, 0, (int)d.tints.size() - 1);
    t.shadow = dark ? IM_COL32(0, 0, 0, 90) : IM_COL32(20, 20, 18, 30);

    if (t.id == "classic") {   // the original PC interface (BrickUiIpad.inl uiTheme(false))
        t.brickSwatch = true;   // (the colours as 1x1 bricks seen from above, as Glass: Wzx 09/11/2026)
        t.dark = true;
        t.bg = IM_COL32(0x13, 0x14, 0x17, 255);
        t.panel = IM_COL32(24, 25, 29, 235);
        t.panelSolid = IM_COL32(0x1b, 0x1c, 0x20, 255);
        t.card = IM_COL32(0x25, 0x26, 0x2b, 255);
        t.cardHi = IM_COL32(0x30, 0x32, 0x3a, 255);
        t.line = IM_COL32(0x2f, 0x31, 0x37, 255);
        t.text = IM_COL32(0xec, 0xec, 0xee, 255);
        t.dim = IM_COL32(0x9a, 0x9c, 0xa4, 255);
        t.faint = IM_COL32(0x67, 0x6a, 0x72, 255);
        t.thumb = IM_COL32_WHITE;
        t.overlay = IM_COL32(15, 16, 19, 219);
        t.hud = IM_COL32(15, 16, 19, 184);
        t.shadow = IM_COL32(0, 0, 0, 60);
        accentOf(t, d, accent, 0xf7c51e, 0xf7c51e, 0xf7c51e, 0xffd84a);
        if (accent == 0) t.onAccent = IM_COL32(0x17, 0x17, 0x1a, 255);
        t.hudStyle = HUD_LEGO;
        t.font = FONT_SEGOE;
        return t;
    }

    if (t.id == "creme") {   // the cream of the iPad kit (Crème) or warm greys (Galet), on the layout of Brique
        const bool galet = d.tints[std::clamp(tint, 0, (int)d.tints.size() - 1)].id == std::string("galet");
        t.dark = false;
        t.brique = false;   // (its own interface: the kit, BrickUiIpad.inl)
        if (!galet) {
            t.bg = IM_COL32(0xf4, 0xed, 0xe3, 255);
            t.panel = IM_COL32(0xfb, 0xf7, 0xf1, 250);
            t.panelSolid = IM_COL32(0xfb, 0xf7, 0xf1, 255);
            t.card = IM_COL32(0xff, 0xfc, 0xf8, 255);
            t.cardHi = IM_COL32(0xf3, 0xea, 0xdf, 255);
            t.line = IM_COL32(0xe6, 0xda, 0xcb, 255);
            t.text = IM_COL32(0x3e, 0x32, 0x29, 255);
            t.dim = IM_COL32(0x80, 0x71, 0x64, 255);
            t.faint = IM_COL32(0xa9, 0x9a, 0x8b, 255);
            t.thumb = IM_COL32(0xc8, 0xb8, 0xa5, 255);
            t.overlay = IM_COL32(0xf8, 0xf2, 0xea, 244);
            t.hud = IM_COL32(0xff, 0xfb, 0xf6, 236);
            t.stage = {true, 0xf3e9d9u, 0xfbf6eeu, 0xeee3d2u, 0xf2e9dcu, 0xe8dccbu, 0xc4b39du, 0.30f};
        } else {
            t.bg = IM_COL32(0xef, 0xee, 0xeb, 255);
            t.panel = IM_COL32(0xfa, 0xf9, 0xf7, 250);
            t.panelSolid = IM_COL32(0xfa, 0xf9, 0xf7, 255);
            t.card = IM_COL32(0xff, 0xff, 0xff, 255);
            t.cardHi = IM_COL32(0xf0, 0xee, 0xea, 255);
            t.line = IM_COL32(0xe2, 0xdf, 0xda, 255);
            t.text = IM_COL32(0x2f, 0x2d, 0x2a, 255);
            t.dim = IM_COL32(0x74, 0x70, 0x6a, 255);
            t.faint = IM_COL32(0xa2, 0x9e, 0x97, 255);
            t.thumb = IM_COL32(0xbd, 0xb8, 0xb0, 255);
            t.overlay = IM_COL32(0xf7, 0xf6, 0xf4, 244);
            t.hud = IM_COL32(0xff, 0xff, 0xff, 236);
            t.stage = {true, 0xe9e8e5u, 0xf6f5f3u, 0xe5e3dfu, 0xeceae6u, 0xe0ddd8u, 0xb2ada5u, 0.28f};
        }
        accentOf(t, d, accent, 0x4a3b30, 0x6a5848, 0x4a3b30, 0x6a5848);
        if (accent == 0 && galet) { t.accent = IM_COL32(0x3a, 0x38, 0x35, 255); t.accent2 = IM_COL32(0x5a, 0x57, 0x53, 255); t.accentTxt = t.accent; t.progress = t.accent; }
        if (accent == 0) t.onAccent = IM_COL32(0xff, 0xf8, 0xee, 255);
        t.shadow = IM_COL32(60, 40, 20, 40);
        t.radius = 1.4f;
        t.pill = false;
        t.lines = false;
        t.font = FONT_FREDOKA;
        t.hudStyle = HUD_FLAT;
        t.hudBtn = galet ? IM_COL32(0x3a, 0x38, 0x35, 225) : IM_COL32(0x4a, 0x3b, 0x30, 225);   // (the brown bubbles of the kit)
        t.hudBtnWhiteIcon = true;
        t.brickSwatch = true;
        return t;
    }

    if (t.id == "brique") {
        t.dark = true;
        t.brique = true;
        t.bg = IM_COL32(0x22, 0x26, 0x2f, 255);
        t.panel = IM_COL32(0x28, 0x2d, 0x37, 248);
        t.panelSolid = IM_COL32(0x28, 0x2d, 0x37, 255);
        t.overlay = IM_COL32(0x24, 0x28, 0x31, 245);
        t.hud = IM_COL32(0x20, 0x24, 0x2c, 228);
        t.card = IM_COL32(0x32, 0x38, 0x44, 255);
        t.cardHi = IM_COL32(0x3a, 0x40, 0x4d, 255);
        t.line = IM_COL32(255, 255, 255, 20);
        t.text = IM_COL32(0xf1, 0xf3, 0xf7, 255);
        t.dim = IM_COL32(0xb4, 0xb9, 0xc4, 255);
        t.faint = IM_COL32(0x8a, 0x90, 0x9c, 255);
        t.thumb = IM_COL32_WHITE;
        t.shadow = IM_COL32(0, 0, 0, 80);
        accentOf(t, d, accent, 0xffc93c, 0xf9b800, 0xffc93c, 0xffd84a);
        if (accent == 0) t.onAccent = IM_COL32(0x23, 0x18, 0x00, 255);
        t.radius = 1.35f;
        t.pill = false;
        t.lines = false;
        t.font = FONT_FREDOKA;
        t.hudStyle = HUD_FLAT;
        t.hudBtn = IM_COL32(0x32, 0x38, 0x44, 235);
        t.hudBtnWhiteIcon = true;
        t.brickSwatch = true;   // (the swatches as small bricks, Wzx 15/11/2026)
        // the stage: a slate studio, lighter than the dark PC one (Wzx 15/11/2026: « le fond est trop sombre pour ce thème »)
        t.stage = {true, 0x6b7384u, 0x535b6au, 0x2f3540u, 0x3a414du, 0x444b58u, 0xaab3c2u, 0.15f};
        return t;
    }

    if (t.id == "glass" || t.id == "red") {   // Maquette 4.0: translucent smoked panels over the 3D view, thin lines
        const bool red = t.id == "red";
        const bool alt = tint == 1;   // Glass · Ardoise (a cool grey) / Red · Noir (the red only in the accent)
        t.dark = true;
        if (!red) {
            const int k = alt ? 4 : 0;   // (Ardoise: a touch of blue in the greys)
            t.bg = IM_COL32(0x12, 0x13, 0x16 + k, 255);
            t.panel = IM_COL32(28, 29, 32 + k, 188);
            t.panelSolid = IM_COL32(0x1c, 0x1d, 0x20 + k, 250);
            t.overlay = IM_COL32(22, 23, 26 + k, 205);
            t.hud = IM_COL32(26, 27, 30 + k, 196);
        } else {
            t.bg = alt ? IM_COL32(0x14, 0x10, 0x10, 255) : IM_COL32(0x1a, 0x0b, 0x0d, 255);
            t.panel = alt ? IM_COL32(30, 24, 25, 190) : IM_COL32(52, 14, 18, 188);
            t.panelSolid = alt ? IM_COL32(0x22, 0x1a, 0x1b, 250) : IM_COL32(0x2e, 0x0e, 0x11, 250);
            t.overlay = alt ? IM_COL32(26, 20, 21, 208) : IM_COL32(44, 12, 15, 205);
            t.hud = alt ? IM_COL32(30, 22, 23, 196) : IM_COL32(48, 14, 17, 196);
        }
        t.card = IM_COL32(255, 255, 255, 16);
        t.cardHi = IM_COL32(255, 255, 255, 30);
        t.line = red && !alt ? IM_COL32(255, 150, 150, 34) : IM_COL32(255, 255, 255, 26);
        t.text = red ? IM_COL32(0xf3, 0xea, 0xea, 255) : IM_COL32(0xe6, 0xe7, 0xea, 255);
        t.dim = red ? IM_COL32(0xc4, 0xa8, 0xaa, 255) : IM_COL32(0xa2, 0xa5, 0xaa, 255);
        t.faint = red ? IM_COL32(0x86, 0x6c, 0x6e, 255) : IM_COL32(0x6e, 0x71, 0x76, 255);
        t.thumb = IM_COL32_WHITE;
        t.shadow = IM_COL32(0, 0, 0, 70);
        if (red) accentOf(t, d, accent, 0xe3141f, 0xb80f18, 0xe3141f, 0xff4a52);
        else accentOf(t, d, accent, 0xd9b443, 0xc49a2a, 0xd9b443, 0xe2bf4f);
        t.radius = 1.3f;   // (the iPad's rounder panels and cards)
        t.pill = true;
        t.lines = true;
        t.font = FONT_SEGOE;
        t.hudStyle = HUD_FLAT;
        t.hudBtn = red ? IM_COL32(120, 24, 30, 205) : IM_COL32(64, 66, 72, 205);
        t.hudBtnWhiteIcon = true;
        t.brickSwatch = true;
        // the stage: a grey studio (Glass) or a dark red one (Red), as the backdrop of the mock-up
        if (red) t.stage = {true, alt ? 0x2a2224u : 0x3a1b1eu, alt ? 0x1c1718u : 0x241315u, alt ? 0x131011u : 0x170c0eu, alt ? 0x1d1819u : 0x22121au, alt ? 0x1d1819u : 0x22141au, 0xc4a3a5u, 0.12f};
        else t.stage = {true, 0x6a707au, 0x4c5058u, 0x26282cu, 0x2e3035u, 0x34363bu, 0xa4aab2u, 0.13f};
        return t;
    }

    // ---- PLUM (plum-themes-kit/kit/themes/<id>/tokens.css): the greys in oklch(L, C, var(--th-h))
    const float h = d.tints[tint].h, c = d.tints[tint].c;
    auto ok = [&](float L, float k = 1.0f) { return oklch(L, c * k, h); };
    const bool sommet = t.id == "sommet";
    uint32_t paper, card, sunk, raise, menu, side, side2, side3, line, ink, ink2, ink3;
    if (!dark) {
        side = ok(0.972f, 1.4f); side2 = ok(0.94f, 1.6f); side3 = ok(0.915f, 1.8f);
        paper = 0xffffff; card = 0xffffff; sunk = ok(0.965f, 1.4f); raise = ok(0.975f, 1.2f); menu = 0xffffff;
        line = ok(0.92f, 1.6f);
        ink = ok(0.22f); ink2 = ok(0.46f, 1.4f); ink3 = ok(0.6f, 1.4f);
        if (sommet) {
            side = 0xf5f5f6; side2 = 0xececee; side3 = 0xe6e6e9;
            raise = ok(0.982f); sunk = ok(0.962f); line = ok(0.93f);
            ink = ok(0.21f); ink2 = ok(0.45f); ink3 = ok(0.62f);
        }
    } else {
        side = ok(0.205f); side2 = ok(0.245f); side3 = ok(0.275f, 1.4f);
        paper = ok(0.18f); card = ok(0.215f); sunk = ok(0.165f); raise = ok(0.245f); menu = ok(0.235f);
        line = ok(0.265f);
        ink = ok(0.93f, 0.5f); ink2 = ok(0.74f); ink3 = ok(0.57f, 1.4f);
        if (sommet) { side = 0x141416; side2 = 0x1e1e21; side3 = 0x26262a; }
    }
    (void)raise;
    t.bg = col(side);
    t.panelSolid = col(menu);
    t.line = col(line);
    t.text = col(ink);
    t.dim = col(ink2);
    t.faint = col(ink3);
    t.thumb = dark ? IM_COL32_WHITE : col(ok(0.5f, 1.4f));
    t.overlay = col(dark ? paper : 0xffffff, 236);
    t.radius = 0.8f;   // --r-sm 5, --r-md 7, --r-lg 10 (the PC's 6 / 7 / 12)
    t.lines = true;
    if (!sommet) {   // Minimal: the bars in the sidebar's tone, the buttons in its hover tones, Émeraude
        t.panel = col(side, 242);
        t.card = col(side2);
        t.cardHi = col(side3);
        accentOf(t, d, accent, 0x1e9a53, 0x17864a, 0x22a55b, 0x33b86c);
        t.font = FONT_INSTRUMENT;
        t.hudStyle = HUD_FLAT;
        t.hudBtn = col(dark ? menu : 0xffffff);
        t.hudBtnWhiteIcon = dark;
    } else {   // Sommet: white (or black) surfaces, fine lines, the black pill for what is active, the blue for progress
        t.panel = col(dark ? paper : 0xffffff, 244);
        t.card = col(dark ? side2 : side);
        t.cardHi = col(dark ? side3 : side2);
        accentOf(t, d, accent, 0x3b6cf6, 0x2c59da, 0x6c91ff, 0x8aa8ff);
        if (accent == 0 && dark) t.onAccent = col(0x0d1020);
        t.progress = t.accent;
        t.accentTxt = dark ? t.accent : t.accent2;
        const uint32_t pill = dark ? 0xf2f2f3 : 0x111113, onPill = dark ? 0x111113 : 0xffffff;
        t.accent = col(pill);            // (the active buttons: the pill, as PLUM's .seg.on and .btn.pri)
        t.accent2 = col(dark ? 0xffffff : 0x2a2a2e);
        t.onAccent = col(onPill);
        t.pill = true;
        t.radius = 1.0f;
        t.font = FONT_GEIST;
        t.hudStyle = HUD_PILL;
        t.hudBtn = col(pill);
        t.hudBtnWhiteIcon = !dark;
    }
    t.hud = col(dark ? card : paper, 214);
    // the stage: Minimal, the canvas of PLUM (warm white, the tint in the floor); Sommet, a pale sky over a white plain
    if (!dark) {
        if (sommet) t.stage = {true, 0xbad2f2, 0xeef3fa, ok(0.95f), ok(0.85f), ok(0.9f), ok(0.62f), 0.22f};
        else t.stage = {true, ok(0.95f, 1.6f), ok(0.985f, 1.2f), ok(0.94f, 1.6f), ok(0.82f, 2.0f), ok(0.9f, 1.6f), ok(0.6f, 2.0f), 0.22f};
    }
    return t;
}

} // namespace brickit::theme
