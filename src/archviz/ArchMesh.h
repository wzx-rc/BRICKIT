// ArchViz - the 3D scene of a building (vp3d::Scene): walls cut by their openings, frames, glass and doors, floors and
// slabs, the ground. Portable: the same scene for the DirectX 12 HQ / ray traced view (PC) and the Lit view (iPad).
#pragma once
#include "ArchAsset.h"
#include "ArchModel.h"
#include "vp3d/Scene.h"

namespace archviz {

inline float3 world(float2 p, float h) { return float3(p.x, h, -p.y); }   // plan (x, y) + height -> vp3d (Y up)

struct SceneMap {
    std::vector<int> wallObject;    // per wall: its object (-1 = not in the scene)
    std::vector<int> frameObject;   // per wall: the frames of its openings (-1 = none; unused since the joinery)
    // The objects of the joinery (ArchJoinery): per piece and material, its opening, the frame of the opening in the
    // world (origin at the left jamb on the sill, rotation of its local axes) and the motion of the piece.
    // A leaf / sash / panel of an opening: its motion, its handle and its box (local space of the opening, closed).
    struct JoineryLeaf {
        int wall = -1, op = -1, leaf = 0, handleKind = 0;
        vp3d::float3 origin{0, 0, 0};
        vp3d::float4 rot{0, 0, 0, 1};
        vp3d::float3 pivot{0, 0, 0}, axis{0, 1, 0}, slide{0, 0, 0};
        float angle = 0;
        bool hasTilt = false;
        vp3d::float3 pivot2{0, 0, 0}, axis2{1, 0, 0}, anchor{0, 0, 0}, anchor2{0, 0, 0};
        float angle2 = 0;
        vp3d::float3 bmin{0, 0, 0}, bmax{0, 0, 0};
    };
    struct JoineryObj {
        int wall = -1, op = -1, obj = -1, mat = 0;   // mat: JoineryMat (the frame takes the selection tint)
        int baseMaterial = 0;
        int leaf = -1;                               // index in leaves (-1: the fixed frame of the opening)
        vp3d::float3 origin{0, 0, 0};
        vp3d::float4 rot{0, 0, 0, 1};
    };
    std::vector<JoineryObj> joinery;
    std::vector<JoineryLeaf> leaves;
    std::vector<int> stairObject;   // per stair: the object of its steps (-1 = not in the scene)
    int mStairSel = -1;
    std::vector<int> stairMaterial;
    std::vector<std::pair<int, int>> itemObjects;   // per item: its first object and their count (0 = asset missing)
    struct WallFaceObj { int obj = -1, wall = -1, material = 0; };   // the finished faces of the walls (tinted too)
    std::vector<WallFaceObj> wallFaces;
    std::vector<int> fixtureObject;   // per fixture: the object of its body (-1 = not in the scene)
    std::vector<int> fixtureMaterial;
    int mFixtureSel = -1;
    int ground = -1;
    int mWall = -1, mWallSel = -1, mFrame = -1, mFrameSel = -1;
    // The head of the scene a rebuild keeps: the meshes and the materials of the assets loaded (and every texture),
    // so that an edit of the walls does not read the furniture again.
    int keepMeshes = 0, keepMaterials = 0;
};

struct BuildOptions {
    int activeLevel = 0;     // the levels above are left out (the view looks into the active level)
    int selectedWall = -1;   // tinted
    int selectedOpeningWall = -1, selectedOpening = -1;
    int selectedRoom = -1;
    int selectedStair = -1;
    int selectedFixture = -1;
    bool ceilings = false;   // a slab over the rooms of the active level (interior renders)
    const std::vector<AssetInfo>* assets = nullptr;   // the library (the items of the assets loaded are drawn)
};

void buildScene(const Building& B, vp3d::Scene& scene, const BuildOptions& o, SceneMap& map);
// The selection tint alone (materials of the wall objects): no geometry rebuilt.
void applySelection(vp3d::Scene& scene, const SceneMap& map, const BuildOptions& o);
// The position / rotation of the objects of an item after it moved (no geometry rebuilt).
void placeItem(vp3d::Scene& scene, const SceneMap& map, const Building& B, int item);
// The leaves of the openings at their Opening::open (no geometry rebuilt: the objects move). wall / op < 0: all.
void placeJoinery(vp3d::Scene& scene, const SceneMap& map, const Building& B, int wall = -1, int op = -1);
// A leaf at open = t (Opening::leafOpen): the position and rotation of its objects; world point of a local point p
// (space of the opening, closed) = pos + qRot(rot, p - L.pivot).
void leafTransform(const SceneMap::JoineryLeaf& L, float t, vp3d::float3& pos, vp3d::float4& rot);
vp3d::float3 leafPoint(const SceneMap::JoineryLeaf& L, float t, const vp3d::float3& p);

} // namespace archviz
