# BrickIt iPad HUD: Recraft icon explorations (08/11/2026)

These are mockups only. No app code uses them. All 15 HUD icons come in 3 Tiny Glade cosy variants, as vector SVG files with transparent backgrounds.

| Folder | Look | Recraft settings |
|---|---|---|
| `A_bold_white_strokes/` | bold rounded white strokes, round caps, soft dark drop shadow (same spirit as `tools/brickit/hud_icons.py`) | recraftv2, icon / outline |
| `B_cream_frosted/` | warm cream hand-drawn glyphs, brown outline, on round frosted (55 % white) buttons | recraftv2, icon / doodle_fill |
| `C_two_tone_stud/` | chunky terracotta 2-tone icons with a toy-brick stud motif | recraftv2, icon / colored_shapes |

Icons: `undo redo library colours rotate_left rotate_right duplicate delete move camera settings new_scene save hand pencil` (+ a few `alt_*` alternatives).

- `raw/` holds the untouched Recraft SVGs (named by Recraft id). Recraft forces its own palette for these substyles (black/yellow/red).
- The root SVGs were restyled by a script. Each variant was recoloured to its palette (A white, B cream + brown #6B4A32, C terracotta #E08A5C / #C9693F / #8E4529 + cream #F1E4CC) and given an SVG drop-shadow on A and B.
- Recraft drew some arrows the wrong way, so a few icons are mirrored copies: A `redo` and `rotate_right`, B `redo` and `rotate_left`, C `redo` and `rotate_left`.
- `preview.html` / `preview.png` show each set on a cosy background.

## Chosen set (Wzx, 08/11/2026): A everywhere, thickened

`A_bold_white_thick/` is the set to use in the iPad app: the A icons with a 44-unit white stroke (viewBox 2048) on every shape, so the lines read as bold as Tiny Glade. `preview.png` compares each icon with the original and shows them on the 46 pt brown translucent bubbles of the mockup (https://claude.ai/artifact/EfypNaC8b69RfpSS6TAVCt).
