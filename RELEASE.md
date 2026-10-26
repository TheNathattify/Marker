# Procédure de release — Marker (fork thenathattify)

Ce document décrit tout ce que **toi** dois faire pour publier une release.
La CI fait le reste automatiquement une fois le tag pushé.

> **Principe** : on utilise **uniquement les outils officiels** de chaque
> écosystème, pas de méthode parallèle.

| Canal | Outil officiel | Comment ça build |
|---|---|---|
| RPM Fedora | **COPR + Packit** (packit.dev) | automatique à chaque tag `v*` |
| DEB Ubuntu | **PPA Launchpad** (miroirs git + recette) | « Request build(s) » manuel à chaque release |
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
3. Installe l'application GitHub **Packit-as-a-Service** sur le dépôt
   `TheNathattify/Marker` : https://github.com/apps/packit-as-a-service
   → « Install it for free » → compte **TheNathattify** → « Only select
   repositories » → `TheNathattify/Marker` → Install.
   (L'app s'appelait « packit-service » avant, l'ancien lien est mort.)
4. **Approuve ton compte** (obligatoire, une seule fois) : à l'installation,
   Packit vérifie ton compte Fedora (FAS) et crée une issue dans
   https://github.com/packit/notifications en te taguant. Tant que tu n'es
   pas approuvé, Packit ignore tous les événements sans rien afficher
   (aucun statut, aucun commentaire).
   - Va sur
     `https://accounts.fedoraproject.org/user/<ton-fas>/settings/profile/#github`
     et mets **GitHub Username = `TheNathattify`**
   - Vérifie que ton profil n'est pas privé :
     `https://accounts.fedoraproject.org/user/<ton-fas>/settings/profile/#is_private`
   - Commente dans l'issue créée par Packit :
     `/packit verify-fas <ton-fas-username>`
   - Packit vérifie le match entre le champ `GitHub Username` et ton compte
     GitHub, puis t'approuve (l'issue est fermée).
5. Au **premier** build, Packit demande la permission « builder » sur le
   projet COPR et poste un commentaire : confirme la demande sur
   https://copr.fedorainfracloud.org/coprs/thenathattify/marker/permissions/
   puis relance le build (bouton « Re-run », ou recrée la release).
   Les builds suivants n'ont plus besoin de cette étape.
6. C'est tout : le fichier `.packit.yaml` est déjà dans le dépôt. À chaque
   tag `v*`, Packit build le RPM dans `thenathattify/marker` automatiquement.

### 1.3 PPA Launchpad (Debian/Ubuntu) — recette « built on request », méthode officielle

Launchpad build le paquet à partir de **miroirs git** poussés sur
Launchpad (pas d'import GitHub), via une recette. Les recettes deb n'ont
**pas** de mode « build auto au push » (réservé aux snaps) : chaque build
se lance à la main, via **Request build(s)** sur la page de la recette.
Particularité de Marker : le build compile directement les sources
du sous-module `src/scidown` (qui a lui-même `charter`, qui a lui-même
`tinyexpr`), et **un build depuis un dépôt git ne récupère pas les
sous-modules** (bug git-build-recipe n°1733603). La recette embarque donc
les 3 sous-modules en plus et les « nest » à la main.

1. Compte Launchpad + clé GPG publiée + Code de Conduite signé
   + **clé SSH** ajoutée : https://launchpad.net/~/+sshkeys
2. Crée le PPA : `ppa:thenathattify/marker`
3. **Miroirs des 4 dépôts sur Launchpad** (méthode CLI — les dépôts sont
   créés automatiquement au premier push) :

   ```sh
   cd ~/Marker
   git push git+ssh://thenathattify@git.launchpad.net/~thenathattify/+git/Marker \
     'refs/heads/main:refs/heads/main'
   git -C src/scidown push \
     git+ssh://thenathattify@git.launchpad.net/~thenathattify/+git/scidown \
     'refs/heads/*:refs/heads/*' 'refs/tags/*:refs/tags/*'
   git -C src/scidown/src/charter push \
     git+ssh://thenathattify@git.launchpad.net/~thenathattify/+git/charter \
     'refs/heads/*:refs/heads/*' 'refs/tags/*:refs/tags/*'
   git -C src/scidown/src/charter/src/tinyexpr push \
     git+ssh://thenathattify@git.launchpad.net/~thenathattify/+git/tinyexpr \
     'refs/heads/*:refs/heads/*' 'refs/tags/*:refs/tags/*'
   ```

   → `lp:~thenathattify/+git/Marker` (branche `main`), `+git/scidown`,
   `+git/charter`, `+git/tinyexpr` (branches + tags : les commits épinglés
   de la recette sont présents).
4. Crée le dépôt **packaging** (il ne contient QUE `debian/`, à la racine)
   en poussant un dépôt neuf — **pas** le dépôt Marker, et branche `main`
   (**pas** `master`) :

   ```sh
   mkdir -p /tmp/marker-packaging && cd /tmp/marker-packaging
   git init -b main
   mkdir -p debian/source
   cp ~/Marker/packaging/debian/control \
      ~/Marker/packaging/debian/copyright \
      ~/Marker/packaging/debian/rules debian/
   cp ~/Marker/packaging/debian/source/format debian/source/
   sed -e "s/@VERSION@/2026.10.7/g" -e "s/@DATE@/$(date -R)/g" \
     ~/Marker/packaging/debian/changelog.in > debian/changelog
   git add -A
   git commit -m "Debian packaging for marker 2026.10.7"
   git remote add origin \
     git+ssh://thenathattify@git.launchpad.net/~thenathattify/+git/marker-packaging
   git push --set-upstream origin main
   ```

5. Sur la page du dépôt `~thenathattify/+git/Marker` → **Create packaging recipe** :
   - **nom** : `marker` (le nom du paquet source)
   - **PPA** : `ppa:thenathattify/marker` (existante)
   - séries : **resolute (26.04 LTS)** uniquement
     (noble 24.04 = gtk 4.14 < 4.16 requis ; questing 25.10 et plucky 25.04
     sont EOL et ne sont plus proposés ; stonking 26.10 = intérimaire
     actuelle, optionnel)
   - **Build schedule : « Built on request »** (laisser tel quel — il
     n'existe pas d'option « auto au push » pour les recettes deb,
     seulement « Build daily », qu'on ne veut pas)
   - texte de la recette (commits épinglés = `git submodule status`) :

     ```
     # git-build-recipe format 0.4 deb-version {debupstream}
     lp:~thenathattify/+git/Marker main
     nest scidown lp:~thenathattify/+git/scidown src/scidown a7b7f063de4f272ef0ec12d00b98470888e8cb32
     nest charter lp:~thenathattify/+git/charter src/scidown/src/charter a25dee1214ea9ba5882325066555cb813efbb489
     nest tinyexpr lp:~thenathattify/+git/tinyexpr src/scidown/src/charter/src/tinyexpr 9476568b69de4c384903f1d5f255907b92592f45
     merge packaging lp:~thenathattify/+git/marker-packaging main
     ```

   Trois détails **obligatoires** (vérifiés avec git-build-recipe 0.3.7,
   la version des buildds Launchpad) :
   - `deb-version {debupstream}` dans l'en-tête : le buildd Launchpad
     passe toujours `--append-version ~ubuntu26.04.1`, et git-build-recipe
     refuse de lancer sans `deb-version`. La version publiée est donc
     `2026.10.X~ubuntu26.04.1` (notre version + le suffixe de la série).
   - `main` à la fin de la ligne `merge` : sans branche, le merge est un
     no-op (« Already up to date ») et le build échoue avec « No control
     file to take the package name from ».
   - le dépôt packaging utilise le format source **3.0 (native)**
     (`debian/source/format`) : cohérent avec la version sans révision
     Debian, et évite le chemin de fallback quilt/pristine-tar.

   Si un sous-module avance un jour, mettre à jour les 3 commits épinglés.
6. **Premier build** : sur
   https://code.launchpad.net/~thenathattify/+recipe/marker → bouton
   **Request build(s)** → ne garder que **amd64** → confirmer.
   Compter 10–30 min ; suivi sur
   https://launchpad.net/~thenathattify/+archive/ubuntu/marker/+builds

> **Important** : Launchpad a besoin d'un vrai `debian/changelog` dans le
> dépôt packaging (pas le template `changelog.in`). La procédure de release
> (§2) le régénère et le pousse à chaque bump.
>
> **Avantage du mode « built on request »** : aucun build parasite —
> seul le clic sur « Request build(s) » lance un build, et la version
> publiée reprend celle du `debian/changelog` (`2026.10.X`, avec le
> suffixe `~ubuntu26.04.1` ajouté par Launchpad).

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
2. Commit + push sur `main`, vérifier que la CI est verte
   - pas d'import GitHub : les miroirs Launchpad sont poussés en CLI
     à l'étape 4, aucune synchronisation à attendre
3. Tag + push :

   ```sh
   git tag -a v2026.10.X -m "Marker 2026.10.X"
   git push monfork v2026.10.X
   ```

4. Pousse la source à jour puis le changelog sur Launchpad (**ordre
   important** : source d'abord, packaging ensuite — sinon le build
   partirait avec l'ancien changelog) :

   ```sh
   cd ~/Marker
   git push git+ssh://thenathattify@git.launchpad.net/~thenathattify/+git/Marker \
     'refs/heads/main:refs/heads/main'

   cd /tmp/marker-packaging   # créé en §1.3 étape 4
   sed -e "s/@VERSION@/2026.10.X/g" -e "s/@DATE@/$(date -R)/g" \
     ~/Marker/packaging/debian/changelog.in > debian/changelog
   cp ~/Marker/packaging/debian/control \
      ~/Marker/packaging/debian/copyright \
      ~/Marker/packaging/debian/rules debian/
   cp ~/Marker/packaging/debian/source/format debian/source/
   git add -A
   git commit -m "Debian packaging for marker 2026.10.X"
   git push
   ```

   → puis lance le build du PPA : sur
   https://code.launchpad.net/~thenathattify/+recipe/marker → **Request
   build(s)** → **amd64** uniquement → confirmer. Launchpad publie
   `marker_2026.10.X~ubuntu26.04.1` dans `ppa:thenathattify/marker`
   (série resolute 26.04). Suivi :
   https://launchpad.net/~thenathattify/+archive/ubuntu/marker/+builds
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
