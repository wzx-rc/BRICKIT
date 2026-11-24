// BrickIt - the Render Settings panel (Wzx 24/11/2026: « un panneau flottant à activer : RENDER SETTING. Dedans tu y
// mets de façon bien organisée tous nos settings de lighting et rendu et tu les sors du panneau Setting »).
// A floating glass panel, moved by its title, opened by the third button beside the views (Lighting, Cameras, Render
// Settings) or from Settings; its sections fold. Everything that was in Settings › Render, Camera and Light is here.
// Included by BrickUi.cpp after BrickUiLights.inl (its helpers: stGlass, stTrack, lcClose, the colours).

void BrickApp::stRenderPanel() {
    if (!m_renderPanel || m_ipad || m_opt.noUi || m_mechOn || m_vePart >= 0 || m_teditPart >= 0 || m_popup == 8) return;
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui, W = 320 * s, pad = 14 * s, headH = 40 * s;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    // ---- where it is: m_rsPos = its right edge from the right of the window, its top (interface units); moved by its title
    if (m_rsPos.x < 0) {
        const float right = g_lcAnchor.x > 0 ? d.x - g_lcAnchor.x : 90 * s;
        m_rsPos = ImVec2(right / s + ((m_lightPanel || m_camPanel) ? 300.0f : 0.0f), (m_stage.y + 14 * s) / s);
    }
    ImVec2 p0(d.x - m_rsPos.x * s - W, m_rsPos.y * s);
    if (m_rsMove) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) p0 = ImVec2(u.mouse.x - m_rsGrab.x, u.mouse.y - m_rsGrab.y);
        else { m_rsMove = false; savePrefs(); }
    }
    p0.x = std::clamp(p0.x, 4 * s, std::max(4 * s, d.x - W - 4 * s));
    p0.y = std::clamp(p0.y, m_stage.y + 4 * s, std::max(m_stage.y + 4 * s, d.y - headH - 8 * s));
    m_rsPos = ImVec2((d.x - p0.x - W) / s, p0.y / s);
    const float contentH = m_rsH > 0 ? m_rsH : 520 * s;
    const float H = std::min(d.y - p0.y - 10 * s, headH + contentH + 8 * s);
    const ImVec2 a = p0, b(p0.x + W, p0.y + H);
    stGlass(u.dl, a, b, 16 * s, s);
    m_uiRects.push_back(ImVec4(a.x, a.y, b.x, b.y));
    const bool inPanel = u.hover(a, b);
    // ---- the title: the grip of the panel, its close button
    {
        const ImVec2 c(b.x - pad - 6 * s, a.y + pad + 6 * s);
        const bool overClose = u.hover(ImVec2(c.x - 11 * s, c.y - 11 * s), ImVec2(c.x + 11 * s, c.y + 11 * s));
        const bool overHead = u.hover(a, ImVec2(b.x, a.y + headH)) && !overClose;
        for (int k = 0; k < 2; ++k) for (int i = 0; i < 3; ++i) u.dl->AddCircleFilled(ImVec2(a.x + pad + i * 5 * s, a.y + pad + 3 * s + k * 5 * s), 1.2f * s, overHead || m_rsMove ? C_TEXT : C_FAINT, 6);
        stTrack(u, ImVec2(a.x + pad + 20 * s, a.y + pad), 10, C_TEXT, "RENDER SETTINGS", 2.2f * s);
        lcClose(u, c, m_renderPanel, s);
        if (!m_renderPanel) { savePrefs(); return; }
        if (overHead && u.pressed && m_popup == 0) { m_rsMove = true; m_rsGrab = ImVec2(u.mouse.x - a.x, u.mouse.y - a.y); }
        if (overHead || m_rsMove) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        u.dl->AddLine(ImVec2(a.x + pad, a.y + headH - 4 * s), ImVec2(b.x - pad, a.y + headH - 4 * s), stW(26), 1.0f);
    }
    // ---- the body, scrolled when it is taller than the window
    const float bodyTop = a.y + headH, bodyBot = b.y - 6 * s;
    const float scrollMax = std::max(0.0f, contentH - (bodyBot - bodyTop));
    if (inPanel && scrollMax > 0 && ImGui::GetIO().MouseWheel != 0) m_rsScroll -= ImGui::GetIO().MouseWheel * 60 * s;
    m_rsScroll = std::clamp(m_rsScroll, 0.0f, scrollMax);
    u.dl->PushClipRect(ImVec2(a.x, bodyTop), ImVec2(b.x, bodyBot), true);
    const float x0 = a.x + pad, x1 = b.x - pad;
    float y = bodyTop + 4 * s - m_rsScroll;
    const bool live = m_popup == 0 && !m_rsMove && u.mouse.y >= bodyTop && u.mouse.y < bodyBot;
    auto hov = [&](ImVec2 p, ImVec2 q) { return live && u.hover(p, q); };
    // a section: its bar (a chevron, its name, what it holds); a click folds it. True when open.
    auto section = [&](int i, const char* name, const char* sub) -> bool {
        const bool open = !(m_rsFold & (1u << i));
        const ImVec2 ha(a.x + 6 * s, y), hb(b.x - 6 * s, y + 26 * s);
        const bool h = hov(ha, hb);
        u.dl->AddRectFilled(ha, hb, h ? stW(26) : stW(12), 7 * s);
        const ImVec2 c(ha.x + 13 * s, (ha.y + hb.y) * 0.5f);
        const ImU32 cc = h ? C_TEXT : C_DIM;
        if (open) { u.dl->AddLine(ImVec2(c.x - 4 * s, c.y - 2 * s), ImVec2(c.x, c.y + 2 * s), cc, 1.5f * s); u.dl->AddLine(ImVec2(c.x, c.y + 2 * s), ImVec2(c.x + 4 * s, c.y - 2 * s), cc, 1.5f * s); }
        else { u.dl->AddLine(ImVec2(c.x - 2 * s, c.y - 4 * s), ImVec2(c.x + 2 * s, c.y), cc, 1.5f * s); u.dl->AddLine(ImVec2(c.x + 2 * s, c.y), ImVec2(c.x - 2 * s, c.y + 4 * s), cc, 1.5f * s); }
        const float nw = stTrack(u, ImVec2(ha.x + 26 * s, c.y - u.fs(9.5f) * 0.52f), 9.5f, C_ACCENT_TXT, name, 1.8f * s);
        if (sub && ha.x + 26 * s + nw + 10 * s + u.textW(9, sub) < hb.x - 8 * s) u.text(ImVec2(hb.x - 8 * s - u.textW(9, sub), c.y - u.fs(9) * 0.52f), 9, C_FAINT, sub);
        if (h && u.pressed) { m_rsFold ^= 1u << i; savePrefs(); }
        y += 26 * s + (open ? 8 * s : 4 * s);
        return open;
    };
    // a row of choices (each as wide as its text asks): the one clicked, -1
    auto seg = [&](const char* label, std::initializer_list<const char*> labs, int on) -> int {
        if (label && *label) { u.text(ImVec2(x0, y), 10, C_DIM, label); y += 15 * s; }
        const float rh = 24 * s, g = 3 * s;
        const int n = (int)labs.size();
        float tw = 0;
        for (const char* l : labs) tw += u.textW(10, l, true);
        const float padx = std::max(4 * s, (x1 - x0 - (n - 1) * g - tw) / n);
        float x = x0;
        int k = 0, hit = -1;
        for (const char* l : labs) {
            const float bw = u.textW(10, l, true) + padx;
            const ImVec2 ba(x, y), bb(x + bw, y + rh);
            const bool o = k == on, h = hov(ba, bb);
            u.dl->AddRectFilled(ba, bb, o ? C_ACCENT : (h ? stW(34) : stW(14)), 7 * s);
            const float w = u.textW(10, l, o);
            u.text(ImVec2((ba.x + bb.x - w) * 0.5f, ba.y + (rh - u.fs(10)) * 0.5f - 0.5f * s), 10, o ? C_INK : C_TEXT, l, o);
            if (h && u.pressed && !o) hit = k;
            x += bw + g;
            ++k;
        }
        y += rh + 8 * s;
        return hit;
    };
    // a switch on its line: true when clicked
    auto toggle = [&](const char* label, bool v, const char* tip = nullptr) -> bool {
        const float rh = 24 * s;
        const bool h = hov(ImVec2(x0, y), ImVec2(x1, y + rh));
        u.text(ImVec2(x0, y + (rh - u.fs(10)) * 0.5f), 10, h ? C_TEXT : C_DIM, label);
        const ImVec2 sa(x1 - 30 * s, y + 4 * s), sb(x1, y + 20 * s);
        u.dl->AddRectFilled(sa, sb, v ? C_ACCENT : stW(34), 8 * s);
        u.dl->AddCircleFilled(ImVec2(v ? sb.x - 8 * s : sa.x + 8 * s, (sa.y + sb.y) * 0.5f), 5.5f * s, v ? C_INK : C_TEXT, 16);
        if (h && tip && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", tip);
        y += rh + 2 * s;
        return h && u.pressed;
    };
    // a slider on its line (label, track, value): true while it moves; the preferences are saved when it is released
    auto sl = [&](int id, const char* label, float& v, float lo, float hi, const char* fmt, float shown, const char* tip = nullptr) -> bool {
        const float rh = 22 * s, labW = 112 * s, vw = 38 * s;
        u.text(ImVec2(x0, y + (rh - u.fs(10)) * 0.5f), 10, C_DIM, label);
        char vb[32];
        snprintf(vb, sizeof(vb), fmt, shown);
        u.text(ImVec2(x1 - u.textW(9.5f, vb), y + (rh - u.fs(9.5f)) * 0.5f), 9.5f, C_TEXT, vb);
        const ImVec2 ta(x0 + labW, y + rh * 0.5f - 3 * s), tb(x1 - vw - 6 * s, y + rh * 0.5f + 3 * s);
        const bool h = hov(ImVec2(ta.x - 6 * s, y), ImVec2(tb.x + 6 * s, y + rh));
        if (hov(ImVec2(x0, y), ImVec2(x1, y + rh)) && tip && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", tip);
        if (h && u.pressed) m_rsDrag = id;
        bool changed = false;
        if (m_rsDrag == id) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                const float t = std::clamp((u.mouse.x - ta.x) / std::max(1.0f, tb.x - ta.x), 0.0f, 1.0f), nv = lo + t * (hi - lo);
                if (std::fabs(nv - v) > 1e-5f * (hi - lo)) { v = nv; changed = true; }
            } else { m_rsDrag = -1; savePrefs(); }
        }
        const float t = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
        u.dl->AddRectFilled(ta, tb, stW(26), 3 * s);
        u.dl->AddRectFilled(ta, ImVec2(ta.x + (tb.x - ta.x) * t, tb.y), C_PROGRESS, 3 * s);
        u.dl->AddCircleFilled(ImVec2(ta.x + (tb.x - ta.x) * t, (ta.y + tb.y) * 0.5f), (h || m_rsDrag == id ? 7.5f : 6.5f) * s, IM_COL32(255, 255, 255, 255), 20);
        y += rh + 2 * s;
        return changed;
    };
    auto note = [&](const char* txt) {
        u.text(ImVec2(x0, y - 2 * s), 9, C_FAINT, txt);
        y += 14 * s;
    };
    auto endSection = [&] { y += 6 * s; };
    enum { R_ENGINE, R_IMAGE, R_ENV, R_MAT, R_GI, R_CAM, R_RC, R_GRID };
    const bool pt = pathTraced();
    bool lookChanged = false, reset = false;

    // ================================================================== ENGINE
    if (section(R_ENGINE, "ENGINE", "mode, resolution")) {
        const int cur = m_rcMode ? 4 : pt ? (m_rtDlss ? 3 : 2) : m_quality == 1 ? 1 : 0;
        const int k = seg(nullptr, {"Fast", "HQ", "Ray tracing", "RT DLSS", "RC"}, cur);
        if (k >= 0) {
            if (k == 4) { setRC(true); applyLook(); }
            else if (k >= 2) { setRC(false); m_rtDlss = k == 3; m_rs.raster = false; applyLook(); }
            else { setRC(false); m_quality = k; m_rs.msaaSamples = m_quality ? 4 : 1; if (!m_rs.raster) { m_rs.raster = true; applyLook(); } }
            reset = true;
            savePrefs();
        }
        static const char* kWhat[5] = {"Fast: simple real time", "HQ: real time, soft GI, SSS, depth of field", "Ray tracing: progressive path tracing",
                                       "Ray tracing, Open Image Denoise still, DLSS moving", "RC: Radiance Cascades (real time GI)"};
        note(kWhat[cur]);
        y += 2 * s;
        static const float kRs[4] = {1.0f, 0.85f, 0.75f, 0.67f};
        const int rs = m_renderScale > 0.95f ? 0 : std::fabs(m_renderScale - 0.85f) < 0.01f ? 1 : std::fabs(m_renderScale - 0.75f) < 0.01f ? 2 : 3;
        const int r = seg("Render scale", {"100 %", "85 %", "75 %", "67 %"}, rs);
        if (r >= 0) { m_renderScale = kRs[r]; reset = true; savePrefs(); }
        endSection();
    }
    // ================================================================== IMAGE
    if (section(R_IMAGE, "IMAGE", "tone curve, bloom, glare")) {
        static const int kTm[4] = {0, 1, 4, 3};   // (RenderSettings::tonemapper: 0 ACES, 1 AgX, 3 Standard, 4 Filmic)
        const int cur = m_rs.tonemapper == 4 ? 2 : m_rs.tonemapper == 3 ? 3 : m_rs.tonemapper == 1 ? 1 : 0;
        const int k = seg("View transform", {"ACES", "AgX", "Filmic", "Standard"}, cur);
        if (k >= 0) { m_rs.tonemapper = kTm[k]; reset = true; savePrefs(); }
        static const char* kTmWhat[4] = {"ACES: the film look, contrast, desaturated highlights", "AgX: soft, the colours kept in the highlights",
                                         "Filmic: vivid colours, a long soft shoulder", "Standard: no curve, the colours as they are"};
        note(kTmWhat[cur]);
        y += 2 * s;
        sl(201, "Bloom · HQ", m_bloom, 0.0f, 1.0f, "%.2f", m_bloom, "The glow of the bright parts in the real time views (0 = off)");
        sl(202, "Bloom · Ray tracing", m_bloomPT, 0.0f, 1.0f, "%.2f", m_bloomPT, "The glow of the bright parts in Ray tracing (0 = off)");
        sl(203, "Glare", m_glare, 0.0f, 1.0f, "%.2f", m_glare, "A wide, soft halo around the highlights, as a lens gives (0 = off)");
        endSection();
    }
    // ================================================================== ENVIRONMENT
    if (section(R_ENV, "ENVIRONMENT", "HDRI, sun")) {
        const float rh = 24 * s, bw = (x1 - x0 - 3 * s) / 2;
        for (size_t i = 0; i < m_hdriPaths.size(); ++i) {
            const float bx = x0 + (i % 2) * (bw + 3 * s);
            const ImVec2 ba(bx, y), bb(bx + bw, y + rh);
            const bool on = m_hdriPaths[i] == m_hdriPath, h = hov(ba, bb);
            u.dl->AddRectFilled(ba, bb, on ? C_ACCENT : (h ? stW(34) : stW(14)), 7 * s);
            std::string name = "Procedural sky";
            if (!m_hdriPaths[i].empty()) {
                std::wstring f = m_hdriPaths[i].substr(m_hdriPaths[i].find_last_of(L"\\/") + 1);
                const size_t dot = f.find_last_of(L'.');
                const std::wstring ext = dot != std::wstring::npos ? f.substr(dot + 1) : L"";
                if (dot != std::wstring::npos) f = f.substr(0, dot);
                for (const wchar_t* cut : {L"_2k", L"_4k", L"_1k", L"_8k"}) { const size_t k = f.find(cut); if (k != std::wstring::npos) f = f.substr(0, k); }
                for (auto& c : f) if (c == L'_') c = L' ';
                if (f.size() > 16) f = f.substr(0, 16);
                if (ext == L"exr") f += L" (exr)";
                name.assign(f.begin(), f.end());   // (file names of the HDRI: plain ASCII)
            }
            const float tw = u.textW(10, name.c_str(), on);
            u.text(ImVec2((ba.x + bb.x - tw) * 0.5f, ba.y + (rh - u.fs(10)) * 0.5f - 0.5f * s), 10, on ? C_INK : C_TEXT, name.c_str(), on);
            if (h && u.pressed && !on) { setHdri(m_hdriPaths[i]); savePrefs(); }
            if (i % 2 == 1 || i + 1 == m_hdriPaths.size()) y += rh + 3 * s;
        }
        {   // a custom HDRI
            const ImVec2 ba(x0, y), bb(x1, y + rh);
            const bool h = hov(ba, bb);
            u.dl->AddRect(ba, bb, h ? stW(120) : stW(50), 7 * s, 0, 1.0f);
            const char* lab = "Load an HDRI…  (.hdr, .exr)";
            u.text(ImVec2((ba.x + bb.x - u.textW(10, lab)) * 0.5f, ba.y + (rh - u.fs(10)) * 0.5f - 0.5f * s), 10, h ? C_TEXT : C_DIM, lab);
            if (h && u.pressed) loadHdriDialog();
            y += rh + 8 * s;
        }
        if (sl(211, "Rotation", m_hdriRotDeg, 0.0f, 360.0f, "%.0f°", m_hdriRotDeg, "Turns the HDRI; the sun follows it")) lookChanged = true;
        if (sl(212, "Intensity", m_hdriInt, 0.0f, 3.0f, "%.2f", m_hdriInt, "The light of the sky")) lookChanged = true;
        endSection();
    }
    // ================================================================== MATERIALS
    if (section(R_MAT, "MATERIALS", "SSS, bevel, glass, clay")) {
        if (sl(221, "SSS", m_hqSSSStrength, 0.0f, 2.0f, "%.2f", m_hqSSSStrength, "The light under the surface of the plastic (0 = off). Per colour: the Material panel") && pt) { applyLook(); reset = true; }
        if (toggle("Rounded edges (bevel)", m_rs.bevel)) { m_rs.bevel = !m_rs.bevel; reset = true; savePrefs(); }
        if (m_rs.bevel && sl(222, "Bevel radius", m_bevelUser, 0.0f, 5.0f, "%.2f", m_bevelUser, "The rounding of the edges in Ray tracing and HQ (0.5 by default)")) {
            m_rs.bevelScale = m_bevelUser / 0.5f;
            reset = true;
        }
        if (toggle("Glass absorption (thickness) · RT", m_glassAbsorb, "Ray tracing: the colour of a transparent part deepens with its thickness (a thin pane paler, a thick part richer)")) { m_glassAbsorb = !m_glassAbsorb; reset = true; savePrefs(); }
        if (toggle("Clay render (plain grey)", m_giLookdev, "Every part the same matte grey: the lighting alone")) setLookdev(!m_giLookdev);
        endSection();
    }
    // ================================================================== GLOBAL ILLUMINATION (the real time HQ)
    if (section(R_GI, "GLOBAL ILLUMINATION · HQ", nullptr)) {
        if (toggle("Soft GI", m_softGI, "The light bounced between the bricks, from probes (the HQ render)")) { m_softGI = !m_softGI; reset = true; savePrefs(); }
        if (m_softGI) {
            sl(231, "GI intensity", m_softGIStrength, 0.0f, 2.0f, "%.2f", m_softGIStrength);   // (live, probes kept)
            static const int kBudgets[3] = {4096, 8192, 16384};   // (16384 at most: lit::placeProbeGrid)
            const int cur = m_softGIProbes == kBudgets[0] ? 0 : m_softGIProbes == kBudgets[1] ? 1 : m_softGIProbes == kBudgets[2] ? 2 : -1;
            const int k = seg("Probe quality", {"Low", "Medium", "High"}, cur);
            if (k >= 0) { m_softGIProbes = kBudgets[k]; reset = true; savePrefs(); }
            if (toggle("Freeze the GI while moving", m_softGIHold)) { m_softGIHold = !m_softGIHold; savePrefs(); }
            if (toggle("Show the probes", m_showProbes)) { m_showProbes = !m_showProbes; savePrefs(); }
        }
        if (toggle("HDRI occlusion", m_ssaoOn, "The sky hidden in the hollows (HQ)")) { m_ssaoOn = !m_ssaoOn; reset = true; savePrefs(); }
        if (toggle("Still anti-aliasing", m_stillAA, "The image refined when the view stops (HQ)")) { m_stillAA = !m_stillAA; reset = true; savePrefs(); }
        endSection();
    }
    // ================================================================== CAMERA (the lens of the HQ)
    if (section(R_CAM, "CAMERA · HQ", "depth of field")) {
        sl(241, "Depth of field", m_dof, 0.0f, 1.0f, "%.2f", m_dof, "The blur away from the focus distance (0 = off)");
        const int k = seg("Focus", {"Auto (reticle)", "Selection", "Manual"}, m_focusMode);
        if (k >= 0) {
            m_focusMode = k;
            if (k == 2) m_focusManual = std::clamp(m_focusCur > 0 ? m_focusCur : m_scene.camera.distance, 2.0f, 400.0f);
            savePrefs();
        }
        if (m_focusMode == 2) sl(242, "Focus distance", m_focusManual, 2.0f, 400.0f, "%.0f cm", m_focusManual);
        if (m_focusMode == 0 && toggle("Focus reticle", m_focusReticle)) { m_focusReticle = !m_focusReticle; savePrefs(); }
        endSection();
    }
    // ================================================================== RADIANCE CASCADES
    if (section(R_RC, "RADIANCE CASCADES", "anti-aliasing, shadows")) {
        const int k = seg("Anti-aliasing", {"MSAA 4x", "MSAA 8x", "AAA"}, m_rs.aaQuality);
        if (k >= 0) { m_rs.aaQuality = k; reset = true; savePrefs(); }
        const int sh = m_rs.shadowQuality <= 1 ? 0 : m_rs.shadowQuality - 1;
        const int q = seg("Shadows", {"Low", "Medium", "High", "AAA"}, sh);
        if (q >= 0) { m_rs.shadowQuality = q == 0 ? 0 : q + 1; reset = true; savePrefs(); }
        endSection();
    }
    // ================================================================== FLOOR GRID
    if (section(R_GRID, "FLOOR GRID", "colour, opacity")) {
        static const uint32_t sw[7] = {0x9aa3b8, 0xffffff, 0xf2c230, 0x4fc3f7, 0xe53935, 0x43a047, 0x7e57c2};
        const float rh = 24 * s, dw = 58 * s, cw = (x1 - x0 - dw - 7 * 3 * s) / 7;
        for (int k = 0; k < 8; ++k) {
            const float bx = k == 0 ? x0 : x0 + dw + 3 * s + (k - 1) * (cw + 3 * s);
            const ImVec2 ba(bx, y), bb(bx + (k == 0 ? dw : cw), y + rh);
            const bool h = hov(ba, bb);
            if (k == 0) {
                const bool on = m_gridAlpha <= 0;
                u.dl->AddRectFilled(ba, bb, on ? C_ACCENT : (h ? stW(34) : stW(14)), 7 * s);
                const float tw = u.textW(10, "Default", on);
                u.text(ImVec2((ba.x + bb.x - tw) * 0.5f, ba.y + (rh - u.fs(10)) * 0.5f - 0.5f * s), 10, on ? C_INK : C_TEXT, "Default", on);
                if (h && u.pressed) { m_gridAlpha = 0; lookChanged = true; savePrefs(); }
            } else {
                const uint32_t c = sw[k - 1];
                const float3 rgb(((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f);
                const bool on = m_gridAlpha > 0 && length(rgb - m_gridRgb) < 0.01f;
                u.dl->AddRectFilled(ba, bb, IM_COL32((c >> 16) & 255, (c >> 8) & 255, c & 255, 255), 7 * s);
                if (on || h) u.dl->AddRect(ba, bb, on ? C_ACCENT : C_TEXT, 7 * s, 0, (on ? 2.5f : 1.5f) * s);
                if (h && u.pressed) { m_gridRgb = rgb; if (m_gridAlpha <= 0) m_gridAlpha = 0.6f; lookChanged = true; savePrefs(); }
            }
        }
        y += rh + 8 * s;
        if (sl(251, "Opacity", m_gridAlpha, 0.0f, 1.0f, "%.0f %%", m_gridAlpha * 100.0f)) lookChanged = true;
        endSection();
    }
    m_rsH = y + m_rsScroll - bodyTop + 4 * s;
    u.dl->PopClipRect();
    if (scrollMax > 0) {   // the scroll bar
        const float trackH = bodyBot - bodyTop - 8 * s, barH = std::max(30 * s, trackH * (bodyBot - bodyTop) / contentH);
        const float by = bodyTop + 4 * s + (trackH - barH) * (m_rsScroll / scrollMax);
        u.dl->AddRectFilled(ImVec2(b.x - 6 * s, by), ImVec2(b.x - 3 * s, by + barH), C_FAINT, 2 * s);
    }
    if (lookChanged) {   // live: the environment and the grid of every renderer
        if (pt) applyLook();   // (the path tracer: its own sky and sun from these)
        else {
            m_scene.env.hdriRotation = radians(m_hdriRotDeg);
            m_scene.sun.direction = hdriSunDir();   // (the sun turns with the HDRI)
            m_scene.env.hdriIntensity = m_hdriInt;
            m_scene.gridLine = gridLineValue();
        }
        reset = true;
    }
    if (reset) m_renderer->resetAccumulation();
}
