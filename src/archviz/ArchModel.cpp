#include "ArchModel.h"
#include "app/Json.h"
#include <algorithm>
#include <cstdio>
#include <map>

namespace archviz {

const char* openingName(int k) { return k == kDoor ? "Door" : k == kSlider ? "Sliding door" : "Window"; }
const char* floorName(int m) { return m == kFloorTile ? "Tiles" : m == kFloorConcrete ? "Concrete" : m == kFloorCarpet ? "Carpet" : "Oak floor"; }

float polyArea(const std::vector<float2>& p) {
    float a = 0;
    for (size_t i = 0; i < p.size(); ++i) a += cross2(p[i], p[(i + 1) % p.size()]);
    return a * 0.5f;
}

bool pointInPoly(float2 p, const std::vector<float2>& poly) {
    bool in = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const float2 a = poly[i], b = poly[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) in = !in;
    }
    return in;
}

void triangulate(const std::vector<float2>& poly, std::vector<int>& tris) {
    tris.clear();
    std::vector<int> idx;
    for (int i = 0; i < (int)poly.size(); ++i) {   // (collinear points dropped: they make zero-area ears)
        const float2 a = poly[(i + poly.size() - 1) % poly.size()], b = poly[i], c = poly[(i + 1) % poly.size()];
        if (std::fabs(cross2(b - a, c - b)) > 1e-7f) idx.push_back(i);
    }
    int guard = 0;
    while (idx.size() > 3 && guard++ < 10000) {
        bool cut = false;
        for (size_t k = 0; k < idx.size(); ++k) {
            const int i0 = idx[(k + idx.size() - 1) % idx.size()], i1 = idx[k], i2 = idx[(k + 1) % idx.size()];
            const float2 a = poly[i0], b = poly[i1], c = poly[i2];
            if (cross2(b - a, c - b) <= 1e-9f) continue;   // reflex
            bool inside = false;
            for (int j : idx) {
                if (j == i0 || j == i1 || j == i2) continue;
                const float2 p = poly[j];
                if (cross2(b - a, p - a) > 0 && cross2(c - b, p - b) > 0 && cross2(a - c, p - c) > 0) { inside = true; break; }
            }
            if (inside) continue;
            tris.push_back(i0); tris.push_back(i1); tris.push_back(i2);
            idx.erase(idx.begin() + k);
            cut = true;
            break;
        }
        if (!cut) break;
    }
    if (idx.size() == 3) { tris.push_back(idx[0]); tris.push_back(idx[1]); tris.push_back(idx[2]); }
}

static float segDist(float2 p, float2 a, float2 b, float* t = nullptr) {
    const float2 d = b - a;
    const float l2 = dot2(d, d);
    float s = l2 > 0 ? std::clamp(dot2(p - a, d) / l2, 0.0f, 1.0f) : 0.0f;
    if (t) *t = s;
    return len2(p - (a + d * s));
}

int Building::nodeAt(float2 p, int level, float tol) {
    for (int i = 0; i < (int)nodes.size(); ++i)
        if (nodes[i].alive && nodes[i].level == level && len2(nodes[i].p - p) <= tol) return i;
    for (int w = 0; w < (int)walls.size(); ++w) {
        const Wall& W = walls[w];
        if (!W.alive || nodes[W.a].level != level) continue;
        float t;
        const float d = segDist(p, nodes[W.a].p, nodes[W.b].p, &t);
        const float L = len2(nodes[W.b].p - nodes[W.a].p);
        if (d <= tol && t * L > tol && (1 - t) * L > tol) return splitWall(w, nodes[W.a].p + (nodes[W.b].p - nodes[W.a].p) * t);
    }
    nodes.push_back({p, level, true});
    return (int)nodes.size() - 1;
}

int Building::splitWall(int w, float2 p) {
    const int lv = nodes[walls[w].a].level;
    nodes.push_back({p, lv, true});
    const int n = (int)nodes.size() - 1;
    Wall second = walls[w];
    second.a = n;
    second.ops.clear();
    const float l1 = len2(p - nodes[walls[w].a].p);
    std::vector<Opening> keep;
    for (Opening o : walls[w].ops) {
        if (o.pos < l1) keep.push_back(o);
        else { o.pos -= l1; second.ops.push_back(o); }
    }
    walls[w].ops = keep;
    walls[w].b = n;
    walls.push_back(second);
    return n;
}

int Building::addWall(float2 A, float2 B, int level, float thickness, float height) {
    const float L = len2(B - A);
    if (L < 0.05f) return 0;
    const float tol = 0.01f;
    const int na = nodeAt(A, level, tol), nb = nodeAt(B, level, tol);
    A = nodes[na].p; B = nodes[nb].p;
    const float2 d = norm2(B - A);
    const float Ln = len2(B - A);
    std::vector<std::pair<float, int>> bp;   // (t along AB, node)
    bp.push_back({0.0f, na});
    bp.push_back({Ln, nb});
    auto onAB = [&](float2 p, float& t) {
        t = dot2(p - A, d);
        return t > tol && t < Ln - tol && std::fabs(cross2(d, p - A)) <= tol;
    };
    // existing nodes on the way (ends of other walls: T junctions, collinear overlaps)
    for (int i = 0; i < (int)nodes.size(); ++i) {
        if (!nodes[i].alive || nodes[i].level != level || i == na || i == nb) continue;
        float t;
        if (onAB(nodes[i].p, t)) bp.push_back({t, i});
    }
    // proper crossings with other walls
    const int nw = (int)walls.size();
    for (int w = 0; w < nw; ++w) {
        if (!walls[w].alive || nodes[walls[w].a].level != level) continue;
        const float2 P = nodes[walls[w].a].p, Q = nodes[walls[w].b].p;
        const float2 e = Q - P;
        const float den = cross2(d, e);
        if (std::fabs(den) < 1e-6f) continue;   // parallel (collinear overlaps: by their nodes above)
        const float t = cross2(P - A, e) / den;
        const float s = cross2(P - A, d) / den;
        const float le = len2(e);
        if (t > tol && t < Ln - tol && s * le > tol && (1 - s) * le > tol) bp.push_back({t, splitWall(w, A + d * t)});
    }
    std::sort(bp.begin(), bp.end());
    int added = 0;
    for (size_t i = 0; i + 1 < bp.size(); ++i) {
        const int n1 = bp[i].second, n2 = bp[i + 1].second;
        if (n1 == n2 || bp[i + 1].first - bp[i].first < tol) continue;
        bool exists = false;
        for (const Wall& w : walls)
            if (w.alive && ((w.a == n1 && w.b == n2) || (w.a == n2 && w.b == n1))) { exists = true; break; }
        if (exists) continue;
        Wall w;
        w.a = n1; w.b = n2; w.thickness = thickness; w.height = height;
        walls.push_back(w);
        ++added;
    }
    return added;
}

void Building::removeWall(int w) {
    walls[w].alive = false;
    for (int n : {walls[w].a, walls[w].b}) {
        bool used = false;
        for (const Wall& o : walls) if (o.alive && (o.a == n || o.b == n)) { used = true; break; }
        if (!used) nodes[n].alive = false;
    }
}

void Building::moveWall(int w, float d) {
    const float2 a = nodes[walls[w].a].p, b = nodes[walls[w].b].p;
    const float2 n = perp(norm2(b - a));
    nodes[walls[w].a].p = a + n * d;
    nodes[walls[w].b].p = b + n * d;
}

void Building::setWallLength(int w, float len) {
    if (len < 0.1f) return;
    const float2 a = nodes[walls[w].a].p, b = nodes[walls[w].b].p;
    nodes[walls[w].b].p = a + norm2(b - a) * len;
}

void Building::compact() {
    std::vector<int> remap(nodes.size(), -1);
    std::vector<Node> nn;
    for (size_t i = 0; i < nodes.size(); ++i) if (nodes[i].alive) { remap[i] = (int)nn.size(); nn.push_back(nodes[i]); }
    std::vector<Wall> nw;
    for (Wall w : walls) {
        if (!w.alive || remap[w.a] < 0 || remap[w.b] < 0) continue;
        w.a = remap[w.a]; w.b = remap[w.b];
        nw.push_back(w);
    }
    nodes = nn;
    walls = nw;
    std::vector<RoomLabel> nl;
    for (const RoomLabel& l : labels) if (l.alive) nl.push_back(l);
    labels = nl;
}

int Building::addLevel() {
    const Level& t = levels.back();
    Level l;
    l.name = "Level " + std::to_string(levels.size());
    l.elevation = t.elevation + t.height + 0.30f;
    l.height = t.height;
    levels.push_back(l);
    return (int)levels.size() - 1;
}

void Building::update() {
    computeGeometry();
    computeRooms();
}

// incident walls of every node, sorted counter-clockwise by the angle of their direction away from the node
static void incidence(const Building& B, std::vector<std::vector<std::pair<float, int>>>& inc) {
    inc.assign(B.nodes.size(), {});
    for (int w = 0; w < (int)B.walls.size(); ++w) {
        const Wall& W = B.walls[w];
        if (!W.alive) continue;
        const float2 d = B.nodes[W.b].p - B.nodes[W.a].p;
        inc[W.a].push_back({std::atan2(d.y, d.x), w});
        inc[W.b].push_back({std::atan2(-d.y, -d.x), w});
    }
    for (auto& v : inc) std::sort(v.begin(), v.end());
}

void Building::computeGeometry() {
    geom.assign(walls.size(), WallGeom());
    std::vector<std::vector<std::pair<float, int>>> inc;
    incidence(*this, inc);
    // corners per (wall, end): left / right seen from the node, looking along the wall
    std::vector<float2> cL(walls.size() * 2), cR(walls.size() * 2);
    for (int n = 0; n < (int)nodes.size(); ++n) {
        const auto& v = inc[n];
        const int k = (int)v.size();
        auto dirOf = [&](int w) { const Wall& W = walls[w]; return norm2(W.a == n ? nodes[W.b].p - nodes[W.a].p : nodes[W.a].p - nodes[W.b].p); };
        auto slot = [&](int w) { return w * 2 + (walls[w].a == n ? 0 : 1); };
        if (k == 1) {
            const int w = v[0].second;
            const float2 d = dirOf(w), l = perp(d);
            cL[slot(w)] = nodes[n].p + l * (walls[w].thickness * 0.5f);
            cR[slot(w)] = nodes[n].p - l * (walls[w].thickness * 0.5f);
            continue;
        }
        for (int i = 0; i < k; ++i) {
            const int wi = v[i].second, wj = v[(i + 1) % k].second;
            const float2 di = dirOf(wi), dj = dirOf(wj);
            const float ti = walls[wi].thickness * 0.5f, tj = walls[wj].thickness * 0.5f;
            const float2 Pi = nodes[n].p + perp(di) * ti, Pj = nodes[n].p - perp(dj) * tj;
            const float den = cross2(di, dj);
            float2 xi = Pi, xj = Pj;
            if (std::fabs(den) > 0.05f) {
                const float s = cross2(Pj - Pi, dj) / den;
                const float2 X = Pi + di * s;
                if (len2(X - nodes[n].p) < 4.0f * std::max(ti, tj) + 0.05f) xi = xj = X;
            }
            cL[slot(wi)] = xi;
            cR[slot(wj)] = xj;
        }
    }
    // joints: 3 or more walls leave a gap at the node between their (mitred / square) ends
    joints.clear();
    for (int n = 0; n < (int)nodes.size(); ++n) {
        const auto& v = inc[n];
        if (v.size() < 3) continue;
        Joint J;
        J.level = nodes[n].level;
        std::vector<std::pair<float, float2>> pts;
        for (const auto& e : v) {
            const int w = e.second, sl = w * 2 + (walls[w].a == n ? 0 : 1);
            J.height = std::max(J.height, walls[w].height);
            for (const float2 c : {cL[sl], cR[sl]}) {
                const float2 d = c - nodes[n].p;
                if (len2(d) > 1e-4f) pts.push_back({std::atan2(d.y, d.x), c});
            }
        }
        std::sort(pts.begin(), pts.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& q : pts)
            if (J.poly.empty() || (len2(q.second - J.poly.back()) > 1e-3f && len2(q.second - J.poly.front()) > 1e-3f)) J.poly.push_back(q.second);
        if (J.poly.size() >= 3) joints.push_back(std::move(J));
    }
    for (int w = 0; w < (int)walls.size(); ++w) {
        if (!walls[w].alive) continue;
        WallGeom& g = geom[w];
        g.a = nodes[walls[w].a].p; g.b = nodes[walls[w].b].p;
        g.len = len2(g.b - g.a);
        g.u = norm2(g.b - g.a); g.n = perp(g.u);
        g.aL = cL[w * 2]; g.aR = cR[w * 2];
        g.bR = cL[w * 2 + 1]; g.bL = cR[w * 2 + 1];
        g.sL0 = dot2(g.aL - g.a, g.u); g.sR0 = dot2(g.aR - g.a, g.u);
        g.sL1 = dot2(g.bL - g.a, g.u); g.sR1 = dot2(g.bR - g.a, g.u);
    }
}

void Building::computeRooms() {
    rooms.clear();
    std::vector<std::vector<std::pair<float, int>>> inc;
    incidence(*this, inc);
    // half-edges: 2w = a->b, 2w+1 = b->a
    auto from = [&](int h) { const Wall& W = walls[h / 2]; return h % 2 ? W.b : W.a; };
    auto to = [&](int h) { const Wall& W = walls[h / 2]; return h % 2 ? W.a : W.b; };
    auto outEdge = [&](int n, int w) { return walls[w].a == n ? w * 2 : w * 2 + 1; };
    std::vector<char> used(walls.size() * 2, 0);
    for (int h0 = 0; h0 < (int)walls.size() * 2; ++h0) {
        if (!walls[h0 / 2].alive || used[h0]) continue;
        std::vector<int> cyc;
        int h = h0, guard = 0;
        bool ok = true;
        do {
            used[h] = 1;
            cyc.push_back(from(h));
            const int v = to(h);
            const auto& lst = inc[v];
            int pos = -1;
            for (int i = 0; i < (int)lst.size(); ++i) if (lst[i].second == h / 2) { pos = i; break; }
            if (pos < 0) { ok = false; break; }
            const int nxt = lst[(pos + lst.size() - 1) % lst.size()].second;   // clockwise neighbour of the way back
            h = outEdge(v, nxt);
            if (lst.size() == 1) h = outEdge(v, h / 2);   // dead end: come back
        } while (h != h0 && ++guard < 10000);
        if (!ok || cyc.size() < 3) continue;
        Room r;
        for (int n : cyc) r.poly.push_back(nodes[n].p);
        const float a = polyArea(r.poly);
        if (a < 0.2f) continue;   // the outer face (negative) and slivers
        r.level = nodes[cyc[0]].level;
        // inside the walls: minus half of each wall around
        float inner = a;
        for (size_t i = 0; i < cyc.size(); ++i) {
            const int n1 = cyc[i], n2 = cyc[(i + 1) % cyc.size()];
            for (const Wall& w : walls)
                if (w.alive && ((w.a == n1 && w.b == n2) || (w.a == n2 && w.b == n1))) { inner -= len2(nodes[n2].p - nodes[n1].p) * w.thickness * 0.5f; break; }
        }
        r.area = std::max(0.0f, inner);
        float cx = 0, cy = 0;
        for (size_t i = 0; i < r.poly.size(); ++i) {
            const float2 p = r.poly[i], q = r.poly[(i + 1) % r.poly.size()];
            const float c = cross2(p, q);
            cx += (p.x + q.x) * c; cy += (p.y + q.y) * c;
        }
        r.centre = float2(cx / (6 * a), cy / (6 * a));
        for (int l = 0; l < (int)labels.size(); ++l)
            if (labels[l].alive && labels[l].level == r.level && pointInPoly(labels[l].p, r.poly)) { r.label = l; break; }
        if (!pointInPoly(r.centre, r.poly)) {   // (L shapes: a point inside for the name)
            std::vector<int> t;
            triangulate(r.poly, t);
            float best = 0;
            for (size_t i = 0; i + 2 < t.size(); i += 3) {
                const float2 A = r.poly[t[i]], B = r.poly[t[i + 1]], C = r.poly[t[i + 2]];
                const float ta = std::fabs(cross2(B - A, C - A));
                if (ta > best) { best = ta; r.centre = (A + B + C) * (1.0f / 3.0f); }
            }
        }
        rooms.push_back(r);
    }
}

void Building::openingRange(int w, float& lo, float& hi) const {
    const WallGeom& g = geom[w];
    lo = std::max(g.sL0, g.sR0) + 0.05f;
    hi = std::min(g.sL1, g.sR1) - 0.05f;
}

void Building::clampOpening(int w, Opening& o) const {
    float lo, hi;
    openingRange(w, lo, hi);
    o.width = std::clamp(o.width, 0.3f, std::max(0.3f, hi - lo));
    o.pos = std::clamp(o.pos, lo + o.width * 0.5f, std::max(lo + o.width * 0.5f, hi - o.width * 0.5f));
    const float H = walls[w].height;
    if (o.kind != kWindow) o.sill = 0;
    o.height = std::clamp(o.height, 0.3f, H - 0.05f);
    o.sill = std::clamp(o.sill, 0.0f, std::max(0.0f, H - 0.05f - o.height));
}

float Building::openingSide(int w, const Opening& o) const {
    float side = -1.0f;
    if (w >= 0 && w < (int)walls.size() && w < (int)geom.size()) {
        const WallGeom& g = geom[w];
        const int lv = nodes[walls[w].a].level;
        const float2 c = g.a + g.u * o.pos;
        const float d = walls[w].thickness * 0.5f + 0.15f;
        const bool plus = roomAt(c + g.n * d, lv) >= 0, minus = roomAt(c - g.n * d, lv) >= 0;
        if (plus && !minus) side = 1.0f;
    }
    return (o.flip & 1) ? -side : side;
}

int Building::roomAt(float2 p, int level) const {
    for (int r = 0; r < (int)rooms.size(); ++r) if (rooms[r].level == level && pointInPoly(p, rooms[r].poly)) return r;
    return -1;
}

int Building::wallAt(float2 p, int level, float tol, float* along) const {
    int best = -1;
    float bd = 1e30f;
    for (int w = 0; w < (int)walls.size(); ++w) {
        if (!walls[w].alive || nodes[walls[w].a].level != level) continue;
        float t;
        const float d = segDist(p, nodes[walls[w].a].p, nodes[walls[w].b].p, &t) - walls[w].thickness * 0.5f;
        if (d <= tol && d < bd) { bd = d; best = w; if (along) *along = t * geom[w].len; }
    }
    return best;
}

// --------------------------------------------------------------------------- JSON

std::string Building::toJson() const {
    std::string s = "{\n  \"format\": \"archviz-building\", \"version\": 1, \"ground\": " + std::to_string(ground) + ",\n  \"levels\": [";
    char b[512];
    for (size_t i = 0; i < levels.size(); ++i) {
        const Level& L = levels[i];
        snprintf(b, sizeof b, "%s\n    {\"name\": \"%s\", \"elevation\": %.4f, \"height\": %.4f, \"roof\": %d, \"roofSlope\": %.2f, \"roofOverhang\": %.3f, \"roofThick\": %.3f, \"roofRidge\": %d, \"roofFinish\": %d}",
                 i ? "," : "", L.name.c_str(), L.elevation, L.height, L.roof, L.roofSlope, L.roofOverhang, L.roofThick, L.roofRidge, L.roofFinish);
        s += b;
    }
    s += "\n  ],\n  \"nodes\": [";
    std::vector<int> remap(nodes.size(), -1);
    int k = 0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i].alive) continue;
        remap[i] = k;
        snprintf(b, sizeof b, "%s\n    [%.4f, %.4f, %d]", k ? "," : "", nodes[i].p.x, nodes[i].p.y, nodes[i].level);
        s += b;
        ++k;
    }
    s += "\n  ],\n  \"walls\": [";
    k = 0;
    for (const Wall& w : walls) {
        if (!w.alive || remap[w.a] < 0 || remap[w.b] < 0) continue;
        snprintf(b, sizeof b, "%s\n    {\"a\": %d, \"b\": %d, \"thickness\": %.4f, \"height\": %.4f, \"faceL\": [%d, %u, %.3f], \"faceR\": [%d, %u, %.3f], \"openings\": [", k++ ? "," : "",
                 remap[w.a], remap[w.b], w.thickness, w.height, w.faceL.kind, w.faceL.color, w.faceL.tile, w.faceR.kind, w.faceR.color, w.faceR.tile);
        s += b;
        for (size_t i = 0; i < w.ops.size(); ++i) {
            const Opening& o = w.ops[i];
            snprintf(b, sizeof b, "%s{\"kind\": %d, \"pos\": %.4f, \"width\": %.4f, \"height\": %.4f, \"sill\": %.4f, \"flip\": %d, \"style\": %d, \"leaves\": %d, \"cols\": %d, \"rows\": %d, \"finish\": %d, \"inset\": %.4f, \"open\": %.3f, \"leafOpen\": [%.3f, %.3f, %.3f, %.3f]}",
                     i ? ", " : "", o.kind, o.pos, o.width, o.height, o.sill, o.flip, o.style, o.leaves, o.cols, o.rows, o.finish, o.inset, o.open, o.leafOpen[0], o.leafOpen[1], o.leafOpen[2], o.leafOpen[3]);
            s += b;
        }
        s += "]}";
    }
    s += "\n  ],\n  \"rooms\": [";
    k = 0;
    for (const RoomLabel& l : labels) {
        if (!l.alive) continue;
        snprintf(b, sizeof b, "%s\n    {\"x\": %.4f, \"y\": %.4f, \"name\": \"%s\", \"floor\": %d, \"level\": %d, \"floorColor\": %u, \"floorAngle\": %.2f, \"floorTile\": %.3f}", k++ ? "," : "", l.p.x, l.p.y, l.name.c_str(), l.floor, l.level,
                 l.floorColor, l.floorAngle, l.floorTile);
        s += b;
    }
    s += "\n  ],\n  \"items\": [";
    k = 0;
    for (const Item& it : items) {
        snprintf(b, sizeof b, "%s\n    {\"asset\": \"%s\", \"x\": %.4f, \"y\": %.4f, \"yaw\": %.5f, \"elev\": %.4f, \"level\": %d, \"w\": %.4f, \"d\": %.4f, \"h\": %.4f}", k++ ? "," : "",
                 it.asset.c_str(), it.p.x, it.p.y, it.yaw, it.elev, it.level, it.size.x, it.size.y, it.height);
        s += b;
    }
    s += "\n  ],\n  \"stairs\": [";
    k = 0;
    for (const Stair& st : stairs) {
        if (!st.alive) continue;
        snprintf(b, sizeof b, "%s\n    {\"x\": %.4f, \"y\": %.4f, \"yaw\": %.5f, \"type\": %d, \"level\": %d, \"width\": %.3f, \"rise\": %.3f, \"risers\": %d, \"handrail\": %d, \"open\": %d, \"turnRight\": %d, \"finish\": %d}",
                 k++ ? "," : "", st.p.x, st.p.y, st.yaw, st.type, st.level, st.width, st.rise, st.risers, (int)st.handrail, (int)st.open, (int)st.turnRight, st.finish);
        s += b;
    }
    s += "\n  ]\n}\n";
    return s;
}

bool Building::fromJson(const std::string& text) {
    app::Json j;
    if (!app::Json::parse(text, j) || j["format"].str() != "archviz-building") return false;
    Building B;
    B.levels.clear();
    B.ground = (int)j["ground"].num();
    for (const app::Json& l : j["levels"].a) {
        Level L;
        L.name = l["name"].str("Level"); L.elevation = (float)l["elevation"].num(); L.height = (float)l["height"].num(2.7);
        L.roof = (int)l["roof"].num(kRoofGable); L.roofSlope = (float)l["roofSlope"].num(35); L.roofOverhang = (float)l["roofOverhang"].num(0.4);
        L.roofThick = (float)l["roofThick"].num(0.25); L.roofRidge = (int)l["roofRidge"].num(); L.roofFinish = (int)l["roofFinish"].num();
        B.levels.push_back(L);
    }
    if (B.levels.empty()) B.levels.push_back({"Level 0", 0, 2.7f});
    for (const app::Json& n : j["nodes"].a) B.nodes.push_back({float2((float)n[0].num(), (float)n[1].num()), (int)n[2].num(), true});
    for (const app::Json& w : j["walls"].a) {
        Wall W;
        W.a = (int)w["a"].num(); W.b = (int)w["b"].num();
        if (W.a < 0 || W.b < 0 || W.a >= (int)B.nodes.size() || W.b >= (int)B.nodes.size()) continue;
        W.thickness = (float)w["thickness"].num(0.3); W.height = (float)w["height"].num(2.7);
        auto face = [](const app::Json& a, WallFace& F) { if (a.a.size() >= 3) { F.kind = (int)a[0].num(); F.color = (uint32_t)a[1].num(); F.tile = (float)a[2].num(0.2); } };
        face(w["faceL"], W.faceL); face(w["faceR"], W.faceR);
        for (const app::Json& o : w["openings"].a) {
            Opening O;
            O.kind = (int)o["kind"].num(); O.pos = (float)o["pos"].num(1); O.width = (float)o["width"].num(1.2);
            O.height = (float)o["height"].num(1.35); O.sill = (float)o["sill"].num(0.9); O.flip = (int)o["flip"].num();
            O.style = (int)o["style"].num(); O.leaves = (int)o["leaves"].num(); O.cols = (int)o["cols"].num(1); O.rows = (int)o["rows"].num(1);
            O.finish = (int)o["finish"].num(); O.inset = (float)o["inset"].num(); O.open = (float)o["open"].num();   // (older files: the defaults)
            O.setAllOpen(O.open);
            for (size_t k = 0; k < 4 && k < o["leafOpen"].a.size(); ++k) O.leafOpen[k] = (float)o["leafOpen"][(int)k].num();
            W.ops.push_back(O);
        }
        B.walls.push_back(W);
    }
    for (const app::Json& r : j["rooms"].a)
    {
        RoomLabel L{float2((float)r["x"].num(), (float)r["y"].num()), r["name"].str("Room"), (int)r["floor"].num(), (int)r["level"].num(), true};
        L.floorColor = (uint32_t)r["floorColor"].num(); L.floorAngle = (float)r["floorAngle"].num(); L.floorTile = (float)r["floorTile"].num(0.6);
        B.labels.push_back(L);
    }
    for (const app::Json& r : j["items"].a) {
        Item it;
        it.asset = r["asset"].str();
        it.p = float2((float)r["x"].num(), (float)r["y"].num());
        it.yaw = (float)r["yaw"].num(); it.elev = (float)r["elev"].num(); it.level = (int)r["level"].num();
        it.size = float2((float)r["w"].num(1), (float)r["d"].num(1)); it.height = (float)r["h"].num(1);
        if (!it.asset.empty()) B.items.push_back(it);
    }
    for (const app::Json& r : j["stairs"].a) {
        Stair st;
        st.p = float2((float)r["x"].num(), (float)r["y"].num());
        st.yaw = (float)r["yaw"].num(); st.type = (int)r["type"].num(); st.level = (int)r["level"].num();
        st.width = (float)r["width"].num(0.9); st.rise = (float)r["rise"].num(); st.risers = (int)r["risers"].num();
        st.handrail = r["handrail"].num(1) != 0; st.open = r["open"].num() != 0; st.turnRight = r["turnRight"].num() != 0;
        st.finish = (int)r["finish"].num();
        B.stairs.push_back(st);
    }
    *this = B;
    update();
    return true;
}

// --------------------------------------------------------------------------- demo

Building Building::demoHouse() {
    Building B;
    const int L = 0;
    const float te = 0.30f, ti = 0.12f, h = 2.70f;
    const float2 P[] = {{0, 0}, {12, 0}, {12, 6}, {7, 6}, {7, 9}, {0, 9}};
    for (int i = 0; i < 6; ++i) B.addWall(P[i], P[(i + 1) % 6], L, te, h);
    B.addWall({4.6f, 0}, {4.6f, 9}, L, ti, h);
    B.addWall({0, 4.8f}, {4.6f, 4.8f}, L, ti, h);
    B.addWall({9.4f, 0}, {9.4f, 6}, L, ti, h);
    B.update();
    auto put = [&](float2 p, int kind, float w, float hh, float sill) {
        float along;
        const int wi = B.wallAt(p, L, 0.05f, &along);
        if (wi < 0) return;
        Opening o;
        o.kind = kind; o.pos = along; o.width = w; o.height = hh; o.sill = sill;
        B.clampOpening(wi, o);
        B.walls[wi].ops.push_back(o);
    };
    put({2.3f, 0}, kWindow, 1.4f, 1.35f, 0.9f);
    put({6.9f, 0}, kSlider, 3.0f, 2.25f, 0);
    put({10.7f, 0}, kWindow, 1.2f, 1.35f, 0.9f);
    put({12, 3}, kWindow, 1.6f, 1.35f, 0.9f);
    put({9.6f, 6}, kWindow, 1.2f, 1.35f, 0.9f);
    put({7, 7.5f}, kDoor, 1.0f, 2.15f, 0);
    put({5.0f, 9}, kWindow, 1.6f, 1.35f, 0.9f);
    put({1.9f, 9}, kWindow, 1.2f, 1.35f, 0.9f);
    put({0, 2.2f}, kWindow, 1.2f, 1.35f, 0.9f);
    put({0, 6.6f}, kDoor, 1.0f, 2.15f, 0);
    put({4.6f, 6.4f}, kDoor, 0.9f, 2.10f, 0);
    put({3.2f, 4.8f}, kDoor, 0.8f, 2.10f, 0);
    put({9.4f, 4.6f}, kDoor, 0.8f, 2.10f, 0);
    B.labels.push_back({{2.3f, 2.4f}, "Bedroom", kFloorWood, L, true});
    B.labels.push_back({{2.3f, 6.9f}, "Entrance", kFloorTile, L, true});
    B.labels.push_back({{6.8f, 3.5f}, "Living", kFloorWood, L, true});
    B.labels.push_back({{10.7f, 3.0f}, "Kitchen", kFloorTile, L, true});
    B.update();
    return B;
}

} // namespace archviz
