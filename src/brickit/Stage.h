// BrickIt - the stage of the viewport shared by the PC and iPad builds: the studio floor, the cool rim light of the
// web builder and one material per palette colour (ABS plastic, tinted glass, metal). BrickApp::setupStage (PC) and
// the iPad / Lit builds call the same functions, so both show the same materials.
#pragma once
#include "Library.h"
#include "vp3d/Scene.h"
#include <vector>

namespace brickit {

// The floor: a plane of size x size cm, light grey, with the stud grid when grid is set. Returns the object index.
int addGround(vp3d::Scene& scene, float size, bool grid, bool visible);
// The cool rim light of the web (raster only). Returns its index in scene.lights.
int addRimLight(vp3d::Scene& scene);
// One material per colour of lib.palette, in palette order. sss: discreet subsurface on the light colours.
std::vector<int> addPaletteMaterials(vp3d::Scene& scene, const Library& lib, bool sss);

} // namespace brickit
