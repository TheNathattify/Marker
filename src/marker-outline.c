/*
 * marker-outline.c
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

#include "marker-outline.h"
#include <string.h>

struct _MarkerOutlineItem
{
  GObject parent_instance;

  gchar *title;
  guint  level;
  guint  line_number;
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LEVEL,
  PROP_LINE_NUMBER,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (MarkerOutlineItem, marker_outline_item, G_TYPE_OBJECT)

static void
marker_outline_item_finalize (GObject *object)
{
  MarkerOutlineItem *self = MARKER_OUTLINE_ITEM (object);

  g_free (self->title);

  G_OBJECT_CLASS (marker_outline_item_parent_class)->finalize (object);
}

static void
marker_outline_item_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MarkerOutlineItem *self = MARKER_OUTLINE_ITEM (object);

  switch (prop_id)
  {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_LEVEL:
      g_value_set_uint (value, self->level);
      break;
    case PROP_LINE_NUMBER:
      g_value_set_uint (value, self->line_number);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
marker_outline_item_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MarkerOutlineItem *self = MARKER_OUTLINE_ITEM (object);

  switch (prop_id)
  {
    case PROP_TITLE:
      g_free (self->title);
      self->title = g_value_dup_string (value);
      break;
    case PROP_LEVEL:
      self->level = g_value_get_uint (value);
      break;
    case PROP_LINE_NUMBER:
      self->line_number = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
marker_outline_item_class_init (MarkerOutlineItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = marker_outline_item_finalize;
  object_class->get_property = marker_outline_item_get_property;
  object_class->set_property = marker_outline_item_set_property;

  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "Heading title",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  properties[PROP_LEVEL] =
    g_param_spec_uint ("level",
                       "Level",
                       "Heading level 1 to 6",
                       1, 6, 1,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  properties[PROP_LINE_NUMBER] =
    g_param_spec_uint ("line-number",
                       "Line Number",
                       "Line number in document (0-indexed)",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
marker_outline_item_init (MarkerOutlineItem *self)
{
  self->title = NULL;
  self->level = 1;
  self->line_number = 0;
}

MarkerOutlineItem *
marker_outline_item_new (const gchar *title,
                         guint        level,
                         guint        line_number)
{
  return g_object_new (MARKER_TYPE_OUTLINE_ITEM,
                       "title", title,
                       "level", level,
                       "line-number", line_number,
                       NULL);
}

const gchar *
marker_outline_item_get_title (MarkerOutlineItem *item)
{
  g_return_val_if_fail (MARKER_IS_OUTLINE_ITEM (item), NULL);
  return item->title;
}

guint
marker_outline_item_get_level (MarkerOutlineItem *item)
{
  g_return_val_if_fail (MARKER_IS_OUTLINE_ITEM (item), 1);
  return item->level;
}

guint
marker_outline_item_get_line_number (MarkerOutlineItem *item)
{
  g_return_val_if_fail (MARKER_IS_OUTLINE_ITEM (item), 0);
  return item->line_number;
}

static gboolean
is_code_fence (const gchar *line, gchar *fence_char_out, guint *fence_len_out)
{
  const gchar *p = line;
  guint leading_spaces = 0;

  while (*p == ' ')
  {
    leading_spaces++;
    p++;
  }

  if (leading_spaces > 3)
    return FALSE;

  if (*p == '`' || *p == '~')
  {
    gchar c = *p;
    guint count = 0;
    while (*p == c)
    {
      count++;
      p++;
    }
    if (count >= 3)
    {
      if (fence_char_out)
        *fence_char_out = c;
      if (fence_len_out)
        *fence_len_out = count;
      return TRUE;
    }
  }

  return FALSE;
}

static gboolean
check_atx_header (const gchar  *line,
                  gchar       **out_title,
                  guint        *out_level)
{
  const gchar *p = line;
  guint leading_spaces = 0;

  while (*p == ' ')
  {
    leading_spaces++;
    p++;
  }

  if (leading_spaces > 3)
    return FALSE;

  if (*p != '#')
    return FALSE;

  guint hashes = 0;
  while (*p == '#')
  {
    hashes++;
    p++;
  }

  if (hashes < 1 || hashes > 6)
    return FALSE;

  /* Must be followed by space, tab, or end of line */
  if (*p != ' ' && *p != '\t' && *p != '\0' && *p != '\r')
    return FALSE;

  while (*p == ' ' || *p == '\t')
    p++;

  /* Copy title and strip trailing whitespace and optional trailing hashes */
  gchar *raw_title = g_strdup (p);
  g_strchomp (raw_title);

  /* Check for trailing hashes */
  gint len = (gint) strlen (raw_title);
  gint end = len - 1;
  while (end >= 0 && raw_title[end] == '#')
    end--;

  /* If trailing hashes are preceded by space or reach beginning */
  if (end < len - 1 && (end < 0 || raw_title[end] == ' ' || raw_title[end] == '\t'))
  {
    raw_title[end + 1] = '\0';
    g_strchomp (raw_title);
  }

  if (out_title)
    *out_title = raw_title;
  else
    g_free (raw_title);

  if (out_level)
    *out_level = hashes;

  return TRUE;
}

static gboolean
check_setext_underline (const gchar *line,
                        guint       *out_level)
{
  const gchar *p = line;
  guint leading_spaces = 0;

  while (*p == ' ')
  {
    leading_spaces++;
    p++;
  }

  if (leading_spaces > 3)
    return FALSE;

  if (*p == '=')
  {
    guint count = 0;
    while (*p == '=')
    {
      count++;
      p++;
    }
    while (*p == ' ' || *p == '\t' || *p == '\r')
      p++;
    if (*p == '\0' && count >= 1)
    {
      if (out_level)
        *out_level = 1;
      return TRUE;
    }
  }
  else if (*p == '-')
  {
    guint count = 0;
    while (*p == '-')
    {
      count++;
      p++;
    }
    while (*p == ' ' || *p == '\t' || *p == '\r')
      p++;
    if (*p == '\0' && count >= 1)
    {
      if (out_level)
        *out_level = 2;
      return TRUE;
    }
  }

  return FALSE;
}

GListStore *
marker_outline_parse_markdown (const gchar *markdown_text)
{
  GListStore *store = g_list_store_new (MARKER_TYPE_OUTLINE_ITEM);

  if (!markdown_text || *markdown_text == '\0')
    return store;

  gchar **lines = g_strsplit (markdown_text, "\n", -1);
  if (!lines)
    return store;

  gboolean in_code_block = FALSE;
  gchar code_block_char = 0;
  guint code_block_len = 0;

  gchar *prev_line = NULL;
  guint prev_line_num = 0;

  for (guint i = 0; lines[i] != NULL; i++)
  {
    const gchar *line = lines[i];

    /* Check code block fence */
    gchar fence_char = 0;
    guint fence_len = 0;
    if (is_code_fence (line, &fence_char, &fence_len))
    {
      if (!in_code_block)
      {
        in_code_block = TRUE;
        code_block_char = fence_char;
        code_block_len = fence_len;
      }
      else if (fence_char == code_block_char && fence_len >= code_block_len)
      {
        in_code_block = FALSE;
        code_block_char = 0;
        code_block_len = 0;
      }

      g_clear_pointer (&prev_line, g_free);
      continue;
    }

    if (in_code_block)
    {
      g_clear_pointer (&prev_line, g_free);
      continue;
    }

    /* Check ATX header */
    gchar *atx_title = NULL;
    guint atx_level = 0;
    if (check_atx_header (line, &atx_title, &atx_level))
    {
      MarkerOutlineItem *item = marker_outline_item_new (atx_title, atx_level, i);
      g_list_store_append (store, item);
      g_object_unref (item);
      g_free (atx_title);

      g_clear_pointer (&prev_line, g_free);
      continue;
    }

    /* Check Setext header */
    guint setext_level = 0;
    if (prev_line != NULL && check_setext_underline (line, &setext_level))
    {
      gchar *trimmed_title = g_strdup (prev_line);
      g_strstrip (trimmed_title);

      if (*trimmed_title != '\0')
      {
        MarkerOutlineItem *item = marker_outline_item_new (trimmed_title, setext_level, prev_line_num);
        g_list_store_append (store, item);
        g_object_unref (item);
      }
      g_free (trimmed_title);

      g_clear_pointer (&prev_line, g_free);
      continue;
    }

    /* Check if current line is blank */
    const gchar *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\r')
      p++;

    if (*p == '\0')
    {
      g_clear_pointer (&prev_line, g_free);
    }
    else
    {
      g_clear_pointer (&prev_line, g_free);
      prev_line = g_strdup (line);
      prev_line_num = i;
    }
  }

  g_clear_pointer (&prev_line, g_free);
  g_strfreev (lines);

  return store;
}
