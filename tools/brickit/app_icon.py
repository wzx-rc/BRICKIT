# BrickIt PC: the application icon (taskbar, title bar, Explorer): the pictogram of the family « Briques » of the
# Studio interface as it is when active (Wzx 09/11/2026): a 2x4 brick in thin amber lines, isometric, its faces
# lightly tinted, on the dark smoked grey of the interface.
# Usage: py tools/brickit/app_icon.py   -> src/brickit/res/brickit.ico (16 to 256 px) and brickit_256.png
import math, os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "src", "brickit", "res")
C30, S30 = math.cos(math.radians(30)), 0.5

def P(x, y, z):
    return ((x - z) * C30, (x + z) * S30 - y)

def faces():
    """The 2x4 brick: two sides, the top, eight studs (side, then top), back to front."""
    w, l, h = 2.0, 4.0, 1.2
    out = [
        [P(0, 0, l), P(w, 0, l), P(w, h, l), P(0, h, l)],
        [P(w, 0, 0), P(w, h, 0), P(w, h, l), P(w, 0, l)],
        [P(0, h, 0), P(w, h, 0), P(w, h, l), P(0, h, l)],
    ]
    for (i, j) in sorted([(i, j) for i in range(2) for j in range(4)], key=lambda q: q[0] + q[1]):
        cx, cz, r, sh = i + 0.5, j + 0.5, 0.3, 0.22
        side = [P(cx + r * math.cos(t), h, cz + r * math.sin(t)) for t in [math.pi / 4 + k * math.pi / 16 for k in range(17)]]
        side += [P(cx + r * math.cos(t), h + sh, cz + r * math.sin(t)) for t in [math.pi / 4 + k * math.pi / 16 for k in range(16, -1, -1)]]
        top = [P(cx + r * math.cos(t), h + sh, cz + r * math.sin(t)) for t in [k * 2 * math.pi / 40 for k in range(40)]]
        out += [side, top]
    return out

def render(size, ios=False):
    S = 1024
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    m = 0 if ios else int(S * 0.06)   # iPad: the whole square, opaque (iPadOS rounds the corners itself)
    # the dark smoked square of the interface, a soft vertical gradient and a fine light rim
    grad = Image.new("RGBA", (S, S))
    gd = ImageDraw.Draw(grad)
    for y in range(S):
        t = y / S
        gd.line([(0, y), (S, y)], fill=(int(44 - 16 * t), int(46 - 16 * t), int(51 - 17 * t), 255))
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((m, m, S - m, S - m), radius=0 if ios else int(S * 0.2), fill=255)
    im.paste(grad, (0, 0), mask)
    if not ios:
        d.rounded_rectangle((m, m, S - m, S - m), radius=int(S * 0.2), outline=(255, 255, 255, 46), width=max(2, int(S * 0.008)))
    # the brick, fitted in the middle
    fs = faces()
    xs = [p[0] for f in fs for p in f]
    ys = [p[1] for f in fs for p in f]
    ext = max(max(xs) - min(xs), max(ys) - min(ys))
    k = S * (0.6 if ios else 0.66) / ext
    cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    # the stroke: thin at 256 px, thicker at the small sizes so that it still reads
    stroke_px = {16: 1.3, 24: 1.5, 32: 1.7, 48: 2.0, 64: 2.4, 128: 3.6, 256: 5.5}.get(size, 5.5)
    width = max(2, int(round(stroke_px * S / size)))
    layer = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ld = ImageDraw.Draw(layer)
    for f in fs:
        pts = [(S / 2 + (x - cx) * k, S / 2 + (y - cy) * k) for (x, y) in f]
        ld.polygon(pts, fill=(217, 180, 67, 46))
        ld.line(pts + [pts[0]], fill=(226, 191, 79, 255), width=width, joint="curve")
    im.alpha_composite(layer)
    return im.resize((size, size), Image.LANCZOS)

if __name__ == "__main__":
    import sys
    if "--ios" in sys.argv:   # the iPad app icon (ios/icons, CFBundleIcons~ipad of ios/Info.plist.in)
        out = os.path.join(ROOT, "ios", "icons")
        os.makedirs(out, exist_ok=True)
        for name, px in (("AppIcon76x76@2x~ipad.png", 152), ("AppIcon83.5x83.5@2x~ipad.png", 167), ("AppIcon76x76~ipad.png", 76), ("AppIcon1024.png", 1024)):
            render(px, ios=True).convert("RGB").save(os.path.join(out, name))
        print("ok", out)
        sys.exit(0)
    os.makedirs(OUT, exist_ok=True)
    sizes = [16, 24, 32, 48, 64, 128, 256]
    imgs = [render(s) for s in sizes]
    imgs[-1].save(os.path.join(OUT, "brickit_256.png"))
    imgs[-1].save(os.path.join(OUT, "brickit.ico"), sizes=[(s, s) for s in sizes], append_images=imgs[:-1])
    print("ok", os.path.join(OUT, "brickit.ico"))
