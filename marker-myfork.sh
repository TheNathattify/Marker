#!/usr/bin/env bash
# marker-myfork.sh — bump version, gtk4 >= 4.16, appdata, README, retrait release-please
# Idempotent. Garde-fou branche. Ancres assertées.
set -euo pipefail

# ── Garde-fou branche ──────────────────────────────────────────────────
BRANCH="$(git branch --show-current)"
if [[ "$BRANCH" != "main" && "$BRANCH" != "task-lists-test" ]]; then
  echo "ERREUR : branche '$BRANCH' interdite."
  echo "  Travaille uniquement sur 'main' (anciennement task-lists-test)."
  echo "  Sur 'task-lists' ou 'master', l'aperçu est TOUT BLANC (fix 4 absent)."
  exit 1
fi
echo "Branche OK : $BRANCH"

cd "$(dirname "$0")"

# ── 1. meson.build : version 2026.12.0 + gtk4 >= 4.16 ──────────────────
python3 - <<'PYEOF'
import re, sys

path = "meson.build"
src = open(path).read()

# Version
if "version: '2026.12.0'" not in src:
    anchor = "version: '2026.10.0'"
    assert src.count(anchor) == 1, f"Ancre version introuvable ou multiple: {anchor}"
    src = src.replace(anchor, "version: '2026.12.0'")
    print("  meson.build: version 2026.10.0 → 2026.12.0")
else:
    print("  meson.build: version déjà 2026.12.0 (skip)")

# gtk4 >= 4.16
if "gtk4', version: '>= 4.16" not in src:
    anchor = "dependency('gtk4', version: '>= 4.10.0')"
    assert src.count(anchor) == 1, f"Ancre gtk4 introuvable ou multiple: {anchor}"
    src = src.replace(anchor, "dependency('gtk4', version: '>= 4.16.0')")
    print("  meson.build: gtk4 >= 4.10.0 → >= 4.16.0")
else:
    print("  meson.build: gtk4 >= 4.16 déjà en place (skip)")

open(path, "w").write(src)
PYEOF

# ── 2. appdata : release 2026.12.0 + developer Nathan Obry ────────────
python3 - <<'PYEOF'
path = "data/com.github.thenathattify.marker.appdata.xml"
src = open(path).read()

# Developer → Nathan Obry (garder Vincent en crédit dans les commentaires en haut)
if "<name>Nathan Obry</name>" not in src:
    anchor = '<developer id="com.github.thenathattify">\n    <name>Vincent van Adrighem</name>\n  </developer>'
    assert src.count(anchor) == 1, f"Ancre developer introuvable ou multiple"
    replacement = ('<developer id="com.github.thenathattify">\n'
                   '    <name>Nathan Obry</name>\n'
                   '  </developer>')
    src = src.replace(anchor, replacement)
    print("  appdata: developer → Nathan Obry")
else:
    print("  appdata: developer déjà Nathan Obry (skip)")

# Ajouter release 2026.12.0 en tête de <releases>
if '<release version="2026.12.0"' not in src:
    anchor = '  <releases>\n'
    assert src.count(anchor) == 1, "Ancre <releases> introuvable ou multiple"
    new_release = (
        '  <releases>\n'
        '    <release version="2026.12.0" date="2026-12-01">\n'
        '      <description translate="no">\n'
        '        <ul>\n'
        '          <li>Hard fork under Nathan Obry (thenathattify). App-ID rebranded to com.github.thenathattify.marker.</li>\n'
        '          <li>Interactive GFM task list checkboxes in the live preview.</li>\n'
        '          <li>Reworked zoom UX: per-pane Ctrl+scroll, editable zoom field, fit-to-width button.</li>\n'
        '          <li>GTK4 regression fixes: view-mode crash, menu loading order, blank preview, fullscreen state, cursor position.</li>\n'
        '          <li>Automatic settings migration from com.github.adrighem.marker schemas.</li>\n'
        '        </ul>\n'
        '      </description>\n'
        '    </release>\n'
    )
    src = src.replace(anchor, new_release)
    print("  appdata: release 2026.12.0 ajoutée")
else:
    print("  appdata: release 2026.12.0 déjà présente (skip)")

open(path, "w").write(src)
PYEOF

# ── 3. README : section fork ───────────────────────────────────────────
python3 - <<'PYEOF'
path = "README.md"
src = open(path).read()

if "## About this fork" not in src:
    anchor = "## Attribution & Acknowledgments"
    assert src.count(anchor) == 1, f"Ancre README introuvable: {anchor}"
    fork_section = (
        "## About this fork\n\n"
        "This is a personal hard fork of [adrighem/Marker](https://github.com/adrighem/Marker) "
        "maintained by **Nathan Obry** ([@thenathattify](https://github.com/thenathattify)).\n\n"
        "It adds interactive task lists, a reworked zoom UX, GTK4 regression fixes, "
        "and is rebranded under the app-ID `com.github.thenathattify.marker`.\n\n"
        "---\n\n"
    )
    src = src.replace(anchor, fork_section + anchor)
    print("  README: section 'About this fork' ajoutée")
else:
    print("  README: section fork déjà présente (skip)")

open(path, "w").write(src)
PYEOF

# ── 4. Retirer release-please ──────────────────────────────────────────
echo "  Retrait de release-please..."
rm -f release-please-config.json
rm -f .release-please-manifest.json
rm -f .github/workflows/release.yml
echo "  release-please retiré (config, manifest, workflow)"

# ── 5. Mettre à jour la dépendance gtk4 dans le README ─────────────────
python3 - <<'PYEOF'
path = "README.md"
src = open(path).read()

if "`gtk4` (>= 4.16.0)" not in src:
    anchor = "- `gtk4` (>= 4.10.0)"
    if anchor in src:
        src = src.replace(anchor, "- `gtk4` (>= 4.16.0)")
        print("  README: dépendance gtk4 mise à jour → >= 4.16.0")
    else:
        print("  README: dépendance gtk4 pattern non trouvé (vérifier manuellement)")
else:
    print("  README: dépendance gtk4 déjà à jour (skip)")

open(path, "w").write(src)
PYEOF

echo ""
echo "✅ marker-myfork.sh terminé."
echo "   Prochaine étape : bash marker-rebrand.sh (migration + purge)"
