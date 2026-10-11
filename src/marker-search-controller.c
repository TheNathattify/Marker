/*
 * marker-search-controller.c
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

#include "marker-search-controller.h"
#include "marker-source-view.h"

struct _MarkerSearchController
{
  GObject parent_instance;

  GtkSearchBar *search_bar;
  GtkSearchEntry *search_entry;
  GtkSearchEntry *replace_entry;
  GtkButton *replace_button;
  GtkButton *replace_all_button;
  GtkToggleButton *replace_toggle_button;
  GtkWidget *replace_box;

  GtkSourceView *source_view;
  GtkSourceSearchContext *search_context;
  GtkSourceSearchSettings *search_settings;

  GtkTextIter text_iter;
  gboolean has_iter;
};

G_DEFINE_TYPE (MarkerSearchController, marker_search_controller, G_TYPE_OBJECT)

static void
on_search_text_changed (GtkSearchEntry         *entry,
                        MarkerSearchController *self)
{
  const gchar *text = gtk_editable_get_text (GTK_EDITABLE (entry));

  if (self->search_settings != NULL)
    gtk_source_search_settings_set_search_text (self->search_settings, text);

  self->has_iter = FALSE;
}

static void
on_search_next (GtkWidget              *widget,
                MarkerSearchController *self)
{
  (void) widget;
  marker_search_controller_find_next (self);
}

static void
on_search_previous (GtkWidget              *widget,
                    MarkerSearchController *self)
{
  (void) widget;
  marker_search_controller_find_previous (self);
}

static void
on_replace_button_clicked (GtkButton              *button,
                           MarkerSearchController *self)
{
  (void) button;
  marker_search_controller_replace_next (self);
}

static void
on_replace_all_button_clicked (GtkButton              *button,
                               MarkerSearchController *self)
{
  (void) button;
  marker_search_controller_replace_all (self);
}

static void
on_replace_entry_activate (GtkSearchEntry         *entry,
                           MarkerSearchController *self)
{
  (void) entry;
  marker_search_controller_replace_next (self);
}

static void
on_replace_toggle_button_toggled (GtkToggleButton        *toggle_button,
                                  MarkerSearchController *self)
{
  gboolean active = gtk_toggle_button_get_active (toggle_button);
  marker_search_controller_set_replace_visible (self, active);
}

static gboolean
on_replace_entry_key_pressed (GtkEventControllerKey *controller,
                              guint                  keyval,
                              guint                  keycode,
                              GdkModifierType        state,
                              gpointer               user_data)
{
  (void) controller;
  (void) keycode;
  MarkerSearchController *self = MARKER_SEARCH_CONTROLLER (user_data);

  if ((state & GDK_CONTROL_MASK) && (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter))
    {
      marker_search_controller_replace_all (self);
      return GDK_EVENT_STOP;
    }
  return GDK_EVENT_PROPAGATE;
}

static void
on_stop_search (GtkSearchEntry         *entry,
                MarkerSearchController *self)
{
  (void) entry;
  if (self->search_bar != NULL)
    gtk_search_bar_set_search_mode (self->search_bar, FALSE);
}

static void
on_buffer_changed (GtkTextBuffer          *buffer,
                   MarkerSearchController *self)
{
  (void) buffer;
  self->has_iter = FALSE;
}

static void
marker_search_controller_dispose (GObject *object)
{
  MarkerSearchController *self = MARKER_SEARCH_CONTROLLER (object);

  g_clear_object (&self->search_bar);
  g_clear_object (&self->search_entry);
  g_clear_object (&self->replace_entry);
  g_clear_object (&self->replace_button);
  g_clear_object (&self->replace_all_button);
  g_clear_object (&self->replace_toggle_button);
  g_clear_object (&self->replace_box);
  g_clear_object (&self->source_view);
  g_clear_object (&self->search_context);
  g_clear_object (&self->search_settings);

  G_OBJECT_CLASS (marker_search_controller_parent_class)->dispose (object);
}

static void
marker_search_controller_class_init (MarkerSearchControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = marker_search_controller_dispose;
}

static void
marker_search_controller_init (MarkerSearchController *self)
{
  self->has_iter = FALSE;
}

MarkerSearchController *
marker_search_controller_new (GtkSearchBar   *search_bar,
                              GtkSearchEntry *search_entry,
                              GtkSourceView  *source_view)
{
  MarkerSearchController *self = g_object_new (MARKER_TYPE_SEARCH_CONTROLLER, NULL);

  if (search_bar != NULL)
    self->search_bar = g_object_ref (search_bar);

  if (search_entry != NULL)
    {
      self->search_entry = g_object_ref (search_entry);

      g_signal_connect_object (self->search_entry,
                               "search-changed",
                               G_CALLBACK (on_search_text_changed),
                               self,
                               0);
      g_signal_connect_object (self->search_entry,
                               "activate",
                               G_CALLBACK (on_search_next),
                               self,
                               0);
      g_signal_connect_object (self->search_entry,
                               "next-match",
                               G_CALLBACK (on_search_next),
                               self,
                               0);
      g_signal_connect_object (self->search_entry,
                               "previous-match",
                               G_CALLBACK (on_search_previous),
                               self,
                               0);
      g_signal_connect_object (self->search_entry,
                               "stop-search",
                               G_CALLBACK (on_stop_search),
                               self,
                               0);
    }

  if (self->search_bar != NULL && self->search_entry != NULL)
    {
      gtk_search_bar_connect_entry (self->search_bar, GTK_EDITABLE (self->search_entry));
      gtk_search_bar_set_show_close_button (self->search_bar, TRUE);
    }

  if (source_view != NULL)
    {
      self->source_view = g_object_ref (source_view);

      if (MARKER_IS_SOURCE_VIEW (source_view))
        {
          self->search_context = marker_source_get_search_context (MARKER_SOURCE_VIEW (source_view));
          if (self->search_context != NULL)
            g_object_ref (self->search_context);
        }

      if (self->search_context == NULL)
        {
          GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
          if (GTK_SOURCE_IS_BUFFER (buffer))
            {
              self->search_settings = gtk_source_search_settings_new ();
              gtk_source_search_settings_set_wrap_around (self->search_settings, TRUE);
              self->search_context = gtk_source_search_context_new (GTK_SOURCE_BUFFER (buffer), self->search_settings);
              gtk_source_search_context_set_highlight (self->search_context, TRUE);
            }
        }
      else
        {
          self->search_settings = gtk_source_search_context_get_settings (self->search_context);
          if (self->search_settings != NULL)
            g_object_ref (self->search_settings);
        }

      GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (source_view));
      if (buf != NULL)
        {
          g_signal_connect_object (buf,
                                   "changed",
                                   G_CALLBACK (on_buffer_changed),
                                   self,
                                   0);
        }
    }

  return self;
}

void
marker_search_controller_setup_replace (MarkerSearchController *self,
                                        GtkSearchEntry         *replace_entry,
                                        GtkButton              *replace_button,
                                        GtkButton              *replace_all_button,
                                        GtkToggleButton        *replace_toggle_button)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (replace_entry != NULL)
    {
      g_clear_object (&self->replace_entry);
      self->replace_entry = g_object_ref (replace_entry);

      g_signal_connect_object (self->replace_entry,
                               "activate",
                               G_CALLBACK (on_replace_entry_activate),
                               self,
                               0);
      g_signal_connect_object (self->replace_entry,
                               "stop-search",
                               G_CALLBACK (on_stop_search),
                               self,
                               0);

      GtkEventController *key_controller = gtk_event_controller_key_new ();
      g_signal_connect_object (key_controller,
                               "key-pressed",
                               G_CALLBACK (on_replace_entry_key_pressed),
                               self,
                               0);
      gtk_widget_add_controller (GTK_WIDGET (self->replace_entry), key_controller);
    }

  if (replace_button != NULL)
    {
      g_clear_object (&self->replace_button);
      self->replace_button = g_object_ref (replace_button);

      g_signal_connect_object (self->replace_button,
                               "clicked",
                               G_CALLBACK (on_replace_button_clicked),
                               self,
                               0);
    }

  if (replace_all_button != NULL)
    {
      g_clear_object (&self->replace_all_button);
      self->replace_all_button = g_object_ref (replace_all_button);

      g_signal_connect_object (self->replace_all_button,
                               "clicked",
                               G_CALLBACK (on_replace_all_button_clicked),
                               self,
                               0);
    }

  if (replace_toggle_button != NULL)
    {
      g_clear_object (&self->replace_toggle_button);
      self->replace_toggle_button = g_object_ref (replace_toggle_button);

      g_signal_connect_object (self->replace_toggle_button,
                               "toggled",
                               G_CALLBACK (on_replace_toggle_button_toggled),
                               self,
                               0);
    }
}

void
marker_search_controller_set_replace_box (MarkerSearchController *self,
                                          GtkWidget              *replace_box)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  g_clear_object (&self->replace_box);
  if (replace_box != NULL)
    self->replace_box = g_object_ref (replace_box);
}

void
marker_search_controller_toggle (MarkerSearchController *self)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->search_bar == NULL)
    return;

  gboolean active = gtk_search_bar_get_search_mode (self->search_bar);
  if (!active)
    {
      gtk_search_bar_set_search_mode (self->search_bar, TRUE);
      if (self->search_entry != NULL)
        {
          gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
          gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
        }
    }
  else
    {
      gtk_search_bar_set_search_mode (self->search_bar, FALSE);
    }
}

void
marker_search_controller_find_next (MarkerSearchController *self)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->search_context == NULL || self->source_view == NULL)
    return;

  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (gtk_source_search_context_get_buffer (self->search_context));
  if (buffer == NULL)
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->source_view));

  GtkTextIter iter;
  GtkTextIter close;

  gtk_text_buffer_get_end_iter (buffer, &close);

  if (self->has_iter)
    {
      iter = self->text_iter;
      if (gtk_text_iter_compare (&iter, &close) == 0)
        gtk_text_buffer_get_start_iter (buffer, &iter);
    }
  else
    {
      GtkTextIter sel_start, sel_end;
      if (gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end))
        iter = sel_end;
      else
        gtk_text_buffer_get_start_iter (buffer, &iter);
    }

  GtkTextIter start;
  GtkTextIter end;
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_start_iter (buffer, &end);

  gboolean match = gtk_source_search_context_forward (self->search_context, &iter, &start, &end, NULL);

  if (match && gtk_text_iter_compare (&start, &end) != 0)
    {
      gtk_text_buffer_select_range (buffer, &start, &end);
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (self->source_view), &start, 0, TRUE, 0, 0);
      self->text_iter = end;
      self->has_iter = TRUE;
    }
  else
    {
      gtk_text_buffer_get_start_iter (buffer, &start);
      self->text_iter = start;
      self->has_iter = FALSE;
    }
}

void
marker_search_controller_find_previous (MarkerSearchController *self)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->search_context == NULL || self->source_view == NULL)
    return;

  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (gtk_source_search_context_get_buffer (self->search_context));
  if (buffer == NULL)
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->source_view));

  GtkTextIter iter;
  GtkTextIter close;

  gtk_text_buffer_get_start_iter (buffer, &close);

  if (self->has_iter)
    {
      iter = self->text_iter;
      if (gtk_text_iter_compare (&iter, &close) == 0)
        gtk_text_buffer_get_end_iter (buffer, &iter);
    }
  else
    {
      GtkTextIter sel_start, sel_end;
      if (gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end))
        iter = sel_start;
      else
        gtk_text_buffer_get_end_iter (buffer, &iter);
    }

  GtkTextIter start;
  GtkTextIter end;
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_start_iter (buffer, &end);

  gboolean match = gtk_source_search_context_backward (self->search_context, &iter, &start, &end, NULL);

  if (match && gtk_text_iter_compare (&start, &end) != 0)
    {
      gtk_text_buffer_select_range (buffer, &start, &end);
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (self->source_view), &start, 0, TRUE, 0, 0);
      self->text_iter = start;
      self->has_iter = TRUE;
    }
  else
    {
      gtk_text_buffer_get_end_iter (buffer, &start);
      self->text_iter = start;
      self->has_iter = FALSE;
    }
}

void
marker_search_controller_replace_next (MarkerSearchController *self)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->search_context == NULL || self->source_view == NULL)
    return;

  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (gtk_source_search_context_get_buffer (self->search_context));
  if (buffer == NULL)
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->source_view));

  const gchar *replace_text = "";
  if (self->replace_entry != NULL)
    replace_text = gtk_editable_get_text (GTK_EDITABLE (self->replace_entry));

  if (replace_text == NULL)
    replace_text = "";

  GtkTextIter sel_start, sel_end;
  gboolean has_selection = gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);
  gboolean replaced = FALSE;

  if (has_selection)
    {
      GError *error = NULL;
      replaced = gtk_source_search_context_replace (self->search_context,
                                                    &sel_start,
                                                    &sel_end,
                                                    replace_text,
                                                    -1,
                                                    &error);
      if (error != NULL)
        {
          g_warning ("Search replace error: %s", error->message);
          g_error_free (error);
        }

      if (replaced)
        {
          self->text_iter = sel_end;
          self->has_iter = TRUE;
        }
    }

  if (!replaced)
    {
      marker_search_controller_find_next (self);
      if (gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end))
        {
          GError *error = NULL;
          replaced = gtk_source_search_context_replace (self->search_context,
                                                        &sel_start,
                                                        &sel_end,
                                                        replace_text,
                                                        -1,
                                                        &error);
          if (error != NULL)
            {
              g_warning ("Search replace error: %s", error->message);
              g_error_free (error);
            }

          if (replaced)
            {
              self->text_iter = sel_end;
              self->has_iter = TRUE;
              marker_search_controller_find_next (self);
            }
        }
    }
  else
    {
      marker_search_controller_find_next (self);
    }
}

void
marker_search_controller_replace_all (MarkerSearchController *self)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->search_context == NULL)
    return;

  const gchar *replace_text = "";
  if (self->replace_entry != NULL)
    replace_text = gtk_editable_get_text (GTK_EDITABLE (self->replace_entry));

  if (replace_text == NULL)
    replace_text = "";

  GError *error = NULL;
  gtk_source_search_context_replace_all (self->search_context,
                                         replace_text,
                                         -1,
                                         &error);
  if (error != NULL)
    {
      g_warning ("Search replace all error: %s", error->message);
      g_error_free (error);
    }

  self->has_iter = FALSE;
}

void
marker_search_controller_set_replace_visible (MarkerSearchController *self,
                                              gboolean                visible)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->replace_box != NULL)
    gtk_widget_set_visible (self->replace_box, visible);
  else
    {
      if (self->replace_entry != NULL)
        gtk_widget_set_visible (GTK_WIDGET (self->replace_entry), visible);
      if (self->replace_button != NULL)
        gtk_widget_set_visible (GTK_WIDGET (self->replace_button), visible);
      if (self->replace_all_button != NULL)
        gtk_widget_set_visible (GTK_WIDGET (self->replace_all_button), visible);
    }

  if (self->replace_toggle_button != NULL)
    {
      gboolean current = gtk_toggle_button_get_active (self->replace_toggle_button);
      if (current != visible)
        gtk_toggle_button_set_active (self->replace_toggle_button, visible);
    }
}

gboolean
marker_search_controller_get_replace_visible (MarkerSearchController *self)
{
  g_return_val_if_fail (MARKER_IS_SEARCH_CONTROLLER (self), FALSE);

  if (self->replace_box != NULL)
    return gtk_widget_get_visible (self->replace_box);
  if (self->replace_entry != NULL)
    return gtk_widget_get_visible (GTK_WIDGET (self->replace_entry));

  return FALSE;
}

void
marker_search_controller_set_search_text (MarkerSearchController *self,
                                          const gchar            *text)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->search_entry != NULL)
    {
      const gchar *current = gtk_editable_get_text (GTK_EDITABLE (self->search_entry));
      if (g_strcmp0 (current, text ? text : "") != 0)
        gtk_editable_set_text (GTK_EDITABLE (self->search_entry), text ? text : "");
    }

  if (self->search_settings != NULL)
    gtk_source_search_settings_set_search_text (self->search_settings, text);

  self->has_iter = FALSE;
}

void
marker_search_controller_set_replace_text (MarkerSearchController *self,
                                           const gchar            *text)
{
  g_return_if_fail (MARKER_IS_SEARCH_CONTROLLER (self));

  if (self->replace_entry != NULL)
    {
      const gchar *current = gtk_editable_get_text (GTK_EDITABLE (self->replace_entry));
      if (g_strcmp0 (current, text ? text : "") != 0)
        gtk_editable_set_text (GTK_EDITABLE (self->replace_entry), text ? text : "");
    }
}

GtkSearchBar *
marker_search_controller_get_search_bar (MarkerSearchController *self)
{
  g_return_val_if_fail (MARKER_IS_SEARCH_CONTROLLER (self), NULL);

  return self->search_bar;
}

GtkSearchEntry *
marker_search_controller_get_search_entry (MarkerSearchController *self)
{
  g_return_val_if_fail (MARKER_IS_SEARCH_CONTROLLER (self), NULL);

  return self->search_entry;
}

GtkSearchEntry *
marker_search_controller_get_replace_entry (MarkerSearchController *self)
{
  g_return_val_if_fail (MARKER_IS_SEARCH_CONTROLLER (self), NULL);

  return self->replace_entry;
}

GtkToggleButton *
marker_search_controller_get_replace_toggle_button (MarkerSearchController *self)
{
  g_return_val_if_fail (MARKER_IS_SEARCH_CONTROLLER (self), NULL);

  return self->replace_toggle_button;
}
