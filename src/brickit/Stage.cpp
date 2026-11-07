#include "Stage.h"
#include "vp3d/MeshGen.h"
#include "vp3d/shaders/Shared.h"
#include <algorithm>

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
        m.bevelRadius = 0.035f;   // no fingerprint mask (Wzx: none anywhere)
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

} // namespace brickit
