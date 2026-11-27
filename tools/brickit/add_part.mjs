// A part added to a built library (full or full_hd) without rebuilding it: converted from its LDraw file and appended
// to the pack of a part it is modelled on (same shape: its connectors, voxels and index entry are copied; only the
// name, the label and the geometry are its own). For a printed variant missing from LDraw (tools/brickit/custom_parts:
// copy the .dat into <ldraw>/parts first).
// Usage: node tools/brickit/add_part.mjs <ldraw dir> <shadow dir> <lib dir> <new id> <id of the part it is like>
//        HIRES=1 BEVEL=1 for the HD library (as build_full.mjs). The old pack and index are kept as .prev.
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const [, , ldraw, shadow, lib, id, like] = process.argv;
if (!like) { console.log("usage: add_part.mjs <ldraw> <shadow> <lib dir> <new id> <like id>"); process.exit(1); }
process.env.LDRAW = ldraw;
process.env.SHADOW = shadow;
const { convertPart } = await import(pathToFileURL(path.join(HERE, "..", "..", "docs", "brickit", "web", "ldraw2brickit.mjs")).href);
console.warn = () => {};

const index = JSON.parse(fs.readFileSync(path.join(lib, "index.json"), "utf8"));
if (index.parts.find((p) => p.name === id)) { console.log(id, ": already in this library (repack_parts.mjs to convert it again)"); process.exit(0); }
const model = index.parts.find((p) => p.name === like.toLowerCase());
if (!model) { console.log(like, ": not in this library"); process.exit(1); }
const pad = (b) => { const k = b.length % 4; return k ? Buffer.concat([b, Buffer.alloc(4 - k)]) : b; };

const r = convertPart(id, { system: model.info.category !== "Technic" });
const vc = r.mesh.verts.length / 6, ic = r.mesh.idx.length, f = r.mesh.verts;
const mn = [1e9, 1e9, 1e9], mx = [-1e9, -1e9, -1e9];
for (let i = 0; i < vc; i++) for (let k = 0; k < 3; k++) { mn[k] = Math.min(mn[k], f[i * 6 + k]); mx[k] = Math.max(mx[k], f[i * 6 + k]); }
const sc = mx.map((v, k) => Math.max(v - mn[k], 1e-6) / 65535);
const pos = new Uint16Array(vc * 3), nrm = new Int8Array(vc * 4);
for (let i = 0; i < vc; i++) for (let k = 0; k < 3; k++) {
  pos[i * 3 + k] = Math.round((f[i * 6 + k] - mn[k]) / sc[k]);
  nrm[i * 4 + k] = Math.round(Math.max(-1, Math.min(1, f[i * 6 + 3 + k])) * 127);
}
const big = vc > 65535, ind = big ? new Uint32Array(r.mesh.idx) : new Uint16Array(r.mesh.idx);
const printed = !!r.mesh.vcol;
const bCol = printed ? Buffer.from(new Uint32Array(r.mesh.vcol.map((c) => (c < 0 ? 0xffffffff : c))).buffer) : Buffer.alloc(0);
const buf = Buffer.concat([pad(Buffer.from(pos.buffer)), Buffer.from(nrm.buffer), pad(Buffer.from(ind.buffer)), bCol]);

const title = (fs.readFileSync(path.join(ldraw, "parts", id + ".dat"), "utf8").split(/\r?\n/)[0] || "").replace(/^0\s+/, "");
const label = title.replace(/\s+/g, " ").replace(/^[~_]/, "").trim();
const file = path.join(lib, `f${model.pack}.bin`);
const z = fs.readFileSync(file);
const raw = zlib.inflateSync(z);
const jl = raw.readUInt32LE(0);
const metas = JSON.parse(raw.subarray(4, 4 + jl).toString("utf8"));
const data = raw.subarray(4 + jl);
const mm = metas.find((m) => m.name === model.name);
if (!mm) { console.log(model.name, ": not in its pack"); process.exit(1); }
metas.push({ ...mm, name: id, label, q: { off: data.length, vc, ic, big, mn, sc, col: printed } });
const json = Buffer.from(JSON.stringify(metas));
const head = Buffer.alloc(4);
head.writeUInt32LE(json.length);
const newRaw = Buffer.concat([head, json, data, buf]);
const newZ = zlib.deflateSync(newRaw, { level: 9 });
fs.copyFileSync(file, file + ".prev");
fs.writeFileSync(file, newZ);
const pk = index.packs.find((p) => p.no === model.pack);
if (pk) { pk.raw = newRaw.length; pk.z = newZ.length; pk.n = metas.length; }
index.parts.push({ ...model, name: id, label, info: { ...model.info, ldraw: id, title, triangles: ic / 3, vertices: vc, keywords: "BrickIt custom part" } });
fs.copyFileSync(path.join(lib, "index.json"), path.join(lib, "index.json.prev"));
fs.writeFileSync(path.join(lib, "index.json"), JSON.stringify(index));
console.log(`${id} added to pack ${model.pack} (${ic / 3} triangles, ${vc} vertices, printed ${printed}), like ${model.name}`);
