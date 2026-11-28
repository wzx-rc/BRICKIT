// ArchViz - the roof over the top level (Level::roof...): flat (a slab over each room and a parapet on the outer walls),
// gable (two pans, the gable walls filled up to the roof), hip (four pans), shed (one pan, the high wall and the side
// gables filled). The pitched roofs cover the rectangle around the walls of the level, turned along its longest wall
// (an L-shaped house gets one roof over its whole rectangle). Portable C++, no renderer.
#pragma once
#include "ArchModel.h"
#include "vp3d/MeshGen.h"
#include <vector>

namespace archviz {

const char* roofTypeName(int t);
const char* roofFinishName(int f);
int roofTopLevel(const Building& B);   // the highest level with walls (-1 = none)

struct RoofGeom {
    vp3d::MeshData roof;                 // the pans / slabs (roof finish)
    vp3d::MeshData walls;                // gables, high wall, parapets (plaster)
    std::vector<std::pair<float2, float2>> ridges;   // plan: ridge, hips (solid)
    std::vector<float2> eaves;           // plan: the edge of the overhang (dashed), closed
    bool rectangular = true;             // false: the level is not a rectangle (a pitched roof covers its rectangle)
};

// The roof of that level (its walls, its rooms); false when it has none.
bool buildRoof(const Building& B, int level, RoofGeom& out);

} // namespace archviz
