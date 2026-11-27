#include "ArchMesh.h"
#include "vp3d/MeshGen.h"
#include "vp3d/shaders/Shared.h"
#include <algorithm>

namespace archviz {

using namespace vp3d;

static float3 lin(int hex) {
    return float3(srgbToLinear(((hex >> 16) & 255) / 255.0f), srgbToLinear(((hex >> 8) & 255) / 255.0f), srgbToLinear((hex & 255) / 255.0f));
}

// quad with its corners turned to face n (whatever order they come in)
static void quad(MeshData& m, float3 a, float3 b, float3 c, float3 d, float3 n) {
    if (dot(cross(b - a, c - a), n) < 0) std::swap(b, d);
    m.addQuad(a, b, c, d, n);
}

// A box in the frame of a wall: s along the axis (from a), o along the left normal, z up (above the floor at elev).
struct Frame {
    float2 a, u, n;
    float elev;
    float3 P(float s, float o, float z) const { return world(a + u * s + n * o, elev + z); }
    float3 U() const { return world(u, 0) - world(float2(0, 0), 0); }
    float3 N() const { return world(n, 0) - world(float2(0, 0), 0); }
};

static void box(MeshData& m, const Frame& f, float s0, float s1, float o0, float o1, float z0, float z1) {
    const float3 U = f.U(), N = f.N(), Y(0, 1, 0);
    quad(m, f.P(s0, o1, z0), f.P(s1, o1, z0), f.P(s1, o1, z1), f.P(s0, o1, z1), N);
    quad(m, f.P(s0, o0, z0), f.P(s1, o0, z0), f.P(s1, o0, z1), f.P(s0, o0, z1), -N);
    quad(m, f.P(s0, o0, z1), f.P(s1, o0, z1), f.P(s1, o1, z1), f.P(s0, o1, z1), Y);
    quad(m, f.P(s0, o0, z0), f.P(s1, o0, z0), f.P(s1, o1, z0), f.P(s0, o1, z0), -Y);
    quad(m, f.P(s0, o0, z0), f.P(s0, o1, z0), f.P(s0, o1, z1), f.P(s0, o0, z1), -U);
    quad(m, f.P(s1, o0, z0), f.P(s1, o1, z0), f.P(s1, o1, z1), f.P(s1, o0, z1), U);
}

// A piece of wall between two cuts: each side has its own start / end (mitred ends), z0..z1.
static void wallPiece(MeshData& m, const Frame& f, float t, float sl0, float sr0, float sl1, float sr1, float z0, float z1) {
    const float3 Y(0, 1, 0), N = f.N();
    const float3 L0b = f.P(sl0, t, z0), L1b = f.P(sl1, t, z0), L0t = f.P(sl0, t, z1), L1t = f.P(sl1, t, z1);
    const float3 R0b = f.P(sr0, -t, z0), R1b = f.P(sr1, -t, z0), R0t = f.P(sr0, -t, z1), R1t = f.P(sr1, -t, z1);
    quad(m, L0b, L1b, L1t, L0t, N);
    quad(m, R0b, R1b, R1t, R0t, -N);
    quad(m, L0t, L1t, R1t, R0t, Y);
    if (z0 > 0.001f) quad(m, L0b, L1b, R1b, R0b, -Y);
    // end caps (slanted when mitred): normal away from the piece
    auto cap = [&](float3 a, float3 b, float3 c, float3 d, float3 out) {
        const float3 e = b - a;
        float3 n = normalize(cross(e, Y));
        if (dot(n, out) < 0) n = -n;
        quad(m, a, b, c, d, n);
    };
    cap(L0b, R0b, R0t, L0t, -f.U());
    cap(L1b, R1b, R1t, L1t, f.U());
}

static void floorPrism(MeshData& m, const std::vector<float2>& poly, float y0, float y1) {
    std::vector<int> tris;
    triangulate(poly, tris);
    const uint32_t base = (uint32_t)m.vertices.size();
    for (const float2& p : poly) m.vertices.push_back({world(p, y1), float3(0, 1, 0), float2(p.x, p.y)});
    for (size_t i = 0; i + 2 < tris.size(); i += 3) {   // plan CCW -> world (z = -y) turns it: wind for +Y
        const float3 a = m.vertices[base + tris[i]].pos, b = m.vertices[base + tris[i + 1]].pos, c = m.vertices[base + tris[i + 2]].pos;
        if (dot(cross(b - a, c - a), float3(0, 1, 0)) >= 0) m.addTri(base + tris[i], base + tris[i + 1], base + tris[i + 2]);
        else m.addTri(base + tris[i], base + tris[i + 2], base + tris[i + 1]);
    }
    if (y1 - y0 > 0.001f)
        for (size_t i = 0; i < poly.size(); ++i) {
            const float2 a = poly[i], b = poly[(i + 1) % poly.size()];
            const float2 n2 = -perp(norm2(b - a));   // CCW polygon: outside is to the right
            quad(m, world(a, y0), world(b, y0), world(b, y1), world(a, y1), world(n2, 0) - world(float2(0, 0), 0));
        }
}

void buildScene(const Building& B, Scene& scene, const BuildOptions& o, SceneMap& map) {
    // everything of the building is rebuilt; the assets (head of the meshes / materials, the textures) stay
    scene.objects.clear();
    scene.meshes.resize(std::min((size_t)map.keepMeshes, scene.meshes.size()));
    scene.meshNames.resize(scene.meshes.size());
    scene.materials.resize(std::min((size_t)map.keepMaterials, scene.materials.size()));
    scene.lods.clear();
    ++scene.geometryVersion;
    scene.unitsPerMeter = 1.0f;

    auto mat = [&](const char* name, int hex, float rough, float bevel = 0.0f) {
        Material m;
        m.name = name; m.baseColor = lin(hex); m.roughness = rough; m.bevelRadius = bevel;
        return scene.addMaterial(m);
    };
    Material g;
    g.name = "Ground"; g.baseColor = lin(0xdfe3e8); g.roughness = 0.9f; g.flags = MAT_FLAG_GRID;
    const int mGround = scene.addMaterial(g);
    const int mWall = mat("Plaster", 0xeeebe4, 0.85f);
    const int mWallSel = mat("Plaster selected", 0xb9cdf5, 0.85f);
    const int mFrame = mat("Alu anthracite", 0x3a4250, 0.35f, 0.004f);
    const int mFrameSel = mat("Frame selected", 0x2f6bff, 0.35f, 0.004f);
    const int mDoor = mat("Oak door", 0xb88c5e, 0.55f, 0.004f);
    const int mFloor[kFloorCount] = {mat("Oak floor", 0xc9a479, 0.45f), mat("Tiles", 0xd9d6cf, 0.3f), mat("Concrete", 0xb8b8b4, 0.8f)};
    const int mSlab = mat("Slab", 0xd2d2ce, 0.85f);
    Material gl;
    gl.name = "Glass"; gl.baseColor = float3(0.95f, 0.97f, 0.98f); gl.roughness = 0.0f; gl.transmission = 1.0f;
    gl.ior = 1.5f; gl.flags = MAT_FLAG_THIN;
    const int mGlass = scene.addMaterial(gl);

    Object go;
    go.name = "Ground"; go.part = "GROUND";
    go.mesh = scene.addMesh("ground", makePlane(400.0f));
    go.material = mGround;
    go.selectable = false;
    map.ground = scene.addObject(go);

    map.wallObject.assign(B.walls.size(), -1);
    map.frameObject.assign(B.walls.size(), -1);
    map.mWall = mWall; map.mWallSel = mWallSel; map.mFrame = mFrame; map.mFrameSel = mFrameSel;
    for (int w = 0; w < (int)B.walls.size(); ++w) {
        const Wall& W = B.walls[w];
        if (!W.alive) continue;
        const int lv = B.nodes[W.a].level;
        if (lv > o.activeLevel) continue;
        const WallGeom& G = B.geom[w];
        const Frame f{G.a, G.u, G.n, B.levels[lv].elevation};
        const float t = W.thickness * 0.5f, H = W.height;
        MeshData m;
        std::vector<Opening> ops = W.ops;
        std::sort(ops.begin(), ops.end(), [](const Opening& a, const Opening& b) { return a.pos < b.pos; });
        float cl = G.sL0, cr = G.sR0;   // current start of each side
        MeshData fr, glass, door;
        for (const Opening& op : ops) {
            const float s0 = op.pos - op.width * 0.5f, s1 = op.pos + op.width * 0.5f;
            const float z0 = op.sill, z1 = std::min(H, op.sill + op.height);
            if (s0 > std::max(cl, cr) + 0.001f) wallPiece(m, f, t, cl, cr, s0, s0, 0, H);
            if (z0 > 0.001f) wallPiece(m, f, t, s0, s0, s1, s1, 0, z0);
            if (z1 < H - 0.001f) wallPiece(m, f, t, s0, s0, s1, s1, z1, H);
            cl = cr = s1;
            // frame (6 cm profile, 7 cm deep, in the middle of the wall)
            const float p = 0.06f, dd = 0.035f;
            box(fr, f, s0, s0 + p, -dd, dd, z0, z1);
            box(fr, f, s1 - p, s1, -dd, dd, z0, z1);
            box(fr, f, s0 + p, s1 - p, -dd, dd, z1 - p, z1);
            if (op.kind == kWindow || op.kind == kSlider) {
                box(fr, f, s0 + p, s1 - p, -dd, dd, z0, z0 + p);
                const int panes = op.kind == kSlider ? 3 : (op.width > 1.3f ? 2 : 1);
                for (int k = 1; k < panes; ++k) {   // mullions
                    const float sm = s0 + (s1 - s0) * k / panes;
                    box(fr, f, sm - p * 0.5f, sm + p * 0.5f, -dd, dd, z0 + p, z1 - p);
                }
                // one thin pane (MAT_FLAG_THIN): a single surface, so a ray crosses the window in one bounce
                quad(glass, f.P(s0 + p, 0, z0 + p), f.P(s1 - p, 0, z0 + p), f.P(s1 - p, 0, z1 - p), f.P(s0 + p, 0, z1 - p), f.N());
            } else {
                box(door, f, s0 + p, s1 - p, -0.02f, 0.02f, 0.0f, z1 - p);
            }
        }
        if (std::min(G.sL1, G.sR1) > std::max(cl, cr) - 0.001f || ops.empty()) wallPiece(m, f, t, cl, cr, G.sL1, G.sR1, 0, H);
        Object wo;
        wo.name = "Wall " + std::to_string(w);
        wo.part = "WALL";
        wo.mesh = scene.addMesh(wo.name, std::move(m));
        wo.material = w == o.selectedWall ? mWallSel : mWall;
        map.wallObject[w] = scene.addObject(wo);
        const bool frameSel = w == o.selectedOpeningWall;
        if (!fr.vertices.empty()) { Object x; x.name = wo.name + " frames"; x.part = "FRAME"; x.mesh = scene.addMesh(x.name, std::move(fr)); x.material = frameSel ? mFrameSel : mFrame; map.frameObject[w] = scene.addObject(x); }
        if (!glass.vertices.empty()) { Object x; x.name = wo.name + " glass"; x.part = "GLASS"; x.mesh = scene.addMesh(x.name, std::move(glass)); x.material = mGlass; scene.addObject(x); }
        if (!door.vertices.empty()) { Object x; x.name = wo.name + " doors"; x.part = "DOOR"; x.mesh = scene.addMesh(x.name, std::move(door)); x.material = mDoor; scene.addObject(x); }
    }
    {   // joints of 3+ walls
        MeshData m;
        for (const Joint& J : B.joints)
            if (J.level <= o.activeLevel) floorPrism(m, J.poly, B.levels[J.level].elevation, B.levels[J.level].elevation + J.height);
        if (!m.vertices.empty()) {
            Object jo;
            jo.name = "Wall joints"; jo.part = "WALL";
            jo.mesh = scene.addMesh(jo.name, std::move(m));
            jo.material = mWall;
            scene.addObject(jo);
        }
    }
    for (int r = 0; r < (int)B.rooms.size(); ++r) {
        const Room& R = B.rooms[r];
        if (R.level > o.activeLevel) continue;
        const float e = B.levels[R.level].elevation;
        MeshData m;
        floorPrism(m, R.poly, R.level == 0 ? e : e - 0.30f, e + 0.02f);
        Object fo;
        fo.name = "Floor " + std::to_string(r);
        fo.part = "FLOOR";
        fo.mesh = scene.addMesh(fo.name, std::move(m));
        const int fm = R.label >= 0 ? std::clamp(B.labels[R.label].floor, 0, kFloorCount - 1) : kFloorConcrete;
        fo.material = mFloor[fm];
        scene.addObject(fo);
        if (o.ceilings && R.level == o.activeLevel) {   // closes the room: the light comes through the openings alone
            const float top = e + B.levels[R.level].height;
            MeshData c;
            floorPrism(c, R.poly, top, top + 0.25f);
            Object co;
            co.name = "Ceiling " + std::to_string(r);
            co.part = "CEILING";
            co.mesh = scene.addMesh(co.name, std::move(c));
            co.material = mSlab;
            scene.addObject(co);
        }
    }
    // the items: one object per part of their asset
    map.itemObjects.assign(B.items.size(), {0, 0});
    for (int i = 0; i < (int)B.items.size(); ++i) {
        const Item& it = B.items[i];
        if (it.level > o.activeLevel || !o.assets) continue;
        const AssetInfo* a = nullptr;
        for (const AssetInfo& x : *o.assets) if (x.slug == it.asset && x.firstMesh >= 0) a = &x;
        if (!a) continue;
        map.itemObjects[i] = {(int)scene.objects.size(), a->partCount};
        for (int k = 0; k < a->partCount; ++k) {
            Object ob;
            ob.name = a->name + " " + std::to_string(i);
            ob.part = "ITEM";
            ob.mesh = a->firstMesh + k;
            ob.material = a->firstMaterial + k;
            scene.addObject(ob);
        }
        placeItem(scene, map, B, i);
    }
}

void applySelection(Scene& scene, const SceneMap& map, const BuildOptions& o) {
    for (int w = 0; w < (int)map.wallObject.size(); ++w) {
        if (map.wallObject[w] >= 0) scene.objects[map.wallObject[w]].material = w == o.selectedWall ? map.mWallSel : map.mWall;
        if (w < (int)map.frameObject.size() && map.frameObject[w] >= 0) scene.objects[map.frameObject[w]].material = w == o.selectedOpeningWall ? map.mFrameSel : map.mFrame;
    }
}

void placeItem(Scene& scene, const SceneMap& map, const Building& B, int item) {
    if (item < 0 || item >= (int)map.itemObjects.size()) return;
    const Item& it = B.items[item];
    const float e = B.levels[std::clamp(it.level, 0, (int)B.levels.size() - 1)].elevation + 0.02f + it.elev;   // (on the floor finish)
    for (int k = 0; k < map.itemObjects[item].second; ++k) {
        Object& ob = scene.objects[map.itemObjects[item].first + k];
        ob.position = world(it.p, e);
        ob.yaw = it.yaw;
    }
}

} // namespace archviz
