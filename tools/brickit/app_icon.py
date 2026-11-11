# BrickIt: the application icons, from the two 1024 px sources chosen by Wzx (11/11/2026, made with Gemini from his
# reference): A = the yellow 2x2 brick in white viewfinder corners on navy (the icon everywhere), B = the same brick
# without the viewfinder (Réglages › Icône de l'app : B).
#   ios/icons/source/brickit_v1_viseur_1024.png       (A)
#   ios/icons/source/brickit_v1_sans_viseur_1024.png  (B)
# Usage: py tools/brickit/app_icon.py
#   -> ios/icons/AppIcon*.png (A) and AppIconB*.png (B): the iPad (full square, iPadOS rounds the corners)
#   -> src/brickit/res/brickit.ico, brickit_b.ico (16 to 256 px, rounded with a margin) and brickit_256.png: the PC
#   -> assets/brickit/ui_kit/logo/brickit_a.png, brickit_b.png (128 px, rounded): the logo of the header in the app
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = {"a": os.path.join(ROOT, "ios", "icons", "source", "brickit_v1_viseur_1024.png"),
       "b": os.path.join(ROOT, "ios", "icons", "source", "brickit_v1_sans_viseur_1024.png")}

def rounded(im, size, margin):
    """The square rounded as the iPad does (22.37 %), with a transparent margin (PC), at `size` px."""
    S = 1024
    m = int(S * margin)
    body = im.convert("RGBA").resize((S - 2 * m, S - 2 * m), Image.LANCZOS)
    mask = Image.new("L", (S - 2 * m, S - 2 * m), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, S - 2 * m - 1, S - 2 * m - 1), radius=int((S - 2 * m) * 0.2237), fill=255)
    out = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    out.paste(body, (m, m), mask)
    return out.resize((size, size), Image.LANCZOS)

if __name__ == "__main__":
    ios = os.path.join(ROOT, "ios", "icons")
    res = os.path.join(ROOT, "src", "brickit", "res")
    logo = os.path.join(ROOT, "assets", "brickit", "ui_kit", "logo")
    os.makedirs(logo, exist_ok=True)
    for k, path in SRC.items():
        src = Image.open(path).convert("RGB")
        pre = "AppIcon" if k == "a" else "AppIconB"
        for name, px in (("76x76@2x~ipad", 152), ("83.5x83.5@2x~ipad", 167), ("76x76~ipad", 76)):
            src.resize((px, px), Image.LANCZOS).save(os.path.join(ios, pre + name + ".png"), optimize=True)
        if k == "a": src.save(os.path.join(ios, "AppIcon1024.png"), optimize=True)
        sizes = [16, 24, 32, 48, 64, 128, 256]
        imgs = [rounded(src, s, 0.03) for s in sizes]
        imgs[-1].save(os.path.join(res, "brickit.ico" if k == "a" else "brickit_b.ico"), sizes=[(s, s) for s in sizes], append_images=imgs[:-1])
        if k == "a": imgs[-1].save(os.path.join(res, "brickit_256.png"))
        rounded(src, 128, 0.0).save(os.path.join(logo, "brickit_%s.png" % k), optimize=True)
    print("ok")
