// BrickIt - the Lighting and Camera panels (Wzx 24/11/2026; the lights and cameras themselves: BrickLights.cpp).
// Included by BrickUi.cpp after BrickUiStudio.inl (its helpers: stGlass, stTrack, stIcon, the colours).

static const char* ST_ALIGHT = "M5 10h14v5H5zM12 3v4M6 5l1.5 2.5M18 5l-1.5 2.5M8 20l1-3M16 20l-1-3M12 21v-4";
static const char* ST_CAM = "M3 8h12v9H3zM15 11l6-3v9l-6-3z";
static const char* ST_LOCK = "M7 11h10v9H7zM9 11V8a3 3 0 0 1 6 0v3";
static const char* ST_ENTER = "M4 12h10M10 8l4 4-4 4M14 4h6v16h-6";
static const char* ST_TRASH = "M5 7h14M10 7V4h4v3M7 7l1 13h8l1-13";
static ImVec2 g_lcAnchor(0, 0);   // (the right edge and top of the panels: left of the navigation buttons)
static float g_lcLightLeft = 0;   // (the left edge of the Lighting panel: the Camera panel beside it when it does not fit under)

// a slider of the panels: its label, its value, the track; true when it moved
static bool lcSlider(Ui& u, int id, int& drag, const char* label, float& v, float lo, float hi, const char* fmt, float shown, float x0, float x1, float& y, float s) {
    const float rowH = 22 * s, labW = 78 * s, vw = 44 * s;
    u.text(ImVec2(x0, y + (rowH - u.fs(10)) * 0.5f), 10, C_DIM, label);
    char vb[32];
    snprintf(vb, sizeof(vb), fmt, shown);
    u.text(ImVec2(x1 - u.textW(9.5f, vb), y + (rowH - u.fs(9.5f)) * 0.5f), 9.5f, C_TEXT, vb);
    const ImVec2 ta(x0 + labW, y + rowH * 0.5f - 3 * s), tb(x1 - vw - 6 * s, y + rowH * 0.5f + 3 * s);
    const bool h = u.hover(ImVec2(ta.x - 6 * s, y), ImVec2(tb.x + 6 * s, y + rowH));
    if (h && u.pressed) drag = id;
    bool changed = false;
    if (drag == id) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float t = std::clamp((u.mouse.x - ta.x) / std::max(1.0f, tb.x - ta.x), 0.0f, 1.0f), nv = lo + t * (hi - lo);
            if (std::fabs(nv - v) > 1e-5f * (hi - lo)) { v = nv; changed = true; }
        } else drag = -1;
    }
    const float t = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
    u.dl->AddRectFilled(ta, tb, stW(22), 3 * s);
    u.dl->AddRectFilled(ta, ImVec2(ta.x + (tb.x - ta.x) * t, tb.y), C_PROGRESS, 3 * s);
    const ImVec2 k(ta.x + (tb.x - ta.x) * t, (ta.y + tb.y) * 0.5f);
    u.dl->AddCircleFilled(k, (h || drag == id ? 7.5f : 6.5f) * s, IM_COL32(255, 255, 255, 255), 20);
    y += rowH;
    return changed;
}

// a colour as hue / saturation / brightness tracks (ids id..id+2); true when it changed
static bool lcColour(Ui& u, int id, int& drag, float3& srgb, float x0, float x1, float& y, float s) {
    static float hueKeep = 0;
    float hh, ss, vv;
    ImGui::ColorConvertRGBtoHSV(srgb.x, srgb.y, srgb.z, hh, ss, vv);
    if (ss > 0.001f && vv > 0.001f) hueKeep = hh; else hh = hueKeep;
    auto col = [](float r, float g, float b) { return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255); };
    {
        const ImVec2 p0(x0, y + 2 * s);
        u.dl->AddRectFilled(p0, ImVec2(p0.x + 16 * s, p0.y + 16 * s), col(srgb.x, srgb.y, srgb.z), 4 * s);
        u.dl->AddRect(p0, ImVec2(p0.x + 16 * s, p0.y + 16 * s), stW(80), 4 * s, 0, 1.0f);
        u.text(ImVec2(p0.x + 22 * s, p0.y + 1 * s), 10, C_TEXT, "Colour");
        char hx[16];
        snprintf(hx, sizeof(hx), "#%02X%02X%02X", (int)(srgb.x * 255 + 0.5f), (int)(srgb.y * 255 + 0.5f), (int)(srgb.z * 255 + 0.5f));
        u.text(ImVec2(x1 - u.textW(9.5f, hx), p0.y + 2 * s), 9.5f, C_DIM, hx);
        y += 22 * s;
    }
    bool changed = false;
    const float rowH = 22 * s, labW = 78 * s, vw = 44 * s;
    auto track = [&](int k, const char* label, float& v, const std::function<void(ImVec2, ImVec2)>& fill) {
        u.text(ImVec2(x0 + 8 * s, y + (rowH - u.fs(10)) * 0.5f), 10, C_DIM, label);
        const ImVec2 ta(x0 + labW, y + rowH * 0.5f - 3 * s), tb(x1 - vw - 6 * s, y + rowH * 0.5f + 3 * s);
        const bool h = u.hover(ImVec2(ta.x - 6 * s, y), ImVec2(tb.x + 6 * s, y + rowH));
        if (h && u.pressed) drag = id + k;
        if (drag == id + k) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                const float nv = std::clamp((u.mouse.x - ta.x) / std::max(1.0f, tb.x - ta.x), 0.0f, 1.0f);
                if (std::fabs(nv - v) > 1e-4f) { v = nv; changed = true; }
            } else drag = -1;
        }
        fill(ta, tb);
        const ImVec2 kk(ta.x + (tb.x - ta.x) * v, (ta.y + tb.y) * 0.5f);
        u.dl->AddCircleFilled(kk, (h || drag == id + k ? 7.5f : 6.5f) * s, IM_COL32(255, 255, 255, 255), 20);
        u.dl->AddCircle(kk, (h || drag == id + k ? 7.5f : 6.5f) * s, IM_COL32(0, 0, 0, 90), 20, 1.0f);
        y += rowH;
    };
    track(0, "Hue", hh, [&](ImVec2 ta, ImVec2 tb) {
        const float w = (tb.x - ta.x) / 6;
        for (int k = 0; k < 6; ++k) {
            float r0, g0, b0, r1, g1, b1;
            ImGui::ColorConvertHSVtoRGB(k / 6.0f, 1, 1, r0, g0, b0);
            ImGui::ColorConvertHSVtoRGB((k + 1) / 6.0f, 1, 1, r1, g1, b1);
            u.dl->AddRectFilledMultiColor(ImVec2(ta.x + k * w, ta.y), ImVec2(ta.x + (k + 1) * w + 0.5f, tb.y), col(r0, g0, b0), col(r1, g1, b1), col(r1, g1, b1), col(r0, g0, b0));
        }
    });
    track(1, "Saturation", ss, [&](ImVec2 ta, ImVec2 tb) {
        float r1, g1, b1;
        const float vb = std::max(vv, 0.25f);
        ImGui::ColorConvertHSVtoRGB(hh, 1, vb, r1, g1, b1);
        u.dl->AddRectFilledMultiColor(ta, tb, col(vb, vb, vb), col(r1, g1, b1), col(r1, g1, b1), col(vb, vb, vb));
    });
    track(2, "Brightness", vv, [&](ImVec2 ta, ImVec2 tb) {
        float r1, g1, b1;
        ImGui::ColorConvertHSVtoRGB(hh, ss, 1, r1, g1, b1);
        u.dl->AddRectFilledMultiColor(ta, tb, IM_COL32(0, 0, 0, 255), col(r1, g1, b1), col(r1, g1, b1), IM_COL32(0, 0, 0, 255));
    });
    if (changed) { hueKeep = hh; ImGui::ColorConvertHSVtoRGB(hh, ss, vv, srgb.x, srgb.y, srgb.z); }
    return changed;
}

// a pill button (icon and text); true when pressed
static bool lcPill(Ui& u, ImVec2 a, ImVec2 b, const char* icon, const char* text, bool on, float s) {
    const bool h = u.hover(a, b);
    u.dl->AddRectFilled(a, b, on ? C_ACCENT : (h ? stW(30) : stW(12)), (b.y - a.y) * 0.5f);
    u.dl->AddRect(a, b, on ? C_ACCENT : stW(40), (b.y - a.y) * 0.5f, 0, 1.0f);
    float x = a.x + 10 * s;
    if (icon) { stIcon(u.dl, icon, ImVec2(x + 8 * s, (a.y + b.y) * 0.5f), 16 * s, on ? C_INK : C_TEXT, 1.4f * s); x += 20 * s; }
    const float tw = u.textW(9.5f, text, true);
    if (!icon) x = (a.x + b.x - tw) * 0.5f;
    u.text(ImVec2(x, (a.y + b.y) * 0.5f - u.fs(9.5f) * 0.55f), 9.5f, on ? C_INK : C_TEXT, text, true);
    return h && u.pressed;
}

// the two buttons under the navigation: Lighting, Cameras (their panels open / closed)
void BrickApp::stLcButtons(Ui& u, ImVec2 a, float bw, float s) {
    const float pad = 6 * s;
    const ImVec2 ga(a.x, a.y), gb(a.x + bw + 2 * pad, a.y + 2 * bw + 2 * pad);
    stGlass(u.dl, ga, gb, 14 * s, s);
    m_uiRects.push_back(ImVec4(ga.x, ga.y, gb.x, gb.y));
    struct B { const char* ico; const char* tip; bool* on; };
    const B bs[2] = {{ST_ALIGHT, "Lighting: area lights", &m_lightPanel}, {ST_CAM, "Cameras", &m_camPanel}};
    float y = ga.y + pad;
    for (const B& t : bs) {
        const ImVec2 ba(ga.x + pad, y), bb(ba.x + bw, y + bw);
        const bool h = u.hover(ba, bb), on = *t.on;
        if (on && g_brique) bqYellow(u.dl, ba, bb, 12 * s, s, h);
        else if (on) u.dl->AddRectFilled(ba, bb, stW(34), 10 * s);
        else if (h) u.dl->AddRectFilled(ba, bb, stW(16), 10 * s);
        stIcon(u.dl, t.ico, ImVec2((ba.x + bb.x) * 0.5f, (ba.y + bb.y) * 0.5f), 22 * s, on && g_brique ? C_INK : C_TEXT, (g_brique ? 1.9f : 1.5f) * s);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", t.tip);
        if (h && u.pressed) *t.on = !*t.on;
        y += bw;
    }
    g_lcAnchor = ImVec2(ga.x - 10 * s, m_stage.y + 14 * s);
}

// the list of a panel: rows (a toggle on the left, the name), selection; double click: rename; right click: a menu.
// kind 0 lights, 1 cameras; returns the bottom of the list
struct LcList {
    int kind = 0, n = 0;
    std::function<std::string(int)> nameOf;
    std::function<bool(int)> toggleOn;
    std::function<void(int)> toggle;
    std::function<void(int, const std::string&)> setName;
    std::function<bool(int)> marked;
    const char* toggleIco = nullptr;
};
static float lcList(Ui& u, const LcList& L, int& sel, int& renameIdx, int& renameKind, int& menuIdx, int& menuKind, ImVec2& menuAt, char* renameBuf, size_t renameN,
                    float x0, float x1, float y, float s) {
    const float rowH = 26 * s;
    if (L.n == 0) {
        u.text(ImVec2(x0, y + 4 * s), 9.5f, C_FAINT, L.kind == 0 ? "No light yet: create one above" : "No camera yet: create one above");
        return y + 24 * s;
    }
    for (int i = 0; i < L.n; ++i) {
        const ImVec2 ra(x0 - 6 * s, y), rb(x1 + 6 * s, y + rowH);
        const bool h = u.hover(ra, rb), on = i == sel;
        if (on) u.dl->AddRectFilled(ra, rb, stA(C_ACCENT, 0.22f), 7 * s);
        else if (h) u.dl->AddRectFilled(ra, rb, stW(12), 7 * s);
        const ImVec2 tc(x0 + 9 * s, y + rowH * 0.5f);   // the toggle (light: visible; camera: locked)
        const bool th = u.hover(ImVec2(tc.x - 10 * s, y), ImVec2(tc.x + 10 * s, y + rowH));
        const bool tOn = L.toggleOn(i);
        stIcon(u.dl, L.toggleIco, tc, 15 * s, tOn ? (th ? C_TEXT : C_DIM) : (th ? C_DIM : C_FAINT), 1.3f * s);
        if (L.kind == 0 && !tOn) u.dl->AddLine(ImVec2(tc.x - 7 * s, tc.y + 6 * s), ImVec2(tc.x + 7 * s, tc.y - 6 * s), C_FAINT, 1.3f * s);
        const std::string nm = L.nameOf(i);
        u.text(ImVec2(x0 + 26 * s, y + (rowH - u.fs(10.5f)) * 0.5f), 10.5f, on ? C_TEXT : C_DIM, nm.c_str(), on);
        if (L.marked && L.marked(i)) {   // (a camera looked through)
            const char* t = "VIEW";
            const float tw = u.textW(8, t, true);
            const ImVec2 pa(x1 - tw - 10 * s, y + 6 * s), pb(x1, y + rowH - 6 * s);
            u.dl->AddRectFilled(pa, pb, C_ACCENT, 4 * s);
            u.text(ImVec2(pa.x + 5 * s, pa.y + (pb.y - pa.y - u.fs(8)) * 0.5f), 8, C_INK, t, true);
        }
        if (th && u.pressed) L.toggle(i);
        else if (h && u.pressed) {
            sel = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { renameIdx = i; renameKind = L.kind; snprintf(renameBuf, renameN, "%s", nm.c_str()); }
        }
        if (h && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { sel = i; menuIdx = i; menuKind = L.kind; menuAt = u.mouse; }
        if (renameIdx == i && renameKind == L.kind) {   // the text field over the row: Entrée keeps it, Échap cancels
            ImGui::SetNextWindowPos(ImVec2(x0 + 22 * s, y + 2 * s));
            ImGui::SetNextWindowSize(ImVec2(x1 - x0 - 22 * s, rowH - 4 * s));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2 * s, 1 * s));
            ImGui::Begin(L.kind == 0 ? "##renamelight" : "##renamecam", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##name", renameBuf, renameN, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                if (renameBuf[0]) L.setName(i, renameBuf);
                renameIdx = -1;
            } else if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (!ImGui::IsWindowFocused() && !ImGui::IsWindowAppearing())) renameIdx = -1;
            ImGui::End();
            ImGui::PopStyleVar();
        }
        y += rowH;
    }
    return y;
}

// the right click menu of a row: (a camera: Look through), Rename, Delete; 0 look through, 1 rename, 2 delete,
// -2 closed by a click elsewhere, -1 nothing yet
static int lcMenu(Ui& u, ImVec2 at, bool camera, float s, std::vector<ImVec4>& rects) {
    std::vector<const char*> items;
    if (camera) items.push_back("Look through");
    items.push_back("Rename");
    items.push_back("Delete");
    const float w = 150 * s, ih = 26 * s;
    const ImVec2 a(at.x, at.y), b(at.x + w, at.y + items.size() * ih + 8 * s);
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    fg->AddRectFilled(ImVec2(a.x + 2 * s, a.y + 4 * s), ImVec2(b.x + 2 * s, b.y + 4 * s), IM_COL32(0, 0, 0, 60), 9 * s);
    fg->AddRectFilled(a, b, stD(IM_COL32(24, 25, 29, 248)), 9 * s);
    fg->AddRect(a, b, stW(40), 9 * s, 0, 1.0f);
    rects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    int hit = -1;
    for (size_t k = 0; k < items.size(); ++k) {
        const ImVec2 ia(a.x + 4 * s, a.y + 4 * s + k * ih), ib(b.x - 4 * s, ia.y + ih);
        const bool h = u.hover(ia, ib);
        if (h) fg->AddRectFilled(ia, ib, stW(22), 6 * s);
        fg->AddText(u.font, u.fs(10.5f), ImVec2(ia.x + 10 * s, ia.y + (ih - u.fs(10.5f)) * 0.5f), std::string(items[k]) == "Delete" ? IM_COL32(255, 120, 110, 255) : C_TEXT, items[k]);
        if (h && u.pressed) hit = camera ? (int)k : (int)k + 1;
    }
    if (hit < 0 && (u.pressed || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) && !u.hover(a, b)) hit = -2;
    return hit;
}

static void lcClose(Ui& u, ImVec2 c, bool& open, float s) {
    const bool h = u.hover(ImVec2(c.x - 9 * s, c.y - 9 * s), ImVec2(c.x + 9 * s, c.y + 9 * s));
    u.dl->AddLine(ImVec2(c.x - 4.5f * s, c.y - 4.5f * s), ImVec2(c.x + 4.5f * s, c.y + 4.5f * s), h ? C_TEXT : C_DIM, 1.5f * s);
    u.dl->AddLine(ImVec2(c.x - 4.5f * s, c.y + 4.5f * s), ImVec2(c.x + 4.5f * s, c.y - 4.5f * s), h ? C_TEXT : C_DIM, 1.5f * s);
    if (h && u.pressed) open = false;
}

void BrickApp::stLightPanel() {
    if (!m_lightPanel || m_opt.noUi || m_mechOn || m_vePart >= 0 || m_teditPart >= 0 || m_popup == 8) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui, W = 290 * s, pad = 14 * s;
    const float x1p = g_lcAnchor.x > 0 ? g_lcAnchor.x : m_stage.z - 90 * s, top = g_lcAnchor.y > 0 ? g_lcAnchor.y : m_stage.y + 14 * s;
    const bool has = m_selLight >= 0 && m_selLight < (int)m_lights.size();
    const int rows = std::max(1, (int)m_lights.size());
    const float H = 40 * s + 40 * s + rows * 26 * s + 10 * s + (has ? 20 * s + 4 * 22 * s + 4 * s + 4 * 22 * s + 6 * s + 30 * s + 26 * s : 0) + pad;
    const ImVec2 a(x1p - W, top), b(x1p, top + H);
    stGlass(u.dl, a, b, 16 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    stTrack(u, ImVec2(a.x + pad, y), 10, C_TEXT, "LIGHTING", 2.2f * s);
    lcClose(u, ImVec2(b.x - pad - 6 * s, y + 6 * s), m_lightPanel, s);
    y += 26 * s;
    // the types of light that can be created (an area light for now)
    const ImVec2 ca(a.x + pad, y), cb(a.x + pad + 124 * s, y + 28 * s);
    if (u.hover(ca, cb) && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Create an area light, aimed at the model");
    if (lcPill(u, ca, cb, ST_ALIGHT, "Area light", false, s)) { addLight(); m_lightPanel = true; }
    y += 40 * s;
    LcList L;
    L.kind = 0;
    L.n = (int)m_lights.size();
    L.nameOf = [&](int i) { return m_lights[i].name; };
    L.toggleOn = [&](int i) { return m_lights[i].visible; };
    L.toggle = [&](int i) { m_lights[i].visible = !m_lights[i].visible; syncLights(); };
    L.setName = [&](int i, const std::string& n) { m_lights[i].name = n; };
    L.toggleIco = ST_EYE;
    int sel = m_selLight;
    y = lcList(u, L, sel, m_lcRename, m_lcRenameKind, m_lcMenu, m_lcMenuKind, m_lcMenuAt, m_renameBuf, sizeof(m_renameBuf), a.x + pad, b.x - pad, y, s);
    if (sel != m_selLight) { m_selLight = sel; if (sel >= 0) m_selCam = -1; }
    y += 10 * s;
    if (m_selLight >= 0 && m_selLight < (int)m_lights.size()) {
        UserLight& l = m_lights[m_selLight];
        const float x0 = a.x + pad, x1 = b.x - pad;
        u.dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), stW(26), 1.0f);
        y += 8 * s;
        stTrack(u, ImVec2(x0, y), 8.5f, C_DIM, "AREA LIGHT", 1.8f * s);
        y += 12 * s;
        bool ch = false;
        ch |= lcSlider(u, 1, m_lcDrag, "Intensity", l.intensity, 0, std::max(200.0f, std::ceil(l.intensity / 100) * 100), "%.0f", l.intensity, x0, x1, y, s);
        ch |= lcSlider(u, 2, m_lcDrag, "Width", l.w, 0.5f, std::max(100.0f, l.w), "%.1f cm", l.w, x0, x1, y, s);
        ch |= lcSlider(u, 3, m_lcDrag, "Height", l.h, 0.5f, std::max(100.0f, l.h), "%.1f cm", l.h, x0, x1, y, s);
        {   // both edges together (a square stays a square)
            float side = std::sqrt(l.w * l.h);
            const float side0 = side;
            if (lcSlider(u, 4, m_lcDrag, "Size", side, 0.5f, std::max(100.0f, side), "%.1f cm", side, x0, x1, y, s) && side0 > 0) {
                const float k = side / side0;
                l.w *= k;
                l.h *= k;
                ch = true;
            }
        }
        y += 4 * s;
        ch |= lcColour(u, 10, m_lcDrag, l.color, x0, x1, y, s);
        y += 6 * s;
        {   // Target: on / off
            u.text(ImVec2(x0, y + 6 * s), 10, C_DIM, "Target");
            const float bw2 = 54 * s;
            if (lcPill(u, ImVec2(x1 - 2 * bw2 - 4 * s, y), ImVec2(x1 - bw2 - 4 * s, y + 24 * s), nullptr, "On", l.targetOn, s) && !l.targetOn) {
                l.targetOn = true;   // (the target where it shone, at the model's distance)
                float3 mn(1e30f), mx(-1e30f);
                for (const Brick& br : m_bricks) { const Part& p = m_lib.parts[br.part]; if (p.loaded) { mn = minv(mn, br.pos + p.bmin); mx = maxv(mx, br.pos + p.bmax); } }
                const float dist = mn.x <= mx.x ? length((mn + mx) * 0.5f - l.pos) : 20.0f;
                l.target = l.pos + l.dir() * dist;
                ch = true;
            }
            if (lcPill(u, ImVec2(x1 - bw2, y), ImVec2(x1, y + 24 * s), nullptr, "Off", !l.targetOn, s) && l.targetOn) {
                l.targetOn = false;   // (it keeps shining where the target was)
                const float3 d = normalize(l.target - l.pos);
                l.yaw = std::atan2(d.x, d.z);
                l.pitch = std::asin(std::clamp(d.y, -1.0f, 1.0f));
                ch = true;
            }
            y += 30 * s;
        }
        if (lcPill(u, ImVec2(x0, y), ImVec2(x0 + 96 * s, y + 26 * s), ST_TRASH, "Delete", false, s)) deleteLight(m_selLight);
        u.text(ImVec2(x0 + 106 * s, y + 7 * s), 9, C_FAINT, "View: arrows, rings, edges");
        if (ch) syncLights();
    }
    if (m_lcMenu >= 0 && m_lcMenuKind == 0) {
        const int k = lcMenu(u, m_lcMenuAt, false, s, m_uiRects);
        if (k == 1 && m_lcMenu < (int)m_lights.size()) { m_lcRename = m_lcMenu; m_lcRenameKind = 0; snprintf(m_renameBuf, sizeof(m_renameBuf), "%s", m_lights[m_lcMenu].name.c_str()); }
        if (k == 2) deleteLight(m_lcMenu);
        if (k != -1) m_lcMenu = -1;
    }
    g_lcAnchor.y = b.y + 10 * s;   // (the camera panel under it)
    g_lcLightLeft = a.x;
}

void BrickApp::stCamPanel() {
    if (m_opt.noUi || m_mechOn || m_vePart >= 0 || m_teditPart >= 0 || m_popup == 8) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    // the preview of the selected camera (as Unreal's): at the bottom left of the view, above the render buttons
    if (m_pipTex && m_camPanel && m_selCam >= 0 && m_selCam < (int)m_cams.size() && m_selCam != m_inCam) {
        const float pw = 320 * s, ph = pw * 9 / 16;
        const ImVec2 pa(m_stage.x + 14 * s, m_stage.w - 74 * s - ph), pb(pa.x + pw, pa.y + ph);
        u.dl->AddRectFilled(ImVec2(pa.x - 3 * s, pa.y - 22 * s), ImVec2(pb.x + 3 * s, pb.y + 3 * s), stD(IM_COL32(16, 17, 19, 225)), 10 * s);
        u.dl->AddImageRounded(m_pipTex, pa, pb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 7 * s);
        u.dl->AddRect(ImVec2(pa.x - 3 * s, pa.y - 22 * s), ImVec2(pb.x + 3 * s, pb.y + 3 * s), stW(40), 10 * s, 0, 1.0f);
        char t[96];
        snprintf(t, sizeof(t), "%s  ·  %.0f mm", m_cams[m_selCam].name.c_str(), 12.0f / std::tan(m_cams[m_selCam].cam.fovY * 0.5f));
        u.text(ImVec2(pa.x + 6 * s, pa.y - 19 * s), 9.5f, C_TEXT, t, true);
        m_uiRects.push_back(ImVec4(pa.x - 3 * s, pa.y - 22 * s, pb.x + 3 * s, pb.y + 3 * s));
        if (u.hover(pa, pb)) {
            if (ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Double click: look through this camera");
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) enterCamera(m_selCam);
        }
    }
    if (!m_camPanel) return;
    const float W = 290 * s, pad = 14 * s;
    const bool has = m_selCam >= 0 && m_selCam < (int)m_cams.size();
    const int rows = std::max(1, (int)m_cams.size());
    const float H = 40 * s + 40 * s + rows * 26 * s + 10 * s + (has ? 20 * s + 34 * s + 22 * s + 4 * s + 26 * s : 0) + pad;
    float x1p = g_lcAnchor.x > 0 ? g_lcAnchor.x : m_stage.z - 90 * s, top = m_lightPanel ? g_lcAnchor.y : m_stage.y + 14 * s;
    if (m_lightPanel && top + H > m_stage.w - 10 * s) { x1p = g_lcLightLeft - 10 * s; top = m_stage.y + 150 * s; }   // (no room under Lighting: beside it)
    const ImVec2 a(x1p - W, top), b(x1p, top + H);
    stGlass(u.dl, a, b, 16 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    float y = a.y + pad;
    stTrack(u, ImVec2(a.x + pad, y), 10, C_TEXT, "CAMERAS", 2.2f * s);
    lcClose(u, ImVec2(b.x - pad - 6 * s, y + 6 * s), m_camPanel, s);
    y += 26 * s;
    {
        const ImVec2 ca(a.x + pad, y), cb(a.x + pad + 110 * s, y + 28 * s);
        if (u.hover(ca, cb) && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Create a camera where the view is");
        if (lcPill(u, ca, cb, ST_CAM, "Camera", false, s)) addCamera();
        if (m_inCam >= 0 && lcPill(u, ImVec2(b.x - pad - 124 * s, y), ImVec2(b.x - pad, y + 28 * s), ST_ENTER, "Exit camera", true, s)) exitCamera();
    }
    y += 40 * s;
    LcList L;
    L.kind = 1;
    L.n = (int)m_cams.size();
    L.nameOf = [&](int i) { return m_cams[i].name; };
    L.toggleOn = [&](int i) { return m_cams[i].locked; };
    L.toggle = [&](int i) { m_cams[i].locked = !m_cams[i].locked; };
    L.setName = [&](int i, const std::string& n) { m_cams[i].name = n; };
    L.marked = [&](int i) { return i == m_inCam; };
    L.toggleIco = ST_LOCK;
    int sel = m_selCam;
    y = lcList(u, L, sel, m_lcRename, m_lcRenameKind, m_lcMenu, m_lcMenuKind, m_lcMenuAt, m_renameBuf, sizeof(m_renameBuf), a.x + pad, b.x - pad, y, s);
    if (sel != m_selCam) { m_selCam = sel; if (sel >= 0) m_selLight = -1; }
    y += 10 * s;
    if (m_selCam >= 0 && m_selCam < (int)m_cams.size()) {
        UserCam& c = m_cams[m_selCam];
        const float x0 = a.x + pad, x1 = b.x - pad;
        u.dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), stW(26), 1.0f);
        y += 8 * s;
        stTrack(u, ImVec2(x0, y), 8.5f, C_DIM, "CAMERA", 1.8f * s);
        y += 12 * s;
        {   // Enter / Exit, Lock, Frame
            const bool in = m_inCam == m_selCam;
            const float g = 5 * s, bw2 = (x1 - x0 - 2 * g) / 3;
            if (lcPill(u, ImVec2(x0, y), ImVec2(x0 + bw2, y + 26 * s), ST_ENTER, in ? "Exit" : "Enter", in, s)) { if (in) exitCamera(); else enterCamera(m_selCam); }
            if (lcPill(u, ImVec2(x0 + bw2 + g, y), ImVec2(x0 + 2 * bw2 + g, y + 26 * s), ST_LOCK, "Lock", c.locked, s)) c.locked = !c.locked;
            const ImVec2 fa(x0 + 2 * (bw2 + g), y), fb(x1, y + 26 * s);
            if (u.hover(fa, fb) && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Frame the model in this camera (its direction kept)");
            if (lcPill(u, fa, fb, ST_FRAME, "Frame", false, s)) frameCamera(m_selCam);
            y += 34 * s;
        }
        {   // the focal length (a 35 mm sensor: 24 mm high)
            float f = 12.0f / std::tan(c.cam.fovY * 0.5f);
            if (lcSlider(u, 30, m_lcDrag, "Focal length", f, 12, 200, "%.0f mm", f, x0, x1, y, s)) {
                c.cam.fovY = 2.0f * std::atan(12.0f / std::max(1.0f, f));
                if (m_inCam == m_selCam) { m_scene.camera.fovY = c.cam.fovY; m_renderer->resetAccumulation(); }
            }
        }
        y += 4 * s;
        if (lcPill(u, ImVec2(x0, y), ImVec2(x0 + 96 * s, y + 26 * s), ST_TRASH, "Delete", false, s)) deleteCamera(m_selCam);
        u.text(ImVec2(x0 + 106 * s, y + 7 * s), 9, C_FAINT, "View: arrows, rings");
    }
    if (m_lcMenu >= 0 && m_lcMenuKind == 1) {
        const int k = lcMenu(u, m_lcMenuAt, true, s, m_uiRects);
        if (k == 0) enterCamera(m_lcMenu);
        if (k == 1 && m_lcMenu < (int)m_cams.size()) { m_lcRename = m_lcMenu; m_lcRenameKind = 1; snprintf(m_renameBuf, sizeof(m_renameBuf), "%s", m_cams[m_lcMenu].name.c_str()); }
        if (k == 2) deleteCamera(m_lcMenu);
        if (k != -1) m_lcMenu = -1;
    }
}
