// A few parts converted again and put back into the packs of a built library (full or full_hd), without rebuilding the
// whole library: the geometry of each named part is replaced in its pack (the other parts of the pack are copied byte
// for byte, their metas kept), index.json gets its new triangle / vertex counts. The metas of the replaced parts are
// kept too (the connector fixes made on the packs stay): only the quantized geometry (q) changes.
// Usage: node tools/brickit/repack_parts.mjs <ldraw dir> <shadow dir> <lib dir> id [id...]
//        HIRES=1 BEVEL=1 for the HD library (as build_full.mjs). The old pack and index are kept as .prev.
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const [, , ldraw, shadow, lib, ...ids] = process.argv;
if (!lib || !ids.length) { console.log("usage: repack_parts.mjs <ldraw> <shadow> <lib dir> id..."); process.exit(1); }
process.env.LDRAW = ldraw;
process.env.SHADOW = shadow;
const { convertPart } = await import(pathToFileURL(path.join(HERE, "..", "..", "docs", "brickit", "web", "ldraw2brickit.mjs")).href);
console.warn = () => {};

const index = JSON.parse(fs.readFileSync(path.join(lib, "index.json"), "utf8"));
const byName = new Map(index.parts.map((p) => [p.name, p]));
const pad = (b) => { const k = b.length % 4; return k ? Buffer.concat([b, Buffer.alloc(4 - k)]) : b; };
const partLen = (q) => ((q.vc * 6 + 3) >> 2 << 2) + q.vc * 4 + ((q.ic * (q.big ? 4 : 2) + 3) >> 2 << 2) + (q.col ? q.vc * 4 : 0);

// the quantized buffers of a converted part (as build_full.mjs)
function quantize(r) {
  const vc = r.mesh.verts.length / 6, ic = r.mesh.idx.length, f = r.mesh.verts, idx = r.mesh.idx;
  const mn = [1e9, 1e9, 1e9], mx = [-1e9, -1e9, -1e9];
  for (let i = 0; i < vc; i++) for (let k = 0; k < 3; k++) { mn[k] = Math.min(mn[k], f[i * 6 + k]); mx[k] = Math.max(mx[k], f[i * 6 + k]); }
  const sc = mx.map((v, k) => Math.max(v - mn[k], 1e-6) / 65535);
  const pos = new Uint16Array(vc * 3), nrm = new Int8Array(vc * 4);
  for (let i = 0; i < vc; i++) for (let k = 0; k < 3; k++) {
    pos[i * 3 + k] = Math.round((f[i * 6 + k] - mn[k]) / sc[k]);
    nrm[i * 4 + k] = Math.round(Math.max(-1, Math.min(1, f[i * 6 + 3 + k])) * 127);
  }
  const big = vc > 65535, ind = big ? new Uint32Array(idx) : new Uint16Array(idx);
  const printed = !!r.mesh.vcol;
  const bCol = printed ? Buffer.from(new Uint32Array(r.mesh.vcol.map((c) => (c < 0 ? 0xffffffff : c))).buffer) : Buffer.alloc(0);
  const buf = Buffer.concat([pad(Buffer.from(pos.buffer)), Buffer.from(nrm.buffer), pad(Buffer.from(ind.buffer)), bCol]);
  return { buf, q: { vc, ic, big, mn, sc, col: printed }, bb: [mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]] };
}

// --all: every part of the library (its geometry converted again, its connectors kept)
if (ids[0] === "--all") ids.splice(0, ids.length, ...index.parts.map((p) => p.name));
const byPack = new Map();
let done = 0;
for (const id of ids) {
  const e = byName.get(id.toLowerCase());
  if (!e) { console.log(id, ": not in this library"); continue; }
  if (!byPack.has(e.pack)) byPack.set(e.pack, []);
  byPack.get(e.pack).push(e);
}
for (const [no, entries] of byPack) {
  const file = path.join(lib, `f${no}.bin`);
  const z = fs.readFileSync(file);
  const raw = zlib.inflateSync(z);
  const jl = raw.readUInt32LE(0);
  const metas = JSON.parse(raw.subarray(4, 4 + jl).toString("utf8"));
  const data = raw.subarray(4 + jl);
  const bufs = [];
  let off = 0;
  for (const m of metas) {
    const e = entries.find((x) => x.name === m.name);
    let buf;
    let r = null;
    if (e) { try { r = convertPart(e.name, { system: e.info.category !== "Technic" }); } catch (err) { console.log(e.name, ": conversion failed, kept as it was:", err.message); } }
    if (e && r && r.mesh.idx.length) {
      const qz = quantize(r);
      buf = qz.buf;
      const before = m.q;
      m.q = { off: 0, ...qz.q };
      e.info.triangles = qz.q.ic / 3;
      e.info.vertices = qz.q.vc;
      console.log(`${e.name} in pack ${no}: ${before.ic / 3} -> ${qz.q.ic / 3} triangles, ${before.vc} -> ${qz.q.vc} vertices`);
      if (++done % 1000 === 0) console.error(done, "parts");
    } else buf = Buffer.from(data.subarray(m.q.off, m.q.off + partLen(m.q)));
    m.q.off = off;
    bufs.push(buf);
    off += buf.length;
  }
  const json = Buffer.from(JSON.stringify(metas));
  const head = Buffer.alloc(4);
  head.writeUInt32LE(json.length);
  const newRaw = Buffer.concat([head, json, ...bufs]);
  const newZ = zlib.deflateSync(newRaw, { level: 9 });
  fs.copyFileSync(file, file + ".prev");
  fs.writeFileSync(file, newZ);
  const pk = index.packs.find((p) => p.no === no);
  if (pk) { pk.raw = newRaw.length; pk.z = newZ.length; }
  console.log(`pack ${no}: ${z.length} -> ${newZ.length} bytes`);
}
fs.copyFileSync(path.join(lib, "index.json"), path.join(lib, "index.json.prev"));
fs.writeFileSync(path.join(lib, "index.json"), JSON.stringify(index));
console.log("index.json updated");
