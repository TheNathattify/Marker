/*
 * marker-utils.c
 *
 * Copyright (C) 2017 - 2018 Fabio Colacio
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

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "marker-utils.h"

#include "scidown/src/autolink.h"

gchar *
marker_utils_read_file (const gchar *infile,
                        long        *out_size)
{
  g_return_val_if_fail (infile != NULL, NULL);

  gchar *contents = NULL;
  gsize length = 0;
  GError *error = NULL;

  if (out_size != NULL)
    *out_size = 0;

  if (!g_file_get_contents (infile, &contents, &length, &error))
  {
    g_warning ("Failed to read file '%s': %s", infile, error ? error->message : "unknown error");
    g_clear_error (&error);
    return NULL;
  }

  if (out_size != NULL)
    *out_size = (long) length;

  return contents;
}

void
marker_utils_surround_selection_with (GtkTextBuffer *buffer,
                                      const gchar   *insertion)
{
  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (insertion != NULL);

  GtkTextIter start, end;
  gint start_index, end_index, selection_len;
  gboolean selected;
  gsize len = strlen (insertion);

  selected = gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (selected)
  {
    start_index = gtk_text_iter_get_line_offset (&start);
    end_index = gtk_text_iter_get_line_offset (&end);
    selection_len = end_index - start_index;

    gtk_text_buffer_insert (buffer, &start, insertion, len);
    gtk_text_iter_forward_chars (&start, selection_len);
    gtk_text_buffer_insert (buffer, &start, insertion, len);
  }
}

gchar *
marker_utils_escape_file_path (const gchar *filename)
{
  if (filename == NULL)
    return NULL;

  GString *escaped = g_string_sized_new (strlen (filename) * 2 + 1);
  for (const gchar *p = filename; *p != '\0'; p++)
  {
    if (*p == ' ')
      g_string_append_c (escaped, '\\');
    g_string_append_c (escaped, *p);
  }

  return g_string_free (escaped, FALSE);
}

gchar *
marker_utils_combo_box_get_active_str (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter (combo_box, &iter))
  {
    GtkTreeModel *model = gtk_combo_box_get_model (combo_box);
    if (model != NULL)
    {
      gchar *str = NULL;
      gtk_tree_model_get (model, &iter, 0, &str, -1);
      return str;
    }
  }

  return NULL;
}

void
marker_utils_combo_box_set_model (GtkComboBox  *combo_box,
                                  GtkTreeModel *model)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_TREE_MODEL (model));

  gtk_combo_box_set_model (combo_box, model);
  GtkCellRenderer *cell_renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell_renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
                                  cell_renderer,
                                  "text", 0,
                                  NULL);
  gtk_combo_box_set_active (combo_box, 0);
}

int
marker_utils_rfind (char         query,
                    const gchar *str)
{
  if (str == NULL)
    return -1;

  const gchar *last = strrchr (str, query);
  if (last == NULL)
    return -1;

  return (int) (last - str);
}

int
marker_utils_get_current_time_seconds (void)
{
  time_t timer;
  struct tm y2k = {0};
  double seconds;

  y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
  y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

  time (&timer);

  seconds = difftime (timer, mktime (&y2k));

  return (int) seconds;
}

int
marker_utils_str_starts_with (const gchar *str,
                              const gchar *sub_str)
{
  if (str == NULL || sub_str == NULL)
    return 0;

  return g_str_has_prefix (str, sub_str) ? 1 : 0;
}

gchar *
marker_utils_allocate_string (const gchar *str)
{
  return g_strdup (str);
}

int
marker_utils_str_ends_with (const gchar *str,
                            const gchar *sub_str)
{
  if (str == NULL || sub_str == NULL)
    return 0;

  return g_str_has_suffix (str, sub_str) ? 1 : 0;
}

gboolean
marker_utils_is_url (const gchar *str)
{
  if (str == NULL)
    return FALSE;

  size_t len = strlen (str);
  if (len == 0)
    return FALSE;

  size_t rewind = 0;
  size_t res = hoedown_autolink__www (&rewind, NULL, (uint8_t *) str, 0, len, 0);

  if (res)
    return TRUE;

  int safe = hoedown_autolink_is_safe ((uint8_t *) str, len);
  if (safe)
  {
    rewind = 0;
    return hoedown_autolink__url (&rewind, NULL, (uint8_t *) str + safe, safe, len, 0) != 0;
  }

  const gchar *e = strchr (str, '@');
  if (e)
  {
    size_t indx = (size_t) (e - str);
    rewind = 0;
    return hoedown_autolink__email (&rewind, NULL, (uint8_t *) str, indx, len, 0) != 0;
  }

  return FALSE;
}

const gchar *
marker_utils_get_scripts_dir (void)
{
  static gchar *scripts_dir = NULL;
  if (scripts_dir != NULL)
    return scripts_dir;

  const gchar *env_dir = g_getenv ("MARKER_SCRIPTS_DIR");
  if (env_dir && g_file_test (env_dir, G_FILE_TEST_IS_DIR))
  {
    scripts_dir = g_str_has_suffix (env_dir, "/") ? g_strdup (env_dir) : g_strdup_printf ("%s/", env_dir);
    return scripts_dir;
  }

  const gchar *env_data = g_getenv ("MARKER_DATA_DIR");
  if (env_data)
  {
    gchar *candidate = g_build_filename (env_data, "scripts", NULL);
    if (g_file_test (candidate, G_FILE_TEST_IS_DIR))
    {
      scripts_dir = g_strdup_printf ("%s/", candidate);
      g_free (candidate);
      return scripts_dir;
    }
    g_free (candidate);
  }

  if (g_file_test (SCRIPTS_DIR, G_FILE_TEST_IS_DIR))
  {
    scripts_dir = g_strdup (SCRIPTS_DIR);
    return scripts_dir;
  }

  const gchar *candidates[] = {
    "./data/scripts",
    "../data/scripts",
    "/home/vincent/src/marker/data/scripts",
    "/app/share/com.github.thenathattify.marker/scripts",
    "/usr/share/com.github.thenathattify.marker/scripts",
    NULL
  };

  for (gsize i = 0; candidates[i] != NULL; i++)
  {
    if (g_file_test (candidates[i], G_FILE_TEST_IS_DIR))
    {
      g_autofree gchar *canonical = g_canonicalize_filename (candidates[i], NULL);
      scripts_dir = g_str_has_suffix (canonical, "/") ? g_strdup (canonical) : g_strdup_printf ("%s/", canonical);
      return scripts_dir;
    }
  }

  scripts_dir = g_strdup (SCRIPTS_DIR);
  return scripts_dir;
}

const gchar *
marker_utils_get_styles_dir (void)
{
  static gchar *styles_dir = NULL;
  if (styles_dir != NULL)
    return styles_dir;

  const gchar *env_dir = g_getenv ("MARKER_STYLES_DIR");
  if (env_dir && g_file_test (env_dir, G_FILE_TEST_IS_DIR))
  {
    styles_dir = g_str_has_suffix (env_dir, "/") ? g_strdup (env_dir) : g_strdup_printf ("%s/", env_dir);
    return styles_dir;
  }

  const gchar *env_data = g_getenv ("MARKER_DATA_DIR");
  if (env_data)
  {
    gchar *candidate = g_build_filename (env_data, "styles", NULL);
    if (g_file_test (candidate, G_FILE_TEST_IS_DIR))
    {
      g_autofree gchar *canonical = g_canonicalize_filename (candidate, NULL);
      styles_dir = g_str_has_suffix (canonical, "/") ? g_strdup (canonical) : g_strdup_printf ("%s/", canonical);
      g_free (candidate);
      return styles_dir;
    }
    g_free (candidate);
  }

  if (g_file_test (STYLES_DIR, G_FILE_TEST_IS_DIR))
  {
    styles_dir = g_strdup (STYLES_DIR);
    return styles_dir;
  }

  const gchar *candidates[] = {
    "./data/styles",
    "../data/styles",
    "/home/vincent/src/marker/data/styles",
    "/app/share/com.github.thenathattify.marker/styles",
    "/usr/share/com.github.thenathattify.marker/styles",
    NULL
  };

  for (gsize i = 0; candidates[i] != NULL; i++)
  {
    if (g_file_test (candidates[i], G_FILE_TEST_IS_DIR))
    {
      g_autofree gchar *canonical = g_canonicalize_filename (candidates[i], NULL);
      styles_dir = g_str_has_suffix (canonical, "/") ? g_strdup (canonical) : g_strdup_printf ("%s/", canonical);
      return styles_dir;
    }
  }

  styles_dir = g_strdup (STYLES_DIR);
  return styles_dir;
}

const gchar *
marker_utils_get_user_styles_dir (void)
{
  static gchar *user_styles_dir = NULL;
  if (user_styles_dir != NULL)
    return user_styles_dir;

  gchar *dir = g_build_filename (g_get_user_data_dir (), "marker", "styles", NULL);
  user_styles_dir = g_str_has_suffix (dir, "/") ? dir : g_strdup_printf ("%s/", dir);
  if (user_styles_dir != dir)
    g_free (dir);

  return user_styles_dir;
}

const gchar *
marker_utils_get_highlight_styles_dir (void)
{
  static gchar *highlight_styles_dir = NULL;
  if (highlight_styles_dir != NULL)
    return highlight_styles_dir;

  const gchar *scripts = marker_utils_get_scripts_dir ();
  gchar *candidate = g_build_filename (scripts, "highlight", "styles", NULL);
  if (g_file_test (candidate, G_FILE_TEST_IS_DIR))
  {
    highlight_styles_dir = g_strdup_printf ("%s/", candidate);
    g_free (candidate);
    return highlight_styles_dir;
  }
  g_free (candidate);

  highlight_styles_dir = g_strdup (HIGHLIGHT_STYLES_DIR);
  return highlight_styles_dir;
}
