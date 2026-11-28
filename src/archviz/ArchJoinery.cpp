#include "ArchJoinery.h"
#include <algorithm>
#include <cmath>

namespace archviz {

using namespace vp3d;

static const float kDeg = 3.14159265f / 180.0f;

int joineryStyleCount(int kind) { return kind == kDoor ? kDoorStyleCount : kind == kSlider ? kSlideStyleCount : kWinStyleCount; }

const char* joineryStyleName(int kind, int style) {
    if (kind == kDoor) {
        static const char* n[] = {"Panel", "Glazed", "Entrance", "French"};
        return n[std::clamp(style, 0, kDoorStyleCount - 1)];
    }
    if (kind == kSlider) {
        static const char* n[] = {"Bypass", "Pocket"};
        return n[std::clamp(style, 0, kSlideStyleCount - 1)];
    }
    static const char* n[] = {"Casement", "Fixed", "Tilt & turn", "Awning", "Sliding"};
    return n[std::clamp(style, 0, kWinStyleCount - 1)];
}

const char* joineryFinishName(int finish) {
    static const char* n[] = {"Anthracite", "White", "Oak", "Black"};
    return n[std::clamp(finish, 0, kFinishCount - 1)];
}

static int minLeaves(const Opening& o) {
    return (o.kind == kSlider && o.style == kSlideBypass) || (o.kind == kWindow && o.style == kWinSliding) ? 2 : 1;
}

int joineryMaxLeaves(const Opening& o) {
    if (o.kind == kDoor) return o.style == kDoorEntrance ? 1 : 2;
    if (o.kind == kSlider) return o.style == kSlidePocket ? 2 : 4;
    switch (o.style) {
    case kWinFixed: case kWinAwning: return 1;
    case kWinSliding: return 4;
    default: return 2;
    }
}

int joineryLeaves(const Opening& o) {
    const int lo = minLeaves(o), hi = joineryMaxLeaves(o);
    if (o.leaves > 0) return std::clamp(o.leaves, lo, hi);
    const float W = o.width;
    int n = 1;
    if (o.kind == kDoor) n = o.style == kDoorFrench ? (W > 1.0f ? 2 : 1) : (W > 1.3f ? 2 : 1);
    else if (o.kind == kSlider) n = o.style == kSlidePocket ? 1 : (W < 2.2f ? 2 : W < 3.4f ? 3 : 4);
    else if (o.style == kWinSliding) n = W < 2.4f ? 2 : 3;
    else n = W > 1.1f ? 2 : 1;
    return std::clamp(n, lo, hi);
}

namespace {

void box(MeshData& m, float x0, float y0, float z0, float x1, float y1, float z1) {
    m.append(makeBox(float3(std::min(x0, x1), std::min(y0, y1), std::min(z0, z1)), float3(std::max(x0, x1), std::max(y0, y1), std::max(z0, z1))));
}

// a single thin pane (glass material, MAT_FLAG_THIN: a ray crosses it in one hit)
void pane(MeshData& m, float x0, float y0, float x1, float y1, float z) {
    m.addQuad(float3(x0, y0, z), float3(x1, y0, z), float3(x1, y1, z), float3(x0, y1, z), float3(0, 0, 1));
}

// glazing bars (cols x rows lights) over [x0, x1] x [y0, y1], across the glass plane z
void bars(MeshData& m, float x0, float y0, float x1, float y1, float z, int cols, int rows, float depth) {
    const float bw = 0.024f, bd = depth * 0.5f;
    for (int c = 1; c < cols; ++c) {
        const float x = x0 + (x1 - x0) * c / cols;
        box(m, x - bw * 0.5f, y0, z - bd, x + bw * 0.5f, y1, z + bd);
    }
    for (int r = 1; r < rows; ++r) {
        const float y = y0 + (y1 - y0) * r / rows;
        box(m, x0, y - bw * 0.5f, z - bd, x1, y + bw * 0.5f, z + bd);
    }
}

// A sash / glazed leaf: rails rw wide (bottom rail rb), d deep, centred on zc, with its glass and glazing bars.
void glazedLeaf(JoineryPiece& p, int railMat, float x0, float y0, float x1, float y1, float zc, float d, float rw, float rb,
                int cols, int rows) {
    MeshData& F = p.mesh[railMat];
    const float z0 = zc - d * 0.5f, z1 = zc + d * 0.5f;
    box(F, x0, y0, z0, x0 + rw, y1, z1);
    box(F, x1 - rw, y0, z0, x1, y1, z1);
    box(F, x0 + rw, y0, z0, x1 - rw, y0 + rb, z1);
    box(F, x0 + rw, y1 - rw, z0, x1 - rw, y1, z1);
    pane(p.mesh[kJGlass], x0 + rw, y0 + rb, x1 - rw, y1 - rw, zc);
    bars(F, x0 + rw, y0 + rb, x1 - rw, y1 - rw, zc, cols, rows, d);
}

// Lever handle with its rose on the face z (sign: the side it stands out to). door: horizontal lever pointing to dir
// (+1 = +x), else a window handle hanging down.
void handle(MeshData& m, float x, float y, float z, float sign, bool door, float dir) {
    box(m, x - 0.017f, y - 0.035f, z, x + 0.017f, y + 0.035f, z + sign * 0.010f);
    const float zf = z + sign * 0.010f, zb = z + sign * 0.050f;
    box(m, x - 0.009f, y - 0.009f, zf, x + 0.009f, y + 0.009f, zb);   // the neck
    if (door) box(m, x, y - 0.011f, zb - sign * 0.018f, x + dir * 0.135f, y + 0.011f, zb);
    else box(m, x - 0.011f, y - 0.115f, zb - sign * 0.018f, x + 0.011f, y + 0.005f, zb);
}

// a vertical pull of a sliding panel on the face z
void pull(MeshData& m, float x, float yc, float z, float sign) {
    box(m, x - 0.010f, yc - 0.16f, z, x + 0.010f, yc + 0.16f, z + sign * 0.022f);
}

void offsetPiece(JoineryPiece& p) {
    for (MeshData& m : p.mesh)
        for (Vertex& v : m.vertices) v.pos = v.pos - p.pivot;
}

} // namespace

void buildJoinery(const Opening& o, float wallThickness, float sideN, std::vector<JoineryPiece>& out) {
    out.clear();
    const float W = std::max(0.2f, o.width), H = std::max(0.2f, o.height);
    const bool door = o.kind == kDoor, slider = o.kind == kSlider;
    const bool winSliding = o.kind == kWindow && o.style == kWinSliding;
    const float F = o.finish == kFinishOak ? 0.068f : 0.055f;                       // face of the frame
    const float D = std::clamp((door || slider) ? 0.11f : 0.08f, 0.04f, std::max(0.04f, wallThickness - 0.01f));   // depth
    const float zc = -o.inset;                                                        // local z = -offset along n
    const float sz = sideN > 0 ? -1.0f : 1.0f;                                        // the side the leaves open to (local z = -n)
    const bool hingeRight = (o.flip & 2) != 0;
    const int cols = std::clamp(o.cols, 1, 8), rows = std::clamp(o.rows, 1, 8);
    const int n = joineryLeaves(o);

    // the fixed frame: jambs, head, bottom rail (windows) or a threshold (doors, sliding doors)
    JoineryPiece fixed;
    MeshData& FM = fixed.mesh[kJFrame];
    const float z0 = zc - D * 0.5f, z1 = zc + D * 0.5f;
    box(FM, 0, 0, z0, F, H, z1);
    box(FM, W - F, 0, z0, W, H, z1);
    box(FM, F, H - F, z0, W - F, H, z1);
    const bool rail = !(door || slider);
    const float ix0 = F, ix1 = W - F, iy0 = rail ? F : 0.012f, iy1 = H - F;
    if (rail) box(FM, F, 0, z0, W - F, F, z1);
    else box(FM, F, 0, z0, W - F, 0.012f, z1);

    auto addMoving = [&](JoineryPiece& p) { p.moving = true; offsetPiece(p); out.push_back(std::move(p)); };

    if (o.kind == kWindow && o.style == kWinFixed) {
        pane(fixed.mesh[kJGlass], ix0, iy0, ix1, iy1, zc);
        bars(FM, ix0, iy0, ix1, iy1, zc, cols, rows, D * 0.6f);
        out.push_back(std::move(fixed));
        return;
    }

    if (slider || winSliding) {
        const bool pocket = slider && o.style == kSlidePocket;
        const float r = winSliding ? 0.06f : 0.055f, d = 0.026f;
        if (pocket) {   // the leaves run into the wall (both ways with two)
            const float lw = (ix1 - ix0) / n;
            for (int i = 0; i < n; ++i) {
                JoineryPiece p;
                const float x0 = ix0 + i * lw, x1 = x0 + lw;
                glazedLeaf(p, kJFrame, x0, iy0, x1, iy1, zc, d, r, r, cols, rows);
                const bool toRight = n == 2 ? i == 1 : hingeRight;
                pull(p.mesh[kJHandle], toRight ? x0 + 0.05f : x1 - 0.05f, (iy0 + iy1) * 0.5f, zc + sz * d * 0.5f, sz);
                p.slide = float3(toRight ? lw - 0.06f : -(lw - 0.06f), 0, 0);
                addMoving(p);
            }
        } else {        // bypass: panels on their own tracks, stacked onto the first (or the last) when open
            const float ov = 0.05f;
            const float pw = (ix1 - ix0 + (n - 1) * ov) / n;
            for (int i = 0; i < n; ++i) {
                JoineryPiece p;
                const float x0 = ix0 + i * (pw - ov), x1 = x0 + pw;
                const float zt = zc + (i - (n - 1) * 0.5f) * (d + 0.004f);
                glazedLeaf(p, kJFrame, x0, iy0, x1, iy1, zt, d, r, r, cols, rows);
                const int anchor = hingeRight ? n - 1 : 0;
                if (i != anchor) pull(p.mesh[kJHandle], i > anchor ? x0 + 0.05f : x1 - 0.05f, (iy0 + iy1) * 0.5f, zt + sz * d * 0.5f, sz);
                p.slide = float3((anchor - i) * (pw - ov), 0, 0);
                if (i == anchor) out.push_back(std::move(p));
                else addMoving(p);
            }
        }
        out.push_back(std::move(fixed));
        return;
    }

    if (o.kind == kWindow) {   // casement, tilt & turn, awning: sashes inside the frame
        const float r = 0.062f, d = std::min(0.062f, D * 0.85f);
        const float lw = (ix1 - ix0) / n;
        for (int i = 0; i < n; ++i) {
            JoineryPiece p;
            const float x0 = ix0 + i * lw, x1 = x0 + lw;
            glazedLeaf(p, kJFrame, x0, iy0, x1, iy1, zc, d, r, r, cols, rows);
            const bool leftHinge = n == 2 ? i == 0 : !hingeRight;
            const bool hasHandle = n == 1 || i == 1;
            if (o.style == kWinAwning) {   // hinged at the top, the bottom swings out
                handle(p.mesh[kJHandle], (x0 + x1) * 0.5f, iy0 + r * 0.5f + 0.02f, zc + sz * d * 0.5f, sz, false, 0);
                p.pivot = float3(x0, iy1, zc - sz * d * 0.5f);
                p.axis = float3(1, 0, 0);
                p.angle = sz * 32.0f * kDeg;
                addMoving(p);
                continue;
            }
            if (hasHandle) {
                const float hx = leftHinge ? x1 - r * 0.5f : x0 + r * 0.5f;
                handle(p.mesh[kJHandle], hx, iy0 + (iy1 - iy0) * 0.5f, zc + sz * d * 0.5f, sz, false, 0);
            }
            if (o.style == kWinTiltTurn) {   // the sash with the handle tilts in at the top (the other one stays)
                if (!hasHandle) { out.push_back(std::move(p)); continue; }
                p.pivot = float3(x0, iy0, zc + sz * d * 0.5f);
                p.axis = float3(1, 0, 0);
                p.angle = sz * 13.0f * kDeg;
            } else {   // casement: around its hinge stile, into the side sz
                p.pivot = float3(leftHinge ? x0 : x1, iy0, zc + sz * d * 0.5f);
                p.axis = float3(0, 1, 0);
                p.angle = (leftHinge ? -sz : sz) * 90.0f * kDeg;
            }
            addMoving(p);
        }
        out.push_back(std::move(fixed));
        return;
    }

    // doors: leaves flush with the frame face on the swing side; an entrance door has a glazed side light
    const float ld = 0.045f;
    const float lz = zc + sz * (D * 0.5f - ld * 0.5f);
    float lx0 = ix0, lx1 = ix1;
    if (o.style == kDoorEntrance) {
        const float sl = std::min(0.42f, (ix1 - ix0) * 0.3f);
        const float mx = hingeRight ? ix0 + sl : ix1 - sl;   // the mullion between the leaf and the side light
        box(FM, mx - F * 0.5f, 0, z0, mx + F * 0.5f, H, z1);
        if (hingeRight) { pane(fixed.mesh[kJGlass], ix0, iy0, mx - F * 0.5f, iy1, zc); bars(FM, ix0, iy0, mx - F * 0.5f, iy1, zc, 1, rows, D * 0.6f); lx0 = mx + F * 0.5f; }
        else { pane(fixed.mesh[kJGlass], mx + F * 0.5f, iy0, ix1, iy1, zc); bars(FM, mx + F * 0.5f, iy0, ix1, iy1, zc, 1, rows, D * 0.6f); lx1 = mx - F * 0.5f; }
    }
    const float lw = (lx1 - lx0) / n;
    const bool glazed = o.style == kDoorGlazed || o.style == kDoorFrench;
    for (int i = 0; i < n; ++i) {
        JoineryPiece p;
        const float x0 = lx0 + i * lw + 0.003f, x1 = lx0 + (i + 1) * lw - 0.003f;
        const bool leftHinge = n == 2 ? i == 0 : (o.style == kDoorEntrance ? !hingeRight : !hingeRight);
        if (glazed) {
            const float rw = o.style == kDoorFrench ? 0.075f : 0.11f, rb = o.style == kDoorFrench ? 0.16f : 0.22f;
            glazedLeaf(p, kJPanel, x0, iy0, x1, iy1 - 0.003f, lz, ld, rw, rb, cols, rows);
        } else {
            box(p.mesh[kJPanel], x0, iy0, lz - ld * 0.5f, x1, iy1 - 0.003f, lz + ld * 0.5f);
            if (o.style == kDoorEntrance)   // a vertical glass strip in the leaf
                pane(p.mesh[kJGlass], (x0 + x1) * 0.5f - 0.06f + (leftHinge ? 0.08f : -0.08f), iy0 + 0.25f,
                     (x0 + x1) * 0.5f + 0.06f + (leftHinge ? 0.08f : -0.08f), iy1 - 0.25f, lz + sz * (ld * 0.5f + 0.001f));
        }
        const bool hasHandle = n == 1 || i == 1;
        if (hasHandle || o.style == kDoorFrench) {
            const float hx = leftHinge ? x1 - 0.075f : x0 + 0.075f, dir = leftHinge ? -1.0f : 1.0f;
            const float hy = std::min(1.02f, (iy1 - iy0) * 0.48f);
            handle(p.mesh[kJHandle], hx, hy, lz + sz * ld * 0.5f, sz, true, dir);
            handle(p.mesh[kJHandle], hx, hy, lz - sz * ld * 0.5f, -sz, true, dir);
        }
        p.pivot = float3(leftHinge ? x0 : x1, iy0, lz + sz * ld * 0.5f);
        p.axis = float3(0, 1, 0);
        p.angle = (leftHinge ? -sz : sz) * 92.0f * kDeg;
        addMoving(p);
    }
    out.push_back(std::move(fixed));
}

} // namespace archviz
