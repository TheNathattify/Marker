/*
 * marker-prefs.c
 *
 * Copyright (C) 2017 - 2026 Marker Developers
 *
 * Marker is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * Marker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with Marker; see the file LICENSE.md. If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <adwaita.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <glib/gi18n.h>

#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "marker.h"
#include "marker-widget.h"
#include "marker-string.h"
#include "marker-window.h"
#include "marker-prefs.h"
#include "marker-utils.h"

MarkerPrefs prefs;

gboolean
marker_prefs_get_use_dark_theme (void)
{
  return g_settings_get_boolean (prefs.window_settings, "enable-dark-mode");
}

void
marker_prefs_set_use_dark_theme (gboolean state)
{
  g_settings_set_boolean (prefs.window_settings, "enable-dark-mode", state);
}

guint
marker_prefs_get_window_width (void)
{
  return g_settings_get_uint (prefs.window_settings, "window-width");
}

void
marker_prefs_set_window_width (guint width)
{
  g_settings_set_uint (prefs.window_settings, "window-width", width);
}

guint
marker_prefs_get_window_height (void)
{
  return g_settings_get_uint (prefs.window_settings, "window-height");
}

void
marker_prefs_set_window_height (guint height)
{
  g_settings_set_uint (prefs.window_settings, "window-height", height);
}

void
marker_prefs_get_window_position (gint *pos_x,
                                  gint *pos_y)
{
  g_settings_get (prefs.window_settings, "window-position", "(ii)", pos_x, pos_y);
}

void
marker_prefs_set_window_position (gint pos_x,
                                  gint pos_y)
{
  g_settings_set (prefs.window_settings, "window-position", "(ii)", pos_x, pos_y);
}

guint
marker_prefs_get_editor_pane_width (void)
{
  return g_settings_get_uint (prefs.window_settings, "editor-pane-width");
}

void
marker_prefs_set_editor_pane_width (guint width)
{
  g_settings_set_uint (prefs.window_settings, "editor-pane-width", width);
}

gboolean
marker_prefs_get_show_sidebar (void)
{
  return g_settings_get_boolean (prefs.window_settings, "show-sidebar");
}

void
marker_prefs_set_show_sidebar (gboolean state)
{
  g_settings_set_boolean (prefs.window_settings, "show-sidebar", state);
}

gboolean
marker_prefs_get_use_syntax_theme (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "enable-syntax-theme");
}

void
marker_prefs_set_use_syntax_theme (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "enable-syntax-theme", state);
}

char *
marker_prefs_get_css_theme (void)
{
  return g_settings_get_string (prefs.preview_settings, "css-theme");
}

void
marker_prefs_set_css_theme (const char *theme)
{
  g_settings_set_string (prefs.preview_settings, "css-theme", theme);
}

gboolean
marker_prefs_get_use_css_theme (void)
{
  return g_settings_get_boolean (prefs.preview_settings, "css-toggle");
}

void
marker_prefs_set_use_css_theme (gboolean state)
{
  g_settings_set_boolean (prefs.preview_settings, "css-toggle", state);
}

char *
marker_prefs_get_highlight_theme (void)
{
  return g_settings_get_string (prefs.preview_settings, "highlight-theme");
}

void
marker_prefs_set_highlight_theme (const char *theme)
{
  g_settings_set_string (prefs.preview_settings, "highlight-theme", theme);
}

gboolean
marker_prefs_get_use_mathjs (void)
{
  return g_settings_get_boolean (prefs.preview_settings, "mathjs-toggle");
}

void
marker_prefs_set_use_mathjs (gboolean state)
{
  g_settings_set_boolean (prefs.preview_settings, "mathjs-toggle", state);
}

gdouble
marker_prefs_get_zoom_level (void)
{
  return g_settings_get_double (prefs.preview_settings, "preview-zoom-level");
}

void
marker_prefs_set_zoom_level (gdouble val)
{
  g_settings_set_double (prefs.preview_settings, "preview-zoom-level", val);
}

gdouble
marker_prefs_get_source_zoom_level (void)
{
  return g_settings_get_double (prefs.preview_settings, "source-zoom-level");
}

void
marker_prefs_set_source_zoom_level (gdouble val)
{
  g_settings_set_double (prefs.preview_settings, "source-zoom-level", val);
}

gboolean
marker_prefs_get_use_mermaid (void)
{
  return g_settings_get_boolean (prefs.preview_settings, "mermaid-toggle");
}

void
marker_prefs_set_use_mermaid (gboolean state)
{
  g_settings_set_boolean (prefs.preview_settings, "mermaid-toggle", state);
}

gboolean
marker_prefs_get_use_charter (void)
{
  return g_settings_get_boolean (prefs.preview_settings, "charter-toggle");
}

void
marker_prefs_set_use_charter (gboolean state)
{
  g_settings_set_boolean (prefs.preview_settings, "charter-toggle", state);
}

gboolean
marker_prefs_get_use_highlight (void)
{
  return g_settings_get_boolean (prefs.preview_settings, "highlight-toggle");
}

void
marker_prefs_set_use_highlight (gboolean state)
{
  g_settings_set_boolean (prefs.preview_settings, "highlight-toggle", state);
}

char *
marker_prefs_get_syntax_theme (void)
{
  return g_settings_get_string (prefs.editor_settings, "syntax-theme");
}

void
marker_prefs_set_syntax_theme (const char *theme)
{
  g_settings_set_string (prefs.editor_settings, "syntax-theme", theme);
}

guint
marker_prefs_get_right_margin_position (void)
{
  return g_settings_get_uint (prefs.editor_settings, "show-right-margin-position");
}

void
marker_prefs_set_right_margin_position (guint position)
{
  g_settings_set_uint (prefs.editor_settings, "show-right-margin-position", position);
}

gboolean
marker_prefs_get_replace_tabs (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "replace-tabs");
}

void
marker_prefs_set_replace_tabs (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "replace-tabs", state);
}

guint
marker_prefs_get_tab_width (void)
{
  return g_settings_get_uint (prefs.editor_settings, "tab-width");
}

void
marker_prefs_set_tab_width (guint width)
{
  g_settings_set_uint (prefs.editor_settings, "tab-width", width);
}

gboolean
marker_prefs_get_auto_indent (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "auto-indent");
}

void
marker_prefs_set_auto_indent (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "auto-indent", state);
}

gboolean
marker_prefs_get_trim_trailing_whitespace (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "trim-trailing-whitespace");
}

void
marker_prefs_set_trim_trailing_whitespace (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "trim-trailing-whitespace", state);
}

guint
marker_prefs_get_editor_font_size (void)
{
  return g_settings_get_uint (prefs.editor_settings, "font-size");
}

void
marker_prefs_set_editor_font_size (guint size)
{
  g_settings_set_uint (prefs.editor_settings, "font-size", size);
}

gboolean
marker_prefs_get_show_spaces (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "show-spaces");
}

void
marker_prefs_set_show_spaces (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "show-spaces", state);
}

gboolean
marker_prefs_get_spell_check (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "spell-check");
}

void
marker_prefs_set_spell_check (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "spell-check", state);
}

gchar *
marker_prefs_get_spell_check_language (void)
{
  return g_settings_get_string (prefs.editor_settings, "spell-check-lang");
}

void
marker_prefs_set_spell_check_language (const gchar *lang)
{
  g_settings_set_string (prefs.editor_settings, "spell-check-lang", lang);
}

gboolean
marker_prefs_get_show_line_numbers (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "show-line-numbers");
}

void
marker_prefs_set_show_line_numbers (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "show-line-numbers", state);
}

gboolean
marker_prefs_get_highlight_current_line (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "highlight-current-line");
}

void
marker_prefs_set_highlight_current_line (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "highlight-current-line", state);
}

gboolean
marker_prefs_get_wrap_text (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "wrap-text");
}

void
marker_prefs_set_wrap_text (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "wrap-text", state);
}

gboolean
marker_prefs_get_show_right_margin (void)
{
  return g_settings_get_boolean (prefs.editor_settings, "show-right-margin");
}

void
marker_prefs_set_show_right_margin (gboolean state)
{
  g_settings_set_boolean (prefs.editor_settings, "show-right-margin", state);
}

MarkerViewMode
marker_prefs_get_default_view_mode (void)
{
  return g_settings_get_enum (prefs.window_settings, "view-mode");
}

void
marker_prefs_set_default_view_mode (MarkerViewMode view_mode)
{
  g_settings_set_enum (prefs.window_settings, "view-mode", view_mode);
}

MarkerMathBackEnd
marker_prefs_get_math_backend (void)
{
  return g_settings_get_enum (prefs.preview_settings, "math-backend");
}

void
marker_prefs_set_math_backend (MarkerMathBackEnd backend)
{
  g_settings_set_enum (prefs.preview_settings, "math-backend", backend);
}

gboolean
marker_prefs_get_gnome_appmenu (void)
{
  return FALSE;
}

void
marker_prefs_set_gnome_appmenu (gboolean state)
{
  (void) state;
}

GList *
marker_prefs_get_available_stylesheets (void)
{
  GList *list = NULL;
  DIR *dir;
  struct dirent *ent;
  const gchar *dirs[] = {
    marker_utils_get_user_styles_dir (),
    marker_utils_get_styles_dir (),
    NULL
  };

  for (gsize i = 0; dirs[i] != NULL; i++)
  {
    if ((dir = opendir (dirs[i])) != NULL)
    {
      while ((ent = readdir (dir)) != NULL)
      {
        char *filename = ent->d_name;
        if (marker_string_ends_with (filename, ".css"))
        {
          if (!g_list_find_custom (list, filename, (GCompareFunc) g_strcmp0))
          {
            list = g_list_prepend (list, marker_string_alloc (filename));
          }
        }
      }
      closedir (dir);
    }
  }

  return g_list_sort (list, (GCompareFunc) g_strcmp0);
}

GList *
marker_prefs_get_available_highlight_themes (void)
{
  GList *list = NULL;
  DIR *dir;
  struct dirent *ent;
  const gchar *highlight_dir = marker_utils_get_highlight_styles_dir ();

  if ((dir = opendir (highlight_dir)) != NULL)
  {
    while ((ent = readdir (dir)) != NULL)
    {
      char *filename = ent->d_name;
      if (marker_string_ends_with (filename, ".css"))
      {
        list = g_list_prepend (list, marker_string_filename_get_name_noext (filename));
      }
    }
    closedir (dir);
  }

  return g_list_reverse (list);
}

GList *
marker_prefs_get_available_languages (void)
{
  GList *list = NULL;
  list = g_list_append (list, g_strdup ("en_US"));
  return list;
}

GList *
marker_prefs_get_available_syntax_themes (void)
{
  GList *list = NULL;
  GtkSourceStyleSchemeManager *style_manager =
    gtk_source_style_scheme_manager_get_default ();
  const gchar * const * ids =
    gtk_source_style_scheme_manager_get_scheme_ids (style_manager);

  if (ids)
  {
    for (int i = 0; ids[i] != NULL; ++i)
    {
      list = g_list_prepend (list, marker_string_alloc (ids[i]));
    }
  }

  return g_list_reverse (list);
}

static void
update_editors (void)
{
  GtkApplication *app = marker_get_app ();
  if (!app)
    return;

  GList *windows = gtk_application_get_windows (app);
  for (GList *item = windows; item != NULL; item = item->next)
  {
    if (MARKER_IS_WINDOW (item->data))
    {
      MarkerWindow *window = item->data;
      MarkerEditor *editor = marker_window_get_active_editor (window);
      if (editor)
        marker_editor_apply_prefs (editor);
    }
  }
}

static void
refresh_preview (void)
{
  GtkApplication *app = marker_get_app ();
  if (!app)
    return;

  GList *windows = gtk_application_get_windows (app);
  for (GList *item = windows; item != NULL; item = item->next)
  {
    if (MARKER_IS_WINDOW (item->data))
    {
      MarkerWindow *window = item->data;
      marker_window_refresh_all_preview (window);
    }
  }
}

static void
on_editor_settings_changed (GSettings   *settings,
                            const gchar *key,
                            gpointer     user_data)
{
  (void) settings;
  (void) key;
  (void) user_data;
  update_editors ();
}

static void
on_preview_settings_changed (GSettings   *settings,
                             const gchar *key,
                             gpointer     user_data)
{
  (void) settings;
  (void) key;
  (void) user_data;
  refresh_preview ();
}

static void
on_window_settings_changed (GSettings   *settings,
                            const gchar *key,
                            gpointer     user_data)
{
  (void) settings;
  (void) user_data;
  if (g_strcmp0 (key, "enable-dark-mode") == 0)
  {
    gboolean dark = marker_prefs_get_use_dark_theme ();
    AdwStyleManager *style_manager = adw_style_manager_get_default ();
    adw_style_manager_set_color_scheme (
      style_manager,
      dark ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT);
  }
}

static gboolean
uint_to_double_mapping (GValue   *value,
                        GVariant *variant,
                        gpointer  user_data)
{
  (void) user_data;
  g_value_set_double (value, (gdouble) g_variant_get_uint32 (variant));
  return TRUE;
}

static GVariant *
double_to_uint_mapping (const GValue       *value,
                        const GVariantType *expected_type,
                        gpointer            user_data)
{
  (void) expected_type;
  (void) user_data;
  return g_variant_new_uint32 ((guint32) g_value_get_double (value));
}

static GtkStringList *
create_string_list_from_glist (GList *list)
{
  GtkStringList *string_list = gtk_string_list_new (NULL);
  for (GList *l = list; l != NULL; l = l->next)
  {
    if (l->data)
      gtk_string_list_append (string_list, (const char *) l->data);
  }
  return string_list;
}

static guint
find_string_in_model (GListModel *model, const char *target)
{
  if (!target)
    return 0;

  guint n_items = g_list_model_get_n_items (model);
  for (guint i = 0; i < n_items; i++)
  {
    const char *str = gtk_string_list_get_string (GTK_STRING_LIST (model), i);
    if (str && g_strcmp0 (str, target) == 0)
      return i;
  }
  return 0;
}

static void
on_syntax_theme_selected (AdwComboRow *row,
                          GParamSpec  *pspec,
                          gpointer     user_data)
{
  (void) pspec;
  (void) user_data;
  guint selected = adw_combo_row_get_selected (row);
  GListModel *model = adw_combo_row_get_model (row);
  if (model && selected != GTK_INVALID_LIST_POSITION)
  {
    const char *str = gtk_string_list_get_string (GTK_STRING_LIST (model), selected);
    if (str)
      marker_prefs_set_syntax_theme (str);
  }
}

static void
on_spell_lang_selected (AdwComboRow *row,
                        GParamSpec  *pspec,
                        gpointer     user_data)
{
  (void) pspec;
  (void) user_data;
  guint selected = adw_combo_row_get_selected (row);
  GListModel *model = adw_combo_row_get_model (row);
  if (model && selected != GTK_INVALID_LIST_POSITION)
  {
    const char *str = gtk_string_list_get_string (GTK_STRING_LIST (model), selected);
    if (str)
      marker_prefs_set_spell_check_language (str);
  }
}

static void
on_css_theme_selected (AdwComboRow *row,
                       GParamSpec  *pspec,
                       gpointer     user_data)
{
  (void) pspec;
  (void) user_data;
  guint selected = adw_combo_row_get_selected (row);
  GListModel *model = adw_combo_row_get_model (row);
  if (model && selected != GTK_INVALID_LIST_POSITION)
  {
    const char *str = gtk_string_list_get_string (GTK_STRING_LIST (model), selected);
    if (str)
      marker_prefs_set_css_theme (str);
  }
}

static void
on_code_highlight_theme_selected (AdwComboRow *row,
                                  GParamSpec  *pspec,
                                  gpointer     user_data)
{
  (void) pspec;
  (void) user_data;
  guint selected = adw_combo_row_get_selected (row);
  GListModel *model = adw_combo_row_get_model (row);
  if (model && selected != GTK_INVALID_LIST_POSITION)
  {
    const char *str = gtk_string_list_get_string (GTK_STRING_LIST (model), selected);
    if (str)
      marker_prefs_set_highlight_theme (str);
  }
}

static void
on_math_backend_selected (AdwComboRow *row,
                          GParamSpec  *pspec,
                          gpointer     user_data)
{
  (void) pspec;
  (void) user_data;
  guint selected = adw_combo_row_get_selected (row);
  if (selected != GTK_INVALID_LIST_POSITION)
    marker_prefs_set_math_backend ((MarkerMathBackEnd) selected);
}

static void
on_view_mode_selected (AdwComboRow *row,
                       GParamSpec  *pspec,
                       gpointer     user_data)
{
  (void) pspec;
  (void) user_data;
  guint selected = adw_combo_row_get_selected (row);
  if (selected != GTK_INVALID_LIST_POSITION)
    marker_prefs_set_default_view_mode ((MarkerViewMode) selected);
}

void
marker_prefs_load (void)
{
  prefs.editor_settings =
    g_settings_new ("com.github.thenathattify.marker.preferences.editor");
  prefs.preview_settings =
    g_settings_new ("com.github.thenathattify.marker.preferences.preview");
  prefs.window_settings =
    g_settings_new ("com.github.thenathattify.marker.preferences.window");

  g_signal_connect (prefs.editor_settings, "changed",
                    G_CALLBACK (on_editor_settings_changed), NULL);
  g_signal_connect (prefs.preview_settings, "changed",
                    G_CALLBACK (on_preview_settings_changed), NULL);
  g_signal_connect (prefs.window_settings, "changed",
                    G_CALLBACK (on_window_settings_changed), NULL);

  gboolean dark = marker_prefs_get_use_dark_theme ();
  AdwStyleManager *style_manager = adw_style_manager_get_default ();
  adw_style_manager_set_color_scheme (
    style_manager,
    dark ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT);
}

void
marker_prefs_show_window (void)
{
  GtkBuilder *builder =
    gtk_builder_new_from_resource (
      "/com/github/thenathattify/marker/ui/marker-prefs-window.ui");

  AdwPreferencesWindow *prefs_win =
    ADW_PREFERENCES_WINDOW (gtk_builder_get_object (builder, "prefs_win"));

  AdwSwitchRow *show_line_numbers_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "show_line_numbers_row"));
  AdwSwitchRow *highlight_current_line_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "highlight_current_line_row"));
  AdwSwitchRow *wrap_text_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "wrap_text_row"));
  AdwSwitchRow *show_spaces_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "show_spaces_row"));
  AdwSwitchRow *show_right_margin_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "show_right_margin_row"));
  AdwSpinRow *right_margin_position_row =
    ADW_SPIN_ROW (gtk_builder_get_object (builder, "right_margin_position_row"));
  AdwSwitchRow *syntax_highlight_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "syntax_highlight_row"));
  AdwComboRow *syntax_theme_row =
    ADW_COMBO_ROW (gtk_builder_get_object (builder, "syntax_theme_row"));

  AdwSwitchRow *auto_indent_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "auto_indent_row"));
  AdwSwitchRow *replace_tabs_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "replace_tabs_row"));
  AdwSpinRow *tab_width_row =
    ADW_SPIN_ROW (gtk_builder_get_object (builder, "tab_width_row"));
  AdwSwitchRow *trim_trailing_whitespace_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "trim_trailing_whitespace_row"));
  AdwSpinRow *editor_font_size_row =
    ADW_SPIN_ROW (gtk_builder_get_object (builder, "editor_font_size_row"));

  AdwSwitchRow *spell_check_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "spell_check_row"));
  AdwComboRow *spell_lang_row =
    ADW_COMBO_ROW (gtk_builder_get_object (builder, "spell_lang_row"));

  AdwSwitchRow *css_theme_toggle_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "css_theme_toggle_row"));
  AdwComboRow *css_theme_row =
    ADW_COMBO_ROW (gtk_builder_get_object (builder, "css_theme_row"));
  AdwSwitchRow *code_highlight_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "code_highlight_row"));
  AdwComboRow *code_highlight_theme_row =
    ADW_COMBO_ROW (gtk_builder_get_object (builder, "code_highlight_theme_row"));

  AdwSwitchRow *mathjs_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "mathjs_row"));
  AdwComboRow *math_backend_row =
    ADW_COMBO_ROW (gtk_builder_get_object (builder, "math_backend_row"));
  AdwSwitchRow *mermaid_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "mermaid_row"));
  AdwSwitchRow *charter_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "charter_row"));

  AdwComboRow *view_mode_row =
    ADW_COMBO_ROW (gtk_builder_get_object (builder, "view_mode_row"));
  AdwSwitchRow *dark_mode_row =
    ADW_SWITCH_ROW (gtk_builder_get_object (builder, "dark_mode_row"));

  /* GSettings bindings for Editor page */
  g_settings_bind (prefs.editor_settings, "show-line-numbers",
                   show_line_numbers_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.editor_settings, "highlight-current-line",
                   highlight_current_line_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.editor_settings, "wrap-text",
                   wrap_text_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.editor_settings, "show-spaces",
                   show_spaces_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.editor_settings, "show-right-margin",
                   show_right_margin_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind_with_mapping (prefs.editor_settings, "show-right-margin-position",
                                right_margin_position_row, "value",
                                G_SETTINGS_BIND_DEFAULT,
                                uint_to_double_mapping, double_to_uint_mapping,
                                NULL, NULL);
  g_settings_bind (prefs.editor_settings, "enable-syntax-theme",
                   syntax_highlight_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.editor_settings, "auto-indent",
                   auto_indent_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.editor_settings, "replace-tabs",
                   replace_tabs_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind_with_mapping (prefs.editor_settings, "tab-width",
                                tab_width_row, "value",
                                G_SETTINGS_BIND_DEFAULT,
                                uint_to_double_mapping, double_to_uint_mapping,
                                NULL, NULL);
  g_settings_bind (prefs.editor_settings, "trim-trailing-whitespace",
                   trim_trailing_whitespace_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind_with_mapping (prefs.editor_settings, "font-size",
                                editor_font_size_row, "value",
                                G_SETTINGS_BIND_DEFAULT,
                                uint_to_double_mapping, double_to_uint_mapping,
                                NULL, NULL);
  g_settings_bind (prefs.editor_settings, "spell-check",
                   spell_check_row, "active", G_SETTINGS_BIND_DEFAULT);

  /* GSettings bindings for Preview page */
  g_settings_bind (prefs.preview_settings, "css-toggle",
                   css_theme_toggle_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.preview_settings, "highlight-toggle",
                   code_highlight_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.preview_settings, "mathjs-toggle",
                   mathjs_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.preview_settings, "mermaid-toggle",
                   mermaid_row, "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (prefs.preview_settings, "charter-toggle",
                   charter_row, "active", G_SETTINGS_BIND_DEFAULT);

  /* GSettings bindings for Window page */
  g_settings_bind (prefs.window_settings, "enable-dark-mode",
                   dark_mode_row, "active", G_SETTINGS_BIND_DEFAULT);

  /* Sensitivity bindings */
  g_object_bind_property (show_right_margin_row, "active",
                          right_margin_position_row, "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (syntax_highlight_row, "active",
                          syntax_theme_row, "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (spell_check_row, "active",
                          spell_lang_row, "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (css_theme_toggle_row, "active",
                          css_theme_row, "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (code_highlight_row, "active",
                          code_highlight_theme_row, "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (mathjs_row, "active",
                          math_backend_row, "sensitive",
                          G_BINDING_SYNC_CREATE);

  /* Populate ComboRows */
  /* Syntax Theme */
  GList *syntax_list = marker_prefs_get_available_syntax_themes ();
  GtkStringList *syntax_model = create_string_list_from_glist (syntax_list);
  adw_combo_row_set_model (syntax_theme_row, G_LIST_MODEL (syntax_model));
  g_object_unref (syntax_model);
  char *syntax_theme = marker_prefs_get_syntax_theme ();
  adw_combo_row_set_selected (syntax_theme_row,
                              find_string_in_model (G_LIST_MODEL (syntax_model), syntax_theme));
  g_free (syntax_theme);
  g_list_free_full (syntax_list, g_free);
  g_signal_connect (syntax_theme_row, "notify::selected",
                    G_CALLBACK (on_syntax_theme_selected), NULL);

  /* Spell Language */
  GList *lang_list = marker_prefs_get_available_languages ();
  GtkStringList *lang_model = create_string_list_from_glist (lang_list);
  adw_combo_row_set_model (spell_lang_row, G_LIST_MODEL (lang_model));
  g_object_unref (lang_model);
  char *lang = marker_prefs_get_spell_check_language ();
  adw_combo_row_set_selected (spell_lang_row,
                              find_string_in_model (G_LIST_MODEL (lang_model), lang));
  g_free (lang);
  g_list_free_full (lang_list, g_free);
  g_signal_connect (spell_lang_row, "notify::selected",
                    G_CALLBACK (on_spell_lang_selected), NULL);

  /* CSS Theme */
  GList *css_list = marker_prefs_get_available_stylesheets ();
  GtkStringList *css_model = create_string_list_from_glist (css_list);
  adw_combo_row_set_model (css_theme_row, G_LIST_MODEL (css_model));
  g_object_unref (css_model);
  char *css_theme = marker_prefs_get_css_theme ();
  char *css_filename = marker_string_filename_get_name (css_theme);
  adw_combo_row_set_selected (css_theme_row,
                              find_string_in_model (G_LIST_MODEL (css_model),
                                                    css_filename ? css_filename : css_theme));
  g_free (css_filename);
  g_free (css_theme);
  g_list_free_full (css_list, g_free);
  g_signal_connect (css_theme_row, "notify::selected",
                    G_CALLBACK (on_css_theme_selected), NULL);

  /* Highlight Theme */
  GList *hl_list = marker_prefs_get_available_highlight_themes ();
  GtkStringList *hl_model = create_string_list_from_glist (hl_list);
  adw_combo_row_set_model (code_highlight_theme_row, G_LIST_MODEL (hl_model));
  g_object_unref (hl_model);
  char *hl_theme = marker_prefs_get_highlight_theme ();
  adw_combo_row_set_selected (code_highlight_theme_row,
                              find_string_in_model (G_LIST_MODEL (hl_model), hl_theme));
  g_free (hl_theme);
  g_list_free_full (hl_list, g_free);
  g_signal_connect (code_highlight_theme_row, "notify::selected",
                    G_CALLBACK (on_code_highlight_theme_selected), NULL);

  /* Math Backend */
  const char * const math_backends[] = { "KaTeX", "MathJax", NULL };
  GtkStringList *math_model = gtk_string_list_new (math_backends);
  adw_combo_row_set_model (math_backend_row, G_LIST_MODEL (math_model));
  g_object_unref (math_model);
  adw_combo_row_set_selected (math_backend_row,
                              (guint) marker_prefs_get_math_backend ());
  g_signal_connect (math_backend_row, "notify::selected",
                    G_CALLBACK (on_math_backend_selected), NULL);

  /* View Mode */
  const char * const view_modes[] = {
    _("Editor Only"),
    _("Preview Only"),
    _("Dual Pane"),
    _("Dual Window"),
    NULL
  };
  GtkStringList *view_mode_model = gtk_string_list_new (view_modes);
  adw_combo_row_set_model (view_mode_row, G_LIST_MODEL (view_mode_model));
  g_object_unref (view_mode_model);
  adw_combo_row_set_selected (view_mode_row,
                              (guint) marker_prefs_get_default_view_mode ());
  g_signal_connect (view_mode_row, "notify::selected",
                    G_CALLBACK (on_view_mode_selected), NULL);

  GtkApplication *app = marker_get_app ();
  GtkWindow *active_win = app ? gtk_application_get_active_window (app) : NULL;
  if (active_win)
    gtk_window_set_transient_for (GTK_WINDOW (prefs_win), active_win);

  gtk_window_present (GTK_WINDOW (prefs_win));

  g_object_unref (builder);
}
