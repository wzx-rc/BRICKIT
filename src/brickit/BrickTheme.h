// BrickIt PC: the theme module (Réglages › Thème). Four themes, each with its light and dark modes (or one forced), its
// « couleur du thème » (the tint of the greys) and its accent colours, remembered per theme:
//   classic  the original dark LEGO interface of the PC (always dark)
//   creme    PC theme 2.0, from the iPad « entre-deux » mock-up (light / dark, translucent pebbles, LEGO bubbles)
//   minimal  PLUM Minimal (Instrument Sans, flat surfaces, fine lines, Émeraude), from plum-themes-kit
//   sommet   PLUM Sommet (Geist Mono, black pills for what is active, one sky blue, painted mountains), plum-themes-kit
// The kit is a web / CSS kit: its tokens (--paper, --ink, --accent… written in oklch) are computed here, in the same
// roles, and BrickUi.cpp maps them onto its colours (C_PANEL, C_TEXT, C_ACCENT…). The ids are PLUM's, so that the
// preferences stay compatible. Adding a theme: a Def in themes(), a case in resolve().
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "imgui.h"

namespace brickit::theme {

struct Tint { const char* id; const char* name; float h, c; uint32_t swatch; };   // oklch hue and chroma of the greys; swatch 0xRRGGBB
struct Accent { const char* name; uint32_t rgb; };                                // 0xRRGGBB

struct Def {
    const char* id;
    const char* label;
    const char* desc;
    int mode;                       // -1: the user chooses (Système / Clair / Sombre), 0: always light, 1: always dark
    std::vector<Tint> tints;        // the first is the default
    std::vector<Accent> accents;    // the first is the theme's own
};

// the 3D stage behind the interface (BrickApp::applyLook): on a light theme a pale sky and a light floor
struct Stage {
    bool light = false;             // false: the dark stage of the PC (the HDRI, the dark floor, the user's grid)
    uint32_t top = 0, horizon = 0, bottom = 0, floor = 0, hemiGround = 0;   // 0xRRGGBB
    uint32_t grid = 0;              // 0xRRGGBB of the lines
    float gridAlpha = 0.25f;
};

enum Font { FONT_SEGOE = 0, FONT_INSTRUMENT = 1, FONT_GEIST = 2 };
enum Hud { HUD_LEGO = 0, HUD_FLAT = 1, HUD_PILL = 2 };   // the bubbles of the selection HUD

// everything the interface needs, resolved for a theme, a mode, a tint and an accent (ImU32 = IM_COL32)
struct Tokens {
    std::string id;
    bool dark = true;
    ImU32 bg = 0, panel = 0, panelSolid = 0, card = 0, cardHi = 0, line = 0, text = 0, dim = 0, faint = 0;
    ImU32 accent = 0, accent2 = 0, accentTxt = 0, onAccent = 0;   // accent: the active buttons; accentTxt: the accent as a text colour
    ImU32 progress = 0;                           // sliders and progress (the accent; Sommet: its blue, not the pill)
    ImU32 thumb = IM_COL32_WHITE, overlay = 0, hud = 0, shadow = IM_COL32(0, 0, 0, 60);
    float radius = 1.0f;                          // multiplier of the radii of the panels and buttons
    bool pill = false;                            // buttons fully rounded (Sommet)
    bool lines = false;                           // panels outlined with a fine line (Minimal, Sommet)
    int font = FONT_SEGOE;
    int hudStyle = HUD_LEGO;
    ImU32 hudBtn = 0;                             // the bubbles of the HUD (HUD_FLAT, HUD_PILL)
    bool hudBtnWhiteIcon = true;
    Stage stage;
};

const std::vector<Def>& themes();
int indexOf(const std::string& id);               // -1 when unknown
// mode: 0 Système, 1 Clair, 2 Sombre; sysDark: Windows in dark mode
bool isDark(const Def& d, int mode, bool sysDark);
Tokens resolve(const std::string& id, bool dark, int tint, int accent);

// oklch (L 0..1, C, h degrees) to 0xRRGGBB (sRGB, clamped)
uint32_t oklch(float L, float C, float h);
inline ImU32 col(uint32_t rgb, int a = 255) { return IM_COL32((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, a); }

} // namespace brickit::theme
