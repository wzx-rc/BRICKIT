# BrickIt PC: the application icon (taskbar, title bar, Explorer), in the language of the Studio interface
# (Maquette 4.0): a red rounded square, a 2x2 brick drawn in white thin lines, isometric, its faces lightly tinted.
# Usage: py tools/brickit/app_icon.py   -> src/brickit/res/brickit.ico (16 to 256 px) and brickit_256.png
import math, os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "src", "brickit", "res")
C30, S30 = math.cos(math.radians(30)), 0.5

def P(x, y, z):
    return ((x - z) * C30, (x + z) * S30 - y)

def faces():
    """The 2x2 brick: two sides, the top, four studs (side, then top), back to front."""
    w, l, h = 2.0, 2.0, 1.2
    out = [
        [P(0, 0, l), P(w, 0, l), P(w, h, l), P(0, h, l)],
        [P(w, 0, 0), P(w, h, 0), P(w, h, l), P(w, 0, l)],
        [P(0, h, 0), P(w, h, 0), P(w, h, l), P(0, h, l)],
    ]
    for (i, j) in [(0, 0), (1, 0), (0, 1), (1, 1)]:
        cx, cz, r, sh = i + 0.5, j + 0.5, 0.3, 0.22
        side = [P(cx + r * math.cos(t), h, cz + r * math.sin(t)) for t in [math.pi / 4 + k * math.pi / 16 for k in range(17)]]
        side += [P(cx + r * math.cos(t), h + sh, cz + r * math.sin(t)) for t in [math.pi / 4 + k * math.pi / 16 for k in range(16, -1, -1)]]
        top = [P(cx + r * math.cos(t), h + sh, cz + r * math.sin(t)) for t in [k * 2 * math.pi / 40 for k in range(40)]]
        out += [side, top]
    return out

def render(size):
    S = 1024
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    m = int(S * 0.06)
    # the red square, a soft vertical gradient and a fine light rim
    grad = Image.new("RGBA", (S, S))
    gd = ImageDraw.Draw(grad)
    for y in range(S):
        t = y / S
        gd.line([(0, y), (S, y)], fill=(int(226 - 40 * t), int(26 - 10 * t), int(36 - 12 * t), 255))
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((m, m, S - m, S - m), radius=int(S * 0.2), fill=255)
    im.paste(grad, (0, 0), mask)
    d.rounded_rectangle((m, m, S - m, S - m), radius=int(S * 0.2), outline=(255, 255, 255, 60), width=max(2, int(S * 0.008)))
    # the brick, fitted in the middle
    fs = faces()
    xs = [p[0] for f in fs for p in f]
    ys = [p[1] for f in fs for p in f]
    ext = max(max(xs) - min(xs), max(ys) - min(ys))
    k = S * 0.58 / ext
    cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    # the stroke: thin at 256 px, thicker at the small sizes so that it still reads
    stroke_px = {16: 1.3, 24: 1.5, 32: 1.7, 48: 2.0, 64: 2.4, 128: 3.6, 256: 5.5}.get(size, 5.5)
    width = max(2, int(round(stroke_px * S / size)))
    layer = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ld = ImageDraw.Draw(layer)
    for f in fs:
        pts = [(S / 2 + (x - cx) * k, S / 2 + (y - cy) * k) for (x, y) in f]
        ld.polygon(pts, fill=(255, 255, 255, 40))
        ld.line(pts + [pts[0]], fill=(255, 255, 255, 255), width=width, joint="curve")
    im.alpha_composite(layer)
    return im.resize((size, size), Image.LANCZOS)

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    sizes = [16, 24, 32, 48, 64, 128, 256]
    imgs = [render(s) for s in sizes]
    imgs[-1].save(os.path.join(OUT, "brickit_256.png"))
    imgs[-1].save(os.path.join(OUT, "brickit.ico"), sizes=[(s, s) for s in sizes], append_images=imgs[:-1])
    print("ok", os.path.join(OUT, "brickit.ico"))
