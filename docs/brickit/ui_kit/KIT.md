# Kit d'interface BrickIt iPad 1.0 (8 novembre 2026)

Référence choisie par Wzx : l'image « maison pastel » (galets à gauche, anneau autour de la brique avec actions à gauche et couleurs à droite, dock en bas). Direction de couleur : **blanc, rouge, jaune**, plus de crème.
Page visuelle du kit (composants, états, écrans) : https://claude.ai/artifact/ED4FP8fYcJCUxSRSQYJZvJ, copie locale `/mnt/project-files/brickit_ipad/ui_kit/BrickIt_UI_Kit.html`.

Toutes les tailles sont en **points iPad** (x2 en pixels sur iPad Pro 11"). Les valeurs exactes sont dans `tokens.json`.

## Fichiers

| Fichier | Usage |
|---|---|
| `tokens.json` | couleurs, rayons, tailles, ombres, mouvement, police |
| `couleurs.json` | 41 couleurs LEGO : code LDraw, nom français, hex, famille (pour le nuancier ; classement fait à la main) |
| `icons/*.png` | 31 icônes, masques **blancs** RGBA 128 px. Toujours dessinées avec une teinte (`ImGui::GetWindowDrawList()->AddImage(tex, a, b, uv0, uv1, tint)`) |
| `icons/*.svg` | sources (grille 24, trait 2,2, bouts ronds) |
| `shapes/blob_mask.png` | forme de pastille (fleur 6 lobes), masque blanc 128 px |
| `shapes/blob_gloss.png` | reflet de pastille (blanc, alpha déjà dosé) |
| `shapes/blob_rim.png` | liseré de pastille (blanc) |
| `shapes/circle_mask.png` | disque antialiasé si besoin d'un disque texturé |
| `shapes/shadow_circle.png` | ombre floue ronde 256 px : le disque utile fait 128 px au centre (bord flou de 64 px autour) |
| `shapes/shadow_rrect_9slice.png` | ombre de rectangle arrondi 192 px, tranches de 48 px (à étirer en 9 morceaux) |
| `exemples/*.jpg` | les 8 écrans d'exemple + la galerie de composants |

Police : **Inter** (déjà dans `assets/fonts`). Titre 22/SemiBold · Corps 17/Regular · Bouton 15/SemiBold · Légende 13/Regular · Surtitre 12/SemiBold capitales.

## Couleurs (tokens.json > color)

encre `#26211D` (texte, icônes) · encre_douce `#6E655C` · encre_pale `#A69C91` (inactif) · blanc `#FFFFFF` · givre `rgba(255,255,255,.84)` (galets, dock) · creux `#F4F2EF` (fonds pressés, pistes, cartes du tiroir) · trait `rgba(38,33,29,.08)` · **jaune `#FFC93C`** (actif, courant, visée) · jaune_doux `#FFF4CC` (carte choisie) · **rouge `#D7261B`** (supprimer, erreurs) · rouge_doux `#FDE4E1` · ombre `rgba(38,33,29,.16)` · ombre_forte `rgba(38,33,29,.24)` · voile `rgba(38,33,29,.28)` · étiquette `rgba(38,33,29,.72)` · anneau `rgba(255,255,255,.92)`.

Le flou d'arrière-plan n'est pas nécessaire : « givre » à 84 % suffit. Si le rendu le permet plus tard, un flou de 10 pt sous les galets et le dock est un bonus.

## Composants

### 1. Galet (bouton permanent)
- Disque givre 60 pt (petit : 48), icône encre 26 pt (petit : 22), centrée.
- Ombre : `shadow_circle.png` teinte ombre, centrée sur le galet décalée de +6 pt en y, taille = 2 × diamètre (le disque utile de la texture fait la moitié).
- États : **pressé** = échelle 0,94 + fond creux ; **actif** = cercle jaune épaisseur 3 autour (rayon + 1,5) ; **inactif** = icône encre_pale.
- ImGui : AddImage(ombre) → AddCircleFilled(c, r, givre, 64) → AddImage(icône, tint encre).

### 2. Bulle d'action (autour de la brique)
- Disque **blanc opaque** 52 pt, icône 24 pt encre, même ombre que le galet.
- Supprimer : icône rouge. Visée (pendant appui + glisser) : fond jaune.
- Position : sur un cercle de rayon R + 34 autour du centre écran de la pièce, côté gauche. Angles (0° = droite, sens horaire, y vers le bas) : Tourner 248°, Dupliquer 202°, Déplacer 156°, Supprimer 112°.
- Tourner : tap = 90° ; glisser autour de la bulle = crans de 15°, angle affiché dans l'étiquette.

### 3. Anneau de sélection
- Centre = centre écran de la pièce (ou du groupe). R = max(56, demi-diagonale du rectangle écran de la pièce + 12).
- AddCircle(c, R, encre 18 %, 64, épaisseur 5) puis AddCircle(c, R, anneau, 64, épaisseur 3).
- **Piste de couleurs** sur la moitié droite, de -95° à +65° : un arc par pastille de l'arc (PathArcTo + PathStroke, épaisseur 6, bouts ronds), 1,2° de vide entre deux.
- Glisser le doigt ou le stylet sur la piste : la pastille correspondante passe « survolée » et la pièce prend la couleur en direct ; relâcher = valider.

### 4. Pastille couleur
- 42 pt dans l'arc, 34 pt dans le nuancier, 30 pt dans le galet Couleur.
- Dessin (4 AddImage du même carré) : ombre = blob_mask teinte encre 28 % décalée +3 pt en y → blob_mask teinte couleur LEGO → blob_gloss teinte blanc → blob_rim teinte blanc (dans une feuille blanche : `#D9D3CC`, sinon le blanc disparaît).
- Transparente : couleur à 78 % + dégradé blanc en biais. Métal : dégradé clair vers sombre par-dessus.
- **Courante** : cercle jaune épaisseur 3 à 6 pt autour. **Survolée** : échelle 1,18.

### 5. Arc de couleurs
- 7 pastilles : la couleur de la pièce d'abord, puis les couleurs les plus présentes dans la scène.
- Réparties sur -95° … +65° (centres au milieu de chaque part), rayon R + 40.
- Bouton « + » (disque blanc 36 pt, icône ajouter 20) à +82°, rayon R + 40 : ouvre le nuancier.
- Apparition : pastille courante d'abord puis vers les bords, 18 ms d'écart, échelle 0,85 → 1 en 160 ms.

### 6. Colonne d'outils
- Bord gauche, marge 20, centrée verticalement, 4 galets 60 pt espacés de 14 : **Annuler · Bibliothèque · Couleur courante · Réglages**.
- Le galet Couleur contient une pastille 30 pt de la couleur courante ; tap = nuancier.
- Après une annulation : petit galet Rétablir (48) à droite d'Annuler pendant 4 s. Tap 2 doigts = annuler, 3 doigts = rétablir (déjà codé).

### 7. Dock des récentes
- Centré en bas, marge 20, fond givre, rayon 32, padding 8, écart 6, ombre en 9 tranches.
- Cartes 84 × 64, rayon 18, padding 6 ; vignette **multipliée par la couleur courante** (couleur pleine, pas désaturée).
- Carte choisie : fond jaune_doux + contour jaune 2. Jusqu'à 8 pièces, la plus récente à gauche. Glisser une carte vers le haut = prendre la pièce (règle des 160°).
- Caché quand le tiroir ou une feuille plein écran est ouvert.

### 8. Tiroir de bibliothèque
- Feuille blanche du bas, hauteur 40 % (glisser la poignée = plein écran), coins hauts rayon 28 (`ImDrawFlags_RoundCornersTop`), ombre vers le haut. Poignée 40 × 5 `#DDD8D2` à 10 pt du haut.
- Ligne d'onglets à 26 pt du haut, marge 20 : onglets 76 × 66, rayon 16, icône ou vignette (teinte encre_douce) 42 × 28 + libellé 13. Actif : fond creux, texte encre SemiBold. Onglets : Récentes, Favoris, Briques, Plaques, Pentes, Rondes, Technic, Fenêtres, Nature (le reste des familles en défilement).
- Recherche : pilule creuse à droite, icône chercher 20 encre_pale, texte 15.
- Puces à 106 pt : pilules 15 pt, fond creux, active = fond encre texte blanc. **Pas de compteurs.**
- Grille à 152 pt : cartes 84 × 64 fond creux, vignettes teintées couleur courante.
- Le galet Bibliothèque passe « actif » tant que le tiroir est ouvert.

### 9. Feuille, menu, segmenté
- Feuille blanche **opaque**, rayon 28, ombre forte (9 tranches). Jamais de texte sur la scène.
- En-tête : titre 22 SemiBold + petits galets 40 pt fond creux sans ombre (pipette, fermer).
- Ligne de menu 48 pt : icône 22 + libellé 17, texte secondaire 14 encre_pale à droite ; pressée : fond creux rayon 14.
- Segmenté : piste creuse rayon pilule, padding 3 ; option active fond blanc + ombre légère, SemiBold.
- Nuancier : 340 pt de large à droite, marge 20 ; familles = surtitres ; pastilles 34 en rangées, écart 8 ; filtre Toutes / Dans la scène / Pour la pièce.
- Réglages : feuille de 380 pt ouverte à droite de la colonne ; 5 lignes (Nouvelle scène, Ouvrir, Enregistrer une copie, Partager une image, Lire une notice) puis 4 réglages segmentés (Qualité, Lumière, Grille, Construire au stylet).
- Nouvelle scène : voile + feuille centrée 820 pt ; 3 cartes (Plaque verte, Sol libre, Depuis une notice) puis « Reprendre » avec 4 vignettes.

### 10. Étiquette et message
- Étiquette : pilule encre 72 %, texte blanc 13, centrée sous l'anneau à R + 64. Nom court français + couleur française (« Brique 1 × 6 · Bleu »).
- Message : pilule encre, icône 18 blanche + texte 15, centré à 110 pt du bas, 2,5 s.

## Scène (rendu)
Sol blanc cassé lumineux, horizon clair légèrement chaud, soleil bas du matin avec ombres longues, grille très légère. L'interface reste lisible sur fond clair grâce aux disques blancs et aux ombres.
