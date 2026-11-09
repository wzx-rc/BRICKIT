# The icons of the PC interface (thème PC 2.0, Wzx 09/11/2026): the fine icon set of the design thread (24 grid, 1.7
# stroke, closed shapes filled at 16 %, a white vertical sheen; docs/brickit/maquette_entre_deux/icones_fines/svg,
# branch design/entre-deux-ref) rasterised at 128 px by Edge headless (transparent background) into
# assets/brickit/ui_pc/icons/<name>_<blanc|encre>.png, read by BrickUi.cpp and drawn at 16-24 px (sharp in 4K).
# Usage: py tools/brickit/pc_icons.py <svg folder>
import os, sys, glob, subprocess, tempfile
from PIL import Image

SVG = sys.argv[1]
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "brickit", "ui_pc", "icons")
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
S, COLS = 128, 8

names = sorted(set(os.path.basename(f).rsplit("_", 1)[0] for f in glob.glob(os.path.join(SVG, "*.svg"))))
cells = [(v, n) for v in ("blanc", "encre") for n in names]
tmp = tempfile.mkdtemp()
html = os.path.join(tmp, "atlas.html")
with open(html, "w", encoding="utf-8") as f:
    f.write('<html><body style="margin:0;background:transparent">')
    for k, (v, n) in enumerate(cells):
        src = "file:///" + os.path.abspath(os.path.join(SVG, f"{n}_{v}.svg")).replace("\\", "/")
        f.write(f'<img src="{src}" style="position:absolute;left:{(k % COLS) * S}px;top:{(k // COLS) * S}px;width:{S}px;height:{S}px">')
    f.write("</body></html>")
rows = (len(cells) + COLS - 1) // COLS
png = os.path.join(tmp, "atlas.png")
subprocess.run([EDGE, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--force-device-scale-factor=1", "--default-background-color=00000000",
                f"--window-size={COLS * S},{rows * S}", f"--screenshot={png}", "file:///" + html.replace("\\", "/")], check=True, timeout=120)
atlas = Image.open(png).convert("RGBA")
os.makedirs(OUT, exist_ok=True)
for k, (v, n) in enumerate(cells):
    x, y = (k % COLS) * S, (k // COLS) * S
    atlas.crop((x, y, x + S, y + S)).save(os.path.join(OUT, f"{n}_{v}.png"))
print(len(cells), "icons ->", os.path.abspath(OUT))
