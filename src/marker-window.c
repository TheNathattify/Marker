/*
 * marker-window.c
 *
 * Copyright (C) 2017-2020 - 2018 Fabio Colacio
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

#include "marker.h"
#include "marker-prefs.h"
#include "marker-editor.h"
#include "marker-exporter.h"
#include "marker-sketcher-window.h"

#include "marker-window.h"
#include "marker-outline.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>

#define MIN_DELTA_T 1

enum {
  TITLE_COLUMN,
  ICON_COLUMN,
  NAME_COLUMN,
  EDITOR_COLUMN,
  N_COLUMNS
};

struct _MarkerWindow
{
  AdwApplicationWindow  parent_instance;

  AdwHeaderBar         *header_bar;
  AdwWindowTitle       *window_title;
  GtkButton            *zoom_original_btn;

  gboolean              is_fullscreen;
  GtkButton            *unfullscreen_btn;

  GtkBox               *vbox;
  MarkerEditor         *active_editor;

  GtkStack             *editors_stack;
  GtkTreeView          *documents_tree_view;
  GtkTreeStore         *documents_tree_store;
  GtkPaned             *main_paned;
  GtkWidget            *paned1;
  GtkWidget            *paned2;
  guint                 editors_counter;
  guint                 untitled_files;
  gboolean              sidebar_visible;

  GtkListBox           *outline_list_box;
  GListStore           *outline_store;
  guint                 outline_timeout_id;

  guint32               last_click_;
};

G_DEFINE_TYPE (MarkerWindow, marker_window, ADW_TYPE_APPLICATION_WINDOW);

typedef void (*UnsavedWarningCallback) (MarkerWindow *window, gboolean proceed, gpointer user_data);

typedef struct {
  MarkerWindow           *window;
  UnsavedWarningCallback  callback;
  gpointer                user_data;
} WarningClosure;

gboolean
get_current_iter (MarkerWindow *window,
                  GtkTreeIter  *iter)
{
  GtkTreeModel     *model = GTK_TREE_MODEL (window->documents_tree_store);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (window->documents_tree_view);
  return gtk_tree_selection_get_selected (selection, &model, iter);
}

static void
on_unsaved_warning_response (AdwAlertDialog *dialog,
                             GAsyncResult   *result,
                             gpointer        user_data)
{
  WarningClosure *closure = user_data;
  const char *response = adw_alert_dialog_choose_finish (dialog, result);
  gboolean proceed = (g_strcmp0 (response, "discard") == 0);

  if (closure->callback)
    closure->callback (closure->window, proceed, closure->user_data);

  g_free (closure);
}

static void
show_unsaved_documents_warning_async (MarkerWindow           *window,
                                      MarkerEditor           *editor,
                                      UnsavedWarningCallback  callback,
                                      gpointer                user_data)
{
  g_assert (MARKER_IS_WINDOW (window));

  GFile *file = editor ? marker_editor_get_file (editor) : NULL;
  AdwAlertDialog *dialog = NULL;

  if (G_IS_FILE (file))
  {
    g_autofree gchar *filename = g_file_get_basename (file);
    g_autofree gchar *heading = g_strdup_printf (_("Discard changes to the document '%s'?"), filename);
    dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (heading,
                                                      _("The document has unsaved changes that will be lost if it is closed now.")));
  }
  else
  {
    dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (_("Discard changes to the document?"),
                                                      _("The document has unsaved changes that will be lost if it is closed now.")));
  }

  adw_alert_dialog_add_responses (dialog,
                                  "cancel", _("_Cancel"),
                                  "discard", _("_Discard"),
                                  NULL);
  adw_alert_dialog_set_response_appearance (dialog, "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_default_response (dialog, "cancel");
  adw_alert_dialog_set_close_response (dialog, "cancel");

  WarningClosure *closure = g_new0 (WarningClosure, 1);
  closure->window = window;
  closure->callback = callback;
  closure->user_data = user_data;

  adw_alert_dialog_choose (dialog, GTK_WIDGET (window), NULL, (GAsyncReadyCallback) on_unsaved_warning_response, closure);
}

static void
on_fullscreen_notify (GtkWindow   *gtk_window,
                      GParamSpec  *pspec,
                      gpointer     user_data)
{
  MarkerWindow *window = MARKER_WINDOW (gtk_window);
  gboolean fullscreen = gtk_window_is_fullscreen (gtk_window);
  GAction *action =
    g_action_map_lookup_action (G_ACTION_MAP (window), "fullscreen");

  if (G_IS_SIMPLE_ACTION (action))
    g_simple_action_set_state (G_SIMPLE_ACTION (action),
                               g_variant_new_boolean (fullscreen));

  if (window->unfullscreen_btn)
    gtk_widget_set_visible (GTK_WIDGET (window->unfullscreen_btn),
                            fullscreen);
}

static void
action_fullscreen (GSimpleAction *action,
                   GVariant      *value,
                   gpointer       window)
{
  gboolean state = g_variant_get_boolean (value);
  g_simple_action_set_state (action, value);

  if (state) {
    marker_window_fullscreen (MARKER_WINDOW (window));
  }
  else {
    marker_window_unfullscreen (MARKER_WINDOW (window));
  }
}

static void
action_sidebar (GSimpleAction *action,
                GVariant      *value,
                gpointer       window)
{
  gboolean state = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);
  marker_prefs_set_show_sidebar (state);

  if (state) {
    marker_window_show_sidebar (MARKER_WINDOW (window));
  }
  else {
    marker_window_hide_sidebar (MARKER_WINDOW (window));
  }
}

static void
action_link (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       window)
{
  MarkerEditor *editor = marker_window_get_active_editor (MARKER_WINDOW (window));
  MarkerSourceView *source_view = marker_editor_get_source_view (editor);
  marker_source_view_insert_link (source_view);
}

static void
action_monospace (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       window)
{
  MarkerEditor *editor = marker_window_get_active_editor (MARKER_WINDOW (window));
  MarkerSourceView *source_view = marker_editor_get_source_view (editor);
  marker_source_view_surround_selection_with (source_view, "``");
}

static void
action_italic (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       window)
{
  MarkerEditor *editor = marker_window_get_active_editor (MARKER_WINDOW (window));
  MarkerSourceView *source_view = marker_editor_get_source_view (editor);
  marker_source_view_surround_selection_with (source_view, "*");
}

static void
action_bold (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       window)
{
  MarkerEditor *editor = marker_window_get_active_editor (MARKER_WINDOW (window));
  MarkerSourceView *source_view = marker_editor_get_source_view (editor);
  marker_source_view_surround_selection_with (source_view, "**");
}

static void
action_refresh (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       window)
{
  MarkerEditor *editor = marker_window_get_active_editor (MARKER_WINDOW (window));
  marker_editor_refresh_preview (editor);
}

static void
action_zoom_out (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  MarkerPreview *preview = marker_editor_get_preview (editor);
  marker_preview_zoom_out (preview);
}

static void
action_zoom_original (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  MarkerPreview *preview = marker_editor_get_preview (editor);
  marker_preview_zoom_original (preview);
}

static void
action_zoom_in (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  MarkerPreview *preview = marker_editor_get_preview (editor);
  marker_preview_zoom_in (preview);
}

static void
action_print (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  MarkerPreview *preview = marker_editor_get_preview (editor);
  marker_preview_run_print_dialog (preview, GTK_WINDOW (window));
}

static void
on_reload_warning_cb (MarkerWindow *window,
                      gboolean      proceed,
                      gpointer      user_data)
{
  if (!proceed)
    return;

  MarkerEditor *editor = MARKER_EDITOR (user_data);
  marker_editor_reload_file (editor);
}

static void
action_reload (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);

  g_return_if_fail (MARKER_IS_EDITOR (editor));

  if (marker_editor_has_unsaved_changes (editor))
  {
    show_unsaved_documents_warning_async (window, editor, on_reload_warning_cb, editor);
    return;
  }

  marker_editor_reload_file (editor);
}

static void
action_editor_only_mode (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  marker_editor_set_view_mode (editor, EDITOR_ONLY_MODE);
}

static void
action_preview_only_mode (GSimpleAction *action,
                          GVariant      *parameter,
                          gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  marker_editor_set_view_mode (editor, PREVIEW_ONLY_MODE);
}

static void
action_dual_pane_mode (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  marker_editor_set_view_mode (editor, DUAL_PANE_MODE);
}

static void
action_dual_window_mode (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
  MarkerWindow *window = user_data;
  MarkerEditor *editor = marker_window_get_active_editor (window);
  marker_editor_set_view_mode (editor, DUAL_WINDOW_MODE);
}

gchar *
make_markup_title (MarkerEditor *editor,
                   const char   *raw_title)
{
  gchar *markup_title;
  if (marker_editor_has_unsaved_changes (editor))
  {
    markup_title = g_strdup_printf ("<i>%s</i>", raw_title);
  }
  else
  {
    markup_title = g_strdup_printf ("%s", raw_title);
  }
  return markup_title;
}

static GtkWidget *
create_outline_row_cb (gpointer item,
                       gpointer user_data)
{
  (void) user_data;
  MarkerOutlineItem *outline_item = MARKER_OUTLINE_ITEM (item);
  GtkWidget *row = gtk_list_box_row_new ();
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *label = gtk_label_new (NULL);

  guint level = marker_outline_item_get_level (outline_item);
  const gchar *title = marker_outline_item_get_title (outline_item);
  guint margin_start = (level > 0 ? (level - 1) : 0) * 12 + 8;

  gtk_widget_set_margin_start (box, margin_start);
  gtk_widget_set_margin_end (box, 8);
  gtk_widget_set_margin_top (box, 6);
  gtk_widget_set_margin_bottom (box, 6);

  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand (label, TRUE);

  g_autofree gchar *escaped_title = g_markup_escape_text (title ? title : "", -1);
  if (level == 1)
  {
    g_autofree gchar *markup = g_strdup_printf ("<b>%s</b>", escaped_title);
    gtk_label_set_markup (GTK_LABEL (label), markup);
  }
  else if (level == 2)
  {
    g_autofree gchar *markup = g_strdup_printf ("<span weight=\"semibold\">%s</span>", escaped_title);
    gtk_label_set_markup (GTK_LABEL (label), markup);
  }
  else
  {
    gtk_label_set_text (GTK_LABEL (label), title ? title : "");
  }

  gtk_box_append (GTK_BOX (box), label);
  gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), box);
  g_object_set_data_full (G_OBJECT (row), "outline-item", g_object_ref (outline_item), g_object_unref);

  return row;
}

static void
on_outline_row_activated (GtkListBox    *box,
                          GtkListBoxRow *row,
                          gpointer       user_data)
{
  (void) box;
  MarkerWindow *window = MARKER_WINDOW (user_data);
  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (!editor || !row)
    return;

  MarkerOutlineItem *item = g_object_get_data (G_OBJECT (row), "outline-item");
  if (!item)
    return;

  guint line_number = marker_outline_item_get_line_number (item);
  MarkerSourceView *source_view = marker_editor_get_source_view (editor);
  if (source_view)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line (buffer, &iter, (gint) line_number);
    gtk_text_buffer_place_cursor (buffer, &iter);
    gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (source_view), &iter, 0.05, TRUE, 0.0, 0.1);
    gtk_widget_grab_focus (GTK_WIDGET (source_view));
  }
}

static void
marker_window_update_outline (MarkerWindow *window)
{
  if (!window->outline_list_box)
    return;

  if (!window->active_editor)
  {
    if (window->outline_store)
      g_list_store_remove_all (window->outline_store);
    return;
  }

  MarkerSourceView *source_view = marker_editor_get_source_view (window->active_editor);
  if (!source_view)
    return;

  g_autofree gchar *text = marker_source_view_get_text (source_view, FALSE);
  GListStore *new_store = marker_outline_parse_markdown (text ? text : "");

  if (window->outline_store)
  {
    g_object_unref (window->outline_store);
  }
  window->outline_store = new_store;
  gtk_list_box_bind_model (window->outline_list_box,
                           G_LIST_MODEL (window->outline_store),
                           create_outline_row_cb,
                           window,
                           NULL);
}

static gboolean
on_outline_debounce_cb (gpointer user_data)
{
  MarkerWindow *window = MARKER_WINDOW (user_data);
  window->outline_timeout_id = 0;
  marker_window_update_outline (window);
  return G_SOURCE_REMOVE;
}

static void
marker_window_queue_outline_update (MarkerWindow *window)
{
  if (window->outline_timeout_id > 0)
  {
    g_source_remove (window->outline_timeout_id);
    window->outline_timeout_id = 0;
  }
  window->outline_timeout_id = g_timeout_add (200, on_outline_debounce_cb, window);
}

static void
title_changed_cb (MarkerEditor *editor,
                  const gchar  *title,
                  const gchar  *raw_title,
                  gpointer      user_data)
{
  MarkerWindow *window = MARKER_WINDOW (user_data);
  if (window->window_title)
    adw_window_title_set_title (window->window_title, title);

  GtkTreeIter iter;
  if (get_current_iter (window, &iter))
  {
    g_autofree gchar *markup_title = make_markup_title (editor, raw_title);
    gtk_tree_store_set (window->documents_tree_store,
                        &iter,
                        TITLE_COLUMN,
                        markup_title, -1);
  }

  if (editor == window->active_editor)
  {
    marker_window_queue_outline_update (window);
  }
}

static void
subtitle_changed_cb (MarkerEditor *editor,
                     const gchar  *subtitle,
                     gpointer      user_data)
{
  MarkerWindow *window = MARKER_WINDOW (user_data);
  if (window->window_title)
    adw_window_title_set_subtitle (window->window_title, subtitle);
}

static void
preview_open_file_requested_cb (MarkerPreview *preview,
                                const gchar   *uri,
                                gpointer       user_data)
{
  MarkerWindow *window = MARKER_WINDOW (user_data);
  g_return_if_fail (MARKER_IS_WINDOW (window));
  g_return_if_fail (uri != NULL);

  g_autoptr (GFile) file = NULL;
  if (g_str_has_prefix (uri, "file://"))
    file = g_file_new_for_uri (uri);
  else
    file = g_file_new_for_commandline_arg (uri);

  marker_window_new_editor_from_file (window, file);
}

static void
preview_zoom_changed_cb (MarkerPreview *preview,
                         gpointer       user_data)
{
  MarkerWindow *window = user_data;
  if (!window->zoom_original_btn || !GTK_IS_BUTTON (window->zoom_original_btn))
    return;

  const gdouble zoom_percentage = 100 * webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (preview));
  g_autofree gchar *zoom_level_str = g_strdup_printf ("%.0f%%", zoom_percentage);
  gtk_button_set_label (window->zoom_original_btn, zoom_level_str);
}

static void
save_window_geometry (MarkerWindow *window)
{
  gint width = gtk_widget_get_width (GTK_WIDGET (window));
  gint height = gtk_widget_get_height (GTK_WIDGET (window));
  if (width > 0 && height > 0)
  {
    marker_prefs_set_window_width (width);
    marker_prefs_set_window_height (height);
  }

  if (window->active_editor)
  {
    guint editor_width = marker_editor_get_pane_width (window->active_editor);
    marker_prefs_set_editor_pane_width (editor_width);
  }
}

static void
on_close_window_warning_cb (MarkerWindow *window,
                            gboolean      proceed,
                            gpointer      user_data)
{
  if (!proceed)
    return;

  save_window_geometry (window);

  GtkTreeModel *model = GTK_TREE_MODEL (window->documents_tree_store);
  gint rows = gtk_tree_model_iter_n_children (model, NULL);
  for (gint i = 0; i < rows; i++)
  {
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter (model, &iter, gtk_tree_path_new_from_indices (i, -1)))
    {
      MarkerEditor *editor = NULL;
      gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &editor, -1);
      if (editor)
        marker_editor_closing (editor);
    }
  }

  gtk_window_destroy (GTK_WINDOW (window));
}

gboolean
marker_window_try_close (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));

  GtkTreeModel *model = GTK_TREE_MODEL (window->documents_tree_store);
  gint rows = gtk_tree_model_iter_n_children (model, NULL);
  MarkerEditor *unsaved_editor = NULL;

  if (rows > 0)
  {
    for (gint i = 0; i < rows; i++)
    {
      GtkTreeIter iter;
      if (gtk_tree_model_get_iter (model, &iter, gtk_tree_path_new_from_indices (i, -1)))
      {
        MarkerEditor *editor = NULL;
        gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &editor, -1);
        if (editor && marker_editor_has_unsaved_changes (editor))
        {
          unsaved_editor = editor;
          break;
        }
      }
    }

    if (unsaved_editor != NULL)
    {
      show_unsaved_documents_warning_async (window, unsaved_editor, on_close_window_warning_cb, NULL);
      return FALSE;
    }

    on_close_window_warning_cb (window, TRUE, NULL);
    return TRUE;
  }
  else
  {
    save_window_geometry (window);
    gtk_window_destroy (GTK_WINDOW (window));
    return TRUE;
  }
}

static gboolean
window_close_request_cb (GtkWindow *window,
                         gpointer   user_data)
{
  marker_window_try_close (MARKER_WINDOW (window));
  return TRUE;
}

void
marker_window_fullscreen (MarkerWindow *window)
{
  g_return_if_fail (MARKER_IS_WINDOW (window));
  g_return_if_fail (!marker_window_is_fullscreen (window));

  window->is_fullscreen = TRUE;
  gtk_window_fullscreen (GTK_WINDOW (window));
  if (window->unfullscreen_btn)
    gtk_widget_set_visible (GTK_WIDGET (window->unfullscreen_btn), TRUE);
}

void
marker_window_unfullscreen (MarkerWindow *window)
{
  g_return_if_fail (MARKER_IS_WINDOW (window));
  g_return_if_fail (marker_window_is_fullscreen (window));

  window->is_fullscreen = FALSE;
  gtk_window_unfullscreen (GTK_WINDOW (window));
  if (window->unfullscreen_btn)
    gtk_widget_set_visible (GTK_WIDGET (window->unfullscreen_btn), FALSE);
}

typedef struct {
  MarkerWindow *window;
  MarkerEditor *editor;
  GtkWidget    *entry;
  gchar        *current_basename;
} RenameClosure;

static void
show_rename_warning_dialog (MarkerWindow *window,
                            const gchar  *message)
{
  AdwAlertDialog *alert = ADW_ALERT_DIALOG (adw_alert_dialog_new (_("Cannot Rename"), message));
  adw_alert_dialog_add_response (alert, "ok", _("_OK"));
  adw_alert_dialog_set_default_response (alert, "ok");
  adw_alert_dialog_choose (alert, GTK_WIDGET (window), NULL, NULL, NULL);
}

static void
on_rename_dialog_response (AdwAlertDialog *dialog,
                           GAsyncResult   *result,
                           gpointer        user_data)
{
  RenameClosure *closure = user_data;
  const char *response = adw_alert_dialog_choose_finish (dialog, result);

  if (g_strcmp0 (response, "rename") == 0)
  {
    const gchar *new_text = gtk_editable_get_text (GTK_EDITABLE (closure->entry));
    g_autofree gchar *trimmed = g_strstrip (g_strdup (new_text));

    if (trimmed == NULL || *trimmed == '\0')
    {
      show_rename_warning_dialog (closure->window, _("File name cannot be empty."));
      goto out;
    }

    if (strchr (trimmed, '/') != NULL)
    {
      show_rename_warning_dialog (closure->window, _("File name cannot contain '/'."));
      goto out;
    }

    if (g_strcmp0 (trimmed, closure->current_basename) == 0)
    {
      goto out;
    }

    GFile *current_file = marker_editor_get_file (closure->editor);
    if (!current_file)
    {
      goto out;
    }

    g_autoptr(GFile) parent = g_file_get_parent (current_file);
    if (parent)
    {
      g_autoptr(GFile) target_file = g_file_get_child (parent, trimmed);
      if (g_file_query_exists (target_file, NULL))
      {
        show_rename_warning_dialog (closure->window, _("A file with that name already exists."));
        goto out;
      }
    }

    if (marker_editor_rename_file (closure->editor, trimmed))
    {
      GtkTreeModel *model = GTK_TREE_MODEL (closure->window->documents_tree_store);
      gint rows = gtk_tree_model_iter_n_children (model, NULL);
      GtkTreeIter iter;
      gboolean found = FALSE;

      for (gint i = 0; i < rows; i++)
      {
        g_autoptr(GtkTreePath) path = gtk_tree_path_new_from_indices (i, -1);
        if (gtk_tree_model_get_iter (model, &iter, path))
        {
          MarkerEditor *row_editor = NULL;
          gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &row_editor, -1);
          if (row_editor == closure->editor)
          {
            found = TRUE;
            break;
          }
        }
      }

      if (found)
      {
        g_autofree gchar *markup_title = make_markup_title (closure->editor, trimmed);
        gtk_tree_store_set (closure->window->documents_tree_store,
                            &iter,
                            TITLE_COLUMN,
                            markup_title, -1);
      }

      if (closure->editor == closure->window->active_editor && closure->window->window_title)
      {
        g_autofree gchar *title = marker_editor_get_title (closure->editor);
        g_autofree gchar *subtitle = marker_editor_get_subtitle (closure->editor);

        adw_window_title_set_title (closure->window->window_title, title);
        adw_window_title_set_subtitle (closure->window->window_title, subtitle);
      }
    }
    else
    {
      show_rename_warning_dialog (closure->window, _("Failed to rename file."));
    }
  }

out:
  g_free (closure->current_basename);
  g_free (closure);
}

static void
action_rename_document_cb (GSimpleAction *action G_GNUC_UNUSED,
                           GVariant      *parameter G_GNUC_UNUSED,
                           gpointer       user_data)
{
  MarkerWindow *window = MARKER_WINDOW (user_data);
  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (!editor)
    return;

  GFile *file = marker_editor_get_file (editor);
  if (!G_IS_FILE (file))
  {
    show_rename_warning_dialog (window, _("The document must be saved before it can be renamed."));
    return;
  }

  gchar *current_name = g_file_get_basename (file);
  if (!current_name)
    return;

  GtkWidget *entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), current_name);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

  const char *dot = g_utf8_strrchr (current_name, -1, '.');
  if (dot && dot != current_name)
  {
    glong len = g_utf8_pointer_to_offset (current_name, dot);
    gtk_editable_select_region (GTK_EDITABLE (entry), 0, len);
  }
  else
  {
    gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
  }

  AdwAlertDialog *dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (_("Rename Document"), NULL));
  adw_alert_dialog_set_extra_child (dialog, entry);
  adw_alert_dialog_add_responses (dialog,
                                  "cancel", _("_Cancel"),
                                  "rename", _("_Rename"),
                                  NULL);
  adw_alert_dialog_set_response_appearance (dialog, "rename", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (dialog, "rename");
  adw_alert_dialog_set_close_response (dialog, "cancel");

  RenameClosure *closure = g_new0 (RenameClosure, 1);
  closure->window = window;
  closure->editor = editor;
  closure->entry = entry;
  closure->current_basename = current_name;

  adw_alert_dialog_choose (dialog, GTK_WIDGET (window), NULL, (GAsyncReadyCallback) on_rename_dialog_response, closure);
}

static void
tree_selection_changed_cb (GtkTreeSelection *selection,
                           gpointer          data)
{
  MarkerWindow *window = MARKER_WINDOW (data);
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  gchar *name = NULL;
  MarkerEditor *editor = NULL;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    gtk_tree_model_get (model, &iter, NAME_COLUMN, &name, -1);
    gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &editor, -1);
    if (name)
    {
      gtk_stack_set_visible_child_name (window->editors_stack, name);
      g_free (name);
    }

    window->active_editor = editor;
    if (window->window_title && editor)
    {
      g_autofree gchar *title = marker_editor_get_title (editor);
      g_autofree gchar *subtitle = marker_editor_get_subtitle (editor);
      adw_window_title_set_title (window->window_title, title);
      adw_window_title_set_subtitle (window->window_title, subtitle);
    }

    if (editor)
    {
      preview_zoom_changed_cb (marker_editor_get_preview (editor), window);
      marker_window_update_outline (window);
    }
  }
}

static void
tree_view_click_pressed_cb (GtkGestureClick *gesture,
                            gint             n_press,
                            gdouble          x,
                            gdouble          y,
                            gpointer         user_data)
{
  MarkerWindow *window = MARKER_WINDOW (user_data);
  GtkTreeView *tree_view = window->documents_tree_view;
  GtkTreePath *path = NULL;
  GtkTreeViewColumn *col = NULL;
  gint cell_x = 0;

  if (gtk_tree_view_get_path_at_pos (tree_view, (gint)x, (gint)y, &path, &col, &cell_x, NULL))
  {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
    gtk_tree_selection_select_path (selection, path);

    if (col != NULL)
    {
      gint colw = gtk_tree_view_column_get_width (col);
      if (cell_x >= colw - 28 && cell_x <= colw)
      {
        marker_window_close_current_document (window);
      }
    }
    gtk_tree_path_free (path);
  }
}

static void
marker_window_init (MarkerWindow *window)
{
  { // SETUP ACTIONS //
    GAction *action = NULL;
    GtkApplication *app = marker_get_app ();

    action = G_ACTION (g_simple_action_new ("refresh", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_refresh), window);
    const gchar *refresh_accels[] = { "<Ctrl>r", NULL };
    gtk_application_set_accels_for_action (app, "win.refresh", refresh_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("sketcher", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_open_sketcher), window);
    const gchar *sketcher_accels[] = { "<Ctrl>d", NULL };
    gtk_application_set_accels_for_action (app, "win.sketcher", sketcher_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("find", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_search), window);
    const gchar *find_accels[] = { "<Ctrl>f", NULL };
    gtk_application_set_accels_for_action (app, "win.find", find_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("replace", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_replace), window);
    const gchar *replace_accels[] = { "<Ctrl>h", NULL };
    gtk_application_set_accels_for_action (app, "win.replace", replace_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("link", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_link), window);
    const gchar *link_accels[] = { "<Ctrl>k", NULL };
    gtk_application_set_accels_for_action (app, "win.link", link_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("italic", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_italic), window);
    const gchar *italic_accels[] = { "<Ctrl>i", NULL };
    gtk_application_set_accels_for_action (app, "win.italic", italic_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("bold", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_bold), window);
    const gchar *bold_accels[] = { "<Ctrl>b", NULL };
    gtk_application_set_accels_for_action (app, "win.bold", bold_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("monospace", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_monospace), window);
    const gchar *monospace_accels[] = { "<Ctrl>m", NULL };
    gtk_application_set_accels_for_action (app, "win.monospace", monospace_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("new", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_create_new_window), NULL);
    const gchar *new_accels[] = { "<Shift><Ctrl>n", NULL };
    gtk_application_set_accels_for_action (app, "win.new", new_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("neweditor", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_new_editor), window);
    const gchar *neweditor_accels[] = { "<Ctrl>n", NULL };
    gtk_application_set_accels_for_action (app, "win.neweditor", neweditor_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("closedocument", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_close_current_document), window);
    const gchar *closedocument_accels[] = { "<Ctrl>w", NULL };
    gtk_application_set_accels_for_action (app, "win.closedocument", closedocument_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("close", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_try_close), window);
    const gchar *close_accels[] = { "<Shift><Ctrl>w", NULL };
    gtk_application_set_accels_for_action (app, "win.close", close_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("zoomoriginal", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_zoom_original), window);
    const gchar *zoomoriginal_accels[] = { "<Ctrl>0", "<Ctrl>KP_0", NULL };
    gtk_application_set_accels_for_action (app, "win.zoomoriginal", zoomoriginal_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("zoomin", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_zoom_in), window);
    const gchar *zoomin_accels[] = { "<Ctrl>plus", "<Ctrl>equal", "<Ctrl>KP_Add", NULL };
    gtk_application_set_accels_for_action (app, "win.zoomin", zoomin_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("zoomout", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_zoom_out), window);
    const gchar *zoomout_accels[] = { "<Ctrl>minus", "<Ctrl>KP_Subtract", NULL };
    gtk_application_set_accels_for_action (app, "win.zoomout", zoomout_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("editoronlymode", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_editor_only_mode), window);
    const gchar *editoronlymode_accels[] = { "<Ctrl>1", NULL };
    gtk_application_set_accels_for_action (app, "win.editoronlymode", editoronlymode_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("previewonlymode", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_preview_only_mode), window);
    const gchar *previewonlymode_accels[] = { "<Ctrl>2", NULL };
    gtk_application_set_accels_for_action (app, "win.previewonlymode", previewonlymode_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("dualpanemode", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_dual_pane_mode), window);
    const gchar *dualpanemode_accels[] = { "<Ctrl>3", NULL };
    gtk_application_set_accels_for_action (app, "win.dualpanemode", dualpanemode_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("dualwindowmode", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_dual_window_mode), window);
    const gchar *dualwindowmode_accels[] = { "<Ctrl>4", NULL };
    gtk_application_set_accels_for_action (app, "win.dualwindowmode", dualwindowmode_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("open", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_open_file), window);
    const gchar *open_accels[] = { "<Ctrl>o", NULL };
    gtk_application_set_accels_for_action (app, "win.open", open_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("reload", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_reload), window);
    const gchar *reload_accels[] = { "F5", "<Ctrl>r", NULL };
    gtk_application_set_accels_for_action (app, "win.reload", reload_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("save", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_save_active_file), window);
    const gchar *save_accels[] = { "<Ctrl>s", NULL };
    gtk_application_set_accels_for_action (app, "win.save", save_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("saveas", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_window_save_active_file_as), window);
    const gchar *saveas_accels[] = { "<Ctrl><Shift>s", NULL };
    gtk_application_set_accels_for_action (app, "win.saveas", saveas_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("export", NULL));
    g_signal_connect_swapped (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (marker_exporter_show_export_dialog), window);
    const gchar *export_accels[] = { "<Ctrl>e", NULL };
    gtk_application_set_accels_for_action (app, "win.export", export_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("print", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_print), window);
    const gchar *print_accels[] = { "<Ctrl>p", NULL };
    gtk_application_set_accels_for_action (app, "win.print", print_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new_stateful ("fullscreen", NULL, g_variant_new_boolean (FALSE)));
    g_signal_connect (G_SIMPLE_ACTION (action), "change-state", G_CALLBACK (action_fullscreen), window);
    const gchar *fullscreen_accels[] = { "F11", NULL };
    gtk_application_set_accels_for_action (app, "win.fullscreen", fullscreen_accels);

    g_signal_connect (window, "notify::fullscreened",
                      G_CALLBACK (on_fullscreen_notify), NULL);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new_stateful ("sidebar", NULL, g_variant_new_boolean (FALSE)));
    g_signal_connect (G_SIMPLE_ACTION (action), "change-state", G_CALLBACK (action_sidebar), window);
    const gchar *sidebar_accels[] = { "F12", NULL };
    gtk_application_set_accels_for_action (app, "win.sidebar", sidebar_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);

    action = G_ACTION (g_simple_action_new ("rename-document", NULL));
    g_signal_connect (G_SIMPLE_ACTION (action), "activate", G_CALLBACK (action_rename_document_cb), window);
    const gchar *rename_accels[] = { "F2", NULL };
    gtk_application_set_accels_for_action (app, "win.rename-document", rename_accels);
    g_action_map_add_action (G_ACTION_MAP (window), action);
  }

  window->is_fullscreen = FALSE;
  window->sidebar_visible = TRUE;
  window->editors_counter = 0;
  window->last_click_ = 0;

  GtkBuilder *builder = gtk_builder_new ();

  /** Content VBox **/
  GtkBox *vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  window->vbox = vbox;
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (window), GTK_WIDGET (vbox));

  /** HeaderBar **/
  gtk_builder_add_from_resource (builder, "/com/github/adrighem/marker/ui/marker-gear-popover.ui", NULL);
  gtk_builder_add_from_resource (builder, "/com/github/adrighem/marker/ui/marker-headerbar.ui", NULL);
  AdwHeaderBar *header_bar = ADW_HEADER_BAR (gtk_builder_get_object (builder, "header_bar"));
  window->header_bar = header_bar;
  window->window_title = ADW_WINDOW_TITLE (gtk_builder_get_object (builder, "window_title"));
  gtk_box_append (vbox, GTK_WIDGET (header_bar));

  GtkButton *unfullscreen_btn = GTK_BUTTON (gtk_builder_get_object (builder, "unfullscreen_btn"));
  window->unfullscreen_btn = unfullscreen_btn;
  if (unfullscreen_btn)
  {
    g_signal_connect_swapped (unfullscreen_btn, "clicked", G_CALLBACK (marker_window_unfullscreen), window);
  }

  /** Popover / Gear Menu **/
  window->zoom_original_btn = GTK_BUTTON (gtk_builder_get_object (builder, "zoom_original_btn"));

  /** Main View **/
  gtk_builder_add_from_resource (builder, "/com/github/adrighem/marker/ui/marker-window-main-view.ui", NULL);

  /** DOCUMENTS TREE **/
  GtkTreeView *documents_tree_view = GTK_TREE_VIEW (gtk_builder_get_object (builder, "documents_tree_view"));
  GtkTreeStore *documents_store = gtk_tree_store_new (N_COLUMNS,
                                                      G_TYPE_STRING,
                                                      G_TYPE_STRING,
                                                      G_TYPE_STRING,
                                                      MARKER_TYPE_EDITOR);

  gtk_tree_view_set_model (documents_tree_view, GTK_TREE_MODEL (documents_store));
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Documents"));

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "markup", TITLE_COLUMN,
                                       NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "icon-name", ICON_COLUMN,
                                       NULL);

  gtk_tree_view_append_column (documents_tree_view, column);

  window->documents_tree_store = documents_store;
  window->documents_tree_view = documents_tree_view;

  GtkTreeSelection *select = gtk_tree_view_get_selection (documents_tree_view);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (select), "changed",
                    G_CALLBACK (tree_selection_changed_cb),
                    window);

  GtkGesture *click_gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (click_gesture, "pressed", G_CALLBACK (tree_view_click_pressed_cb), window);
  gtk_widget_add_controller (GTK_WIDGET (documents_tree_view), GTK_EVENT_CONTROLLER (click_gesture));

  /** OUTLINE LIST **/
  GtkListBox *outline_list_box = GTK_LIST_BOX (gtk_builder_get_object (builder, "outline_list_box"));
  window->outline_list_box = outline_list_box;
  if (outline_list_box)
  {
    GtkWidget *placeholder = gtk_label_new (_("No headings found"));
    gtk_widget_add_css_class (placeholder, "dim-label");
    gtk_widget_set_margin_top (placeholder, 24);
    gtk_widget_set_margin_bottom (placeholder, 24);
    gtk_list_box_set_placeholder (outline_list_box, placeholder);
    g_signal_connect (outline_list_box, "row-activated", G_CALLBACK (on_outline_row_activated), window);
  }

  /** EDITOR STACKS **/
  window->editors_stack = GTK_STACK (gtk_builder_get_object (builder, "documents_stack"));

  /** MAIN PANED **/
  GtkWidget *main_paned = GTK_WIDGET (gtk_builder_get_object (builder, "main_paned"));
  gtk_widget_set_vexpand (main_paned, TRUE);
  gtk_widget_set_hexpand (main_paned, TRUE);
  gtk_box_append (vbox, main_paned);
  gtk_paned_set_position (GTK_PANED (main_paned), 0);
  window->paned1 = gtk_paned_get_start_child (GTK_PANED (main_paned));
  window->paned2 = gtk_paned_get_end_child (GTK_PANED (main_paned));
  window->main_paned = GTK_PANED (main_paned);

  /** Window size & preferences **/
  marker_window_hide_sidebar (window);
  guint width = marker_prefs_get_window_width ();
  guint height = marker_prefs_get_window_height ();
  if (width == 0)
    marker_prefs_set_window_width (900);
  if (height == 0)
    marker_prefs_set_window_height (600);

  gtk_window_set_default_size (GTK_WINDOW (window),
                               width ? width : 900,
                               height ? height : 600);

  if (marker_prefs_get_show_sidebar ())
  {
    g_action_group_activate_action (G_ACTION_GROUP (window), "sidebar", NULL);
  }

  g_signal_connect (window, "close-request", G_CALLBACK (window_close_request_cb), window);
  g_object_unref (builder);
}

static void
marker_window_dispose (GObject *object)
{
  MarkerWindow *window = MARKER_WINDOW (object);

  if (window->outline_timeout_id > 0)
  {
    g_source_remove (window->outline_timeout_id);
    window->outline_timeout_id = 0;
  }

  g_clear_object (&window->outline_store);

  G_OBJECT_CLASS (marker_window_parent_class)->dispose (object);
}

static void
marker_window_class_init (MarkerWindowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  object_class->dispose = marker_window_dispose;
}

void
marker_window_add_editor (MarkerWindow *window,
                          MarkerEditor *editor)
{
  window->active_editor = editor;
  g_autofree gchar *name = g_strdup_printf ("edit%u", window->editors_counter);

  gtk_stack_add_named (window->editors_stack, GTK_WIDGET (editor), name);
  marker_editor_refresh_preview (editor);
  gtk_widget_set_visible (GTK_WIDGET (editor), TRUE);
  gtk_stack_set_visible_child_name (window->editors_stack, name);

  if (window->window_title)
  {
    g_autofree gchar *title = marker_editor_get_title (editor);
    g_autofree gchar *subtitle = marker_editor_get_subtitle (editor);
    adw_window_title_set_title (window->window_title, title);
    adw_window_title_set_subtitle (window->window_title, subtitle);
  }

  GtkTreeIter iter;
  gtk_tree_store_append (window->documents_tree_store, &iter, NULL);

  gtk_tree_store_set (window->documents_tree_store, &iter,
                      TITLE_COLUMN, marker_editor_get_title (editor),
                      ICON_COLUMN, "window-close-symbolic",
                      NAME_COLUMN, name,
                      EDITOR_COLUMN, editor,
                      -1);

  gtk_tree_selection_select_iter (gtk_tree_view_get_selection (window->documents_tree_view),
                                  &iter);

  if (window->editors_counter >= 1)
  {
    marker_window_show_sidebar (window);
  }

  g_signal_connect (editor, "title-changed",
                    G_CALLBACK (title_changed_cb),
                    window);
  g_signal_connect (editor, "subtitle-changed",
                    G_CALLBACK (subtitle_changed_cb),
                    window);
  g_signal_connect (marker_editor_get_preview (editor), "zoom-changed",
                    G_CALLBACK (preview_zoom_changed_cb),
                    window);
  g_signal_connect (marker_editor_get_preview (editor), "open-file-requested",
                    G_CALLBACK (preview_open_file_requested_cb),
                    window);
  preview_zoom_changed_cb (marker_editor_get_preview (editor), window);
  window->editors_counter++;

  GtkWidget *source_view = GTK_WIDGET (marker_editor_get_source_view (editor));
  gtk_widget_grab_focus (source_view);
  marker_window_update_outline (window);
}

void
marker_window_new_editor (MarkerWindow *window)
{
  MarkerEditor *editor = marker_editor_new ();
  if (window->untitled_files)
  {
    g_autofree gchar *new_name = g_strdup_printf ("Untitled_%u.md", window->untitled_files);
    marker_editor_rename_file (editor, new_name);
  }
  window->untitled_files++;
  marker_window_add_editor (window, editor);
}

void
marker_window_new_editor_from_file (MarkerWindow *window,
                                    GFile        *file)
{
  gboolean duplicate = FALSE;

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (window->editors_stack));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
  {
    if (!MARKER_IS_EDITOR (child))
      continue;

    GFile *editor_file = marker_editor_get_file (MARKER_EDITOR (child));
    if (editor_file != NULL)
    {
      g_autofree char *uri1 = g_file_get_uri (editor_file);
      g_autofree char *uri2 = g_file_get_uri (file);
      if (g_strcmp0 (uri1, uri2) == 0)
      {
        GtkTreeModel *model = GTK_TREE_MODEL (window->documents_tree_store);
        GtkTreeIter iter;
        duplicate = TRUE;

        if (gtk_tree_model_get_iter_first (model, &iter))
        {
          do
          {
            MarkerEditor *editor = NULL;
            gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &editor, -1);
            if (editor == MARKER_EDITOR (child))
            {
              GtkTreeSelection *selection = gtk_tree_view_get_selection (window->documents_tree_view);
              gtk_tree_selection_select_iter (selection, &iter);
              break;
            }
          } while (gtk_tree_model_iter_next (model, &iter));
        }
        break;
      }
    }
  }

  if (!duplicate)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    if (editor != NULL)
    {
      GFile *active_file = marker_editor_get_file (editor);
      MarkerSourceView *source_view = marker_editor_get_source_view (editor);
      g_autofree gchar *md = marker_source_view_get_text (source_view, FALSE);

      if (g_strcmp0 (md, "") == 0 && active_file == NULL)
      {
        window->editors_counter++;
        marker_window_close_current_document (window);
        window->editors_counter--;
      }
    }
    editor = marker_editor_new_from_file (file);
    marker_window_add_editor (window, editor);
  }
}

MarkerWindow *
marker_window_new (GtkApplication *app)
{
  MarkerWindow *window = g_object_new (MARKER_TYPE_WINDOW, "application", app, NULL);
  marker_window_new_editor (window);
  return window;
}

MarkerWindow *
marker_window_new_from_file (GtkApplication *app,
                             GFile          *file)
{
  MarkerWindow *window = g_object_new (MARKER_TYPE_WINDOW, "application", app, NULL);
  marker_window_new_editor_from_file (window, file);
  return window;
}

static void
on_open_file_dialog_cb (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
  MarkerWindow *window = MARKER_WINDOW (user_data);
  GError *error = NULL;

  GFile *file = gtk_file_dialog_open_finish (dialog, result, &error);
  if (file != NULL)
  {
    if (MARKER_IS_WINDOW (window))
    {
      marker_window_new_editor_from_file (window, file);
    }
    g_object_unref (file);
  }
  else if (error && !g_error_matches (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
  {
    g_warning ("Failed to open file: %s", error->message);
  }

  if (error)
    g_error_free (error);
  g_object_unref (dialog);
}

void
marker_window_open_file (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));

  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _("Open Document"));

  gtk_file_dialog_open (dialog,
                        GTK_WINDOW (window),
                        NULL,
                        on_open_file_dialog_cb,
                        window);
}

static void
on_open_file_in_new_window_dialog_cb (GObject      *source_object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
  GError *error = NULL;

  GFile *file = gtk_file_dialog_open_finish (dialog, result, &error);
  if (file != NULL)
  {
    marker_create_new_window_from_file (file);
    g_object_unref (file);
  }
  else if (error && !g_error_matches (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
  {
    g_warning ("Failed to open file in new window: %s", error->message);
  }

  if (error)
    g_error_free (error);
  g_object_unref (dialog);
}

void
marker_window_open_file_in_new_window (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));

  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _("Open Document"));

  gtk_file_dialog_open (dialog,
                        GTK_WINDOW (window),
                        NULL,
                        on_open_file_in_new_window_dialog_cb,
                        window);
}

static void
on_save_file_dialog_cb (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
  MarkerWindow *window = MARKER_WINDOW (user_data);
  GError *error = NULL;

  GFile *file = gtk_file_dialog_save_finish (dialog, result, &error);
  if (file != NULL)
  {
    if (MARKER_IS_WINDOW (window))
    {
      MarkerEditor *editor = marker_window_get_active_editor (window);
      if (editor && MARKER_IS_EDITOR (editor))
        marker_editor_save_file_as (editor, file);
    }
    g_object_unref (file);
  }
  else if (error && !g_error_matches (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
  {
    g_warning ("Failed to save file: %s", error->message);
  }

  if (error)
    g_error_free (error);
  g_object_unref (dialog);
}

void
marker_window_save_active_file_as (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));
  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (!editor)
    return;

  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _("Save Document As"));

  GFile *current_file = marker_editor_get_file (editor);
  if (current_file != NULL)
  {
    g_autofree gchar *basename = g_file_get_basename (current_file);
    gtk_file_dialog_set_initial_name (dialog, basename);
    GFile *parent = g_file_get_parent (current_file);
    if (parent)
    {
      gtk_file_dialog_set_initial_folder (dialog, parent);
      g_object_unref (parent);
    }
  }
  else
  {
    gtk_file_dialog_set_initial_name (dialog, "Untitled.md");
  }

  gtk_file_dialog_save (dialog,
                        GTK_WINDOW (window),
                        NULL,
                        on_save_file_dialog_cb,
                        window);
}

void
marker_window_save_active_file (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));
  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (!editor)
    return;

  if (marker_editor_get_file (editor))
  {
    marker_editor_save_file (editor);
  }
  else
  {
    marker_window_save_active_file_as (window);
  }
}

void
marker_window_toggle_fullscreen (MarkerWindow *window)
{
  g_return_if_fail (MARKER_IS_WINDOW (window));
  if (marker_window_is_fullscreen (window))
  {
    marker_window_unfullscreen (window);
  }
  else
  {
    marker_window_fullscreen (window);
  }
}

gboolean
marker_window_is_fullscreen (MarkerWindow *window)
{
  return gtk_window_is_fullscreen (GTK_WINDOW (window));
}

MarkerEditor *
marker_window_get_active_editor (MarkerWindow *window)
{
  g_return_val_if_fail (MARKER_IS_WINDOW (window), NULL);
  return window->active_editor;
}

gboolean
marker_window_is_active_editor (MarkerWindow *window,
                                MarkerEditor *editor)
{
  g_assert (MARKER_IS_WINDOW (window));
  g_assert (MARKER_IS_EDITOR (editor));

  return (marker_window_get_active_editor (window) == editor);
}

void
marker_window_open_sketcher (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));

  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (!editor)
    return;

  GFile *file = marker_editor_get_file (editor);
  MarkerSourceView *source_view = marker_editor_get_source_view (editor);

  marker_sketcher_window_show (GTK_WINDOW (window), file, source_view);
}

static void
on_close_document_warning_cb (MarkerWindow *window,
                              gboolean      proceed,
                              gpointer      user_data)
{
  if (!proceed)
    return;

  MarkerEditor *editor = MARKER_EDITOR (user_data);
  GtkTreeModel *model = GTK_TREE_MODEL (window->documents_tree_store);
  GtkTreeIter iter;
  gboolean found = FALSE;

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    do
    {
      MarkerEditor *item_editor = NULL;
      gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &item_editor, -1);
      if (item_editor == editor)
      {
        found = TRUE;
        break;
      }
    } while (gtk_tree_model_iter_next (model, &iter));
  }

  if (found)
  {
    marker_editor_closing (editor);
    gtk_tree_store_remove (window->documents_tree_store, &iter);
    gtk_stack_remove (window->editors_stack, GTK_WIDGET (editor));
    window->editors_counter--;

    if (window->editors_counter == 1)
    {
      marker_window_hide_sidebar (window);
    }
    else if (window->editors_counter < 1)
    {
      marker_window_try_close (window);
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection (window->documents_tree_view);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gint rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (window->documents_tree_store), NULL);
      if (rows > 0)
      {
        gtk_tree_selection_select_path (selection, gtk_tree_path_new_from_indices (rows - 1, -1));
      }
    }
  }
}

void
marker_window_close_current_document (MarkerWindow *window)
{
  g_assert (MARKER_IS_WINDOW (window));

  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (!editor)
    return;

  if (marker_editor_has_unsaved_changes (editor))
  {
    show_unsaved_documents_warning_async (window, editor, on_close_document_warning_cb, editor);
    return;
  }

  on_close_document_warning_cb (window, TRUE, editor);
}

void
marker_window_toggle_sidebar (MarkerWindow *window)
{
  if (window->sidebar_visible) {
    marker_window_hide_sidebar (window);
  }
  else {
    marker_window_show_sidebar (window);
  }
}

void
marker_window_hide_sidebar (MarkerWindow *window)
{
  if (window->sidebar_visible && window->paned1) {
    window->sidebar_visible = FALSE;
    gtk_widget_set_visible (window->paned1, FALSE);
    gtk_paned_set_position (window->main_paned, 0);
  }
}

void
marker_window_show_sidebar (MarkerWindow *window)
{
  if (!window->sidebar_visible && window->paned1) {
    window->sidebar_visible = TRUE;
    gtk_widget_set_visible (window->paned1, TRUE);
    gtk_paned_set_position (window->main_paned, 200);
  }
}

void
marker_window_search (MarkerWindow *window)
{
  if (window->active_editor)
  {
    marker_editor_toggle_search_bar (window->active_editor);
  }
}

void
marker_window_replace (MarkerWindow *window)
{
  if (window->active_editor)
  {
    marker_editor_toggle_replace_bar (window->active_editor);
  }
}

void
marker_window_refresh_all_preview (MarkerWindow *window)
{
  GtkTreeModel *model = GTK_TREE_MODEL (window->documents_tree_store);
  gint rows = gtk_tree_model_iter_n_children (model, NULL);

  if (rows > 0)
  {
    for (gint i = 0; i < rows; i++)
    {
      GtkTreeIter iter;
      if (gtk_tree_model_get_iter (model, &iter, gtk_tree_path_new_from_indices (i, -1)))
      {
        MarkerEditor *editor = NULL;
        gtk_tree_model_get (model, &iter, EDITOR_COLUMN, &editor, -1);
        if (editor) {
          marker_editor_refresh_preview (editor);
        }
      }
    }
  }
}
