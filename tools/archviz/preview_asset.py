# Renders exported archviz-asset folders back in Blender (EEVEE, the asset's own colours / textures), to check them
# without the engine: blender -b --factory-startup --python tools\archviz\preview_asset.py -- [--out dir] [slug ...]
import bpy, json, math, os, sys
import numpy as np

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
MODELS = os.path.join(os.path.expanduser("~"), "BrickIt", "ArchViz", "models")
out = args[args.index("--out") + 1] if "--out" in args else os.path.join(MODELS, "_previews")
slugs = [a for i, a in enumerate(args) if not a.startswith("--") and (i == 0 or args[i - 1] != "--out")]
slugs = slugs or sorted(d for d in os.listdir(MODELS) if os.path.exists(os.path.join(MODELS, d, "asset.json")))
os.makedirs(out, exist_ok=True)


def srgb_to_lin(c):
    return [x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4 for x in c]


for slug in slugs:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    folder = os.path.join(MODELS, slug)
    a = json.load(open(os.path.join(folder, "asset.json"), encoding="utf-8"))
    data = open(os.path.join(folder, a["bin"]), "rb").read()
    o = 0
    for p in a["parts"]:
        V, I = p["vertices"], p["indices"]
        v = np.frombuffer(data, np.float32, V * 8, o).reshape(-1, 8); o += V * 32
        idx = np.frombuffer(data, np.uint32, I, o).reshape(-1, 3); o += I * 4
        me = bpy.data.meshes.new(p["name"])
        P = v[:, :3]
        me.vertices.add(V); me.vertices.foreach_set("co", np.stack([P[:, 0], -P[:, 2], P[:, 1]], 1).reshape(-1))
        me.loops.add(I); me.loops.foreach_set("vertex_index", idx.reshape(-1).astype(np.int32))
        me.polygons.add(len(idx)); me.polygons.foreach_set("loop_start", np.arange(0, I, 3, dtype=np.int32))
        me.polygons.foreach_set("loop_total", np.full(len(idx), 3, np.int32))
        me.update(calc_edges=True)
        uv = me.uv_layers.new()
        U = v[idx.reshape(-1), 6:8].copy(); U[:, 1] = 1 - U[:, 1]
        uv.data.foreach_set("uv", U.reshape(-1))
        me.shade_smooth()   # (preview only: the exported normals are checked by export_asset.py)
        ob = bpy.data.objects.new(p["name"], me)
        bpy.context.scene.collection.objects.link(ob)
        m = p["material"]
        mat = bpy.data.materials.new(p["name"]); mat.use_nodes = True
        b = mat.node_tree.nodes["Principled BSDF"]
        col = srgb_to_lin(m["color"]) + [1]
        b.inputs["Base Color"].default_value = col
        b.inputs["Roughness"].default_value = m["roughness"]
        b.inputs["Metallic"].default_value = m["metallic"]
        b.inputs["Transmission Weight"].default_value = m["transmission"]
        if m.get("texture"):
            t = mat.node_tree.nodes.new("ShaderNodeTexImage")
            t.image = bpy.data.images.load(os.path.join(folder, m["texture"]))
            mix = mat.node_tree.nodes.new("ShaderNodeMix"); mix.data_type = 'RGBA'; mix.blend_type = 'MULTIPLY'
            mix.inputs[0].default_value = 1.0
            mat.node_tree.links.new(t.outputs["Color"], mix.inputs[6])
            mix.inputs[7].default_value = col
            mat.node_tree.links.new(mix.outputs[2], b.inputs["Base Color"])
        me.materials.append(mat)
    sx, sy, sz = a["size"]
    r = max(sx, sy, sz)
    sc = bpy.context.scene
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    sc.collection.objects.link(cam); sc.camera = cam
    cam.data.lens = 50
    d = r * 2.1
    az, el = math.radians(-30), math.radians(18)   # front (+Z out of the asset = -Y in Blender), a bit from the left
    tgt = (0, 0, sy * 0.45)
    cam.location = (tgt[0] + d * math.sin(az) * math.cos(el), tgt[1] - d * math.cos(az) * math.cos(el), tgt[2] + d * math.sin(el))
    dirv = np.array(tgt) - np.array(cam.location)
    cam.rotation_euler = (math.atan2(math.hypot(dirv[0], dirv[1]), -dirv[2]), 0, math.atan2(dirv[0], -dirv[1]) * -1 + 0)
    import mathutils
    cam.rotation_euler = mathutils.Vector(dirv).to_track_quat('-Z', 'Y').to_euler()
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", 'SUN'))
    sun.data.energy = 3.0; sun.rotation_euler = (math.radians(50), 0, math.radians(-40))
    sc.collection.objects.link(sun)
    w = bpy.data.worlds.new("w"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.8, 0.82, 0.85, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 0.8
    sc.render.engine = 'BLENDER_EEVEE'
    sc.render.resolution_x = sc.render.resolution_y = 700
    sc.view_settings.view_transform = 'Standard'
    sc.render.filepath = os.path.join(out, slug + ".png")
    bpy.ops.render.render(write_still=True)
    print("preview", sc.render.filepath, flush=True)
