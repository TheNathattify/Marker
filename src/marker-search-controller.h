/*
 * marker-search-controller.h
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

#ifndef __MARKER_SEARCH_CONTROLLER_H__
#define __MARKER_SEARCH_CONTROLLER_H__

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define MARKER_TYPE_SEARCH_CONTROLLER (marker_search_controller_get_type ())

G_DECLARE_FINAL_TYPE (MarkerSearchController, marker_search_controller, MARKER, SEARCH_CONTROLLER, GObject)

MarkerSearchController *marker_search_controller_new               (GtkSearchBar           *search_bar,
                                                                    GtkSearchEntry         *search_entry,
                                                                    GtkSourceView          *source_view);
void                    marker_search_controller_setup_replace     (MarkerSearchController *self,
                                                                    GtkSearchEntry         *replace_entry,
                                                                    GtkButton              *replace_button,
                                                                    GtkButton              *replace_all_button,
                                                                    GtkToggleButton        *replace_toggle_button);
void                    marker_search_controller_set_replace_box   (MarkerSearchController *self,
                                                                    GtkWidget              *replace_box);
void                    marker_search_controller_toggle            (MarkerSearchController *self);
void                    marker_search_controller_find_next         (MarkerSearchController *self);
void                    marker_search_controller_find_previous     (MarkerSearchController *self);
void                    marker_search_controller_replace_next      (MarkerSearchController *self);
void                    marker_search_controller_replace_all       (MarkerSearchController *self);
void                    marker_search_controller_set_replace_visible (MarkerSearchController *self,
                                                                      gboolean                visible);
gboolean                marker_search_controller_get_replace_visible (MarkerSearchController *self);
void                    marker_search_controller_set_search_text   (MarkerSearchController *self,
                                                                    const gchar            *text);
void                    marker_search_controller_set_replace_text  (MarkerSearchController *self,
                                                                    const gchar            *text);
GtkSearchBar           *marker_search_controller_get_search_bar    (MarkerSearchController *self);
GtkSearchEntry         *marker_search_controller_get_search_entry  (MarkerSearchController *self);
GtkSearchEntry         *marker_search_controller_get_replace_entry (MarkerSearchController *self);
GtkToggleButton        *marker_search_controller_get_replace_toggle_button (MarkerSearchController *self);

G_END_DECLS

#endif /* __MARKER_SEARCH_CONTROLLER_H__ */
