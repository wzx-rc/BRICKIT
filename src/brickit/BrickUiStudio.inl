// BrickIt PC - the « Studio » interface (Wzx 09/11/2026), rebuilt from the Maquette 4.0 (Design canvas « BrickIt PC ·
// Maquette 3.0 », board Technic.dc.html): smoked translucent grey panels over the 3D view, thin lines, pills, line
// pictograms in one language (a constant thin stroke, isometric, faces tinted so the hidden edges show through), the
// colour swatches as a 1x1 brick seen from above, and the press-and-hold marking menu (six actions on a ring, the
// recent colours on an arc above). Included at the end of BrickUi.cpp (it uses its helpers and colours).
//   header (m_uiRects[0]): logo, scene, modes (Construire / Peindre / Mécanique / Notice), undo / redo, render, more,
//   settings, Exporter;  left: the tools;  right: Couleur;  bottom (m_uiRects[1]): the library drawer.

// ------------------------------------------------------------------ small drawing helpers

static ImU32 stA(ImU32 c, float k) { return (c & 0x00FFFFFF) | ((ImU32)(((c >> 24) & 0xFF) * std::clamp(k, 0.0f, 1.0f)) << 24); }

// a smoked glass panel: the translucent fill, a hairline, a soft drop shadow
static void stGlass(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float s) {
    dl->AddRectFilled(ImVec2(a.x, a.y + 6 * s), ImVec2(b.x, b.y + 8 * s), IM_COL32(0, 0, 0, 40), r + 2 * s);
    dl->AddRectFilled(a, b, C_PANEL, r);
    dl->AddRectFilledMultiColor(ImVec2(a.x + r * 0.3f, a.y + 1), ImVec2(b.x - r * 0.3f, a.y + 2 * s), IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    dl->AddRect(a, b, C_LINE, r, 0, 1.0f);
}

// the length in bytes of the UTF-8 character at p
static int stU8(const char* p) { const unsigned char c = (unsigned char)*p; return c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4; }
// text with letter spacing (the uppercase labels of the mock-up), returns its width
static float stTrackW(Ui& u, float px, const char* t, float track, bool bold) {
    ImFont* f = bold ? u.bold : u.font;
    float w = 0;
    for (const char* p = t; *p;) {
        const int n = stU8(p);
        w += f->CalcTextSizeA(u.fs(px), 1e9f, 0, p, p + n).x + track;
        p += n;
    }
    return w - track;
}
static float stTrack(Ui& u, ImVec2 pos, float px, ImU32 col, const char* t, float track, bool bold = true) {
    ImFont* f = bold ? u.bold : u.font;
    float x = pos.x;
    for (const char* p = t; *p;) {
        const int n = stU8(p);
        u.dl->AddText(f, u.fs(px), ImVec2(x, pos.y), col, p, p + n);
        x += f->CalcTextSizeA(u.fs(px), 1e9f, 0, p, p + n).x + track;
        p += n;
    }
    return x - track - pos.x;
}
static std::string stUpper(std::string t) { return upperFr(t); }

// the line icons of the mock-up (24 x 24, stroked), centred on c
static void stIcon(ImDrawList* dl, const char* d, ImVec2 c, float size, ImU32 col, float thick) {
    const float k = size / 24.0f;
    if (d[0] == 'F') { svg::shape(dl, d + 1, ImVec2(c.x - 12 * k, c.y - 12 * k), k, col, 0, 0); return; }   // a filled icon (Recraft: Pipette, Pinceau)
    svg::path(dl, d, ImVec2(c.x - 12 * k, c.y - 12 * k), k, col, thick);
}
static const char* ST_SEL = "M5 3l6 16 2-6 6-2z";
static const char* ST_BUILD = "M4 10h16v9H4zM7 10V7M12 10V7M17 10V7";
// Pinceau and Pipette: filled, drawn with Recraft (Wzx 09/11/2026: « plus lisibles »), the 24 box
static const char* ST_PAINT = "FM9.49 12.65C9.58 12.57 9.7 12.44 9.79 12.35L17.8 4.35C18.25 3.9 18.67 3.35 19.34 3.31C20.64 3.33 21.05 4.7 20.17 5.53C19.87 5.81 19.59 6.11 19.3 6.41L12.42 13.27C12.02 13.67 11.56 14.16 11.15 14.53C11.09 14.47 11.03 14.4 10.98 14.34C10.49 13.77 9.96 13.22 9.49 12.65zM3.29 20.24C3.49 19.41 3.7 18.58 3.92 17.76C4.13 16.99 4.23 16.34 4.64 15.64C5.33 14.46 6.18 13.9 7.48 13.56C7.48 13.56 7.9 14 7.94 14.04L9.55 15.62C9.81 15.87 10.2 16.21 10.43 16.47C10.11 18.54 8.41 20.24 6.26 20.24L3.29 20.24z";
static const char* ST_PICK = "FM13.53 5.27C13.97 4.81 14.44 4.37 14.89 3.92C15.5 3.31 15.94 2.71 16.89 2.68C18.68 2.49 19.79 4.78 18.67 6C17.96 6.77 17.11 7.47 16.43 8.23C17.07 8.46 17.1 9.25 16.4 9.34C16.12 9.37 15.69 8.84 15.48 8.64C15.25 8.41 15.03 8.18 14.81 7.95L12.58 5.74C12.17 5.34 12.76 4.35 13.53 5.27zM7.51 15.48C7.17 15.57 6.42 15.81 6.12 15.83C5.46 15.89 5.62 15.17 5.71 14.8L6.06 13.38C6.12 13.11 6.39 12.92 6.58 12.73L10.4 8.91C11.05 8.27 11.76 7.6 12.38 6.95C12.66 7.2 12.96 7.52 13.23 7.78L14.06 8.6C14.18 8.73 14.5 9.08 14.62 9.15C14.37 9.43 14.05 9.73 13.78 10L11.36 12.41C10.5 13.28 9.53 14.27 8.62 15.11C8.49 15.23 7.73 15.42 7.51 15.48zM6.24 20.87C5.34 20.85 4.65 19.98 4.98 19.11C5.26 18.38 5.71 18.06 6.19 17.53C6.63 17.93 7.17 18.54 7.42 19.08C7.82 19.92 7.14 20.82 6.24 20.87z";
static const char* ST_MOVE = "M12 3v18M3 12h18M12 3 9 6M12 3l3 3M12 21l-3-3M12 21l3-3M3 12l3-3M3 12l3 3M21 12l-3-3M21 12l-3 3";
static const char* ST_ROT = "M20 12a8 8 0 1 1-2.3-5.6M20 4v5h-5";
static const char* ST_ROTL = "M4 12a8 8 0 1 0 2.3-5.6M4 4v5h5";
static const char* ST_MECH = "M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8zM12 2v3M12 19v3M2 12h3M19 12h3M5 5l2 2M17 17l2 2M5 19l2-2M17 7l2-2";
static const char* ST_EYE = "M2 12C5 7 8.5 5 12 5C15.5 5 19 7 22 12C19 17 15.5 19 12 19C8.5 19 5 17 2 12zM12 9a3 3 0 1 0 0 6 3 3 0 0 0 0-6z";
static const char* ST_DUP = "M8 8h11v11H8zM5 16V5h11";
static const char* ST_TILT = "M4 18 18 4M9 4h9v9";
static const char* ST_DEL = "M4 7h16M10 11v6M14 11v6M6 7l1 13h10l1-13M9 7V4h6v3";
static const char* ST_INFO = "M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18zM12 11v6M12 7.5v.5";
static const char* ST_UNDO = "M9 14 4 9l5-5M4 9h10.5a5.5 5.5 0 0 1 0 11H11";
static const char* ST_REDO = "m15 14 5-5-5-5M20 9H9.5a5.5 5.5 0 0 0 0 11H13";
static const char* ST_SET = "M4 7h10M18 7h2M4 17h4M12 17h8M16 5a2 2 0 1 0 0 4 2 2 0 0 0 0-4zM10 15a2 2 0 1 0 0 4 2 2 0 0 0 0-4z";
static const char* ST_MORE = "M4 12h2M11 12h2M18 12h2";
static const char* ST_SEARCH = "M11 4.5a6.5 6.5 0 1 0 0 13 6.5 6.5 0 0 0 0-13zM20 20l-4.2-4.2";
static const char* ST_CLOSE = "M6 6l12 12M18 6 6 18";

// ------------------------------------------------------------------ the pictograms of the parts (isometric line art)
// kinds: brick plate tile slope round beam gear wheel window fig tree clock star dots

struct StPic {
    struct Face { std::vector<ImVec2> pts; bool fill = true; bool closed = true; };
    std::vector<Face> faces;
    ImVec2 mn{1e9f, 1e9f}, mx{-1e9f, -1e9f};
    ImVec2 P(float x, float y, float z) {
        const ImVec2 p((x - z) * 0.8660254f, (x + z) * 0.5f - y);
        mn = ImVec2(std::min(mn.x, p.x), std::min(mn.y, p.y));
        mx = ImVec2(std::max(mx.x, p.x), std::max(mx.y, p.y));
        return p;
    }
    void quad(std::initializer_list<float3> q) { Face f; for (const float3& v : q) f.pts.push_back(P(v.x, v.y, v.z)); faces.push_back(f); }
    void ellipseXZ(float cx, float y, float cz, float r, int n = 24) {   // a horizontal circle
        Face f;
        for (int i = 0; i < n; ++i) { const float t = i * 6.2831853f / n; f.pts.push_back(P(cx + r * std::cos(t), y, cz + r * std::sin(t))); }
        faces.push_back(f);
    }
    void cylinder(float cx, float cz, float r, float y0, float y1) {   // its side, then its top
        Face side;
        const int n = 16;
        for (int i = 0; i <= n; ++i) { const float t = 0.7853982f + i * 3.1415927f / n; side.pts.push_back(P(cx + r * std::cos(t), y0, cz + r * std::sin(t))); }
        for (int i = n; i >= 0; --i) { const float t = 0.7853982f + i * 3.1415927f / n; side.pts.push_back(P(cx + r * std::cos(t), y1, cz + r * std::sin(t))); }
        faces.push_back(side);
        ellipseXZ(cx, y1, cz, r);
    }
    void box(float w, float h, float l, bool studs) {
        quad({float3(0, 0, l), float3(w, 0, l), float3(w, h, l), float3(0, h, l)});
        quad({float3(w, 0, 0), float3(w, h, 0), float3(w, h, l), float3(w, 0, l)});
        quad({float3(0, h, 0), float3(w, h, 0), float3(w, h, l), float3(0, h, l)});
        if (studs)
            for (int s = 0; s < (int)(w + l - 1); ++s)
                for (int i = 0; i < (int)w; ++i) {
                    const int j = s - i;
                    if (j >= 0 && j < (int)l) cylinder(i + 0.5f, j + 0.5f, 0.3f, h, h + 0.18f);
                }
    }
};

static StPic stPicOf(const std::string& kind, int w, int l) {
    StPic p;
    if (kind == "brick") p.box((float)w, 1.2f, (float)l, true);
    else if (kind == "plate") p.box((float)w, 0.4f, (float)l, true);
    else if (kind == "tile") p.box((float)w, 0.4f, (float)l, false);
    else if (kind == "slope") {   // two studs at the back, the slanted face towards the viewer
        const float W = 2, L = 2, H = 1.2f, ys = 0.35f;
        p.quad({float3(0, 0, L), float3(W, 0, L), float3(W, ys, L), float3(1, H, L), float3(0, H, L)});
        p.quad({float3(W, 0, 0), float3(W, ys, 0), float3(W, ys, L), float3(W, 0, L)});
        p.quad({float3(0, H, 0), float3(1, H, 0), float3(1, H, L), float3(0, H, L)});
        p.quad({float3(1, H, 0), float3(W, ys, 0), float3(W, ys, L), float3(1, H, L)});
        p.cylinder(0.5f, 0.5f, 0.3f, H, H + 0.18f);
        p.cylinder(0.5f, 1.5f, 0.3f, H, H + 0.18f);
    } else if (kind == "round") { p.cylinder(1, 1, 0.98f, 0, 1.2f); p.cylinder(1, 1, 0.3f, 1.2f, 1.38f); }
    else if (kind == "beam") {   // a studless beam, its holes across the side
        const float L = 5;
        p.box(1, 1, L, false);
        for (int i = 0; i < (int)L; ++i) {
            StPic::Face f;
            for (int a = 0; a < 16; ++a) { const float t = a * 6.2831853f / 16; f.pts.push_back(p.P(1, 0.5f + 0.3f * std::sin(t), i + 0.5f + 0.3f * std::cos(t))); }
            p.faces.push_back(f);
        }
    }
    return p;
}

// a 2D pictogram in a 100 x 100 box: closed (filled) and open (lines) polylines
static void stFlat(const std::string& kind, int n, std::vector<StPic::Face>& out) {
    auto circ = [&](float cx, float cy, float r, bool fill = true) { StPic::Face f; f.fill = fill; for (int i = 0; i < 40; ++i) { const float t = i * 6.2831853f / 40; f.pts.push_back(ImVec2(cx + r * std::cos(t), cy + r * std::sin(t))); } out.push_back(f); };
    auto poly = [&](std::initializer_list<ImVec2> q, bool fill = true, bool closed = true) { StPic::Face f; f.pts = q; f.fill = fill; f.closed = closed; out.push_back(f); };
    if (kind == "wheel") {
        circ(50, 50, 40); circ(50, 50, 25); circ(50, 50, 7);
        for (int i = 0; i < 5; ++i) { const float t = -1.5707963f + i * 6.2831853f / 5; poly({ImVec2(50 + 7 * std::cos(t), 50 + 7 * std::sin(t)), ImVec2(50 + 25 * std::cos(t), 50 + 25 * std::sin(t))}, false, false); }
    } else if (kind == "gear") {
        StPic::Face f;
        const int t = std::clamp(n, 8, 16);
        for (int j = 0; j < t; ++j) {
            const float a0 = j * 6.2831853f / t, a1 = (j + 0.2f) * 6.2831853f / t, a2 = (j + 0.5f) * 6.2831853f / t, a3 = (j + 0.7f) * 6.2831853f / t;
            f.pts.push_back(ImVec2(50 + 32 * std::cos(a0), 50 + 32 * std::sin(a0)));
            f.pts.push_back(ImVec2(50 + 40 * std::cos(a1), 50 + 40 * std::sin(a1)));
            f.pts.push_back(ImVec2(50 + 40 * std::cos(a2), 50 + 40 * std::sin(a2)));
            f.pts.push_back(ImVec2(50 + 32 * std::cos(a3), 50 + 32 * std::sin(a3)));
        }
        out.push_back(f);
        circ(50, 50, 13);
        poly({ImVec2(44, 50), ImVec2(56, 50)}, false, false);
        poly({ImVec2(50, 44), ImVec2(50, 56)}, false, false);
    } else if (kind == "window") {
        poly({ImVec2(26, 10), ImVec2(74, 10), ImVec2(74, 90), ImVec2(26, 90)});
        poly({ImVec2(33, 17), ImVec2(67, 17), ImVec2(67, 83), ImVec2(33, 83)});
        poly({ImVec2(50, 17), ImVec2(50, 83)}, false, false);
        poly({ImVec2(33, 50), ImVec2(67, 50)}, false, false);
    } else if (kind == "fig") {
        poly({ImVec2(41, 12), ImVec2(59, 12), ImVec2(62, 16), ImVec2(62, 32), ImVec2(59, 36), ImVec2(41, 36), ImVec2(38, 32), ImVec2(38, 16)});
        poly({ImVec2(45, 6), ImVec2(55, 6), ImVec2(55, 12), ImVec2(45, 12)});
        poly({ImVec2(37, 40), ImVec2(63, 40), ImVec2(66, 70), ImVec2(34, 70)});
        poly({ImVec2(31, 42), ImVec2(37, 40), ImVec2(35, 60), ImVec2(29, 59)});
        poly({ImVec2(69, 42), ImVec2(63, 40), ImVec2(65, 60), ImVec2(71, 59)});
        poly({ImVec2(38, 70), ImVec2(49, 70), ImVec2(49, 92), ImVec2(38, 92)});
        poly({ImVec2(51, 70), ImVec2(62, 70), ImVec2(62, 92), ImVec2(51, 92)});
    } else if (kind == "tree") {
        poly({ImVec2(50, 80), ImVec2(50, 92)}, false, false);
        poly({ImVec2(50, 44), ImVec2(82, 80), ImVec2(18, 80)});
        poly({ImVec2(50, 26), ImVec2(76, 58), ImVec2(24, 58)});
        poly({ImVec2(50, 10), ImVec2(70, 38), ImVec2(30, 38)});
    } else if (kind == "clock") {
        circ(50, 50, 38);
        poly({ImVec2(50, 26), ImVec2(50, 50), ImVec2(66, 60)}, false, false);
    } else if (kind == "star") {
        StPic::Face f;
        for (int i = 0; i < 10; ++i) { const float t = -1.5707963f + i * 3.1415927f / 5, r = (i & 1) ? 17 : 38; f.pts.push_back(ImVec2(50 + r * std::cos(t), 52 + r * std::sin(t))); }
        out.push_back(f);
    } else if (kind == "dots") {
        circ(24, 50, 6); circ(50, 50, 6); circ(76, 50, 6);
    }
}

// draws a pictogram centred on c in a box of `size` pixels: thin stroke, faces tinted (the hidden edges show through)
static void stPicto(ImDrawList* dl, const std::string& kind, int w, int l, ImVec2 c, float size, ImU32 stroke, float thick) {
    const ImU32 fill = IM_COL32(225, 228, 234, 14);
    std::vector<StPic::Face> faces;
    ImVec2 mn, mx;
    const bool flat = kind == "wheel" || kind == "gear" || kind == "window" || kind == "fig" || kind == "tree" || kind == "clock" || kind == "star" || kind == "dots";
    if (flat) { stFlat(kind, w, faces); mn = ImVec2(0, 0); mx = ImVec2(100, 100); }
    else { StPic p = stPicOf(kind, std::max(1, w), std::max(1, l)); faces = p.faces; mn = p.mn; mx = p.mx; }
    const float ext = std::max(mx.x - mn.x, mx.y - mn.y);
    if (ext <= 0) return;
    const float k = size / ext;
    const ImVec2 m((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
    std::vector<ImVec2> pts;
    for (const StPic::Face& f : faces) {
        pts.clear();
        for (const ImVec2& q : f.pts) pts.push_back(ImVec2(c.x + (q.x - m.x) * k, c.y + (q.y - m.y) * k));
        if (f.fill && f.closed && pts.size() >= 3) dl->AddConvexPolyFilled(pts.data(), (int)pts.size(), fill);
        dl->AddPolyline(pts.data(), (int)pts.size(), stroke, f.closed ? ImDrawFlags_Closed : 0, thick);
    }
}

// the pictogram of a part of the library, from its LDraw category and title
static void stPartKind(const Part& p, std::string& kind, int& w, int& l) {
    const std::string& c = p.category;
    w = l = 0;
    {   // « Brick  2 x  4 », « Plate  1 x  6 »: the size in studs
        const std::string& t = p.label;
        for (size_t i = 0; i < t.size(); ++i) {
            if (t[i] != 'x' || i == 0 || i + 1 >= t.size()) continue;
            size_t a = i, b = i + 1;
            while (a > 0 && t[a - 1] == ' ') --a;
            while (b < t.size() && t[b] == ' ') ++b;
            size_t a0 = a;
            while (a0 > 0 && std::isdigit((unsigned char)t[a0 - 1])) --a0;
            size_t b1 = b;
            while (b1 < t.size() && std::isdigit((unsigned char)t[b1])) ++b1;
            if (a0 < a && b < b1) { w = std::atoi(t.substr(a0, a - a0).c_str()); l = std::atoi(t.substr(b, b1 - b).c_str()); break; }
        }
    }
    if (w > 6 || l > 8) { const float k = std::max(w / 6.0f, l / 8.0f); w = std::max(1, (int)(w / k)); l = std::max(1, (int)(l / k)); }
    if (w > l) std::swap(w, l);
    if (c == "Brick") kind = "brick";
    else if (c == "Plate" || c == "Baseplate") kind = "plate";
    else if (c == "Tile") kind = "tile";
    else if (c == "Slope" || c == "Wedge") kind = "slope";
    else if (c == "Cylinder" || c == "Cone" || c == "Dish" || c == "Sphere") kind = "round";
    else if (c == "Technic") kind = p.sub == "Engrenages" ? "gear" : "beam";
    else if (c == "Wheel" || c == "Tyre") kind = "wheel";
    else if (c == "Window" || c == "Door" || c == "Glass") kind = "window";
    else if (c.rfind("Minifig", 0) == 0 || c.rfind("Figure", 0) == 0) kind = "fig";
    else if (c == "Plant" || c == "Animal" || c == "Rock") kind = "tree";
    else kind = "brick";
    if (kind == "gear") w = 12;
}

// ------------------------------------------------------------------ the families of the drawer (Maquette 4.1)
// The nine families of before (Basique ... Autres gammes) and Récentes / Favoris; each family has its LDraw categories
// (Technic: its sub-families). A category takes the parts of its LDraw categories; with `tsub`, the Technic ones only
// when their sub-family is that one. `rest`: everything that no family takes.

#include "BrickStudioIcons.inl"

// a pictogram of the maquette centred on c, `size` pixels: thin stroke, faces lightly tinted (trans-clear)
static void stIco(ImDrawList* dl, const char* key, ImVec2 c, float size, ImU32 stroke, float thick) {
    static std::unordered_map<std::string, const StIco*> idx;
    if (idx.empty()) for (const StIco& i : stIcons()) idx[i.key] = &i;
    auto it = idx.find(key);
    if (it == idx.end()) it = idx.find("dots");
    const StIco& I = *it->second;
    const float k = size / I.vw;
    const ImVec2 o(c.x - size * 0.5f - I.vx * k, c.y - size * 0.5f - I.vy * k);
    const ImU32 face = IM_COL32(225, 228, 234, (int)(15 * ((stroke >> 24) & 0xFF) / 255.0f));
    for (const StIcoPath& q : I.paths) svg::shape(dl, q.d, o, k, q.fill ? face : 0, stroke, thick);
}

struct StSub { const char* label; const char* ico; std::vector<const char*> cats; const char* tsub; bool rest; };
struct StFam { const char* key; const char* label; const char* ico; std::vector<StSub> subs; };
static const std::vector<StFam>& stFams() {
    auto T = [](const char* l, const char* ico, const char* sub) { return StSub{l, ico, {"Technic"}, sub, false}; };
    auto C = [](const char* l, const char* ico, std::vector<const char*> cats) { return StSub{l, ico, cats, nullptr, false}; };
    static const std::vector<StFam> f = {
        {"recent", "Récentes", "clock", {}},
        {"fav", "Favoris", "star", {}},
        {"basique", "Basique", "brick", {C("Briques", "brick", {"Brick"}), C("Plaques", "plate", {"Plate"}), C("Tuiles", "tile", {"Tile"}),
                                         C("Plaques de base", "baseplate", {"Baseplate"}), C("Équerres", "bracket", {"Bracket"}), C("Panneaux", "panel", {"Panel"})}},
        {"formes", "Formes", "slope", {C("Pentes", "slope", {"Slope"}), C("Biseaux", "wedge", {"Wedge"}), C("Arches", "arch", {"Arch"}), C("Cylindres", "round", {"Cylinder"}),
                                       C("Cônes", "cone", {"Cone"}), C("Paraboles", "dish", {"Dish"}), C("Sphères", "sphere", {"Sphere"})}},
        {"technic", "Technic", "beam", {T("Poutres", "beam", "Poutres"), T("Axes", "axle", "Axes"), T("Pins", "pin", "Pins"), T("Connecteurs", "connector", "Connecteurs"),
                                        T("Engrenages", "gear", "Engrenages"), T("Briques", "tbrick", "Briques"), T("Panneaux", "tpanel", "Panneaux"), T("Direction", "steering", "Direction"),
                                        T("Pneumatique", "pneumatic", "Pneumatique"), T("Chenilles", "tread", "Chenilles"), T("Moteurs", "motor", "Moteurs"), T("Rotation", "turntable", "Rotation"),
                                        T("Flex", "flex", "Flex"), C("Électrique", "bolt", {"Electric"}), C("Znap", "znap", {"Znap"}),
                                        StSub{"Bionicle", "mask", {"Technic", "Constraction", "Constraction Accessory"}, "Bionicle", false}, T("Divers", "dots", "Divers")}},
        {"mecanique", "Mécanique", "gear8", {C("Charnières", "hinge", {"Hinge"}), C("Plateaux tournants", "turntable", {"Turntable"}), C("Barres", "bar", {"Bar"}),
                                             C("Leviers", "lever", {"Lever"}), C("Treuils", "winch", {"Winch"}), C("Grues", "crane", {"Crane"}), C("Pinces", "grab", {"Grab"}),
                                             C("Aimants", "magnet", {"Magnet"}), C("Convoyeurs", "conveyor", {"Conveyor"}), C("Vis", "screw", {"Screw"}),
                                             C("Crémaillères", "rack", {"Rack"}), C("Tuyaux", "hose", {"Hose"}), C("Robinets", "tap", {"Tap"})}},
        {"vehicules", "Véhicules", "car", {C("Roues", "wheel", {"Wheel"}), C("Pneus", "tyre", {"Tyre"}), C("Voitures", "car", {"Car"}), C("Véhicules", "vehicle", {"Vehicle"}),
                                           C("Trains", "train", {"Train"}), C("Monorail", "monorail", {"Monorail"}), C("Bateaux", "boat", {"Boat"}), C("Avions", "plane", {"Plane"}),
                                           C("Ailes", "wing", {"Wing"}), C("Dérives", "tail", {"Tail"}), C("Hélices", "propeller", {"Propeller"}), C("Cockpits", "cockpit", {"Cockpit"}),
                                           C("Pare-brise", "windscreen", {"Windscreen"}), C("Échappements", "exhaust", {"Exhaust"}), C("Tracteurs", "tractor", {"Tractor"}),
                                           C("Remorques", "trailer", {"Trailer"}), C("Bennes", "tipper", {"Tipper"}), C("Chariots", "forklift", {"Forklift"})}},
        {"batiment", "Bâtiment", "window", {C("Portes", "door", {"Door"}), C("Fenêtres", "windowf", {"Window"}), C("Vitres", "glass", {"Glass"}), C("Clôtures", "fence", {"Fence"}),
                                            C("Échelles", "ladder", {"Ladder"}), C("Escaliers", "stairs", {"Staircase"}), C("Supports", "support", {"Support"}),
                                            C("Plateformes", "platform", {"Platform"}), C("Garages", "garage", {"Garage"}), C("Conteneurs", "container", {"Container"}),
                                            C("Panneaux routiers", "roadsign", {"Roadsign"}), C("Drapeaux", "flag", {"Flag"}), C("Antennes", "antenna", {"Antenna"})}},
        {"personnages", "Personnages", "fig", {C("Minifigs", "fig", {"Minifig"}), C("Accessoires", "sword", {"Minifig Accessory"}), C("Chaussures", "footwear", {"Minifig Footwear"}),
                                               C("Coiffes", "headwear", {"Minifig Headwear"}), C("Bas", "hipwear", {"Minifig Hipwear"}), C("Cols et capes", "neckwear", {"Minifig Neckwear"}),
                                               C("Figurines", "figure", {"Figure"}), C("Acc. figurines", "handbag", {"Figure Accessory"}), C("Bras", "arm", {"Arm"})}},
        {"nature", "Nature", "tree", {C("Animaux", "animal", {"Animal"}), C("Plantes", "leaves", {"Plant"}), C("Rochers", "rock", {"Rock"})}},
        {"autres", "Autres gammes", "dots", {C("Scala", "figure", {"Scala"}), C("Belville", "flower", {"Belville"}), C("Clikits", "heart", {"Clikits"}),
                                              C("Homemaker", "homemaker", {"Homemaker"}), C("Freestyle", "dots", {"Freestyle"}), C("Autocollants", "sticker", {"Sticker"}),
                                              C("Carton", "cardboard", {"Sheet Cardboard"}), C("Tissu", "fabric", {"Sheet Fabric"}), C("Plastique", "sheet", {"Sheet Plastic"}),
                                              C("Obsolètes", "obsolete", {"Obsolete"}), C("POV-Ray", "povray", {"Pov-RAY"}), StSub{"Divers", "dots", {}, nullptr, true}}}};
    return f;
}
// does a part (its LDraw category, its Technic sub-family) belong to this category of the drawer
static bool stInSub(const StSub& q, const std::string& cat, const std::string& sub) {
    if (q.rest) {
        for (const StFam& f : stFams())
            for (const StSub& o : f.subs)
                if (!o.rest && stInSub(o, cat, sub)) return false;
        return true;
    }
    for (const char* k : q.cats)
        if (cat == k) return !q.tsub || cat != "Technic" || sub == q.tsub;
    return false;
}

// ------------------------------------------------------------------ header

void BrickApp::stHeader() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // iPad: a strip of a few points above the bar, where a finger coming down from the top of the screen (iPadOS:
    // notifications, Control Centre) lands without pressing Exporter (BrickTouch ignores the touches that start there)
    const float T = m_ipad ? kIpadTopStrip * s : 0;
    const float H = 46 * s + T;
    u.dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(d.x, H), IM_COL32(8, 9, 11, 235), IM_COL32(8, 9, 11, 235), IM_COL32(12, 13, 15, 200), IM_COL32(12, 13, 15, 200));
    u.dl->AddLine(ImVec2(0, H - 0.5f), ImVec2(d.x, H - 0.5f), IM_COL32(255, 255, 255, 20), 1.0f);
    m_uiRects.push_back(ImVec4(0, 0, d.x, H));
    const float cy = T + 23 * s;
    // the logo: a red square, a brick pictogram in white; BRICKIT / B U I L D E R
    float x = 14 * s;
    if (m_ipad) {   // iPad (Wzx 10/11/2026): the app icon, a 2x4 brick in thin amber lines on smoked grey
        const ImVec2 la(x, cy - 15 * s), lb(x + 30 * s, cy + 15 * s);
        u.dl->AddRectFilled(la, lb, IM_COL32(38, 40, 45, 255), 7 * s);
        u.dl->AddRect(la, lb, IM_COL32(255, 255, 255, 40), 7 * s, 0, 1.0f);
        stPicto(u.dl, "brick", 2, 4, ImVec2(x + 15 * s, cy), 24 * s, IM_COL32(226, 191, 79, 255), 1.2f * s);
    } else {
        u.dl->AddRectFilled(ImVec2(x, cy - 14 * s), ImVec2(x + 28 * s, cy + 14 * s), IM_COL32(0xd0, 0x10, 0x1b, 255), 3 * s);
        stPicto(u.dl, "brick", 2, 2, ImVec2(x + 14 * s, cy), 18 * s, IM_COL32(255, 255, 255, 235), 1.1f * s);
    }
    x += 38 * s;
    stTrack(u, ImVec2(x, cy - 15 * s), 13, C_TEXT, "BRICKIT", 1.6f * s);
    stTrack(u, ImVec2(x, cy + 1 * s), 8, C_FAINT, "BUILDER", 3.2f * s, false);
    x += 92 * s;
    u.dl->AddLine(ImVec2(x, cy - 11 * s), ImVec2(x, cy + 11 * s), IM_COL32(255, 255, 255, 30), 1.0f);
    x += 12 * s;
    {   // the scene menu: a click on the logo or on the name (Nouvelle scène, Ouvrir, Enregistrer)
        const ImVec2 fa(0, T), fb(x + 190 * s, H);
        const bool hf = u.hover(fa, fb) && (m_popup == 0 || m_popup == 8);
        if (hf || m_popup == 8) u.dl->AddRectFilled(ImVec2(x - 6 * s, cy - 17 * s), ImVec2(x + 180 * s, cy + 17 * s), IM_COL32(255, 255, 255, m_popup == 8 ? 22 : 12), 10 * s);
        {   // a small chevron after the name
            const ImVec2 c(x + 170 * s, cy - 8 * s);
            u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - 3.5f * s, c.y - 1.8f * s), ImVec2(c.x, c.y + 1.8f * s), ImVec2(c.x + 3.5f * s, c.y - 1.8f * s)}.data(), 3, C_DIM, 0, 1.3f * s);
        }
        if (hf && ImGui::GetIO().MouseDownDuration[0] < 0 && m_popup == 0) ImGui::SetTooltip("Scène : nouvelle, ouvrir, enregistrer");
        if (hf && u.pressed) { m_popup = m_popup == 8 ? 0 : 8; m_popupAt = ImVec2(10 * s, H + 6 * s); }
    }
    {   // the scene: its name, the count of pieces
        const std::string nm = stUpper(m_modelName.empty() ? std::string("Ma scène") : m_modelName);
        stTrack(u, ImVec2(x, cy - 15 * s), 11, C_TEXT, nm.c_str(), 1.4f * s);
        char b[64];
        snprintf(b, sizeof(b), "%d pièces", (int)m_bricks.size());
        u.text(ImVec2(x, cy + 1 * s), 9.5f, C_DIM, b);
    }
    // the modes, in the middle: a segmented pill
    struct Seg { const char* label; bool on; int id; };
    const bool notice = m_pManual.visible;
    std::vector<Seg> modes = {{"CONSTRUIRE", !m_mechOn && !m_paintMode && !notice, 0}, {"PEINDRE", m_paintMode && !m_mechOn, 1}, {"MÉCANIQUE", m_mechOn, 2}, {"NOTICE", notice && !m_mechOn, 3}};
    auto segW = [&](const char* t) { return stTrackW(u, 10, t, 1.6f * s, true) + 26 * s; };
    float total = 6 * s;
    for (const Seg& m : modes) total += segW(m.label) + 2 * s;
    const float sh = 30 * s;
    ImVec2 a((d.x - total) * 0.5f, cy - sh * 0.5f), b(a.x + total, cy + sh * 0.5f);
    u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), sh * 0.5f);
    u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 34), sh * 0.5f, 0, 1.0f);
    float mx = a.x + 3 * s;
    for (const Seg& m : modes) {
        const float w = segW(m.label);
        const ImVec2 ma(mx, a.y + 3 * s), mb(mx + w, b.y - 3 * s);
        const bool h = u.hover(ma, mb) && m_popup == 0;
        if (m.on) u.dl->AddRectFilled(ma, mb, IM_COL32(255, 255, 255, 34), (mb.y - ma.y) * 0.5f);
        else if (h) u.dl->AddRectFilled(ma, mb, IM_COL32(255, 255, 255, 14), (mb.y - ma.y) * 0.5f);
        const float tw = stTrackW(u, 10, m.label, 1.6f * s, true);
        stTrack(u, ImVec2((ma.x + mb.x - tw) * 0.5f, (ma.y + mb.y) * 0.5f - u.fs(10) * 0.52f), 10, m.on ? C_TEXT : C_DIM, m.label, 1.6f * s);
        if (h && u.pressed) {
            switch (m.id) {
            case 0: if (m_mechOn) mechExit(); m_paintMode = false; m_paintTool = false; m_pipette = false; m_pManual.visible = false; break;
            case 1: if (m_mechOn) mechExit(); m_paintMode = true; m_paintTool = true; m_pipette = false; break;
            case 2: if (m_mechOn) mechExit(); else mechEnter(); m_paintMode = false; m_paintTool = false; m_pipette = false; break;
            case 3: m_pManual.visible = !m_pManual.visible; break;
            }
        }
        mx += w + 2 * s;
    }
    // on the right: undo / redo, the render, more, settings, Exporter
    float rx = d.x - 14 * s;
    {   // Exporter: the accent pill (the scene: open / save)
        const float w = stTrackW(u, 10, "EXPORTER", 1.6f * s, true) + 30 * s;
        const ImVec2 ea(rx - w, cy - 15 * s), eb(rx, cy + 15 * s);
        const bool h = u.hover(ea, eb);
        u.dl->AddRectFilled(ea, eb, h ? stA(C_ACCENT, 0.85f) : C_ACCENT, 15 * s);
        stTrack(u, ImVec2(ea.x + 15 * s, cy - u.fs(10) * 0.52f), 10, C_INK, "EXPORTER", 1.6f * s);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Scène : ouvrir, enregistrer, exporter (.v3d.json, LDraw, notice)");
        if (h && u.pressed) { m_popup = m_popup == 4 ? 0 : 4; m_popupAt = ImVec2(rx - 260 * s, H + 6 * s); }
        rx -= w + 8 * s;
    }
    auto roundBtn = [&](const char* icon, const char* tip, bool on, bool enabled) {
        const float r = 15 * s;
        const ImVec2 c(rx - r, cy);
        const bool h = enabled && (u.mouse.x - c.x) * (u.mouse.x - c.x) + (u.mouse.y - c.y) * (u.mouse.y - c.y) <= r * r && (m_popup == 0 || on);   // (open: a second click closes it)
        if (on || h) u.dl->AddCircleFilled(c, r, IM_COL32(255, 255, 255, on ? 34 : 16), 32);
        u.dl->AddCircle(c, r, IM_COL32(255, 255, 255, 30), 32, 1.0f);
        stIcon(u.dl, icon, c, 17 * s, enabled ? C_TEXT : stA(C_TEXT, 0.3f), 1.3f * s);
        if (h && tip && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", tip);
        rx -= 2 * r + 6 * s;
        return h && u.pressed;
    };
    if (roundBtn(ST_SET, "Réglages (échelle, rendu, lumière) et Thème", m_popup == 5, true)) { m_popup = m_popup == 5 ? 0 : 5; m_popupAt = ImVec2(d.x - 660 * s, H + 6 * s); }
    if (roundBtn(ST_MORE, "Plus : grille, auto pivot, sécurité, animations, voxels, fenêtres, DEV, bac", m_popup == 7, true)) { m_popup = m_popup == 7 ? 0 : 7; m_popupAt = ImVec2(rx - 200 * s, H + 6 * s); }
    if (!m_ipad) {   // the render: Rapide / HQ / Ray tracing (iPad: none, Wzx 10/11/2026)
        struct R { const char* label; bool on; int id; const char* tip; };
        std::vector<R> rs = {{"RAPIDE", m_rs.raster && m_quality == 0 && !m_rcMode, 0, "Temps réel rapide"}, {"HQ", m_rs.raster && m_quality == 1 && !m_rcMode, 1, "Temps réel haute qualité : ombres, GI douce"},
                             {"RAY TRACING", pathTraced(), 2, "Rendu progressif réaliste (path tracing)"}};
        float w = 6 * s;
        for (const R& r : rs) w += stTrackW(u, 9.5f, r.label, 1.3f * s, true) + 20 * s + 2 * s;
        const ImVec2 ra(rx - w, cy - 14 * s), rb(rx, cy + 14 * s);
        u.dl->AddRectFilled(ra, rb, IM_COL32(255, 255, 255, 10), 14 * s);
        u.dl->AddRect(ra, rb, IM_COL32(255, 255, 255, 34), 14 * s, 0, 1.0f);
        float xx = ra.x + 3 * s;
        for (const R& r : rs) {
            const float bw = stTrackW(u, 9.5f, r.label, 1.3f * s, true) + 20 * s;
            const ImVec2 ba(xx, ra.y + 3 * s), bb(xx + bw, rb.y - 3 * s);
            const bool h = u.hover(ba, bb) && m_popup == 0;
            if (r.on) u.dl->AddRectFilled(ba, bb, C_ACCENT, (bb.y - ba.y) * 0.5f);
            else if (h) u.dl->AddRectFilled(ba, bb, IM_COL32(255, 255, 255, 16), (bb.y - ba.y) * 0.5f);
            stTrack(u, ImVec2(ba.x + 10 * s, (ba.y + bb.y) * 0.5f - u.fs(9.5f) * 0.52f), 9.5f, r.on ? C_INK : C_DIM, r.label, 1.3f * s);
            if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", r.tip);
            if (h && u.pressed && !r.on) {
                if (r.id == 2) { setRC(false); m_rs.raster = false; applyLook(); }
                else { setRC(false); m_quality = r.id; m_rs.msaaSamples = m_quality ? 4 : 1; if (!m_rs.raster) { m_rs.raster = true; applyLook(); } }
                m_renderer->resetAccumulation();
                savePrefs();
            }
            xx += bw + 2 * s;
        }
        rx -= w + 10 * s;
    }
    if (!m_ipad) {   // HD: the parts in high definition (Wzx 10/11/2026: « un switcher HighPoly »; not on the iPad: too heavy)
        const char* t = "HD";
        const float bw = stTrackW(u, 9.5f, t, 1.3f * s, true) + 26 * s;
        const ImVec2 ba(rx - bw, cy - 14 * s), bb(rx, cy + 14 * s);
        const bool h = u.hover(ba, bb) && m_popup == 0, on = m_highPoly;
        u.dl->AddRectFilled(ba, bb, on ? C_ACCENT : IM_COL32(255, 255, 255, h ? 20 : 10), 14 * s);
        u.dl->AddRect(ba, bb, on ? C_ACCENT : IM_COL32(255, 255, 255, 34), 14 * s, 0, 1.0f);
        stTrack(u, ImVec2(ba.x + 13 * s, cy - u.fs(9.5f) * 0.52f), 9.5f, on ? C_INK : (m_lib.hasHd ? C_DIM : C_FAINT), t, 1.3f * s);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0)
            ImGui::SetTooltip("%s", m_lib.hasHd ? "Pièces HD : les arrondis lisses (primitives 48 segments, tenons biseautés) quand elles existent" : "Pièces HD : la bibliothèque HD n'est pas encore construite");
        if (h && u.pressed) setHighPoly(!m_highPoly);
        rx -= bw + 10 * s;
    }
    if (roundBtn(ST_REDO, "Rétablir (Ctrl+Y)", false, !m_redo.empty())) redo();
    if (roundBtn(ST_UNDO, "Annuler (Ctrl+Z)", false, !m_undo.empty() || m_holding)) undo();
}

// ------------------------------------------------------------------ the tools (left column)

void BrickApp::stTools() {
    if (m_mechOn) return;
    if (!m_paintMode) { m_paintT0 = -1; m_paintSwatchAt = ImVec2(-1, -1); }   // (the colours of PEINDRE come out again next time)
    if (m_paintMode) { stPaintPanel(); stPaintColors(); return; }   // (the tools under the strip of colours)   // (PEINDRE: no tools on the left)
    Ui u = ui(m_ui);
    const float s = m_ui;
    const float top = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 12 * s;
    struct T { const char* icon; const char* tip; const char* key; bool on; int id; };
    std::vector<T> ts = {
        {ST_SEL, "Sélection : clic sur une brique, rectangle pour plusieurs (1)", "1", m_mode == 0 && !m_paintTool && !m_pipette, 0},
        {ST_BUILD, "Construire : glisse une brique pour la déplacer (2)", "2", m_mode == 1 && !m_paintTool && !m_pipette, 1},
        {ST_PAINT, "Pinceau : clic sur une brique pour la peindre (3)", "3", m_paintTool, 2},
        {ST_PICK, "Pipette : clic sur une brique pour prendre sa couleur (4)", "4", m_pipette, 3},
        {ST_MOVE, "Déplacer : flèches sur la sélection (5)", "5", m_mode == 2 && !m_paintTool && !m_pipette, 4},
        {ST_ROT, "Tourner : anneaux sur la sélection (6)", "6", m_mode == 3 && !m_paintTool && !m_pipette, 5},
        {ST_EYE, "Isoler : la sélection reste claire, le reste à 50 % (F)", "F", m_focusDim, 7}};
    const float W = 50 * s, bh = 44 * s, pad = 4 * s;
    // iPad (Wzx 10/11/2026): a row at the top centre (the left hand keeps the ring of the side); PC: the column on the left
    const bool row = m_ipad;
    const float bw = row ? 46 * s : W - pad * 2;
    const float rowW = pad * 2 + ts.size() * (bw + 2 * s) - 2 * s;
    const ImVec2 a = row ? ImVec2(std::floor(ImGui::GetIO().DisplaySize.x * 0.5f - rowW * 0.5f), top) : ImVec2(14 * s + m_dockL, top);
    const ImVec2 b = row ? ImVec2(a.x + rowW, top + bh + pad * 2) : ImVec2(a.x + W, top + pad * 2 + ts.size() * (bh + 2 * s) - 2 * s);
    stGlass(u.dl, a, b, 18 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad, x = a.x + pad;
    for (const T& t : ts) {
        const ImVec2 ba = row ? ImVec2(x, a.y + pad) : ImVec2(a.x + pad, y), bb = row ? ImVec2(x + bw, b.y - pad) : ImVec2(b.x - pad, y + bh);
        x += bw + 2 * s;
        const bool h = u.hover(ba, bb) && m_popup == 0;
        if (t.on) {   // the active tool: a translucent yellow, a fine rim
            u.dl->AddRectFilled(ba, bb, stA(C_ACCENT, 0.2f), 13 * s);
            u.dl->AddRect(ba, bb, stA(C_ACCENT, 0.8f), 13 * s, 0, 1.0f);
        } else if (h) u.dl->AddRectFilled(ba, bb, IM_COL32(255, 255, 255, 16), 13 * s);
        stIcon(u.dl, t.icon, ImVec2((ba.x + bb.x) * 0.5f, (ba.y + bb.y) * 0.5f), 19 * s, t.on ? C_ACCENT_TXT : C_TEXT, 1.2f * s);
        if (t.key[0] && !m_ipad) u.text(ImVec2(bb.x - 9 * s, bb.y - 13 * s), 7.5f, t.on ? C_ACCENT_TXT : C_FAINT, t.key, true);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", t.tip);
        if (h && u.pressed) stTool(t.id);
        y += bh + 2 * s;
    }
}

void BrickApp::stTool(int id) {
    switch (id) {
    case 0: case 1: case 4: case 5: m_mode = id == 0 ? 0 : id == 1 ? 1 : id == 4 ? 2 : 3; m_paintTool = false; m_pipette = false; m_paintMode = false; break;
    case 2: m_paintTool = !m_paintTool; m_pipette = false; if (m_paintTool) toast("Pinceau : clique une brique pour lui donner la couleur choisie"); break;
    case 3: m_pipette = !m_pipette; m_paintTool = false; break;
    case 6: if (m_mechOn) mechExit(); else mechEnter(); break;
    case 7: m_focusDim = !m_focusDim; syncObjects(); break;
    }
}

// the badge top left of the view: 1 : 1 | GRILLE 1 TENON (a click: the grid on / off)
void BrickApp::stBadge() {
    if (m_mechOn) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const float top = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 12 * s;
    const char* g = m_build.gridSnap ? "GRILLE 1 TENON" : "SANS GRILLE";
    const float w1 = stTrackW(u, 9, "1 : 1", 1.2f * s, true) + 16 * s, w2 = stTrackW(u, 9, g, 1.2f * s, true) + 16 * s;
    float bx = 76 * s + m_dockL;   // right of the panels on the left (tools / Peindre, Groupes)
    for (size_t k = 2; k < m_uiRects.size(); ++k) if (m_uiRects[k].x < 200 * s && m_uiRects[k].y < top + 30 * s) bx = std::max(bx, m_uiRects[k].z + 10 * s);
    const ImVec2 a(bx, top), b(a.x + w1 + w2, top + 22 * s);
    const bool h = u.hover(a, b) && m_popup == 0;
    u.dl->AddRectFilled(a, b, IM_COL32(10, 11, 13, h ? 170 : 120), 11 * s);
    u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 50), 11 * s, 0, 1.0f);
    u.dl->AddLine(ImVec2(a.x + w1, a.y), ImVec2(a.x + w1, b.y), IM_COL32(255, 255, 255, 50), 1.0f);
    stTrack(u, ImVec2(a.x + 8 * s, (a.y + b.y) * 0.5f - u.fs(9) * 0.52f), 9, C_TEXT, "1 : 1", 1.2f * s);
    stTrack(u, ImVec2(a.x + w1 + 8 * s, (a.y + b.y) * 0.5f - u.fs(9) * 0.52f), 9, C_DIM, g, 1.2f * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Grille : snap sur la grille du sol (G)");
    if (h && u.pressed) { m_build.gridSnap = !m_build.gridSnap; toast(m_build.gridSnap ? "Grille : oui" : "Grille : non"); }
}

// « … »: the toggles of the old top bar
void BrickApp::stMore() {
    if (m_popup != 7) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    struct It { const char* label; bool on; int id; };
    std::vector<It> its = {{"Grille (G)", m_build.gridSnap, 0}, {"Auto pivot", m_autoPivot, 1}, {"Sécurité (refus > 50 %)", m_build.safety, 2},
                                 {"Animations des briques (A)", m_animOn, 3}, {"Voxels (V)", m_showVox, 4}, {"Groupes", m_pGroups.visible, 5},
                                 {"Mode développeur (DEV)", m_dev, 7}, {"Transformation de vue : AgX", m_rs.tonemapper == 1, 8}};
    if (m_ipad)   // iPad (Wzx 10/11/2026): no voxels, no developer mode, no view transform
        its.erase(std::remove_if(its.begin(), its.end(), [](const It& t) { return t.id == 4 || t.id == 7 || t.id == 8; }), its.end());
    const float W = 240 * s, ih = 28 * s, pad = 6 * s;
    const ImVec2 a(m_popupAt.x, m_popupAt.y), b(a.x + W, a.y + pad * 2 + its.size() * ih);
    stGlass(u.dl, a, b, 14 * s, s);
    u.dl->AddRectFilled(a, b, IM_COL32(16, 17, 19, 150), 14 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    bool clicked = false;
    for (const It& it : its) {
        const ImVec2 ia(a.x + pad, y), ib(b.x - pad, y + ih);
        const bool h = u.hover(ia, ib);
        if (h) u.dl->AddRectFilled(ia, ib, IM_COL32(255, 255, 255, 16), 9 * s);
        {   // a small switch
            const ImVec2 sa(ib.x - 34 * s, (ia.y + ib.y) * 0.5f - 8 * s), sb(ib.x - 8 * s, (ia.y + ib.y) * 0.5f + 8 * s);
            u.dl->AddRectFilled(sa, sb, it.on ? stA(C_ACCENT, 0.85f) : IM_COL32(255, 255, 255, 30), 8 * s);
            u.dl->AddCircleFilled(ImVec2(it.on ? sb.x - 8 * s : sa.x + 8 * s, (sa.y + sb.y) * 0.5f), 6 * s, it.on ? C_INK : IM_COL32(220, 222, 226, 255), 20);
        }
        u.text(ImVec2(ia.x + 8 * s, (ia.y + ib.y) * 0.5f - u.fs(11.5f) * 0.55f), 11.5f, C_TEXT, it.label);
        if (h && u.pressed) {
            clicked = true;
            switch (it.id) {
            case 0: m_build.gridSnap = !m_build.gridSnap; break;
            case 1: m_autoPivot = !m_autoPivot; break;
            case 2: m_build.safety = !m_build.safety; break;
            case 3: m_animOn = !m_animOn; break;
            case 4: m_showVox = !m_showVox; syncObjects(); break;
            case 5: m_pGroups.visible = !m_pGroups.visible; break;
            case 6: m_bacOpen = !m_bacOpen; break;
            case 7: devSetOn(!m_dev); break;
            case 8: m_rs.tonemapper = m_rs.tonemapper == 1 ? 0 : 1; m_renderer->resetAccumulation(); break;
            }
            savePrefs();
        }
        y += ih;
    }
    if (u.pressed && !clicked && !u.hover(a, b) && u.mouse.y > (m_uiRects.empty() ? 0 : m_uiRects[0].w)) m_popup = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_popup = 0;
}


// ------------------------------------------------------------------ Peindre: the big Pinceau / Pipette, Groupe entier

static std::string stFit(Ui& u, float px, std::string t, float maxW, bool bold);
static ImU32 stSrgb(const Color& c, int alpha) {
    return IM_COL32((int)(std::clamp(c.srgb.x, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.y, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.z, 0.0f, 1.0f) * 255), alpha);
}

// the mode PEINDRE (Wzx 09/11/2026): under the header in the middle, the colour taken in big (a 1x1 brick), the Pinceau, the Pipette,
// « Groupe entier »; around it every colour on an arc (stPaintColors), out one by one like the marking menu
void BrickApp::stPaintPanel() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const bool small = m_ipad;   // iPad (Wzx 10/11/2026): twice smaller, the icons without their labels
    const float s = m_ui * (small ? 0.5f : 1.0f);
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float top = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 16 * s;
    const float bw = 92 * s, bh = 76 * s, pad = 8 * s, swW = 128 * s, cw = 96 * s;
    const float W = pad + cw + 14 * s + 2 * bw + 6 * s + 14 * s + swW + pad, H = bh + 2 * pad;
    const ImVec2 a((m_dockL + d.x - m_dockR - W) * 0.5f, top), b(a.x + W, top + H);   // (the middle of the view)
    stGlass(u.dl, a, b, 20 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float x = a.x + pad;
    if (!m_lib.palette.empty()) {   // the colour taken, in big: a 1x1 brick, its name under it
        const Color& c = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
        const float tp = (float)(ImGui::GetTime() - m_paintSwatchPop);   // the drop of the Pipette landed: a pop
        const float pop = tp >= 0 && tp < 0.35f ? 1.0f + 0.22f * std::sin(tp / 0.35f * 3.14159f) : 1.0f;
        const float sz = 50 * s * pop, cx = x + cw * 0.5f, y0 = a.y + pad + 3 * s + 25 * s - sz * 0.5f;
        m_paintSwatchAt = ImVec2(cx, a.y + pad + 28 * s);
        const ImVec2 p0(cx - sz * 0.5f, y0), p1(cx + sz * 0.5f, y0 + sz);
        u.dl->AddRectFilled(ImVec2(p0.x + 2 * s, p0.y + 4 * s), ImVec2(p1.x + 2 * s, p1.y + 4 * s), IM_COL32(0, 0, 0, 70), 6 * s);
        const ImU32 cc = stSrgb(c, c.group == "transparent" ? 170 : 255);
        if (g_studTex) u.dl->AddImage(g_studTex, p0, p1, ImVec2(0, 0), ImVec2(1, 1), cc);
        else u.dl->AddRectFilled(p0, p1, cc, 6 * s);
        if (!small) {
        const std::string nm = stFit(u, 7.5f, stUpper(colorFr(c.name)), cw - 4 * s, true);
        const float lw = stTrackW(u, 7.5f, nm.c_str(), 1.2f * s, true);
        stTrack(u, ImVec2(cx - lw * 0.5f, b.y - pad - 15 * s), 7.5f, C_TEXT, nm.c_str(), 1.2f * s);
        }
        if (u.hover(ImVec2(x, a.y), ImVec2(x + cw, b.y)) && m_popup == 0) ImGui::SetTooltip("Couleur active : %s · %d", colorFr(c.name), c.id);
        x += cw + 7 * s;
        u.dl->AddLine(ImVec2(x, a.y + 16 * s), ImVec2(x, b.y - 16 * s), IM_COL32(255, 255, 255, 24), 1.0f);
        x += 7 * s;
    }
    struct B { const char* icon; const char* label; const char* tip; bool on; int id; };
    const B bs[2] = {{ST_PICK, "PIPETTE", "Pipette : clic sur une brique pour prendre sa couleur (4)", m_pipette, 3},
                     {ST_PAINT, "PINCEAU", "Pinceau : clic sur une brique pour la peindre (3)", m_paintTool, 2}};   // (Pipette left, Pinceau right: Wzx)
    for (const B& t : bs) {
        const ImVec2 ba(x, a.y + pad), bb(x + bw, a.y + pad + bh);
        const bool h = u.hover(ba, bb) && m_popup == 0;
        if (t.on) { u.dl->AddRectFilled(ba, bb, stA(C_ACCENT, 0.2f), 14 * s); u.dl->AddRect(ba, bb, stA(C_ACCENT, 0.85f), 14 * s, 0, 1.0f); }
        else { u.dl->AddRectFilled(ba, bb, IM_COL32(255, 255, 255, h ? 18 : 8), 14 * s); u.dl->AddRect(ba, bb, IM_COL32(255, 255, 255, h ? 60 : 26), 14 * s, 0, 1.0f); }
        const ImU32 col = t.on ? C_ACCENT_TXT : C_TEXT;
        stIcon(u.dl, t.icon, ImVec2((ba.x + bb.x) * 0.5f, small ? (ba.y + bb.y) * 0.5f : ba.y + 30 * s), (small ? 44 : 32) * s, col, (small ? 2.2f : 1.5f) * s);
        if (!small) {
            const float lw = stTrackW(u, 8, t.label, 1.6f * s, true);
            stTrack(u, ImVec2((ba.x + bb.x - lw) * 0.5f, bb.y - 19 * s), 8, col, t.label, 1.6f * s);
        }
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", t.tip);
        if (h && u.pressed) { if (t.id == 2) { m_paintTool = true; m_pipette = false; } else { m_pipette = true; m_paintTool = false; } }
        x += bw + 6 * s;
    }
    x += 6 * s;
    u.dl->AddLine(ImVec2(x, a.y + 16 * s), ImVec2(x, b.y - 16 * s), IM_COL32(255, 255, 255, 24), 1.0f);
    x += 8 * s;
    {   // Groupe entier: a switch
        const float cy = a.y + pad + 20 * s;
        if (!small) stTrack(u, ImVec2(x + 4 * s, cy - u.fs(7.5f) * 0.52f), 7.5f, C_DIM, "GROUPE ENTIER", 1.4f * s);
        else stTrack(u, ImVec2(x + 4 * s, a.y + pad - 1 * m_ui), 6.5f, C_DIM, "GROUPE", 1.0f * m_ui);
        const ImVec2 sa(x + 4 * s, small ? b.y - pad - 14 * m_ui : cy + 14 * s), sb(sa.x + (small ? 30 * m_ui : 34 * s), sa.y + (small ? 15 * m_ui : 18 * s));
        const ImVec2 ha(x, a.y), hb(b.x, b.y);
        const bool h = u.hover(ha, hb) && m_popup == 0;
        const float kr = (sb.y - sa.y) * 0.5f;
        u.dl->AddRectFilled(sa, sb, m_paintGroup ? stA(C_ACCENT, 0.85f) : IM_COL32(255, 255, 255, h ? 44 : 30), kr);
        u.dl->AddCircleFilled(ImVec2(m_paintGroup ? sb.x - kr : sa.x + kr, (sa.y + sb.y) * 0.5f), kr - 2 * m_ui, m_paintGroup ? C_INK : IM_COL32(220, 222, 226, 255), 20);
        if (!small) u.text(ImVec2(sb.x + 8 * s, (sa.y + sb.y) * 0.5f - u.fs(8.5f) * 0.55f), 8.5f, m_paintGroup ? C_ACCENT_TXT : C_FAINT, m_paintGroup ? "tout le groupe" : "la brique seule");
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Activé : un clic peint tout le groupe de la brique. Désactivé : la brique seule, même dans un groupe");
        if (h && u.pressed) { m_paintGroup = !m_paintGroup; savePrefs(); }
    }
}

// PEINDRE: every colour on an arc that hangs under the panel (unies, transparentes, métallisées), out one by one from the middle
// (the pop of the marking menu: 0.4 > 1.15 > 1), the one under the mouse grows; the current one ringed
void BrickApp::stPaintColors() {
    Ui u = ui(m_ui);
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const ImVec4 pr = m_uiRects.empty() ? ImVec4(0, 46 * s, d.x, 140 * s) : m_uiRects.back();   // the panel (drawn just before)
    const char* keys[3] = {"standard", "transparent", "metallic"};
    std::vector<int> order;   // the palette in its groups, with -1 between two groups
    for (int g = 0; g < 3; ++g) {
        bool any = false;
        for (int i = 0; i < (int)m_lib.palette.size(); ++i)
            if (m_lib.palette[i].group == keys[g]) { if (!any && !order.empty()) order.push_back(-1); any = true; order.push_back(i); }
    }
    if (order.empty()) return;
    if (m_paintT0 < 0) m_paintT0 = ImGui::GetTime();
    const float tIn = (float)(ImGui::GetTime() - m_paintT0), dt = ImGui::GetIO().DeltaTime;
    if (m_paintHov.size() != m_lib.palette.size()) m_paintHov.assign(m_lib.palette.size(), 0.0f);
    // the arc: a chord across the screen (its ends at the height of the panel), its lowest point under the panel
    const float vw = d.x - m_dockL - m_dockR, cxm = m_dockL + vw * 0.5f, half = std::max(200 * s, std::min(vw * 0.5f - 300 * s, 760 * s));   // (clear of the Couleur panel on the right)
    const float yEnd = pr.y + 56 * s, sag = std::max(160 * s, pr.w - yEnd + 70 * s);   // (the ends under the badge of the grid)
    const float R = (half * half + sag * sag) / (2 * sag), cy = yEnd + sag - R;   // the centre of the circle (above)
    const float th = std::asin(std::clamp(half / R, 0.0f, 1.0f));
    const float L = 2 * th * R;   // the length of the arc
    int nCol = 0, nGap = 0;
    for (int i : order) (i < 0 ? nGap : nCol)++;
    const float gapW = 0.9f;   // a group gap, in steps
    const float step = L / std::max(1.0f, (float)(nCol - 1) + nGap * gapW);
    struct P { int i; ImVec2 p; float e; };
    std::vector<P> ps;
    std::vector<float> gaps;   // the angles of the gaps between two groups (a small line: Wzx 09/11/2026)
    float t = 0;
    float sw = std::clamp(step - 4 * s, 12 * s, 26 * s);
    float pitchX = step;   // (the width of the hit box of a colour)
    if (m_ipad) {   // iPad (Wzx 10/11/2026): colours three times bigger, on two or three rings under one another
        sw = 34 * s;
        const float pitch = sw + 8 * s, ringGap = sw + 12 * s;
        pitchX = pitch;
        int k = 0, ring = 0, rank = 0;
        while (k < (int)order.size() && ring < 6) {
            const float Rr = R + ring * ringGap, Lr = 2 * th * Rr;
            const int cap = std::max(1, (int)(Lr / pitch) + 1);
            const int n = std::min(cap, (int)order.size() - k);
            const float st = Lr / std::max(1, cap - 1), t0 = (Lr - (n - 1) * st) * 0.5f;   // (a last ring not full: centred)
            for (int j = 0; j < n; ++j, ++k) {
                if (order[k] < 0) continue;   // (a group gap: an empty place)
                const float ang = -th + (t0 + j * st) / Rr;
                const ImVec2 p(cxm + std::sin(ang) * Rr, cy + std::cos(ang) * Rr);
                ps.push_back({order[k], p, std::clamp((tIn - 0.012f * rank++) / 0.28f, 0.0f, 1.0f)});
            }
            ++ring;
        }
    }
    for (int k = 0; k < (int)order.size() && !m_ipad; ++k) {
        if (order[k] < 0) { gaps.push_back(-th + (t + step * (gapW - 1) * 0.5f) / R); t += step * gapW; continue; }
        const float ang = -th + t / R;   // 0 = straight down
        const ImVec2 p(cxm + std::sin(ang) * R, cy + std::cos(ang) * R);
        const float rank = std::fabs(t - L * 0.5f) / std::max(1.0f, step);   // out from the middle
        ps.push_back({order[k], p, std::clamp((tIn - 0.012f * rank) / 0.28f, 0.0f, 1.0f)});
        t += step;
    }
    // the colour under the mouse: the nearest within its square
    int hot = -1;
    float best = 1e9f;
    if (m_popup == 0 && !(m_grip.id >= 0 && m_grip.moved))
        for (const P& q : ps) {
            const float dx = u.mouse.x - q.p.x, dy = u.mouse.y - q.p.y, dd = dx * dx + dy * dy;
            if (std::fabs(dx) < pitchX * 0.5f + 1 && std::fabs(dy) < sw * 0.8f && dd < best) { best = dd; hot = q.i; }
        }
    for (const P& q : ps) {
        float& hv = m_paintHov[q.i];
        hv += ((q.i == hot ? 1.0f : 0.0f) - hv) * std::min(1.0f, dt * 16);
        m_uiRects.push_back(ImVec4(q.p.x - pitchX * 0.5f, q.p.y - sw * 0.6f, q.p.x + pitchX * 0.5f, q.p.y + sw * 0.6f));
    }
    auto draw = [&](const P& q) {
        if (q.e <= 0) return;
        const float pop = q.e < 0.6f ? 0.4f + 0.75f * (q.e / 0.6f) : 1.15f - 0.15f * ((q.e - 0.6f) / 0.4f);
        const float hv = m_paintHov[q.i];
        const float hs = sw * 0.5f * pop * (1.0f + 0.6f * hv);
        // out from the panel: from 72 % of the way, like the radius of the marking menu
        const float k = 0.72f + 0.28f * std::min(1.0f, q.e / 0.6f);
        const ImVec2 o(cxm, (pr.y + pr.w) * 0.5f);
        const ImVec2 p(o.x + (q.p.x - o.x) * k, o.y + (q.p.y - o.y) * k - 4 * s * hv);
        const Color& c = m_lib.palette[q.i];
        const ImU32 cc = stSrgb(c, c.group == "transparent" ? 170 : 255);
        u.dl->AddRectFilled(ImVec2(p.x - hs + 2 * s, p.y - hs + 3 * s), ImVec2(p.x + hs + 2 * s, p.y + hs + 3 * s), IM_COL32(0, 0, 0, (int)(70 + 40 * hv)), 4 * s);
        if (g_studTex) u.dl->AddImage(g_studTex, ImVec2(p.x - hs, p.y - hs), ImVec2(p.x + hs, p.y + hs), ImVec2(0, 0), ImVec2(1, 1), cc);
        else u.dl->AddRectFilled(ImVec2(p.x - hs, p.y - hs), ImVec2(p.x + hs, p.y + hs), cc, 4 * s);
        if (q.i == m_color) u.dl->AddRect(ImVec2(p.x - hs - 3 * s, p.y - hs - 3 * s), ImVec2(p.x + hs + 3 * s, p.y + hs + 3 * s), C_ACCENT, 5 * s, 0, 1.5f * s);
    };
    for (float ga : gaps) {   // between unies, transparentes, métallisées: a small line across the arc
        const float a = std::clamp((tIn - 0.15f) / 0.25f, 0.0f, 1.0f);
        const ImVec2 p(cxm + std::sin(ga) * R, cy + std::cos(ga) * R), n(std::sin(ga), std::cos(ga));
        const float h = sw * 0.75f;
        u.dl->AddLine(ImVec2(p.x - n.x * h, p.y - n.y * h), ImVec2(p.x + n.x * h, p.y + n.y * h), IM_COL32(255, 255, 255, (int)(70 * a)), 1.5f * s);
    }
    for (const P& q : ps) if (m_paintHov[q.i] < 0.01f) draw(q);
    for (const P& q : ps) if (m_paintHov[q.i] >= 0.01f && q.i != hot) draw(q);
    for (const P& q : ps) if (q.i == hot) draw(q);   // (the hovered one on top)
    if (hot >= 0) {
        const Color& c = m_lib.palette[hot];
        ImGui::SetTooltip("%s · %d", colorFr(c.name), c.id);
        if (u.pressed) {   // the colour of the Pinceau (the selection is not painted: the next click paints)
            m_color = hot;
            m_recentColors.erase(std::remove(m_recentColors.begin(), m_recentColors.end(), hot), m_recentColors.end());
            m_recentColors.insert(m_recentColors.begin(), hot);
            if (m_recentColors.size() > 12) m_recentColors.resize(12);
            m_paintTool = true;
            m_pipette = false;
        }
    }
}

// the brick alone painted (Peindre, Groupe entier off): its group stays as it is
// PEINDRE: what confirms a gesture (Wzx 09/11/2026). The Pipette: a ring closes on the brick, a drop of its colour flies
// to the active colour (which pops). The Pinceau: a splash, a ring and drops; held, the circle of the brush
void BrickApp::stPaintFx() {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float s = m_ui;
    const double now = ImGui::GetTime();
    auto colU = [&](int i, float a) {
        if (i < 0 || i >= (int)m_lib.palette.size()) return IM_COL32(255, 255, 255, (int)(255 * a));
        const Color& c = m_lib.palette[i];
        return IM_COL32((int)(std::clamp(c.srgb.x, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.y, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.z, 0.0f, 1.0f) * 255), (int)(255 * std::clamp(a, 0.0f, 1.0f)));
    };
    for (size_t n = 0; n < m_paintFx.size(); ++n) {
        const PaintFx& f = m_paintFx[n];
        const float t = (float)(now - f.t0);
        if (f.kind == 0) {   // Pipette
            const float e1 = std::clamp(t / 0.22f, 0.0f, 1.0f);
            if (e1 < 1) {   // a ring that closes on the brick, the colour inside
                const float r = (30 - 22 * e1 * e1) * s;
                dl->AddCircleFilled(f.p, r, colU(f.color, 0.35f * (1 - e1)), 32);
                dl->AddCircle(f.p, r, IM_COL32(255, 255, 255, (int)(230 * (1 - e1 * 0.5f))), 32, 2.0f * s);
            }
            const float e2 = std::clamp((t - 0.18f) / 0.42f, 0.0f, 1.0f);
            if (t >= 0.18f && e2 < 1) {   // the drop flies to the active colour (or up, out of the mode Peindre)
                const ImVec2 to = m_paintSwatchAt.x >= 0 ? m_paintSwatchAt : ImVec2(f.p.x, f.p.y - 60 * s);
                const float k = e2 < 0.5f ? 2 * e2 * e2 : 1 - 2 * (1 - e2) * (1 - e2);   // ease in-out
                const ImVec2 mid((f.p.x + to.x) * 0.5f, std::min(f.p.y, to.y) - 90 * s);
                const float u = 1 - k;
                const ImVec2 q(u * u * f.p.x + 2 * u * k * mid.x + k * k * to.x, u * u * f.p.y + 2 * u * k * mid.y + k * k * to.y);
                const float r = (9 - 3 * k) * s, a = m_paintSwatchAt.x >= 0 ? 1.0f : 1 - k;
                dl->AddCircleFilled(ImVec2(q.x + 1.5f * s, q.y + 2.5f * s), r, IM_COL32(0, 0, 0, (int)(80 * a)), 24);
                dl->AddCircleFilled(q, r, colU(f.color, a), 24);
                dl->AddCircle(q, r, IM_COL32(255, 255, 255, (int)(220 * a)), 24, 1.5f * s);
                dl->AddCircleFilled(ImVec2(q.x - r * 0.35f, q.y - r * 0.35f), r * 0.25f, IM_COL32(255, 255, 255, (int)(150 * a)), 12);
            }
        } else {   // Pinceau: a splash
            const float e = std::clamp(t / 0.45f, 0.0f, 1.0f), o = 1 - (1 - e) * (1 - e);
            const float big = f.kind == 1 ? 1.0f : 0.6f;
            dl->AddCircle(f.p, (6 + 30 * o) * s * big, colU(f.color, 0.9f * (1 - e)), 32, (3.0f - 2.0f * e) * s);
            dl->AddCircle(f.p, (6 + 30 * o) * s * big + 1.5f * s, IM_COL32(255, 255, 255, (int)(110 * (1 - e))), 32, 1.0f * s);
            const uint32_t seed = (uint32_t)(f.p.x * 13 + f.p.y * 7);
            for (int k = 0; k < 7; ++k) {   // drops thrown out
                const float ang = (k / 7.0f + ((seed >> (k * 3)) & 7) / 56.0f) * 6.2831853f;
                const float d = (8 + 26 * o * (0.7f + 0.3f * (((seed >> k) & 3) / 3.0f))) * s * big;
                const ImVec2 q(f.p.x + std::cos(ang) * d, f.p.y + std::sin(ang) * d + 10 * s * e * e);
                dl->AddCircleFilled(q, (3.2f * (1 - e) + 0.6f) * s * big, colU(f.color, 1 - e), 12);
            }
        }
    }
    m_paintFx.erase(std::remove_if(m_paintFx.begin(), m_paintFx.end(), [&](const PaintFx& f) { return now - f.t0 > 0.7; }), m_paintFx.end());
    if (m_paintStroke && m_paintTool) {   // the brush held: its circle (grows while the press waits)
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const float g = m_paintCircle ? 1.0f : std::clamp((float)(now - m_paintStrokeT0) / 0.18f, 0.0f, 1.0f);
        const float r = m_paintRadius * s * g;
        if (r > 2) {
            dl->AddCircleFilled(mp, r, colU(m_color, m_paintCircle ? 0.18f : 0.1f), 48);
            dl->AddCircle(mp, r, colU(m_color, 0.95f), 48, 2.0f * s);
            dl->AddCircle(mp, r + 1.5f * s, IM_COL32(255, 255, 255, 120), 48, 1.0f * s);
        }
    }
}

// the Pinceau on a brick (its whole group with « Groupe entier »): the colour, once per stroke, one undo for the stroke
bool BrickApp::paintHit(int b, ImVec2 at, bool fx) {
    if (b < 0 || b >= (int)m_bricks.size() || std::find(m_paintDone.begin(), m_paintDone.end(), b) != m_paintDone.end()) return false;
    const std::vector<int> unit = m_paintGroup ? unitOf(b) : std::vector<int>{b};
    bool changed = false;
    for (int i : unit) {
        if (std::find(m_paintDone.begin(), m_paintDone.end(), i) == m_paintDone.end()) m_paintDone.push_back(i);
        if (m_bricks[i].color != m_color) {
            if (!m_paintUndo) { pushUndo(); m_paintUndo = true; }
            m_bricks[i].color = m_color;
            changed = true;
        }
    }
    if (fx && m_paintFx.size() < 40) m_paintFx.push_back({at, m_color, ImGui::GetTime(), m_paintCircle ? 2 : 1});
    if (changed) {
        m_recentColors.erase(std::remove(m_recentColors.begin(), m_recentColors.end(), m_color), m_recentColors.end());
        m_recentColors.insert(m_recentColors.begin(), m_color);
        if (m_recentColors.size() > 12) m_recentColors.resize(12);
    }
    return changed;
}

// mode DEV: the colour of the source of the connectors (Wzx 10/11/2026): sky blue LDCad, apple green validated by
// hand, orange automatic (rules or the old BrickIt), red none
ImU32 stConnCol(int src) {
    switch (src) {
    case 1: return IM_COL32(0x5a, 0xc8, 0xfa, 255);
    case 2: return IM_COL32(0xff, 0x95, 0x00, 255);
    case 3: return IM_COL32(0xff, 0x3b, 0x30, 255);
    case 4: return IM_COL32(0x8e, 0xe0, 0x00, 255);
    default: return IM_COL32(0x80, 0x80, 0x80, 255);
    }
}
const char* stConnName(int src) {
    switch (src) {
    case 1: return "Shadow Library (LDCad)";
    case 2: return "Auto (sans Shadow Library)";
    case 3: return "Échec, aucun connecteur";
    case 4: return "Validé à la main";
    default: return "Inconnu";
    }
}

void BrickApp::stConnLegend() {
    if (!m_dev || !m_studioUi) return;
    if (m_connCountsDirty) {
        for (int& c : m_connCounts) c = 0;
        for (const Part& p : m_lib.parts) {
            std::string t = p.label;
            for (char& c : t) c = (char)std::tolower((unsigned char)c);
            if (t.find("pattern") != std::string::npos || t.find("sticker") != std::string::npos) continue;   // (no stickers)
            ++m_connCounts[std::clamp((int)p.connSrc, 0, 4)];
        }
        m_connCountsDirty = false;
    }
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const float W = 236 * s, rowH = 19 * s, pad = 12 * s;
    const int order[4] = {1, 4, 2, 3};
    const float H = pad + 16 * s + 4 * rowH + 18 * s + pad;
    const float bottom = (m_uiRects.size() > 1 ? m_uiRects[1].y : ImGui::GetIO().DisplaySize.y) - 12 * s;
    float x0 = 14 * s + m_dockL;   // (right of the panel of the mode DEV when it is open there)
    for (size_t k = 2; k < m_uiRects.size(); ++k) if (m_uiRects[k].x < 40 * s && m_uiRects[k].w > bottom - H) x0 = std::max(x0, m_uiRects[k].z + 12 * s);
    const ImVec2 a(x0, bottom - H), b(a.x + W, bottom);
    stGlass(u.dl, a, b, 14 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    stTrack(u, ImVec2(a.x + pad, a.y + pad), 7.5f, C_DIM, "DEV · CONNECTEURS", 1.4f * s);
    float y = a.y + pad + 16 * s;
    for (int k : order) {
        u.dl->AddRect(ImVec2(a.x + pad, y + 3 * s), ImVec2(a.x + pad + 16 * s, y + 15 * s), stConnCol(k), 4 * s, 0, 2.0f * s);
        u.text(ImVec2(a.x + pad + 24 * s, y + 9 * s - u.fs(9) * 0.55f), 9, C_TEXT, stConnName(k));
        char n[16];
        snprintf(n, sizeof(n), "%d", m_connCounts[k]);
        u.text(ImVec2(b.x - pad - u.textW(9, n), y + 9 * s - u.fs(9) * 0.55f), 9, C_DIM, n);
        y += rowH;
    }
    u.text(ImVec2(a.x + pad, y + 4 * s), 8, C_FAINT, "clic droit sur une carte : changer");
}

void BrickApp::paintOne(int brick) {
    if (brick < 0 || brick >= (int)m_bricks.size()) return;
    m_recentColors.erase(std::remove(m_recentColors.begin(), m_recentColors.end(), m_color), m_recentColors.end());
    m_recentColors.insert(m_recentColors.begin(), m_color);
    if (m_recentColors.size() > 12) m_recentColors.resize(12);
    if (m_bricks[brick].color == m_color) return;
    pushUndo();
    m_bricks[brick].color = m_color;
    syncObjects();
    autosave();
}

// ------------------------------------------------------------------ the scene menu: Nouvelle, Ouvrir, Enregistrer

void BrickApp::stFile() {
    if (m_popup != 8) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    struct It { const char* label; const char* key; int id; };
    const It its[4] = {{"NOUVELLE SCÈNE", "Ctrl+N", 0}, {"OUVRIR…", "Ctrl+O", 1}, {"ENREGISTRER", "Ctrl+S", 2}, {"ENREGISTRER SOUS…", "Ctrl+Maj+S", 3}};
    const float W = 260 * s, ih = 36 * s, pad = 6 * s;
    const ImVec2 a(m_popupAt.x, m_popupAt.y), b(a.x + W, a.y + pad * 2 + 4 * ih + 22 * s);
    stGlass(u.dl, a, b, 14 * s, s);
    u.dl->AddRectFilled(a, b, IM_COL32(16, 17, 19, 228), 14 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    bool clicked = false;
    for (const It& it : its) {
        const ImVec2 ia(a.x + pad, y), ib(b.x - pad, y + ih);
        const bool h = u.hover(ia, ib);
        if (h) u.dl->AddRectFilled(ia, ib, IM_COL32(255, 255, 255, 16), 10 * s);
        stTrack(u, ImVec2(ia.x + 12 * s, (ia.y + ib.y) * 0.5f - u.fs(9) * 0.52f), 9, h ? C_TEXT : IM_COL32(214, 216, 220, 255), it.label, 1.6f * s);
        const float kw = u.textW(9, it.key);
        u.text(ImVec2(ib.x - 12 * s - kw, (ia.y + ib.y) * 0.5f - u.fs(9) * 0.55f), 9, C_FAINT, it.key);
        if (h && u.pressed) {
            clicked = true;
            m_popup = 0;
            if (it.id == 0) newScene();
            else if (it.id == 1) openDialog();
            else if (it.id == 2) saveNow();
            else saveDialog();
        }
        y += ih;
    }
    {   // the file of the scene
        std::string f = m_scenePath.empty() ? std::string("pas encore enregistrée") : std::string(m_scenePath.begin(), m_scenePath.end());
        if (!m_scenePath.empty()) { const size_t k = f.find_last_of("\\/"); if (k != std::string::npos) f = f.substr(k + 1); }
        u.dl->AddLine(ImVec2(a.x + 12 * s, y + 4 * s), ImVec2(b.x - 12 * s, y + 4 * s), IM_COL32(255, 255, 255, 20), 1.0f);
        u.text(ImVec2(a.x + 18 * s, y + 8 * s), 8.5f, C_FAINT, f.c_str());
    }
    if (u.pressed && !clicked && !u.hover(a, b) && !u.hover(ImVec2(0, 0), ImVec2(330 * s, 46 * s))) m_popup = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_popup = 0;
}

void BrickApp::newScene() {
    pushUndo();
    m_bricks.clear();
    m_selected = -1;
    m_multi.clear();
    m_scenePath.clear();
    m_modelName.clear();
    rebuildScene();
    toast("Nouvelle scène");
}

// Enregistrer: into its file when it has one, else the dialog
void BrickApp::saveNow() {
    if (m_scenePath.empty() || m_ipad) { saveDialog(); return; }
    saveScene(m_scenePath);
}

// ------------------------------------------------------------------ Groupes (Studio): the outliner

// a text cut to a width, with « … »
static std::string stFit(Ui& u, float px, std::string t, float maxW, bool bold) {
    if (u.textW(px, t.c_str(), bold) <= maxW) return t;
    while (!t.empty() && u.textW(px, (t + "…").c_str(), bold) > maxW) {
        t.pop_back();
        while (!t.empty() && ((unsigned char)t.back() & 0xC0) == 0x80) t.pop_back();
    }
    return t + "…";
}

void BrickApp::stGroups() {
    if (!m_pGroups.visible || m_mechOn) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const int G = (int)m_build.groups.size();
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
    // the prefix most groups share (« 31153 - modern house - »): shown once under the title, cut from the names; a
    // sub-group drops the name of its parent (« foyer - balcony » under « foyer »: « balcony »)
    std::string common;
    {
        std::vector<const std::string*> tops;
        for (int g = 0; g < G; ++g) if (parent[g] == -1 && count[g] > 0) tops.push_back(&m_build.groups[g].name);
        if (tops.size() >= 2) {
            const std::string& f = *tops[0];
            for (size_t p = f.find(" - "); p != std::string::npos; p = f.find(" - ", p + 3)) {
                const std::string pre = f.substr(0, p + 3);
                int n = 0;
                for (const std::string* t : tops) n += t->rfind(pre, 0) == 0 ? 1 : 0;
                if (n * 2 >= (int)tops.size()) common = pre;
            }
        }
    }
    auto shortName = [&](int g) {
        std::string n = m_build.groups[g].name;
        const int p = parent[g];
        if (p >= 0) {
            const std::string& pn = m_build.groups[p].name;
            if (n.size() > pn.size() + 3 && n.rfind(pn + " - ", 0) == 0) return n.substr(pn.size() + 3);
        }
        if (!common.empty() && n.size() > common.size() && n.rfind(common, 0) == 0) return n.substr(common.size());
        return n;
    };
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
    const float top = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 12 * s;
    const float drawerTop = m_uiRects.size() > 1 ? m_uiRects[1].y : d.y;
    const float W = 320 * s, pad = 12 * s, rowH = 30 * s, headH = common.empty() ? 30 * s : 46 * s, footH = 30 * s;
    const float maxH = std::max(160 * s, drawerTop - 12 * s - top);
    const float listH = std::min(std::max(1, (int)rows.size()) * rowH, maxH - pad - headH - footH - pad);
    const ImVec2 a((m_paintMode ? 14 : 74) * s + m_dockL, top + (m_paintMode ? 52 * s : 0)), b(a.x + W, a.y + pad + headH + listH + footH + pad);
    stGlass(u.dl, a, b, 18 * s, s);
    u.dl->AddRectFilled(a, b, IM_COL32(14, 15, 17, 120), 18 * s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    // the title: GROUPES, how many; the shared prefix; Tout afficher while one is isolated; the close
    {
        stTrack(u, ImVec2(a.x + pad + 2 * s, a.y + pad + 2 * s), 9.5f, C_TEXT, "GROUPES", 2.0f * s);
        char n[16];
        snprintf(n, sizeof(n), "%d", (int)rows.size());
        u.text(ImVec2(a.x + pad + 2 * s + stTrackW(u, 9.5f, "GROUPES", 2.0f * s, true) + 8 * s, a.y + pad + 2 * s), 9, C_FAINT, n);
        if (!common.empty()) {
            std::string c = common.substr(0, common.size() - 3);
            c = stFit(u, 9, c, W - 2 * pad - 30 * s, false);
            u.text(ImVec2(a.x + pad + 2 * s, a.y + pad + 22 * s), 9, C_DIM, c.c_str());
        }
        const ImVec2 xc(b.x - pad - 8 * s, a.y + pad + 8 * s);
        const bool hx = u.hover(ImVec2(xc.x - 10 * s, xc.y - 10 * s), ImVec2(xc.x + 10 * s, xc.y + 10 * s));
        stIcon(u.dl, ST_CLOSE, xc, 13 * s, hx ? C_TEXT : C_DIM, 1.3f * s);
        if (hx && u.pressed) { m_pGroups.visible = false; savePrefs(); return; }
        if (m_isolateGroup >= 0) {
            const char* lab = "TOUT AFFICHER";
            const float tw = stTrackW(u, 7.5f, lab, 1.4f * s, true);
            const ImVec2 ba(xc.x - 18 * s - tw - 16 * s, a.y + pad - 1 * s), bb(xc.x - 18 * s, a.y + pad + 19 * s);
            const bool h = u.hover(ba, bb);
            u.dl->AddRectFilled(ba, bb, stA(C_ACCENT, h ? 0.3f : 0.18f), 10 * s);
            u.dl->AddRect(ba, bb, stA(C_ACCENT, 0.8f), 10 * s, 0, 1.0f);
            stTrack(u, ImVec2(ba.x + 8 * s, (ba.y + bb.y) * 0.5f - u.fs(7.5f) * 0.52f), 7.5f, C_ACCENT_TXT, lab, 1.4f * s);
            if (h && u.pressed) { m_isolateGroup = -1; syncObjects(); }
        }
    }
    // the list (the wheel scrolls it)
    const ImVec2 la(a.x + 6 * s, a.y + pad + headH), lb(b.x - 6 * s, la.y + listH);
    const float contentH = rows.size() * rowH;
    if (u.hover(la, lb) && ImGui::GetIO().MouseWheel != 0) m_groupsScroll -= ImGui::GetIO().MouseWheel * rowH * 3;
    m_groupsScroll = std::clamp(m_groupsScroll, 0.0f, std::max(0.0f, contentH - listH));
    u.dl->PushClipRect(la, lb, true);
    if (rows.empty()) u.text(ImVec2(la.x + 8 * s, la.y + 6 * s), 10, C_DIM, "Aucun groupe : sélectionne des pièces puis Ctrl+G");
    const int sel = unitGroup(m_selected);
    for (size_t r = 0; r < rows.size(); ++r) {
        const float y = la.y + r * rowH - m_groupsScroll;
        if (y + rowH < la.y || y > lb.y) continue;
        const int g = rows[r].g;
        Group& gr = m_build.groups[g];
        const ImVec2 ra(la.x, y + 1 * s), rb(lb.x, y + rowH - 1 * s);
        const bool h = u.hover(ra, rb) && u.hover(la, lb);
        const bool on = g == sel;
        if (on) { u.dl->AddRectFilled(ra, rb, stA(C_ACCENT, 0.2f), 10 * s); u.dl->AddRect(ra, rb, stA(C_ACCENT, 0.85f), 10 * s, 0, 1.0f); }
        else if (h) u.dl->AddRectFilled(ra, rb, IM_COL32(255, 255, 255, 14), 10 * s);
        const float cy = (ra.y + rb.y) * 0.5f;
        float x = ra.x + 8 * s + rows[r].depth * 16 * s;
        for (int dpt = 0; dpt < rows[r].depth; ++dpt)   // the guides of the tree
            u.dl->AddLine(ImVec2(ra.x + 15 * s + dpt * 16 * s, ra.y + 2 * s), ImVec2(ra.x + 15 * s + dpt * 16 * s, rb.y - 2 * s), IM_COL32(255, 255, 255, 22), 1.0f);
        bool hasKids = false;
        for (int k = 0; k < G; ++k) if (parent[k] == g && count[k] > 0) hasKids = true;
        const bool open = std::find(m_groupsOpen.begin(), m_groupsOpen.end(), g) != m_groupsOpen.end();
        const ImVec2 ca(x, cy - 8 * s), cb(x + 14 * s, cy + 8 * s);
        if (hasKids) {
            const ImVec2 c((ca.x + cb.x) * 0.5f, cy);
            const float k = 3.2f * s;
            const ImU32 cc = on ? C_ACCENT_TXT : C_DIM;
            if (open) u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - k, c.y - k * 0.5f), ImVec2(c.x, c.y + k * 0.5f), ImVec2(c.x + k, c.y - k * 0.5f)}.data(), 3, cc, 0, 1.5f * s);
            else u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - k * 0.5f, c.y - k), ImVec2(c.x + k * 0.5f, c.y), ImVec2(c.x - k * 0.5f, c.y + k)}.data(), 3, cc, 0, 1.5f * s);
        }
        const bool overChev = hasKids && u.hover(ca, cb);
        x += 18 * s;
        // right: the count (its own column), the eye, isolate
        const float bx2 = rb.x - 8 * s;
        const ImVec2 ia(bx2 - 20 * s, cy - 10 * s), ib(bx2, cy + 10 * s);
        const ImVec2 ea(ia.x - 26 * s, cy - 10 * s), eb(ia.x - 6 * s, cy + 10 * s);
        char cn[16];
        snprintf(cn, sizeof(cn), "%d", count[g]);
        const float cw = u.textW(9.5f, cn), cx = ea.x - 8 * s - cw;
        u.text(ImVec2(cx, cy - u.fs(9.5f) * 0.55f), 9.5f, on ? C_ACCENT_TXT : C_FAINT, cn);
        const std::string nm = stFit(u, 11, shortName(g), ea.x - 44 * s - x, on);
        u.text(ImVec2(x, cy - u.fs(11) * 0.55f), 11, gr.hidden ? C_FAINT : (on ? C_ACCENT_TXT : C_TEXT), nm.c_str(), on);
        const bool he = u.hover(ea, eb), hi = u.hover(ia, ib);
        {   // eye: shown / hidden
            const ImVec2 c((ea.x + eb.x) * 0.5f, cy);
            const ImU32 col = he ? C_TEXT : (on ? C_ACCENT_TXT : C_DIM);
            u.dl->AddEllipse(c, ImVec2(7 * s, 4 * s), col, 0, 0, 1.2f * s);
            if (!gr.hidden) u.dl->AddCircleFilled(c, 2.0f * s, col);
            else u.dl->AddLine(ImVec2(c.x - 7 * s, c.y + 5 * s), ImVec2(c.x + 7 * s, c.y - 5 * s), col, 1.2f * s);
        }
        {   // isolate: a target, filled when this group is the one isolated
            const ImVec2 c((ia.x + ib.x) * 0.5f, cy);
            const ImU32 col = m_isolateGroup == g ? C_ACCENT_TXT : (hi ? C_TEXT : (on ? C_ACCENT_TXT : C_DIM));
            u.dl->AddCircle(c, 5.5f * s, col, 0, 1.2f * s);
            if (m_isolateGroup == g) u.dl->AddCircleFilled(c, 2.5f * s, col);
        }
        if (h && (he || hi)) ImGui::SetTooltip("%s", he ? (gr.hidden ? "Afficher" : "Masquer") : "Isoler ce groupe");
        else if (h && nm != gr.name && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", gr.name.c_str());
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
            ImGui::SetNextWindowPos(ImVec2(x - 4 * s, ra.y + 2 * s));
            ImGui::SetNextWindowSize(ImVec2(ea.x - 40 * s - x, rowH - 6 * s));
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
    }
    u.dl->PopClipRect();
    if (contentH > listH) {   // a thin scroll bar
        const float bh = listH * listH / contentH, by = la.y + (listH - bh) * (m_groupsScroll / (contentH - listH));
        u.dl->AddRectFilled(ImVec2(b.x - 5 * s, by), ImVec2(b.x - 3 * s, by + bh), IM_COL32(255, 255, 255, 50), 1 * s);
    }
    // the foot: the loose pieces, the shortcuts
    u.dl->AddLine(ImVec2(a.x + pad, lb.y + 6 * s), ImVec2(b.x - pad, lb.y + 6 * s), IM_COL32(255, 255, 255, 18), 1.0f);
    char f[96];
    snprintf(f, sizeof(f), "%d pièces hors groupe   ·   Ctrl+G grouper   ·   Ctrl+Maj+G dégrouper", loose);
    const std::string fs = stFit(u, 8.5f, f, W - 2 * pad, false);
    u.text(ImVec2(a.x + pad + 2 * s, lb.y + 12 * s), 8.5f, C_FAINT, fs.c_str());
}

// ------------------------------------------------------------------ Couleur (right)

void BrickApp::stColors() {
    if (!m_pColors.visible || m_mechOn) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float top = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 12 * s, bottom = (m_uiRects.size() > 1 ? m_uiRects[1].y : d.y) - 12 * s;
    const float W = 214 * s, pad = 12 * s, cols = 6, gap = 5 * s;
    const float sw = std::floor((W - 2 * pad - (cols - 1) * gap) / cols);
    const ImVec2 a(d.x - 14 * s - m_dockR - W, top), b(d.x - 14 * s - m_dockR, bottom);
    stGlass(u.dl, a, b, 16 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    // vertical ribs, as in the trans-clear brick (very faint)
    for (float x = a.x + 48 * s; x < b.x - 8 * s; x += 48 * s) u.dl->AddLine(ImVec2(x, a.y + 8 * s), ImVec2(x, b.y - 8 * s), IM_COL32(255, 255, 255, 6), 1.0f);
    const Color& cur = m_lib.palette[std::clamp(m_color, 0, (int)m_lib.palette.size() - 1)];
    float y = a.y + pad;
    stTrack(u, ImVec2(a.x + pad, y), 10, C_TEXT, "COULEUR", 2.2f * s);
    {
        char b2[32];
        snprintf(b2, sizeof(b2), "LDraw %d", cur.id);
        const float tw = u.textW(9.5f, b2);
        u.text(ImVec2(b.x - pad - tw, y + 1 * s), 9.5f, C_DIM, b2);
    }
    y += 22 * s;
    auto colOf = [](const Color& c, int alpha = 255) { return IM_COL32((int)(std::clamp(c.srgb.x, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.y, 0.0f, 1.0f) * 255), (int)(std::clamp(c.srgb.z, 0.0f, 1.0f) * 255), alpha); };
    auto swatch = [&](ImVec2 p0, float size, const Color& c) {
        const bool tr = c.group == "transparent";
        if (g_studTex) u.dl->AddImage(g_studTex, p0, ImVec2(p0.x + size, p0.y + size), ImVec2(0, 0), ImVec2(1, 1), colOf(c, tr ? 165 : 255));
        else u.dl->AddRectFilled(p0, ImVec2(p0.x + size, p0.y + size), colOf(c, tr ? 165 : 255), 3 * s);
        if (c.group == "metallic") u.dl->AddRectFilledMultiColor(ImVec2(p0.x + size * 0.08f, p0.y + size * 0.08f), ImVec2(p0.x + size * 0.8f, p0.y + size * 0.45f), IM_COL32(255, 255, 255, 60), IM_COL32(255, 255, 255, 60), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    };
    {   // the current colour: a big brick, its name and hex
        const float bs = 44 * s;
        swatch(ImVec2(a.x + pad, y), bs, cur);
        u.text(ImVec2(a.x + pad + bs + 10 * s, y + 6 * s), 13, C_TEXT, colorFr(cur.name), true);
        char hx[16];
        snprintf(hx, sizeof(hx), "#%02X%02X%02X", (int)(cur.srgb.x * 255), (int)(cur.srgb.y * 255), (int)(cur.srgb.z * 255));
        u.text(ImVec2(a.x + pad + bs + 10 * s, y + 24 * s), 9.5f, C_DIM, hx);
        y += bs + 12 * s;
    }
    // the filter of the real colours of the part (Rebrickable): « Toutes les couleurs »
    const int fbp = focusBrick();
    const int fpart = fbp >= 0 && fbp < (int)m_bricks.size() ? m_bricks[fbp].part : -1;
    const bool filter = !m_allColors && fpart >= 0 && m_lib.hasColorTable(fpart);
    {
        const ImVec2 ca(a.x + pad, y + 1 * s), cb(ca.x + 12 * s, ca.y + 12 * s);
        const bool h = u.hover(ImVec2(a.x, y), ImVec2(b.x, y + 15 * s));
        if (m_allColors) u.dl->AddRectFilled(ca, cb, C_ACCENT, 3 * s);
        else u.dl->AddRect(ca, cb, h ? C_TEXT : C_FAINT, 3 * s, 0, 1.0f);
        u.text(ImVec2(cb.x + 6 * s, y), 9.5f, h ? C_TEXT : C_DIM, "Toutes les couleurs");
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Non coché : seules les couleurs qui existent pour cette pièce");
        if (h && u.pressed) m_allColors = !m_allColors;
        y += 22 * s;
    }
    u.dl->PushClipRect(ImVec2(a.x, y), ImVec2(b.x, b.y - 6 * s), true);
    auto group = [&](const char* title, const char* key) {
        stTrack(u, ImVec2(a.x + pad, y), 8.5f, C_DIM, title, 1.8f * s);
        u.dl->AddLine(ImVec2(a.x + pad + stTrackW(u, 8.5f, title, 1.8f * s, true) + 8 * s, y + 7 * s), ImVec2(b.x - pad, y + 7 * s), IM_COL32(255, 255, 255, 22), 1.0f);
        y += 17 * s;
        int k = 0;
        for (int i = 0; i < (int)m_lib.palette.size(); ++i) {
            const Color& c = m_lib.palette[i];
            if (c.group != key) continue;
            const ImVec2 p0(a.x + pad + (k % (int)cols) * (sw + gap), y + (k / (int)cols) * (sw + gap)), p1(p0.x + sw, p0.y + sw);
            swatch(p0, sw, c);
            const bool exists = !filter || m_lib.colorExists(fpart, i);
            if (!exists) {
                u.dl->AddRectFilled(p0, p1, IM_COL32(16, 17, 19, 170), 3 * s);
                u.dl->AddLine(ImVec2(p0.x + 4 * s, p1.y - 4 * s), ImVec2(p1.x - 4 * s, p0.y + 4 * s), IM_COL32(255, 255, 255, 60), 1.0f);
            }
            if (i == m_color) m_colCurAt = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
            if (i == m_color) u.dl->AddRect(ImVec2(p0.x - 2.5f * s, p0.y - 2.5f * s), ImVec2(p1.x + 2.5f * s, p1.y + 2.5f * s), C_ACCENT, 4 * s, 0, 1.5f * s);
            if (u.hover(p0, p1)) {
                if (m_colPress < 0) ImGui::SetTooltip(exists ? "%s · %d" : "%s · %d : n'existe pas pour cette pièce", colorFr(c.name), c.id);
                if (u.pressed) { m_colPress = i; m_colPressPos = u.mouse; }   // (a click applies on release; a drag out takes a 1x1)
            }
            ++k;
        }
        y += ((k + (int)cols - 1) / (int)cols) * (sw + gap) + 8 * s;
    };
    group("UNIES", "standard");
    group("TRANSPARENTES", "transparent");
    group("MÉTALLISÉES", "metallic");
    u.dl->PopClipRect();
    // a swatch pressed: released in place it applies the colour (to the selection); dragged out of the panel, a brick
    // 1x1 of that colour comes into the hand with a little burst (Wzx 10/11/2026)
    if (m_colPress >= 0 && m_colPress < (int)m_lib.palette.size()) {
        const ImVec2 dm(u.mouse.x - m_colPressPos.x, u.mouse.y - m_colPressPos.y);
        const bool moved = dm.x * dm.x + dm.y * dm.y > 64 * s * s;
        const bool inside = u.mouse.x >= a.x && u.mouse.x <= b.x && u.mouse.y >= a.y && u.mouse.y <= b.y;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!moved && (!filter || m_lib.colorExists(fpart, m_colPress))) applyColor(m_colPress);
            m_colPress = -1;
        } else if (moved && inside) {   // the swatch follows the finger, a little bigger, wobbling
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            const float t = (float)ImGui::GetTime();
            const float z = 34 * s * (1 + 0.06f * std::sin(t * 18));
            const ImVec2 c(u.mouse.x, u.mouse.y - 30 * s);
            fg->AddCircleFilled(ImVec2(c.x + 2 * s, c.y + 4 * s), z * 0.62f, IM_COL32(0, 0, 0, 70), 24);
            ImDrawList* keep = u.dl;
            u.dl = fg;
            swatch(ImVec2(c.x - z * 0.5f, c.y - z * 0.5f), z, m_lib.palette[m_colPress]);
            u.dl = keep;
            fg->AddRect(ImVec2(c.x - z * 0.5f - 2 * s, c.y - z * 0.5f - 2 * s), ImVec2(c.x + z * 0.5f + 2 * s, c.y + z * 0.5f + 2 * s), IM_COL32(255, 255, 255, 200), 5 * s, 0, 1.5f * s);
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        } else if (moved && !inside) {
            int part = m_lib.find("3005");
            if (part < 0) part = m_lib.find("3005.dat");
            const int col = m_colPress;
            m_colPress = -1;
            if (part >= 0) {
                m_color = col;
                m_recentColors.erase(std::remove(m_recentColors.begin(), m_recentColors.end(), col), m_recentColors.end());
                m_recentColors.insert(m_recentColors.begin(), col);
                if (m_recentColors.size() > 12) m_recentColors.resize(12);
                spawn(part, u.mouse.x, u.mouse.y);
                m_dragStarted = true;
                m_pressed = false;
                if (m_holding) {   // the burst: it springs from higher, squashed, with a spin
                    m_held.vpos = m_held.target + float3(0, 2.4f, 0);
                    m_held.squash = 0.35f;
                    m_held.vrot = qMul(qAxisAngle(float3(0, 1, 0), 1.6f), m_held.rot);
                }
                const double now = ImGui::GetTime();
                m_paintFx.push_back({u.mouse, col, now, 1});
                m_paintFx.push_back({ImVec2(u.mouse.x + 10 * s, u.mouse.y - 8 * s), col, now + 0.08, 2});
                m_paintFx.push_back({ImVec2(u.mouse.x - 9 * s, u.mouse.y + 6 * s), col, now + 0.15, 2});
                toast("Brique 1x1 en main : pose-la, puis tourne-la avec l'anneau");
            }
        }
    }
}

// ------------------------------------------------------------------ the library drawer (bottom)

// ------------------------------------------------------------------ the library: its grip, its dock, the Classic layout
// (Wzx 10/11/2026: « avec le grip redimensionner la fenêtre Bibliothèque ou la déplacer à droite ou à gauche », « un switch
// pour l'avoir en Classic BrickIt UX style »: LEGO / TECHNIC, the tree of the families, a grid of big thumbnails)

// STUDIO | CLASSIC, right aligned at `right`; returns true when it changed
static float stStyleSwitchW(Ui& u, float s) { return stTrackW(u, 7.6f, "STUDIO", 1.2f * s, true) + stTrackW(u, 7.6f, "CLASSIC", 1.2f * s, true) + 38 * s; }
static bool stStyleSwitch(Ui& u, bool& classic, float right, float y, float h, bool live, float s) {
    static const char* kL[2] = {"STUDIO", "CLASSIC"};
    float w[2];
    for (int k = 0; k < 2; ++k) w[k] = stTrackW(u, 7.6f, kL[k], 1.2f * s, true) + 16 * s;
    const ImVec2 a(right - w[0] - w[1] - 6 * s, y), b(right, y + h);
    u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 8), h * 0.5f);
    u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 34), h * 0.5f, 0, 1.0f);
    bool changed = false;
    float x = a.x + 3 * s;
    for (int k = 0; k < 2; ++k) {
        const ImVec2 ka(x, a.y + 3 * s), kb(x + w[k], b.y - 3 * s);
        const bool on = classic == (k == 1), hv = live && u.hover(ka, kb);
        if (on) u.dl->AddRectFilled(ka, kb, stA(C_ACCENT, 0.9f), (kb.y - ka.y) * 0.5f);
        else if (hv) u.dl->AddRectFilled(ka, kb, IM_COL32(255, 255, 255, 14), (kb.y - ka.y) * 0.5f);
        stTrack(u, ImVec2(ka.x + 8 * s, (ka.y + kb.y) * 0.5f - u.fs(7.6f) * 0.52f), 7.6f, on ? C_INK : C_DIM, kL[k], 1.2f * s);
        if (hv && u.pressed && !on) { classic = k == 1; changed = true; }
        if (hv && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip(k ? "Bibliothèque Classic : LEGO / TECHNIC, arbre des familles, grande grille" : "Bibliothèque Studio : familles en pictos, cartes en bandeau");
        x += w[k];
    }
    return changed;
}
// DÉCORS: the parts with stickers or printed patterns shown or hidden (Réglages too), a chip ending at `right`;
// returns its left edge
static float stDecoChip(Ui& u, bool& hideDeco, float right, float y, float h, bool live, float s, bool& changed) {
    const char* t = "DÉCORS";
    const float w = stTrackW(u, 7.6f, t, 1.2f * s, true) + 34 * s;
    const ImVec2 a(right - w, y), b(right, y + h);
    const bool on = !hideDeco, hv = live && u.hover(a, b);
    u.dl->AddRectFilled(a, b, on ? stA(C_ACCENT, 0.16f) : IM_COL32(255, 255, 255, hv ? 16 : 8), h * 0.5f);
    u.dl->AddRect(a, b, on ? stA(C_ACCENT, 0.7f) : IM_COL32(255, 255, 255, 34), h * 0.5f, 0, 1.0f);
    const ImVec2 c(a.x + 12 * s, (a.y + b.y) * 0.5f);   // a little box: ticked when they are shown
    u.dl->AddRect(ImVec2(c.x - 4.5f * s, c.y - 4.5f * s), ImVec2(c.x + 4.5f * s, c.y + 4.5f * s), on ? C_ACCENT : C_DIM, 2 * s, 0, 1.2f * s);
    if (on) u.dl->AddPolyline(std::vector<ImVec2>{ImVec2(c.x - 2.5f * s, c.y), ImVec2(c.x - 0.5f * s, c.y + 2.2f * s), ImVec2(c.x + 3 * s, c.y - 2.5f * s)}.data(), 3, C_ACCENT, 0, 1.4f * s);
    stTrack(u, ImVec2(a.x + 22 * s, (a.y + b.y) * 0.5f - u.fs(7.6f) * 0.52f), 7.6f, on ? C_ACCENT_TXT : C_DIM, t, 1.2f * s);
    if (hv && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip(on ? "Pièces décorées (stickers, motifs imprimés) affichées : clic pour les cacher" : "Pièces décorées (stickers, motifs imprimés) cachées : clic pour les afficher");
    if (hv && u.pressed) { hideDeco = !hideDeco; changed = true; }
    return a.x;
}

// the grip of the library: a drag along the edge resizes it, dragged to a side of the screen it docks there (a ghost
// shows where), a click without moving folds the bottom drawer
void BrickApp::stLibGrip(Ui& u, ImVec2 a, ImVec2 b, bool live) {
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const bool bottom = m_libDock == 0;
    const ImVec2 hc = bottom ? ImVec2((a.x + b.x) * 0.5f, a.y + 6.5f * s) : ImVec2(m_libDock == 1 ? b.x - 6 * s : a.x + 6 * s, (a.y + b.y) * 0.5f);
    const float gz = m_ipad ? 2.0f : 1.0f;   // iPad: a grip for a finger
    const ImVec2 za = bottom ? ImVec2(hc.x - 70 * s, a.y - (gz - 1) * 6 * s) : ImVec2(hc.x - 8 * gz * s, hc.y - 70 * s);
    const ImVec2 zb = bottom ? ImVec2(hc.x + 70 * s, a.y + 13 * gz * s) : ImVec2(hc.x + 8 * gz * s, hc.y + 70 * s);
    const bool hv = live && u.hover(za, zb);
    const ImU32 hcol = m_libGrip || hv ? stA(C_ACCENT, 0.9f) : IM_COL32(255, 255, 255, 46);
    if (bottom) u.dl->AddRectFilled(ImVec2(hc.x - 22 * s, hc.y - 1.5f * s), ImVec2(hc.x + 22 * s, hc.y + 1.5f * s), hcol, 2 * s);
    else u.dl->AddRectFilled(ImVec2(hc.x - 1.5f * s, hc.y - 22 * s), ImVec2(hc.x + 1.5f * s, hc.y + 22 * s), hcol, 2 * s);
    if (hv && !m_libGrip) {
        ImGui::SetMouseCursor(bottom ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
        if (ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Glisser : redimensionner · vers un bord : accrocher à gauche, à droite ou en bas · clic : replier");
    }
    if (hv && u.pressed) {
        m_libGrip = 1;
        m_libGripAt = u.mouse;
        m_libGrip0 = bottom ? b.y - a.y : b.x - a.x;
        m_libDockPreview = -1;
    }
    if (!m_libGrip) return;
    ImGui::SetMouseCursor(bottom ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
    const ImVec2 m = u.mouse;
    const bool moved = std::fabs(m.x - m_libGripAt.x) + std::fabs(m.y - m_libGripAt.y) > 5 * s;
    int prev = -1;
    if (bottom) {
        if (m.y < d.y * 0.8f && m.x < d.x * 0.1f) prev = 1;
        else if (m.y < d.y * 0.8f && m.x > d.x * 0.9f) prev = 2;
        else if (moved && !m_pTray.collapsed) m_libH = std::max(120 * s, m_libGrip0 + (m_libGripAt.y - m.y));
    } else {
        if (m.y > d.y * 0.86f) prev = 0;
        else if (m_libDock == 1 && m.x > d.x * 0.9f) prev = 2;
        else if (m_libDock == 2 && m.x < d.x * 0.1f) prev = 1;
        else m_libW = std::clamp(m_libGrip0 + (m_libDock == 1 ? m.x - m_libGripAt.x : m_libGripAt.x - m.x), 280 * s, d.x * 0.5f);
    }
    m_libDockPreview = prev;
    if (prev >= 0) {   // the ghost of where it docks
        const float top = m_uiRects.empty() ? 46 * s : m_uiRects[0].w;
        const float gw = std::clamp(m_libW > 0 ? m_libW : 540 * s, 280 * s, d.x * 0.5f);
        const ImVec2 ga = prev == 0 ? ImVec2(0, d.y * 0.7f) : prev == 1 ? ImVec2(0, top) : ImVec2(d.x - gw, top);
        const ImVec2 gb = prev == 0 ? ImVec2(d.x, d.y) : prev == 1 ? ImVec2(gw, d.y) : ImVec2(d.x, d.y);
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        fg->AddRectFilled(ga, gb, stA(C_ACCENT, 0.12f), 14 * s);
        fg->AddRect(ga, gb, stA(C_ACCENT, 0.8f), 14 * s, 0, 2 * s);
        const char* t = prev == 0 ? "BIBLIOTHÈQUE EN BAS" : prev == 1 ? "BIBLIOTHÈQUE À GAUCHE" : "BIBLIOTHÈQUE À DROITE";
        ImDrawList* keep = u.dl;
        u.dl = fg;
        const float tw = stTrackW(u, 10, t, 2 * s, true);
        stTrack(u, ImVec2((ga.x + gb.x - tw) * 0.5f, (ga.y + gb.y) * 0.5f - u.fs(10) * 0.5f), 10, C_ACCENT_TXT, t, 2 * s);
        u.dl = keep;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (prev >= 0 && prev != m_libDock) { m_libDock = prev; m_pTray.collapsed = false; m_cardsScroll = 0; m_clsGridScroll = 0; }
        else if (!moved && bottom) m_pTray.collapsed = !m_pTray.collapsed;
        m_libGrip = 0;
        m_libDockPreview = -1;
        savePrefs();
    }
}

// a press on a card (Studio or Classic): long = its sheet; dragged out of the library = in hand under the cursor; click =
// in hand in the middle of the view
void BrickApp::stLibPressEnd(Ui& u, ImVec2 p0, ImVec2 p1) {
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const double now = ImGui::GetTime();
    if (m_libPress >= 0 && (swipeTaken(0) || m_swipeDragged == 0)) m_libPress = -1;
    if (m_libPress < 0) return;
    const bool inside = u.mouse.x >= p0.x && u.mouse.x <= p1.x && u.mouse.y >= p0.y && u.mouse.y <= p1.y;
    const ImVec2 dm(u.mouse.x - m_libPressPos.x, u.mouse.y - m_libPressPos.y);
    const bool moved = dm.x * dm.x + dm.y * dm.y > 49 * s * s;
    if (!m_libLong && !moved && now - m_libPressTime > 0.48) { m_libLong = true; m_infoPart = m_libPress; }
    if (moved && ImGui::IsMouseDown(ImGuiMouseButton_Left) && inside) {
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(m_libPress, tex, uv0, uv1)) {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            const float g = 80 * s;
            const ImVec2 c(u.mouse.x, u.mouse.y - g * 0.35f);
            fg->AddRectFilled(ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), IM_COL32(30, 31, 34, 200), 14 * s);
            fg->AddRect(ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), C_ACCENT, 14 * s, 0, 1.2f * s);
            const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
            fg->AddImage(tex, ImVec2(c.x - g * 0.5f, c.y - g * 0.5f), ImVec2(c.x + g * 0.5f, c.y + g * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), IM_COL32(255, 255, 255, 235));
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (moved && !inside) {
        const int part = m_libPress;
        m_libPress = -1;
        m_libSel = part;
        spawn(part, u.mouse.x, u.mouse.y);
        m_dragStarted = true;
        m_pressed = false;
    } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float stageBottom = m_libDock == 0 ? p0.y : d.y;
        if (!m_libLong && !moved) { m_libSel = m_libPress; spawn(m_libPress, (m_dockL + d.x - m_dockR) * 0.5f, (46 * s + stageBottom) * 0.55f); }
        m_libPress = -1;
    }
}

// the Classic layout (any dock), and the Studio layout docked on a side: LEGO / TECHNIC, the search, the tree of the
// families and their categories (+ / -), a grid of big thumbnails scrolling down
void BrickApp::stDrawerClassic() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float top = m_uiRects.empty() ? 46 * s : m_uiRects[0].w;
    const bool bottom = m_libDock == 0;
    ImVec2 p0, p1;
    if (bottom) {
        const float H = m_pTray.collapsed ? 30 * s : (m_libH > 0 ? std::clamp(m_libH, 220 * s, d.y * 0.62f) : 320 * s);
        p0 = ImVec2(0, d.y - H);
        p1 = ImVec2(d.x, d.y);
    } else {
        const float W = std::clamp(m_libW > 0 ? m_libW : 540 * s, 280 * s, d.x * 0.5f);
        p0 = ImVec2(m_libDock == 1 ? 0 : d.x - W, top);
        p1 = ImVec2(m_libDock == 1 ? W : d.x, d.y);
        (m_libDock == 1 ? m_dockL : m_dockR) = W;
    }
    const float alpha = m_mechOn ? 0.45f : 1.0f;
    u.dl->AddRectFilledMultiColor(p0, p1, IM_COL32(10, 11, 13, 240), IM_COL32(10, 11, 13, 240), IM_COL32(14, 15, 17, 248), IM_COL32(14, 15, 17, 248));
    if (bottom) u.dl->AddLine(ImVec2(0, p0.y), ImVec2(d.x, p0.y), IM_COL32(255, 255, 255, 28), 1.0f);
    else u.dl->AddLine(ImVec2(m_libDock == 1 ? p1.x : p0.x, p0.y), ImVec2(m_libDock == 1 ? p1.x : p0.x, p1.y), IM_COL32(255, 255, 255, 28), 1.0f);
    if (bottom) m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y));
    else { m_uiRects.push_back(ImVec4(0, d.y, d.x, d.y)); m_uiRects.push_back(ImVec4(p0.x, p0.y, p1.x, p1.y)); }   // ([1]: nothing at the bottom)
    const bool live = !m_mechOn && m_popup == 0 && m_ctxPart < 0;
    stLibGrip(u, p0, p1, live);
    if (bottom && m_pTray.collapsed) { stTrack(u, ImVec2(14 * s, p0.y + 10 * s), 9, C_DIM, "BIBLIOTHÈQUE", 2 * s); return; }
    const float pad = 12 * s;
    const float x0 = p0.x + pad + (m_libDock == 2 ? 8 * s : 0), x1 = p1.x - pad - (m_libDock == 1 ? 8 * s : 0);
    float y = p0.y + (bottom ? 16 * s : 12 * s);
    // Studio docked on a side: the compact Studio (Wzx 10/11/2026): the families in pictograms, the categories in pills
    // going to the line, the Studio cards in a grid; Classic: LEGO / TECHNIC and the tree
    const bool studio = !m_libClassic;
    // ---- LEGO / TECHNIC (Classic) or the title (Studio), the switch STUDIO | CLASSIC
    const auto& fams = stFams();
    const bool techFam = m_stFam == "technic" || m_stFam == "mecanique";
    if (!m_searchOpen) m_clsTechnic = techFam;
    {
        const float th = 28 * s, tw = std::min(118 * s, (x1 - x0 - 270 * s) * 0.5f);
        if (studio) stTrack(u, ImVec2(x0 + 2 * s, y + th * 0.5f - u.fs(9) * 0.52f), 9, C_DIM, "BIBLIOTHÈQUE", 2.4f * s);
        for (int k = 0; k < 2 && !studio; ++k) {
            const ImVec2 a(x0 + k * (tw + 6 * s), y), b(a.x + tw, y + th);
            const bool on = m_clsTechnic == (k == 1), h = live && u.hover(a, b);
            u.dl->AddRectFilled(a, b, on ? stA(C_ACCENT, 0.95f) : IM_COL32(255, 255, 255, h ? 16 : 7), 6 * s);
            const char* t = k ? "TECHNIC" : "LEGO";
            const float lw = stTrackW(u, 9, t, 3 * s, true);
            stTrack(u, ImVec2((a.x + b.x - lw) * 0.5f, (a.y + b.y) * 0.5f - u.fs(9) * 0.52f), 9, on ? C_INK : C_DIM, t, 3 * s);
            if (h && u.pressed && !on) {
                m_clsTechnic = k == 1;
                m_stFam = k ? "technic" : "basique";
                auto it = m_stFamSub.find(m_stFam);
                m_stSub = it != m_stFamSub.end() ? it->second : -1;
                m_clsOpen.insert(m_stFam);
                m_searchOpen = false;
                m_search.clear();
                m_clsGridScroll = 0;
            }
        }
        if (stStyleSwitch(u, m_libClassic, x1, y + 1 * s, 26 * s, live, s)) { m_cardsScroll = 0; savePrefs(); }
        {   // DÉCORS, left of the switch (Wzx 10/11/2026: « je ne vois pas le filtre avec / sans pièces décals »)
            bool ch = false;
            stDecoChip(u, m_hideDeco, x1 - stStyleSwitchW(u, s) - 8 * s, y + 1 * s, 26 * s, live, s, ch);
            if (ch) { m_stKey.clear(); m_clsGridScroll = 0; savePrefs(); }
        }
        y += th + 8 * s;
    }
    // ---- the search
    {
        const ImVec2 sa(x0, y), sb(x1, y + 30 * s);
        stGlass(u.dl, sa, sb, 15 * s, s);
        stIcon(u.dl, ST_SEARCH, ImVec2(sa.x + 16 * s, (sa.y + sb.y) * 0.5f), 14 * s, C_DIM, 1.4f * s);
        ImGui::SetNextWindowPos(ImVec2(sa.x + 30 * s, sa.y + 2 * s));
        ImGui::SetNextWindowSize(ImVec2(sb.x - sa.x - 40 * s, sb.y - sa.y - 4 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2 * s, 5 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C_TEXT));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(C_FAINT));
        ImGui::Begin("##clsearch", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s", m_search.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##q", "3001, 2x4, roue…", buf, sizeof(buf))) { m_search = buf; m_searchOpen = !m_search.empty(); m_clsGridScroll = 0; }
        ImGui::End();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);
        y += 30 * s + 10 * s;
    }
    // ---- the tree: Récentes, Favoris, the families of the tab, their categories
    const float treeW = studio ? 0.0f : bottom ? std::clamp((x1 - x0) * 0.2f, 150 * s, 230 * s) : std::clamp((x1 - x0) * 0.34f, 140 * s, 190 * s);
    if (studio) {   // the families: pictograms with their name, as many per line as fit
        const int nc = std::max(3, (int)((x1 - x0) / (78 * s)));
        const float cw = (x1 - x0) / nc, chh = 52 * s;
        int k = 0;
        for (const StFam& f : fams) {
            const int c = k % nc, r = k / nc;
            const ImVec2 a(x0 + c * cw, y + r * chh), b(a.x + cw, a.y + chh);
            const bool on = m_stFam == f.key && !m_searchOpen, h = live && u.hover(a, b);
            if (on) {
                u.dl->AddRectFilledMultiColor(ImVec2(a.x + 2 * s, (a.y + b.y) * 0.5f), ImVec2(b.x - 2 * s, b.y - 2 * s), IM_COL32(246, 196, 0, 0), IM_COL32(246, 196, 0, 0), IM_COL32(246, 196, 0, 24), IM_COL32(246, 196, 0, 24));
                u.dl->AddLine(ImVec2(a.x + 12 * s, b.y - 3 * s), ImVec2(b.x - 12 * s, b.y - 3 * s), IM_COL32(246, 196, 0, 255), 2.0f * s);
            } else if (h) u.dl->AddRectFilled(ImVec2(a.x + 2 * s, a.y + 2 * s), ImVec2(b.x - 2 * s, b.y - 2 * s), IM_COL32(255, 255, 255, 9), 10 * s);
            stIco(u.dl, f.ico, ImVec2((a.x + b.x) * 0.5f, a.y + 17 * s), 23 * s, stA(on ? IM_COL32(217, 180, 67, 255) : IM_COL32(162, 165, 170, 255), alpha), 1.0f * s);
            std::string lab = stUpper(f.label);
            while (lab.size() > 3 && stTrackW(u, 7, lab.c_str(), 0.9f * s, true) > cw - 6 * s) { lab.pop_back(); while (!lab.empty() && ((unsigned char)lab.back() & 0xC0) == 0x80) lab.pop_back(); }
            const float lw = stTrackW(u, 7, lab.c_str(), 0.9f * s, true);
            stTrack(u, ImVec2((a.x + b.x - lw) * 0.5f, b.y - 15 * s), 7, on ? C_ACCENT_TXT : (h ? C_TEXT : C_DIM), lab.c_str(), 0.9f * s);
            if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", f.label);
            if (h && u.pressed) {
                m_stFamSub[m_stFam] = m_stSub;
                m_stFam = f.key;
                auto it = m_stFamSub.find(f.key);
                m_stSub = it != m_stFamSub.end() ? it->second : -1;
                m_searchOpen = false;
                m_search.clear();
                m_clsGridScroll = 0;
            }
            ++k;
        }
        y += ((k + nc - 1) / nc) * chh + 8 * s;
        const StFam* cf = nullptr;
        for (const StFam& f : fams) if (m_stFam == f.key) cf = &f;
        if (cf && !cf->subs.empty() && !m_searchOpen) {   // the categories: pills, to the line
            const float ph = 26 * s;
            float px = x0;
            for (int i = -1; i < (int)cf->subs.size(); ++i) {
                const std::string lab = stUpper(i < 0 ? "Toutes" : cf->subs[i].label);
                const float lw = stTrackW(u, 7.6f, lab.c_str(), 1.0f * s, true), w = 6 * s + 20 * s + 5 * s + lw + 10 * s;
                if (px + w > x1 && px > x0) { px = x0; y += ph + 5 * s; }
                const ImVec2 a(px, y), b(px + w, y + ph);
                const bool on = m_stSub == i, h = live && u.hover(a, b);
                u.dl->AddRectFilled(a, b, on ? IM_COL32(246, 196, 0, 18) : IM_COL32(255, 255, 255, h ? 14 : 8), ph * 0.5f);
                u.dl->AddRect(a, b, on ? IM_COL32(246, 196, 0, 255) : IM_COL32(255, 255, 255, h ? 76 : 36), ph * 0.5f, 0, 1.0f);
                stIco(u.dl, i < 0 ? "all" : cf->subs[i].ico, ImVec2(a.x + 6 * s + 10 * s, (a.y + b.y) * 0.5f), 19 * s, stA(on ? IM_COL32(217, 180, 67, 255) : IM_COL32(162, 165, 170, 255), alpha), 1.0f * s);
                stTrack(u, ImVec2(a.x + 6 * s + 20 * s + 5 * s, (a.y + b.y) * 0.5f - u.fs(7.6f) * 0.55f), 7.6f, on ? IM_COL32(246, 196, 0, 255) : IM_COL32(195, 198, 203, 255), lab.c_str(), 1.0f * s);
                if (h && u.pressed && !on) { m_stSub = i; m_stW = m_stL = 0; m_clsGridScroll = 0; }
                px += w + 5 * s;
            }
            y += ph + 10 * s;
        }
    }
    const ImVec2 ta(x0, y), tb(x0 + treeW, p1.y - pad - 40 * s);
    if (!studio) {
        struct Row { const StFam* f; int sub; };
        std::vector<Row> rows;
        for (const StFam& f : fams) {
            const bool tech = !strcmp(f.key, "technic") || !strcmp(f.key, "mecanique");
            const bool always = !strcmp(f.key, "recent") || !strcmp(f.key, "fav");
            if (!always && tech != m_clsTechnic) continue;
            rows.push_back({&f, -1});
            if (m_clsOpen.count(f.key))
                for (int q = 0; q < (int)f.subs.size(); ++q) rows.push_back({&f, q});
        }
        const float rh = 23 * s, viewH = tb.y - ta.y;
        const float maxS = std::max(0.0f, rows.size() * rh - viewH);
        if (live && u.hover(ta, tb) && ImGui::GetIO().MouseWheel != 0) m_clsTreeScroll -= ImGui::GetIO().MouseWheel * rh * 3;
        m_clsTreeScroll = std::clamp(m_clsTreeScroll, 0.0f, maxS);
        u.dl->PushClipRect(ta, tb, true);
        float ry = ta.y - m_clsTreeScroll;
        for (size_t i = 0; i < rows.size(); ++i, ry += rh) {
            if (ry + rh < ta.y || ry > tb.y) continue;
            const Row& r = rows[i];
            const ImVec2 ra(ta.x, ry), rb(tb.x, ry + rh);
            const bool h = live && u.hover(ra, rb) && u.hover(ta, tb);
            const bool sel = !m_searchOpen && m_stFam == r.f->key && m_stSub == r.sub;
            const bool famRow = r.sub < 0, leaf = r.f->subs.empty();
            if (h) u.dl->AddRectFilled(ra, rb, IM_COL32(255, 255, 255, 10), 5 * s);
            const float cy = ry + rh * 0.5f;
            if (famRow) {
                float tx = ra.x + 6 * s;
                if (!leaf) {   // + / -
                    const bool open = m_clsOpen.count(r.f->key) > 0;
                    u.dl->AddLine(ImVec2(tx, cy), ImVec2(tx + 8 * s, cy), C_DIM, 1.2f * s);
                    if (!open) u.dl->AddLine(ImVec2(tx + 4 * s, cy - 4 * s), ImVec2(tx + 4 * s, cy + 4 * s), C_DIM, 1.2f * s);
                }
                tx += 16 * s;
                u.text(ImVec2(tx, cy - u.fs(11) * 0.55f), 11, sel ? C_ACCENT_TXT : C_TEXT, r.f->label);
            } else {
                const float lx = ra.x + 10 * s;   // the tree line
                const bool last = r.sub == (int)r.f->subs.size() - 1;
                u.dl->AddLine(ImVec2(lx, ry), ImVec2(lx, last ? cy : ry + rh), IM_COL32(255, 255, 255, 50), 1.0f);
                u.dl->AddLine(ImVec2(lx, cy), ImVec2(lx + 8 * s, cy), IM_COL32(255, 255, 255, 50), 1.0f);
                u.text(ImVec2(lx + 12 * s, cy - u.fs(10.5f) * 0.55f), 10.5f, sel ? C_ACCENT_TXT : C_DIM, r.f->subs[r.sub].label);
            }
            if (h && u.pressed) {
                if (famRow && !leaf) { if (m_clsOpen.count(r.f->key) && m_stFam == r.f->key) m_clsOpen.erase(r.f->key); else m_clsOpen.insert(r.f->key); }
                m_stFamSub[m_stFam] = m_stSub;
                m_stFam = r.f->key;
                m_stSub = r.sub;
                m_stW = m_stL = 0;
                m_searchOpen = false;
                m_search.clear();
                m_clsGridScroll = 0;
            }
        }
        u.dl->PopClipRect();
    }
    {   // the size of the thumbnails, under the tree
        const float sw = studio ? std::min(220 * s, x1 - x0) : treeW;
        const float sy = p1.y - pad - 18 * s, tx0 = x0 + 22 * s, tx1 = x0 + sw - 22 * s;
        const float cs = std::clamp(m_stCardScale, 0.6f, 2.0f);
        stIco(u.dl, "brick", ImVec2(x0 + 7 * s, sy), 13 * s, C_DIM, 1.0f * s);
        stIco(u.dl, "brick", ImVec2(x0 + sw - 9 * s, sy), 19 * s, C_DIM, 1.0f * s);
        const float k = (cs - 0.6f) / 1.4f, kx = tx0 + (tx1 - tx0) * k;
        u.dl->AddLine(ImVec2(tx0, sy), ImVec2(tx1, sy), IM_COL32(255, 255, 255, 40), 2 * s);
        u.dl->AddLine(ImVec2(tx0, sy), ImVec2(kx, sy), stA(C_ACCENT, 0.9f), 2 * s);
        const bool hs = live && u.hover(ImVec2(tx0 - 8 * s, sy - 10 * s), ImVec2(tx1 + 8 * s, sy + 10 * s));
        if (hs && u.pressed) m_stCardDrag = true;
        if (m_stCardDrag) {
            m_stCardScale = 0.6f + std::clamp((u.mouse.x - tx0) / (tx1 - tx0), 0.0f, 1.0f) * 1.4f;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                m_stCardDrag = false;
                for (auto& t : m_lazyThumbs) m_gui->releaseTexture(t.second.first);
                m_lazyThumbs.clear();
                savePrefs();
            }
        }
        u.dl->AddCircleFilled(ImVec2(kx, sy), (hs || m_stCardDrag ? 7 : 6) * s, IM_COL32(0xf4, 0xf4, 0xf6, 255), 20);
    }
    // ---- the parts: the category, or the whole family (its categories together), or the search (cached)
    const StFam* fam = nullptr;
    for (const StFam& f : fams) if (m_stFam == f.key) fam = &f;
    if (!fam) { m_stFam = "basique"; m_stSub = -1; fam = &fams[2]; }
    if (m_stSub >= (int)fam->subs.size()) m_stSub = -1;
    const StSub* sub = !m_searchOpen && m_stSub >= 0 ? &fam->subs[m_stSub] : nullptr;
    {
        int avail = 0;
        for (const Part& p : m_lib.parts) avail += p.available() ? 1 : 0;
        const std::string key = "cls|" + m_stFam + "|" + std::to_string(m_stSub) + "|" + (m_searchOpen ? m_search : std::string()) + "|" + std::to_string(avail) + "|" +
                                std::to_string(m_recent.size()) + "|" + std::to_string(m_fav.size()) + (m_hideDeco ? "|d" : "");
        if (key != m_stKey) {
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
    }
    const std::vector<int>& parts = m_stParts;
    // ---- the grid
    const float gx0 = studio ? x0 : x0 + treeW + 12 * s;
    const ImVec2 ga(gx0, ta.y), gb(x1, studio ? p1.y - pad - 38 * s : p1.y - pad);
    const float cs = std::clamp(m_stCardScale, 0.6f, 2.0f), gap = (studio ? 8 : 4) * s;
    const float cell0 = 104 * s * cs;
    const int cols = std::max(1, (int)((gb.x - ga.x + gap) / (cell0 + gap)));
    const float cell = std::min(cell0 * 1.3f, (gb.x - ga.x - (cols - 1) * gap) / cols);   // (the cells fill the width, not much bigger than asked)
    const int rowsN = ((int)parts.size() + cols - 1) / cols;
    const float contentH = rowsN * (cell + gap) - gap, viewH = gb.y - ga.y;
    if (live && u.hover(ga, gb) && ImGui::GetIO().MouseWheel != 0) m_clsGridScroll -= ImGui::GetIO().MouseWheel * (cell + gap);
    for (int k = 0; k < (int)parts.size() && m_libFoundScroll; ++k)
        if (parts[k] == m_libFound) { m_clsGridScroll = (k / cols) * (cell + gap) - (viewH - cell) * 0.5f; m_libFoundScroll = false; }
    if (!bottom) swipeRow(0, ga, gb, m_clsGridScroll, m_clsVel, std::max(0.0f, contentH - viewH), live, true);   // (a side: drag up / down to scroll, sideways to take)
    m_clsGridScroll = std::clamp(m_clsGridScroll, 0.0f, std::max(0.0f, contentH - viewH));
    if (parts.empty()) {
        const char* msg = m_searchOpen ? "Aucune pièce pour cette recherche" : m_stFam == "recent" ? "Les pièces que tu poses apparaîtront ici"
                        : m_stFam == "fav" ? "Aucun favori : appui long sur une pièce, puis Favori" : "Aucune pièce ici";
        u.text(ImVec2(ga.x + 8 * s, ga.y + 12 * s), 11.5f, C_DIM, msg);
    }
    u.dl->PushClipRect(ga, gb, true);
    const double now = ImGui::GetTime();
    const int taken = m_libPress >= 0 ? m_libPress : (m_holding ? m_libSel : -1);
    bool hovered = false;
    for (int k = 0; k < (int)parts.size(); ++k) {
        const int c = k % cols, r = k / cols;
        const ImVec2 a(ga.x + c * (cell + gap), ga.y + r * (cell + gap) - m_clsGridScroll), b(a.x + cell, a.y + cell);
        if (b.y < ga.y || a.y > gb.y) continue;
        const int part = parts[k];
        const Part& P = m_lib.parts[part];
        const bool h = live && u.hover(a, b) && u.hover(ga, gb);
        const bool on = part == taken;
        const float rr = (studio ? 12 : 4) * s;   // (Studio: the cards of the drawer, Classic: flat cells)
        if (studio) {
            u.dl->AddRectFilled(a, b, stA(IM_COL32(18, 19, 24, 255), alpha), rr);
            u.dl->AddRect(a, b, stA(on ? C_ACCENT : IM_COL32(255, 255, 255, h ? 102 : 41), alpha), rr, 0, on ? 1.5f * s : 1.0f);
        } else {
            u.dl->AddRectFilled(a, b, stA(h ? IM_COL32(30, 31, 36, 255) : IM_COL32(22, 23, 27, 255), alpha), rr);
            if (on) u.dl->AddRect(a, b, C_ACCENT, rr, 0, 1.5f * s);
        }
        if (m_dev && P.connSrc) u.dl->AddRect(a, b, stConnCol(P.connSrc), 4 * s, 0, 2.0f * s);
        if (part == m_libFound) u.dl->AddRect(ImVec2(a.x - 1 * s, a.y - 1 * s), ImVec2(b.x + 1 * s, b.y + 1 * s), C_ACCENT, 5 * s, 0, 2.5f * s);
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (thumbUv(part, tex, uv0, uv1)) {
            const float z = studio ? 1.5f : 1.3f;
            const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / z) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / z) * 0.5f);
            u.dl->AddImage(tex, ImVec2(a.x + 4 * s, a.y + 4 * s), ImVec2(b.x - 4 * s, b.y - 4 * s), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y),
                           stA(IM_COL32(232, 234, 238, 255), alpha * (on ? 0.4f : 1.0f)));
        }
        if (std::find(m_fav.begin(), m_fav.end(), part) != m_fav.end()) stPicto(u.dl, "star", 0, 0, ImVec2(b.x - 10 * s, a.y + 10 * s), 10 * s, C_ACCENT_TXT, 1.0f * s);
        if (h) {
            if (m_stHov != part) { m_stHov = part; m_stHovT = now - m_stSheetSeen < 0.25 ? now - 0.6 : now; m_stHovA = a; m_stHovB = b; }
            hovered = true;
            if (u.pressed) { m_libFound = -1; m_libPress = part; m_libPressPos = u.mouse; m_libPressTime = now; m_libLong = false; }
            if (m_dev && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { m_ctxPart = part; m_ctxPos = u.mouse; }
        }
    }
    u.dl->PopClipRect();
    if (contentH > viewH) {   // a thin scroll bar
        const float bh = viewH * viewH / contentH, by = ga.y + (viewH - bh) * (m_clsGridScroll / (contentH - viewH));
        u.dl->AddRectFilled(ImVec2(gb.x + 3 * s, by), ImVec2(gb.x + 5 * s, by + bh), IM_COL32(255, 255, 255, 40), 2 * s);
    }
    if (!hovered || m_libPress >= 0 || ImGui::IsMouseDown(ImGuiMouseButton_Left)) { if (!hovered) m_stHov = -1; }
    else if (m_stHov >= 0 && now - m_stHovT > 0.5) { stCardSheet(m_stHov, m_stHovA, m_stHovB, (float)(now - m_stHovT - 0.5)); m_stSheetSeen = now; }
    stLibPressEnd(u, p0, p1);
}

void BrickApp::stDrawer() {
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    m_dockL = m_dockR = 0;
    if (!m_pTray.visible) { m_uiRects.push_back(ImVec4(0, d.y, 0, d.y)); return; }
    if (m_libDock != 0 || m_libClassic) { stDrawerClassic(); return; }   // (Classic, or docked on a side)
    const bool sheet = m_stCardMode == 1;   // Réglages › Bibliothèque: Image (the image alone) / Fiche (image, size, name, id)
    // the cards at the size of the slider of the drawer (Wzx 09/11/2026); the drawer keeps its height: as many rows as fit
    const float cs = std::clamp(m_stCardScale, 0.6f, 2.0f);
    const float famH = (m_ipad ? 46 : 58) * s, subH = (m_ipad ? 32 : 36) * s, cardW = (sheet ? 116 : 88) * s * cs, cardH = 88 * s * cs, tileW = 140 * s, tileH = 88 * s, gap = 8 * s, padX = 14 * s;
    // the height of the drawer: its grip (m_libH), never less than two rows of tiles
    const float chrome = 12 * s + famH + 8 * s + subH + 10 * s + 12 * s;
    // iPad (Wzx 10/11/2026: « elle prend la moitié de l'écran »): one row by default, no empty band under it
    // (iPad: as high as the left column of the page needs: the tiles of Toutes 96, cards 124 with their slider, 150 with
    // the size in studs of Briques / Plaques / Tuiles)
    const bool tilesP = !m_searchOpen && m_stSub < 0 && m_stFam != "recent" && m_stFam != "fav";
    const bool sizableP = !m_searchOpen && m_stFam == "basique" && m_stSub >= 0 && m_stSub <= 2;
    const float minBody = m_ipad ? std::max((tilesP ? 96 : sizableP ? 150 : 124) * s, tilesP ? tileH : cardH) : 2 * tileH + gap;
    const float bodyH = m_libH > 0 ? std::clamp(m_libH - chrome, minBody, std::max(minBody, d.y * 0.62f - chrome)) : minBody;
    const int nRows = std::max(1, (int)((bodyH + gap) / (cardH + gap)));
    const float rowsY = (bodyH - (nRows * (cardH + gap) - gap)) * 0.5f;
    const float H = m_pTray.collapsed ? 30 * s : 12 * s + famH + 8 * s + subH + 10 * s + bodyH + 12 * s;
    const ImVec2 p0(0, d.y - H), p1(d.x, d.y);
    const float alpha = m_mechOn ? 0.45f : 1.0f;
    u.dl->AddRectFilledMultiColor(p0, p1, IM_COL32(10, 11, 13, 235), IM_COL32(10, 11, 13, 235), IM_COL32(14, 15, 17, 245), IM_COL32(14, 15, 17, 245));
    u.dl->AddLine(ImVec2(0, p0.y), ImVec2(d.x, p0.y), IM_COL32(255, 255, 255, 28), 1.0f);
    for (float x = 96 * s; x < d.x; x += 96 * s) u.dl->AddLine(ImVec2(x, p0.y + 1), ImVec2(x, p1.y), IM_COL32(255, 255, 255, 5), 1.0f);   // the faint ribs
    m_uiRects.push_back(ImVec4(0, p0.y, d.x, d.y));
    const bool live = !m_mechOn && m_popup == 0 && m_ctxPart < 0;   // (the menu of a card open, DEV: its click is not a press on the card under it)
    // the grip: dragged it resizes the drawer or docks it on a side, a click folds it
    stLibGrip(u, p0, p1, live);
    if (m_pTray.collapsed) { stTrack(u, ImVec2(padX, p0.y + 10 * s), 9, C_DIM, "BIBLIOTHÈQUE", 2 * s); return; }
    // ---- line 1, the families: a glass strip of pictogram tabs (Maquette 4.1)
    const auto& fams = stFams();
    const StFam* fam = nullptr;
    for (const StFam& f : fams) if (m_stFam == f.key) fam = &f;
    if (!fam) { m_stFam = "basique"; m_stSub = -1; fam = &fams[2]; }
    if (m_stSub >= (int)fam->subs.size()) m_stSub = -1;
    {   // each family and category keeps where its cards were scrolled (Wzx 09/11/2026)
        const std::string sk = m_searchOpen ? std::string("?") : m_stFam + "|" + std::to_string(m_stSub);
        if (sk != m_stScrollKey) {
            if (!m_stScrollKey.empty()) m_stScrollMem[m_stScrollKey] = m_cardsScroll;
            auto it = m_stScrollMem.find(sk);
            m_cardsScroll = it != m_stScrollMem.end() && sk != "?" ? it->second : 0.0f;
            m_cardsVel = 0;
            m_stScrollKey = sk;
        }
    }
    float y = p0.y + 12 * s;
    const ImVec2 fa(padX, y), fb(d.x - padX, y + famH);
    stGlass(u.dl, fa, fb, 16 * s, s);
    {
        const float tw = (fb.x - fa.x - 8 * s) / fams.size();
        for (size_t i = 0; i < fams.size(); ++i) {
            const StFam& f = fams[i];
            const ImVec2 ta(fa.x + 4 * s + i * tw + 1 * s, fa.y + 4 * s), tb(ta.x + tw - 2 * s, fb.y - 4 * s);
            const bool on = &f == fam && !m_searchOpen;
            const bool h = live && u.hover(ta, tb);
            if (on) {   // the amber glow from below, the underline
                u.dl->AddRectFilledMultiColor(ImVec2(ta.x, (ta.y + tb.y) * 0.5f), tb, IM_COL32(246, 196, 0, 0), IM_COL32(246, 196, 0, 0), IM_COL32(246, 196, 0, 22), IM_COL32(246, 196, 0, 22));
                u.dl->AddLine(ImVec2(ta.x + 10 * s, tb.y - 1 * s), ImVec2(tb.x - 10 * s, tb.y - 1 * s), IM_COL32(246, 196, 0, 255), 2.0f * s);
            } else if (h) u.dl->AddRectFilled(ta, tb, IM_COL32(255, 255, 255, 9), 12 * s);
            const ImU32 pic = stA(on ? IM_COL32(217, 180, 67, 255) : IM_COL32(162, 165, 170, 255), alpha);
            const ImU32 txt = stA(on ? IM_COL32(246, 196, 0, 255) : (h ? C_TEXT : IM_COL32(167, 170, 175, 255)), alpha);
            const float icy = m_ipad ? 12 * s : 18 * s, icz = m_ipad ? 20 * s : 27 * s;   // (iPad: a lower strip)
            stIco(u.dl, f.ico, ImVec2((ta.x + tb.x) * 0.5f, ta.y + icy - (h && !on ? 1.5f * s : 0)), icz, pic, 1.0f * s);
            const std::string lab = stUpper(f.label);
            const float lw = stTrackW(u, 8.6f, lab.c_str(), 1.4f * s, true);
            stTrack(u, ImVec2((ta.x + tb.x - lw) * 0.5f, tb.y - (m_ipad ? 14 : 17) * s), 8.6f, txt, lab.c_str(), 1.4f * s);
            if (h && u.pressed) {   // back on a family: its last category (Wzx 09/11/2026)
                m_stFamSub[m_stFam] = m_stSub;
                m_stFam = f.key;
                auto it = m_stFamSub.find(f.key);
                m_stSub = it != m_stFamSub.end() ? it->second : -1;
                m_stW = m_stL = 0;
                m_searchOpen = false;
                m_search.clear();
                m_stSubScroll = 0;
            }
        }
    }
    y += famH + 8 * s;
    // ---- line 2, the categories of the family: pills with their pictogram (TOUTES first)
    if (!fam->subs.empty() && !m_searchOpen) {
        const ImVec2 ra(padX, y), rb(d.x - padX - 290 * s, y + subH);   // (DÉCORS, STUDIO | CLASSIC on the right)
        float x = ra.x - m_stSubScroll, total = 0;
        u.dl->PushClipRect(ra, rb, true);
        for (int i = -1; i < (int)fam->subs.size(); ++i) {
            const char* lab0 = i < 0 ? "Toutes" : fam->subs[i].label;
            const std::string lab = stUpper(lab0);
            const float lw = stTrackW(u, 8.6f, lab.c_str(), 1.2f * s, true), w = 6 * s + 28 * s + 7 * s + lw + 12 * s;
            const ImVec2 a(x, y), b(x + w, y + subH);
            const bool on = m_stSub == i, h = live && u.hover(a, b) && u.hover(ra, rb);
            u.dl->AddRectFilled(a, b, on ? IM_COL32(246, 196, 0, 18) : IM_COL32(255, 255, 255, h ? 14 : 8), subH * 0.5f);
            u.dl->AddRect(a, b, on ? IM_COL32(246, 196, 0, 255) : IM_COL32(255, 255, 255, h ? 76 : 36), subH * 0.5f, 0, 1.0f);
            stIco(u.dl, i < 0 ? "all" : fam->subs[i].ico, ImVec2(a.x + 6 * s + 14 * s, (a.y + b.y) * 0.5f), 26 * s, stA(on ? IM_COL32(217, 180, 67, 255) : IM_COL32(162, 165, 170, 255), alpha), 1.0f * s);
            stTrack(u, ImVec2(a.x + 6 * s + 28 * s + 7 * s, (a.y + b.y) * 0.5f - u.fs(8.6f) * 0.55f), 8.6f, stA(on ? IM_COL32(246, 196, 0, 255) : IM_COL32(195, 198, 203, 255), alpha), lab.c_str(), 1.2f * s);
            if (h && u.pressed && !on) { m_stSub = i; m_stW = m_stL = 0; }
            x += w + 6 * s;
            total += w + 6 * s;
        }
        u.dl->PopClipRect();
        const float maxS = std::max(0.0f, total - 6 * s - (rb.x - ra.x));
        if (live && u.hover(ra, rb) && ImGui::GetIO().MouseWheel != 0) m_stSubScroll -= ImGui::GetIO().MouseWheel * 120 * s;
        m_stSubScroll = std::clamp(m_stSubScroll, 0.0f, maxS);
    }
    if (stStyleSwitch(u, m_libClassic, d.x - padX, y + 5 * s, subH - 10 * s, live, s)) { m_clsGridScroll = 0; savePrefs(); }
    {
        bool ch = false;
        stDecoChip(u, m_hideDeco, d.x - padX - stStyleSwitchW(u, s) - 8 * s, y + 5 * s, subH - 10 * s, live, s, ch);
        if (ch) { m_stKey.clear(); m_cardsScroll = 0; savePrefs(); }
    }
    y += subH + 10 * s;
    const bool tiles = !m_searchOpen && !fam->subs.empty() && m_stSub < 0;   // Toutes: one tile per category
    const StSub* sub = !m_searchOpen && m_stSub >= 0 ? &fam->subs[m_stSub] : nullptr;
    const bool sizable = sub && m_stFam == "basique" && m_stSub <= 2;      // Briques, Plaques, Tuiles: the size in studs
    // ---- left column: the search, the size in studs (or the path), the display of the cards
    const float colW = 210 * s;
    {
        const ImVec2 sa(padX, y), sb(padX + colW, y + 32 * s);
        stGlass(u.dl, sa, sb, 16 * s, s);
        stIcon(u.dl, ST_SEARCH, ImVec2(sa.x + 17 * s, (sa.y + sb.y) * 0.5f), 15 * s, C_DIM, 1.4f * s);
        ImGui::SetNextWindowPos(ImVec2(sa.x + 32 * s, sa.y + 3 * s));
        ImGui::SetNextWindowSize(ImVec2(sb.x - sa.x - 42 * s, sb.y - sa.y - 6 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2 * s, 5 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C_TEXT));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(C_FAINT));
        ImGui::Begin("##stsearch", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s", m_search.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##q", "3001, 2x4, roue…", buf, sizeof(buf))) { m_search = buf; m_searchOpen = !m_search.empty(); m_cardsScroll = 0; }
        ImGui::End();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);
    }
    if (sizable) {
        // the stud picker, 1.5 x smaller than before (Wzx 09/11/2026): a press on a stud then held, the size follows the
        // cursor over the studs (lit from the corner), the release sets it; TOUTES (the label) resets it
        const float mw = 140 * s, cell = (mw - 12 * s) / 8.0f, hdr = 16 * s;
        const ImVec2 ka(padX, y + 40 * s), kb(padX + mw, y + 40 * s + hdr + 4 * cell + 6 * s);
        stGlass(u.dl, ka, kb, 12 * s, s);
        const ImVec2 g0(ka.x + 6 * s, ka.y + hdr);
        int pw = m_stW, pl = m_stL;   // what the studs show: the size, or the one under the held cursor
        const bool overGrid = live && u.hover(g0, ImVec2(g0.x + 8 * cell, g0.y + 4 * cell));
        if (overGrid && u.pressed) m_stSizeDrag = true;
        if (m_stSizeDrag) {
            pw = std::clamp((int)((u.mouse.y - g0.y) / cell) + 1, 1, 4);
            pl = std::clamp((int)((u.mouse.x - g0.x) / cell) + 1, 1, 8);
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { m_stSizeDrag = false; m_stW = pw; m_stL = pl; m_cardsScroll = 0; }
        }
        stTrack(u, ImVec2(ka.x + 7 * s, ka.y + 5 * s), 6.6f, C_DIM, "TAILLE", 1.4f * s);
        char lab[16];
        if (pw > 0) snprintf(lab, sizeof(lab), "%d × %d", pw, pl); else snprintf(lab, sizeof(lab), "TOUTES");
        const float lw = stTrackW(u, 7.6f, lab, 0.6f * s, true);
        const ImVec2 la(kb.x - 7 * s - lw, ka.y + 3.5f * s);
        stTrack(u, la, 7.6f, C_ACCENT_TXT, lab, 0.6f * s);
        if (live && !m_stSizeDrag && u.hover(ImVec2(la.x - 4 * s, ka.y), ImVec2(kb.x, ka.y + hdr)) && u.pressed) { m_stW = m_stL = 0; m_cardsScroll = 0; }
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 8; ++c) {
                const ImVec2 cc(g0.x + (c + 0.5f) * cell, g0.y + (r + 0.5f) * cell);
                const float rad = cell * 0.34f;
                const bool in = pw > 0 && r < pw && c < pl;
                const bool h = live && !m_stSizeDrag && overGrid && (int)((u.mouse.x - g0.x) / cell) == c && (int)((u.mouse.y - g0.y) / cell) == r;
                if (in) { u.dl->AddCircleFilled(cc, rad, stA(C_ACCENT, m_stSizeDrag ? 0.95f : 0.85f), 18); u.dl->AddCircleFilled(cc, rad * 0.45f, stA(C_ACCENT, 0.35f), 14); }
                else u.dl->AddCircle(cc, rad, h ? C_TEXT : IM_COL32(255, 255, 255, 56), 18, 1.0f);
            }
    } else {   // the path: FAMILLE › CATÉGORIE
        const ImVec2 ka(padX, y + 40 * s), kb(padX + colW, y + 40 * s + 52 * s);
        stGlass(u.dl, ka, kb, 14 * s, s);
        stTrack(u, ImVec2(ka.x + 12 * s, ka.y + 10 * s), 7, C_DIM, "CHEMIN", 1.8f * s);
        std::string path = m_searchOpen ? std::string("Recherche") : std::string(fam->label) + (sub ? std::string(" › ") + sub->label : std::string());
        path = stUpper(path);
        while (path.size() > 4 && stTrackW(u, 9, path.c_str(), 1.2f * s, true) > colW - 24 * s) { path.pop_back(); while (!path.empty() && ((unsigned char)path.back() & 0xC0) == 0x80) path.pop_back(); }
        stTrack(u, ImVec2(ka.x + 12 * s, ka.y + 28 * s), 9, C_TEXT, path.c_str(), 1.2f * s);
    }
    if (!tiles) {   // the display of the cards: IMAGE / FICHE (also in Réglages)
        const float ty = y + bodyH - (m_ipad ? 0 : 24) * s;   // (iPad: no IMAGE / FICHE here, it is in Réglages)
        {   // the size of the cards: a slider (small brick, big brick), released: the thumbnails made again at that size
            const float sy = ty - 22 * s, x0 = padX + 2 * s, tx0 = x0 + 22 * s, tx1 = padX + colW - 22 * s;
            stIco(u.dl, "brick", ImVec2(x0 + 7 * s, sy), 13 * s, C_DIM, 1.0f * s);
            stIco(u.dl, "brick", ImVec2(padX + colW - 9 * s, sy), 19 * s, C_DIM, 1.0f * s);
            const float k = (cs - 0.6f) / 1.4f, kx = tx0 + (tx1 - tx0) * k;
            u.dl->AddLine(ImVec2(tx0, sy), ImVec2(tx1, sy), IM_COL32(255, 255, 255, 40), 2 * s);
            u.dl->AddLine(ImVec2(tx0, sy), ImVec2(kx, sy), stA(C_ACCENT, 0.9f), 2 * s);
            const bool hs = live && u.hover(ImVec2(tx0 - 8 * s, sy - 10 * s), ImVec2(tx1 + 8 * s, sy + 10 * s));
            if (hs && u.pressed) m_stCardDrag = true;
            if (m_stCardDrag) {
                m_stCardScale = 0.6f + std::clamp((u.mouse.x - tx0) / (tx1 - tx0), 0.0f, 1.0f) * 1.4f;
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    m_stCardDrag = false;
                    for (auto& t : m_lazyThumbs) m_gui->releaseTexture(t.second.first);   // (made again at the new size)
                    m_lazyThumbs.clear();
                    savePrefs();
                }
            }
            u.dl->AddCircleFilled(ImVec2(kx, sy), (hs || m_stCardDrag ? 7 : 6) * s, IM_COL32(0xf4, 0xf4, 0xf6, 255), 20);
            if (hs && !m_stCardDrag && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Taille des vignettes");
        }
        if (!m_ipad) {
        stTrack(u, ImVec2(padX + 2 * s, ty + 12 * s - u.fs(8) * 0.52f), 8, C_DIM, "AFFICHAGE", 1.6f * s);
        static const char* kModes[2] = {"IMAGE", "FICHE"};
        float bx = padX + colW;
        for (int k = 1; k >= 0; --k) {
            const float bw = stTrackW(u, 8, kModes[k], 1.2f * s, true) + 18 * s;
            const ImVec2 ba(bx - bw, ty), bb(bx, ty + 24 * s);
            const bool on = m_stCardMode == k, h = live && u.hover(ba, bb);
            u.dl->AddRectFilled(ba, bb, on ? stA(C_ACCENT, 0.2f) : IM_COL32(255, 255, 255, h ? 18 : 8), 12 * s);
            u.dl->AddRect(ba, bb, on ? stA(C_ACCENT, 0.8f) : IM_COL32(255, 255, 255, 34), 12 * s, 0, 1.0f);
            stTrack(u, ImVec2(ba.x + 9 * s, ty + 12 * s - u.fs(8) * 0.52f), 8, on ? C_ACCENT_TXT : C_DIM, kModes[k], 1.2f * s);
            if (h && u.pressed && !on) { m_stCardMode = k; m_cardsScroll = 0; savePrefs(); }
            bx -= bw + 4 * s;
        }
        }
    }
    // ---- the parts of the category, and how many each category has (cached: the full library has 22 000 parts)
    int avail = 0;
    for (const Part& p : m_lib.parts) avail += p.available() ? 1 : 0;
    const std::string key = m_stFam + "|" + std::to_string(m_stSub) + "|" + std::to_string(m_stW) + "x" + std::to_string(m_stL) + "|" + (m_searchOpen ? m_search : std::string()) + "|" +
                            std::to_string(avail) + "|" + std::to_string(m_recent.size()) + "|" + std::to_string(m_fav.size()) + (m_hideDeco ? "|d" : "");
    if (key != m_stKey) {
        m_stKey = key;
        m_stParts.clear();
        m_stSubCount.assign(fam->subs.size(), 0);
        if (m_searchOpen) m_stParts = visibleParts();
        else if (m_stFam == "recent") m_stParts = m_recent;
        else if (m_stFam == "fav") m_stParts = m_fav;
        else {
            for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
                const Part& p = m_lib.parts[i];
                if (!p.available()) continue;
                if ((m_hideDeco && partDeco(p)) || partDuplo(p)) continue;   // (Réglages: the stickers and printed patterns hidden; no Duplo)
                const std::string& c = partCat(i);
                for (size_t q = 0; q < fam->subs.size(); ++q)
                    if (stInSub(fam->subs[q], c, p.sub)) ++m_stSubCount[q];
                bool in = sub && stInSub(*sub, c, p.sub);
                if (in && m_stW > 0) {
                    int tw = 0, tl = 0;   // the size in studs, from the title
                    const std::string& t = p.label;
                    const size_t xpos = t.find(" x ");
                    if (xpos != std::string::npos) {
                        size_t a0 = xpos;
                        while (a0 > 0 && (std::isdigit((unsigned char)t[a0 - 1]) || t[a0 - 1] == ' ')) --a0;
                        tw = std::atoi(t.substr(a0, xpos - a0).c_str());
                        tl = std::atoi(t.substr(xpos + 3).c_str());
                    }
                    in = (tw == m_stW && tl == m_stL) || (tw == m_stL && tl == m_stW);
                }
                if (in) m_stParts.push_back(i);
            }
        }
    }
    const std::vector<int>& parts = m_stParts;
    // ---- the cards: two rows, column by column, sliding sideways (wheel, drag)
    const ImVec2 ca(padX + colW + 12 * s, y), cb(d.x - padX, y + bodyH);
    const int nItems = tiles ? (int)fam->subs.size() : (int)parts.size();
    const float itemW = tiles ? tileW : cardW;
    const float viewW = cb.x - ca.x;
    // the tiles read row by row, as in the maquette (in one row when they fit); the cards go column by column
    const int perRow = tiles ? (m_ipad ? nItems : std::min(nItems, std::max((nItems + 1) / 2, (int)((viewW + gap) / (tileW + gap))))) : 0;   // (iPad: one row)
    const int cols = tiles ? perRow : (nItems + nRows - 1) / nRows;
    const float contentW = cols * (itemW + gap) - gap;
    if (live && u.hover(ca, cb) && ImGui::GetIO().MouseWheel != 0) m_cardsScroll -= ImGui::GetIO().MouseWheel * itemW;
    swipeRow(0, ca, cb, m_cardsScroll, m_cardsVel, std::max(0.0f, contentW - viewW), live);
    u.dl->PushClipRect(ca, cb, true);
    if (!sizable) m_stSizeDrag = false;
    // the ground of a card or a tile: the grey of the maquette (the thumbnails are transparent PNG), a hairline
    auto cardBg = [&](ImVec2 a, ImVec2 b, bool h, bool on) {
        u.dl->AddRectFilled(a, b, stA(IM_COL32(18, 19, 24, 255), alpha), 12 * s);
        u.dl->AddRectFilledMultiColor(ImVec2(a.x + 6 * s, (a.y + b.y) * 0.5f), ImVec2(b.x - 6 * s, b.y - 6 * s), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 14), IM_COL32(0, 0, 0, 14));
        u.dl->AddRect(a, b, stA(on ? C_ACCENT : IM_COL32(255, 255, 255, h ? 102 : 41), alpha), 12 * s, 0, on ? 1.5f * s : 1.0f);
    };
    if (!tiles && parts.empty()) {
        const char* msg = m_searchOpen ? "Aucune pièce pour cette recherche" : m_stFam == "recent" ? "Les pièces que tu poses apparaîtront ici"
                        : m_stFam == "fav" ? "Aucun favori : appui long sur une pièce, puis Favori" : m_stW > 0 ? "Aucune pièce de cette taille ici : clique TOUTES" : "Aucune pièce importée ici";
        u.text(ImVec2(ca.x + 8 * s, ca.y + 20 * s), 11.5f, C_DIM, msg);
    }
    const double now = ImGui::GetTime();
    const int taken = m_libPress >= 0 ? m_libPress : (m_holding ? m_libSel : -1);
    bool hovered = false;
    if (tiles)   // Toutes: one tile per category, its pictogram big (Maquette 4.1); a click opens it
        for (int k = 0; k < nItems; ++k) {
            const int c = k % std::max(1, perRow), r = k / std::max(1, perRow);
            const ImVec2 a(ca.x + c * (tileW + gap) - m_cardsScroll, ca.y + (m_ipad ? (bodyH - tileH) * 0.5f : 0) + r * (tileH + gap)), b(a.x + tileW, a.y + tileH);
            if (b.x < ca.x || a.x > cb.x) continue;
            const StSub& q = fam->subs[k];
            const int n = k < (int)m_stSubCount.size() ? m_stSubCount[k] : 0;
            const bool h = live && u.hover(a, b) && u.hover(ca, cb);
            const float lift = h ? -1 * s : 0, op = alpha * (n > 0 ? 1.0f : 0.4f);
            cardBg(ImVec2(a.x, a.y + lift), ImVec2(b.x, b.y + lift), h, false);
            stIco(u.dl, q.ico, ImVec2((a.x + b.x) * 0.5f, a.y + 30 * s + lift), 46 * s, stA(IM_COL32(184, 187, 192, 255), op), 1.1f * s);
            std::string lab = stUpper(q.label);
            while (lab.size() > 4 && stTrackW(u, 8, lab.c_str(), 1.2f * s, true) > tileW - 12 * s) { lab.pop_back(); while (!lab.empty() && ((unsigned char)lab.back() & 0xC0) == 0x80) lab.pop_back(); }
            const float lw = stTrackW(u, 8, lab.c_str(), 1.2f * s, true);
            stTrack(u, ImVec2((a.x + b.x - lw) * 0.5f, a.y + 58 * s + lift), 8, stA(C_TEXT, op), lab.c_str(), 1.2f * s);
            const char* cat = "catégorie";
            const float cw = u.textW(8, cat);
            u.text(ImVec2((a.x + b.x - cw) * 0.5f, a.y + 71 * s + lift), 8, stA(IM_COL32(117, 120, 126, 255), op), cat);
            if (h && u.pressed) { m_stSub = k; m_stW = m_stL = 0; }
        }
    else for (int k = 0; k < (int)parts.size(); ++k) {
        const int c = k / nRows, r = k % nRows;
        if (parts[k] == m_libFound && m_libFoundScroll) {   // (found from the marking menu: brought into view once, centred)
            m_cardsScroll = std::max(0.0f, c * (cardW + gap) - (cb.x - ca.x - cardW) * 0.5f);
            m_libFoundScroll = false;
        }
        const ImVec2 a(ca.x + c * (cardW + gap) - m_cardsScroll, ca.y + rowsY + r * (cardH + gap)), b(a.x + cardW, a.y + cardH);
        if (b.x < ca.x || a.x > cb.x) continue;
        const int part = parts[k];
        const Part& P = m_lib.parts[part];
        if (k == 3) m_mtCard = ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
        const bool h = live && u.hover(a, b) && u.hover(ca, cb);
        const bool on = part == taken;
        cardBg(a, b, h, on);
        if (m_dev && P.connSrc) u.dl->AddRect(a, b, stConnCol(P.connSrc), 12 * s, 0, 2.0f * s);
        if (part == m_libFound) u.dl->AddRect(ImVec2(a.x - 2 * s, a.y - 2 * s), ImVec2(b.x + 2 * s, b.y + 2 * s), C_ACCENT, 13 * s, 0, 2.5f * s);   // (found from the marking menu)   // DEV: where its connectors come from
        ImTextureID tex;
        ImVec2 uv0, uv1;
        if (!sheet && thumbUv(part, tex, uv0, uv1)) {   // IMAGE: the image alone, zoomed as far as the part allows (Wzx 09/11/2026)
            const float z = 1.5f;
            const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / z) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / z) * 0.5f);
            u.dl->AddImageRounded(tex, ImVec2(a.x + 2 * s, a.y + 2 * s), ImVec2(b.x - 2 * s, b.y - 2 * s), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y),
                                  stA(IM_COL32(226, 229, 234, 255), alpha * (on ? 0.4f : 1.0f)), 10 * s);
        }
        if (sheet) {   // FICHE: the image, the dimension line, the name in tracked capitals, the id
            const float imgH = cardH - 44 * s;
            if (thumbUv(part, tex, uv0, uv1)) {
                const float z = 1.6f;
                const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / z) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / z) * 0.5f);
                const ImVec2 m((a.x + b.x) * 0.5f, a.y + 6 * s + imgH * 0.5f);
                const float half = imgH * 0.5f;
                u.dl->AddImage(tex, ImVec2(m.x - half * 1.25f, m.y - half), ImVec2(m.x + half * 1.25f, m.y + half), ImVec2(uv0.x + du.x * 0.8f, uv0.y + du.y * 1.25f), ImVec2(uv1.x - du.x * 0.8f, uv1.y - du.y * 1.25f),
                               stA(IM_COL32(222, 226, 232, 255), alpha * (on ? 0.4f : 1.0f)));
            }
            char dim[24];
            int tw = 0, tl = 0;
            const size_t xpos = P.label.find(" x ");
            if (xpos != std::string::npos) {
                size_t a0 = xpos;
                while (a0 > 0 && (std::isdigit((unsigned char)P.label[a0 - 1]) || P.label[a0 - 1] == ' ')) --a0;
                tw = std::atoi(P.label.substr(a0, xpos - a0).c_str());
                tl = std::atoi(P.label.substr(xpos + 3).c_str());
            }
            if (tw > 0 && tl > 0) snprintf(dim, sizeof(dim), "%d × %d", tw, tl); else snprintf(dim, sizeof(dim), "%s", catFr(P.category));
            const float dy = a.y + 6 * s + imgH + 6 * s, dw = u.textW(8, dim), mxc = (a.x + b.x) * 0.5f;
            u.dl->AddLine(ImVec2(a.x + 8 * s, dy), ImVec2(mxc - dw * 0.5f - 4 * s, dy), IM_COL32(255, 255, 255, 40), 1.0f);
            u.dl->AddLine(ImVec2(mxc + dw * 0.5f + 4 * s, dy), ImVec2(b.x - 8 * s, dy), IM_COL32(255, 255, 255, 40), 1.0f);
            u.text(ImVec2(mxc - dw * 0.5f, dy - u.fs(8) * 0.55f), 8, C_FAINT, dim);
            std::string nm = stUpper(partFr(P.label));
            u.dl->PushClipRect(ImVec2(a.x + 6 * s, a.y), ImVec2(b.x - 6 * s, b.y), true);
            stTrack(u, ImVec2(a.x + 8 * s, b.y - 26 * s), 8.5f, C_TEXT, nm.c_str(), 0.8f * s);
            std::string id = P.name;
            if (id.size() > 4 && id.substr(id.size() - 4) == ".dat") id = id.substr(0, id.size() - 4);
            u.text(ImVec2(a.x + 8 * s, b.y - 14 * s), 8, C_FAINT, id.c_str());
            u.dl->PopClipRect();
        }
        if (std::find(m_fav.begin(), m_fav.end(), part) != m_fav.end()) stPicto(u.dl, "star", 0, 0, ImVec2(b.x - 11 * s, a.y + 11 * s), 11 * s, C_ACCENT_TXT, 1.0f * s);
        if (k == m_opt.cardSheet && !h) { m_stHov = part; m_stHovT = now - 1; m_stHovA = a; m_stHovB = b; hovered = true; }   // (captures: --card-sheet N)
        if (h) {
            if (m_stHov != part) {   // a sheet shown a moment ago: the next card shows its own at once
                m_stHov = part;
                m_stHovT = now - m_stSheetSeen < 0.25 ? now - 0.6 : now;
                m_stHovA = a;
                m_stHovB = b;
            }
            hovered = true;
            if (u.pressed) { m_libFound = -1; m_libPress = part; m_libPressPos = u.mouse; m_libPressTime = now; m_libLong = false; }
            if (m_dev && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { m_ctxPart = part; m_ctxPos = u.mouse; }
        }
    }
    u.dl->PopClipRect();
    if (!hovered || m_libPress >= 0 || ImGui::IsMouseDown(ImGuiMouseButton_Left)) { if (!hovered) m_stHov = -1; }
    else if (m_stHov >= 0 && now - m_stHovT > 0.5) { stCardSheet(m_stHov, m_stHovA, m_stHovB, (float)(now - m_stHovT - 0.5)); m_stSheetSeen = now; }
    if (contentW > viewW) {   // a thin scroll bar
        const float bw = viewW * viewW / contentW, bx = ca.x + (viewW - bw) * (m_cardsScroll / (contentW - viewW));
        u.dl->AddRectFilled(ImVec2(bx, cb.y + 3 * s), ImVec2(bx + bw, cb.y + 5 * s), IM_COL32(255, 255, 255, 40), 2 * s);
    }
    stLibPressEnd(u, p0, p1);
}

// the sheet of a card held under the cursor for 0.5 s: the image big, the dimension line, the name, the id
void BrickApp::stCardSheet(int part, ImVec2 ca, ImVec2 cb, float t) {
    if (part < 0 || part >= (int)m_lib.parts.size()) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    u.dl = ImGui::GetForegroundDrawList();
    const float s = m_ui, e = std::clamp(t / 0.08f, 0.0f, 1.0f), ease = 1 - (1 - e) * (1 - e);
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const Part& P = m_lib.parts[part];
    const float W = 236 * s, H = 276 * s, pad = 12 * s;
    const float cx = std::clamp((ca.x + cb.x) * 0.5f - W * 0.5f, 10 * s, d.x - W - 10 * s);
    const ImVec2 a(cx, ca.y - H - 10 * s + (1 - ease) * 8 * s), b(a.x + W, a.y + H);
    u.dl->AddRectFilled(ImVec2(a.x, a.y + 10 * s), ImVec2(b.x, b.y + 14 * s), IM_COL32(0, 0, 0, (int)(70 * ease)), 18 * s);
    u.dl->AddRectFilled(a, b, stA(IM_COL32(24, 25, 28, 240), ease), 16 * s);
    u.dl->AddRect(a, b, stA(IM_COL32(255, 255, 255, 46), ease), 16 * s, 0, 1.0f);
    const ImVec2 ia(a.x + pad, a.y + pad), ib(b.x - pad, a.y + pad + 168 * s);
    u.dl->AddRectFilled(ia, ib, stA(IM_COL32(18, 19, 24, 255), ease), 12 * s);   // the ground of the cards
    u.dl->AddRect(ia, ib, stA(IM_COL32(255, 255, 255, 30), ease), 12 * s, 0, 1.0f);
    ImTextureID tex = 0;
    ImVec2 uv0, uv1;
    {   // the image at the size it is drawn (no aliasing), else the thumbnail of the cards
        const float side0 = std::min(ib.x - ia.x, ib.y - ia.y);
        const ImTextureID st = sheetTexture(part, (int)std::lround(side0 * 1.2f));
        if (st) { tex = st; uv0 = ImVec2(0, 0); uv1 = ImVec2(1, 1); }
    }
    if (tex || thumbUv(part, tex, uv0, uv1)) {
        const float z = 1.2f, side = std::min(ib.x - ia.x, ib.y - ia.y);
        const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / z) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / z) * 0.5f);
        const ImVec2 m((ia.x + ib.x) * 0.5f, (ia.y + ib.y) * 0.5f);
        u.dl->AddImage(tex, ImVec2(m.x - side * 0.5f, m.y - side * 0.5f), ImVec2(m.x + side * 0.5f, m.y + side * 0.5f), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), stA(IM_COL32(232, 235, 240, 255), ease));
    }
    float y = ib.y + 14 * s;
    {   // ——— 2 × 4 ———
        char dim[32];
        int tw = 0, tl = 0;
        const size_t xpos = P.label.find(" x ");
        if (xpos != std::string::npos) {
            size_t a0 = xpos;
            while (a0 > 0 && (std::isdigit((unsigned char)P.label[a0 - 1]) || P.label[a0 - 1] == ' ')) --a0;
            tw = std::atoi(P.label.substr(a0, xpos - a0).c_str());
            tl = std::atoi(P.label.substr(xpos + 3).c_str());
        }
        if (tw > 0 && tl > 0) snprintf(dim, sizeof(dim), "%d × %d", tw, tl); else snprintf(dim, sizeof(dim), "%s", catFr(P.category));
        const float dw = u.textW(9.5f, dim), mx = (a.x + b.x) * 0.5f;
        u.dl->AddLine(ImVec2(a.x + pad, y), ImVec2(mx - dw * 0.5f - 6 * s, y), stA(IM_COL32(255, 255, 255, 46), ease), 1.0f);
        u.dl->AddLine(ImVec2(mx + dw * 0.5f + 6 * s, y), ImVec2(b.x - pad, y), stA(IM_COL32(255, 255, 255, 46), ease), 1.0f);
        u.text(ImVec2(mx - dw * 0.5f, y - u.fs(9.5f) * 0.55f), 9.5f, stA(C_DIM, ease), dim);
    }
    y += 14 * s;
    {   // the name in tracked capitals (cut to the width), the id and the family
        std::string nm = stUpper(partFr(P.label));
        while (nm.size() > 4 && stTrackW(u, 11, nm.c_str(), 1.2f * s, true) > W - 2 * pad) { nm.pop_back(); while (!nm.empty() && ((unsigned char)nm.back() & 0xC0) == 0x80) nm.pop_back(); }
        stTrack(u, ImVec2(a.x + pad, y), 11, stA(C_TEXT, ease), nm.c_str(), 1.2f * s);
        y += 20 * s;
        std::string id = P.name;
        if (id.size() > 4 && id.substr(id.size() - 4) == ".dat") id = id.substr(0, id.size() - 4);
        const std::string meta = id + "  ·  " + catFr(P.category);
        u.text(ImVec2(a.x + pad, y), 9.5f, stA(C_DIM, ease), meta.c_str());
        y += 18 * s;
        u.text(ImVec2(a.x + pad, y), 9, stA(C_FAINT, ease), m_ipad ? "touche : en main  ·  glisse-la dans la vue" : "clic : en main  ·  glisse-la dans la vue");
    }
}

// ------------------------------------------------------------------ the selection: its tag, the marking menu

// the six actions of the ring, by direction (0° right, clockwise)
struct StAct { float ang; const char* label; const char* icon; bool danger; };
static const int ST_NACT = 5;
static const StAct kStActs[ST_NACT] = {{0, "TOURNER", ST_ROT, false}, {90, "DUPLIQUER", ST_DUP, false}, {135, "SUPPRIMER", ST_DEL, true},   // (Dupliquer straight down, Supprimer bottom left and smaller: Wzx)
                                 {180, "TOURNER", ST_ROTL, false}, {45, "BIBLIOTHÈQUE", ST_SEARCH, false}};   // (bottom right: the part found in the library, Wzx 10/11/2026)   // (no Basculer, no Fiche: Wzx 09/11/2026)
static const float ST_R_ACT = 100, ST_R_COL = 156,   // (the actions further out: Wzx 09/11/2026)
    ST_ARC0 = 204, ST_ARC1 = 336;

// the colours of the arc: the recent ones, then the usual ones
std::vector<int> BrickApp::stArcColors() const {
    std::vector<int> out;
    for (int c : m_recentColors) if ((int)out.size() < 10 && c >= 0 && c < (int)m_lib.palette.size() && std::find(out.begin(), out.end(), c) == out.end()) out.push_back(c);
    static const int usual[] = {4, 15, 14, 0, 1, 2, 71, 72, 25, 19};   // (LDraw ids: red, white, yellow, black, blue, green, greys, orange, tan)
    for (int id : usual) {
        if ((int)out.size() >= 10) break;
        for (int i = 0; i < (int)m_lib.palette.size(); ++i)
            if (m_lib.palette[i].id == id && std::find(out.begin(), out.end(), i) == out.end()) { out.push_back(i); break; }
    }
    return out;
}

void BrickApp::stSelHud() {
    const std::vector<int> sel = selection();
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    if (sel.empty() || m_mechOn || m_reading) m_stMenu = false;
    if (m_opt.stMenu && !sel.empty() && !m_stMenu && !m_stMenuShown && ImGui::GetFrameCount() > 8) {   // (captures: --st-menu, the menu open on the selection)
        const Box bx = m_build.aabbOf(m_bricks[sel[0]].part, m_bricks[sel[0]].rot, m_bricks[sel[0]].pos);
        const float3 c3 = (bx.mn + bx.mx) * 0.5f;
        float px, py;
        if (view().project(c3, px, py)) { m_stMenu = true; m_stMenuShown = true; m_stMenuAt = ImVec2(px, py); }
    }
    // ---- the marking menu (opened by editInput: a press held still on a brick)
    if (m_stMenu) {
        const ImVec2 c = m_stMenuAt;
        const float dx = u.mouse.x - c.x, dy = u.mouse.y - c.y, dist = std::sqrt(dx * dx + dy * dy);
        float ang = std::atan2(dy, dx) * 57.29578f;
        if (ang < 0) ang += 360;
        const std::vector<int> cols = stArcColors();
        int hot = -1;   // 0..5 an action, 100 + k a colour
        // quick mode (as Maya): in the first instants a short flick is enough, the nearest action within 45°
        const bool quick = ImGui::GetTime() - m_stMenuT0 < 0.35 && !m_stMenuSticky;
        // the colours: their band of the upper arc; elsewhere, past the middle, the nearest action, however far the
        // gesture goes (the line stops at the button, the button still acts: Wzx 09/11/2026)
        const bool inCols = dist >= (ST_R_ACT + 24) * s && dist < (ST_R_COL + 54) * s && ang > ST_ARC0 - 10 && ang < ST_ARC1 + 10 && !cols.empty();
        if (inCols) {
            const int k = (int)std::lround((ang - ST_ARC0) / (ST_ARC1 - ST_ARC0) * (cols.size() - 1));
            hot = 100 + std::clamp(k, 0, (int)cols.size() - 1);
        }
        else if (dist > (quick ? 12 : 30) * s) {   // the nearest action within 30° (quick: 45°)
            float bestA = quick ? 45.0f : 30.0f;
            for (int k = 0; k < ST_NACT; ++k) {
                float da = std::fabs(ang - kStActs[k].ang);
                da = std::min(da, 360 - da);
                if (da <= bestA) { bestA = da; hot = k; }
            }
        }
        // quick mode, a flick under way: not the whole menu, only the action aimed (Wzx 09/11/2026); the menu is drawn
        // into a list thrown away
        const bool flick = quick && dist > 12 * s;
        static ImDrawList* s_void = nullptr;
        if (!s_void) s_void = IM_NEW(ImDrawList)(ImGui::GetDrawListSharedData());
        s_void->_ResetForNewFrame();
        s_void->PushClipRectFullScreen();
        s_void->PushTexture(ImGui::GetIO().Fonts->TexRef);
        ImDrawList* dl = flick ? s_void : ImGui::GetForegroundDrawList();
        if (flick && hot >= 0 && hot < ST_NACT) {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            const StAct& A = kStActs[hot];
            const float r = A.ang * 0.0174533f, rad = 30 * s;
            const ImVec2 p(c.x + std::cos(r) * ST_R_ACT * s, c.y + std::sin(r) * ST_R_ACT * s);
            const ImU32 tint = A.danger ? IM_COL32(208, 16, 27, 255) : C_ACCENT;
            fg->AddCircleFilled(ImVec2(p.x, p.y + 3 * s), rad, IM_COL32(0, 0, 0, 80), 40);
            fg->AddCircleFilled(p, rad, IM_COL32(30, 32, 36, 235), 40);
            fg->AddCircleFilled(p, rad, stA(tint, 0.42f), 40);
            fg->AddCircle(p, rad, tint, 40, 1.5f * s);
            stIcon(fg, A.icon, p, 21 * s, IM_COL32_WHITE, 1.3f * s);
            const float lw = stTrackW(u, 8, A.label, 1.4f * s, true);
            ImDrawList* keep = u.dl;
            u.dl = fg;
            stTrack(u, ImVec2(p.x - lw * 0.5f, p.y + rad + 4 * s), 8, IM_COL32_WHITE, A.label, 1.4f * s);
            u.dl = keep;
        }
        // a dark halo, the guides, the gesture
        for (int k = 6; k >= 1; --k) dl->AddCircleFilled(c, 184 * s + k * 6 * s, IM_COL32(6, 7, 9, 10), 64);
        dl->AddCircleFilled(c, 184 * s, IM_COL32(6, 7, 9, 70), 64);
        dl->AddCircle(c, ST_R_ACT * s, IM_COL32(255, 255, 255, 30), 64, 1.0f);
        dl->AddCircle(c, 34 * s, IM_COL32(255, 255, 255, 50), 40, 1.0f);
        for (float a = ST_ARC0; a < ST_ARC1; a += 4) {   // the dotted guide of the arc
            const float r = a * 0.0174533f;
            dl->AddCircleFilled(ImVec2(c.x + std::cos(r) * ST_R_COL * s, c.y + std::sin(r) * ST_R_COL * s), 0.8f * s, stA(C_ACCENT, 0.4f), 6);
        }
        if (dist > 4 * s) {   // the dashed line of the gesture (stops at the button of the action aimed)
            const float lim = hot >= 0 && hot < ST_NACT ? std::min(dist, ST_R_ACT * s) : hot >= 100 ? std::min(dist, ST_R_COL * s) : dist;
            const float kq = lim / dist;
            const float dx = (u.mouse.x - c.x) * kq, dy = (u.mouse.y - c.y) * kq;
            const int n = (int)(lim / (6 * s));
            for (int i = 0; i < n; i += 2) {
                const float t0 = i / (float)n, t1 = std::min(1.0f, (i + 1) / (float)n);
                dl->AddLine(ImVec2(c.x + dx * t0, c.y + dy * t0), ImVec2(c.x + dx * t1, c.y + dy * t1), C_ACCENT, 1.3f * s);
            }
        }
        dl->AddCircleFilled(c, 3.5f * s, C_ACCENT, 12);
        {   // the part in the middle, faded when something is aimed
            std::string kd;
            int w, l;
            stPartKind(m_lib.parts[m_bricks[sel[0]].part], kd, w, l);
            stPicto(dl, kd, w, l, c, 40 * s, stA(C_TEXT, hot >= 0 ? 0.3f : 0.85f), 1.0f * s);
        }
        for (int k = 0; k < ST_NACT; ++k) {
            const StAct& A = kStActs[k];
            const bool on = hot == k;
            const float r = A.ang * 0.0174533f, rad = (A.danger ? (on ? 23 : 19) : (on ? 30 : 26)) * s;   // (Supprimer smaller)
            const ImVec2 p(c.x + std::cos(r) * ST_R_ACT * s, c.y + std::sin(r) * ST_R_ACT * s);
            const ImU32 tint = A.danger ? IM_COL32(208, 16, 27, 255) : C_ACCENT;
            dl->AddCircleFilled(ImVec2(p.x, p.y + 3 * s), rad, IM_COL32(0, 0, 0, 70), 40);
            dl->AddCircleFilled(p, rad, on ? stA(tint, 0.42f) : IM_COL32(44, 46, 50, 215), 40);
            dl->AddCircle(p, rad, on ? tint : IM_COL32(255, 255, 255, 70), 40, on ? 1.5f * s : 1.0f);
            stIcon(dl, A.icon, p, (A.danger ? 15 : 21) * s, on ? IM_COL32_WHITE : (A.danger ? IM_COL32(255, 154, 144, 255) : C_TEXT), 1.3f * s);
            const float lw = stTrackW(u, 8, A.label, 1.4f * s, true);
            const ImVec2 lp(p.x - lw * 0.5f, p.y + rad + 4 * s);
            ImDrawList* keep = u.dl;
            u.dl = dl;
            stTrack(u, lp, 8, on ? IM_COL32_WHITE : C_DIM, A.label, 1.4f * s);
            u.dl = keep;
        }
        const float tMenu = (float)(ImGui::GetTime() - m_stMenuT0);
        for (int k = 0; k < (int)cols.size(); ++k) {   // the colours on the arc: 1x1 bricks, out one by one (the chipPop of the web)
            const float a = (ST_ARC0 + (ST_ARC1 - ST_ARC0) * k / std::max(1, (int)cols.size() - 1)) * 0.0174533f;
            const bool on = hot == 100 + k;
            const float e = std::clamp((tMenu - 0.03f * k) / 0.28f, 0.0f, 1.0f);
            if (e <= 0) continue;
            const float pop = e < 0.6f ? 0.4f + 0.75f * (e / 0.6f) : 1.15f - 0.15f * ((e - 0.6f) / 0.4f);   // 0.4 > 1.15 > 1
            const float half = (on ? 21 : 14) * s * pop, rr = ST_R_COL * (0.72f + 0.28f * std::min(1.0f, e / 0.6f));
            const ImVec2 p(c.x + std::cos(a) * rr * s, c.y + std::sin(a) * rr * s);
            const Color& col = m_lib.palette[cols[k]];
            const ImU32 cc = IM_COL32((int)(std::clamp(col.srgb.x, 0.0f, 1.0f) * 255), (int)(std::clamp(col.srgb.y, 0.0f, 1.0f) * 255), (int)(std::clamp(col.srgb.z, 0.0f, 1.0f) * 255), col.group == "transparent" ? 170 : 255);
            dl->AddRectFilled(ImVec2(p.x - half + 2 * s, p.y - half + 3 * s), ImVec2(p.x + half + 2 * s, p.y + half + 3 * s), IM_COL32(0, 0, 0, 60), 4 * s);
            if (g_studTex) dl->AddImage(g_studTex, ImVec2(p.x - half, p.y - half), ImVec2(p.x + half, p.y + half), ImVec2(0, 0), ImVec2(1, 1), cc);
            else dl->AddRectFilled(ImVec2(p.x - half, p.y - half), ImVec2(p.x + half, p.y + half), cc, 4 * s);
            if (on) dl->AddRect(ImVec2(p.x - half - 3 * s, p.y - half - 3 * s), ImVec2(p.x + half + 3 * s, p.y + half + 3 * s), C_ACCENT, 5 * s, 0, 1.5f * s);
        }
        {   // the title above: what the release will do
            std::string t = "GLISSE VERS UNE ACTION";
            if (hot >= 0 && hot < ST_NACT) t = std::string(kStActs[hot].label) + (hot == 0 ? " À DROITE" : hot == 3 ? " À GAUCHE" : "");
            else if (hot >= 100) t = stUpper(colorFr(m_lib.palette[cols[hot - 100]].name));
            const float tw = stTrackW(u, 9.5f, t.c_str(), 1.8f * s, true);
            ImDrawList* keep = u.dl;
            u.dl = dl;
            stTrack(u, ImVec2(c.x - tw * 0.5f, c.y - 202 * s), 9.5f, C_ACCENT_TXT, t.c_str(), 1.8f * s);
            u.dl = keep;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { m_stMenu = false; return; }
        if (m_opt.stMenu) return;   // (captures: stays open)
        if (m_stMenuWait) {   // opened by a finger: its press reaches ImGui a frame later
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) return;
            m_stMenuWait = false;
        }
        const bool act = m_stMenuSticky ? (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                                        : !ImGui::IsMouseDown(m_stMenuBtn == 1 ? ImGuiMouseButton_Right : ImGuiMouseButton_Left);
        if (act) {   // released (or, opened by a quick right click, the next click): the action
            m_stMenu = false;
            if (m_stMenuSticky) m_stMenuEat = ImGui::GetFrameCount();
            m_stMenuSticky = false;
            if (hot == 0) rotateSelected(1);
            else if (hot == 3) rotateSelected(-1);
            else if (hot == 1) duplicateSelected();
            else if (hot == 2) deleteSelected();
            else if (hot == 4 && !sel.empty()) {   // the part in the library: searched by its id, its card ringed and shown
                const Part& P = m_lib.parts[m_bricks[sel[0]].part];
                std::string id = P.name;
                if (id.size() > 4 && id.substr(id.size() - 4) == ".dat") id = id.substr(0, id.size() - 4);
                // its family and its category opened and selected, the card shown there (Wzx 10/11/2026: where it comes
                // from); a part in no category: found by a search
                const int pi = m_bricks[sel[0]].part;
                bool placed = false;
                for (const StFam& f : stFams()) {
                    if (!strcmp(f.key, "recent") || !strcmp(f.key, "fav")) continue;
                    for (size_t q = 0; q < f.subs.size() && !placed; ++q)
                        if (stInSub(f.subs[q], partCat(pi), P.sub)) { m_stFam = f.key; m_stSub = (int)q; placed = true; }
                    if (placed) break;
                }
                m_stW = m_stL = 0;
                m_search = placed ? std::string() : id;
                m_searchOpen = !placed;
                m_cardsScroll = 0;
                m_libFound = pi;
                m_libFoundScroll = true;
                m_pTray.collapsed = false;
            }
            else if (hot >= 100) applyColor(cols[hot - 100]);
        }
        return;
    }
    // ---- the tag above the selection: its name, « maintenir le clic : menu »
    if (sel.empty() || m_holding || m_mechOn || m_reading || (m_pressed && m_dragStarted) || !m_selHud) return;
    float3 mn(1e30f), mx(-1e30f);
    for (int i : sel) {
        const Box b = m_build.aabbOf(m_bricks[i].part, m_bricks[i].rot, m_bricks[i].pos);
        mn = minv(mn, b.mn);
        mx = maxv(mx, b.mx);
    }
    const View v = view();
    float tx, ty;
    if (!v.project(float3((mn.x + mx.x) * 0.5f, mx.y, (mn.z + mx.z) * 0.5f), tx, ty)) return;
    ImVec2 a(1e30f, 1e30f), b(-1e30f, -1e30f);
    for (int k = 0; k < 8; ++k) {
        float x, y;
        if (!v.project(float3((k & 1) ? mx.x : mn.x, (k & 2) ? mx.y : mn.y, (k & 4) ? mx.z : mn.z), x, y)) return;
        a = ImVec2(std::min(a.x, x), std::min(a.y, y));
        b = ImVec2(std::max(b.x, x), std::max(b.y, y));
    }
    const float top = m_uiRects.empty() ? 46 * s : m_uiRects[0].w;
    if (b.x < 0 || a.x > d.x || b.y < top || a.y > d.y) return;
    // a group or a piece alone (Wzx 09/11/2026: « différencier un groupe d'une pièce seule »): a group shows a stack and
    // GROUPE · its name · its count, rimmed amber; a piece its colour and its name
    const int ug = unitGroup(m_selected);
    const bool isGroup = ug >= 0 && ug < (int)m_build.groups.size() && sel.size() > 1;
    std::string nm = isGroup ? stUpper(m_build.groups[ug].name) : sel.size() > 1 ? std::to_string(sel.size()) + " BRIQUES" : stUpper(partFr(m_lib.parts[m_bricks[sel[0]].part].label));
    if (nm.size() > 48) { nm = nm.substr(nm.size() - 44); nm = "…" + nm.substr(nm.find(' ') == std::string::npos ? 0 : nm.find(' ')); }
    char gl[32];
    snprintf(gl, sizeof(gl), "GROUPE · %d", (int)sel.size());
    const char* hint = m_ipad ? "appui long : menu" : "maintenir le clic : menu";
    const float wg = isGroup ? stTrackW(u, 8, gl, 1.4f * s, true) + 30 * s : 0;
    const float w1 = stTrackW(u, 9, nm.c_str(), 1.4f * s, true), w2 = u.textW(9, hint);
    const float W = 10 * s + wg + 8 * s + w1 + 10 * s + w2 + 10 * s, H = 24 * s;
    const ImVec2 ta(std::clamp((a.x + b.x) * 0.5f - W * 0.5f, 70 * s, d.x - W - 240 * s), std::max(top + 6 * s, a.y - H - 12 * s)), tb(ta.x + W, ta.y + H);
    u.dl->AddRectFilled(ta, tb, IM_COL32(12, 13, 15, 190), H * 0.5f);
    u.dl->AddRect(ta, tb, isGroup ? stA(C_ACCENT, 0.75f) : IM_COL32(255, 255, 255, 46), H * 0.5f, 0, 1.0f);
    if (isGroup) {   // the stack (two offset squares) and GROUPE · n, in amber
        const ImVec2 g0(ta.x + 10 * s, (ta.y + tb.y) * 0.5f - 5 * s);
        u.dl->AddRect(ImVec2(g0.x + 3 * s, g0.y - 2 * s), ImVec2(g0.x + 11 * s, g0.y + 6 * s), stA(C_ACCENT_TXT, 0.7f), 1.5f * s, 0, 1.0f);
        u.dl->AddRectFilled(ImVec2(g0.x, g0.y + 2 * s), ImVec2(g0.x + 8 * s, g0.y + 10 * s), stA(C_ACCENT, 0.9f), 1.5f * s);
        stTrack(u, ImVec2(ta.x + 26 * s, (ta.y + tb.y) * 0.5f - u.fs(8) * 0.52f), 8, C_ACCENT_TXT, gl, 1.4f * s);
        u.dl->AddLine(ImVec2(ta.x + wg + 2 * s, ta.y + 6 * s), ImVec2(ta.x + wg + 2 * s, tb.y - 6 * s), IM_COL32(255, 255, 255, 40), 1.0f);
    }
    const Color& cc = m_lib.palette[std::clamp(m_bricks[sel[0]].color, 0, (int)m_lib.palette.size() - 1)];
    const ImVec2 sq(ta.x + 10 * s + wg, (ta.y + tb.y) * 0.5f - 4 * s);
    if (!isGroup) u.dl->AddRectFilled(sq, ImVec2(sq.x + 8 * s, sq.y + 8 * s), IM_COL32((int)(cc.srgb.x * 255), (int)(cc.srgb.y * 255), (int)(cc.srgb.z * 255), 255), 2 * s);
    stTrack(u, ImVec2(sq.x + 14 * s, (ta.y + tb.y) * 0.5f - u.fs(9) * 0.52f), 9, C_TEXT, nm.c_str(), 1.4f * s);
    u.text(ImVec2(sq.x + 14 * s + w1 + 10 * s, (ta.y + tb.y) * 0.5f - u.fs(9) * 0.55f), 9, C_DIM, hint);
    m_uiRects.push_back(ImVec4(ta.x, ta.y, tb.x, tb.y));
}

// the part in hand: a pill in the top middle « EN MAIN · Brique 2 × 4 · clic pour poser, R pour tourner »
void BrickApp::stHeld() {
    if (!m_holding || m_libSel < 0 || m_libSel >= (int)m_lib.parts.size()) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float top = (m_uiRects.empty() ? 46 * s : m_uiRects[0].w) + 12 * s;
    const Part& P = m_lib.parts[m_libSel];
    const std::string t = partFr(P.label) + (m_ipad ? " · lève le doigt pour poser · 2e doigt pour tourner" : " · clic pour poser · R pour tourner · clic droit pour lâcher");
    const float tw = u.textW(10.5f, t.c_str()), lw = stTrackW(u, 9, "EN MAIN", 1.6f * s, true);
    const float W = 8 * s + 30 * s + 8 * s + lw + 10 * s + tw + 14 * s, H = 34 * s;
    const ImVec2 a((d.x - W) * 0.5f, top), b(a.x + W, top + H);
    u.dl->AddRectFilled(a, b, IM_COL32(16, 17, 19, 200), H * 0.5f);
    u.dl->AddRect(a, b, stA(C_ACCENT, 0.6f), H * 0.5f, 0, 1.0f);
    std::string kd;
    int w, l;
    stPartKind(P, kd, w, l);
    stPicto(u.dl, kd, w, l, ImVec2(a.x + 8 * s + 15 * s, (a.y + b.y) * 0.5f), 24 * s, C_ACCENT_TXT, 1.0f * s);
    stTrack(u, ImVec2(a.x + 46 * s, (a.y + b.y) * 0.5f - u.fs(9) * 0.52f), 9, C_ACCENT_TXT, "EN MAIN", 1.6f * s);
    u.text(ImVec2(a.x + 46 * s + lw + 10 * s, (a.y + b.y) * 0.5f - u.fs(10.5f) * 0.55f), 10.5f, C_TEXT, t.c_str());
}

// ------------------------------------------------------------------ the whole interface

void BrickApp::drawUiStudio() {
    if (m_vePart >= 0) {   // the Voxel Editor: the whole screen (nothing of the Studio under it)
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        m_uiRects.push_back(ImVec4(0, 0, d.x, 0));
        m_uiRects.push_back(ImVec4(0, d.y, d.x, d.y));
        uiVoxelEditor();
        return;
    }
    stHeader();      // m_uiRects[0]
    stDrawer();      // m_uiRects[1]
    if (m_focusReticle && m_focusMode == 0 && m_rs.dof > 0 && !m_opt.noUi) {
        const ImVec2 c((m_stage.x + m_stage.z) * 0.5f, (m_stage.y + m_stage.w) * 0.5f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddCircle(c, 9 * m_ui, IM_COL32(255, 255, 255, 120), 32, 1.0f * m_ui);
    }
    stTools();
    stGroups();      // (before the badge: it goes right of it)
    stBadge();
    stColors();
    stSelHud();
    stRing();
    stPaintFx();
    stHeld();
    uiMechPanel();
    uiReader();
    stMore();
    stFile();
    uiPopups();
    uiManual();
    uiDevPanel();
    stConnLegend();
    uiInfoSheet();   // (no Bac à briques in the Studio interface, Wzx 09/11/2026)
    uiThumbEditor();
    uiVoxelEditor();
    uiCtxMenu();
    uiDevReview();
    uiManualBig();
    drawDropzone();
}

// ------------------------------------------------------------------ the ring of the left hand (Wzx 10/11/2026)
// Studio: on the left of the view, the ring of the web: tilt up / down, turn left / right, in the middle Dupliquer
// (Poser while a brick is in hand). On the iPad a finger of the left hand presses it while the Pencil holds the brick:
// those touches are caught before the gestures (BrickTouch.cpp, stRingHit)

int BrickApp::stRingHit(float x, float y) const {
    if (m_ringR <= 0) return -1;
    const float dx = x - m_ringC.x, dy = y - m_ringC.y, d = std::sqrt(dx * dx + dy * dy);
    const float rin = m_ringR * 0.42f;
    if (d > m_ringR * 1.06f) return -1;
    if (d <= rin) return 4;
    const float a = std::atan2(dy, dx) * 57.29578f;   // 0 right, 90 down
    if (a >= -45 && a < 45) return 1;
    if (a >= 45 && a < 135) return 2;
    if (a >= -135 && a < -45) return 0;
    return 3;
}

void BrickApp::stRingAct(int part) {
    const bool active = m_holding || m_selected >= 0 || !m_multi.empty();
    if (!active || part < 0) return;
    m_ringFlash = part;
    m_ringFlashT = ImGui::GetTime();
    switch (part) {
    case 0: tiltSelected(-1); break;
    case 2: tiltSelected(1); break;
    case 1: rotateSelected(-1); break;
    case 3: rotateSelected(1); break;
    case 4: if (m_holding) dropHeld(); else duplicateSelected(); break;
    }
}

void BrickApp::stRing() {
    m_ringR = 0;
    if (!m_touchHud || m_mechOn || m_reading || m_paintMode || m_vePart >= 0 || m_opt.noUi) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const float R = 72 * s, rin = R * 0.42f;
    const ImVec2 c(m_dockL + 22 * s + R, std::floor((m_stage.y + m_stage.w) * 0.5f + 10 * s));
    m_ringC = c;
    m_ringR = R;
    const bool active = m_holding || m_selected >= 0 || !m_multi.empty();
    const float al = active ? 1.0f : 0.4f;
    const double now = ImGui::GetTime();
    const int hov = (!m_ipad && u.hover(ImVec2(c.x - R, c.y - R), ImVec2(c.x + R, c.y + R))) ? stRingHit(u.mouse.x, u.mouse.y) : -1;
    ImDrawList* dl = u.dl;
    // the glass disc, then the four wedges with their gaps
    stGlass(dl, ImVec2(c.x - R - 4 * s, c.y - R - 4 * s), ImVec2(c.x + R + 4 * s, c.y + R + 4 * s), R + 4 * s, s);
    m_uiRects.push_back(ImVec4(c.x - R - 4 * s, c.y - R - 4 * s, c.x + R + 4 * s, c.y + R + 4 * s));
    static const char* kIco[4] = {"M7 14l5-5 5 5", ST_ROT, "M7 10l5 5 5-5", ST_ROTL};
    static const char* kTip[4] = {"Basculer vers le haut", "Tourner à droite 90°", "Basculer vers le bas", "Tourner à gauche 90°"};
    for (int i = 0; i < 4; ++i) {
        const float a = (i - 1) * 1.5707963f;   // 0 up (-90°), 1 right, 2 down, 3 left
        const float half = 0.70f;
        const float flash = m_ringFlash == i ? std::max(0.0f, 1.0f - (float)(now - m_ringFlashT) / 0.35f) : 0.0f;
        const bool on = active && (hov == i || flash > 0);
        dl->PathArcTo(c, R - 3 * s, a - half, a + half, 18);
        dl->PathArcTo(c, rin + 5 * s, a + half, a - half, 10);
        dl->PathFillConvex(on ? stA(C_ACCENT, 0.25f + 0.45f * flash) : stA(IM_COL32(255, 255, 255, 255), 0.06f * al));
        dl->PathArcTo(c, R - 3 * s, a - half, a + half, 18);
        dl->PathArcTo(c, rin + 5 * s, a + half, a - half, 10);
        dl->PathStroke(on ? stA(C_ACCENT, 0.9f) : stA(C_LINE, al), ImDrawFlags_Closed, 1.0f);
        const float rm = (R - 3 * s + rin + 5 * s) * 0.5f;
        const ImVec2 ic(c.x + std::cos(a) * rm, c.y + std::sin(a) * rm);
        stIcon(dl, kIco[i], ic, 20 * s, stA(on ? C_ACCENT_TXT : C_TEXT, al), 1.3f * s);
        if (hov == i && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", kTip[i]);
    }
    // the middle: Dupliquer, or Poser with a brick in hand (the accent)
    const float flash = m_ringFlash == 4 ? std::max(0.0f, 1.0f - (float)(now - m_ringFlashT) / 0.35f) : 0.0f;
    const bool onC = active && (hov == 4 || flash > 0);
    if (m_holding) dl->AddCircleFilled(c, rin, C_ACCENT, 40);
    else dl->AddCircleFilled(c, rin, onC ? stA(C_ACCENT, 0.3f + 0.4f * flash) : stA(IM_COL32(255, 255, 255, 255), 0.08f * al), 40);
    dl->AddCircle(c, rin, stA(C_ACCENT, 0.8f * al), 40, 1.2f * s);
    const char* lab = m_holding ? "POSER" : "DUPLIQUER";
    if (!m_holding) stIcon(dl, ST_DUP, ImVec2(c.x, c.y - 5 * s), 16 * s, stA(onC ? C_ACCENT_TXT : C_TEXT, al), 1.2f * s);
    const float lw = stTrackW(u, m_holding ? 8.5f : 6.5f, lab, 1.0f * s, true);
    stTrack(u, ImVec2(c.x - lw * 0.5f, m_holding ? c.y - u.fs(8.5f) * 0.52f : c.y + 8 * s), m_holding ? 8.5f : 6.5f, m_holding ? C_INK : stA(C_DIM, al), lab, 1.0f * s);
    if (hov == 4 && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", m_holding ? "Poser la brique" : "Dupliquer la sélection");
    if (!m_ipad && hov >= 0 && u.pressed) stRingAct(hov);   // (iPad: the touches, BrickTouch.cpp)
}
