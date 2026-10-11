/*
 * marker-string.c
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

#include <glib.h>
#include <string.h>

#include "marker-string.h"

int
marker_string_ends_with(const char* str,
                        const char* sub_str)
{
  if (str == NULL || sub_str == NULL)
  {
    return 0;
  }
  return g_str_has_suffix (str, sub_str) ? 1 : 0;
}

int
marker_string_contains(const char   *str,
                       const char   *sub_str)
{
  if (str == NULL || sub_str == NULL)
  {
    return 0;
  }
  return (strstr (str, sub_str) != NULL) ? 1 : 0;
}

char*
marker_string_alloc(const char* str)
{
  if (str == NULL)
  {
    return NULL;
  }
  return g_strdup (str);
}

char*
marker_string_prepend(const char* str,
                      const char* addition,
                      char*       buffer,
                      size_t      buffer_size)
{
  if (str == NULL || addition == NULL)
  {
    return NULL;
  }
  if (buffer != NULL && buffer_size > 0)
  {
    g_snprintf (buffer, buffer_size, "%s%s", addition, str);
    return buffer;
  }
  return g_strconcat (addition, str, NULL);
}

char*
marker_string_append(const char* str,
                     const char* addition,
                     char*       buffer,
                     size_t      buffer_size)
{
  if (str == NULL || addition == NULL)
  {
    return NULL;
  }
  if (buffer != NULL && buffer_size > 0)
  {
    g_snprintf (buffer, buffer_size, "%s%s", str, addition);
    return buffer;
  }
  return g_strconcat (str, addition, NULL);
}

int
marker_string_buffer_set(const char* str,
                         char*       buffer,
                         size_t      buffer_size)
{
  if (str == NULL || buffer == NULL || buffer_size == 0)
  {
    return 1;
  }
  size_t written = g_strlcpy (buffer, str, buffer_size);
  return (written >= buffer_size) ? 1 : 0;
}

char*
marker_string_escape(const char* str)
{
  if (str == NULL)
  {
    return NULL;
  }
  GString *gs = g_string_new (NULL);
  for (const char *p = str; *p != '\0'; p++)
  {
    if (*p == ' ')
    {
      g_string_append_c (gs, '\\');
    }
    g_string_append_c (gs, *p);
  }
  return g_string_free (gs, FALSE);
}

char*
marker_string_filename_get_name(const char* filename)
{
  if (filename == NULL)
  {
    return NULL;
  }
  return g_path_get_basename (filename);
}

char*
marker_string_filename_get_name_noext(const char* filename)
{
  if (filename == NULL)
  {
    return NULL;
  }
  g_autofree gchar *base = g_path_get_basename (filename);
  char *dot = strrchr (base, '.');
  if (dot != NULL && dot != base)
  {
    *dot = '\0';
  }
  return g_strdup (base);
}

char*
marker_string_filename_get_path(const char* filename)
{
  if (filename == NULL)
  {
    return NULL;
  }
  return g_path_get_dirname (filename);
}

