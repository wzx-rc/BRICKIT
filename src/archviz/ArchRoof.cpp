#include "ArchRoof.h"
#include <algorithm>
#include <cmath>

namespace archviz {

using namespace vp3d;

const char* roofTypeName(int t) {
    static const char* n[] = {"None", "Flat", "Gable", "Hip", "Shed"};
    return n[std::clamp(t, 0, kRoofTypeCount - 1)];
}

const char* roofFinishName(int f) {
    static const char* n[] = {"Grey tiles", "Red tiles", "Zinc", "Slate"};
    return n[std::clamp(f, 0, kRoofFinishCount - 1)];
}

int roofTopLevel(const Building& B) {
    int top = -1;
    for (const Wall& w : B.walls)
        if (w.alive && w.a >= 0 && w.a < (int)B.nodes.size()) top = std::max(top, B.nodes[w.a].level);
    return top;
}

namespace {

inline float3 W3(float2 p, float z) { return float3(p.x, z, -p.y); }   // plan + height -> world (as ArchMesh world())

void tri(MeshData& m, float3 a, float3 b, float3 c, float3 hint) {
    float3 n = cross(b - a, c - a);
    const float l = length(n);
    if (l < 1e-9f) return;
    n = n / l;
    if (dot(n, hint) < 0) { std::swap(b, c); n = -n; }
    const uint32_t i = (uint32_t)m.vertices.size();
    m.vertices.push_back({a, n, float2(a.x, a.z)});
    m.vertices.push_back({b, n, float2(b.x, b.z)});
    m.vertices.push_back({c, n, float2(c.x, c.z)});
    m.addTri(i, i + 1, i + 2);
}

void quad(MeshData& m, float3 a, float3 b, float3 c, float3 d, float3 hint) {
    tri(m, a, b, c, hint);
    tri(m, a, c, d, hint);
}

// A pan of a given thickness: its top polygon (3 or 4 points), the same lowered by t, and its edges.
void pan(MeshData& m, const std::vector<float3>& top, float t, float2 centre, float cz) {
    std::vector<float3> bot;
    for (const float3& p : top) bot.push_back(p - float3(0, t, 0));
    const float3 up(0, 1, 0);
    for (size_t i = 1; i + 1 < top.size(); ++i) {
        tri(m, top[0], top[i], top[i + 1], up);
        tri(m, bot[0], bot[i], bot[i + 1], -up);
    }
    const float3 c3 = W3(centre, cz);
    for (size_t i = 0; i < top.size(); ++i) {
        const size_t j = (i + 1) % top.size();
        float3 out = (top[i] + top[j]) * 0.5f - c3;
        out.y = 0;
        quad(m, top[i], top[j], bot[j], bot[i], out);
    }
}

// a vertical prism over a plan polygon (counter-clockwise or not), from z0 to z1
void prism(MeshData& m, const std::vector<float2>& poly, float z0, float z1) {
    if (poly.size() < 3 || z1 <= z0) return;
    std::vector<int> tris;
    std::vector<float2> p = poly;
    if (polyArea(p) < 0) std::reverse(p.begin(), p.end());
    triangulate(p, tris);
    for (size_t i = 0; i + 2 < tris.size(); i += 3) {
        tri(m, W3(p[tris[i]], z1), W3(p[tris[i + 1]], z1), W3(p[tris[i + 2]], z1), float3(0, 1, 0));
        tri(m, W3(p[tris[i]], z0), W3(p[tris[i + 1]], z0), W3(p[tris[i + 2]], z0), float3(0, -1, 0));
    }
    float2 c(0, 0);
    for (const float2& q : p) c = c + q;
    c = c * (1.0f / p.size());
    for (size_t i = 0; i < p.size(); ++i) {
        const float2 a = p[i], b = p[(i + 1) % p.size()];
        const float2 mid = (a + b) * 0.5f;
        quad(m, W3(a, z0), W3(b, z0), W3(b, z1), W3(a, z1), W3(mid - c, 0));
    }
}

} // namespace

bool buildRoof(const Building& B, int level, RoofGeom& out) {
    out = RoofGeom();
    if (level < 0 || level >= (int)B.levels.size()) return false;
    const Level& L = B.levels[level];
    if (L.roof == kRoofNone) return false;
    // the walls of the level: their outline corners, the longest one, the outer ones
    std::vector<int> ws;
    float topH = 0, longest = -1, outerT = 0.3f;
    float2 u0(1, 0);
    for (int w = 0; w < (int)B.walls.size(); ++w) {
        const Wall& W = B.walls[w];
        if (!W.alive || w >= (int)B.geom.size() || B.nodes[W.a].level != level) continue;
        ws.push_back(w);
        topH = std::max(topH, W.height);
        if (B.geom[w].len > longest) { longest = B.geom[w].len; u0 = B.geom[w].u; }
    }
    if (ws.empty()) return false;
    const float z0 = L.elevation + topH;
    const float t = std::clamp(L.roofThick, 0.05f, 0.8f);
    auto outer = [&](int w, float& side) {   // a wall with a room on one side only: side = its outside (+1 / -1 along n)
        const WallGeom& g = B.geom[w];
        const float2 c = (g.a + g.b) * 0.5f;
        const float d = B.walls[w].thickness * 0.5f + 0.15f;
        const bool plus = B.roomAt(c + g.n * d, level) >= 0, minus = B.roomAt(c - g.n * d, level) >= 0;
        side = plus ? -1.0f : 1.0f;
        return plus != minus;
    };

    if (L.roof == kRoofFlat) {   // a slab over each room, a parapet on the outer walls
        for (const Room& R : B.rooms)
            if (R.level == level) prism(out.roof, R.poly, z0, z0 + t);
        for (int w : ws) {
            float side;
            if (!outer(w, side)) continue;
            const WallGeom& g = B.geom[w];
            prism(out.walls, {g.aL, g.bL, g.bR, g.aR}, z0, z0 + t + 0.45f);
        }
        return true;
    }

    // the rectangle around the walls, turned along the longest one; the ridge along its long side (or across)
    float2 v0 = perp(u0);
    float smin = 1e9f, smax = -1e9f, rmin = 1e9f, rmax = -1e9f;
    for (int w : ws) {
        const WallGeom& g = B.geom[w];
        const float2 cs[4] = {g.aL, g.aR, g.bL, g.bR};
        for (const float2& p : cs) {
            const float s = dot2(p, u0), r = dot2(p, v0);
            smin = std::min(smin, s); smax = std::max(smax, s); rmin = std::min(rmin, r); rmax = std::max(rmax, r);
        }
    }
    bool alongU = (smax - smin) >= (rmax - rmin);
    if (L.roofRidge && L.roof != kRoofHip) alongU = !alongU;
    const float2 U = alongU ? u0 : v0, V = alongU ? v0 : -u0;
    const float2 c = u0 * ((smin + smax) * 0.5f) + v0 * ((rmin + rmax) * 0.5f);
    const float A = (alongU ? (smax - smin) : (rmax - rmin)) * 0.5f, Bh = (alongU ? (rmax - rmin) : (smax - smin)) * 0.5f;
    // rectangular? the outer face of every outer wall lies on the rectangle
    for (int w : ws) {
        float side;
        if (!outer(w, side)) continue;
        const WallGeom& g = B.geom[w];
        outerT = std::max(0.1f, B.walls[w].thickness);
        const float2 face[2] = {side > 0 ? g.aL : g.aR, side > 0 ? g.bL : g.bR};
        for (const float2& p : face) {
            const float s = dot2(p - c, U), r = dot2(p - c, V);
            const float d = std::min(std::min(std::fabs(std::fabs(s) - A), std::fabs(std::fabs(r) - Bh)), 1e9f);
            if (d > 0.06f) out.rectangular = false;
        }
    }
    const float o = std::clamp(L.roofOverhang, 0.0f, 2.0f);
    const float tn = std::tan(std::clamp(L.roofSlope, 3.0f, 70.0f) * 3.14159265f / 180.0f);
    const float A2 = A + o, B2 = Bh + o, zE = z0 - o * tn;
    auto P = [&](float s, float r, float z) { return W3(c + U * s + V * r, z); };
    auto Q = [&](float s, float r) { return c + U * s + V * r; };
    out.eaves = {Q(-A2, -B2), Q(A2, -B2), Q(A2, B2), Q(-A2, B2)};

    if (L.roof == kRoofGable) {
        const float zR = z0 + Bh * tn, zRe = zE + B2 * tn;
        (void)zR;
        pan(out.roof, {P(-A2, -B2, zE), P(A2, -B2, zE), P(A2, 0, zRe), P(-A2, 0, zRe)}, t, c, zE);
        pan(out.roof, {P(-A2, B2, zE), P(-A2, 0, zRe), P(A2, 0, zRe), P(A2, B2, zE)}, t, c, zE);
        out.ridges.push_back({Q(-A2, 0), Q(A2, 0)});
        // the gable walls: triangles from the top of the walls up under the roof, in the outer walls' thickness
        const float apex = z0 + Bh * tn - t;
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            const float s1 = sgn * A, s0 = sgn * (A - outerT);
            std::vector<float3> a = {P(s0, -Bh, z0), P(s0, Bh, z0), P(s0, 0, apex)}, b = {P(s1, -Bh, z0), P(s1, Bh, z0), P(s1, 0, apex)};
            const float3 outD = W3(U * (float)sgn, 0);
            tri(out.walls, b[0], b[1], b[2], outD);
            tri(out.walls, a[0], a[1], a[2], -outD);
            quad(out.walls, a[0], a[2], b[2], b[0], W3(-V, 0) + float3(0, 1, 0));
            quad(out.walls, a[2], a[1], b[1], b[2], W3(V, 0) + float3(0, 1, 0));
        }
    } else if (L.roof == kRoofHip) {
        const float h = std::max(0.0f, A2 - B2), zR = zE + B2 * tn;
        pan(out.roof, {P(-A2, -B2, zE), P(A2, -B2, zE), P(h, 0, zR), P(-h, 0, zR)}, t, c, zE);
        pan(out.roof, {P(A2, B2, zE), P(-A2, B2, zE), P(-h, 0, zR), P(h, 0, zR)}, t, c, zE);
        pan(out.roof, {P(A2, -B2, zE), P(A2, B2, zE), P(h, 0, zR)}, t, c, zE);
        pan(out.roof, {P(-A2, B2, zE), P(-A2, -B2, zE), P(-h, 0, zR)}, t, c, zE);
        if (h > 0.01f) out.ridges.push_back({Q(-h, 0), Q(h, 0)});
        out.ridges.push_back({Q(h, 0), Q(A2, -B2)});
        out.ridges.push_back({Q(h, 0), Q(A2, B2)});
        out.ridges.push_back({Q(-h, 0), Q(-A2, -B2)});
        out.ridges.push_back({Q(-h, 0), Q(-A2, B2)});
    } else {   // shed: low on the -V side
        const float zH = zE + 2.0f * B2 * tn;
        pan(out.roof, {P(-A2, -B2, zE), P(A2, -B2, zE), P(A2, B2, zH), P(-A2, B2, zH)}, t, c, zE);
        // the high wall and the two side gables (up under the roof)
        auto under = [&](float r) { return z0 + (r + Bh) * tn - t; };
        prism(out.walls, {Q(-A, Bh - outerT), Q(A, Bh - outerT), Q(A, Bh), Q(-A, Bh)}, z0, under(Bh - outerT));
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            const float s1 = sgn * A, s0 = sgn * (A - outerT);
            const float3 a0 = P(s0, -Bh, z0), a1 = P(s0, Bh - outerT, z0), a2 = P(s0, Bh - outerT, under(Bh - outerT));
            const float3 b0 = P(s1, -Bh, z0), b1 = P(s1, Bh - outerT, z0), b2 = P(s1, Bh - outerT, under(Bh - outerT));
            const float3 outD = W3(U * (float)sgn, 0);
            tri(out.walls, b0, b1, b2, outD);
            tri(out.walls, a0, a1, a2, -outD);
            quad(out.walls, a0, a2, b2, b0, W3(-V, 0) + float3(0, 1, 0));
        }
    }
    return true;
}

} // namespace archviz
