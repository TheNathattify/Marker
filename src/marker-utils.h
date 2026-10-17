/*
 * marker-utils.h
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

#ifndef __MARKER_UTILS_H__
#define __MARKER_UTILS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

gchar               *marker_utils_read_file                      (const gchar        *infile,
                                                                  long               *out_size);
void                 marker_utils_surround_selection_with        (GtkTextBuffer      *buffer,
                                                                  const gchar        *insertion);
int                  marker_utils_rfind                          (char                query,
                                                                  const gchar        *str);
int                  marker_utils_get_current_time_seconds       (void);
int                  marker_utils_str_starts_with                (const gchar        *str,
                                                                  const gchar        *sub_str);
int                  marker_utils_str_ends_with                  (const gchar        *str,
                                                                  const gchar        *sub_str);
gchar               *marker_utils_combo_box_get_active_str       (GtkComboBox        *combo_box);
void                 marker_utils_combo_box_set_model            (GtkComboBox        *combo_box,
                                                                  GtkTreeModel       *model);
gchar               *marker_utils_allocate_string                (const gchar        *str);
gchar               *marker_utils_escape_file_path               (const gchar        *filename);
gboolean             marker_utils_is_url                         (const gchar        *str);
const gchar         *marker_utils_get_scripts_dir                (void);
const gchar         *marker_utils_get_styles_dir                 (void);
const gchar         *marker_utils_get_user_styles_dir            (void);
const gchar         *marker_utils_get_highlight_styles_dir       (void);

G_END_DECLS

#endif
