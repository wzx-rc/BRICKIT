// ------------------------------------------------------------------ the hub of the scenes (Wzx 15/11/2026)
// A click on the name of the scene (top left): a panel over the app. Left, the scene open: its thumbnail (the 3D view,
// taken again on demand), its name (a click renames it, its file and its cloud copy with it), its pieces, its file, the
// state of its sync with the cloud, the actions (Enregistrer, Nouvelle, Ouvrir, Partager...). Right, the scenes:
// « Sur cet appareil » (Mes scènes: thumbnails, rename, duplicate, delete), « Cloud » (the account, iPad and PC),
// « Démos ». Opening another scene first saves the open one into Mes scènes: nothing is lost.
// (BrickHub.cpp: the files and the thumbnails.)

static const char* ST_PEN = "M4 20h4L19 9l-4-4L4 16zM13 7l4 4";
static const char* ST_CLOUD = "M7 18h10a4 4 0 0 0 .6-7.95A6 6 0 0 0 6.1 9.2 4.5 4.5 0 0 0 7 18z";
static const char* ST_SAVE = "M5 4h11l3 3v13H5zM8 4v5h7V4M8 20v-6h8v6";
static const char* ST_OPEN = "M3 7h7l2 2h9v10H3zM3 7V5h6";
static const char* ST_NEW = "M12 5v14M5 12h14";
static const char* ST_SHARE = "M12 3v12M8 7l4-4 4 4M5 13v7h14v-7";
static const char* ST_SNAP = "M4 8h4l2-3h4l2 3h4v11H4zM12 10a3.5 3.5 0 1 0 0 7 3.5 3.5 0 0 0 0-7z";
static const char* ST_SYNC = "M20 12a8 8 0 0 1-14 5.3M4 12a8 8 0 0 1 14-5.3M18 3v4h-4M6 21v-4h4";

// « aujourd'hui 18:20 », « hier 09:12 », « 12/11 14:03 »
static std::string hubDate(long long t) {
    if (t <= 0) return std::string();
    const std::time_t tt = (std::time_t)t, now = std::time(nullptr);
    std::tm a{}, n{};
#ifdef _WIN32
    localtime_s(&a, &tt);
    localtime_s(&n, &now);
#else
    localtime_r(&tt, &a);
    localtime_r(&now, &n);
#endif
    char b[48];
    const int days = (n.tm_year - a.tm_year) * 366 + (n.tm_yday - a.tm_yday);
    if (days == 0) snprintf(b, sizeof(b), "aujourd'hui %02d:%02d", a.tm_hour, a.tm_min);
    else if (days == 1) snprintf(b, sizeof(b), "hier %02d:%02d", a.tm_hour, a.tm_min);
    else snprintf(b, sizeof(b), "%02d/%02d/%02d", a.tm_mday, a.tm_mon + 1, a.tm_year % 100);
    return b;
}

void BrickApp::stHub() {
    if (m_popup != 8) { m_hubEdit = -1; return; }
    if (m_hubStale) hubRefresh();
    Ui u = ui(m_ui);
    u.mouse = ImGui::GetIO().MousePos;   // (the rest of the interface is blind while the hub is open: g_uiBlock)
    u.pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = u.dl;
    const double now = ImGui::GetTime();
    const bool editing = m_hubEdit != -1;
    bool clicked = false, close = false;
    // the veil, the panel
    dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(4, 5, 7, 165));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const float W = std::min(1180 * s, d.x - 40 * s), H = std::min(780 * s, d.y - 56 * s);
    const ImVec2 a(std::floor((d.x - W) * 0.5f), std::floor((d.y - H) * 0.5f)), b(a.x + W, a.y + H);
    dl->AddRectFilled(ImVec2(a.x, a.y + 10 * s), ImVec2(b.x, b.y + 14 * s), IM_COL32(0, 0, 0, 70), 24 * s);
    dl->AddRectFilled(a, b, IM_COL32(17, 18, 21, 250), 20 * s);
    dl->AddRect(a, b, IM_COL32(255, 255, 255, 22), 20 * s, 0, 1.0f);
    const float pad = 24 * s;
    // ---- header: MES SCÈNES, the close button
    stTrack(u, ImVec2(a.x + pad, a.y + 22 * s), 12, C_TEXT, "MES SCÈNES", 2.4f * s);
    {
        char sub[96];
        snprintf(sub, sizeof(sub), "%d sur cet appareil%s", (int)m_hubLocal.size(), m_cloud.signedIn() ? (" · " + std::to_string(m_cloudList.size()) + " dans le cloud").c_str() : "");
        u.text(ImVec2(a.x + pad + stTrackW(u, 12, "MES SCÈNES", 2.4f * s, true) + 14 * s, a.y + 23 * s), 10.5f, C_DIM, sub);
    }
    {
        const ImVec2 c(b.x - pad - 14 * s, a.y + 30 * s);
        const bool h = u.hover(ImVec2(c.x - 18 * s, c.y - 18 * s), ImVec2(c.x + 18 * s, c.y + 18 * s));
        dl->AddCircleFilled(c, 17 * s, IM_COL32(255, 255, 255, h ? 26 : 10), 32);
        stIcon(dl, ST_CLOSE, c, 16 * s, h ? C_TEXT : C_DIM, 1.6f * s);
        if (h && u.pressed) { clicked = close = true; }
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Fermer (Échap)");
    }
    const float top = a.y + 62 * s;
    dl->AddLine(ImVec2(a.x + pad, top - 4 * s), ImVec2(b.x - pad, top - 4 * s), IM_COL32(255, 255, 255, 14), 1.0f);

    // an edit field (rename) over a rectangle: 1 Entrée, -1 Échap, 0 typing
    auto editField = [&](ImVec2 ea, ImVec2 eb, float px) -> int {
        dl->AddRectFilled(ea, eb, IM_COL32(255, 255, 255, 14), 8 * s);
        dl->AddRect(ea, eb, stA(C_ACCENT, 0.8f), 8 * s, 0, 1.2f * s);
        ImGui::SetNextWindowPos(ImVec2(ea.x + 8 * s, ea.y));
        ImGui::SetNextWindowSize(ImVec2(eb.x - ea.x - 12 * s, eb.y - ea.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, (eb.y - ea.y - u.fs(px)) * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C_TEXT));
        ImGui::Begin("##hubedit", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
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
    };
    auto startEdit = [&](int i, const std::string& name) {
        m_hubEdit = i;
        m_hubEditFocus = true;
        snprintf(m_hubName, sizeof(m_hubName), "%s", name.c_str());
    };
    auto finishEdit = [&](int r) {
        const int i = m_hubEdit;
        m_hubEdit = -1;
        if (r == 1 && m_hubName[0]) hubRename(i, m_hubName);
    };
    // the open scene goes into Mes scènes before another one replaces it
    auto keepOpen = [&] {
        const std::wstring dir = scenesDir();
        if (m_bricks.empty()) return;
        if (!m_scenePath.empty() && m_scenePath.compare(0, dir.size(), dir) == 0) saveScene(m_scenePath);
        else if (m_scenePath.empty()) hubSaveHere();
    };
    // a small round icon button
    auto roundBtn = [&](ImVec2 c, float r, const char* ico, const char* tip, bool danger = false) {
        const bool h = u.hover(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
        dl->AddCircleFilled(c, r, h ? (danger ? IM_COL32(200, 50, 45, 230) : IM_COL32(60, 62, 70, 240)) : IM_COL32(20, 21, 24, 215), 24);
        stIcon(dl, ico, c, r * 1.05f, h ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 222, 226, 255), 1.4f * s);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("%s", tip);
        return h && u.pressed;
    };
    // the placeholder of a scene without a thumbnail: a soft gradient, a brick
    auto noThumb = [&](ImVec2 ta, ImVec2 tb, float r, ImDrawFlags fl, const char* ico) {
        dl->AddRectFilled(ta, tb, IM_COL32(30, 32, 37, 255), r, fl);
        dl->AddRectFilledMultiColor(ImVec2(ta.x, (ta.y + tb.y) * 0.5f), ImVec2(tb.x, tb.y - (fl ? 0 : r)), IM_COL32(246, 196, 0, 0), IM_COL32(246, 196, 0, 0), IM_COL32(246, 196, 0, 10), IM_COL32(246, 196, 0, 10));
        const ImVec2 c((ta.x + tb.x) * 0.5f, (ta.y + tb.y) * 0.5f);
        if (ico) stIcon(dl, ico, c, (tb.y - ta.y) * 0.32f, IM_COL32(120, 124, 132, 255), 1.4f * s);
        else stPicto(dl, "brick", 2, 4, c, (tb.y - ta.y) * 0.42f, IM_COL32(120, 124, 132, 255), 1.2f * s);
    };

    // ======== left: the scene open
    const float LW = std::clamp(W * 0.3f, 280 * s, 360 * s);
    const float lx = a.x + pad;
    float y = top + 10 * s;
    stTrack(u, ImVec2(lx, y), 7.5f, C_DIM, "SCÈNE OUVERTE", 1.6f * s);
    y += 18 * s;
    const std::wstring dir = scenesDir();
    const bool inLib = !m_scenePath.empty() && m_scenePath.size() > dir.size() && m_scenePath.compare(0, dir.size(), dir) == 0;
    {   // its thumbnail
        const float th = std::floor(LW * 0.625f);
        const ImVec2 ta(lx, y), tb(lx + LW, y + th);
        ImTextureID tex = inLib ? hubThumb(m_scenePath) : 0;
        if (tex) dl->AddImageRounded(tex, ta, tb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 14 * s);
        else {
            noThumb(ta, tb, 14 * s, 0, nullptr);
            const char* t = "Pas encore de vignette";
            u.text(ImVec2((ta.x + tb.x - u.textW(10, t)) * 0.5f, tb.y - 30 * s), 10, C_FAINT, t);
        }
        dl->AddRect(ta, tb, IM_COL32(255, 255, 255, 26), 14 * s, 0, 1.0f);
        if (!m_thumbReq.empty()) dl->AddRectFilled(ta, tb, IM_COL32(255, 255, 255, (int)(60 * (0.5f + 0.5f * std::sin(now * 8)))), 14 * s);
        // « Vignette »: the 3D view of now becomes its thumbnail (saved first if needed)
        const char* lab = "VIGNETTE";
        const float bw = stTrackW(u, 7.5f, lab, 1.2f * s, true) + 40 * s, bh = 28 * s;
        const ImVec2 ba(tb.x - bw - 10 * s, tb.y - bh - 10 * s), bb(tb.x - 10 * s, tb.y - 10 * s);
        const bool h = u.hover(ba, bb);
        dl->AddRectFilled(ba, bb, h ? IM_COL32(246, 196, 0, 255) : IM_COL32(12, 13, 15, 210), bh * 0.5f);
        stIcon(dl, ST_SNAP, ImVec2(ba.x + 16 * s, (ba.y + bb.y) * 0.5f), 14 * s, h ? C_INK : C_TEXT, 1.4f * s);
        stTrack(u, ImVec2(ba.x + 28 * s, (ba.y + bb.y) * 0.5f - u.fs(7.5f) * 0.52f), 7.5f, h ? C_INK : C_TEXT, lab, 1.2f * s);
        if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("La vue 3D actuelle devient la vignette de la scène");
        if (h && u.pressed) {
            clicked = true;
            if (inLib) m_thumbReq = m_scenePath;
            else hubSaveHere();   // (into Mes scènes, its thumbnail taken with it)
        }
        y += th + 14 * s;
    }
    {   // its name: a click renames it (its file, its copy in the cloud)
        const std::string nm = m_modelName.empty() ? std::string("Ma scène") : m_modelName;
        const ImVec2 na(lx - 6 * s, y), nb(lx + LW, y + 34 * s);
        if (m_hubEdit == -2) {
            const int r = editField(na, nb, 16);
            if (r) { clicked = true; finishEdit(r); }
        } else {
            const bool h = u.hover(na, nb) && !editing;
            if (h) dl->AddRectFilled(na, nb, IM_COL32(255, 255, 255, 10), 8 * s);
            u.text(ImVec2(lx, y + 17 * s - u.fs(16) * 0.55f), 16, C_TEXT, stFit(u, 16, nm, LW - 30 * s, true).c_str(), true);
            stIcon(dl, ST_PEN, ImVec2(nb.x - 14 * s, y + 17 * s), 15 * s, h ? C_ACCENT : C_FAINT, 1.4f * s);
            if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Renommer la scène");
            if (h && u.pressed) { clicked = true; startEdit(-2, nm); }
        }
        y += 38 * s;
        std::string info = std::to_string(m_bricks.size()) + (m_bricks.size() > 1 ? " pièces" : " pièce");
        if (inLib) info += " · enregistrée " + hubDate(hubFileTime(m_scenePath));
        else if (!m_scenePath.empty()) { std::string f = plat::toUtf8(m_scenePath); const size_t k = f.find_last_of("\\/"); info += " · " + (k != std::string::npos ? f.substr(k + 1) : f); }
        else info += " · pas encore enregistrée";
        u.text(ImVec2(lx, y), 10.5f, C_DIM, stFit(u, 10.5f, info, LW, false).c_str());
        y += 26 * s;
    }
    {   // the cloud: its state, Synchroniser, Synchro auto
        const bool in = m_cloud.signedIn();
        const float ch = in ? 92 * s : 70 * s;
        const ImVec2 ca(lx, y), cb(lx + LW, y + ch);
        dl->AddRectFilled(ca, cb, IM_COL32(255, 255, 255, 7), 12 * s);
        dl->AddRect(ca, cb, IM_COL32(255, 255, 255, 16), 12 * s, 0, 1.0f);
        stIcon(dl, ST_CLOUD, ImVec2(ca.x + 22 * s, ca.y + 22 * s), 18 * s, C_DIM, 1.4f * s);
        ImU32 dot = IM_COL32(120, 124, 132, 255);
        std::string st;
        if (!in) st = "Cloud : non connecté";
        else if (m_cloudSending) { st = "Envoi en cours…"; dot = C_ACCENT; }
        else if (m_cloudMsg.rfind("envoi impossible", 0) == 0) { st = "Envoi impossible, nouvel essai bientôt"; dot = IM_COL32(229, 72, 60, 255); }
        else if (m_cloudDirty) { st = "Modifiée : envoyée dans la minute"; dot = C_ACCENT; }
        else if (!m_cloudId.empty()) { st = "À jour dans le cloud"; dot = IM_COL32(76, 190, 110, 255); }
        else st = "Pas encore dans le cloud";
        if (m_cloudSending) dot = stA(dot, 0.5f + 0.5f * (float)std::sin(now * 6));
        dl->AddCircleFilled(ImVec2(ca.x + 44 * s, ca.y + 22 * s), 4 * s, dot, 12);
        u.text(ImVec2(ca.x + 56 * s, ca.y + 22 * s - u.fs(11.5f) * 0.55f), 11.5f, C_TEXT, st.c_str(), true);
        if (!in) {
            u.text(ImVec2(ca.x + 14 * s, ca.y + 42 * s), 10, C_DIM, "Retrouve tes scènes sur l'iPad et le PC");
            float bw = 0;
            const char* lab = "Se connecter";
            const float lw = u.textW(11, lab, true) + 20 * s;
            if (webButton(u, ImVec2(cb.x - lw - 10 * s, ca.y + 36 * s), lab, bw, true, C_ACCENT, C_INK, false, 26, 11)) {
                clicked = true;
                m_popup = 5; m_setSec = kSetCloud; m_setScroll = 0; m_popupAt = ImVec2(d.x - 560 * s, 54 * s);
            }
        } else {
            u.text(ImVec2(ca.x + 14 * s, ca.y + 42 * s), 10, C_DIM, stFit(u, 10, m_cloud.email(), LW - 28 * s, false).c_str());
            // Synchro auto: oui / non
            const float sy = ca.y + 62 * s;
            u.text(ImVec2(ca.x + 14 * s, sy + 4 * s), 10, C_DIM, "Synchro auto");
            const float tx = ca.x + 14 * s + u.textW(10, "Synchro auto") + 10 * s;
            const ImVec2 ta(tx, sy + 2 * s), tb(tx + 34 * s, sy + 20 * s);
            const bool ht = u.hover(ta, tb);
            dl->AddRectFilled(ta, tb, m_cloudAuto ? C_ACCENT : IM_COL32(255, 255, 255, ht ? 40 : 26), 9 * s);
            dl->AddCircleFilled(ImVec2(m_cloudAuto ? tb.x - 9 * s : ta.x + 9 * s, (ta.y + tb.y) * 0.5f), 7 * s, m_cloudAuto ? C_INK : C_TEXT, 16);
            if (ht && u.pressed) { clicked = true; m_cloudAuto = !m_cloudAuto; savePrefs(); }
            // Synchroniser maintenant
            const char* lab = "Synchroniser";
            const float bw2 = u.textW(11, lab, true) + 40 * s;
            const ImVec2 ba(cb.x - bw2 - 10 * s, sy - 2 * s), bb(cb.x - 10 * s, sy + 24 * s);
            const bool hb = u.hover(ba, bb) && !m_cloudSending;
            dl->AddRectFilled(ba, bb, hb ? C_CARD_HI : C_CARD, 13 * s);
            stIcon(dl, ST_SYNC, ImVec2(ba.x + 16 * s, (ba.y + bb.y) * 0.5f), 13 * s, m_cloudSending ? C_ACCENT : C_TEXT, 1.4f * s);
            u.text(ImVec2(ba.x + 28 * s, (ba.y + bb.y) * 0.5f - u.fs(11) * 0.55f), 11, m_cloudSending ? C_DIM : C_TEXT, lab, true);
            if (hb && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Envoyer la scène au cloud maintenant");
            if (hb && u.pressed) {
                clicked = true;
                m_cloudSentKey.clear();   // (sent even when only its name changed)
                cloudSend();
                cloudRefreshList();
            }
        }
        y += ch + 16 * s;
    }
    {   // the actions
        struct Act { const char* ico; const char* label; const char* key; int id; bool primary; };
        std::vector<Act> acts = {{ST_SAVE, "Enregistrer", m_ipad ? "" : "Ctrl+S", 0, true}};
        if (m_ipad) acts.push_back({ST_SHARE, "Partager (AirDrop, Mail, PC)", "", 4, false});
        else acts.push_back({ST_SAVE, "Enregistrer sous…", "Ctrl+Maj+S", 3, false});
        acts.push_back({ST_NEW, "Nouvelle scène", m_ipad ? "" : "Ctrl+N", 1, false});
        acts.push_back({ST_OPEN, m_ipad ? "Ouvrir (Fichiers)…" : "Ouvrir un fichier…", m_ipad ? "" : "Ctrl+O", 2, false});
        acts.push_back({ST_PRESET, "Presets…", "", 5, false});
        if (m_ipad) acts.push_back({ST_INFO, "Envoyer un rapport (bug)", "", 6, false});
        const float ih = 36 * s, room = b.y - pad - y;
        const int fit = std::max(1, (int)(room / (ih + 4 * s)));
        for (int k = 0; k < (int)acts.size() && k < fit; ++k) {
            const Act& ac = acts[k];
            const ImVec2 ia(lx, y), ib(lx + LW, y + ih);
            const bool h = u.hover(ia, ib) && !editing;
            if (ac.primary) dl->AddRectFilled(ia, ib, h ? IM_COL32(255, 212, 40, 255) : C_ACCENT, 10 * s);
            else if (h) dl->AddRectFilled(ia, ib, IM_COL32(255, 255, 255, 16), 10 * s);
            const ImU32 fg = ac.primary ? C_INK : C_TEXT;
            stIcon(dl, ac.ico, ImVec2(ia.x + 20 * s, (ia.y + ib.y) * 0.5f), 16 * s, fg, 1.5f * s);
            u.text(ImVec2(ia.x + 40 * s, (ia.y + ib.y) * 0.5f - u.fs(11.5f) * 0.55f), 11.5f, fg, ac.label, ac.primary);
            if (ac.key[0]) u.text(ImVec2(ib.x - 12 * s - u.textW(9.5f, ac.key), (ia.y + ib.y) * 0.5f - u.fs(9.5f) * 0.55f), 9.5f, ac.primary ? stA(C_INK, 0.6f) : C_FAINT, ac.key);
            if (h && u.pressed) {
                clicked = true;
                switch (ac.id) {
                case 0: if (m_scenePath.empty() || inLib) hubSaveHere(); else saveNow(); break;
                case 3: saveDialog(); break;
                case 4: shareScene(); break;
                case 1: keepOpen(); newScene(); close = true; break;
                case 2: keepOpen(); openDialog(); close = true; break;
                case 5: m_popup = 10; m_presetsKey.clear(); m_popupAt = ImVec2(10 * s, 54 * s); break;
                case 6: sendReport(); break;
                }
            }
            y += ih + 4 * s;
        }
    }
    dl->AddLine(ImVec2(lx + LW + pad * 0.5f, top + 4 * s), ImVec2(lx + LW + pad * 0.5f, b.y - pad), IM_COL32(255, 255, 255, 12), 1.0f);

    // ======== right: the scenes
    const float rx0 = lx + LW + pad, rx1 = b.x - pad;
    y = top + 4 * s;
    {   // the tabs
        const int nCloud = m_cloud.signedIn() ? (int)m_cloudList.size() : -1;
        const std::string t0 = "SUR CET APPAREIL  " + std::to_string(m_hubLocal.size());
        const std::string t1 = nCloud >= 0 ? "CLOUD  " + std::to_string(nCloud) : std::string("CLOUD");
        const char* labs[3] = {t0.c_str(), t1.c_str(), "DÉMOS"};
        float x = rx0;
        for (int k = 0; k < 3; ++k) {
            const float tw = stTrackW(u, 8.5f, labs[k], 1.5f * s, true) + 28 * s;
            const ImVec2 ta(x, y), tb(x + tw, y + 32 * s);
            const bool on = m_hubTab == k, h = u.hover(ta, tb) && !editing;
            if (on) { dl->AddRectFilled(ta, tb, IM_COL32(246, 196, 0, 26), 16 * s); dl->AddRect(ta, tb, IM_COL32(246, 196, 0, 150), 16 * s, 0, 1.0f); }
            else if (h) dl->AddRectFilled(ta, tb, IM_COL32(255, 255, 255, 12), 16 * s);
            stTrack(u, ImVec2(ta.x + 14 * s, (ta.y + tb.y) * 0.5f - u.fs(8.5f) * 0.52f), 8.5f, on ? C_ACCENT_TXT : (h ? C_TEXT : C_DIM), labs[k], 1.5f * s);
            if (h && u.pressed) {
                clicked = true;
                m_hubTab = k;
                m_hubScroll = 0;
                if (k == 1) cloudRefreshList();
                if (k == 0) m_hubStale = true;
            }
            x += tw + 6 * s;
        }
        if (m_hubTab == 1 && m_cloud.signedIn()) {   // refresh
            const ImVec2 c(rx1 - 16 * s, y + 16 * s);
            const bool h = u.hover(ImVec2(c.x - 16 * s, c.y - 16 * s), ImVec2(c.x + 16 * s, c.y + 16 * s));
            dl->AddCircleFilled(c, 15 * s, IM_COL32(255, 255, 255, h ? 22 : 8), 24);
            stIcon(dl, ST_SYNC, c, 15 * s, m_cloudListBusy ? C_ACCENT : C_DIM, 1.4f * s);
            if (h && ImGui::GetIO().MouseDownDuration[0] < 0) ImGui::SetTooltip("Rafraîchir");
            if (h && u.pressed) { clicked = true; cloudRefreshList(); }
        }
        y += 32 * s + 16 * s;
    }
    // the grid of the cards
    const ImVec2 ga(rx0, y), gb(rx1, b.y - pad);
    const float gap = 14 * s, gw = gb.x - ga.x;
    const int cols = std::max(2, (int)((gw + gap) / (230 * s + gap)));
    const float cw = std::floor((gw - (cols - 1) * gap) / cols), thH = std::floor(cw * 0.625f), cH = thH + 54 * s;
    struct Card { int kind; int i; };   // kind 0 a scene here, 1 a scene of the cloud, 2 a demo, 3 « Nouvelle scène »
    std::vector<Card> cards;
    if (m_hubTab == 0) {
        cards.push_back({3, 0});
        for (int i = 0; i < (int)m_hubLocal.size(); ++i) cards.push_back({0, i});
    } else if (m_hubTab == 1) {
        if (m_cloud.signedIn()) for (int i = 0; i < (int)m_cloudList.size(); ++i) cards.push_back({1, i});
    } else {
        cards.push_back({2, 0});
        cards.push_back({2, 1});
    }
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
        const ImVec2 ta = ca, tb(cb.x, ca.y + thH);
        const bool h = inGrid && u.hover(ca, cb) && !editing;
        if (cd.kind == 3) {   // + Nouvelle scène
            dl->AddRectFilled(ca, cb, IM_COL32(255, 255, 255, h ? 14 : 5), 14 * s);
            for (float t = 0; t < 1; t += 0.04f) {   // a dashed frame
                auto P = [&](float q) {
                    const float per = 2 * (cw + cH);
                    float l = q * per;
                    if (l < cw) return ImVec2(ca.x + l, ca.y);
                    l -= cw;
                    if (l < cH) return ImVec2(cb.x, ca.y + l);
                    l -= cH;
                    if (l < cw) return ImVec2(cb.x - l, cb.y);
                    l -= cw;
                    return ImVec2(ca.x, cb.y - l);
                };
                dl->AddLine(P(t), P(t + 0.02f), IM_COL32(255, 255, 255, h ? 70 : 34), 1.2f * s);
            }
            const ImVec2 c((ca.x + cb.x) * 0.5f, (ca.y + cb.y) * 0.5f - 10 * s);
            dl->AddCircleFilled(c, 22 * s, h ? C_ACCENT : IM_COL32(255, 255, 255, 18), 32);
            stIcon(dl, ST_NEW, c, 20 * s, h ? C_INK : C_TEXT, 1.8f * s);
            const char* t = "NOUVELLE SCÈNE";
            stTrack(u, ImVec2(c.x - stTrackW(u, 8.5f, t, 1.4f * s, true) * 0.5f, c.y + 32 * s), 8.5f, h ? C_TEXT : C_DIM, t, 1.4f * s);
            if (h && u.pressed) { clicked = true; keepOpen(); newScene(); close = true; }
            continue;
        }
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
            sub = (c.device.empty() ? std::string() : c.device + " · ") + Cloud::localDate(c.updated) + " · " + std::to_string(c.bricks) + " pièces";
            cur = c.id == m_cloudId;
            ico = ST_CLOUD;
        } else {
            name = cd.i == 0 ? "Maison (démo)" : "Pelleteuse et mécanismes";
            sub = cd.i == 0 ? "Le set 31153 et ses figurines" : "4 moteurs, 2 vérins : passe en Mécanique";
            ico = cd.i == 0 ? "M4 11 12 4l8 7v9H4zM10 20v-5h4v5" : ST_MECH;
        }
        // the card
        dl->AddRectFilled(ca, cb, h ? IM_COL32(40, 42, 48, 255) : IM_COL32(28, 29, 33, 255), 14 * s);
        if (tex) dl->AddImageRounded(tex, ta, tb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 14 * s, ImDrawFlags_RoundCornersTop);
        else noThumb(ta, tb, 14 * s, ImDrawFlags_RoundCornersTop, ico);
        if (h && tex) dl->AddRectFilled(ta, tb, IM_COL32(255, 255, 255, 12), 14 * s, ImDrawFlags_RoundCornersTop);
        dl->AddRect(ca, cb, cur ? IM_COL32(246, 196, 0, 200) : IM_COL32(255, 255, 255, h ? 40 : 18), 14 * s, 0, cur ? 1.5f * s : 1.0f);
        if (cur) {   // OUVERTE
            const char* t = "OUVERTE";
            const float w = stTrackW(u, 7, t, 1.2f * s, true) + 16 * s;
            const ImVec2 pa(ta.x + 10 * s, ta.y + 10 * s), pb(pa.x + w, pa.y + 20 * s);
            dl->AddRectFilled(pa, pb, C_ACCENT, 10 * s);
            stTrack(u, ImVec2(pa.x + 8 * s, (pa.y + pb.y) * 0.5f - u.fs(7) * 0.52f), 7, C_INK, t, 1.2f * s);
        }
        if (cloudMark && !h && !m_ipad) {
            const ImVec2 c(tb.x - 20 * s, ta.y + 20 * s);
            dl->AddCircleFilled(c, 13 * s, IM_COL32(12, 13, 15, 200), 20);
            stIcon(dl, ST_CLOUD, c, 15 * s, IM_COL32(76, 190, 110, 255), 1.4f * s);
        }
        // the name (renamed in place), the date and the pieces
        const float ny = tb.y + 8 * s;
        if (cd.kind == 0 && m_hubEdit == cd.i) {
            const int r = editField(ImVec2(ca.x + 6 * s, ny - 2 * s), ImVec2(cb.x - 6 * s, ny + 22 * s), 12.5f);
            if (r) { clicked = true; finishEdit(r); }
        } else u.text(ImVec2(ca.x + 12 * s, ny), 12.5f, C_TEXT, stFit(u, 12.5f, name, cw - 24 * s, true).c_str(), true);
        u.text(ImVec2(ca.x + 12 * s, ny + 22 * s), 9.5f, C_DIM, stFit(u, 9.5f, sub, cw - 24 * s, false).c_str());
        // the actions on the card (hovered)
        bool act = false;
        const bool acts = h || (m_ipad && !editing);   // (iPad: no hover, the buttons always there)
        if (acts && cd.kind == 0) {
            float bx = tb.x - 20 * s;
            const float by = ta.y + 20 * s, r = 14 * s;
            if (roundBtn(ImVec2(bx, by), r, ST_DEL, "Supprimer", true)) { act = true; hubDelete(cd.i); }
            bx -= 2 * r + 6 * s;
            if (roundBtn(ImVec2(bx, by), r, ST_DUP, "Dupliquer")) { act = true; hubDuplicate(cd.i); }
            bx -= 2 * r + 6 * s;
            if (roundBtn(ImVec2(bx, by), r, ST_PEN, "Renommer")) { act = true; startEdit(cd.i, name); }
        } else if (acts && cd.kind == 1) {
            if (roundBtn(ImVec2(tb.x - 20 * s, ta.y + 20 * s), 14 * s, ST_DEL, "Supprimer du cloud", true)) {
                act = true;
                const CloudScene cs = m_cloudList[cd.i];
                plat::confirm(L"Supprimer du cloud", plat::fromUtf8("Supprimer « " + (cs.name.empty() ? std::string("Ma scène") : cs.name) + " » du cloud ? (Sur cet appareil, rien ne change.)"),
                              [this, cs] {
                                  m_cloud.remove(cs.id, [this, cs](bool ok, const std::string& err) {
                                      if (!ok) { toast("Cloud : " + err); return; }
                                      if (cs.id == m_cloudId) cloudForget();
                                      toast("Cloud : « " + cs.name + " » supprimée");
                                      cloudRefreshList();
                                  });
                              });
            }
        }
        if (act) { clicked = true; continue; }
        if (h && u.pressed) {   // open it
            clicked = true;
            if (cd.kind == 0) {
                const HubScene sc = m_hubLocal[cd.i];
                if (sc.path != m_scenePath) {
                    keepOpen();
                    if (openScene(sc.path)) { m_modelName = sc.name; close = true; }
                } else close = true;
            } else if (cd.kind == 1) {
                const CloudScene c = m_cloudList[cd.i];
                if (!cur) { keepOpen(); cloudOpen(c, false); }
                close = true;
            } else {
                keepOpen();
                if (cd.i == 0) openOfficialHouse();
                else { newScene(); buildExcavatorDemo(); rebuildScene(); frameAll(); m_scene.camera.pitch = radians(24.0f); m_scene.camera.yaw = radians(-30.0f); m_modelName = "Pelleteuse et mécanismes"; toast("Démo : pelleteuse et mécanismes (passe en Mécanique : 4 moteurs, 2 vérins)"); }
                close = true;
            }
        }
    }
    dl->PopClipRect();
    if (cards.size() <= 1 && m_hubTab != 2) {   // nothing yet
        const char* t = m_hubTab == 0 ? "Aucune scène enregistrée ici pour l'instant. « Enregistrer » range la scène ouverte dans Mes scènes, avec sa vignette."
                      : !m_cloud.signedIn() ? "Connecte-toi (à gauche) pour retrouver tes scènes de l'iPad et du PC."
                      : m_cloudListBusy ? "Chargement…" : !m_cloudListErr.empty() ? m_cloudListErr.c_str() : "Aucune scène dans le cloud pour l'instant : la tienne y sera après ta prochaine modification.";
        const float ty = m_hubTab == 0 ? ga.y + cH + gap + 10 * s : ga.y + 10 * s;
        u.text(ImVec2(ga.x + 4 * s, ty), 11, C_DIM, t);
    }
    if (contentH > gb.y - ga.y) {   // the scroll bar
        const float vh = gb.y - ga.y, bh = std::max(30 * s, vh * vh / contentH), by = ga.y + (vh - bh) * (m_hubScroll / (contentH - vh));
        dl->AddRectFilled(ImVec2(gb.x + 8 * s, by), ImVec2(gb.x + 11 * s, by + bh), IM_COL32(255, 255, 255, 50), 2 * s);
    }
    // closing: ×, Échap, a click outside the panel
    if (!editing && ImGui::IsKeyPressed(ImGuiKey_Escape)) close = true;
    if (u.pressed && !clicked && !u.hover(a, b) && !editing) close = true;
    if (close && m_popup == 8) m_popup = 0;
}
