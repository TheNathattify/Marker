/*
 * marker-document.h
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

#ifndef __MARKER_DOCUMENT_H__
#define __MARKER_DOCUMENT_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define MARKER_TYPE_DOCUMENT (marker_document_get_type ())

G_DECLARE_FINAL_TYPE (MarkerDocument, marker_document, MARKER, DOCUMENT, GObject)

MarkerDocument *marker_document_new              (void);
MarkerDocument *marker_document_new_for_file     (GFile          *file);

GFile          *marker_document_get_file         (MarkerDocument *self);
void            marker_document_set_file         (MarkerDocument *self,
                                                  GFile          *file);

gchar          *marker_document_get_title        (MarkerDocument *self);
gchar          *marker_document_get_subtitle     (MarkerDocument *self);

gboolean        marker_document_get_is_modified  (MarkerDocument *self);
void            marker_document_set_is_modified  (MarkerDocument *self,
                                                  gboolean        is_modified);

gchar          *marker_document_load_contents    (MarkerDocument *self,
                                                  gsize          *out_size,
                                                  GError        **error);
gboolean        marker_document_save_contents    (MarkerDocument *self,
                                                  const gchar    *contents,
                                                  gsize           length,
                                                  GError        **error);
gboolean        marker_document_save_as          (MarkerDocument *self,
                                                  GFile          *new_file,
                                                  const gchar    *contents,
                                                  gsize           length,
                                                  GError        **error);
gboolean        marker_document_save_draft       (MarkerDocument *self,
                                                  const gchar    *contents,
                                                  gsize           length,
                                                  GError        **error);
gboolean        marker_document_remove_draft     (MarkerDocument *self,
                                                  GError        **error);

G_END_DECLS

#endif /* __MARKER_DOCUMENT_H__ */
