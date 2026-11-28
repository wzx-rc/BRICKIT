#include "ArchStair.h"
#include <algorithm>
#include <cmath>

namespace archviz {

using namespace vp3d;

const char* stairTypeName(int t) {
    static const char* n[] = {"Straight", "L", "U", "Spiral"};
    return n[std::clamp(t, 0, kStairTypeCount - 1)];
}

const char* stairFinishName(int f) {
    static const char* n[] = {"Oak", "Concrete", "White"};
    return n[std::clamp(f, 0, kStairFinishCount - 1)];
}

StairInfo stairInfo(const Building& B, const Stair& s) {
    StairInfo I;
    const int lv = std::clamp(s.level, 0, (int)B.levels.size() - 1);
    float rise = B.levels[lv].height + 0.30f;   // (no level above: the height of the level and a slab)
    if (lv + 1 < (int)B.levels.size()) rise = B.levels[lv + 1].elevation - B.levels[lv].elevation;
    if (s.rise > 0.2f) rise = s.rise;
    I.rise = std::max(0.3f, rise);
    I.risers = s.risers > 1 ? std::min(s.risers, 40) : std::clamp((int)std::lround(I.rise / 0.175f), 2, 40);
    I.riserH = I.rise / I.risers;
    I.tread = std::clamp(0.63f - 2.0f * I.riserH, 0.20f, 0.34f);
    I.blondel = 2.0f * I.riserH + I.tread;
    return I;
}

namespace {

const float kPi = 3.14159265f;

void tri(MeshData& m, float3 a, float3 b, float3 c, float3 hint) {
    float3 n = cross(b - a, c - a);
    const float l = length(n);
    if (l < 1e-10f) return;
    n = n / l;
    if (dot(n, hint) < 0) { std::swap(b, c); n = -n; }
    const uint32_t i = (uint32_t)m.vertices.size();
    m.vertices.push_back({a, n, float2(a.x + a.y, a.z)});
    m.vertices.push_back({b, n, float2(b.x + b.y, b.z)});
    m.vertices.push_back({c, n, float2(c.x + c.y, c.z)});
    m.addTri(i, i + 1, i + 2);
}

// a vertical prism over a convex plan polygon
void prism(MeshData& m, const std::vector<float3>& poly, float z0, float z1) {
    if (poly.size() < 3 || z1 <= z0 + 1e-4f) return;
    float3 c(0, 0, 0);
    for (const float3& p : poly) c = c + p;
    c = c * (1.0f / poly.size());
    for (size_t i = 1; i + 1 < poly.size(); ++i) {
        tri(m, float3(poly[0].x, z1, poly[0].z), float3(poly[i].x, z1, poly[i].z), float3(poly[i + 1].x, z1, poly[i + 1].z), float3(0, 1, 0));
        tri(m, float3(poly[0].x, z0, poly[0].z), float3(poly[i].x, z0, poly[i].z), float3(poly[i + 1].x, z0, poly[i + 1].z), float3(0, -1, 0));
    }
    for (size_t i = 0; i < poly.size(); ++i) {
        const float3 a = poly[i], b = poly[(i + 1) % poly.size()];
        float3 out = (a + b) * 0.5f - c;
        out.y = 0;
        const float3 a0(a.x, z0, a.z), b0(b.x, z0, b.z), a1(a.x, z1, a.z), b1(b.x, z1, b.z);
        tri(m, a0, b0, b1, out);
        tri(m, a0, b1, a1, out);
    }
}

// a beam from a to b, w wide (horizontal) and d deep
void beam(MeshData& m, float3 a, float3 b, float w, float d) {
    float3 dir = b - a;
    const float l = length(dir);
    if (l < 1e-5f) return;
    dir = dir / l;
    float3 side = cross(dir, float3(0, 1, 0));
    if (length(side) < 1e-4f) side = float3(1, 0, 0);
    side = normalize(side);
    const float3 up = normalize(cross(side, dir));
    const float3 sx = side * (w * 0.5f), uy = up * (d * 0.5f);
    const float3 p[8] = {a - sx - uy, a + sx - uy, a + sx + uy, a - sx + uy, b - sx - uy, b + sx - uy, b + sx + uy, b - sx + uy};
    auto q = [&](int i, int j, int k, int l2, float3 h) { tri(m, p[i], p[j], p[k], h); tri(m, p[i], p[k], p[l2], h); };
    q(0, 1, 2, 3, -dir); q(4, 5, 6, 7, dir); q(0, 1, 5, 4, -up); q(3, 2, 6, 7, up); q(0, 3, 7, 4, -side); q(1, 2, 6, 5, side);
}

struct Tread { std::vector<float2> poly; float top; };   // local plan (x forward, y left), top above the foot

} // namespace

void buildStair(const Building& B, const Stair& s, StairGeom& out) {
    out = StairGeom();
    const StairInfo I = stairInfo(B, s);
    const int lv = std::clamp(s.level, 0, (int)B.levels.size() - 1);
    const float base = B.levels[lv].elevation + 0.02f;   // on the floor finish
    const float h = I.riserH, g = I.tread, w = std::clamp(s.width, 0.5f, 3.0f);
    const int T = I.risers - 1;   // treads (the last riser reaches the floor above)
    const float2 f(std::cos(s.yaw), std::sin(s.yaw)), lf = perp(f);
    auto P = [&](float2 q) { return s.p + f * q.x + lf * q.y; };
    auto W3 = [&](float2 q, float z) { const float2 p = P(q); return float3(p.x, base + z, -p.y); };
    const float sg = s.turnRight ? -1.0f : 1.0f;
    std::vector<Tread> tr;
    std::vector<float2> walk, rail;   // local
    auto rect = [](float x0, float y0, float x1, float y1) { return std::vector<float2>{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}; };
    const float ri = w * 0.5f - 0.04f;   // the rail line, 4 cm in from the edge

    if (s.type == kStairSpiral) {
        const float r0 = 0.09f, r1 = w + r0, rm = (r0 + r1) * 0.5f;
        const float2 c(0, sg * rm);
        const float a = 2.0f * kPi / 13.0f, th0 = std::atan2(-sg * rm, 0.0f);
        auto at = [&](float r, float th) { return c + float2(std::cos(th), std::sin(th)) * r; };
        for (int k = 0; k < T; ++k) {
            const float ta = th0 + sg * k * a, tb = th0 + sg * (k + 1) * a, tm = (ta + tb) * 0.5f;
            tr.push_back({{at(r0, ta), at(r1, ta), at(r1, tm), at(r1, tb), at(r0, tb)}, (k + 1) * h});
            walk.push_back(at(rm, ta));
            rail.push_back(at(r1 - 0.04f, tm));
        }
        walk.push_back(at(rm, th0 + sg * T * a));
        // the central post
        std::vector<float3> post;
        for (int i = 0; i < 8; ++i) post.push_back(W3(at(r0, i * kPi / 4), 0));
        prism(out.steps, post, base, base + I.rise + 0.9f);
        out.footprint = {P(c + float2(-r1, -r1)), P(c + float2(r1, -r1)), P(c + float2(r1, r1)), P(c + float2(-r1, r1))};
    } else {
        const int m = s.type == kStairStraight ? T : std::max(1, T / 2);   // the landing's index
        for (int k = 0; k < std::min(m, T); ++k) tr.push_back({rect(k * g, -w * 0.5f, (k + 1) * g, w * 0.5f), (k + 1) * h});
        walk.push_back({0, 0});
        for (int k = 0; k < std::min(m, T); ++k) rail.push_back({k * g + g * 0.5f, -sg * ri});
        if (s.type == kStairStraight) {
            walk.push_back({T * g, 0});
            out.footprint = {P({0, -w * 0.5f}), P({T * g, -w * 0.5f}), P({T * g, w * 0.5f}), P({0, w * 0.5f})};
        } else {
            const float lx = m * g, gap = 0.10f;
            const bool U = s.type == kStairU;
            const float ly1 = U ? w * 0.5f + gap + w : w * 0.5f;
            // the landing (one riser up from the last tread of the first flight)
            std::vector<float2> land = rect(lx, -w * 0.5f, lx + w, ly1);
            if (sg < 0) for (float2& q : land) q.y = -q.y;
            tr.push_back({land, (m + 1) * h});
            rail.push_back({lx + w - 0.04f, -sg * ri});
            const int n2 = T - m - 1;
            if (!U) {   // L: the second flight to the side of the landing
                for (int j = 0; j < n2; ++j) {
                    std::vector<float2> q = rect(lx, w * 0.5f + j * g, lx + w, w * 0.5f + (j + 1) * g);
                    if (sg < 0) for (float2& p : q) p.y = -p.y;
                    tr.push_back({q, (m + 2 + j) * h});
                    rail.push_back({lx + w - 0.04f, sg * (w * 0.5f + j * g + g * 0.5f)});
                }
                walk.push_back({lx + w * 0.5f, 0});
                walk.push_back({lx + w * 0.5f, sg * (w * 0.5f + n2 * g)});
                const float y0 = std::min(-w * 0.5f, sg * (w * 0.5f + n2 * g)), y1 = std::max(w * 0.5f, sg * (w * 0.5f + n2 * g));
                out.footprint = {P({0, y0}), P({lx + w, y0}), P({lx + w, y1}), P({0, y1})};
            } else {    // U: back beside the first flight
                const float yc = w * 0.5f + gap + w * 0.5f;
                rail.push_back({lx + w - 0.04f, sg * (ly1 - 0.04f)});
                for (int j = 0; j < n2; ++j) {
                    std::vector<float2> q = rect(lx - (j + 1) * g, w * 0.5f + gap, lx - j * g, ly1);
                    if (sg < 0) for (float2& p : q) p.y = -p.y;
                    tr.push_back({q, (m + 2 + j) * h});
                    rail.push_back({lx - j * g - g * 0.5f, sg * (ly1 - 0.04f)});
                }
                walk.push_back({lx + w * 0.5f, 0});
                walk.push_back({lx + w * 0.5f, sg * yc});
                walk.push_back({lx - n2 * g, sg * yc});
                const float y0 = std::min(-w * 0.5f, sg * ly1), y1 = std::max(w * 0.5f, sg * ly1);
                out.footprint = {P({std::min(0.0f, lx - n2 * g), y0}), P({lx + w, y0}), P({lx + w, y1}), P({std::min(0.0f, lx - n2 * g), y1})};
            }
        }
    }

    // the steps: solid blocks down to the floor (closed) or treads on a spine (open)
    for (const Tread& t : tr) {
        std::vector<float3> poly;
        for (const float2& q : t.poly) poly.push_back(W3(q, 0));
        if (s.open) prism(out.steps, poly, base + t.top - 0.045f, base + t.top);
        else prism(out.steps, poly, base, base + t.top);
        for (size_t i = 0; i < t.poly.size(); ++i) out.lines.push_back({P(t.poly[i]), P(t.poly[(i + 1) % t.poly.size()])});
    }
    if (s.open && s.type != kStairSpiral && tr.size() >= 2) {   // a spine under each flight
        for (size_t i = 0; i + 1 < tr.size(); ++i) {
            float2 a(0, 0), b(0, 0);
            for (const float2& q : tr[i].poly) a = a + q;
            for (const float2& q : tr[i + 1].poly) b = b + q;
            a = a * (1.0f / tr[i].poly.size()); b = b * (1.0f / tr[i + 1].poly.size());
            beam(out.steps, W3(a, tr[i].top - 0.16f), W3(b, tr[i + 1].top - 0.16f), 0.12f, 0.20f);
        }
        float2 a0(0, 0);
        for (const float2& q : tr[0].poly) a0 = a0 + q;
        a0 = a0 * (1.0f / tr[0].poly.size());
        beam(out.steps, W3(a0 - float2(g, 0), 0.02f), W3(a0, tr[0].top - 0.16f), 0.12f, 0.20f);
    }
    // the handrail: posts every other step, the rail 90 cm over the nosings
    if (s.handrail && rail.size() >= 2) {
        std::vector<float3> pts;
        for (size_t i = 0; i < rail.size(); ++i) {
            const float top = i < tr.size() ? tr[i].top : tr.back().top;
            pts.push_back(W3(rail[i], top + 0.9f));
            if (i % 2 == 0 || i + 1 == rail.size()) beam(out.rail, W3(rail[i], top), W3(rail[i], top + 0.9f), 0.025f, 0.025f);
        }
        for (size_t i = 0; i + 1 < pts.size(); ++i) beam(out.rail, pts[i], pts[i + 1], 0.045f, 0.045f);
    }
    for (const float2& q : walk) out.walk.push_back(P(q));
    if (!tr.empty()) {
        const Tread& c = tr[std::min(tr.size() - 1, (size_t)(tr.size() * 0.55f))];
        out.cut = {P(c.poly[0]), P(c.poly[c.poly.size() / 2])};
    }
}

} // namespace archviz
