// ArchViz - the 3D scene of a building (vp3d::Scene): walls cut by their openings, frames, glass and doors, floors and
// slabs, the ground. Portable: the same scene for the DirectX 12 HQ / ray traced view (PC) and the Lit view (iPad).
#pragma once
#include "ArchModel.h"
#include "vp3d/Scene.h"

namespace archviz {

inline float3 world(float2 p, float h) { return float3(p.x, h, -p.y); }   // plan (x, y) + height -> vp3d (Y up)

struct SceneMap {
    std::vector<int> wallObject;   // per wall: its object (-1 = not in the scene)
    int ground = -1;
};

struct BuildOptions {
    int activeLevel = 0;     // the levels above are left out (the view looks into the active level)
    int selectedWall = -1;   // tinted
    int selectedOpeningWall = -1, selectedOpening = -1;
    int selectedRoom = -1;
};

void buildScene(const Building& B, vp3d::Scene& scene, const BuildOptions& o, SceneMap& map);

} // namespace archviz
