// Diagnostic (Wzx 14/11/2026): the parts of the old BrickIt (Maya library) matched to the LDraw parts by their shape,
// whatever their names, and their « VoxelNNN » locators compared with the connectors of the converter.
// The locators of the .ma: position = rotate pivot (.rp) + translate (.t) (old_library_index.mjs read .rp alone).
// The mesh of the .ma (<name>_BrickGeoShape, .vt) and the LDraw mesh (converter) are both in cm, origin at the min corner,
// Y up: scored by their box, then by a two-way nearest-point distance (chamfer) of the vertices.
// Usage: node tools/brickit/old_match.mjs <ldraw dir> <shadow dir> <old category dir> <LDraw title regexp> <out.md>
import fs from "node:fs";
import path from "node:path";
import { pathToFileURL, fileURLToPath } from "node:url";

const [, , ldraw, shadow, oldDir, titleRe, out] = process.argv;
process.env.LDRAW = ldraw;
process.env.SHADOW = shadow;
const HERE = path.dirname(fileURLToPath(import.meta.url));
const { convertPart } = await import(pathToFileURL(path.join(HERE, "..", "..", "docs", "brickit", "web", "ldraw2brickit.mjs")).href);
console.warn = () => {};

// ---- the old parts
function readMa(file) {
  const t = fs.readFileSync(file, "latin1");
  const nodes = t.split(/\ncreateNode /);
  const vox = [];
  let verts = [];
  let geoT = [0, 0, 0];
  const num = (s) => s.trim().split(/\s+/).map(Number);
  for (const n of nodes) {
    const head = n.slice(0, n.indexOf("\n"));
    const attr = (a) => { const m = n.match(new RegExp('setAttr "\\.' + a + '" -type "double3" ([-\\d.e]+) ([-\\d.e]+) ([-\\d.e]+)')); return m ? [+m[1], +m[2], +m[3]] : null; };
    if (/^transform -n "Voxel\d+"/.test(head)) {
      const rp = attr("rp") || [0, 0, 0], tr = attr("t") || [0, 0, 0];
      vox.push([rp[0] + tr[0], rp[1] + tr[1], rp[2] + tr[2]].map((v) => +v.toFixed(3)));
    } else if (/^transform -n "\w+_BrickGeo"/.test(head)) geoT = attr("t") || [0, 0, 0];
    else if (/^mesh -n "\w+_BrickGeoShape"/.test(head)) {
      const re = /setAttr[^\n]*"\.vt\[\d+:\d+\]"([^;]*);/g;
      let m;
      while ((m = re.exec(n))) verts = verts.concat(num(m[1].replace(/-type "float3"/, "")));
    }
  }
  const v = [];
  for (let i = 0; i + 2 < verts.length; i += 3) v.push([verts[i] + geoT[0], verts[i + 1] + geoT[1], verts[i + 2] + geoT[2]]);
  return { vox, verts: v };
}
const olds = [];
for (const d of fs.readdirSync(oldDir, { withFileTypes: true })) {
  if (!d.isDirectory()) continue;
  const f = path.join(oldDir, d.name, d.name + ".ma");
  if (!fs.existsSync(f)) continue;
  const r = readMa(f);
  if (!r.verts.length) continue;
  olds.push({ name: d.name, ...r });
}
// ---- the LDraw candidates
const cands = [];
for (const f of fs.readdirSync(path.join(ldraw, "parts"))) {
  if (!f.endsWith(".dat")) continue;
  const first = fs.readFileSync(path.join(ldraw, "parts", f), "latin1").split(/\r?\n/, 1)[0].replace(/^0\s+/, "");
  if (!new RegExp(titleRe).test(first) || /^[~=_]/.test(first)) continue;
  const id = f.slice(0, -4);
  try {
    const r = convertPart(id, { system: true });
    const vv = [];
    const mn = r.mesh.mn;
    for (let i = 0; i < r.mesh.verts.length; i += 6) vv.push([r.mesh.verts[i] - mn[0], r.mesh.verts[i + 1] - mn[1], r.mesh.verts[i + 2] - mn[2]]);
    cands.push({ id, title: first, verts: vv, vox: r.meta.voxels, types: r.meta.connectorTypes });
  } catch (e) { /* (a part the converter cannot read) */ }
}
const box = (v) => { const mn = [1e9, 1e9, 1e9], mx = [-1e9, -1e9, -1e9]; for (const p of v) for (let k = 0; k < 3; k++) { mn[k] = Math.min(mn[k], p[k]); mx[k] = Math.max(mx[k], p[k]); } return { mn, mx, size: mx.map((x, k) => x - mn[k]) }; };
const sample = (v, n) => { if (v.length <= n) return v; const o = []; for (let i = 0; i < n; i++) o.push(v[Math.floor((i * v.length) / n)]); return o; };
const near = (p, set) => { let b = 1e9; for (const q of set) { const d = (p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 + (p[2] - q[2]) ** 2; if (d < b) b = d; } return Math.sqrt(b); };
const chamfer = (a, b) => { const A = sample(a, 400), B = sample(b, 400); let s = 0; for (const p of A) s += near(p, B); for (const q of B) s += near(q, A); return s / (A.length + B.length); };
for (const c of cands) { c.box = box(c.verts); }
const lines = [`# Équerres : l'ancienne bibliothèque Maya face à LDraw\n`, `${olds.length} pièces Maya, ${cands.length} pièces LDraw candidates (titre ${titleRe}).\n`];
const rows = [];
for (const o of olds) {
  // the old part may be turned on the floor relative to LDraw: the 4 quarter turns about Y are tried
  const rotY = (pts, r) => pts.map(([x, y, z]) => (r === 0 ? [x, y, z] : r === 1 ? [z, y, -x] : r === 2 ? [-x, y, -z] : [-z, y, x]));
  let scored = [], ob = box(o.verts), ovox = [];
  for (let r = 0; r < 4; r++) {
    const rv = rotY(o.verts, r), rb = box(rv);
    const ov = rv.map((p) => [p[0] - rb.mn[0], p[1] - rb.mn[1], p[2] - rb.mn[2]]);
    const vx = rotY(o.vox, r).map((p) => [p[0] - rb.mn[0], p[1] - rb.mn[1], p[2] - rb.mn[2]].map((x) => +x.toFixed(2)));
    for (const c of cands) {
      const db = Math.max(...c.box.size.map((x, k) => Math.abs(x - rb.size[k])));
      if (db < 0.12) scored.push({ c, db, r, vx, size: rb.size, ch: chamfer(ov, c.verts) });
    }
  }
  scored.sort((a, b) => a.ch - b.ch);
  if (scored.length) { ovox = scored[0].vx; ob = { size: scored[0].size }; }
  { const seen = new Set(); scored = scored.filter((x) => (seen.has(x.c.id) ? false : (seen.add(x.c.id), true))); }
  const best = scored[0];
  lines.push(`## ${o.name}`);
  lines.push(`- boîte Maya : ${ob.size.map((x) => x.toFixed(2)).join(" × ")} cm, ${o.vox.length} locators : ${ovox.map((p) => "(" + p.join(", ") + ")").join(" ")}`);
  if (!best) { lines.push(`- **aucune pièce LDraw de la même taille**\n`); continue; }
  for (const s of scored.slice(0, 3)) lines.push(`- ${s === best ? "**" : ""}${s.c.id}${s === best ? "**" : ""} · ${s.c.title} · écart de forme ${s.ch.toFixed(3)} cm${s.r ? ` (pièce Maya tournée de ${s.r * 90}°)` : ""}`);
  // the locators against the connectors of the best match
  const used = new Set();
  const cmp = ovox.map((p) => {
    let bi = -1, bd = 1e9;
    best.c.vox.forEach((q, i) => { const d = Math.hypot(p[0] - q[0], p[1] - q[1], p[2] - q[2]); if (d < bd) { bd = d; bi = i; } });
    if (bi >= 0 && bd < 0.06) used.add(bi);
    return bi >= 0 && bd < 0.06 ? `(${p.join(", ")}) = ${best.c.types[bi]}` : `(${p.join(", ")}) **sans équivalent**${bi >= 0 ? ` (le plus proche : ${best.c.types[bi]} à ${bd.toFixed(2)} cm)` : ""}`;
  });
  const extra = best.c.vox.map((q, i) => (used.has(i) ? null : `(${q.join(", ")}) ${best.c.types[i]}`)).filter(Boolean);
  lines.push(`- locators Maya / connecteurs de ${best.c.id} : ${cmp.join(" ; ")}`);
  if (extra.length) lines.push(`- connecteurs de ${best.c.id} sans locator Maya : ${extra.join(" ; ")}`);
  lines.push("");
}
fs.writeFileSync(out, lines.join("\n"));
console.log("written", out, olds.length, cands.length);
