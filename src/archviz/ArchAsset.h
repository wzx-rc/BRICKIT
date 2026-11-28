// ArchViz - the library of objects (furniture, doors, plants...): assets converted from the DCC files by
// tools/archviz/export_asset.py (Blender) into the "archviz-asset" format, one folder per asset:
//   asset.json   name, size (metres), parts (one per material: vertex / index counts, material, texture file)
//   mesh.bin     per part: vertices (8 float32: position, normal, uv) then uint32 indices
//   *.jpg        base colour textures
// Metres, Y up, the origin under the centre of the object (it stands on the floor).
#pragma once
#include "vp3d/Scene.h"
#include <string>
#include <vector>

namespace archviz {

struct AssetInfo {            // an entry of the library (asset.json read, the geometry not loaded yet)
    std::string slug, name, category;   // category: the tab of the library ("" = Other)
    std::wstring dir;
    vp3d::float3 size{1, 1, 1};
    int tris = 0, parts = 0, textures = 0;
    // loaded into the scene: its meshes and materials there (-1 = not loaded)
    int firstMesh = -1, firstMaterial = -1, partCount = 0;
};

// The assets of a folder (its sub-folders with an asset.json), sorted by name.
void scanAssets(const std::wstring& dir, std::vector<AssetInfo>& out);

// Reads the geometry, the materials and the textures of an asset and appends them to the scene (meshes, materials,
// textures; no object). Textures beyond the limit of the renderer (MAX_TEXTURES) are left out: the material keeps its
// colour. Fills firstMesh / firstMaterial / partCount.
bool loadAsset(AssetInfo& a, vp3d::Scene& scene, std::string* error = nullptr);

// The thumbnail of an asset (thumb.png beside its asset.json, transparent background): straight RGBA.
bool loadThumb(const AssetInfo& a, std::vector<uint8_t>& rgba, int& w, int& h);

} // namespace archviz
