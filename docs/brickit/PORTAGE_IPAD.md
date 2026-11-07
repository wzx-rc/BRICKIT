# BrickIt iPad natif (Metal)

Plan complet (validé par Wzx le 07/11/2026) : https://claude.ai/artifact/KuB884T2XwQ6ca2BPgh9KJ

- Cible : iPad Pro 11 pouces 2e génération (2020, A12Z, 6 Go). Pas de ray tracing Metal sur cette puce :
  rendu iPad tout raster en v1, le ray tracing est gardé pour un futur iPad à puce M.
- Pas de Mac : l'.ipa est construit par GitHub Actions (`.github/workflows/ios-build.yml`, runner macOS),
  puis signé et installé depuis le PC avec Sideloadly et un Apple ID gratuit (à réinstaller tous les 7 jours).
- Jalons : J0 chaîne de build, J1 couche plateforme, J2 raster Metal, J3 édition au doigt,
  J4 HQ iPad (raster), J5 mécanique, notices, PDF.

## J0 : installer l'app de test

1. Onglet Actions du dépôt, workflow « iPad build », dernier run vert : télécharger l'artefact `BrickIt-iPad-unsigned`
   et le dézipper (il contient `BrickIt-iPad-<version>-<hash>.ipa`).
2. Brancher l'iPad au PC en USB, ouvrir Sideloadly, glisser l'.ipa, saisir l'Apple ID, Start.
3. Sur l'iPad, la première fois : Réglages > Général > VPN et gestion de l'appareil > faire confiance au profil
   de l'Apple ID (et, sur iPadOS 16+, activer Réglages > Confidentialité et sécurité > Mode développeur).
4. L'app affiche un plateau de tenons dessiné en Metal, une lumière sous chaque doigt et une ligne de diagnostic
   (version, GPU, famille Apple, ray tracing Metal oui/non, taille d'écran, images par seconde).

Le code iOS est dans `ios/` (CMake, Objective-C++).
