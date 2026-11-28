// ArchViz - stairs (Stair): straight, L (quarter landing), U (half landing), spiral. The rise from the floor of its level
// to the floor of the next one, the risers by Blondel (2h + g = 63 cm), closed (solid steps) or open (treads on two
// stringers), a handrail on the free side. The plan: the treads, the walk line with its arrow, the cut line. Its
// footprint cuts the slab of the level above (ArchMesh). Portable C++, no renderer.
#pragma once
#include "ArchModel.h"
#include "vp3d/MeshGen.h"
#include <vector>

namespace archviz {

const char* stairTypeName(int t);
const char* stairFinishName(int f);

struct StairInfo {
    float rise = 0;      // floor to floor
    int risers = 0;
    float riserH = 0;    // h
    float tread = 0;     // g
    float blondel = 0;   // 2h + g
};
StairInfo stairInfo(const Building& B, const Stair& s);

struct StairGeom {
    vp3d::MeshData steps;   // the steps, landings, stringers (the finish)
    vp3d::MeshData rail;    // handrail and its posts
    std::vector<std::pair<float2, float2>> lines;   // plan: the outline and the nosing of each tread
    std::vector<float2> walk;                       // plan: the walk line, foot to top (arrow at the end)
    std::pair<float2, float2> cut;                  // plan: the cut line across the flight
    std::vector<float2> footprint;                  // plan: the rectangle around it (picking, the slab opening above)
};
void buildStair(const Building& B, const Stair& s, StairGeom& out);

} // namespace archviz
