# Contexte projet Marker — passation à Vibe Code

> Document de passation auto-contenu. À coller en premier message dans une session Vibe Code ouverte dans `~/Marker`, ou à sauvegarder en tant que `AGENTS.md` à la racine du dépôt (Vibe Code le lit automatiquement).

## 1. Identité du projet

- **Fork dur personnel de [adrighem/Marker](https://github.com/adrighem/Marker)** : éditeur Markdown GTK4/libadwaita/WebKitGTK 6.0 (lui-même port GTK4 de fabiocolacio/Marker). Propriétaire : **Nathan Obry**, handle GitHub **`thenathattify`**.
- **Clone local : `~/Marker`**, branche de dev : `task-lists-test` (à renommer `main` — voir §5). Remote `monfork` = fork GitHub de Nathan (auth PAT) ; upstream = adrighem.
- **Décision ferme : hard fork.** Plus aucun PR/rebase upstream. Deux PRs restent ouvertes chez adrighem (#3 `gtk4-port-fixes` : 6 fixes GTK4 ; #4 `task-lists` : feature cases à cocher) — laissées ouvertes, sans attente, non bloquantes.
- Nathan **n'y code pas habituellement** : il utilise Marker pour rédiger/lire des documents. Toute feature se juge dans cet usage.
- Machine : **Fedora 44, GNOME 51, Wayland, GPU NVIDIA propriétaire**.

## 2. ⚠️ Règle de branche CRITIQUE (à respecter en toutes circonstances)

- **Toujours travailler et builder depuis `task-lists-test` (ou `main` après renommage).**
- **NE JAMAIS tester le GUI depuis `task-lists` ni `master`** : le fix « aperçu blanc » (fix 4) y est absent → l'aperçu est TOUT BLANC et on croit à tort avoir « perdu les fixes » (c'est arrivé 3 fois).
- Diagnostic express si aperçu blanc :
  ```bash
  git branch --show-current
  grep -c "WEBKIT_NAVIGATION_TYPE_LINK_CLICKED" src/marker-preview.c   # 0 = fix 4 absent
  ```
- Si un patch a été appliqué par erreur sur `task-lists` : `git checkout -- src/` puis retour sur `task-lists-test`.

## 3. État du code — ce qui est FAIT (commité sur task-lists-test)

**6 régressions du port GTK4 corrigées** (chaîne de fixes sous la feature ; PR #3 ouverte) :
1. Crash au changement de mode de vue — refcount GTK3→GTK4 invalide dans `marker_editor_set_view_mode` (refs permanentes dans init, `g_clear_object` dans dispose).
2. Menu hamburger inerte — ordre de chargement des .ui dans le même GtkBuilder (`marker-headerbar.ui` référence `primary_menu` défini dans `marker-gear-popover.ui` → charger gear-popover AVANT headerbar ; ne jamais « réordonner » ça).
3. Entrées Preferences/About/etc. grisées — tableau `APP_MENU_ACTION_ENTRIES` restauré dans `marker.c`.
4. **Aperçu blanc** — `navigate()` de `marker-preview.c` interceptait toute navigation vers URI `.md` ; ne garder que `WEBKIT_NAVIGATION_TYPE_LINK_CLICKED` sur vrais liens locaux.
5. État plein écran désynchronisé — `marker_window_is_fullscreen()` via `gtk_window_is_fullscreen` + callback sur `notify::fullscreened`.
6. Documents ouverts en bas de page — `marker_source_view_set_text()` place le curseur au début.

**Feature task-lists** (PR #4 ouverte) : `- [ ]` / `- [x]` rendus en **cases à cocher cliquables** dans l'aperçu, bascule dans la source avec undo, scroll préservé, PUA parasites neutralisés. Tout vit dans `src/marker-markdown.c` (pré/post-processing), **JAMAIS dans scidown** (voir §6).

**Zoom UX** (commit `f1565d8b`) : Ctrl+molette par panneau (éditeur/aperçu indépendants, accumulation des deltas → cran de 10 %), widget topbar `[− champ % éditable + ⤢]` **pane-aware** (suit le panneau focus/survolé), bouton fit-to-width (`body.scrollWidth` + `max-width` calculé — ne pas utiliser `innerWidth`, voir §6), bornes éditeur 20–500 % / aperçu 10–400 %, clés prefs `source-zoom-level` + `preview-zoom-level`.

**GFM rendering fix (gfmfix)** — script `marker-gfmfix.sh` livré, **validé sur simulation mais pas encore validé GUI ni commité** : `preprocess_fences()` (une fence peut interrompre un paragraphe, style GitHub, avec compensation du curseur), `sluganchors_postprocess()` (ancres GitHub-style `<a id="slug">` sur les titres — les id numériques `toc_N.M` de scidown restent fonctionnels), mermaid `suppressErrors: true` (un diagramme invalide n'empêche plus les suivants de rendre).

**Thèmes CSS** : 8 thèmes adaptés (Mistral L/D, Hatsune Miku L/D, Fedora44 L/D, Dark Magician, Yugi Mutou) dans `~/.local/share/marker/styles/` (hors dépôt). Règles : cases à cocher stylées (`li:has(> input.task-list-item-checkbox)`), items cochés assombris (`color: inherit` sur strong/code/a… pour battre les couleurs inline des thèmes — PAS de line-through, Nathan l'a rejeté), `@media print` lisible, `color-scheme` posé.

## 4. En cours : identité du fork (myfork + rebrand)

Deux scripts ont été livrés via le chat (dans `~/Téléchargements/` a priori), **à exécuter dans cet ordre sur `task-lists-test`** :

1. **`marker-myfork.sh`** : meson `2026.12.0` + **gtk4 >= 4.16** (nécessaire : `gtk_text_buffer_undo/redo`) ; appdata release 2026.12.0 + developer Nathan Obry ; README réécrit (crédits upstream gardés) ; release-please retiré.
2. **`marker-rebrand.sh`** : renomme tout l'app-id `com.github.adrighem.marker` → **`com.github.thenathattify.marker`** (gsettings, gresource, icônes, desktop, appdata, flatpak, packaging, About) + **migration automatique des préférences** (copie clé par clé ancien→nouveau schéma, si les 2 coexistent) + **purge de l'ancienne installation** (schémas, icônes, caches GNOME recompilés). Requiert un rebuild AVANT la migration → relancer le script après `ninja` si la migration a été sautée.

Ce qui ne change PAS : nom « Marker », commande/binaire `marker`, thèmes `~/.local/share/marker/styles` (chemin littéral dans `marker-utils.c`), attribution.

**Séquence complète à effectuer** :
```bash
bash marker-myfork.sh
bash marker-rebrand.sh          # rebuild + migration + purge
git add -A && git commit -m 'feat: rebrand to com.github.thenathattify.marker with settings migration'
git branch -m task-lists-test main && git push monfork main --force-with-lease   # + default branch GitHub
git tag v2026.12.0 && git push monfork v2026.12.0    # quand il veut publier
flatpak uninstall --user com.github.adrighem.marker  # optionnel (segfaultait)
```
Vérifs : `killall marker` puis relancer ; préférences/thème/zooms conservés ; À propos = 2026.12.0 avec Nathan en tête ; icône du dash correcte.

**Versionnage : calver `YYYY.MM.PP`**, première version `2026.12.0` (2026.11.0 pris par le tag upstream `marker-v2026.11.0` ; les tags du fork sont `v*` sans collision).

## 5. Build & test — commandes et pièges

- Build natif : `meson setup build && ninja -C build && sudo ninja -C build install` → `/usr/local/bin/marker`. Paquets dev : meson, gdb, itstool, gtk4-devel, libadwaita-devel, gtksourceview5-devel, webkitgtk6.0-devel.
- **Mono-instance** (GtkApplication, D-Bus) : **toujours `killall marker` avant de tester au terminal**, sinon la nouvelle instance relaie à l'ancienne. (`pkill -af` n'existe pas sur Fedora.)
- **Permissions build/** : si `ninja: Error writing to build log: Permission denied` → des fichiers appartiennent à root (un `sudo ninja` a compilé en root) → `sudo chown -R nathan:nathan ~/Marker/build` une fois.
- Export CLI = bon test de la chaîne de rendu même si l'aperçu GUI est cassé : `marker -o /tmp/test.html /tmp/test.md`.
- **gschema** : une clé lue par le code mais absente du schéma compilé = **crash au lancement** (g_settings). Après ajout de clé : recompiler (`sudo glib-compile-schemas /usr/local/share/glib-2.0/schemas/`, meson le fait aussi au post-install).
- Schémas : `com.github.adrighem.marker.preferences[.editor|.preview|.window]` (→ `...thenathattify...` après rebrand). Thème sélectionné : clé `css-theme` de `preferences.preview`.
- `git push monfork task-lists-test --force-with-lease` a déjà été nécessaire (historique réécrit par amend) ; aucune PR ne pointe sur task-lists-test.

## 6. Architecture & pièges techniques (à connaître avant de toucher au code)

- **scidown = submodule pinné (`a7b7f063`) : ne JAMAIS le modifier.** Tous les correctifs de rendu passent par des pré/post-traitements dans `src/marker-markdown.c` (pattern éprouvé : `preprocess_*()` avant le rendu + `*_postprocess()` sur le HTML rendu). Exemples : task-lists (placeholders U+E000/E001, **3→3 octets → offsets et curseur préservés**), fences, ancres slug.
- **Offsets : GtkTextBuffer compte en CARACTÈRES, scidown en OCTETS** → toujours convertir (`g_utf8_pointer_to_offset`). Piège récurrent, déjà cause de bugs.
- **Undo/redo** : GtkSourceView 5 ne gère PAS l'undo (API gtk_source_buffer_undo n'existe plus) → `gtk_text_buffer_undo/redo` (GTK ≥ 4.16, d'où le bump meson). Ctrl+Z depuis l'aperçu passe par le key controller du MarkerEditor.
- **WebKitGTK** :
  - `window.innerWidth` n'est PAS divisé par le zoom plein-page (contrairement aux navigateurs) — formules de zoom : utiliser `gtk_widget_get_width()` côté GTK + `body.scrollWidth` côté JS ; le `scrollWidth` de `documentElement` est planché à la largeur du viewport (inutilisable pour le fit-to-width).
  - Interception d'URI custom : schéma `marker://` dans `navigate()` de marker-preview.c (utilisée par task-toggle et zoomfit), toujours après le garde `WEBKIT_NAVIGATION_TYPE_LINK_CLICKED` du fix 4.
  - Follow-cursor : ancre `#cursor_pos` scrollée à chaque `LOAD_FINISHED` — tout mécanisme qui recharge la page doit neutraliser l'ancre ou sauvegarder/restaurer `window.scrollY` (cf. scrollfix task-lists).
  - Un clic dans l'aperçu laisse le focus au WebView (d'où le forward des raccourcis vers le buffer).
- **hoedown/scidown (rendu)** : une fence ne peut PAS interrompre un paragraphe (corrigé côté Marker par préprocesseur — GitHub/CommonMark l'autorise) ; ids de titres numériques `toc_N.M` ; inline parser avale les `[ ]` (d'où les placeholders). Les docs rédigés « style GitHub » sont la norme attendue par Nathan.
- **Vérification d'équilibre des accolades/parenthèses** : toujours un lexer à états (les commentaires et char literals `')'` faussent le comptage naïf).
- Bug upstream scidown repéré, pas encore remonté : `rndr_blockcode` passe le contenu ```charter/```gnuplot comme format-string à printf (`%` → garbage) et gnuplot fait un popen (injection possible).
- Limites connues à NE PAS « corriger » : l'aide (Help) pointe vers un URI de l'ancien app-id (cassé, à rebrancher un jour sur le nouveau) ; version de l'À propos = `meson.build`.

## 7. Conventions pour tout nouveau script/patch (méthode éprouvée du projet)

1. **Idempotent** : grep de détection en tête de section, re-exécution = skip propre.
2. **Garde-fou branche** : n'accepter QUE `task-lists-test` OU `main`, refus explicite de `task-lists` avec message « aperçu tout blanc ».
3. **Ancres assertées** : chaque patch python vérifie que son ancre est présente EXACTEMENT 1 fois (`assert src.count(anchor) == 1`) avant d'écrire.
4. Ne jamais toucher `CHANGELOG.md` ; garder l'attribution partout (Vincent van Adrighem, Fabio Colacio, Martino Ferrari).
5. Rebuild + rappel : `killall marker` avant test GUI.
6. Valider par reconstruction (patch sur arbre frais) + lexer accolades + `bash -n` le script final.

## 8. Backlog & features candidates (décisions déjà prises)

- **Image paste (candidat n°1 retenu)** : Ctrl+V avec une image dans le presse-papiers → sauvegarde dans un sous-dossier **`assets/` à côté du .md** (pas de demande à chaque collage), insertion `![]()` dans le buffer. Pistes : `GdkTexture` → PNG/JPEG sur disque → insertion markdown.
- **EPUB export** : seul gap export (HTML/PDF/RTF/DOCX/ODDT/LaTeX déjà là, via pandoc/scidown — vérifier le branchement exact dans le code export).
- **Wikilinks `[[nom]]`** : réalisable en préprocesseur (pattern task-lists) ; le panneau backlinks = vrai morceau, jugé utile seulement en usage « vault ».
- **AutoGit** : HORS Marker (si voulu : service/timer Fedora qui committe le dossier de docs).
- **Flatpak segfault** : build flatpak utilisateur segfaulte au démarrage (récursion GTK infinie measure/allocate → stack overflow, visible via `coredumpctl`) — cause non trouvée, priorité basse.
- SoloMD écarté sur Linux (même moteur WebKitGTK, zéro avantage) ; pertinent seulement comme app Android complémentaire.