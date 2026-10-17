/*
 * marker-document.c
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

#include "marker-document.h"
#include "marker-prefs.h"
#include "marker-string.h"

#include <glib/gi18n.h>

struct _MarkerDocument
{
  GObject       parent_instance;

  GFile        *file;
  GFileMonitor *file_monitor;
  gint64        last_mtime_usec;
  gchar        *untitled_id;
  gboolean      is_modified;
  gboolean      is_saving;
};

G_DEFINE_TYPE (MarkerDocument, marker_document, G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_FILE,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_IS_MODIFIED,
  N_PROPS
};

static GParamSpec *properties[N_PROPS] = { NULL, };

enum
{
  SIGNAL_EXTERNAL_CHANGE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

static gint64
marker_document_query_mtime_usec (GFile *file)
{
  if (file == NULL)
    return 0;

  g_autoptr(GFileInfo) info = g_file_query_info (file,
                                                 G_FILE_ATTRIBUTE_TIME_MODIFIED "," G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 NULL,
                                                 NULL);
  if (info == NULL)
    return 0;

  guint64 sec = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
  guint32 usec = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);

  return (gint64) sec * G_USEC_PER_SEC + (gint64) usec;
}

static void
on_file_monitor_changed (GFileMonitor      *monitor,
                         GFile             *file,
                         GFile             *other_file,
                         GFileMonitorEvent  event_type,
                         gpointer           user_data)
{
  (void) monitor;
  (void) file;
  (void) other_file;
  MarkerDocument *self = MARKER_DOCUMENT (user_data);

  g_return_if_fail (MARKER_IS_DOCUMENT (self));

  if (self->is_saving)
    return;

  if (event_type == G_FILE_MONITOR_EVENT_CHANGED ||
      event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
      event_type == G_FILE_MONITOR_EVENT_CREATED)
  {
    if (self->file == NULL)
      return;

    gint64 current_mtime = marker_document_query_mtime_usec (self->file);
    if (current_mtime != 0 && current_mtime == self->last_mtime_usec)
    {
      return;
    }

    self->last_mtime_usec = current_mtime;
    g_signal_emit (self, signals[SIGNAL_EXTERNAL_CHANGE], 0);
  }
}

static void
marker_document_setup_file_monitor (MarkerDocument *self)
{
  if (self->file_monitor != NULL)
  {
    g_file_monitor_cancel (self->file_monitor);
    g_clear_object (&self->file_monitor);
  }

  if (self->file != NULL)
  {
    g_autoptr(GError) error = NULL;
    self->file_monitor = g_file_monitor_file (self->file,
                                              G_FILE_MONITOR_NONE,
                                              NULL,
                                              &error);
    if (self->file_monitor != NULL)
    {
      g_signal_connect (self->file_monitor,
                        "changed",
                        G_CALLBACK (on_file_monitor_changed),
                        self);
    }
    else if (error != NULL)
    {
      g_warning ("Failed to create file monitor: %s", error->message);
    }
  }
}

static void
marker_document_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MarkerDocument *self = MARKER_DOCUMENT (object);

  switch (prop_id)
  {
    case PROP_FILE:
      g_value_set_object (value, marker_document_get_file (self));
      break;

    case PROP_TITLE:
      g_value_take_string (value, marker_document_get_title (self));
      break;

    case PROP_SUBTITLE:
      g_value_take_string (value, marker_document_get_subtitle (self));
      break;

    case PROP_IS_MODIFIED:
      g_value_set_boolean (value, marker_document_get_is_modified (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
marker_document_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MarkerDocument *self = MARKER_DOCUMENT (object);

  switch (prop_id)
  {
    case PROP_FILE:
      marker_document_set_file (self, g_value_get_object (value));
      break;

    case PROP_IS_MODIFIED:
      marker_document_set_is_modified (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
marker_document_dispose (GObject *object)
{
  MarkerDocument *self = MARKER_DOCUMENT (object);

  marker_document_remove_draft (self, NULL);

  if (self->file_monitor != NULL)
  {
    g_file_monitor_cancel (self->file_monitor);
    g_clear_object (&self->file_monitor);
  }

  g_clear_object (&self->file);
  g_clear_pointer (&self->untitled_id, g_free);
  self->last_mtime_usec = 0;

  G_OBJECT_CLASS (marker_document_parent_class)->dispose (object);
}

static void
marker_document_class_init (MarkerDocumentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = marker_document_get_property;
  object_class->set_property = marker_document_set_property;
  object_class->dispose = marker_document_dispose;

  properties[PROP_FILE] =
    g_param_spec_object ("file",
                         "File",
                         "The underlying file for the document",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title of the document",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SUBTITLE] =
    g_param_spec_string ("subtitle",
                         "Subtitle",
                         "The subtitle/path of the document",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_IS_MODIFIED] =
    g_param_spec_boolean ("is-modified",
                          "Is Modified",
                          "Whether the document has unsaved modifications",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals[SIGNAL_EXTERNAL_CHANGE] =
    g_signal_new ("external-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

static void
marker_document_init (MarkerDocument *self)
{
  self->file = NULL;
  self->file_monitor = NULL;
  self->last_mtime_usec = 0;
  self->untitled_id = g_uuid_string_random ();
  self->is_modified = FALSE;
  self->is_saving = FALSE;
}

MarkerDocument *
marker_document_new (void)
{
  return g_object_new (MARKER_TYPE_DOCUMENT, NULL);
}

MarkerDocument *
marker_document_new_for_file (GFile *file)
{
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  return g_object_new (MARKER_TYPE_DOCUMENT,
                       "file", file,
                       NULL);
}

GFile *
marker_document_get_file (MarkerDocument *self)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), NULL);

  return self->file;
}

void
marker_document_set_file (MarkerDocument *self,
                          GFile          *file)
{
  g_return_if_fail (MARKER_IS_DOCUMENT (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (self->file == file)
    return;

  if (self->file != NULL && file != NULL && g_file_equal (self->file, file))
    return;

  g_set_object (&self->file, file);
  self->last_mtime_usec = marker_document_query_mtime_usec (self->file);
  marker_document_setup_file_monitor (self);

  g_object_freeze_notify (G_OBJECT (self));
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SUBTITLE]);
  g_object_thaw_notify (G_OBJECT (self));
}

gchar *
marker_document_get_title (MarkerDocument *self)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), NULL);

  g_autofree gchar *base_name = NULL;

  if (self->file != NULL)
  {
    base_name = g_file_get_basename (self->file);
  }
  else
  {
    base_name = g_strdup (_("Untitled.md"));
  }

  if (self->is_modified)
  {
    return g_strdup_printf ("*%s", base_name ? base_name : "");
  }

  return g_strdup (base_name ? base_name : "");
}

gchar *
marker_document_get_subtitle (MarkerDocument *self)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), NULL);

  if (self->file == NULL)
    return NULL;

  g_autofree gchar *path = g_file_get_path (self->file);
  if (path != NULL)
  {
    return marker_string_filename_get_path (path);
  }

  g_autoptr(GFile) parent = g_file_get_parent (self->file);
  if (parent != NULL)
  {
    return g_file_get_parse_name (parent);
  }

  return NULL;
}

gboolean
marker_document_get_is_modified (MarkerDocument *self)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), FALSE);

  return self->is_modified;
}

void
marker_document_set_is_modified (MarkerDocument *self,
                                 gboolean        is_modified)
{
  g_return_if_fail (MARKER_IS_DOCUMENT (self));

  if (self->is_modified != is_modified)
  {
    self->is_modified = is_modified;

    g_object_freeze_notify (G_OBJECT (self));
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_MODIFIED]);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
    g_object_thaw_notify (G_OBJECT (self));
  }
}

gchar *
marker_document_load_contents (MarkerDocument  *self,
                               gsize           *out_size,
                               GError         **error)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), NULL);

  if (self->file == NULL)
  {
    g_set_error_literal (error,
                         G_IO_ERROR,
                         G_IO_ERROR_NOT_FOUND,
                         "No file set for document");
    return NULL;
  }

  gchar *contents = NULL;
  gsize length = 0;

  if (!g_file_load_contents (self->file, NULL, &contents, &length, NULL, error))
    return NULL;

  self->last_mtime_usec = marker_document_query_mtime_usec (self->file);

  if (out_size != NULL)
    *out_size = length;

  marker_document_set_is_modified (self, FALSE);

  return contents;
}

static gchar *
trim_trailing_whitespace_from_text (const gchar *text)
{
  if (text == NULL)
    return NULL;

  GString *result = g_string_sized_new (strlen (text));
  const gchar *p = text;

  while (*p != '\0')
  {
    const gchar *line_start = p;
    while (*p != '\0' && *p != '\n' && *p != '\r')
      p++;

    const gchar *line_end = p;
    while (line_end > line_start && (*(line_end - 1) == ' ' || *(line_end - 1) == '\t'))
      line_end--;

    g_string_append_len (result, line_start, line_end - line_start);

    if (*p == '\r' && *(p + 1) == '\n')
    {
      g_string_append (result, "\r\n");
      p += 2;
    }
    else if (*p == '\r')
    {
      g_string_append_c (result, '\r');
      p++;
    }
    else if (*p == '\n')
    {
      g_string_append_c (result, '\n');
      p++;
    }
  }

  return g_string_free (result, FALSE);
}

static gchar *
marker_document_get_draft_path (MarkerDocument *self)
{
  if (self == NULL)
    return NULL;

  g_autofree gchar *ident = NULL;
  if (self->file != NULL)
  {
    ident = g_file_get_uri (self->file);
  }
  else if (self->untitled_id != NULL)
  {
    ident = g_strdup (self->untitled_id);
  }

  if (ident == NULL)
    return NULL;

  g_autofree gchar *hash = g_compute_checksum_for_string (G_CHECKSUM_SHA256, ident, -1);
  const gchar *cache_dir = g_get_user_cache_dir ();
  g_autofree gchar *filename = g_strdup_printf ("%s.md", hash);

  return g_build_filename (cache_dir, "marker", "drafts", filename, NULL);
}

gboolean
marker_document_save_draft (MarkerDocument  *self,
                            const gchar     *contents,
                            gsize            length,
                            GError         **error)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  if (length == (gsize) -1)
    length = strlen (contents);

  g_autofree gchar *draft_path = marker_document_get_draft_path (self);
  if (draft_path == NULL)
  {
    g_set_error_literal (error,
                         G_IO_ERROR,
                         G_IO_ERROR_FAILED,
                         "Could not determine draft path");
    return FALSE;
  }

  const gchar *cache_dir = g_get_user_cache_dir ();
  g_autofree gchar *drafts_dir = g_build_filename (cache_dir, "marker", "drafts", NULL);
  if (g_mkdir_with_parents (drafts_dir, 0700) != 0)
  {
    g_set_error_literal (error,
                         G_IO_ERROR,
                         G_IO_ERROR_FAILED,
                         "Could not create drafts cache directory");
    return FALSE;
  }

  g_autoptr(GFile) draft_file = g_file_new_for_path (draft_path);
  return g_file_replace_contents (draft_file,
                                  contents,
                                  length,
                                  NULL,
                                  FALSE,
                                  G_FILE_CREATE_NONE,
                                  NULL,
                                  NULL,
                                  error);
}

gboolean
marker_document_remove_draft (MarkerDocument  *self,
                              GError         **error)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), FALSE);

  g_autofree gchar *draft_path = marker_document_get_draft_path (self);
  if (draft_path == NULL)
    return TRUE;

  g_autoptr(GFile) draft_file = g_file_new_for_path (draft_path);
  if (!g_file_query_exists (draft_file, NULL))
    return TRUE;

  return g_file_delete (draft_file, NULL, error);
}

gboolean
marker_document_save_contents (MarkerDocument  *self,
                               const gchar     *contents,
                               gsize            length,
                               GError         **error)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  if (self->file == NULL)
  {
    g_set_error_literal (error,
                         G_IO_ERROR,
                         G_IO_ERROR_NOT_FOUND,
                         "No file set for document");
    return FALSE;
  }

  g_autofree gchar *trimmed = NULL;
  g_autofree gchar *with_newline = NULL;
  const gchar *data_to_save = contents;
  gsize data_length = (length == (gsize) -1) ? strlen (contents) : length;

  if (marker_prefs_get_trim_trailing_whitespace ())
  {
    trimmed = trim_trailing_whitespace_from_text (contents);
    data_to_save = trimmed;
    data_length = strlen (trimmed);
  }

  if (data_length > 0 && data_to_save[data_length - 1] != '\n')
  {
    with_newline = g_malloc (data_length + 2);
    memcpy (with_newline, data_to_save, data_length);
    with_newline[data_length] = '\n';
    with_newline[data_length + 1] = '\0';
    data_to_save = with_newline;
    data_length += 1;
  }

  self->is_saving = TRUE;

  gboolean success = g_file_replace_contents (self->file,
                                              data_to_save,
                                              data_length,
                                              NULL,
                                              FALSE,
                                              G_FILE_CREATE_NONE,
                                              NULL,
                                              NULL,
                                              error);

  if (success)
  {
    self->last_mtime_usec = marker_document_query_mtime_usec (self->file);
  }

  self->is_saving = FALSE;

  if (success)
  {
    marker_document_set_is_modified (self, FALSE);
    marker_document_remove_draft (self, NULL);
  }

  return success;
}

gboolean
marker_document_save_as (MarkerDocument  *self,
                         GFile           *new_file,
                         const gchar     *contents,
                         gsize            length,
                         GError         **error)
{
  g_return_val_if_fail (MARKER_IS_DOCUMENT (self), FALSE);
  g_return_val_if_fail (G_IS_FILE (new_file), FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  g_autofree gchar *trimmed = NULL;
  g_autofree gchar *with_newline = NULL;
  const gchar *data_to_save = contents;
  gsize data_length = (length == (gsize) -1) ? strlen (contents) : length;

  if (marker_prefs_get_trim_trailing_whitespace ())
  {
    trimmed = trim_trailing_whitespace_from_text (contents);
    data_to_save = trimmed;
    data_length = strlen (trimmed);
  }

  if (data_length > 0 && data_to_save[data_length - 1] != '\n')
  {
    with_newline = g_malloc (data_length + 2);
    memcpy (with_newline, data_to_save, data_length);
    with_newline[data_length] = '\n';
    with_newline[data_length + 1] = '\0';
    data_to_save = with_newline;
    data_length += 1;
  }

  gboolean success = g_file_replace_contents (new_file,
                                              data_to_save,
                                              data_length,
                                              NULL,
                                              FALSE,
                                              G_FILE_CREATE_NONE,
                                              NULL,
                                              NULL,
                                              error);

  if (success)
  {
    marker_document_remove_draft (self, NULL);
    marker_document_set_file (self, new_file);
    self->last_mtime_usec = marker_document_query_mtime_usec (self->file);
    marker_document_set_is_modified (self, FALSE);
    marker_document_remove_draft (self, NULL);
  }

  return success;
}
