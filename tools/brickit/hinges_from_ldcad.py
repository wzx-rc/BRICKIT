"""BrickIt - the hinge axes of the LDCad shadow library (SNAP_FGR), for the parts of the full library.

The conversion of the library kept the studs, the anti-studs and the Technic cylinders, not the « fingers » of the
hinges (SNAP_FGR): a hinge 1 x 2 base (3937) and its top (3938) had nothing to join them (Wzx 22/11/2026). This script
reads the fingers of every part (and of the sub-files it includes with SNAP_INCL) and writes
assets/brickit/hinges.json: {part: [[axis, x, y, z, length, male], ...]} in the frame of the BrickIt part (cm, the
LDraw part turned 180 degrees around X, origin at its minimum corner: info.ldrawOrigin of the full index).
axis 0 x, 1 y, 2 z; male 1 (genderOfs M: the pin of the hinge) or 0 (F: its holes).

usage: python hinges_from_ldcad.py <LDCadShadowLibrary dir> <full library index.json> <out hinges.json>
"""
import json
import os
import re
import sys

LDU = 0.04


def attrs(line):
    return dict(re.findall(r"\[(\w+)=([^\]]*)\]", line))


def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def mat_vec(a, v):
    return [sum(a[i][k] * v[k] for k in range(3)) for i in range(3)]


def ori_of(at):
    if "ori" not in at:
        return [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    n = [float(x) for x in at["ori"].split()]
    return [n[0:3], n[3:6], n[6:9]]


def pos_of(at):
    return [float(x) for x in at["pos"].split()] if "pos" in at else [0.0, 0.0, 0.0]


def fingers(lib, name, ori, pos, depth=0, seen=None):
    """the fingers of a shadow file: (centre, axis direction, length, male) in LDraw units of the part"""
    out = []
    if depth > 4:
        return out
    path = None
    for sub in ("parts", os.path.join("parts", "s"), "p"):
        p = os.path.join(lib, sub, name)
        if os.path.isfile(p):
            path = p
            break
    if not path:
        return out
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if "!LDCAD" not in line:
                continue
            at = attrs(line)
            if "SNAP_FGR" in line:
                seq = [float(x) for x in at.get("seq", "").split()]
                if not seq:
                    continue
                length = sum(seq)
                o = mat_mul(ori, ori_of(at))
                p = [pos[i] + v for i, v in enumerate(mat_vec(ori, pos_of(at)))]
                d = mat_vec(o, [0, 1, 0])   # the axis of the fingers: the local Y of the snap
                if at.get("center", "true").lower() == "false":
                    p = [p[i] + d[i] * length * 0.5 for i in range(3)]
                male = at.get("genderOfs", at.get("gender", "M")).upper().startswith("M")
                out.append((p, d, length, male))
            elif "SNAP_INCL" in line and "ref" in at:
                o = mat_mul(ori, ori_of(at))
                p = [pos[i] + v for i, v in enumerate(mat_vec(ori, pos_of(at)))]
                out += fingers(lib, at["ref"], o, p, depth + 1)
    return out


def main():
    lib, index_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(index_path, encoding="utf-8") as f:
        index = json.load(f)
    origin = {}
    for p in index["parts"]:
        o = (p.get("info") or {}).get("ldrawOrigin")
        if o:
            origin[p["name"]] = o
    table = {}
    skipped = 0
    for name in sorted(origin):
        fs = fingers(lib, name + ".dat", [[1, 0, 0], [0, 1, 0], [0, 0, 1]], [0, 0, 0])
        rows = []
        for p, d, length, male in fs:
            ax = max(range(3), key=lambda i: abs(d[i]))
            if abs(abs(d[ax]) - 1) > 1e-3:   # (an axis on a slant: not handled by the voxels)
                skipped += 1
                continue
            o = origin[name]
            c = [o[0] + p[0] * LDU, o[1] - p[1] * LDU, o[2] - p[2] * LDU]
            rows.append([ax] + [round(v, 4) for v in c] + [round(length * LDU, 4), 1 if male else 0])
        if rows:
            table[name] = rows
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("{\n" + ",\n".join('  "%s": %s' % (k, json.dumps(v)) for k, v in table.items()) + "\n}\n")
    print("%d parts with hinge axes, %d slanted axes left out" % (len(table), skipped))


if __name__ == "__main__":
    main()
