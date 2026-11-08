# Bibliothèque complète LDraw (BrickIt natif)

BrickIt lit, en plus de la base (29 pièces) et des packs Technic (741 pièces, dans git), toute la bibliothèque officielle
LDraw : 22 545 pièces (pièces imprimées, minifigs, obsolètes compris), converties avec les connecteurs de la LDCad shadow
library. Ces fichiers sont générés et **ne sont pas dans git** (≈ 470 Mo) ; ils vivent hors des dossiers synchronisés :

```
%USERPROFILE%\BrickIt\
  raw\ldraw_complete\ldraw\      bibliothèque officielle (complete.zip de library.ldraw.org)
  raw\ldcad_shadow\...\          LDCad shadow library (github.com/RolandMelkert/LDCadShadowLibrary)
  raw\rebrickable\*.csv.gz       données Rebrickable (couleurs réelles, tools/brickit/part_colors.py)
  full\index.json, f<n>.bin      packs de la bibliothèque complète (635 packs, ≈ 420 Mo, index 13 Mo)
  full\thumbs.pack               miniatures v3 (path tracing, PNG 179 px dont la marge, 22 545, ≈ 262 Mo ; v1 : thumbs_v1_backup.pack)
  full\thumbs_v3_parts\          une image par pièce (reprise si le rendu est interrompu)
  full\aliases.txt               alias et pièces déplacées -> pièce cible (1 662)
  full\LDConfig.ldr              couleurs LDraw (import des modèles : couleur hors palette -> la plus proche)
  omr\*.mpd                      modèles officiels téléchargés (OMR)
```

`Library::load` cherche `assets\brickit\full` à côté de l'exe, sinon `%USERPROFILE%\BrickIt\full`. Sans ces fichiers,
BrickIt fonctionne comme avant (base + Technic).

## Régénérer

```
:: 1. télécharger (une fois)
::    https://library.ldraw.org/library/updates/complete.zip  -> raw\ldraw_complete (dézippé)
::    https://github.com/RolandMelkert/LDCadShadowLibrary/archive/refs/heads/master.zip -> raw\ldcad_shadow (dézippé)
:: 2. convertir (≈ 6 min, 16 processus)
node tools\brickit\build_full.mjs "%USERPROFILE%\BrickIt\raw\ldraw_complete\ldraw" "%USERPROFILE%\BrickIt\raw\ldcad_shadow\LDCadShadowLibrary-main" "%USERPROFILE%\BrickIt\full" 16
py tools\brickit\ldraw_aliases.py "%USERPROFILE%\BrickIt\raw\ldraw_complete\ldraw" "%USERPROFILE%\BrickIt\full\aliases.txt"
copy "%USERPROFILE%\BrickIt\raw\ldraw_complete\ldraw\LDConfig.ldr" "%USERPROFILE%\BrickIt\full\"
:: 3. miniatures v3 (path tracing, ≈ 10 min, fenêtre cachée ; écrit aussi full\thumbs_base_v3.pack -> assets\brickit\thumbs_v3.pack)
build\bin\BrickIt.exe --build-full-thumbs "%USERPROFILE%\BrickIt\full\thumbs.pack"
```

## Une seule source pour le PC et l'iPad : la branche `bibliotheque`

Le dossier `full` (sans `thumbs_v3_parts` ni les sauvegardes) est publié dans la branche de données `bibliotheque` du
repo BRICKIT, sans historique (chaque publication la remplace ; `thumbs.pack` y est coupé en morceaux de 95 Mo) :

```
py tools\brickit\library_publish.py          :: après une régénération, depuis le PC
python tools/brickit/library_fetch.py <dossier> :: récupère et recolle (la CI iPad : assets/brickit/full, dans le .ipa)
```

## Limites connues

- Pièces imprimées : rendues (couleur par sommet) ; quelques pièces anciennes superposent deux impressions dans le même plan et scintillent.
- 4 pièces sans géométrie après conversion (6637a–d).

Crédits : géométrie LDraw (ldraw.org, CC BY 4.0 / CC BY 2.0), connecteurs LDCad shadow library (Roland Melkert),
couleurs réelles Rebrickable (rebrickable.com), modèles officiels LDraw OMR (CC BY / CCAL selon le modèle).
