# BrickIt

Constructeur de briques natif (PC, DirectX 12) : bibliothèque LDraw complète, mécanique Technic, lecture des notices,
rendu HQ / path tracing du moteur WZX Engine.

Le moteur (rendu `vp3d`, ImGui, thème) est le sous-module `engine/` (dépôt `wzx-rc/WZX-ENGINE`, branche `brickit-native`).

## Builder

```
git clone --recurse-submodules https://github.com/wzx-rc/BRICKIT.git
cd BRICKIT
build.bat
```

Déjà cloné sans le moteur : `git submodule update --init`. Il faut Visual Studio 2022 (outils C++) et le Windows SDK
(dxc). Sortie : `build\bin\BrickIt.exe` (autre dossier : `set VP3D_OUT=...`). Tests : `build\bin\BrickIt.exe --mechtests`.

La bibliothèque LDraw complète et ses miniatures ne sont pas dans git : `%LOCALAPPDATA%\BrickIt\full`
(voir `docs/brickit/BIBLIOTHEQUE_COMPLETE.md`). Les préférences et l'autosave sont à côté de l'exe.

## Contenu

- `src/brickit` : l'application
- `assets/brickit` : bibliothèque de base, couleurs, réglages et scène par défaut, icônes du HUD, miniatures de base
- `docs/brickit` : suivi (journal, captures), tests, build web de référence
- `tools/brickit` : outils de conversion (LDraw, miniatures, icônes)
