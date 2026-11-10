// The library of the old BrickIt (Maya, BrickIt 2.0): one .ma per part with « VoxelNNN » locators (positions in cm, origin
// at the min corner of the part, as BrickIt). Indexed by a key made from the name (lower case, letters and digits, no
// leading zeros: « Brick_01x1x03 » = « Brick  1 x  1 x  3 » of LDraw), for the converter (build_full.mjs): the voxels of
// a part the LDCad shadow library does not know.
// Usage: node tools/brickit/old_library_index.mjs "<BRICKIT_LIBRARY dir>" <out.json>
import fs from "node:fs";
import path from "node:path";

export const nameKey = (s) => s.toLowerCase().replace(/[^a-z0-9]+/g, " ").replace(/\b0+(\d)/g, "$1").replace(/ /g, "");

const [, , root, out] = process.argv;
if (root && out) {
  const index = {};
  let files = 0, withVox = 0;
  const walk = (d) => {
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) walk(p);
      else if (e.name.endsWith(".ma")) {
        files++;
        const txt = fs.readFileSync(p, "latin1");
        const vox = [];
        const re = /createNode transform -n "Voxel\d+"[^\n]*\n(?:[^\n]*\n){0,4}?\s*setAttr "\.rp" -type "double3" ([-\d.e]+) ([-\d.e]+) ([-\d.e]+)/g;
        let m;
        while ((m = re.exec(txt))) vox.push([+(+m[1]).toFixed(3), +(+m[2]).toFixed(3), +(+m[3]).toFixed(3)]);
        const mx = txt.match(/setAttr "\.max" -type "float3" ([-\d.e]+) ([-\d.e]+) ([-\d.e]+)/);
        const name = e.name.replace(/\.ma$/, "");
        if (vox.length) withVox++;
        index[nameKey(name)] = { name, rel: path.relative(root, p), vox, max: mx ? [+mx[1], +mx[2], +mx[3]] : null };
      }
    }
  };
  walk(root);
  fs.writeFileSync(out, JSON.stringify(index));
  console.log("files", files, "with voxels", withVox, "keys", Object.keys(index).length);
}
