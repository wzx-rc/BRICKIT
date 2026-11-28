#include "ArchLight.h"
#include "ArchMesh.h"
#include <algorithm>
#include <cmath>

namespace archviz {

using namespace vp3d;

namespace {

constexpr float kLuxPerUnit = 31250.0f;   // the sun of the sky: 3.2 for about 100 000 lux

// a box of half sizes h along the axes ax, ay, az around c
void obox(MeshData& m, float3 c, float3 ax, float3 ay, float3 az, float3 h) {
    auto P = [&](float x, float y, float z) { return c + ax * (x * h.x) + ay * (y * h.y) + az * (z * h.z); };
    auto q = [&](float3 a, float3 b, float3 cc, float3 d, float3 n) {
        if (dot(cross(b - a, cc - a), n) < 0) std::swap(b, d);
        m.addQuad(a, b, cc, d, n);
    };
    q(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), az);
    q(P(-1, -1, -1), P(1, -1, -1), P(1, 1, -1), P(-1, 1, -1), -az);
    q(P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), P(1, -1, 1), ax);
    q(P(-1, -1, -1), P(-1, 1, -1), P(-1, 1, 1), P(-1, -1, 1), -ax);
    q(P(-1, 1, -1), P(1, 1, -1), P(1, 1, 1), P(-1, 1, 1), ay);
    q(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), -ay);
}

void append(MeshData& to, const MeshData& from, float3 offset) {
    const uint32_t base = (uint32_t)to.vertices.size();
    for (Vertex v : from.vertices) { v.pos = v.pos + offset; to.vertices.push_back(v); }
    for (uint32_t i : from.indices) to.indices.push_back(base + i);
}

float ceilingUnder(const Building& B, int level) {
    const Level& L = B.levels[std::clamp(level, 0, (int)B.levels.size() - 1)];
    return L.height - 0.04f;   // (the ceiling slab: ArchMesh)
}

} // namespace

const char* fixtureName(int kind) {
    static const char* n[] = {"Ceiling light", "Pendant", "Wall light", "Spot", "LED strip"};
    return n[std::clamp(kind, 0, kFixKindCount - 1)];
}

float fixtureDefaultLumens(int kind) {
    static const float l[] = {1200, 800, 500, 450, 600};
    return l[std::clamp(kind, 0, kFixKindCount - 1)];
}

float fixtureHeight(const Building& B, const Fixture& f) {
    if (f.height > 0.01f) return f.height;
    const float top = ceilingUnder(B, f.level);
    switch (f.kind) {
    case kFixPendant: return std::max(1.6f, top - 0.9f);
    case kFixWall: return std::min(1.8f, top - 0.3f);
    case kFixStrip: return top - 0.02f;
    default: return top;
    }
}

void buildFixture(const Building& B, const Fixture& f, MeshData& body, AreaLight& light) {
    const Level& L = B.levels[std::clamp(f.level, 0, (int)B.levels.size() - 1)];
    const float e = L.elevation, top = e + ceilingUnder(B, f.level), h = e + fixtureHeight(B, f);
    const float3 Y(0, 1, 0);
    const float3 D = normalize(world(float2(std::cos(f.yaw), std::sin(f.yaw)), 0) - world(float2(0, 0), 0));   // facing / running
    const float3 S = cross(Y, D);   // across
    float w = 0.3f, hh = 0.3f;
    float3 pos, nrm = -Y, up = D;
    switch (f.kind) {
    case kFixCeiling:   // a flat square panel under the ceiling
        obox(body, world(f.p, top - 0.03f), D, Y, S, float3(0.17f, 0.03f, 0.17f));
        pos = world(f.p, top - 0.061f); w = hh = 0.30f;
        break;
    case kFixSpot:      // recessed: a thin ring, a small light
        obox(body, world(f.p, top - 0.005f), D, Y, S, float3(0.055f, 0.005f, 0.055f));
        pos = world(f.p, top - 0.0105f); w = hh = 0.08f;
        break;
    case kFixPendant: {   // a conical shade on its cord; the light at the opening, downwards
        const float shadeH = 0.22f;
        std::vector<LathePoint> prof = {{0.0f, shadeH}, {0.07f, shadeH}, {0.20f, 0.0f}, {0.19f, 0.0f}, {0.065f, shadeH - 0.012f}, {0.0f, shadeH - 0.012f}};
        append(body, makeLathe(prof, 40, 20.0f), world(f.p, h));
        const float cl = std::max(0.01f, top - (h + shadeH));
        obox(body, world(f.p, h + shadeH + cl * 0.5f), D, Y, S, float3(0.004f, cl * 0.5f, 0.004f));
        obox(body, world(f.p, top - 0.01f), D, Y, S, float3(0.05f, 0.01f, 0.05f));   // the canopy
        pos = world(f.p, h + 0.004f); w = hh = 0.26f;
        break;
    }
    case kFixWall:      // a box against the wall, its diffuser facing the room
        obox(body, world(f.p, h) + D * 0.035f, S, Y, D, float3(0.13f, 0.08f, 0.035f));
        pos = world(f.p, h) + D * 0.0712f; nrm = D; up = Y; w = 0.22f; hh = 0.12f;
        break;
    case kFixStrip: {   // an aluminium profile, the strip under it
        const float len = std::max(0.1f, f.length);
        obox(body, world(f.p, h + 0.008f), D, Y, S, float3(len * 0.5f, 0.008f, 0.012f));
        pos = world(f.p, h - 0.0005f); up = S; w = len; hh = 0.02f;
        break;
    }
    }
    light = AreaLight();
    light.name = fixtureName(f.kind);
    light.position = pos;
    light.target = pos + nrm;
    light.up = up;
    light.width = w; light.height = hh;
    light.color = float3(0.4f, 0.4f, 0.4f) + kelvinColor(f.kelvin) * 0.6f;   // (a camera white balance halfway: warm, not orange)
    light.intensity = lumensToIntensity(f.lumens, w * hh);
    light.visibleToCamera = true;
    light.enabled = f.on && f.lumens > 0;
    light.selectable = false;
}

float3 kelvinColor(float kelvin) {
    // Tanner Helland's fit of the black body (sRGB), then linear, luminance 1
    const float t = std::clamp(kelvin, 1000.0f, 12000.0f) / 100.0f;
    float r, g, b;
    if (t <= 66) { r = 255; g = 99.4708025861f * std::log(t) - 161.1195681661f; }
    else { r = 329.698727446f * std::pow(t - 60, -0.1332047592f); g = 288.1221695283f * std::pow(t - 60, -0.0755148492f); }
    if (t >= 66) b = 255;
    else if (t <= 19) b = 0;
    else b = 138.5177312231f * std::log(t - 10) - 305.0447927307f;
    auto lin = [](float v) { return srgbToLinear(std::clamp(v, 0.0f, 255.0f) / 255.0f); };
    float3 c(lin(r), lin(g), lin(b));
    const float y = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    return c * (1.0f / std::max(1e-4f, y));
}

float lumensToIntensity(float lumens, float area) {
    return std::max(0.0f, lumens) / kLuxPerUnit / (3.14159265f * std::max(1e-5f, area));
}

float3 sunDirection(float latitudeDeg, int month, int day, float solarHour, float* elevation) {
    static const int start[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    const int n = start[std::clamp(month, 1, 12) - 1] + std::clamp(day, 1, 31);
    const float rad = 3.14159265f / 180.0f;
    const float decl = -23.44f * rad * std::cos(2.0f * 3.14159265f / 365.0f * (n + 10));
    const float lat = std::clamp(latitudeDeg, -89.9f, 89.9f) * rad, H = (solarHour - 12.0f) * 15.0f * rad;
    const float sinEl = std::sin(lat) * std::sin(decl) + std::cos(lat) * std::cos(decl) * std::cos(H);
    const float el = std::asin(std::clamp(sinEl, -1.0f, 1.0f));
    // azimuth from the north, clockwise (east = 90 degrees)
    const float az = std::atan2(std::sin(H), std::cos(H) * std::sin(lat) - std::tan(decl) * std::cos(lat)) + 3.14159265f;
    if (elevation) *elevation = el;
    return normalize(float3(std::sin(az) * std::cos(el), std::sin(el), -std::cos(az) * std::cos(el)));
}

} // namespace archviz
