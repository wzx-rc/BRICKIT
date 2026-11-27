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
    std::vector<int> frameObject;   // per wall: the frames of its openings (-1 = none)
    std::vector<std::pair<int, int>> itemObjects;   // per item: its first object and their count (0 = asset missing)
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
    const std::vector<AssetInfo>* assets = nullptr;   // the library (the items of the assets loaded are drawn)
};

void buildScene(const Building& B, vp3d::Scene& scene, const BuildOptions& o, SceneMap& map);
// The selection tint alone (materials of the wall objects): no geometry rebuilt.
void applySelection(vp3d::Scene& scene, const SceneMap& map, const BuildOptions& o);
// The position / rotation of the objects of an item after it moved (no geometry rebuilt).
void placeItem(vp3d::Scene& scene, const SceneMap& map, const Building& B, int item);

} // namespace archviz
