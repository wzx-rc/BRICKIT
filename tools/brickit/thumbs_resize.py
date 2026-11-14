"""BrickIt - a thumbnail pack ("BTF1": count, then for each part its name and its PNG) reduced to a smaller size, for the
iPad build: thumbs_hq.pack (512 px, about 610 MB) at 384 px is sharp on the iPad cards (up to 2x the slider) and much
lighter in the .ipa. Images are reduced by area (Lanczos), alpha kept.

    python tools/brickit/thumbs_resize.py <pack> <size px> [out pack (default: in place)]
"""
import io, os, struct, sys
from concurrent.futures import ProcessPoolExecutor

from PIL import Image


def shrink(args):
    data, px = args
    im = Image.open(io.BytesIO(data))
    if max(im.size) <= px:
        return data
    k = px / max(im.size)
    im = im.convert("RGBA").resize((max(1, round(im.size[0] * k)), max(1, round(im.size[1] * k))), Image.LANCZOS)
    out = io.BytesIO()
    im.save(out, "PNG", optimize=False, compress_level=6)
    return out.getvalue()


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, px = sys.argv[1], int(sys.argv[2])
    dst = sys.argv[3] if len(sys.argv) > 3 else src
    d = open(src, "rb").read()
    if d[:4] != b"BTF1":
        sys.exit("not a BTF1 pack: " + src)
    count = struct.unpack_from("<I", d, 4)[0]
    o, names, imgs = 8, [], []
    for _ in range(count):
        nl = struct.unpack_from("<H", d, o)[0]; o += 2
        names.append(d[o:o + nl]); o += nl
        pl = struct.unpack_from("<I", d, o)[0]; o += 4
        imgs.append(d[o:o + pl]); o += pl
    with ProcessPoolExecutor() as ex:
        small = list(ex.map(shrink, [(i, px) for i in imgs], chunksize=64))
    out = bytearray(b"BTF1" + struct.pack("<I", count))
    for n, im in zip(names, small):
        out += struct.pack("<H", len(n)) + n + struct.pack("<I", len(im)) + im
    open(dst, "wb").write(out)
    print("%s: %d thumbnails, %d -> %d MB at %d px" % (dst, count, len(d) >> 20, len(out) >> 20, px))


if __name__ == "__main__":
    main()
