// Recomputes the connectors (voxels, their types, the grab point) of every part of the libraries with the current
// converter (docs/brickit/web/ldraw2brickit.mjs), without rebuilding the meshes: the metas of the packs and the counts
// of the indexes are rewritten in place (a copy of each file kept as <file>.prev). Wzx 13/11/2026: the holes of a beam
// lying flat were deep anti-studs, the slides of the crane arms pins.
// Usage: node tools/brickit/fix_connectors.mjs <ldraw dir> <shadow dir> <report.txt> <technic dir> <full dir>... [--base base.json] [--dry]
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { fork } from "node:child_process";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const CONVERTER = path.join(HERE, "..", "..", "docs", "brickit", "web", "ldraw2brickit.mjs");

if (process.argv[2] === "--worker") {
  process.env.LDRAW = process.argv[3];
  process.env.SHADOW = process.argv[4];
  const { convertPart } = await import(pathToFileURL(CONVERTER).href);
  console.warn = () => {};
  process.on("message", (ids) => {
    const res = [];
    for (const [id, sys] of ids) {
      try {
        const r = convertPart(id, { system: sys });
        res.push({ id, sys, src: r.meta.connSource, voxels: r.meta.voxels, types: r.meta.connectorTypes, grab: r.meta.grab, grabPoints: r.meta.grabPoints, size: [0, 1, 2].map((k) => r.mesh.mx[k] - r.mesh.mn[k]) });
      } catch (e) { res.push({ id, sys, err: String(e) }); }
    }
    process.send(res);
  });
} else {
  const args = process.argv.slice(2);
  const dry = args.includes("--dry");
  const bi = args.indexOf("--base");
  const base = bi >= 0 ? args[bi + 1] : null;
  const pos = args.filter((a, i) => !a.startsWith("--") && !(bi >= 0 && i === bi + 1));
  const [ldraw, shadow, report, ...dirs] = pos;
  // the parts: name -> system (a System part: not Technic)
  const want = new Map();
  const isSys = (cat) => cat !== "Technic";
  const libs = [];   // {dir, kind: "technic" | "full"}
  for (const d of dirs) {
    const ix = JSON.parse(fs.readFileSync(path.join(d, "index.json"), "utf8"));
    const technic = ix.set !== "ldraw-full";
    libs.push({ dir: d, technic, ix });
    for (const p of ix.parts) if (!want.has(p.name)) want.set(p.name, technic ? false : isSys((p.info && p.info.category) || ""));
  }
  let baseJ = null;
  if (base) {
    baseJ = JSON.parse(fs.readFileSync(base, "utf8"));
    for (const p of baseJ.parts) if (!want.has(p.name)) want.set(p.name, isSys((p.info && p.info.category) || p.label.split(/\s+/)[0]));
  }
  const list = [...want.entries()].filter(([id]) => fs.existsSync(path.join(ldraw, "parts", id + ".dat")));
  console.log("parts", list.length);
  const fresh = new Map();
  const t0 = Date.now();
  await new Promise((done) => {
    let next = 0, running = 0;
    const CH = 40, workers = 14;
    for (let w = 0; w < workers; ++w) {
      const ch = fork(fileURLToPath(import.meta.url), ["--worker", ldraw, shadow], { stdio: ["ignore", "ignore", "ignore", "ipc"] });
      const feed = () => {
        if (next >= list.length) { ch.kill(); if (--running === 0) done(); return; }
        ch.send(list.slice(next, next + CH));
        next += CH;
      };
      ch.on("message", (r) => {
        for (const x of r) fresh.set(x.id, x);
        if (fresh.size % 2000 < CH) console.log(fresh.size, "/", list.length, ((Date.now() - t0) / 1000).toFixed(0) + " s");
        feed();
      });
      ++running;
      feed();
    }
  });
  const count = (t) => { const c = {}; for (const k of t || []) c[k] = (c[k] || 0) + 1; return c; };
  const fmt = (c) => Object.entries(c).map(([k, v]) => k + " " + v).join(", ") || "rien";
  const lines = [];
  const changed = new Set();
  // a part of a pack: its new connectors (only the parts whose connectors come from LDCad: the others came from rules or
  // the old BrickIt, and the converter did not change for them)
  const pinCount = (t) => { const c = count(t); return (c.pin || 0) + "/" + (c.pinhole || 0); };
  const skipped = [];
  const apply = (m, where) => {
    const f = fresh.get(m.name);
    if (!f || f.err || f.src !== "shadow" || !m.voxels) return false;
    // only the parts whose pins or pin holes changed (the rest: the same connectors, or a change of another rule)
    if (pinCount(m.connectorTypes) === pinCount(f.types)) return false;
    // the new voxels in the frame of the mesh of the pack (its min corner): the two meshes centred on each other, when
    // they have the same size (the LDraw file may have changed since the pack was built)
    let shift = [0, 0, 0];
    if (m.q) {
      const old = m.q.sc.map((v) => v * 65535);
      if (old.some((v, k) => Math.abs(v - f.size[k]) > 0.06)) { skipped.push(`${m.name}\t${m.label || ""}\tgéométrie différente du pack`); return false; }
      shift = old.map((v, k) => m.q.mn[k] + (v - f.size[k]) / 2);
    }
    const vox = f.voxels.map((v) => v.map((x, k) => Math.round((x + shift[k]) * 10000) / 10000));
    if (!changed.has(m.name)) lines.push(`${m.name}\t${m.label || ""}\tavant : ${fmt(count(m.connectorTypes))}\taprès : ${fmt(count(f.types))}`);
    changed.add(m.name);
    m.voxels = vox;
    m.connectorTypes = f.types;
    if (!m.hasStuds && f.grab) { m.grab = [f.grab[0] + shift[0], f.grab[1] + shift[2]]; m.grabPoints = f.grabPoints.map((g) => [g[0] + shift[0], g[1] + shift[2]]); }
    return true;
  };
  for (const L of libs) {
    const voxN = new Map();
    const files = fs.readdirSync(L.dir).filter((f) => L.technic ? /^g\d+\.wasm$/.test(f) : /^f\d+\.bin$/.test(f));
    let nPacks = 0;
    for (const f of files) {
      const full = path.join(L.dir, f);
      const raw = zlib.inflateSync(fs.readFileSync(full));
      const jl = raw.readUInt32LE(0);
      const metas = JSON.parse(raw.subarray(4, 4 + jl).toString("utf8"));
      let any = false;
      for (const m of metas) if (apply(m, f)) { any = true; voxN.set(m.name, m.voxels.length); }
      if (!any || dry) continue;
      const js = Buffer.from(JSON.stringify(metas), "utf8"), hd = Buffer.alloc(4);
      hd.writeUInt32LE(js.length, 0);
      if (!fs.existsSync(full + ".prev")) fs.copyFileSync(full, full + ".prev");
      fs.writeFileSync(full, zlib.deflateSync(Buffer.concat([hd, js, raw.subarray(4 + jl)]), { level: 9 }));
      ++nPacks;
    }
    if (voxN.size && !dry) {
      for (const p of L.ix.parts) if (voxN.has(p.name) && p.voxN !== undefined) p.voxN = voxN.get(p.name);
      const ixf = path.join(L.dir, "index.json");
      if (!fs.existsSync(ixf + ".prev")) fs.copyFileSync(ixf, ixf + ".prev");
      fs.writeFileSync(ixf, JSON.stringify(L.ix));
    }
    console.log(L.dir, "parts changed", voxN.size, "packs rewritten", nPacks);
  }
  if (baseJ) {
    let n = 0;
    for (const m of baseJ.parts) if (apply(m, "base")) ++n;
    if (n && !dry) {
      if (!fs.existsSync(base + ".prev")) fs.copyFileSync(base, base + ".prev");
      fs.writeFileSync(base, JSON.stringify(baseJ));
    }
    console.log("base parts changed", n);
  }
  lines.sort();
  fs.writeFileSync(report, `Connecteurs corrigés (${lines.length} pièces)\n` + lines.join("\n") + `\n\nNon corrigées (${skipped.length})\n` + skipped.join("\n") + "\n");
  console.log("report", report, lines.length);
}
