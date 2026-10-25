#!/usr/bin/env bash
# marker-rebrand.sh — migration préférences + purge ancienne installation
# Le rebrand (renommage app-id) est déjà fait dans les sources.
# Ce script : migre les préférences dconf ancien→nouveau schéma, purge l'ancienne install.
# Idempotent. Garde-fou branche.
set -euo pipefail

# ── Garde-fou branche ──────────────────────────────────────────────────
BRANCH="$(git branch --show-current)"
if [[ "$BRANCH" != "main" && "$BRANCH" != "task-lists-test" ]]; then
  echo "ERREUR : branche '$BRANCH' interdite."
  echo "  Travaille uniquement sur 'main' (anciennement task-lists-test)."
  exit 1
fi
echo "Branche OK : $BRANCH"

SUDO_NEEDED=0

OLD_ID="com.github.adrighem.marker"
NEW_ID="com.github.thenathattify.marker"
SCHEMA_DIR="/usr/local/share/glib-2.0/schemas"
PREFIX="/usr/local"

# ── Vérifier que le nouveau schéma est installé (rebuild requis) ───────
SCHEMAS_LIST="$(gsettings list-schemas 2>/dev/null || true)"
if ! echo "$SCHEMAS_LIST" | grep -q "^${NEW_ID}\.preferences$"; then
  echo "⚠️  Le nouveau schéma ${NEW_ID}.preferences n'est pas encore installé."
  echo "   Fais d'abord : meson setup build && ninja -C build && sudo ninja -C build install"
  echo "   Puis relance ce script pour la migration."
  exit 1
fi
echo "Nouveau schéma installé : OK"

# ── 1. Migration des préférences (dconf) ───────────────────────────────
echo ""
echo "── Migration des préférences ──"

MIGRATED=0
SKIPPED=0

for sub in editor preview window; do
  OLD_SCHEMA="${OLD_ID}.preferences.${sub}"
  NEW_SCHEMA="${NEW_ID}.preferences.${sub}"

  # Vérifier si l'ancien schéma a des données (s'il est encore listé)
  if ! gsettings list-keys "$OLD_SCHEMA" 2>/dev/null; then
    echo "  [$sub] Ancien schéma ${OLD_SCHEMA} non présent dans dconf (rien à migrer)"
    continue
  fi

  echo "  [$sub] Migration ${OLD_SCHEMA} → ${NEW_SCHEMA}"

  for key in $(gsettings list-keys "$OLD_SCHEMA" 2>/dev/null); do
    old_val=$(gsettings get "$OLD_SCHEMA" "$key" 2>/dev/null || echo "")
    if [[ -n "$old_val" && "$old_val" != "@* nothing" ]]; then
      if gsettings set "$NEW_SCHEMA" "$key" "$old_val" 2>/dev/null; then
        echo "    ✓ $key = $old_val"
        ((MIGRATED++)) || true
      else
        echo "    ✗ $key (échec)"
      fi
    fi
  done
done

echo ""
echo "  Migration terminée : $MIGRATED clés copiées"

# ── 2. Purge de l'ancienne installation ───────────────────────────────
echo ""
echo "── Purge de l'ancienne installation (${OLD_ID}) ──"

purge_file() {
  local f="$1"
  if [[ -f "$f" ]]; then
    if sudo -n rm -f "$f" 2>/dev/null; then
      echo "  supprimé : $f"
    else
      echo "  ⚠️  $f existe — à supprimer manuellement : sudo rm -f $f"
      SUDO_NEEDED=1
    fi
  fi
}

purge_file "${PREFIX}/share/glib-2.0/schemas/${OLD_ID}.gschema.xml"
purge_file "${SCHEMA_DIR}/${OLD_ID}.gschema.xml"
purge_file "${SCHEMA_DIR}/${OLD_ID}.gschema.valid"
purge_file "${PREFIX}/share/icons/hicolor/scalable/apps/${OLD_ID}.svg"
purge_file "${PREFIX}/share/icons/hicolor/symbolic/apps/${OLD_ID}-symbolic.svg"
purge_file "${PREFIX}/share/applications/${OLD_ID}.desktop"
purge_file "${PREFIX}/share/metainfo/${OLD_ID}.appdata.xml"
purge_file "${PREFIX}/share/metainfo/${OLD_ID}.metainfo.xml"

# Purge aussi les schémas compilés dans dconf (clés résiduelles)
echo "  Nettoyage dconf (anciennes clés)..."
for sub in editor preview window; do
  OLD_PATH="/com/github/adrighem/marker/preferences/${sub}/"
  dconf reset -f "$OLD_PATH" 2>/dev/null || true
done
dconf reset -f "/com/github/adrighem/marker/preferences/" 2>/dev/null || true
echo "  dconf nettoyé"

# ── 3. Recompiler les caches ───────────────────────────────────────────
echo ""
echo "── Recompilation des caches ──"
if [[ -d "${SCHEMA_DIR}" ]]; then
  if sudo -n glib-compile-schemas "${SCHEMA_DIR}" 2>/dev/null; then echo "  schémas recompilés"; else echo "  ⚠️  recompiler manuellement : sudo glib-compile-schemas ${SCHEMA_DIR}"; SUDO_NEEDED=1; fi
fi
if sudo -n gtk-update-icon-cache -f -t "${PREFIX}/share/icons/hicolor/" 2>/dev/null; then echo "  cache icônes mis à jour"; else echo "  ⚠️  mettre à jour manuellement : sudo gtk-update-icon-cache -f -t ${PREFIX}/share/icons/hicolor/"; SUDO_NEEDED=1; fi
if sudo -n update-desktop-database -q "${PREFIX}/share/applications/" 2>/dev/null; then echo "  base desktop mise à jour"; else echo "  ⚠️  mettre à jour manuellement : sudo update-desktop-database -q ${PREFIX}/share/applications/"; SUDO_NEEDED=1; fi

# ── 4. Tuer les instances en cours ─────────────────────────────────────
echo ""
echo "── Arrêt des instances marker en cours ──"
killall marker 2>/dev/null && echo "  instances arrêtées" || echo "  aucune instance en cours"

echo ""
if [[ "$SUDO_NEEDED" == "1" ]]; then
  echo ""
  echo "⚠️  Certaines opérations nécessitent sudo. Exécute manuellement :"
  echo "   sudo rm -f ${PREFIX}/share/glib-2.0/schemas/${OLD_ID}.gschema.xml"
  echo "   sudo rm -f ${PREFIX}/share/icons/hicolor/scalable/apps/${OLD_ID}.svg"
  echo "   sudo rm -f ${PREFIX}/share/icons/hicolor/symbolic/apps/${OLD_ID}-symbolic.svg"
  echo "   sudo rm -f ${PREFIX}/share/applications/${OLD_ID}.desktop"
  echo "   sudo rm -f ${PREFIX}/share/metainfo/${OLD_ID}.appdata.xml"
  echo "   sudo glib-compile-schemas ${SCHEMA_DIR}"
  echo "   sudo gtk-update-icon-cache -f -t ${PREFIX}/share/icons/hicolor/"
  echo "   sudo update-desktop-database -q ${PREFIX}/share/applications/"
fi

echo ""
echo "✅ marker-rebrand.sh terminé (migration OK)."
echo "   Vérifie : préférences conservées, À propos = 2026.12.0, icône dash correcte."
