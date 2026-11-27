#include "ArchAsset.h"
#include "Image.h"
#include "Platform.h"
#include "app/Json.h"
#include "vp3d/AssetImport.h"
#include "vp3d/shaders/Shared.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace archviz {

using namespace vp3d;

static bool readFile(const std::wstring& path, std::string& out) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) return false;
    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? (size_t)n : 0);
    const size_t got = out.empty() ? 0 : fread(&out[0], 1, out.size(), f);
    fclose(f);
    return got == out.size();
}

static float3 vec3(const app::Json& j, float3 def) {
    return j.a.size() >= 3 ? float3((float)j[0].num(), (float)j[1].num(), (float)j[2].num()) : def;
}

void scanAssets(const std::wstring& dir, std::vector<AssetInfo>& out) {
    out.clear();
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(std::filesystem::path(dir), ec)) {
        if (!e.is_directory(ec)) continue;
        AssetInfo a;
        a.dir = e.path().wstring();
        a.slug = brickit::plat::toUtf8(e.path().filename().wstring());
        std::string text;
        app::Json j;
        if (!readFile(a.dir + L"/asset.json", text) || !app::Json::parse(text, j) || j["format"].str() != "archviz-asset") continue;
        a.name = j["name"].str(a.slug);
        a.size = vec3(j["size"], float3(1, 1, 1));
        a.tris = (int)j["tris"].num();
        a.parts = (int)j["parts"].a.size();
        for (const app::Json& p : j["parts"].a) if (!p["material"]["texture"].str().empty()) ++a.textures;
        out.push_back(a);
    }
    std::sort(out.begin(), out.end(), [](const AssetInfo& x, const AssetInfo& y) { return x.name < y.name; });
}

bool loadAsset(AssetInfo& a, Scene& scene, std::string* error) {
    auto fail = [&](const std::string& e) { if (error) *error = a.slug + ": " + e; return false; };
    std::string text;
    app::Json j;
    if (!readFile(a.dir + L"/asset.json", text) || !app::Json::parse(text, j)) return fail("asset.json unreadable");
    FILE* f = nullptr;
    const std::wstring bin = a.dir + L"/" + brickit::plat::fromUtf8(j["bin"].str("mesh.bin"));
    if (_wfopen_s(&f, bin.c_str(), L"rb") != 0 || !f) return fail("mesh.bin missing");
    const int firstMesh = (int)scene.meshes.size(), firstMat = (int)scene.materials.size();
    std::vector<std::pair<std::string, int>> texOf;   // file -> scene texture (shared between the parts)
    int parts = 0;
    bool ok = true;
    for (const app::Json& p : j["parts"].a) {
        const size_t nv = (size_t)p["vertices"].num(), ni = (size_t)p["indices"].num();
        if (nv == 0 || ni < 3 || nv > 50000000 || ni > 150000000) { ok = false; break; }
        MeshData m;
        m.vertices.resize(nv);
        m.indices.resize(ni);
        static_assert(sizeof(Vertex) == 32, "the vertices of mesh.bin are read as they are");
        if (fread(m.vertices.data(), sizeof(Vertex), nv, f) != nv || fread(m.indices.data(), 4, ni, f) != ni) { ok = false; break; }
        for (uint32_t& i : m.indices) if (i >= nv) i = 0;
        const app::Json& jm = p["material"];
        Material mat;
        mat.name = a.slug + "/" + p["name"].str("part");
        const float3 c = vec3(jm["color"], float3(0.8f, 0.8f, 0.8f));
        mat.baseColor = float3(srgbToLinear(c.x), srgbToLinear(c.y), srgbToLinear(c.z));
        mat.roughness = std::clamp((float)jm["roughness"].num(0.5), 0.0f, 1.0f);
        mat.metallic = std::clamp((float)jm["metallic"].num(0), 0.0f, 1.0f);
        mat.transmission = std::clamp((float)jm["transmission"].num(0), 0.0f, 1.0f);
        mat.ior = (float)jm["ior"].num(1.5);
        if (mat.transmission > 0.5f) mat.flags |= MAT_FLAG_THIN;   // panes and bottles modelled as single surfaces
        mat.emission = vec3(jm["emission"], float3(0, 0, 0));
        mat.emissionStrength = (float)jm["emissionStrength"].num(0);
        const std::string tex = jm["texture"].str();
        if (!tex.empty()) {
            int ti = -1;
            for (const auto& t : texOf) if (t.first == tex) ti = t.second;
            if (ti < 0 && scene.textures.size() < MAX_TEXTURES) {
                Texture T;
                std::string err;
                bool got = loadImage(a.dir + L"/" + brickit::plat::fromUtf8(tex), T, &err, 2048);
                if (!got) {   // (where the engine has no decoder: the Linux test build)
                    std::string bytes;
                    int w = 0, h = 0;
                    if (readFile(a.dir + L"/" + brickit::plat::fromUtf8(tex), bytes) && brickit::image::decode((const uint8_t*)bytes.data(), bytes.size(), T.rgba, w, h)) {
                        T.width = (uint32_t)w; T.height = (uint32_t)h;
                        got = w > 0 && h > 0;
                    }
                }
                if (got) {
                    T.name = a.slug + "/" + tex;
                    ti = scene.addTexture(std::move(T));
                    texOf.push_back({tex, ti});
                }
            }
            mat.baseColorTex = ti;
        }
        scene.addMesh(a.slug + "#" + std::to_string(parts), std::move(m));
        scene.addMaterial(mat);
        ++parts;
    }
    fclose(f);
    if (!ok || parts == 0) {   // nothing half loaded
        scene.meshes.resize(firstMesh);
        scene.meshNames.resize(firstMesh);
        scene.materials.resize(firstMat);
        return fail("mesh.bin does not match asset.json");
    }
    a.firstMesh = firstMesh; a.firstMaterial = firstMat; a.partCount = parts;
    return true;
}

} // namespace archviz
