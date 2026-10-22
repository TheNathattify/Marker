/*
 * marker-editor.c
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

#include <string.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "marker-prefs.h"
#include "marker-string.h"
#include "marker-editor.h"

struct _MarkerEditor
{
  GtkBox                 parent_instance;

  MarkerDocument        *document;
  MarkerSearchController *search_controller;

  GtkPaned              *paned;
  GtkBox                *vbox;
  MarkerPreview         *preview;
  MarkerSourceView      *source_view;
  GtkScrolledWindow     *source_scroll;
  MarkerViewMode         view_mode;

  gboolean               needs_refresh;
  guint                  timer_id;
};

G_DEFINE_TYPE (MarkerEditor, marker_editor, GTK_TYPE_BOX);

static void
emit_signal_title_changed (MarkerEditor *editor)
{
  g_autofree gchar *title = marker_editor_get_title (editor);
  g_autofree gchar *raw_title = marker_editor_get_raw_title (editor);
  g_signal_emit_by_name (editor, "title-changed", title, raw_title);
}

static void
emit_signal_subtitle_changed (MarkerEditor *editor)
{
  g_autofree gchar *subtitle = marker_editor_get_subtitle (editor);
  g_signal_emit_by_name (editor, "subtitle-changed", subtitle);
}

static gboolean
on_debounce_refresh_preview_cb (gpointer user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  editor->timer_id = 0;
  marker_editor_refresh_preview (editor);
  return G_SOURCE_REMOVE;
}

static void
on_document_external_change (MarkerDocument *doc,
                             gpointer        user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  gsize size = 0;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *contents = marker_document_load_contents (doc, &size, &error);

  if (!error && contents)
  {
    MarkerSourceView *source_view = editor->source_view;
    GtkTextBuffer *buffer =
      gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
    gtk_text_buffer_begin_irreversible_action (buffer);
    marker_source_view_set_text (source_view, contents, size);
    gtk_text_buffer_end_irreversible_action (buffer);
    gtk_text_buffer_set_modified (buffer, FALSE);
    marker_document_set_is_modified (doc, FALSE);
    editor->needs_refresh = TRUE;
    marker_editor_refresh_preview (editor);
  }
  else if (error)
  {
    g_warning ("Failed to reload externally changed file: %s", error->message);
  }
}

static void
on_document_notify_title (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  emit_signal_title_changed (editor);
}

static void
on_document_notify_subtitle (GObject    *object,
                            GParamSpec *pspec,
                            gpointer    user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  emit_signal_subtitle_changed (editor);
}

static void
buffer_modified_changed_cb (GtkTextBuffer *buffer,
                            gpointer       user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  gboolean modified = gtk_text_buffer_get_modified (buffer);
  marker_document_set_is_modified (editor->document, modified);
  emit_signal_title_changed (editor);
}

static void
buffer_changed_cb (GtkTextBuffer *buffer G_GNUC_UNUSED,
                   gpointer       user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  editor->needs_refresh = TRUE;
  emit_signal_title_changed (editor);

  if (editor->timer_id > 0)
  {
    g_source_remove (editor->timer_id);
    editor->timer_id = 0;
  }
  editor->timer_id = g_timeout_add (150, on_debounce_refresh_preview_cb, editor);
}

static gboolean
preview_window_close_request_cb (GtkWindow *preview_window,
                                 gpointer   user_data)
{
  GtkWidget *preview = user_data;
  g_object_ref (preview);
  gtk_window_set_child (preview_window, NULL);
  g_object_unref (preview);
  return FALSE;
}

static void
marker_editor_dispose (GObject *object)
{
  MarkerEditor *editor = MARKER_EDITOR (object);

  if (editor->timer_id > 0)
  {
    g_source_remove (editor->timer_id);
    editor->timer_id = 0;
  }

  g_clear_object (&editor->document);
  g_clear_object (&editor->search_controller);

  g_clear_object ((GObject **) &editor->preview);
  g_clear_object ((GObject **) &editor->vbox);
  G_OBJECT_CLASS (marker_editor_parent_class)->dispose (object);
}

static gboolean
marker_editor_key_pressed_cb (GtkEventControllerKey *controller,
                              guint                  keyval,
                              guint                  keycode,
                              GdkModifierType        state,
                              gpointer               user_data)
{
  (void) controller;
  (void) keycode;
  MarkerEditor *editor = MARKER_EDITOR (user_data);

  if ((state & GDK_CONTROL_MASK) && (keyval == GDK_KEY_h || keyval == GDK_KEY_H))
  {
    marker_editor_toggle_replace_bar (editor);
    return GDK_EVENT_STOP;
  }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
source_scroll_cb (GtkEventControllerScroll *controller,
                  gdouble                   dx G_GNUC_UNUSED,
                  gdouble                   dy,
                  gpointer                  user_data)
{
  MarkerEditor *editor = MARKER_EDITOR (user_data);
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  if ((state & GDK_CONTROL_MASK) != 0)
  {
    if (dy > 0)
    {
      marker_preview_zoom_out (editor->preview);
      return TRUE;
    }
    else if (dy < 0)
    {
      marker_preview_zoom_in (editor->preview);
      return TRUE;
    }
  }
  return FALSE;
}

static void
marker_editor_init (MarkerEditor *editor)
{
  editor->document = marker_document_new ();
  g_signal_connect (editor->document, "external-change",
                    G_CALLBACK (on_document_external_change), editor);
  g_signal_connect (editor->document, "notify::title",
                    G_CALLBACK (on_document_notify_title), editor);
  g_signal_connect (editor->document, "notify::subtitle",
                    G_CALLBACK (on_document_notify_subtitle), editor);

  editor->paned = GTK_PANED (gtk_paned_new (GTK_ORIENTATION_HORIZONTAL));
  editor->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

  GtkSearchBar *sbar = GTK_SEARCH_BAR (gtk_search_bar_new ());
  gtk_search_bar_set_search_mode (sbar, FALSE);
  gtk_search_bar_set_show_close_button (sbar, TRUE);

  GtkWidget *search_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_top (search_container, 4);
  gtk_widget_set_margin_bottom (search_container, 4);
  gtk_widget_set_margin_start (search_container, 4);
  gtk_widget_set_margin_end (search_container, 4);

  GtkWidget *search_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkToggleButton *replace_toggle = GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  gtk_button_set_icon_name (GTK_BUTTON (replace_toggle), "edit-find-replace-symbolic");
  gtk_widget_set_tooltip_text (GTK_WIDGET (replace_toggle), _("Toggle Replace"));

  GtkSearchEntry *search_entry = GTK_SEARCH_ENTRY (gtk_search_entry_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (search_entry), TRUE);

  GtkButton *prev_button = GTK_BUTTON (gtk_button_new_from_icon_name ("go-up-symbolic"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (prev_button), _("Previous Match"));
  GtkButton *next_button = GTK_BUTTON (gtk_button_new_from_icon_name ("go-down-symbolic"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (next_button), _("Next Match"));

  gtk_box_append (GTK_BOX (search_row), GTK_WIDGET (replace_toggle));
  gtk_box_append (GTK_BOX (search_row), GTK_WIDGET (search_entry));
  gtk_box_append (GTK_BOX (search_row), GTK_WIDGET (prev_button));
  gtk_box_append (GTK_BOX (search_row), GTK_WIDGET (next_button));
  gtk_box_append (GTK_BOX (search_container), search_row);

  GtkWidget *replace_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkSearchEntry *replace_entry = GTK_SEARCH_ENTRY (gtk_search_entry_new ());
  gtk_search_entry_set_placeholder_text (replace_entry, _("Replace with..."));
  gtk_widget_set_hexpand (GTK_WIDGET (replace_entry), TRUE);

  GtkButton *replace_button = GTK_BUTTON (gtk_button_new_with_label (_("Replace")));
  GtkButton *replace_all_button = GTK_BUTTON (gtk_button_new_with_label (_("Replace All")));

  gtk_box_append (GTK_BOX (replace_row), GTK_WIDGET (replace_entry));
  gtk_box_append (GTK_BOX (replace_row), GTK_WIDGET (replace_button));
  gtk_box_append (GTK_BOX (replace_row), GTK_WIDGET (replace_all_button));
  gtk_box_append (GTK_BOX (search_container), replace_row);

  gtk_widget_set_visible (replace_row, FALSE);

  gtk_search_bar_set_child (sbar, search_container);
  gtk_box_append (editor->vbox, GTK_WIDGET (sbar));

  gtk_paned_set_position (editor->paned, 450);
  gtk_widget_set_hexpand (GTK_WIDGET (editor->paned), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (editor->paned), TRUE);
  gtk_box_append (GTK_BOX (editor), GTK_WIDGET (editor->paned));

  editor->preview = marker_preview_new ();

  editor->source_view = marker_source_view_new ();
  editor->source_scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (editor->source_scroll), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (editor->source_scroll), TRUE);
  gtk_scrolled_window_set_child (editor->source_scroll, GTK_WIDGET (editor->source_view));

  GtkEventController *source_scroll_controller =
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (source_scroll_controller, "scroll", G_CALLBACK (source_scroll_cb), editor);
  gtk_widget_add_controller (GTK_WIDGET (editor->source_view), source_scroll_controller);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->source_view));
  g_signal_connect (buffer, "changed", G_CALLBACK (buffer_changed_cb), editor);
  g_signal_connect (buffer, "modified-changed", G_CALLBACK (buffer_modified_changed_cb), editor);

  gtk_box_append (editor->vbox, GTK_WIDGET (editor->source_scroll));

  editor->search_controller = marker_search_controller_new (sbar, search_entry,
                                                            GTK_SOURCE_VIEW (editor->source_view));
  marker_search_controller_setup_replace (editor->search_controller,
                                          replace_entry,
                                          replace_button,
                                          replace_all_button,
                                          replace_toggle);
  marker_search_controller_set_replace_box (editor->search_controller, replace_row);

  g_signal_connect_swapped (prev_button, "clicked",
                            G_CALLBACK (marker_search_controller_find_previous),
                            editor->search_controller);
  g_signal_connect_swapped (next_button, "clicked",
                            G_CALLBACK (marker_search_controller_find_next),
                            editor->search_controller);

  GtkEventController *key_controller = gtk_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed", G_CALLBACK (marker_editor_key_pressed_cb), editor);
  gtk_widget_add_controller (GTK_WIDGET (editor), key_controller);

  editor->view_mode = marker_prefs_get_default_view_mode ();
  editor->needs_refresh = FALSE;
  editor->timer_id = 0;

  g_object_ref (editor->preview);
  g_object_ref (editor->vbox);
  marker_editor_set_view_mode (editor, editor->view_mode);

  marker_editor_apply_prefs (editor);
}

static void
marker_editor_class_init (MarkerEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  object_class->dispose = marker_editor_dispose;

  g_signal_new ("title-changed",
                G_TYPE_FROM_CLASS (class),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

  g_signal_new ("subtitle-changed",
                G_TYPE_FROM_CLASS (class),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 1, G_TYPE_STRING);
}

MarkerEditor *
marker_editor_new (void)
{
  return g_object_new (MARKER_TYPE_EDITOR,
                       "orientation", GTK_ORIENTATION_VERTICAL,
                       "spacing",     0,
                       NULL);
}

MarkerEditor *
marker_editor_new_from_file (GFile *file)
{
  MarkerEditor *editor = g_object_new (MARKER_TYPE_EDITOR,
                                       "orientation", GTK_ORIENTATION_VERTICAL,
                                       "spacing", 0, NULL);
  marker_editor_open_file (editor, file);
  return editor;
}

void
marker_editor_refresh_preview (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));

  editor->needs_refresh = FALSE;

  g_autofree gchar *markdown = marker_source_view_get_text (editor->source_view, false);
  int cursor = marker_source_view_get_cursor_position (editor->source_view);

  const char *css_theme = (marker_prefs_get_use_css_theme ()) ? marker_prefs_get_css_theme () : NULL;
  GFile *file = marker_document_get_file (editor->document);
  g_autofree gchar *uri = (G_IS_FILE (file)) ? g_file_get_path (file) : NULL;

  marker_preview_render_markdown (editor->preview, markdown, css_theme, uri, cursor);
}

MarkerViewMode
marker_editor_get_view_mode (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return editor->view_mode;
}

void
marker_editor_set_view_mode (MarkerEditor   *editor,
                             MarkerViewMode  view_mode)
{
  g_assert (MARKER_IS_EDITOR (editor));
  editor->view_mode = view_mode;

  GtkWidget * const paned = GTK_WIDGET (editor->paned);
  GtkWidget * const preview = GTK_WIDGET (editor->preview);
  GtkWidget * const source_scroll = GTK_WIDGET (editor->vbox);

  GtkWidget *preview_parent = gtk_widget_get_parent (preview);
  if (preview_parent)
  {
    if (GTK_IS_PANED (preview_parent))
    {
      if (gtk_paned_get_start_child (GTK_PANED (preview_parent)) == preview)
        gtk_paned_set_start_child (GTK_PANED (preview_parent), NULL);
      else if (gtk_paned_get_end_child (GTK_PANED (preview_parent)) == preview)
        gtk_paned_set_end_child (GTK_PANED (preview_parent), NULL);
    }
    else if (GTK_IS_WINDOW (preview_parent))
    {
      gtk_window_set_child (GTK_WINDOW (preview_parent), NULL);
      gtk_window_destroy (GTK_WINDOW (preview_parent));
    }
    else if (GTK_IS_BOX (preview_parent))
    {
      gtk_box_remove (GTK_BOX (preview_parent), preview);
    }
  }

  GtkWidget *source_parent = gtk_widget_get_parent (source_scroll);
  if (source_parent)
  {
    if (GTK_IS_PANED (source_parent))
    {
      if (gtk_paned_get_start_child (GTK_PANED (source_parent)) == source_scroll)
        gtk_paned_set_start_child (GTK_PANED (source_parent), NULL);
      else if (gtk_paned_get_end_child (GTK_PANED (source_parent)) == source_scroll)
        gtk_paned_set_end_child (GTK_PANED (source_parent), NULL);
    }
    else if (GTK_IS_BOX (source_parent))
    {
      gtk_box_remove (GTK_BOX (source_parent), source_scroll);
    }
  }

  switch (view_mode)
  {
    case EDITOR_ONLY_MODE:
      gtk_paned_set_start_child (GTK_PANED (paned), source_scroll);
      gtk_paned_set_resize_start_child (GTK_PANED (paned), TRUE);
      gtk_paned_set_shrink_start_child (GTK_PANED (paned), FALSE);
      gtk_widget_grab_focus (GTK_WIDGET (editor->source_view));
      break;

    case PREVIEW_ONLY_MODE:
      gtk_paned_set_end_child (GTK_PANED (paned), preview);
      gtk_paned_set_resize_end_child (GTK_PANED (paned), TRUE);
      gtk_paned_set_shrink_end_child (GTK_PANED (paned), FALSE);
      gtk_widget_grab_focus (GTK_WIDGET (preview));
      break;

    case DUAL_PANE_MODE:
      gtk_paned_set_start_child (GTK_PANED (paned), source_scroll);
      gtk_paned_set_end_child (GTK_PANED (paned), preview);
      gtk_paned_set_resize_start_child (GTK_PANED (paned), TRUE);
      gtk_paned_set_resize_end_child (GTK_PANED (paned), TRUE);
      gtk_paned_set_shrink_start_child (GTK_PANED (paned), FALSE);
      gtk_paned_set_shrink_end_child (GTK_PANED (paned), FALSE);
      guint pane_pos = marker_prefs_get_editor_pane_width ();
      if (pane_pos == 0)
        pane_pos = 450;
      gtk_paned_set_position (GTK_PANED (paned), pane_pos);
      gtk_widget_grab_focus (GTK_WIDGET (editor->source_view));
      break;

    case DUAL_WINDOW_MODE:
      gtk_paned_set_start_child (GTK_PANED (paned), source_scroll);
      gtk_paned_set_resize_start_child (GTK_PANED (paned), TRUE);
      gtk_paned_set_shrink_start_child (GTK_PANED (paned), FALSE);

      GtkWindow *preview_window = GTK_WINDOW (gtk_window_new ());
      g_signal_connect (preview_window, "close-request", G_CALLBACK (preview_window_close_request_cb), preview);
      gtk_window_set_child (preview_window, preview);
      gtk_window_set_title (preview_window, "Preview");
      gtk_window_set_default_size (preview_window, 500, 600);
      gtk_window_present (preview_window);
      gtk_widget_grab_focus (GTK_WIDGET (editor->source_view));
      break;
  }
}

guint
marker_editor_get_pane_width (MarkerEditor *editor)
{
  if (marker_editor_get_view_mode (editor) == DUAL_PANE_MODE)
  {
    return gtk_paned_get_position (GTK_PANED (editor->paned));
  }
  else
  {
    return marker_prefs_get_editor_pane_width ();
  }
}

void
marker_editor_open_file (MarkerEditor *editor,
                         GFile        *file)
{
  g_assert (MARKER_IS_EDITOR (editor));
  g_return_if_fail (G_IS_FILE (file));

  marker_document_set_file (editor->document, file);

  gsize size = 0;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *contents = marker_document_load_contents (editor->document, &size, &error);

  if (!error && contents)
  {
    MarkerSourceView *source_view = editor->source_view;
    GtkTextBuffer *buffer =
      gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
    gtk_text_buffer_begin_irreversible_action (buffer);
    marker_source_view_set_text (source_view, contents, size);
    gtk_text_buffer_end_irreversible_action (buffer);
    gtk_text_buffer_set_modified (buffer, FALSE);
  }
  else if (error)
  {
    g_autofree gchar *path = g_file_get_parse_name (file);
    g_warning ("Failed to load file '%s': %s", path, error->message);
  }

  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->source_view));
  gtk_text_buffer_set_modified (buffer, FALSE);
  marker_document_set_is_modified (editor->document, FALSE);
  editor->needs_refresh = TRUE;
  marker_editor_refresh_preview (editor);

  emit_signal_title_changed (editor);
  emit_signal_subtitle_changed (editor);
}

void
marker_editor_reload_file (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));

  GFile *file = marker_document_get_file (editor->document);
  if (!file)
  {
    return;
  }

  gsize size = 0;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *contents = marker_document_load_contents (editor->document, &size, &error);

  if (error)
  {
    g_warning ("Unable to reload file: %s", error->message);
    return;
  }

  MarkerSourceView *source_view = editor->source_view;
  GtkSourceBuffer *buffer =
    GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view)));

  g_signal_handlers_block_by_func (buffer, buffer_changed_cb, editor);
  gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (buffer));
  marker_source_view_set_text (source_view, contents, size);
  gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (buffer));
  g_signal_handlers_unblock_by_func (buffer, buffer_changed_cb, editor);

  gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (buffer), FALSE);
  marker_document_set_is_modified (editor->document, FALSE);
  editor->needs_refresh = TRUE;

  emit_signal_title_changed (editor);
  emit_signal_subtitle_changed (editor);
}

void
marker_editor_save_file (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));

  GFile *file = marker_document_get_file (editor->document);
  g_return_if_fail (G_IS_FILE (file));

  g_autofree gchar *text = marker_source_view_get_text (editor->source_view, false);
  g_autoptr(GError) error = NULL;

  if (marker_document_save_contents (editor->document, text, strlen (text), &error))
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->source_view));
    gtk_text_buffer_set_modified (buffer, FALSE);
    marker_document_set_is_modified (editor->document, FALSE);
    emit_signal_title_changed (editor);
  }
  else if (error)
  {
    g_warning ("Unable to save file: %s", error->message);
  }
}

void
marker_editor_save_file_as (MarkerEditor *editor,
                            GFile        *file)
{
  g_assert (MARKER_IS_EDITOR (editor));
  g_return_if_fail (G_IS_FILE (file));

  g_autofree gchar *text = marker_source_view_get_text (editor->source_view, false);
  g_autoptr(GError) error = NULL;

  if (marker_document_save_as (editor->document, file, text, strlen (text), &error))
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->source_view));
    gtk_text_buffer_set_modified (buffer, FALSE);
    marker_document_set_is_modified (editor->document, FALSE);
    emit_signal_title_changed (editor);
    emit_signal_subtitle_changed (editor);
  }
  else if (error)
  {
    g_warning ("Unable to save file as: %s", error->message);
  }
}

GFile *
marker_editor_get_file (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return marker_document_get_file (editor->document);
}

gboolean
marker_editor_has_unsaved_changes (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return marker_document_get_is_modified (editor->document);
}

gchar *
marker_editor_get_title (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return marker_document_get_title (editor->document);
}

gchar *
marker_editor_get_raw_title (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  GFile *file = marker_document_get_file (editor->document);
  if (file)
  {
    return g_file_get_basename (file);
  }
  return g_strdup (_("Untitled.md"));
}

gchar *
marker_editor_get_subtitle (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return marker_document_get_subtitle (editor->document);
}

MarkerPreview *
marker_editor_get_preview (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return editor->preview;
}

MarkerSourceView *
marker_editor_get_source_view (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return editor->source_view;
}

MarkerDocument *
marker_editor_get_document (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return editor->document;
}

MarkerSearchController *
marker_editor_get_search_controller (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));
  return editor->search_controller;
}

void
marker_editor_apply_prefs (MarkerEditor *editor)
{
  g_assert (MARKER_IS_EDITOR (editor));

  GtkSourceView * const source_view = GTK_SOURCE_VIEW (marker_editor_get_source_view (editor));

  gboolean state = marker_prefs_get_show_line_numbers ();
  gtk_source_view_set_show_line_numbers (source_view, state);

  state = marker_prefs_get_wrap_text ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (source_view), (state) ? GTK_WRAP_WORD : GTK_WRAP_NONE);

  state = marker_prefs_get_show_right_margin ();
  gtk_source_view_set_show_right_margin (source_view, state);

  guint position = marker_prefs_get_right_margin_position ();
  gtk_source_view_set_right_margin_position (source_view, position);

  state = marker_prefs_get_spell_check ();
  marker_source_view_set_spell_check (MARKER_SOURCE_VIEW (source_view), state);

  g_autofree gchar *lang = marker_prefs_get_spell_check_language ();
  marker_source_view_set_spell_check_lang (MARKER_SOURCE_VIEW (source_view), lang);

  state = marker_prefs_get_highlight_current_line ();
  gtk_source_view_set_highlight_current_line (source_view, state);

  GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view)));
  state = marker_prefs_get_use_syntax_theme ();
  gtk_source_buffer_set_highlight_syntax (buffer, state);

  g_autofree gchar *theme = marker_prefs_get_syntax_theme ();
  marker_source_view_set_syntax_theme (MARKER_SOURCE_VIEW (source_view), theme);

  state = marker_prefs_get_auto_indent ();
  gtk_source_view_set_auto_indent (source_view, state);

  state = marker_prefs_get_replace_tabs ();
  gtk_source_view_set_insert_spaces_instead_of_tabs (source_view, state);

  state = marker_prefs_get_show_spaces ();
  GtkSourceSpaceDrawer *space_drawer = gtk_source_view_get_space_drawer (source_view);
  if (state)
  {
    gtk_source_space_drawer_set_types_for_locations (space_drawer, GTK_SOURCE_SPACE_LOCATION_ALL,
                                                     GTK_SOURCE_SPACE_TYPE_NBSP |
                                                     GTK_SOURCE_SPACE_TYPE_SPACE |
                                                     GTK_SOURCE_SPACE_TYPE_TAB);
  }
  gtk_source_space_drawer_set_enable_matrix (space_drawer, state);

  guint tab_width = marker_prefs_get_tab_width ();
  gtk_source_view_set_indent_width (source_view, tab_width);

  marker_source_view_set_zoom_level (marker_prefs_get_zoom_level ());
}

void
marker_editor_closing (MarkerEditor *editor)
{
  if (editor->timer_id > 0)
  {
    g_source_remove (editor->timer_id);
    editor->timer_id = 0;
  }
  editor->needs_refresh = FALSE;
}

gboolean
marker_editor_rename_file (MarkerEditor *editor,
                           const gchar  *name)
{
  g_return_val_if_fail (MARKER_IS_EDITOR (editor), FALSE);
  g_return_val_if_fail (name != NULL && *name != '\0', FALSE);

  GFile *current_file = marker_document_get_file (editor->document);
  if (!G_IS_FILE (current_file))
    return FALSE;

  g_autoptr(GFile) parent = g_file_get_parent (current_file);
  if (!parent)
    return FALSE;

  g_autoptr(GFile) new_file = g_file_get_child (parent, name);
  if (!new_file)
    return FALSE;

  if (g_file_equal (current_file, new_file))
    return TRUE;

  if (g_file_query_exists (new_file, NULL))
  {
    g_warning ("Cannot rename: destination file already exists");
    return FALSE;
  }

  GError *error = NULL;
  if (!g_file_move (current_file, new_file, G_FILE_COPY_NONE, NULL, NULL, NULL, &error))
  {
    g_autofree gchar *path = g_file_get_path (new_file);
    g_warning ("Failed to rename file to '%s': %s",
               path ? path : name, error ? error->message : "unknown error");
    g_clear_error (&error);
    return FALSE;
  }

  marker_document_set_file (editor->document, new_file);
  return TRUE;
}

void
marker_editor_toggle_search_bar (MarkerEditor *editor)
{
  marker_search_controller_toggle (editor->search_controller);
}

void
marker_editor_toggle_replace_bar (MarkerEditor *editor)
{
  g_return_if_fail (MARKER_IS_EDITOR (editor));

  GtkSearchBar *search_bar = marker_search_controller_get_search_bar (editor->search_controller);
  gboolean search_active = gtk_search_bar_get_search_mode (search_bar);
  gboolean replace_visible = marker_search_controller_get_replace_visible (editor->search_controller);

  if (!search_active)
  {
    gtk_search_bar_set_search_mode (search_bar, TRUE);
    marker_search_controller_set_replace_visible (editor->search_controller, TRUE);
    GtkSearchEntry *search_entry = marker_search_controller_get_search_entry (editor->search_controller);
    if (search_entry != NULL)
      gtk_widget_grab_focus (GTK_WIDGET (search_entry));
  }
  else if (!replace_visible)
  {
    marker_search_controller_set_replace_visible (editor->search_controller, TRUE);
    GtkSearchEntry *replace_entry = marker_search_controller_get_replace_entry (editor->search_controller);
    if (replace_entry != NULL)
      gtk_widget_grab_focus (GTK_WIDGET (replace_entry));
  }
  else
  {
    marker_search_controller_set_replace_visible (editor->search_controller, FALSE);
  }
}

void
marker_editor_set_replace_bar_visible (MarkerEditor *editor,
                                       gboolean      visible)
{
  g_return_if_fail (MARKER_IS_EDITOR (editor));

  marker_search_controller_set_replace_visible (editor->search_controller, visible);
}

GtkSearchBar *
marker_editor_get_search_bar (MarkerEditor *editor)
{
  return marker_search_controller_get_search_bar (editor->search_controller);
}
