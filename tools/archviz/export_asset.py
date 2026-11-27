# ArchViz asset converter: 3D models (FBX / OBJ, CGMood style 3ds Max exports) -> "archviz-asset" v1 folders that
# ArchViz loads (asset.json + mesh.bin + base colour textures).
#
#   blender -b --factory-startup --python tools\archviz\export_asset.py -- [--manifest tools\archviz\models.json]
#           [--out %USERPROFILE%\BrickIt\ArchViz\models] [--only slug1,slug2] [--check-only]
#
# The manifest (models.json) says, per model: the source file (relative to "root"), its unit, the objects to drop and
# the material of each part, matched by regular expressions on "object name|material name". The exported vertex data
# is in metres, Y up (Blender (x, y, z) -> (x, z, -y)), centred in X / Z, standing on Y = 0; uv v flipped (1 - v).
# One part per resulting material (the objects that share it are joined); identical (position, normal, uv) corners are
# welded. Objects without UVs that get a texture are box-projected (the "box_uv" metres per repeat of the override).
#
# Engine limits (vp3d): one base colour texture per material, scalar roughness / metallic, transmission, emission. No
# normal / bump / roughness / opacity maps; at most 16 textures in a scene: the manifest keeps a budget per model.
import bpy, json, math, os, re, struct, sys, time
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return args[args.index(name) + 1] if name in args else default


MANIFEST = arg("--manifest", os.path.join(HERE, "models.json"))
OUT = arg("--out", os.path.join(os.path.expanduser("~"), "BrickIt", "ArchViz", "models"))
ONLY = set(filter(None, (arg("--only", "") or "").split(",")))
CHECK_ONLY = "--check-only" in args
MAX_TEX = 2048


def principled(mat):
    """Base colour (linear) and roughness of a Blender material (the importer's guess from the FBX / MTL)."""
    if mat and mat.use_nodes and mat.node_tree:
        for n in mat.node_tree.nodes:
            if n.type == 'BSDF_PRINCIPLED':
                return list(n.inputs['Base Color'].default_value[:3])
    return [0.6, 0.6, 0.6]


def lin_to_srgb(c):
    return [round((12.92 * x if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055), 4) for x in c]


def import_model(path, opts):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if path.lower().endswith(".obj"):
        bpy.ops.wm.obj_import(filepath=path, use_split_objects=True, use_split_groups=True)
    elif path.lower().endswith((".fbx",)):
        bpy.ops.import_scene.fbx(filepath=path)
    elif path.lower().endswith((".glb", ".gltf")):
        bpy.ops.import_scene.gltf(filepath=path)
    else:
        raise RuntimeError("unsupported format: " + path)


def split_loose(min_faces):
    """Separates the loose parts of every mesh; the pieces of at least min_faces faces are renamed big_0, big_1... by
    decreasing size (one-object OBJ exports: cushions and throws told from the frame by their size)."""
    for ob in [o for o in bpy.context.scene.objects if o.type == 'MESH']:
        bpy.ops.object.select_all(action='DESELECT')
        bpy.context.view_layer.objects.active = ob
        ob.select_set(True)
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.separate(type='LOOSE')
        bpy.ops.object.mode_set(mode='OBJECT')
    pieces = sorted([o for o in bpy.context.scene.objects if o.type == 'MESH'], key=lambda o: -len(o.data.polygons))
    for i, o in enumerate([p for p in pieces if len(p.data.polygons) >= min_faces]):
        o.name = "big_%d" % i


def material_for(model, objname, matname):
    key = objname + "|" + matname
    for ov in model.get("materials", []):
        if re.search(ov["match"], key, re.IGNORECASE):
            return ov
    return None


def box_uv(P, N, scale):
    """per corner uv of a box projection (P, N: (n, 3) corners of whole triangles, scale: metres per repeat)"""
    a = np.abs(N.reshape(-1, 3, 3).sum(axis=1))           # face normal (sum of its corners) per triangle
    ax = np.argmax(a, axis=1).repeat(3)
    u = np.where(ax == 0, P[:, 2], P[:, 0])
    v = np.where(ax == 1, P[:, 2], P[:, 1])
    return np.stack([u / scale, v / scale], axis=1)


def save_texture(src, dst):
    img = bpy.data.images.load(src, check_existing=False)
    w, h = img.size
    if max(w, h) > MAX_TEX:
        k = MAX_TEX / max(w, h)
        img.scale(max(1, round(w * k)), max(1, round(h * k)))
    sc = bpy.context.scene
    st = sc.render.image_settings
    st.file_format = 'JPEG'
    st.quality = 90
    st.color_mode = 'RGB'
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.view_settings.exposure = 0
    sc.view_settings.gamma = 1
    sc.display_settings.display_device = 'sRGB'
    img.save_render(dst, scene=sc)
    out = img.size[:]
    bpy.data.images.remove(img)
    return out


def export_model(model, root):
    slug = model["slug"]
    base = os.path.join(root, model["folder"])
    src = os.path.join(base, model["source"])
    t0 = time.time()
    import_model(src, model)
    if model.get("split_loose"):
        split_loose(int(model["split_loose"]))
    unit = float(model.get("unit", 1.0))      # source units -> metres (after Blender's own import scaling)
    drop = [re.compile(p, re.IGNORECASE) for p in model.get("drop", [])]
    keep = [re.compile(p, re.IGNORECASE) for p in model.get("keep", [])]
    dg = bpy.context.evaluated_depsgraph_get()
    groups = {}        # material key -> {"mat": spec, "P": [], "N": [], "UV": []}
    dropped, kept = [], []
    for ob in list(bpy.context.scene.objects):
        if ob.type != 'MESH':
            continue
        if any(d.search(ob.name) for d in drop) or (keep and not any(k.search(ob.name) for k in keep)):
            dropped.append(ob.name)
            continue
        kept.append(ob.name)
        ev = ob.evaluated_get(dg)
        me = ev.to_mesh()
        me.calc_loop_triangles()
        nt = len(me.loop_triangles)
        if nt == 0:
            ev.to_mesh_clear()
            continue
        tl = np.empty(nt * 3, np.int32); me.loop_triangles.foreach_get("loops", tl)
        tm = np.empty(nt, np.int32); me.loop_triangles.foreach_get("material_index", tm)
        lv = np.empty(len(me.loops), np.int32); me.loops.foreach_get("vertex_index", lv)
        co = np.empty(len(me.vertices) * 3, np.float32); me.vertices.foreach_get("co", co)
        co = co.reshape(-1, 3)
        cn = np.empty(len(me.loops) * 3, np.float32); me.corner_normals.foreach_get("vector", cn)
        cn = cn.reshape(-1, 3)
        has_uv = len(me.uv_layers) > 0
        if has_uv:
            uvl = me.uv_layers.active.data
            uv = np.empty(len(me.loops) * 2, np.float32); uvl.foreach_get("uv", uv)
            uv = uv.reshape(-1, 2)
        M = np.array(ob.matrix_world, dtype=np.float64)
        R = M[:3, :3]
        det = np.linalg.det(R)
        Nm = np.linalg.inv(R).T
        tl3 = tl.reshape(-1, 3)
        if det < 0:
            tl3 = tl3[:, ::-1]           # a mirrored object: keep the winding counter-clockwise
        for mi in np.unique(tm):
            mat = ob.material_slots[mi].material if mi < len(ob.material_slots) else None
            mname = mat.name if mat else "-"
            ov = material_for(model, ob.name, mname)
            if ov is not None and ov.get("drop"):
                continue
            spec = {"color": lin_to_srgb(principled(mat)), "roughness": 0.6, "metallic": 0.0, "transmission": 0.0,
                    "ior": 1.5, "emission": [0, 0, 0], "emissionStrength": 0.0}
            name = re.sub(r"[^A-Za-z0-9_]+", "_", mname).strip("_") or "part"
            boxs = 0.0
            tex = None
            if ov is not None:
                for k in ("color", "roughness", "metallic", "transmission", "ior", "emission", "emissionStrength"):
                    if k in ov:
                        spec[k] = ov[k]
                name = ov.get("name", name)
                tex = ov.get("texture")
                boxs = float(ov.get("box_uv", 0.0))
                if tex and "color" not in ov:
                    spec["color"] = [1, 1, 1]
            key = json.dumps([spec, tex], sort_keys=True)
            g = groups.setdefault(key, {"name": name, "spec": spec, "tex": tex, "box": boxs, "P": [], "N": [], "UV": []})
            if boxs and not g["box"]:
                g["box"] = boxs
            sel = tl3[tm == mi].reshape(-1)
            P = co[lv[sel]].astype(np.float64) @ R.T + M[:3, 3]
            Nn = cn[sel].astype(np.float64) @ Nm.T
            P *= unit
            # Blender Z up -> Y up: (x, y, z) -> (x, z, -y)
            P = np.stack([P[:, 0], P[:, 2], -P[:, 1]], axis=1)
            Nn = np.stack([Nn[:, 0], Nn[:, 2], -Nn[:, 1]], axis=1)
            ln = np.linalg.norm(Nn, axis=1, keepdims=True)
            Nn = np.where(ln > 1e-12, Nn / np.maximum(ln, 1e-12), np.array([0.0, 1.0, 0.0]))
            g["P"].append(P.astype(np.float32))
            g["N"].append(Nn.astype(np.float32))
            if has_uv:
                U = uv[sel].astype(np.float32).copy()
                U[:, 1] = 1.0 - U[:, 1]
            else:
                U = None          # box projection once the model is centred (below)
            g["UV"].append(U)
        ev.to_mesh_clear()
    if not groups:
        raise RuntimeError("nothing exported")
    allP = np.concatenate([p for g in groups.values() for p in g["P"]])
    lo, hi = allP.min(axis=0), allP.max(axis=0)
    off = np.array([-(lo[0] + hi[0]) / 2, -lo[1], -(lo[2] + hi[2]) / 2], np.float32)
    size = (hi - lo).tolist()

    folder = os.path.join(OUT, slug)
    os.makedirs(folder, exist_ok=True)
    for f in os.listdir(folder):
        os.remove(os.path.join(folder, f))
    parts, blobs, texmap = [], [], {}
    ttot = 0
    names = set()
    for g in sorted(groups.values(), key=lambda g: g["name"]):
        P = np.concatenate(g["P"]) + off
        N = np.concatenate(g["N"])
        U = []
        for i, u in enumerate(g["UV"]):
            if u is None:
                p = g["P"][i] + off
                u = box_uv(p, g["N"][i], g["box"] or 1.0) if g["tex"] else np.zeros((len(p), 2), np.float32)
            U.append(u.astype(np.float32))
        U = np.concatenate(U)
        corners = np.ascontiguousarray(np.concatenate([P, N, U], axis=1).astype(np.float32))
        if not np.isfinite(corners).all():
            corners = np.nan_to_num(corners)
        view = corners.view(np.dtype((np.void, corners.dtype.itemsize * 8))).reshape(-1)
        _, first, inv = np.unique(view, return_index=True, return_inverse=True)
        verts = corners[first]
        idx = inv.astype(np.uint32).reshape(-1)
        nm = g["name"]
        k = 2
        while nm in names:
            nm = "%s_%d" % (g["name"], k); k += 1
        names.add(nm)
        mat = dict(g["spec"])
        if g["tex"]:
            srcp = os.path.join(base, g["tex"])
            if srcp not in texmap:
                fn = "tex_" + re.sub(r"[^A-Za-z0-9]+", "_", os.path.splitext(os.path.basename(srcp))[0]).strip("_").lower() + ".jpg"
                texmap[srcp] = (fn, save_texture(srcp, os.path.join(folder, fn)))
            mat["texture"] = texmap[srcp][0]
        parts.append({"name": nm, "vertices": int(len(verts)), "indices": int(len(idx)), "material": mat})
        blobs.append(verts.tobytes())
        blobs.append(idx.tobytes())
        ttot += len(idx) // 3
    with open(os.path.join(folder, "mesh.bin"), "wb") as f:
        for b in blobs:
            f.write(b)
    asset = {"format": "archviz-asset", "version": 1, "name": model.get("name", slug),
             "size": [round(s, 4) for s in size], "tris": int(ttot), "bin": "mesh.bin", "parts": parts}
    with open(os.path.join(folder, "asset.json"), "w", encoding="utf-8") as f:
        json.dump(asset, f, indent=1)
    print("[%s] %d parts, %d tris, size %.2f x %.2f x %.2f m, bin %.1f MB, textures: %s, dropped: %s (%.0f s)" % (
        slug, len(parts), ttot, size[0], size[1], size[2], os.path.getsize(os.path.join(folder, "mesh.bin")) / 1e6,
        ", ".join("%s %dx%d" % (v[0], v[1][0], v[1][1]) for v in texmap.values()) or "none",
        ", ".join(dropped) or "nothing", time.time() - t0), flush=True)


def check(slug):
    folder = os.path.join(OUT, slug)
    a = json.load(open(os.path.join(folder, "asset.json"), encoding="utf-8"))
    data = open(os.path.join(folder, a["bin"]), "rb").read()
    o, lo, hi, tris, bad = 0, np.full(3, 1e30), np.full(3, -1e30), 0, []
    for p in a["parts"]:
        V, I = p["vertices"], p["indices"]
        v = np.frombuffer(data, np.float32, V * 8, o).reshape(-1, 8); o += V * 32
        i = np.frombuffer(data, np.uint32, I, o); o += I * 4
        if I % 3 or (I and i.max() >= V):
            bad.append(p["name"] + ": indices")
        if not np.isfinite(v).all():
            bad.append(p["name"] + ": NaN")
        nl = np.linalg.norm(v[:, 3:6], axis=1)
        if V and np.abs(nl - 1).max() > 1e-3:
            bad.append(p["name"] + ": normals")
        if p["material"].get("texture") and not os.path.exists(os.path.join(folder, p["material"]["texture"])):
            bad.append(p["name"] + ": texture missing")
        if V:
            lo = np.minimum(lo, v[:, :3].min(0)); hi = np.maximum(hi, v[:, :3].max(0))
        tris += I // 3
    if o != len(data):
        bad.append("bin size %d, parts say %d" % (len(data), o))
    tex = sorted({p["material"]["texture"] for p in a["parts"] if p["material"].get("texture")})
    print("check %-15s %s  bbox %.3f x %.3f x %.3f (min y %.3f)  tris %d (json %d)  textures %d  %s" % (
        slug, "OK " if not bad else "BAD", *(hi - lo), lo[1], tris, a["tris"], len(tex), "; ".join(bad)), flush=True)
    return len(tex), not bad


def main():
    man = json.load(open(MANIFEST, encoding="utf-8"))
    root = os.path.expandvars(arg("--root", man["root"]))
    if not os.path.isabs(root):
        root = os.path.normpath(os.path.join(HERE, root))
    ok, ntex = True, 0
    for m in man["models"]:
        if ONLY and m["slug"] not in ONLY:
            continue
        if not CHECK_ONLY:
            try:
                export_model(m, root)
            except Exception as e:
                import traceback; traceback.print_exc()
                print("[%s] FAILED: %s" % (m["slug"], e), flush=True)
                ok = False
                continue
        n, good = check(m["slug"])
        ntex += n
        ok = ok and good
    print("total textures: %d  %s" % (ntex, "all OK" if ok else "PROBLEMS"), flush=True)


main()
