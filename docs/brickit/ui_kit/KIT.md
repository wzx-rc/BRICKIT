# Kit d'interface BrickIt iPad 2.0 (8 novembre 2026)

Référence validée par Wzx (« Exactement ça ») : l'écran « menu compact » de la maquette v5 Tiny Glade (maison moderne, brique bleue 1 × 6 sélectionnée, quatre bulles brunes translucides autour, feuilles crème). **Remplace le kit 1.0** (anneau, colonne de galets à gauche, blanc/rouge/jaune) : ne plus l'utiliser.

Page visuelle du kit (écrans, composants, jetons, icônes) : https://claude.ai/artifact/ED4FP8fYcJCUxSRSQYJZvJ, copie locale `/mnt/project-files/brickit_ipad/ui_kit/BrickIt_UI_Kit.html`.

Les couleurs ont été **mesurées au pixel** sur l'image validée. Toutes les tailles sont en **points iPad** (x2 en pixels sur iPad Pro 11"). Valeurs exactes dans `tokens.json`.

## Fichiers

| Fichier | Usage |
|---|---|
| `tokens.json` | couleurs, rayons, tailles, ombres, flous, mouvement, styles de texte |
| `couleurs.json` | 38 couleurs LEGO : code LDraw, nom français, hex, famille du nuancier (classement à la main) |
| `icons/*.png` | 34 icônes, masques **blancs** RGBA 128 px, à teinter : brun `#574638` sur galet et feuille, blanc sur bulle, rouge `#C9452F` pour Supprimer |
| `icons/*.svg` | sources (grille 24, trait 2,6, bouts et angles ronds) |
| `fonts/Fredoka-400/500/600.ttf` | police de toute l'interface (OFL, latin complet avec accents). Remplace Inter |
| `shapes/tenon_relief.png` | relief du tenon couleur, 128 px, à poser **sans teinte** sur un disque de la couleur LEGO |
| `shapes/shadow_circle.png` | ombre ronde floue 256 px, disque utile 128 px au centre (galets, bulles, tenons) |
| `shapes/shadow_rrect_9slice.png` | ombre de rectangle arrondi 192 px, tranches de 48 (dock, feuilles, tiroir) |
| `shapes/circle_mask.png` | disque antialiasé si besoin |
| `exemples/*.jpg` | les 10 écrans d'exemple + la galerie de composants |

Texte (Fredoka) : titre de feuille 20/500 · grand titre 28/500 · ligne de menu 17/400 · pilule du haut 16 (détail 14) · onglet 13 · étiquette 13/500 · surtitre 13 capitales, interlettre 0,06.

## Couleurs (mesurées sur l'image)

| Jeton | Valeur | Où |
|---|---|---|
| brun | `#574638` | texte, icônes sur galet et feuille, trait du geste |
| brun_pale | `#968672` | détail de la pilule, surtitres, texte secondaire |
| brun_doux | `#8D7A64` | onglets inactifs, segmenté inactif |
| inactif | `#B6A690` | icône d'un galet inactif (Rétablir sans rien à rétablir) |
| actif_fond | `#312E27` | galet actif (fond sombre, icône blanche) |
| **galet** | `rgba(255,249,241,0.78)` | galets, pilule du haut, dock : **translucides** |
| **bulle** | `rgba(64,46,32,0.48)` | bulles d'action autour de la brique : **brun translucide**, icône blanche |
| bulle_visee | `rgba(255,255,255,0.92)` | bulle visée pendant le geste (icône brune) |
| bulle_geste | `#CABBA6` | les autres bulles pendant le geste |
| feuille | `rgba(251,246,237,0.96)` | feuilles et menus (rendu mesuré `#F9F5EB`) |
| tiroir | `rgba(250,244,233,0.94)` | tiroir de bibliothèque |
| creme_creux | `#EFE5D4` | pistes de segmenté, puces |
| separateur / poignee | `#E3D7C3` / `#CDBCA4` | |
| jaune | `#F5C32C` | bouton « toutes les pièces » du dock |
| rouge | `#C9452F` | Supprimer (texte + icône) |
| vert_enregistre | `#8BBE68` | point de la pilule du haut |
| etiquette / message | `rgba(64,46,32,0.55)` / `0.72` | pilules sombres à texte blanc |
| voile | `rgba(60,42,28,0.28)` | derrière la feuille Nouvelle scène |

Le flou d'arrière-plan (galet 10, bulle 8, dock 12, feuille 16) est un bonus : sans flou, garder les opacités telles quelles, c'est ce qui fait le côté transparent.

## Composants

### 1. Galet (bouton permanent)
- Disque `galet` 52 pt (petit : 44), icône brune 24 pt centrée. Liseré clair 1 pt blanc 80 % en haut.
- Ombre : `shadow_circle.png` teinte `rgba(70,45,20,.35)`, décalée +4 en y, taille 2 × diamètre.
- États : **pressé** = échelle 0,94 + fond `creme_creux` ; **actif** = fond `#312E27`, icône blanche ; **inactif** = icône `#B6A690` ; **blanc 38** = fond blanc, icône 22, pour l'en-tête d'une feuille (pipette, fermer).
- ImGui : AddImage(ombre) → AddCircleFilled(c, r, galet, 64) → AddImage(icône, tint brun).

### 2. Bulle d'action
- Disque `bulle` 46 pt (petite : 36), icône **blanche** 24 pt (petite : 18) avec ombre portée : la même icône dessinée avant en `rgba(30,18,8,.55)` décalée de +1 pt en y.
- Ombre de bulle : y +3, `rgba(40,25,10,.5)`.
- États : pressée = échelle 0,94 + alpha 0,58 ; **visée** = fond blanc 92 %, icône brune, échelle 1,08 ; pendant le geste les autres passent en `#CABBA6`.

### 3. Menu compact (brique touchée)
- **Cadre de sélection** : rectangle écran de la pièce, pointillés blancs 95 % de 2 pt (tirets 6, vides 4), rayon 8, halo brun `rgba(40,25,10,.6)` de 1,5 pt dessous.
- Bulles centrées sur le cadre : **Tourner** au-dessus (haut − 36), **Déplacer** dessous (bas + 38), **Dupliquer** à gauche (gauche − 42), **Couleur** à droite (droite + 42). **« … »** petite bulle au coin haut droit (+24, −24).
- Apparition : 160 ms, échelle 0,85 → 1, 25 ms d'écart (Tourner, Couleur, Déplacer, Dupliquer, « … »).
- Tourner : tap = 90°, glisser autour = crans de 15° avec l'angle dans une étiquette. Couleur : ouvre l'arc. « … » : feuille avec Supprimer (rouge), Grouper la sélection, Isoler, Prendre sa couleur, Fiche de la pièce, accrochée à droite de la bulle.

### 4. Geste rapide (appui + glisser)
- Appui de 0,3 s sur la brique puis glisser : la croix s'ouvre autour du point d'appui, bulles à 75 pt (même disposition que le menu compact).
- Anneau pointillé `#D9CBB4` 2 pt de rayon 75. Trait brun 4 pt du point d'appui vers le doigt, point brun de 7 pt au départ.
- La bulle la plus proche de la direction passe « visée ». Relâcher = action ; relâcher près du centre = rien.

### 5. Barre du haut
- Galet **Maison** à (22, 20). **Annuler** et **Rétablir** en petits galets à droite, à 76 et 24 pt du bord, y 24.
- **Pilule** centrée à 22 pt du haut : fond `galet`, padding 9 × 18, point vert 8 pt, nom 16 brun, détail 14 `brun_pale`.
- Rien de sélectionné : « Maison moderne · 214 pièces · enregistrée ». Pièce sélectionnée : « Brique 1 × 6 · Bleu · 3009 ».

### 6. Tenon couleur
- 40 pt dans l'arc, 34 dans le nuancier, 60 pour la couleur active.
- Dessin : ombre ronde (y +3, `rgba(40,25,10,.45)`, pas dans une feuille) → AddCircleFilled(couleur LEGO) → AddImage(`tenon_relief.png`, sans teinte).
- **Actuelle** : contour blanc 4 pt à 2 pt du bord (dans la scène) ou brun 3 pt (dans une feuille). **Survolée** : échelle 1,15. Le blanc dans une feuille garde un liseré `#D9CBB4` 1 pt.
- Transparente : dégradé en biais vers blanc 60 %, opacité 88 %. Métal : reflet clair en haut à gauche, sombre en bas à droite.

### 7. Arc de couleurs
- 9 tenons de 40 + bouton « + » (disque `rgba(255,249,239,.9)` 40 pt, icône ajouter 22) au bout.
- Position i (0 à 9) : θ = −84° + 168° × i / 9, x = cx + 40 + 0,8 × 150 × cos θ, y = cy + 150 × sin θ (cx, cy = centre écran de la pièce).
- Ordre : couleur actuelle, puis les plus présentes dans la scène. Tap = applique et ferme. Glisser le long = aperçu en direct. Étiquette « Bleu · 1 » sous le cadre (+50 pt).

### 8. Dock
- Pilule `galet`, rayon 28, padding 8 × 10, écart 6, centrée à 18 pt du bas, ombre 9 tranches (y +8).
- Bouton jaune 56 × 56 rayon 16 avec icône brique : ouvre le tiroir. Poignée 46 × 5 `#CDBCA4` à 12 pt au-dessus.
- Cartes 70 × 56 rayon 16, fond blanc 55 %, vignette **teintée dans la couleur active** (multiplication). Jusqu'à 8, la plus récente à gauche. Glisser vers le haut (arc de 160°) = prendre la pièce.

### 9. Coins bas
- Bas gauche (24, 24) : petit galet **boussole** (tourne avec la caméra ; tap = vue droite au 45° le plus proche).
- Bas droit (22, 22) : **couleur active** en tenon 60, petit galet **recadrer** au-dessus (écart 12). Tap sur la couleur = arc ; appui long = pinceau.

### 10. Tiroir de bibliothèque
- Feuille `tiroir`, hauteur 318 pt (glisser la poignée = plein écran), coins haut 30, ombre vers le haut. Poignée à 10 pt du haut.
- Onglets à 28 pt du haut, marge 24 : 78 de large, vignette ou icône 46 × 34 `#8D7A64` + libellé 13. Actif : fond blanc + ombre douce. Récentes, Favoris, Briques, Plaques, Pentes, Rondes, Technic, Fenêtres, Nature.
- Recherche : pilule blanche 190 pt à droite, icône chercher 20 `#A39079`.
- Puces à 116 pt : 14 pt sur `creme_creux`, active brune texte blanc. Grille à 156 pt : 9 colonnes, cartes blanches 72 pt rayon 16, écart 10. La carte prise passe à 35 %.
- Pièce prise : vignette suit le stylet/doigt, fantôme à 50 % avec halo blanc là où elle s'emboîte, étiquette « Brique 2 × 4 · s'emboîte ici ».

### 11. Feuille, menu, segmenté
- Feuille `feuille`, rayon 26, ombre 9 tranches (y +18, `rgba(60,40,20,.5)`).
- En-tête : titre 20/500 + galets blancs 38. Surtitres 13 capitales `brun_pale`.
- Ligne de menu : padding 11 × 14, icône 24 + libellé 17, détail 14 à droite. Proposée / pressée : fond blanc rayon 14. Séparateur 1 pt `#E3D7C3`.
- Segmenté : piste `creme_creux` pilule, padding 3, options 14 ; active fond blanc + ombre douce.
- **Nuancier** : 328 pt à droite (808, 22), padding 18, filtre Toutes / Dans la scène / Pour la pièce, familles en surtitres, tenons 34 écart 8.
- **Menu Maison** : feuille de 380 pt sous le galet Maison (22, 84), qui passe actif. 5 lignes (Nouvelle scène, Ouvrir, Enregistrer une copie, Partager une image, Lire une notice) puis 4 réglages segmentés (Qualité, Lumière, Grille, Construire au stylet seulement).
- **Nouvelle scène** : voile + feuille 880 pt (157, 70), padding 28, titre 28 ; 3 grandes cartes blanches (Plaque verte, Sol libre, Depuis une notice) puis « Reprendre » avec 4 vignettes.

### 12. Étiquette et message
- Étiquette : pilule `etiquette`, texte blanc 13/500, padding 5 × 9.
- Message : pilule `message`, icône 18 blanche + texte 15, 118 pt au-dessus du bas, 2,5 s.

## Écrans d'exemple (`exemples/`)
1 repos · 2 brique touchée · 3 « … » ouvert · 4 geste rapide · 5 arc de couleurs · 6 nuancier · 7 bibliothèque au stylet · 8 menu Maison · 9 nouvelle scène · 10 message.

## Scène (rendu)
Ciel sable et pêche, sol crème avec grille très légère, soleil bas du matin, ombres longues et douces (décor de la maquette v5).
