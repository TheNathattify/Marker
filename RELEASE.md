# Procédure de release — Marker (fork thenathattify)

Ce document décrit tout ce que **toi** dois faire pour publier une release.
La CI fait le reste automatiquement une fois le tag pushé.

> **Principe** : on utilise **uniquement les outils officiels** de chaque
> écosystème, pas de méthode parallèle.

| Canal | Outil officiel | Comment ça build |
|---|---|---|
| RPM Fedora | **COPR + Packit** (packit.dev) | automatique à chaque tag `v*` |
| DEB Ubuntu | **PPA Launchpad** (import GitHub + recette) | automatique à chaque push sur `main` |
| Flatpak | **dépôt perso GitHub Pages** signé GPG (docs.flatpak.org) | automatique à chaque tag, via la CI |
| Release | **GitHub Releases** + notes auto-générées | automatique à chaque tag, via la CI |

## 1. Prérequis (à faire une seule fois)

### 1.1 GitHub Pages (dépôt flatpak personnel)

Le pipeline publie le dépôt flatpak sur la branche `gh-pages`. Active GitHub
Pages pour que le site soit servi :

1. Sur GitHub → dépôt `TheNathattify/Marker` → **Settings → Pages**
2. **Source** : *Deploy from a branch* → branche `gh-pages` → dossier `/ (root)`
3. **Save**

- URL du dépôt flatpak : `https://thenathattify.github.io/Marker/repo`
- Fichier d'installation : `https://thenathattify.github.io/Marker/marker.flatpakrepo`

> La branche `gh-pages` est créée automatiquement par le job
> `flatpak-repo-publish` au premier tag. Active Pages juste après.

### 1.2 COPR (RPM Fedora) — Packit, méthode officielle

**Packit** (packit.dev) est l'outil officiel Fedora/Red Hat : il build
automatiquement ton RPM dans COPR à chaque release taggée.

1. Crée un compte Fedora : https://accounts.fedoraproject.org (avec 2FA)
2. Crée le projet COPR : https://copr.fedorainfracloud.org/coprs/new/
   - Owner : `thenathattify`, nom : `marker`
   - Description : « Marker (thenathattify fork) — markdown editor »
   - Chroots : `fedora-43-x86_64`, `fedora-44-x86_64`, `fedora-45-x86_64`
   - Fedora 42 est EOL (mai 2026) : son chroot n'existe plus sur COPR.
     Règle : à chaque sortie Fedora (2×/an, ~avril et ~octobre), ajouter le
     nouveau chroot `fedora-N-x86_64` et retirer le plus vieux (EOL).
     Mettre à jour la liste `targets` de `.packit.yaml` en même temps.
   - Fedora 41 et plus vieilles : exclues (libadwaita < 1.8 requis)
3. Installe l'application GitHub **Packit** (packit-service) sur le dépôt
   `TheNathattify/Marker` : https://github.com/apps/packit-service
4. C'est tout : le fichier `.packit.yaml` est déjà dans le dépôt. À chaque
   tag `v*`, Packit build le RPM dans `thenathattify/marker` automatiquement.

### 1.3 PPA Launchpad (Debian/Ubuntu) — recette automatique, méthode officielle

Launchpad build le paquet **tout seul** à partir du dépôt GitHub :

1. Compte Launchpad + clé GPG publiée + Code de Conduite signé
2. Crée le PPA : `ppa:thenathattify/marker`
3. **Code → Import a repository** : importe `TheNathattify/Marker` (GitHub)
4. Sur la page du projet importé → **Create packaging recipe** :
   - branche `main`, dossier `debian/` du dépôt
   - coche *Automatically build when the branch changes*
   - séries : **questing (25.10)** et **26.04 LTS** uniquement
     (noble = gtk 4.14 < 4.16 requis ; oracular/plucky = libadwaita < 1.8)
5. Au prochain push sur `main`, Launchpad build et publie le deb dans le PPA.

> **Important** : Launchpad a besoin d'un vrai `debian/changelog` dans le
> dépôt (pas le template `changelog.in`). La procédure de release (§2) génère
> et committe ce fichier à chaque bump.

### 1.4 Signature GPG du dépôt flatpak — déjà en place

Le dépôt flatpak est **signé** (bonne pratique officielle, docs.flatpak.org
*Hosting a repository*) : la clé publique est dans `marker.flatpakrepo`
(`GPGKey=`) et le pipeline signe le dépôt à chaque tag. Les utilisateurs
installent **sans** `--no-gpg-verify`.

Les secrets GitHub suivants sont configurés (clé générée pour
`Marker Flatpak Repo <nathan170262@proton.me>`) :

| Secret | Usage |
|---|---|
| `FLATPAK_GPG_PRIVATE_KEY` | clé privée (signing) |
| `FLATPAK_GPG_KEY_ID` | ID de la clé (ex: `EB5B61A4A70047CB`) |

La clé privée est aussi dans ton trousseau GPG local. En cas de perte :
régénère une clé, mets à jour les deux secrets et le `GPGKey=` de
`marker.flatpakrepo` (`gpg --export --armor <ID> | base64 -w0`).

### 1.5 Secrets GitHub (récapitulatif)

| Secret | Obligatoire | Usage |
|---|---|---|
| `GITHUB_TOKEN` | non (fourni automatiquement) | création de la release + publication gh-pages |
| `FLATPAK_GPG_PRIVATE_KEY` | oui (signature flatpak) | clé privée pour signer le dépôt |
| `FLATPAK_GPG_KEY_ID` | oui (signature flatpak) | ID de la clé à utiliser |

## 2. Publier une release

1. Bump la version dans `meson.build` (`version: '2026.10.X'`), dans
   `packaging/rpm/marker.spec` (`Version:`) et dans le `<release>` de
   `data/com.github.thenathattify.marker.appdata.xml`
   - la CI release vérifie que le tag = `meson.build` = le spec (fail-fast)
   - Packit met à jour le spec tout seul depuis le tag pour le build COPR
2. Génère et committe le changelog Debian (nécessaire pour Launchpad) :

   ```sh
   sed -e "s/@VERSION@/2026.10.X/g" -e "s/@DATE@/$(date -R)/g" \
     packaging/debian/changelog.in > packaging/debian/changelog
   git add packaging/debian/changelog
   ```

3. Commit + push sur `main`, vérifier que la CI est verte
   - Launchpad build le PPA automatiquement au push
4. Tag + push :

   ```sh
   git tag -a v2026.10.X -m "Marker 2026.10.X"
   git push monfork v2026.10.X
   ```

5. Le pipeline fait le reste : tarball source, rpm, deb, flatpak signé,
   release GitHub (assets + `SHA256SUMS.txt` + notes auto-générées),
   publication gh-pages. Packit build le COPR en parallèle.

> **Garde-fou** : si le tag ne correspond pas à la version dans
> `meson.build`, le pipeline s'arrête immédiatement (job `version-guard`).

## 3. Vérifier une release

- Release GitHub : https://github.com/TheNathattify/Marker/releases
- Vérifier les sommes : `sha256sum -c SHA256SUMS.txt`
- Tester le flatpak (depuis le dépôt signé, sans `--no-gpg-verify`) :

  ```sh
  flatpak remote-add --if-not-exists --from marker \
    https://thenathattify.github.io/Marker/marker.flatpakrepo
  flatpak install marker com.github.thenathattify.marker
  flatpak run com.github.thenathattify.marker --version   # → marker 2026.10.X
  ```

- Tester le rpm (asset de la release) :
  `sudo dnf install ./marker-2026.10.X-1.fcXX.x86_64.rpm`
- Tester le deb (asset de la release) :
  `sudo apt install ./marker_2026.10.X-1_amd64.deb`

## 4. Test e2e du pipeline (tag jetable)

Pour tester le pipeline de bout en bout sans polluer les vraies releases :

```sh
git checkout -b test-e2e-release
sed -i "s/^  version: '.*',$/  version: '0.0.0.test.1',/" meson.build
git commit -am "chore: bump version to 0.0.0.test.1 for e2e test"
git tag -a v0.0.0.test.1 -m "E2E test (throwaway)"
git push monfork v0.0.0.test.1
git checkout main && git branch -D test-e2e-release
```

Nettoyage après le test :

```sh
gh release delete v0.0.0.test.1 --yes -R TheNathattify/Marker
git push monfork --delete v0.0.0.test.1
git tag -d v0.0.0.test.1
flatpak uninstall --user com.github.thenathattify.marker   # si installé pour le test
```

## 5. Installation côté utilisateur

### Flatpak (recommandé)

```sh
flatpak remote-add --if-not-exists --from marker \
  https://thenathattify.github.io/Marker/marker.flatpakrepo
flatpak install marker com.github.thenathattify.marker
```

### COPR (Fedora, via Packit)

```sh
sudo dnf copr enable thenathattify/marker
sudo dnf install marker
```

### PPA (Ubuntu, via Launchpad)

```sh
sudo add-apt-repository ppa:thenathattify/marker
sudo apt update && sudo apt install marker
```

### RPM / DEB manuels

Télécharger les assets sur la release GitHub.
