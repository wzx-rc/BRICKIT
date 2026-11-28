// ArchViz - the library of objects (Furnish), on the base of the library of BrickIt (BrickUiStudio.inl: stDrawer,
// stLibGrip, stLibPressEnd) with the same behaviours:
//   - docked on the left, on the right or at the bottom; its grip resizes it, dragged to an edge of the screen it docks
//     there (a ghost shows where), a click on the grip folds it
//   - the search, the categories, a grid of thumbnails scrolling down
//   - a card: clicked = the object in hand (the next click in a room places it); dragged out of the library = in hand
//     under the cursor, dropped where the button is released; held = its sheet
#include "ArchApp.h"
#include "ArchUi.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace archviz {

using namespace vp3d;

static std::string lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

static std::string trisText(int tris) {
    char b[32];
    if (tris >= 1000000) snprintf(b, sizeof b, "%.1f M triangles", tris / 1e6);
    else if (tris >= 1000) snprintf(b, sizeof b, "%d k triangles", tris / 1000);
    else snprintf(b, sizeof b, "%d triangles", tris);
    return b;
}

// The thumbnail of an asset (thumb.png of its folder), read the first time it shows; 0 = none.
ImTextureID ArchApp::thumbOf(int i) {
    if (m_thumbs.size() != m_assets.size()) { m_thumbs.assign(m_assets.size(), (ImTextureID)0); m_thumbTried.assign(m_assets.size(), 0); }
    if (i < 0 || i >= (int)m_assets.size()) return (ImTextureID)0;
    static int loadedAtFrame = -1;
    if (!m_thumbTried[i] && loadedAtFrame != m_frame) {   // (one per frame: no hitch when the library opens)
        loadedAtFrame = m_frame;
        m_thumbTried[i] = 1;
        std::vector<uint8_t> rgba;
        int w = 0, h = 0;
        if (loadThumb(m_assets[i], rgba, w, h)) m_thumbs[i] = m_g->createTexture(rgba.data(), (uint32_t)w, (uint32_t)h);
    }
    return m_thumbs[i];
}

// No thumbnail yet: the footprint of the object and its height, as a small drawing.
void ArchApp::drawThumbFallback(ImDrawList* dl, ImVec2 a, ImVec2 b, const AssetInfo& as) {
    const float w = b.x - a.x, h = b.y - a.y;
    const float m = std::max(as.size.x, std::max(as.size.y, as.size.z));
    const float sx = as.size.x / m * w * 0.62f, sy = as.size.y / m * h * 0.62f, sz = as.size.z / m * w * 0.22f;
    const ImVec2 c((a.x + b.x) * 0.5f - sz * 0.5f, (a.y + b.y) * 0.5f + sz * 0.3f);
    const ImVec2 f0(c.x - sx * 0.5f, c.y - sy * 0.5f), f1(c.x + sx * 0.5f, c.y + sy * 0.5f);
    const ImVec2 o(sz, -sz * 0.6f);
    const ImU32 line = IM_COL32(154, 162, 175, 255);
    const ImVec2 top[4] = {f0, ImVec2(f0.x + o.x, f0.y + o.y), ImVec2(f1.x + o.x, f0.y + o.y), ImVec2(f1.x, f0.y)};
    const ImVec2 side[4] = {ImVec2(f1.x, f0.y), ImVec2(f1.x + o.x, f0.y + o.y), ImVec2(f1.x + o.x, f1.y + o.y), f1};
    dl->AddConvexPolyFilled(top, 4, IM_COL32(244, 246, 249, 255));
    dl->AddConvexPolyFilled(side, 4, IM_COL32(220, 225, 232, 255));
    dl->AddRectFilled(f0, f1, IM_COL32(233, 237, 242, 255));
    dl->AddPolyline(top, 4, line, ImDrawFlags_Closed, 1.2f * m_ui);
    dl->AddPolyline(side, 4, line, ImDrawFlags_Closed, 1.2f * m_ui);
    dl->AddRect(f0, f1, line, 0, 0, 1.2f * m_ui);
}

void ArchApp::drawLibrary() {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 ds = io.DisplaySize;
    const float k = m_ui, M = 14 * k;
    ImFont* bold = m_g->fontBold;
    const bool bottom = m_libDock == 0;

    // ---- where it is docked
    const float top = 90 * k + (m_b.levels.size() * 32 + 78) * k;   // (on the left: under the levels)
    const float W = std::clamp(m_libW * k, 220 * k, ds.x * 0.5f);
    const float H = m_libFold ? 44 * k : std::clamp(m_libH * k, 150 * k, ds.y * 0.6f);
    ImVec2 a, b;
    if (m_libDock == 1) { a = ImVec2(M, top); b = ImVec2(M + W, ds.y - M); }
    else if (m_libDock == 2) { a = ImVec2(ds.x - M - W, 106 * k); b = ImVec2(ds.x - M, ds.y - M); }
    else { a = ImVec2(M, ds.y - M - H); b = ImVec2(ds.x - M, ds.y - M); }
    m_libRect = ImVec4(a.x, a.y, b.x, b.y);

    ImGui::SetNextWindowPos(a);
    ImGui::SetNextWindowSize(ImVec2(b.x - a.x, b.y - a.y));
    const ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("##library", nullptr, fl);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 ink2 = kInk2, ink3 = kInk3;

    // ---- the header: its name, the count, the search; then the categories
    std::vector<std::string> cats;
    for (const AssetInfo& as : m_assets) {
        const std::string c = as.category.empty() ? "Other" : as.category;
        if (std::find(cats.begin(), cats.end(), c) == cats.end()) cats.push_back(c);
    }
    std::sort(cats.begin(), cats.end());
    const std::string needle = lower(m_libSearch);
    std::vector<int> shown;
    for (int i = 0; i < (int)m_assets.size(); ++i) {
        const AssetInfo& as = m_assets[i];
        const std::string c = as.category.empty() ? "Other" : as.category;
        if (!m_libCat.empty() && c != m_libCat) continue;
        if (!needle.empty() && lower(as.name).find(needle) == std::string::npos && lower(as.slug).find(needle) == std::string::npos && lower(c).find(needle) == std::string::npos) continue;
        shown.push_back(i);
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ink3), "LIBRARY");
    ImGui::SameLine(0, 8 * k);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ink3), "%d", (int)shown.size());
    ImGui::SameLine(0, 12 * k);
    ImGui::SetNextItemWidth(bottom ? 220 * k : -1.0f);
    ImGui::InputTextWithHint("##libsearch", "Search", m_libSearch, sizeof m_libSearch);
    if (!m_libFold || !bottom) {
        // the categories: chips on one or more lines (at the bottom: beside the search)
        const float right = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x;
        if (bottom) ImGui::SameLine(0, 12 * k);
        for (int c = -1; c < (int)cats.size(); ++c) {
            const char* name = c < 0 ? "All" : cats[c].c_str();
            const bool on = c < 0 ? m_libCat.empty() : m_libCat == cats[c];
            const ImVec2 ts = ImGui::CalcTextSize(name);
            const ImVec2 sz(ts.x + 18 * k, 24 * k);
            if (c >= 0) {
                const float x = ImGui::GetItemRectMax().x + 5 * k;
                if (x + sz.x <= right) ImGui::SameLine(0, 5 * k);
            }
            ImGui::PushID(c);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("cat", sz)) m_libCat = c < 0 ? "" : cats[c];
            const bool hv = ImGui::IsItemHovered();
            dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), on ? kAccent : hv ? kHollow : IM_COL32(244, 246, 249, 255), 12 * k);
            if (!on) dl->AddRect(p, ImVec2(p.x + sz.x, p.y + sz.y), kLine, 12 * k);
            dl->AddText(ImVec2(p.x + 9 * k, p.y + (sz.y - ts.y) * 0.5f), on ? kWhite : ink2, name);
            ImGui::PopID();
        }
    }

    // ---- the grid of cards
    if (!(m_libFold && bottom)) {
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 7 * k);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImGui::ColorConvertU32ToFloat4(IM_COL32(205, 211, 221, 255)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImGui::ColorConvertU32ToFloat4(IM_COL32(176, 184, 197, 255)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImGui::ColorConvertU32ToFloat4(kInk3));
        ImGui::BeginChild("##libgrid", ImVec2(0, 0), 0, ImGuiWindowFlags_NoNav);
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        const float avail = ImGui::GetContentRegionAvail().x - 10 * k, gap = 6 * k;   // (room for the scrollbar)
        const int cols = std::max(1, (int)std::floor((avail + gap) / (84 * k + gap)));
        const float cw = std::floor((avail - gap * (cols - 1)) / cols), ch = cw + 38 * k;
        if (m_assets.empty()) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ink2), "No object yet.\nConverted models go in\nBrickIt/ArchViz/models.");
        else if (shown.empty()) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ink2), "Nothing matches.");
        const float fs = ImGui::GetFontSize();
        for (int n = 0; n < (int)shown.size(); ++n) {
            const int i = shown[n];
            const AssetInfo& as = m_assets[i];
            if (n % cols) ImGui::SameLine(0, gap);
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImVec2 q(p.x + cw, p.y + ch);
            ImGui::InvisibleButton("card", ImVec2(cw, ch));
            const bool hv = ImGui::IsItemHovered(), vis = ImGui::IsItemVisible();
            if (ImGui::IsItemActivated()) {   // the press: what it becomes is decided by libPressEnd
                m_libPress = i;
                m_libPressPos = io.MousePos;
                m_libPressTime = ImGui::GetTime();
                m_libLong = false;
            }
            if (vis) {
                const bool on = (m_tool == kPlace && m_placeAsset == i) || m_libPress == i || m_libInfo == i;
                cdl->AddRectFilled(p, q, on ? kWhite : hv ? IM_COL32(246, 248, 251, 255) : IM_COL32(250, 251, 252, 255), 10 * k);
                cdl->AddRect(p, q, on ? alpha(kAccent, 0.75f) : hv ? IM_COL32(205, 212, 222, 255) : kLine, 10 * k, 0, on ? 1.6f * k : 1.0f);
                const float pad = 6 * k;
                const ImVec2 t0(p.x + pad, p.y + pad), t1(q.x - pad, p.y + cw - pad);
                const ImTextureID tex = thumbOf(i);
                if (tex) cdl->AddImage(tex, t0, t1);
                else drawThumbFallback(cdl, t0, t1, as);
                // its name (cut to the card), its size under it
                std::string name = as.name;
                ImFont* f = bold ? bold : ImGui::GetFont();
                const float nfs = fs * 0.92f, maxW = cw - 12 * k;
                bool cut = false;
                while (name.size() > 1 && f->CalcTextSizeA(nfs, FLT_MAX, 0, (name + (cut ? ".." : "")).c_str()).x > maxW) { name.pop_back(); cut = true; }
                if (cut) name += "..";
                cdl->AddText(f, nfs, ImVec2(p.x + 7 * k, p.y + cw - 2 * k), kInk, name.c_str());
                char sub[48];
                snprintf(sub, sizeof sub, "%.2f x %.2f m", as.size.x, as.size.z);
                cdl->AddText(ImGui::GetFont(), fs * 0.8f, ImVec2(p.x + 7 * k, p.y + cw - 2 * k + nfs + 2 * k), ink3, sub);
            }
            if (hv && m_libPress < 0 && io.MouseDownDuration[0] < 0)
                ImGui::SetTooltip("%s\n%.2f x %.2f m, %.2f m high\n%s\nClick: in hand  \xc2\xb7  drag it into a room  \xc2\xb7  hold: its sheet", as.name.c_str(), as.size.x, as.size.z, as.size.y,
                                  trisText(as.tris).c_str());
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
    }

    // ---- the grip (BrickIt: stLibGrip): resize, dock on another edge, fold
    {
        const ImVec2 hc = bottom ? ImVec2((a.x + b.x) * 0.5f, a.y + 6.5f * k) : ImVec2(m_libDock == 1 ? b.x - 6 * k : a.x + 6 * k, (a.y + b.y) * 0.5f);
        const ImVec2 za = bottom ? ImVec2(hc.x - 70 * k, a.y) : ImVec2(hc.x - 8 * k, hc.y - 70 * k);
        const ImVec2 zb = bottom ? ImVec2(hc.x + 70 * k, a.y + 13 * k) : ImVec2(hc.x + 8 * k, hc.y + 70 * k);
        const ImVec2 m = io.MousePos;
        const bool hv = m_libPress < 0 && m.x >= za.x && m.x <= zb.x && m.y >= za.y && m.y <= zb.y && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        const ImU32 hcol = m_libGrip || hv ? alpha(kAccent, 0.9f) : IM_COL32(190, 197, 208, 255);
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        if (bottom) fg->AddRectFilled(ImVec2(hc.x - 22 * k, hc.y - 1.5f * k), ImVec2(hc.x + 22 * k, hc.y + 1.5f * k), hcol, 2 * k);
        else fg->AddRectFilled(ImVec2(hc.x - 1.5f * k, hc.y - 22 * k), ImVec2(hc.x + 1.5f * k, hc.y + 22 * k), hcol, 2 * k);
        if (hv && !m_libGrip) {
            ImGui::SetMouseCursor(bottom ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            if (io.MouseDownDuration[0] < 0) ImGui::SetTooltip("Drag: resize  \xc2\xb7  to an edge: dock left, right or bottom  \xc2\xb7  click: fold");
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_libGrip = 1;
                m_libGripAt = m;
                m_libGrip0 = bottom ? m_libH : m_libW;
                m_libDockPreview = -1;
            }
        }
        if (m_libGrip) {
            ImGui::SetMouseCursor(bottom ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            const bool moved = std::fabs(m.x - m_libGripAt.x) + std::fabs(m.y - m_libGripAt.y) > 5 * k;
            int prev = -1;
            if (bottom) {
                if (m.y < ds.y * 0.8f && m.x < ds.x * 0.1f) prev = 1;
                else if (m.y < ds.y * 0.8f && m.x > ds.x * 0.9f) prev = 2;
                else if (moved && !m_libFold) m_libH = std::clamp(m_libGrip0 + (m_libGripAt.y - m.y) / k, 150.0f, ds.y * 0.6f / k);
            } else {
                if (m.y > ds.y * 0.86f) prev = 0;
                else if (m_libDock == 1 && m.x > ds.x * 0.9f) prev = 2;
                else if (m_libDock == 2 && m.x < ds.x * 0.1f) prev = 1;
                else if (moved) m_libW = std::clamp(m_libGrip0 + (m_libDock == 1 ? m.x - m_libGripAt.x : m_libGripAt.x - m.x) / k, 220.0f, ds.x * 0.5f / k);
            }
            m_libDockPreview = prev;
            if (prev >= 0) {   // the ghost of where it docks
                const ImVec2 ga = prev == 0 ? ImVec2(M, ds.y - M - std::clamp(m_libH * k, 150 * k, ds.y * 0.6f)) : prev == 1 ? ImVec2(M, top) : ImVec2(ds.x - M - W, 106 * k);
                const ImVec2 gb = prev == 0 ? ImVec2(ds.x - M, ds.y - M) : prev == 1 ? ImVec2(M + W, ds.y - M) : ImVec2(ds.x - M, ds.y - M);
                fg->AddRectFilled(ga, gb, alpha(kAccent, 0.12f), 14 * k);
                fg->AddRect(ga, gb, alpha(kAccent, 0.8f), 14 * k, 0, 2 * k);
                const char* t = prev == 0 ? "LIBRARY AT THE BOTTOM" : prev == 1 ? "LIBRARY ON THE LEFT" : "LIBRARY ON THE RIGHT";
                const ImVec2 ts = ImGui::CalcTextSize(t);
                fg->AddText(ImVec2((ga.x + gb.x - ts.x) * 0.5f, (ga.y + gb.y - ts.y) * 0.5f), kAccent, t);
            }
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (prev >= 0 && prev != m_libDock) { m_libDock = prev; m_libFold = false; }
                else if (!moved) m_libFold = bottom ? !m_libFold : false;
                m_libGrip = 0;
                m_libDockPreview = -1;
                savePrefs();
            }
        }
    }
    ImGui::End();
    libPressEnd();

    // ---- the sheet of an object (a card held): beside the library
    if (m_libInfo >= 0 && m_libInfo < (int)m_assets.size()) {
        const AssetInfo& as = m_assets[m_libInfo];
        const ImVec2 at = m_libDock == 1 ? ImVec2(b.x + 8 * k, a.y) : m_libDock == 2 ? ImVec2(a.x - 8 * k, a.y) : ImVec2(a.x, a.y - 8 * k);
        ImGui::SetNextWindowPos(at, 0, m_libDock == 1 ? ImVec2(0, 0) : m_libDock == 2 ? ImVec2(1, 0) : ImVec2(0, 1));
        ImGui::Begin("##libsheet", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);
        ImDrawList* sdl = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float g = 200 * k;
        sdl->AddRectFilled(p, ImVec2(p.x + g, p.y + g), IM_COL32(246, 248, 251, 255), 10 * k);
        const ImTextureID tex = thumbOf(m_libInfo);
        if (tex) sdl->AddImage(tex, ImVec2(p.x + 6 * k, p.y + 6 * k), ImVec2(p.x + g - 6 * k, p.y + g - 6 * k));
        else drawThumbFallback(sdl, ImVec2(p.x + 20 * k, p.y + 20 * k), ImVec2(p.x + g - 20 * k, p.y + g - 20 * k), as);
        ImGui::Dummy(ImVec2(g, g));
        if (bold) ImGui::PushFont(bold, 0.0f);
        ImGui::TextUnformatted(as.name.c_str());
        if (bold) ImGui::PopFont();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ink2), "%s\n%.2f x %.2f m, %.2f m high\n%s\n%d parts, %d textures", as.category.empty() ? "Other" : as.category.c_str(), as.size.x, as.size.z,
                           as.size.y, trisText(as.tris).c_str(), as.parts, as.textures);
        ImGui::Spacing();
        if (ImGui::Button("Take it in hand", ImVec2(g, 0))) {
            m_tool = kPlace; m_placeAsset = m_libInfo; m_libInfo = -1;
            m_selKind = 0; m_selItem = -1; m_selDirty = true;
        }
        const bool over = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        ImGui::End();
        if (m_libInfo >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !over && m_libPress < 0) m_libInfo = -1;   // a click elsewhere closes it
    }
}

// A press on a card (BrickIt: stLibPressEnd): held = its sheet; dragged out of the library = in hand under the cursor
// (dropped on release); a click = in hand.
void ArchApp::libPressEnd() {
    if (m_libPress < 0 || m_libPress >= (int)m_assets.size()) { m_libPress = -1; return; }
    ImGuiIO& io = ImGui::GetIO();
    const float k = m_ui;
    const double now = ImGui::GetTime();
    const ImVec2 m = io.MousePos;
    const bool inside = inRect(m, m_libRect);
    const ImVec2 dm(m.x - m_libPressPos.x, m.y - m_libPressPos.y);
    const bool moved = dm.x * dm.x + dm.y * dm.y > 49 * k * k;
    const bool down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (!m_libLong && !moved && down && now - m_libPressTime > 0.48) { m_libLong = true; m_libInfo = m_libPress; }
    auto inHand = [&](bool dragged) {
        m_tool = kPlace;
        m_placeAsset = m_libPress;
        m_libDragPlace = dragged;
        m_libInfo = -1;
        m_drawing = false;
        m_selKind = 0; m_selWall = m_selOp = m_selRoom = m_selItem = -1;
        m_selDirty = true;
    };
    if (moved && down && inside) {   // still over the library: its thumbnail follows the cursor
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const float g = 84 * k;
        const ImVec2 c(m.x, m.y - g * 0.35f), p0(c.x - g * 0.5f, c.y - g * 0.5f), p1(c.x + g * 0.5f, c.y + g * 0.5f);
        fg->AddRectFilled(ImVec2(p0.x, p0.y + 3 * k), ImVec2(p1.x, p1.y + 3 * k), IM_COL32(31, 39, 51, 40), 14 * k);
        fg->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 240), 14 * k);
        fg->AddRect(p0, p1, kAccent, 14 * k, 0, 1.4f * k);
        const ImTextureID tex = thumbOf(m_libPress);
        if (tex) fg->AddImage(tex, ImVec2(p0.x + 5 * k, p0.y + 5 * k), ImVec2(p1.x - 5 * k, p1.y - 5 * k));
        else drawThumbFallback(fg, ImVec2(p0.x + 10 * k, p0.y + 10 * k), ImVec2(p1.x - 10 * k, p1.y - 10 * k), m_assets[m_libPress]);
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (moved && !inside && down) {
        inHand(true);
        m_libPress = -1;
    } else if (!down) {
        if (!m_libLong && !moved) inHand(false);
        m_libPress = -1;
    }
}

} // namespace archviz
