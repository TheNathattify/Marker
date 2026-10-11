/*
 * marker-outline.h
 *
 * Copyright (C) 2026 Marker Developers
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

#ifndef __MARKER_OUTLINE_H__
#define __MARKER_OUTLINE_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define MARKER_TYPE_OUTLINE_ITEM (marker_outline_item_get_type ())

G_DECLARE_FINAL_TYPE (MarkerOutlineItem, marker_outline_item, MARKER, OUTLINE_ITEM, GObject)

MarkerOutlineItem *marker_outline_item_new             (const gchar *title,
                                                        guint        level,
                                                        guint        line_number);

const gchar       *marker_outline_item_get_title       (MarkerOutlineItem *item);
guint              marker_outline_item_get_level       (MarkerOutlineItem *item);
guint              marker_outline_item_get_line_number (MarkerOutlineItem *item);

GListStore        *marker_outline_parse_markdown       (const gchar *markdown_text);

G_END_DECLS

#endif /* __MARKER_OUTLINE_H__ */
