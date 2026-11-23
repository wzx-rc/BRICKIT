// BrickIt - the hub of the scenes (Wzx 15/11/2026): « Mes scènes », a folder of .brick in the data of the app, each with
// its thumbnail next to it (<scene>.brick.png, the 3D view without the interface, taken at each save). This file: the
// files (list, save, rename, duplicate, delete) and the thumbnails; the panel is BrickHub.inl (in BrickUi.cpp).
#include "BrickApp.h"
#include "Platform.h"
#include "Image.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>

namespace brickit {

std::wstring BrickApp::scenesDir() const {
    const std::wstring d = plat::dataDir() + L"\\Mes scenes";
    plat::makeDir(d);
    return d;
}

long long BrickApp::hubFileTime(const std::wstring& path) {
#ifdef _WIN32
    struct _stat64 st;
    if (_wstat64(path.c_str(), &st) != 0) return 0;
    return (long long)st.st_mtime;
#else
    struct stat st;
    if (stat(plat::nativePath(path).c_str(), &st) != 0) return 0;
    return (long long)st.st_mtime;
#endif
}

static bool hubMove(const std::wstring& from, const std::wstring& to) {
#ifdef _WIN32
    return MoveFileW(from.c_str(), to.c_str()) != 0;
#else
    return std::rename(plat::nativePath(from).c_str(), plat::nativePath(to).c_str()) == 0;
#endif
}

// a name as a file name: the characters a file system refuses become « _ »
static std::string hubFileName(const std::string& name) {
    std::string f;
    for (char c : name) f += (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || (unsigned char)c < 32) ? '_' : c;
    while (!f.empty() && (f.back() == ' ' || f.back() == '.')) f.pop_back();
    while (!f.empty() && f.front() == ' ') f.erase(f.begin());
    return f.empty() ? std::string("My scene") : f;
}

static bool hubInDir(const std::wstring& path, const std::wstring& dir) {
    return path.size() > dir.size() && path.compare(0, dir.size(), dir) == 0 && (path[dir.size()] == L'\\' || path[dir.size()] == L'/');
}

// a free path in « Mes scènes » for this name (« Maison », « Maison 2 »...), except the path given (the scene itself)
static std::wstring hubFreePath(const std::wstring& dir, const std::string& name, const std::wstring& self = L"") {
    std::wstring p = dir + L"\\" + plat::fromUtf8(name) + L".brick";
    for (int n = 2; p != self && GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; ++n) p = dir + L"\\" + plat::fromUtf8(name + " " + std::to_string(n)) + L".brick";
    return p;
}

static std::string hubStem(const std::wstring& path) {
    std::wstring n = path;
    const size_t k = n.find_last_of(L"\\/");
    if (k != std::wstring::npos) n = n.substr(k + 1);
    if (n.size() > 6 && n.compare(n.size() - 6, 6, L".brick") == 0) n = n.substr(0, n.size() - 6);
    return plat::toUtf8(n);
}

void BrickApp::hubRefresh() {
    m_hubStale = false;
    m_hubLocal.clear();
    const std::wstring dir = scenesDir();
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*.brick").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        HubScene sc;
        sc.path = dir + L"\\" + fd.cFileName;
        sc.name = hubStem(sc.path);
        sc.mtime = hubFileTime(sc.path);
        FILE* f = nullptr;
        if (_wfopen_s(&f, sc.path.c_str(), L"rb") == 0 && f) {   // its number of pieces: the "part" keys of its bricks
            std::string text;
            char buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
            fclose(f);
            for (size_t p = text.find("\"part\""); p != std::string::npos; p = text.find("\"part\"", p + 6)) ++sc.bricks;
        }
        m_hubLocal.push_back(sc);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(m_hubLocal.begin(), m_hubLocal.end(), [](const HubScene& a, const HubScene& b) { return a.mtime > b.mtime; });
}

// Enregistrer (the hub, Ctrl+S on a scene without a file): into « Mes scènes » under its name, its thumbnail with it
void BrickApp::hubSaveHere() {
    const std::wstring dir = scenesDir();
    if (!m_scenePath.empty() && hubInDir(m_scenePath, dir)) { saveScene(m_scenePath); return; }
    const std::string name = hubFileName(m_modelName.empty() ? std::string("My scene") : m_modelName);
    const std::wstring p = hubFreePath(dir, name);
    if (saveScene(p)) m_modelName = hubStem(p);
}

bool BrickApp::hubRename(int i, const std::string& name0) {
    const std::string name = hubFileName(name0);
    const std::wstring dir = scenesDir();
    std::wstring path = i >= 0 && i < (int)m_hubLocal.size() ? m_hubLocal[i].path : std::wstring();
    const bool open = i == -2 || (!path.empty() && path == m_scenePath);
    if (open) {
        m_modelName = name;
        m_cloudSentKey.clear();   // (the new name sent to the cloud with the scene)
        cloudNoteChange();
        if (path.empty() && hubInDir(m_scenePath, dir)) path = m_scenePath;
    }
    if (!path.empty()) {
        const std::wstring to = hubFreePath(dir, name, path);
        if (to != path) {
            if (!hubMove(path, to)) { toast("Rename failed (is the file open elsewhere?)"); return false; }
            hubMove(path + L".png", to + L".png");
            if (path == m_scenePath) { m_scenePath = to; m_modelName = hubStem(to); }
        }
    }
    m_hubStale = true;
    toast("Scene renamed: “" + name + "”");
    return true;
}

void BrickApp::hubDuplicate(int i) {
    if (i < 0 || i >= (int)m_hubLocal.size()) return;
    const HubScene sc = m_hubLocal[i];
    const std::wstring to = hubFreePath(scenesDir(), sc.name + " (copy)");
    if (!CopyFileW(sc.path.c_str(), to.c_str(), TRUE)) { toast("Duplicate: cannot write the copy"); return; }
    CopyFileW((sc.path + L".png").c_str(), (to + L".png").c_str(), TRUE);
    m_hubStale = true;
    toast("Copy: “" + hubStem(to) + "”");
}

void BrickApp::hubDelete(int i) {
    if (i < 0 || i >= (int)m_hubLocal.size()) return;
    const HubScene sc = m_hubLocal[i];
    plat::confirm(L"Delete the scene", plat::fromUtf8("Delete “" + sc.name + "” from this device? (Its cloud copy, if any, stays.)"), [this, sc] {
        DeleteFileW(sc.path.c_str());
        DeleteFileW((sc.path + L".png").c_str());
        if (sc.path == m_scenePath) m_scenePath.clear();
        m_hubStale = true;
        toast("“" + sc.name + "” deleted");
    });
}

// the thumbnail asked by a save: the 3D view just rendered (no interface, no gizmo), its centre at 16:10, 480 x 300
void BrickApp::hubTakeThumb() {
    if (m_thumbReq.empty()) return;
    const std::wstring path = m_thumbReq + L".png";
    m_thumbReq.clear();
    std::vector<uint8_t> img;
    uint32_t w = 0, h = 0;
    if (!m_renderer->readViewport(img, w, h) || w < 16 || h < 16) return;
    const uint32_t TW = 480, TH = 300;
    float cw = (float)w, ch = cw * TH / TW;
    if (ch > h) { ch = (float)h; cw = ch * TW / TH; }
    const float x0 = (w - cw) * 0.5f, y0 = (h - ch) * 0.5f, k = cw / TW;
    std::vector<uint8_t> out((size_t)TW * TH * 4);
    const int n = std::max(1, (int)k);
    for (uint32_t y = 0; y < TH; ++y)
        for (uint32_t x = 0; x < TW; ++x) {   // the average of the pixels under each one
            uint32_t acc[3] = {0, 0, 0}, cnt = 0;
            const int sx0 = (int)(x0 + x * k), sy0 = (int)(y0 + y * k);
            for (int dy = 0; dy < n; ++dy)
                for (int dx = 0; dx < n; ++dx) {
                    const int sx = std::min((int)w - 1, sx0 + dx), sy = std::min((int)h - 1, sy0 + dy);
                    const uint8_t* p = &img[((size_t)sy * w + sx) * 4];
                    acc[0] += p[0]; acc[1] += p[1]; acc[2] += p[2]; ++cnt;
                }
            uint8_t* o = &out[((size_t)y * TW + x) * 4];
            o[0] = (uint8_t)(acc[0] / cnt); o[1] = (uint8_t)(acc[1] / cnt); o[2] = (uint8_t)(acc[2] / cnt); o[3] = 255;
        }
    savePng(path, out, TW, TH);
    m_hubStale = true;
}

ImTextureID BrickApp::hubThumb(const std::wstring& scenePath) {
    const std::wstring png = scenePath + L".png";
    const long long t = hubFileTime(png);
    auto it = m_hubTex.find(png);
    if (it != m_hubTex.end() && it->second.first == t) return it->second.second;
    if (it != m_hubTex.end()) { if (it->second.second) m_gui->releaseTexture(it->second.second); m_hubTex.erase(it); }
    ImTextureID tex = 0;
    if (t) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, png.c_str(), L"rb") == 0 && f) {
            std::vector<uint8_t> data;
            uint8_t buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
            fclose(f);
            std::vector<uint8_t> rgba;
            int w = 0, h = 0;
            if (image::decode(data.data(), data.size(), rgba, w, h) && w > 0 && h > 0) tex = m_gui->createTexture(rgba.data(), (uint32_t)w, (uint32_t)h);
        }
    }
    m_hubTex[png] = {t, tex};
    return tex;
}

} // namespace brickit
