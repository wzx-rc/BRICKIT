// BrickIt - the Material panel (Wzx 24/11/2026: « dans le panneau couleur, en dessous des swatchs, un second panneau
// matériau: couleur, metalness, transparence, roughness, et un reset to original »): the material of a palette colour
// edited; every brick of that colour follows (the scene now and the bricks laid later), and the edits are kept in the
// preferences. The factory material (Stage.cpp) stays aside for the reset.
#include "BrickApp.h"
#include <algorithm>
#include <cmath>

namespace brickit {

static float linToSrgb(float c) {
    c = std::clamp(c, 0.0f, 1.0f);
    return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

BrickApp::MatEdit BrickApp::matEditOf(int ci) const {
    MatEdit e;
    if (ci < 0 || ci >= (int)m_lib.palette.size()) return e;
    auto it = m_matEdits.find(m_lib.palette[ci].id);
    if (it != m_matEdits.end()) return it->second;
    if (ci < (int)m_colorMaterialFactory.size()) {
        const Material& m = m_colorMaterialFactory[ci];
        e.srgb = float3(linToSrgb(m.baseColor.x), linToSrgb(m.baseColor.y), linToSrgb(m.baseColor.z));
        e.metal = m.metallic;
        e.trans = m.transmission;
        e.rough = m.roughness;
        e.sss = m.subsurface;
    }
    return e;
}

void BrickApp::applyMatEdits() {
    for (size_t i = 0; i < m_colorMaterialFactory.size() && i < m_colorMaterialOrig.size() && i < m_lib.palette.size(); ++i) {
        Material m = m_colorMaterialFactory[i];
        auto it = m_matEdits.find(m_lib.palette[i].id);
        if (it != m_matEdits.end()) {
            const MatEdit& e = it->second;
            m.baseColor = float3(srgbToLinear(e.srgb.x), srgbToLinear(e.srgb.y), srgbToLinear(e.srgb.z));
            m.metallic = e.metal;
            m.transmission = e.trans;
            m.roughness = e.rough;
            if (e.sss >= 0) {   // SSS intensity and scale (Wzx 24/11/2026): the light under the surface, its distance (the Stage.cpp radius x scale)
                m.subsurface = e.sss;
                m.sssRadius = (m.baseColor * 0.85f + float3(0.15f)) * (0.14f * std::max(0.05f, e.sssScale));
            }
            if (e.metal > 0.5f || e.trans > 0.5f) { m.coat = 0.0f; m.subsurface = 0.0f; }   // (metal and glass: no varnish, no light under the surface)
            else if (m_colorMaterialFactory[i].metallic > 0.5f || m_colorMaterialFactory[i].transmission > 0.5f) m.coat = 0.35f;   // (made plastic: the ABS varnish)
        }
        m_colorMaterialOrig[i] = m;
    }
    setLookdev(m_giLookdev);   // (the materials of the scene from these: the lookdev grey or the varnish of the path tracer over them)
}

void BrickApp::setMatEdit(int ci, const MatEdit& e) {
    if (ci < 0 || ci >= (int)m_lib.palette.size()) return;
    m_matEdits[m_lib.palette[ci].id] = e;
    applyMatEdits();
}

void BrickApp::resetMatEdit(int ci) {
    if (ci < 0 || ci >= (int)m_lib.palette.size()) return;
    m_matEdits.erase(m_lib.palette[ci].id);
    applyMatEdits();
    savePrefs();
}

} // namespace brickit
