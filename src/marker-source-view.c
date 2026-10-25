/*
 * marker-source-view.c
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

#include <string.h>
#include <stdlib.h>

#include "marker-source-view.h"
#include "marker-prefs.h"
#include "marker-utils.h"

#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

struct _MarkerSourceView
{
  GtkSourceView           parent_instance;
  GSettings              *settings;
  GtkSourceSearchContext *search_context;
};

G_DEFINE_TYPE (MarkerSourceView, marker_source_view, GTK_SOURCE_TYPE_VIEW)

void
marker_source_view_surround_selection_with (MarkerSourceView *source_view,
                                            const char       *insertion)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  GtkTextIter start, end;
  gboolean selected;
  size_t len = strlen (insertion);

  gtk_text_buffer_begin_user_action (buffer);

  selected = gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

  if (selected)
  {
    GtkTextMark *end_mark = gtk_text_buffer_create_mark (buffer, NULL, &end, TRUE);

    gtk_text_buffer_insert (buffer, &start, insertion, len);
    gtk_text_buffer_get_iter_at_mark (buffer, &end, end_mark);
    gtk_text_buffer_insert (buffer, &end, insertion, len);

    gtk_text_buffer_delete_mark (buffer, end_mark);
  }
  else
  {
    gtk_text_buffer_insert (buffer, &start, insertion, len);
    gtk_text_buffer_insert (buffer, &start, insertion, len);
    gtk_text_iter_backward_chars (&start, len);
    gtk_text_buffer_place_cursor (buffer, &start);
  }

  gtk_text_buffer_end_user_action (buffer);
}

void
marker_source_view_insert_link (MarkerSourceView *source_view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  GtkTextIter start, end;
  gboolean selected;

  gtk_text_buffer_begin_user_action (buffer);

  selected = gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (selected)
  {
    GtkTextMark *end_mark = gtk_text_buffer_create_mark (buffer, NULL, &end, TRUE);
    gchar *selected_text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

    if (!marker_utils_is_url (selected_text))
    {
      gtk_text_buffer_insert (buffer, &start, "[", 1);
      gtk_text_buffer_get_iter_at_mark (buffer, &end, end_mark);
      gtk_text_buffer_insert (buffer, &end, "]()", 3);
      gtk_text_iter_backward_chars (&end, 1);
      gtk_text_buffer_place_cursor (buffer, &end);
    }
    else
    {
      GtkTextMark *start_mark = gtk_text_buffer_create_mark (buffer, NULL, &start, TRUE);
      gtk_text_buffer_insert (buffer, &start, "[](", 3);
      gtk_text_buffer_get_iter_at_mark (buffer, &end, end_mark);
      gtk_text_buffer_insert (buffer, &end, ")", 1);
      gtk_text_buffer_get_iter_at_mark (buffer, &start, start_mark);
      gtk_text_iter_forward_chars (&start, 1);
      gtk_text_buffer_place_cursor (buffer, &start);
      gtk_text_buffer_delete_mark (buffer, start_mark);
    }
    gtk_text_buffer_delete_mark (buffer, end_mark);
    g_free (selected_text);
  }
  else
  {
    const gchar *link = "[]()";
    GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &start, mark);

    gtk_text_buffer_insert (buffer, &start, link, 4);
    gtk_text_iter_backward_chars (&start, 3);
    gtk_text_buffer_place_cursor (buffer, &start);
  }

  gtk_text_buffer_end_user_action (buffer);
}

void
marker_source_view_insert_image (MarkerSourceView *source_view,
                                 const char       *image_path)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  GtkTextIter start;

  gtk_text_buffer_get_iter_at_mark (buffer, &start, gtk_text_buffer_get_insert (buffer));

  gchar *img = g_strdup_printf ("![](%s)", image_path);
  size_t len = strlen (img);

  gtk_text_buffer_insert (buffer, &start, img, len);
  g_free (img);
}

void
marker_source_view_set_spell_check (MarkerSourceView *source_view G_GNUC_UNUSED,
                                    gboolean          state G_GNUC_UNUSED)
{
  /* Stub for spell checker compatibility */
}

void
marker_source_view_set_spell_check_lang (MarkerSourceView *source_view G_GNUC_UNUSED,
                                         const gchar      *lang G_GNUC_UNUSED)
{
  /* Stub for spell checker compatibility */
}

void
marker_source_view_set_syntax_theme (MarkerSourceView *source_view,
                                     const char       *theme)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  GtkSourceStyleSchemeManager *style_manager =
    gtk_source_style_scheme_manager_get_default ();
  GtkSourceStyleScheme *scheme =
    gtk_source_style_scheme_manager_get_scheme (style_manager, theme);
  gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (buffer), scheme);
}

gboolean
marker_source_view_get_modified (MarkerSourceView *source_view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  return gtk_text_buffer_get_modified (buffer);
}

void
marker_source_view_set_modified (MarkerSourceView *source_view,
                                 gboolean          modified)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  gtk_text_buffer_set_modified (buffer, modified);
}

gchar *
marker_source_view_get_text (MarkerSourceView *source_view,
                             gboolean          include_position)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);
  if (include_position && !gtk_text_iter_equal (&start, &end))
  {
    gchar *identifier = g_strdup ("<span id=\"cursor_pos\"></span>");
    GtkTextIter pos;
    gtk_text_buffer_get_selection_bounds (buffer, &pos, NULL);
    gchar *beginning = gtk_text_buffer_get_text (buffer, &start, &pos, FALSE);
    gchar *ending = gtk_text_buffer_get_text (buffer, &pos, &end, FALSE);
    gchar *res = g_strconcat (beginning, identifier, ending, NULL);
    g_free (identifier);
    g_free (beginning);
    g_free (ending);
    return res;
  }
  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

int
marker_source_view_get_cursor_position (MarkerSourceView *source_view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  GtkTextIter pos;
  gtk_text_buffer_get_selection_bounds (buffer, &pos, NULL);
  return gtk_text_iter_get_offset (&pos);
}

void
marker_source_view_set_text (MarkerSourceView *source_view,
                             const char       *text,
                             size_t            size)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
  gtk_text_buffer_set_text (buffer, text, size);

  /* gtk_text_buffer_set_text() leaves the insert mark at the END of the
   * inserted text, so opening a document put the cursor at the bottom
   * and the preview (which scrolls to the #cursor_pos anchor on every
   * refresh) started at the end of the document.
   * Place the cursor at the start instead. */
  GtkTextIter start_iter;
  gtk_text_buffer_get_start_iter (buffer, &start_iter);
  gtk_text_buffer_place_cursor (buffer, &start_iter);
}

void
marker_source_view_set_language (MarkerSourceView *source_view,
                                 const gchar      *language_name)
{
  if (GTK_SOURCE_IS_VIEW (source_view))
  {
    GtkSourceLanguageManager *manager =
      gtk_source_language_manager_get_default ();

    GtkSourceLanguage *language =
      gtk_source_language_manager_get_language (manager, language_name);

    GtkSourceBuffer *buffer =
      GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view)));

    gtk_source_buffer_set_language (buffer, language);
  }
}

void
marker_source_view_set_zoom_level (MarkerSourceView *source_view,
                                   gdouble           zoom)
{
  int percent = (int) (zoom * 100.0 + 0.5);
  if (percent < 20)
    percent = 20;
  if (percent > 500)
    percent = 500;

  /* Per-view provider (the editor and preview zooms are independent). */
  GtkCssProvider *provider = g_object_get_data (G_OBJECT (source_view), "zoom-provider");
  if (provider == NULL)
  {
    provider = gtk_css_provider_new ();
    gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (source_view)),
                                    GTK_STYLE_PROVIDER (provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_set_data_full (G_OBJECT (source_view), "zoom-provider", provider, g_object_unref);
  }

  /* Combine the user-configurable base font size (pt) with the zoom percentage. */
  guint base_pt = marker_prefs_get_editor_font_size ();
  double font_pt = base_pt * percent / 100.0;
  g_autofree gchar *css = g_strdup_printf (
    "textview.marker-source-view, textview.marker-source-view text { font-size: %.1fpt; }",
    font_pt);
  gtk_css_provider_load_from_string (provider, css);

  g_object_set_data (G_OBJECT (source_view), "zoom-percent", GINT_TO_POINTER (percent));

  g_signal_emit_by_name (source_view, "zoom-changed");
}

gdouble
marker_source_view_get_zoom_level (MarkerSourceView *source_view)
{
  int percent = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (source_view), "zoom-percent"));
  if (percent <= 0)
    return marker_prefs_get_source_zoom_level ();
  return percent / 100.0;
}

static void
marker_source_view_init (MarkerSourceView *source_view)
{
  GtkSourceSearchContext *search_context =
    gtk_source_search_context_new (GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view))), NULL);
  source_view->search_context = search_context;

  marker_source_view_set_language (source_view, "markdown");
  gtk_widget_add_css_class (GTK_WIDGET (source_view), "monospace");
  gtk_widget_add_css_class (GTK_WIDGET (source_view), "marker-source-view");

  marker_source_view_set_zoom_level (source_view, marker_prefs_get_source_zoom_level ());

  gtk_source_view_set_insert_spaces_instead_of_tabs (GTK_SOURCE_VIEW (source_view), marker_prefs_get_replace_tabs ());
  gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (source_view), marker_prefs_get_tab_width ());
  gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (source_view), marker_prefs_get_auto_indent ());

  GMenu *menu = g_menu_new ();
  g_menu_append (menu, _("_Reload from Disk"), "win.reload");
  gtk_text_view_set_extra_menu (GTK_TEXT_VIEW (source_view), G_MENU_MODEL (menu));
  g_object_unref (menu);
}

static void
marker_source_view_class_init (MarkerSourceViewClass *class)
{
  g_signal_new ("zoom-changed",
                G_TYPE_FROM_CLASS (class),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
}

MarkerSourceView *
marker_source_view_new (void)
{
  return g_object_new (MARKER_TYPE_SOURCE_VIEW, NULL);
}

GtkSourceSearchContext *
marker_source_get_search_context (MarkerSourceView *source_view)
{
  return source_view->search_context;
}
