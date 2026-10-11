/*
 * marker-sketcher-window.c
 *
 * Copyright (C) 2017 - 2018 Marker Project
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

#include "marker-sketcher-window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define P_SIZE_S 3
#define P_SIZE_M 6
#define P_SIZE_L 12

#define F_SIZE_S 12
#define F_SIZE_M 14
#define F_SIZE_L 18

struct _MarkerSketcherWindow
{
  AdwWindow         parent_instance;

  GtkPopover       *text_popover;
  GtkEntry         *text_entry;

  GtkDrawingArea   *drawing_area;
  cairo_surface_t  *surface;

  GList            *history;
  GList            *future;

  gboolean          status;
  gdouble           pos_x;
  gdouble           pos_y;

  gdouble           size;
  SketchTool        tool;
  GdkRGBA           color;

  char             *base_file;
  MarkerSourceView *source_view;
};

G_DEFINE_TYPE (MarkerSketcherWindow, marker_sketcher_window, ADW_TYPE_WINDOW)

static const char noname[] = "Untitled.md";

static void
clean_surface_list (GList *list)
{
  for (GList *l = list; l != NULL; l = l->next)
  {
    if (l->data)
    {
      cairo_surface_destroy ((cairo_surface_t *)l->data);
      l->data = NULL;
    }
  }
}

static void
clear_surface (cairo_surface_t *surface)
{
  cairo_t *cr = cairo_create (surface);
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_paint (cr);
  cairo_destroy (cr);
}

static void
sketcher_draw_func (GtkDrawingArea *drawing_area G_GNUC_UNUSED,
                    cairo_t        *cr,
                    int             width,
                    int             height,
                    gpointer        user_data)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (user_data);

  if (w->surface == NULL)
  {
    w->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    clear_surface (w->surface);
  }
  else
  {
    int surf_w = cairo_image_surface_get_width (w->surface);
    int surf_h = cairo_image_surface_get_height (w->surface);
    if (surf_w != width || surf_h != height)
    {
      cairo_surface_t *destination = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
      clear_surface (destination);
      cairo_t *cr_dest = cairo_create (destination);
      cairo_set_source_surface (cr_dest, w->surface, 0, 0);
      cairo_paint (cr_dest);
      cairo_destroy (cr_dest);
      cairo_surface_destroy (w->surface);
      w->surface = destination;
    }
  }

  cairo_set_source_surface (cr, w->surface, 0, 0);
  cairo_paint (cr);
}

static void
draw_text (MarkerSketcherWindow *w)
{
  GtkWidget *widget = GTK_WIDGET (w->drawing_area);
  gdouble x = w->pos_x;
  gdouble y = w->pos_y;
  const gchar *text = gtk_editable_get_text (GTK_EDITABLE (w->text_entry));
  if (!widget || !w->surface)
    return;

  cairo_t *cr = cairo_create (w->surface);
  if (w->size == P_SIZE_S)
    cairo_set_font_size (cr, F_SIZE_S);
  else if (w->size == P_SIZE_M)
    cairo_set_font_size (cr, F_SIZE_M);
  else
    cairo_set_font_size (cr, F_SIZE_L);

  cairo_set_source_rgb (cr, w->color.red, w->color.green, w->color.blue);
  cairo_move_to (cr, x, y);
  cairo_show_text (cr, text);
  cairo_destroy (cr);
  gtk_widget_queue_draw (widget);
}

static void
draw_brush (GtkWidget            *widget,
            gdouble               x,
            gdouble               y,
            MarkerSketcherWindow *w)
{
  if (!widget || !w->surface)
    return;

  cairo_t *cr = cairo_create (w->surface);
  if (w->tool == PEN)
    cairo_set_source_rgba (cr, w->color.red, w->color.green, w->color.blue, w->color.alpha);
  else if (w->tool == ERASER)
    cairo_set_source_rgb (cr, 1, 1, 1);

  if (!w->status)
  {
    cairo_arc (cr, x, y, w->size / 2, 0, 2.0 * M_PI);
    cairo_fill (cr);
    cairo_destroy (cr);
    gtk_widget_queue_draw (widget);
  }
  else
  {
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width (cr, w->size);
    cairo_move_to (cr, w->pos_x, w->pos_y);
    cairo_line_to (cr, x, y);
    cairo_stroke (cr);
    cairo_destroy (cr);
    gtk_widget_queue_draw (widget);
  }
  w->status = TRUE;
  w->pos_x = x;
  w->pos_y = y;
}

static void
add_history (MarkerSketcherWindow *w)
{
  if (!w->surface)
    return;

  int width = cairo_image_surface_get_width (w->surface);
  int height = cairo_image_surface_get_height (w->surface);
  cairo_surface_t *destination = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create (destination);
  cairo_set_source_surface (cr, w->surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);

  w->history = g_list_append (w->history, destination);
  if (w->future)
  {
    clean_surface_list (w->future);
    g_list_free (w->future);
    w->future = NULL;
  }
}

static void
on_click_pressed_cb (GtkGestureClick *gesture G_GNUC_UNUSED,
                     gint             n_press G_GNUC_UNUSED,
                     gdouble          x,
                     gdouble          y,
                     gpointer         user_data)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (user_data);
  if (w->surface == NULL)
    return;

  if (w->tool == PEN || w->tool == ERASER)
  {
    if (!w->status)
    {
      add_history (w);
    }
    draw_brush (GTK_WIDGET (w->drawing_area), x, y, w);
  }
  else
  {
    GdkRectangle rect = { (int)x, (int)y, 1, 1 };
    w->pos_x = x;
    w->pos_y = y;
    gtk_popover_set_pointing_to (w->text_popover, &rect);
    gtk_popover_popup (w->text_popover);
  }
}

static void
on_click_released_cb (GtkGestureClick *gesture G_GNUC_UNUSED,
                      gint             n_press G_GNUC_UNUSED,
                      gdouble          x G_GNUC_UNUSED,
                      gdouble          y G_GNUC_UNUSED,
                      gpointer         user_data)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (user_data);
  w->status = FALSE;
}

static void
on_motion_cb (GtkEventControllerMotion *controller,
              gdouble                   x,
              gdouble                   y,
              gpointer                  user_data)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (user_data);
  if (w->surface == NULL)
    return;

  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  if (state & GDK_BUTTON1_MASK)
  {
    draw_brush (GTK_WIDGET (w->drawing_area), x, y, w);
  }
}

static void
clean (MarkerSketcherWindow *window)
{
  gtk_window_destroy (GTK_WINDOW (window));
}

static void
close_cb (GtkButton *widget G_GNUC_UNUSED,
          gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  clean (window);
}

static void
pen_cb (GtkButton *button G_GNUC_UNUSED,
        gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  window->tool = PEN;
  gtk_widget_set_cursor_from_name (GTK_WIDGET (window->drawing_area), "crosshair");
}

static void
eraser_cb (GtkButton *button G_GNUC_UNUSED,
           gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  window->tool = ERASER;
  gtk_widget_set_cursor_from_name (GTK_WIDGET (window->drawing_area), "cell");
}

static void
text_cb (GtkButton *button G_GNUC_UNUSED,
         gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  window->tool = TEXT;
  gtk_widget_set_cursor_from_name (GTK_WIDGET (window->drawing_area), "text");
}

static void
small_cb (GtkButton *button G_GNUC_UNUSED,
          gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  window->size = P_SIZE_S;
}

static void
medium_cb (GtkButton *button G_GNUC_UNUSED,
           gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  window->size = P_SIZE_M;
}

static void
large_cb (GtkButton *button G_GNUC_UNUSED,
          gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  window->size = P_SIZE_L;
}

static void
color_set_cb (GtkColorChooser *button,
              gpointer         user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  gtk_color_chooser_get_rgba (button, &window->color);
}

static char *
create_unic_name (const char *base)
{
  GRand *rand = g_rand_new ();
  int uuid = g_rand_int_range (rand, 0, 9999);
  g_rand_free (rand);
  return g_strdup_printf ("%s.%d.png", base ? base : "sketch", uuid);
}

static void
insert_sketch_cb (GtkButton *button G_GNUC_UNUSED,
                  gpointer   user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);

  char *fpath = create_unic_name (window->base_file);
  cairo_surface_write_to_png (window->surface, fpath);
  marker_source_view_insert_image (window->source_view, fpath);
  g_free (fpath);

  clean (window);
}

static void
redo (MarkerSketcherWindow *window)
{
  GList *last = g_list_last (window->future);
  if (last)
  {
    window->history = g_list_append (window->history, window->surface);
    window->surface = (cairo_surface_t *)last->data;
    gtk_widget_queue_draw (GTK_WIDGET (window->drawing_area));
    window->future = g_list_remove_link (window->future, last);
    g_list_free_1 (last);
  }
}

static void
undo (MarkerSketcherWindow *window)
{
  GList *last = g_list_last (window->history);
  if (last)
  {
    window->future = g_list_append (window->future, window->surface);
    window->surface = (cairo_surface_t *)last->data;
    gtk_widget_queue_draw (GTK_WIDGET (window->drawing_area));
    window->history = g_list_remove_link (window->history, last);
    g_list_free_1 (last);
  }
}

static gboolean
on_key_pressed_cb (GtkEventControllerKey *controller G_GNUC_UNUSED,
                   guint                  keyval,
                   guint                  keycode G_GNUC_UNUSED,
                   GdkModifierType        state,
                   gpointer               user_data)
{
  MarkerSketcherWindow *window = MARKER_SKETCHER_WINDOW (user_data);
  gboolean ctrl_pressed = (state & GDK_CONTROL_MASK) != 0;
  if (ctrl_pressed)
  {
    switch (keyval)
    {
    case GDK_KEY_z:
      undo (window);
      return TRUE;
    case GDK_KEY_Z:
      redo (window);
      return TRUE;
    }
  }
  return FALSE;
}

static void
add_text_cb (GtkWidget *button G_GNUC_UNUSED,
             gpointer   user_data)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (user_data);
  gtk_popover_popdown (w->text_popover);
  add_history (w);
  draw_text (w);
  gtk_editable_set_text (GTK_EDITABLE (w->text_entry), "");
}

static void
close_text_cb (GtkButton *button G_GNUC_UNUSED,
               gpointer   user_data)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (user_data);
  gtk_popover_popdown (w->text_popover);
  gtk_editable_set_text (GTK_EDITABLE (w->text_entry), "");
}

static void
init_ui (MarkerSketcherWindow *window)
{
  GtkBuilder *builder = gtk_builder_new_from_resource ("/com/github/fabiocolacio/marker/ui/marker-sketcher-window.ui");

  GtkDrawingArea *drawing_area = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  window->drawing_area = drawing_area;
  gtk_widget_set_hexpand (GTK_WIDGET (drawing_area), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (drawing_area), TRUE);

  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);

  GtkBox *vbox = GTK_BOX (gtk_builder_get_object (builder, "vbox"));
  gtk_box_append (vbox, GTK_WIDGET (drawing_area));

  GtkWidget *hbar = GTK_WIDGET (gtk_builder_get_object (builder, "header_bar"));

  GtkPopover *text_popover = GTK_POPOVER (gtk_builder_get_object (builder, "text_popover"));
  GtkEntry *text_entry = GTK_ENTRY (gtk_builder_get_object (builder, "text_entry"));
  window->text_entry = text_entry;
  window->text_popover = text_popover;
  if (text_popover)
  {
    gtk_widget_set_parent (GTK_WIDGET (text_popover), GTK_WIDGET (drawing_area));
    gtk_popover_set_position (text_popover, GTK_POS_LEFT);
  }

  adw_window_set_content (ADW_WINDOW (window), GTK_WIDGET (vbox));
  if (hbar)
    gtk_window_set_titlebar (GTK_WINDOW (window), hbar);

  gtk_drawing_area_set_draw_func (drawing_area, sketcher_draw_func, window, NULL);

  GtkGesture *click_gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (click_gesture, "pressed", G_CALLBACK (on_click_pressed_cb), window);
  g_signal_connect (click_gesture, "released", G_CALLBACK (on_click_released_cb), window);
  gtk_widget_add_controller (GTK_WIDGET (drawing_area), GTK_EVENT_CONTROLLER (click_gesture));

  GtkEventController *motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect (motion_controller, "motion", G_CALLBACK (on_motion_cb), window);
  gtk_widget_add_controller (GTK_WIDGET (drawing_area), motion_controller);

  GtkEventController *key_controller = gtk_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed", G_CALLBACK (on_key_pressed_cb), window);
  gtk_widget_add_controller (GTK_WIDGET (window), key_controller);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (drawing_area), "crosshair");

  GObject *obj;
  if ((obj = gtk_builder_get_object (builder, "close_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (close_cb), window);
  if ((obj = gtk_builder_get_object (builder, "pen_radio_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (pen_cb), window);
  if ((obj = gtk_builder_get_object (builder, "eraser_radio_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (eraser_cb), window);
  if ((obj = gtk_builder_get_object (builder, "text_radio_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (text_cb), window);
  if ((obj = gtk_builder_get_object (builder, "color_selection_button")))
    g_signal_connect (obj, "color-set", G_CALLBACK (color_set_cb), window);
  if ((obj = gtk_builder_get_object (builder, "insert_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (insert_sketch_cb), window);
  if ((obj = gtk_builder_get_object (builder, "small_radio_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (small_cb), window);
  if ((obj = gtk_builder_get_object (builder, "medium_radio_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (medium_cb), window);
  if ((obj = gtk_builder_get_object (builder, "large_radio_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (large_cb), window);
  if ((obj = gtk_builder_get_object (builder, "close_text_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (close_text_cb), window);
  if ((obj = gtk_builder_get_object (builder, "add_text_button")))
    g_signal_connect (obj, "clicked", G_CALLBACK (add_text_cb), window);
  if (text_entry)
    g_signal_connect (text_entry, "activate", G_CALLBACK (add_text_cb), window);

  g_object_unref (builder);
}

MarkerSketcherWindow *
marker_sketcher_window_show (GtkWindow        *parent,
                             GFile            *file,
                             MarkerSourceView *source_view)
{
  GtkApplication *app = gtk_window_get_application (parent);
  MarkerSketcherWindow *w = marker_sketcher_window_new (app);
  gtk_window_set_transient_for (GTK_WINDOW (w), parent);
  w->source_view = source_view;

  if (file)
  {
    w->base_file = g_file_get_path (file);
  }
  else
  {
    g_autofree char *current_dir = g_get_current_dir ();
    w->base_file = g_strdup_printf ("%s/%s", current_dir, noname);
  }

  gtk_window_present (GTK_WINDOW (w));
  return w;
}

MarkerSketcherWindow *
marker_sketcher_window_new (GtkApplication *app)
{
  return g_object_new (MARKER_TYPE_SKETCHER_WINDOW, "application", app, NULL);
}

static void
marker_sketcher_window_dispose (GObject *object)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (object);

  if (w->history)
  {
    clean_surface_list (w->history);
    g_list_free (w->history);
    w->history = NULL;
  }

  if (w->future)
  {
    clean_surface_list (w->future);
    g_list_free (w->future);
    w->future = NULL;
  }

  if (w->surface)
  {
    cairo_surface_destroy (w->surface);
    w->surface = NULL;
  }

  G_OBJECT_CLASS (marker_sketcher_window_parent_class)->dispose (object);
}

static void
marker_sketcher_window_finalize (GObject *object)
{
  MarkerSketcherWindow *w = MARKER_SKETCHER_WINDOW (object);

  g_clear_pointer (&w->base_file, g_free);

  G_OBJECT_CLASS (marker_sketcher_window_parent_class)->finalize (object);
}

static void
marker_sketcher_window_class_init (MarkerSketcherWindowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  object_class->dispose = marker_sketcher_window_dispose;
  object_class->finalize = marker_sketcher_window_finalize;
}

static void
marker_sketcher_window_init (MarkerSketcherWindow *sketcher)
{
  sketcher->surface = NULL;
  sketcher->history = NULL;
  sketcher->future = NULL;
  sketcher->status = FALSE;
  sketcher->pos_x = 0;
  sketcher->pos_y = 0;
  sketcher->tool = PEN;
  sketcher->size = P_SIZE_M;

  sketcher->color.alpha = 1.0;
  sketcher->color.blue = 0.0;
  sketcher->color.green = 0.0;
  sketcher->color.red = 0.0;

  init_ui (sketcher);
}
