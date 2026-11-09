# BrickIt PC, Réglages > Thème: the fonts and images of the PLUM themes (Minimal, Sommet) for the native interface.
# The kit (plum-themes-kit/kit) ships variable woff2 fonts and webp images; Dear ImGui reads static TTF and BrickIt's
# decoder reads PNG. Usage: py tools/brickit/theme_fonts.py <plum-themes-kit/kit>   (needs fonttools, brotli, Pillow)
import os, sys
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer
from PIL import Image

kit = sys.argv[1]
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
fonts = os.path.join(root, "assets", "fonts")
JOBS = [  # (woff2 of the kit, axes, static TTF)
    ("themes/minimal/assets/fonts/instrument-sans.woff2", {"wght": 400, "wdth": 100}, "InstrumentSans-Regular.ttf"),
    ("themes/minimal/assets/fonts/instrument-sans.woff2", {"wght": 600, "wdth": 100}, "InstrumentSans-SemiBold.ttf"),
    ("themes/minimal/assets/fonts/manrope.woff2", {"wght": 300}, "Manrope-Light.ttf"),
    ("themes/sommet/assets/fonts/geist-mono.woff2", {"wght": 400}, "GeistMono-Regular.ttf"),
    ("themes/sommet/assets/fonts/geist-mono.woff2", {"wght": 600}, "GeistMono-SemiBold.ttf"),
]
licenses = {}
for rel, loc, name in JOBS:
    f = TTFont(os.path.join(kit, rel))
    axes = {a.axisTag for a in f["fvar"].axes} if "fvar" in f else set()
    g = instancer.instantiateVariableFont(f, {k: v for k, v in loc.items() if k in axes}) if axes else f
    g.flavor = None
    g.save(os.path.join(fonts, name))
    fam = name.split("-")[0]
    licenses[fam] = (g["name"].getDebugName(0) or "", g["name"].getDebugName(13) or "", g["name"].getDebugName(14) or "")
    print(name)
for fam, (cr, lic, url) in licenses.items():
    with open(os.path.join(fonts, "LICENSE_%s.txt" % fam), "w", encoding="utf-8") as o:
        o.write("%s\n\n%s\n%s\n" % (cr, lic, url))
# Sommet: the painted mountains (banner-wide.webp, 1600 px) for the preview card of Réglages > Thème
out = os.path.join(root, "assets", "brickit", "ui_pc", "themes", "sommet")
os.makedirs(out, exist_ok=True)
im = Image.open(os.path.join(kit, "themes/sommet/assets/banner-wide.webp")).convert("RGB")
im = im.resize((640, round(640 * im.height / im.width)), Image.LANCZOS)
im.save(os.path.join(out, "banner-wide.png"), optimize=True)
print("banner-wide.png", im.size)
