// BrickIt - export of the model to a WZX Engine scene (Wzx 24/11/2026: « un export du tracteur dans le WZX Engine, en
// modèle brut + shader »): the raw geometry, one OBJ per part shape (shared by its copies), and a .wzx scene that
// places each brick (position, quaternion) with the BrickIt material of its colour (base colour, roughness, coat,
// subsurface, bevel; the rubber of the tyres), the sun, the area lights and the camera of the view.
// Files: <project>/game/meshes/<name>/<part>.obj and <project>/game/scenes/<name>.wzx (mesh paths relative to the
// project, as the editor writes them).
#include "BrickApp.h"
#include "Platform.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>

namespace brickit {

namespace {

std::string num(double v) {
    if (std::fabs(v) < 1e-9) v = 0;
    char b[32];
    snprintf(b, sizeof(b), "%.6g", v);
    return b;
}
std::string vec(const float3& v) { return "[" + num(v.x) + ", " + num(v.y) + ", " + num(v.z) + "]"; }
std::string quote(const std::string& s) {
    std::string o = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') o += '\\';
        o += c;
    }
    return o + "\"";
}
std::string fileSafe(const std::string& s) {
    std::string o;
    for (char c : s) o += (isalnum((unsigned char)c) || c == '-' || c == '_') ? c : '_';
    return o.empty() ? std::string("part") : o;
}

bool writeObj(const std::filesystem::path& p, const MeshData& m, const std::string& name) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, p.wstring().c_str(), L"wb") || !f) return false;
    std::string s;
    s.reserve(m.vertices.size() * 90 + m.indices.size() * 12);
    s += "# BrickIt part " + name + " (cm, Y up)\no " + name + "\n";
    char b[128];
    for (const Vertex& v : m.vertices) { snprintf(b, sizeof(b), "v %.5f %.5f %.5f\n", v.pos.x, v.pos.y, v.pos.z); s += b; }
    for (const Vertex& v : m.vertices) { snprintf(b, sizeof(b), "vn %.4f %.4f %.4f\n", v.nrm.x, v.nrm.y, v.nrm.z); s += b; }
    for (const Vertex& v : m.vertices) { snprintf(b, sizeof(b), "vt %.5f %.5f\n", v.uv.x, v.uv.y); s += b; }
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        const uint32_t a = m.indices[i] + 1, c = m.indices[i + 1] + 1, d = m.indices[i + 2] + 1;
        snprintf(b, sizeof(b), "f %u/%u/%u %u/%u/%u %u/%u/%u\n", a, a, a, c, c, c, d, d, d);
        s += b;
    }
    const bool ok = fwrite(s.data(), 1, s.size(), f) == s.size();
    fclose(f);
    return ok;
}

} // namespace

bool BrickApp::wzxExport(const std::wstring& projectDir, const std::string& name, std::string& report) {
    namespace fs = std::filesystem;
    const std::string safe = fileSafe(name);
    const std::string meshRel = "game/meshes/" + safe + "/";
    const fs::path root(projectDir), meshDir = root / "game" / "meshes" / safe, sceneDir = root / "game" / "scenes";
    std::error_code ec;
    fs::create_directories(meshDir, ec);
    fs::create_directories(sceneDir, ec);

    std::map<int, std::string> meshFile;   // scene mesh -> its OBJ (relative to the project)
    std::map<int, int> matIndex;           // scene material -> index in the file
    std::vector<int> mats;
    std::set<std::string> used;
    std::string objects;
    int count = 0;
    for (size_t i = 0; i < m_bricks.size(); ++i) {
        const Object& o = m_scene.objects[m_firstBrickObject + i];
        if (o.mesh < 0 || o.mesh >= (int)m_scene.meshes.size() || m_scene.meshes[o.mesh].indices.empty()) continue;
        const Part& part = m_lib.parts[m_bricks[i].part];
        auto it = meshFile.find(o.mesh);
        if (it == meshFile.end()) {
            std::string file = fileSafe(part.name) + ".obj";
            for (int k = 2; used.count(file); ++k) file = fileSafe(part.name) + "_" + std::to_string(k) + ".obj";   // (the same part, another mesh)
            used.insert(file);
            if (!writeObj(meshDir / file, m_scene.meshes[o.mesh], part.name)) { report = "cannot write " + (meshDir / file).string(); return false; }
            it = meshFile.emplace(o.mesh, meshRel + file).first;
        }
        // the material of the colour, never the focus copy (50 %) nor the red of a blocked brick
        int mat = o.material;
        const int c = std::clamp(m_bricks[i].color, 0, (int)m_colorMaterial.size() - 1);
        if (mat != m_colorMaterial[c] && (c >= (int)m_rubberMaterial.size() || mat != m_rubberMaterial[c])) {
            const bool rubber = c < (int)m_rubberDim.size() && mat == m_rubberDim[c];
            mat = rubber ? m_rubberMaterial[c] : m_colorMaterial[c];
        }
        if (!matIndex.count(mat)) { matIndex[mat] = (int)mats.size(); mats.push_back(mat); }
        const float4 q = m_bricks[i].rot;
        char nm[160];
        snprintf(nm, sizeof(nm), "%s_%03d", part.name.c_str(), (int)i + 1);
        objects += std::string(count ? ",\n" : "") + "    {\"name\": " + quote(nm) + ", \"type\": \"MESH\", \"mesh\": " + quote(it->second) +
                   ", \"position\": " + vec(m_bricks[i].pos) + ", \"rotation\": [" + num(q.x) + ", " + num(q.y) + ", " + num(q.z) + ", " + num(q.w) +
                   "], \"scale\": [1, 1, 1], \"visible\": true, \"selectable\": true, \"material\": " + std::to_string(matIndex[mat]) + "}";
        ++count;
    }
    if (!count) { report = "no brick"; return false; }

    std::string s = "{\n  \"format\": \"wzx-scene\",\n  \"version\": 1,\n  \"units\": \"cm\",\n";
    s += "  \"scene\": {\"name\": " + quote(name) + ", \"unitsPerMeter\": 100, \"gridScale\": 1},\n";
    const Camera& cam = m_scene.camera;
    const double d = 57.29577951308232;
    s += "  \"camera\": {\"target\": " + vec(cam.target) + ", \"yaw\": " + num(cam.yaw * d) + ", \"pitch\": " + num(cam.pitch * d) +
         ", \"distance\": " + num(cam.distance) + ", \"fov\": " + num(cam.fovY * d) + "},\n";
    const SunLight& sun = m_scene.sun;
    s += "  \"sun\": {\"enabled\": " + std::string(sun.enabled ? "true" : "false") + ", \"direction\": " + vec(sun.direction) +
         ", \"color\": " + vec(sun.color) + ", \"intensity\": " + num(sun.intensity) + "},\n";
    s += "  \"lights\": [";
    for (size_t k = 0; k < m_scene.lights.size(); ++k) {
        const AreaLight& l = m_scene.lights[k];
        s += std::string(k ? ",\n" : "\n") + "    {\"name\": " + quote(l.name) + ", \"position\": " + vec(l.position) + ", \"target\": " + vec(l.target) +
             ", \"width\": " + num(l.width) + ", \"height\": " + num(l.height) + ", \"color\": " + vec(l.color) + ", \"intensity\": " + num(l.intensity) +
             ", \"visible\": " + (l.visibleToCamera ? "true" : "false") + ", \"enabled\": " + (l.enabled ? "true" : "false") + "}";
    }
    s += "\n  ],\n  \"render\": {\"bevel\": true, \"bevelScale\": " + num(m_rs.bevelScale) + ", \"sssScale\": " + num(m_rs.sssScale) + "},\n";
    s += "  \"materials\": [";
    for (size_t k = 0; k < mats.size(); ++k) {
        const Material& m = m_scene.materials[mats[k]];
        s += std::string(k ? ",\n" : "\n") + "    {\"name\": " + quote(m.name) + ", \"baseColor\": " + vec(m.baseColor) + ", \"roughness\": " + num(m.roughness) +
             ", \"metallic\": " + num(m.metallic) + ", \"specular\": " + num(m.specular) + ", \"transmission\": " + num(m.transmission) +
             ", \"ior\": " + num(m.ior) + ", \"subsurface\": " + num(m.subsurface) + ", \"subsurfaceRadius\": " + vec(m.sssRadius) +
             ", \"emission\": " + vec(m.emission) + ", \"emissionStrength\": " + num(m.emissionStrength) + ", \"coat\": " + num(m.coat) +
             ", \"coatRoughness\": " + num(m.coatRoughness) + ", \"bevelRadius\": " + num(m.bevelRadius) +
             ", \"flags\": " + std::to_string(m.flags & ~4u) + "}";   // (no vertex colours in an OBJ: the print flag off)
    }
    s += "\n  ],\n  \"objects\": [\n    {\"name\": \"Ground\", \"type\": \"GROUND\", \"shape\": {\"primitive\": \"plane\", \"size\": 400}, \"position\": [0, 0, 0], "
         "\"rotation\": [0, 0, 0, 1], \"scale\": [1, 1, 1], \"visible\": true, \"selectable\": false, \"material\": " + std::to_string(mats.size()) + "},\n";
    s += objects + "\n  ]\n}\n";
    // the ground: a light studio grey, rough (the last material)
    {
        const std::string g = ",\n    {\"name\": \"Ground\", \"baseColor\": [0.45, 0.46, 0.48], \"roughness\": 0.6, \"specular\": 0.3}\n  ],\n  \"objects\"";
        const size_t at = s.find("\n  ],\n  \"objects\"");
        s.replace(at, strlen("\n  ],\n  \"objects\""), g);
    }
    const fs::path scenePath = sceneDir / (safe + ".wzx");
    FILE* f = nullptr;
    if (_wfopen_s(&f, scenePath.wstring().c_str(), L"wb") || !f) { report = "cannot write " + scenePath.string(); return false; }
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
    report = "scene=" + scenePath.string() + "\nobjects=" + std::to_string(count) + "\nmeshes=" + std::to_string(meshFile.size()) +
             "\nmaterials=" + std::to_string(mats.size()) + "\n";
    return true;
}

} // namespace brickit
