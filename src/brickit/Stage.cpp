#include "Stage.h"
#include "vp3d/MeshGen.h"
#include "vp3d/shaders/Shared.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace brickit {

using namespace vp3d;

int addGround(Scene& scene, float size, bool grid, bool visible) {
    Material g;
    g.name = "Ground";
    g.baseColor = float3(0.70f, 0.71f, 0.73f);   // light studio floor
    g.roughness = 0.5f;
    g.flags = grid ? MAT_FLAG_GRID : 0u;
    Object o;
    o.name = "Ground";
    o.part = "GROUND";
    o.mesh = scene.addMesh("ground", makePlane(size));
    o.material = scene.addMaterial(g);
    o.selectable = false;
    o.visible = visible;
    return scene.addObject(o);
}

// The cyclorama of the studio stage (BrickIt 24/11/2026, after the lookdev scene of Wzx in Maya: its Background, 49.6 cm
// wide, 27.4 deep, 23.5 high, the model near the middle of the floor): the floor at y = 0 from z = 13.5 to the corner,
// a quarter of a circle, the wall at z = -13.9. One unit mesh (scaled by the stage for bigger models), smooth normals.
int addCyclo(Scene& scene) {
    const float W = 24.8f, zFront = 13.5f, zWall = -13.9f, H = 23.5f, Rc = 6.0f;
    struct P { float z, y, nz, ny; };
    std::vector<P> prof;
    prof.push_back({zFront, 0, 0, 1});
    const int N = 24;
    for (int i = 0; i <= N; ++i) {
        const float a = (float)i / N * 1.5707963f;
        prof.push_back({zWall + Rc - Rc * std::sin(a), Rc - Rc * std::cos(a), std::sin(a), std::cos(a)});
    }
    prof.push_back({zWall, H, 1, 0});
    MeshData m;
    for (const P& p : prof)
        for (int s = 0; s < 2; ++s) {
            Vertex v;
            v.pos = float3(s ? W : -W, p.y, p.z);
            v.nrm = float3(0, p.ny, p.nz);
            v.uv = float2(0, 0);
            m.vertices.push_back(v);
        }
    for (uint32_t i = 0; i + 1 < (uint32_t)prof.size(); ++i) {
        const uint32_t a = i * 2, b = a + 1, c = a + 2, d = a + 3;   // a (-W, i), b (+W, i), c (-W, i + 1), d (+W, i + 1)
        m.addTri(a, b, d);
        m.addTri(a, d, c);
    }
    m.quadEdges();
    Material g;
    g.name = "Cyclo";
    g.baseColor = float3(0.365f);   // (its Arnold shader: base 0.73 x a grey of 0.5)
    g.roughness = 0.2f;
    g.specular = 0.5f;
    g.flags = 16u;   // MAT_FLAG_FRESNEL
    Object o;
    o.name = "Cyclo";
    o.part = "GROUND";
    o.mesh = scene.addMesh("cyclo", m);
    o.material = scene.addMaterial(g);
    o.selectable = false;
    o.visible = false;
    return scene.addObject(o);
}

int addRimLight(Scene& scene) {
    // cool rim of the web: DirectionalLight(0xc0d4ff, 1.3) from (26, 22, -30), no shadow; a far area light whose
    // irradiance at the target is 1.3 (raster only, the path traced studio has its own lights)
    AreaLight rim;
    rim.name = "Rim";
    const float3 tgt(0.4f, 1.2f, 0.4f), dir = normalize(float3(26, 22, -30));
    const float D = 80.0f, W = 8.0f;
    rim.position = tgt + dir * D;
    rim.target = tgt;
    rim.width = rim.height = W;
    rim.color = float3(srgbToLinear(0xc0 / 255.0f), srgbToLinear(0xd4 / 255.0f), 1.0f);
    rim.intensity = 1.3f * D * D / (W * W);
    rim.visibleToCamera = false;
    scene.lights.push_back(rim);
    return (int)scene.lights.size() - 1;
}

// ABS plastic (light clear coat, discreet subsurface on the light colours), transparent = tinted glass, metal colours =
// metal; bevel on every edge (rounded edges and the groove between two bricks that touch)
std::vector<int> addPaletteMaterials(Scene& scene, const Library& lib, bool sss) {
    std::vector<int> out;
    for (const Color& c : lib.palette) {
        Material m;
        m.name = c.name;
        float3 srgb = c.srgb;
        if (c.id == 4) srgb = float3(201, 26, 9) * (1.0f / 255.0f);   // LDConfig Red is pure 180,0,0: LEGO Bright Red instead
        m.baseColor = float3(srgbToLinear(srgb.x), srgbToLinear(srgb.y), srgbToLinear(srgb.z));
        m.roughness = 0.24f;
        m.specular = 0.5f;
        m.coat = 0.35f;
        m.coatRoughness = 0.05f;
        m.bevelRadius = 0.0245f;   // no fingerprint mask (Wzx: none anywhere); 30 % tighter (Wzx 14/11/2026, 0.035 before)
        m.flags |= 4u;   // MAT_FLAG_VCOLOR: the prints of the printed parts (their colour per vertex)
        const float lum = 0.2126f * m.baseColor.x + 0.7152f * m.baseColor.y + 0.0722f * m.baseColor.z;
        if (lum > 0.06f && sss) {   // light goes into the ABS: more on the light colours (white, yellow, red),
            m.subsurface = std::min(0.35f, 0.15f + lum * 0.4f);   // a diffusion distance of about a millimetre, tinted
            m.sssRadius = (m.baseColor * 0.85f + float3(0.15f)) * 0.14f;
        }
        if (c.group.find("trans") != std::string::npos) {
            m.transmission = 1.0f;
            m.roughness = 0.03f;
            m.coat = 0.0f;
            m.subsurface = 0.0f;
        } else if (c.group.find("metal") != std::string::npos || c.group.find("chrome") != std::string::npos) {
            m.metallic = 1.0f;
            m.roughness = 0.2f;
            m.coat = 0.0f;
            m.subsurface = 0.0f;
        }
        out.push_back(scene.addMaterial(m));
    }
    return out;
}

// Tyres (Wzx 24/11/2026: « un shader plus mat et SSS pour les pneus »): rubber has no varnish and a wide dull highlight;
// light goes a little into it, which softens the tread and the sidewall. The black of LDraw (#1B2A34) is bluish: the
// rubber black is a neutral dark grey; the other colours a little less saturated (the rubber is never as vivid as ABS).
std::vector<int> addRubberMaterials(Scene& scene, const Library& lib, bool sss) {
    std::vector<int> out;
    for (const Color& c : lib.palette) {
        Material m;
        m.name = c.name + " (rubber)";
        float3 lin(srgbToLinear(c.srgb.x), srgbToLinear(c.srgb.y), srgbToLinear(c.srgb.z));
        const float lum = 0.2126f * lin.x + 0.7152f * lin.y + 0.0722f * lin.z;
        m.baseColor = lum < 0.04f ? float3(0.021f, 0.021f, 0.022f) : lin * 0.75f + float3(lum * 0.25f);
        m.roughness = 0.58f;   // (Wzx 24/11/2026: « un poil moins mat, une légère spéculaire visible »; 0.8 / 0.3 before)
        m.specular = 0.45f;
        m.coat = 0.0f;
        m.bevelRadius = 0.0245f;
        m.flags |= 4u;   // (MAT_FLAG_VCOLOR: printed tyres)
        if (sss) {
            m.subsurface = 0.45f;
            m.sssRadius = (m.baseColor * 0.5f + float3(0.5f)) * 0.12f;
        }
        out.push_back(scene.addMaterial(m));
    }
    return out;
}

bool isRubberPart(const Part& p) {
    auto starts = [&](const char* w) { return p.label.rfind(w, 0) == 0; };
    return p.category == "Tyre" || starts("Tyre") || starts("Tire") || p.label.find("Rubber") != std::string::npos;
}

} // namespace brickit
