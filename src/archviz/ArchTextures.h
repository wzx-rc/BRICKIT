// ArchViz - the textures of the building finishes, made by the app (no image file, nothing downloaded): oak parquet,
// tiles with their joints, concrete, carpet, brick, wood cladding, grass, gravel, paving. Tileable, generated once per
// scene (found again by name), coloured by the material where it makes sense (tiles, carpet, paint is a plain colour).
#pragma once
#include "vp3d/Scene.h"

namespace archviz {

enum BuildTex { kTexParquet = 0, kTexTiles, kTexConcrete, kTexCarpet, kTexBrick, kTexCladding, kTexGrass, kTexGravel, kTexPaving, kBuildTexCount };

float buildTexMetres(int t);                  // the size (metres) one repeat of the texture covers
int buildTexture(vp3d::Scene& scene, int t);  // its index in scene.textures (made the first time); -1 when the scene is full

} // namespace archviz
