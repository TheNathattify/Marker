/*
 * marker-exporter.c
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>

#include "marker.h"
#include "marker-utils.h"
#include "marker-string.h"
#include "marker-markdown.h"
#include "marker-prefs.h"
#include "marker-preview.h"
#include "marker-editor.h"

#include "marker-exporter.h"

MarkerExportFormat
marker_exporter_str_to_fmt (const char *str)
{
  if (str == NULL)
  {
    return HTML;
  }

  if (strstr (str, "PDF") != NULL || strcmp (str, "PDF") == 0)
  {
    return PDF;
  }

  if (strstr (str, "RTF") != NULL || strcmp (str, "RTF") == 0)
  {
    return RTF;
  }

  if (strstr (str, "ODT") != NULL || strcmp (str, "ODT") == 0)
  {
    return ODT;
  }

  if (strstr (str, "DOCX") != NULL || strcmp (str, "DOCX") == 0)
  {
    return DOCX;
  }

  if (strstr (str, "LATEX") != NULL || strstr (str, "LaTeX") != NULL || strstr (str, "tex") != NULL)
  {
    return LATEX;
  }

  return HTML;
}

void
marker_exporter_export_pandoc (const char *markdown,
                               const char *stylesheet_path,
                               const char *outfile)
{
  if (markdown == NULL || outfile == NULL)
  {
    return;
  }

  g_autofree gchar *dir = g_path_get_dirname (outfile);
  g_autoptr(GSubprocessLauncher) launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE);
  if (dir != NULL)
  {
    g_subprocess_launcher_set_cwd (launcher, dir);
  }

  const gchar *argv[] = {
    "pandoc",
    "-s",
    "-c", (stylesheet_path != NULL) ? stylesheet_path : "",
    "-o", outfile,
    "-",
    NULL
  };

  g_autoptr(GError) error = NULL;
  g_autoptr(GSubprocess) proc = g_subprocess_launcher_spawnv (launcher, argv, &error);
  if (proc != NULL)
  {
    g_subprocess_communicate_utf8 (proc, markdown, NULL, NULL, NULL, &error);
  }
}

static void
on_export_file_dialog_cb (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
  MarkerWindow *window = MARKER_WINDOW (user_data);
  g_autoptr(GError) error = NULL;

  g_autoptr(GFile) file = gtk_file_dialog_save_finish (dialog, res, &error);
  if (file == NULL)
  {
    if (error && !g_error_matches (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
    {
      g_warning ("Failed to export file: %s", error->message);
    }
    return;
  }

  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (editor == NULL)
    return;

  g_autofree gchar *filename = g_file_get_path (file);
  if (filename == NULL)
    return;

  MarkerSourceView *source_view = marker_editor_get_source_view (editor);
  if (source_view == NULL)
    return;

  g_autofree gchar *markdown = marker_source_view_get_text (source_view, false);
  if (markdown == NULL)
    return;

  g_autofree gchar *stylesheet_path = marker_prefs_get_css_theme ();

  GFile *source = marker_editor_get_file (editor);
  g_autofree gchar *base_folder = NULL;
  if (source != NULL)
  {
    g_autoptr(GFile) parent = g_file_get_parent (source);
    if (parent != NULL)
      base_folder = g_file_get_path (parent);
  }

  size_t len = strlen (markdown);
  metadata *meta = marker_markdown_metadata (markdown, len);
  enum scidown_paper_size paper_size = B43;
  GtkPageOrientation orientation = GTK_PAGE_ORIENTATION_PORTRAIT;
  if (meta != NULL)
  {
    paper_size = meta->paper_size;
    if (meta->doc_class == CLASS_BEAMER && !(paper_size == B43 || paper_size == B169))
      paper_size = B43;
    orientation = meta->doc_class == CLASS_BEAMER ? GTK_PAGE_ORIENTATION_LANDSCAPE : GTK_PAGE_ORIENTATION_PORTRAIT;
  }

  MarkerExportFormat fmt = HTML;
  if (marker_string_ends_with (filename, ".pdf"))
  {
    fmt = PDF;
  }
  else if (marker_string_ends_with (filename, ".tex"))
  {
    fmt = LATEX;
  }
  else if (marker_string_ends_with (filename, ".rtf"))
  {
    fmt = RTF;
  }
  else if (marker_string_ends_with (filename, ".docx"))
  {
    fmt = DOCX;
  }
  else if (marker_string_ends_with (filename, ".odt"))
  {
    fmt = ODT;
  }
  else if (marker_string_ends_with (filename, ".html") || marker_string_ends_with (filename, ".htm"))
  {
    fmt = HTML;
  }
  else
  {
    GtkFileFilter *filter = gtk_file_dialog_get_default_filter (dialog);
    if (filter != NULL)
    {
      const gchar *file_type = gtk_file_filter_get_name (filter);
      fmt = marker_exporter_str_to_fmt (file_type);
    }

    const gchar *ext = NULL;
    switch (fmt)
    {
      case PDF:
        ext = ".pdf";
        break;
      case LATEX:
        ext = ".tex";
        break;
      case RTF:
        ext = ".rtf";
        break;
      case DOCX:
        ext = ".docx";
        break;
      case ODT:
        ext = ".odt";
        break;
      case HTML:
      default:
        ext = ".html";
        break;
    }

    if (ext != NULL)
    {
      gchar *new_filename = g_strconcat (filename, ext, NULL);
      g_free (filename);
      filename = new_filename;
    }
  }

  switch (fmt)
  {
    case HTML:
      marker_markdown_to_html_file_with_css_inline (markdown,
                                                    len,
                                                    base_folder,
                                                    (marker_prefs_get_use_mathjs ())
                                                      ? MATHJS_NET
                                                      : MATHJS_OFF,
                                                    (marker_prefs_get_use_highlight ())
                                                      ? HIGHLIGHT_NET
                                                      : HIGHLIGHT_OFF,
                                                    (marker_prefs_get_use_mermaid ())
                                                      ? MERMAID_NET
                                                      : MERMAID_OFF,
                                                    stylesheet_path,
                                                    filename);
      break;

    case PDF:
    {
      MarkerPreview *preview = marker_editor_get_preview (editor);
      marker_preview_print_pdf (preview, filename, paper_size, orientation);
      break;
    }

    case LATEX:
      marker_markdown_to_latex_file (markdown,
                                     len,
                                     base_folder,
                                     (marker_prefs_get_use_mathjs ())
                                       ? MATHJS_NET
                                       : MATHJS_OFF,
                                     (marker_prefs_get_use_highlight ())
                                       ? HIGHLIGHT_NET
                                       : HIGHLIGHT_OFF,
                                     (marker_prefs_get_use_mermaid ())
                                       ? MERMAID_NET
                                       : MERMAID_OFF,
                                     filename);
      break;

    default:
    {
      char *html = marker_markdown_to_html_with_css_inline (markdown,
                                                            len,
                                                            base_folder,
                                                            (marker_prefs_get_use_mathjs ())
                                                              ? MATHJS_NET
                                                              : MATHJS_OFF,
                                                            (marker_prefs_get_use_highlight ())
                                                              ? HIGHLIGHT_NET
                                                              : HIGHLIGHT_OFF,
                                                            (marker_prefs_get_use_mermaid ())
                                                              ? MERMAID_NET
                                                              : MERMAID_OFF,
                                                            stylesheet_path,
                                                            -1);
      if (html != NULL)
      {
        marker_exporter_export_pandoc (html, stylesheet_path, filename);
        free (html);
      }
      break;
    }
  }
}

void
marker_exporter_show_export_dialog (MarkerWindow *window)
{
  g_return_if_fail (MARKER_IS_WINDOW (window));

  MarkerEditor *editor = marker_window_get_active_editor (window);
  if (editor == NULL)
    return;

  g_autoptr(GtkFileDialog) dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _("Export"));

  g_autoptr(GListStore) filters = g_list_store_new (GTK_TYPE_FILE_FILTER);

  GtkFileFilter *filter_html = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_html, "HTML (*.html)");
  gtk_file_filter_add_pattern (filter_html, "*.html");
  g_list_store_append (filters, filter_html);

  GtkFileFilter *filter_pdf = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_pdf, "PDF (*.pdf)");
  gtk_file_filter_add_pattern (filter_pdf, "*.pdf");
  g_list_store_append (filters, filter_pdf);

  GtkFileFilter *filter_rtf = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_rtf, "RTF (*.rtf)");
  gtk_file_filter_add_pattern (filter_rtf, "*.rtf");
  g_list_store_append (filters, filter_rtf);

  GtkFileFilter *filter_docx = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_docx, "DOCX (*.docx)");
  gtk_file_filter_add_pattern (filter_docx, "*.docx");
  g_list_store_append (filters, filter_docx);

  GtkFileFilter *filter_odt = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_odt, "ODT (*.odt)");
  gtk_file_filter_add_pattern (filter_odt, "*.odt");
  g_list_store_append (filters, filter_odt);

  GtkFileFilter *filter_latex = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_latex, "LaTeX (*.tex)");
  gtk_file_filter_add_pattern (filter_latex, "*.tex");
  g_list_store_append (filters, filter_latex);

  g_object_unref (filter_html);
  g_object_unref (filter_pdf);
  g_object_unref (filter_rtf);
  g_object_unref (filter_docx);
  g_object_unref (filter_odt);
  g_object_unref (filter_latex);

  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));

  GFile *source = marker_editor_get_file (editor);
  if (source != NULL)
  {
    g_autofree gchar *basename = g_file_get_basename (source);
    if (basename != NULL)
    {
      g_autofree gchar *name_noext = marker_string_filename_get_name_noext (basename);
      if (name_noext != NULL && strlen (name_noext) > 0)
      {
        g_autofree gchar *default_name = g_strdup_printf ("%s.html", name_noext);
        gtk_file_dialog_set_initial_name (dialog, default_name);
      }
    }
    GFile *parent = g_file_get_parent (source);
    if (parent != NULL)
    {
      gtk_file_dialog_set_initial_folder (dialog, parent);
      g_object_unref (parent);
    }
  }

  gtk_file_dialog_save (dialog,
                        GTK_WINDOW (window),
                        NULL,
                        on_export_file_dialog_cb,
                        window);
}

typedef struct {
  GMainLoop *loop;
  MarkerPreview *preview;
  const gchar *outfile;
  enum scidown_paper_size paper_size;
  GtkPageOrientation orientation;
  guint timeout_id;
  gulong load_handler_id;
  gulong print_handler_id;
  gboolean printed;
} HeadlessPdfContext;

static void
on_headless_pdf_load_changed (WebKitWebView   *web_view,
                              WebKitLoadEvent  load_event,
                              gpointer         user_data)
{
  (void) web_view;
  HeadlessPdfContext *ctx = (HeadlessPdfContext *) user_data;
  if (load_event == WEBKIT_LOAD_FINISHED && !ctx->printed)
  {
    ctx->printed = TRUE;
    marker_preview_print_pdf (ctx->preview, ctx->outfile, ctx->paper_size, ctx->orientation);
  }
}

static void
on_headless_pdf_print_finished (MarkerPreview *preview,
                                gboolean       success,
                                gpointer       user_data)
{
  (void) preview;
  (void) success;
  HeadlessPdfContext *ctx = (HeadlessPdfContext *) user_data;
  if (ctx->loop != NULL && g_main_loop_is_running (ctx->loop))
  {
    g_main_loop_quit (ctx->loop);
  }
}

static gboolean
on_headless_pdf_timeout (gpointer user_data)
{
  HeadlessPdfContext *ctx = (HeadlessPdfContext *) user_data;
  g_warning ("PDF export timed out after 15 seconds");
  ctx->timeout_id = 0;
  if (ctx->loop != NULL && g_main_loop_is_running (ctx->loop))
  {
    g_main_loop_quit (ctx->loop);
  }
  return G_SOURCE_REMOVE;
}

void
marker_exporter_export (const gchar *infile,
                        const gchar *outfile)
{
  g_return_if_fail (infile != NULL && outfile != NULL);

  long len = 0;
  g_autofree gchar *markdown = marker_utils_read_file (infile, &len);
  if (markdown == NULL)
    return;

  g_autofree gchar *stylesheet = marker_prefs_get_css_theme ();
  g_autofree gchar *base_folder = marker_string_filename_get_path (infile);

  metadata *meta = marker_markdown_metadata (markdown, len);
  enum scidown_paper_size paper_size = B43;
  GtkPageOrientation orientation = GTK_PAGE_ORIENTATION_PORTRAIT;
  if (meta != NULL)
  {
    paper_size = meta->paper_size;
    if (meta->doc_class == CLASS_BEAMER && !(paper_size == B43 || paper_size == B169))
      paper_size = B43;
    orientation = meta->doc_class == CLASS_BEAMER ?
      GTK_PAGE_ORIENTATION_LANDSCAPE :
      GTK_PAGE_ORIENTATION_PORTRAIT;
  }

  if (marker_string_ends_with (outfile, ".html")) {
    marker_markdown_to_html_file_with_css_inline (markdown, len, base_folder,
                                                  (marker_prefs_get_use_mathjs ())
                                                    ? MATHJS_NET
                                                    : MATHJS_OFF,
                                                  (marker_prefs_get_use_highlight ())
                                                    ? HIGHLIGHT_NET
                                                    : HIGHLIGHT_OFF,
                                                  (marker_prefs_get_use_mermaid ())
                                                    ? MERMAID_NET
                                                    : MERMAID_OFF,
                                                  stylesheet, outfile);
  }
  else if (marker_string_ends_with (outfile, ".pdf")) {
    g_autoptr (MarkerPreview) preview = g_object_ref_sink (marker_preview_new ());
    GMainLoop *loop = g_main_loop_new (NULL, FALSE);

    HeadlessPdfContext ctx = {
      .loop = loop,
      .preview = preview,
      .outfile = outfile,
      .paper_size = paper_size,
      .orientation = orientation,
      .timeout_id = 0,
      .load_handler_id = 0,
      .print_handler_id = 0,
      .printed = FALSE
    };

    ctx.load_handler_id = g_signal_connect (preview, "load-changed",
                                            G_CALLBACK (on_headless_pdf_load_changed), &ctx);
    ctx.print_handler_id = g_signal_connect (preview, "print-finished",
                                             G_CALLBACK (on_headless_pdf_print_finished), &ctx);
    ctx.timeout_id = g_timeout_add_seconds (15, on_headless_pdf_timeout, &ctx);

    marker_preview_render_markdown (preview, markdown, stylesheet, base_folder, -1);

    g_main_loop_run (loop);

    if (ctx.timeout_id > 0)
    {
      g_source_remove (ctx.timeout_id);
      ctx.timeout_id = 0;
    }
    if (ctx.load_handler_id > 0 && g_signal_handler_is_connected (preview, ctx.load_handler_id))
    {
      g_signal_handler_disconnect (preview, ctx.load_handler_id);
      ctx.load_handler_id = 0;
    }
    if (ctx.print_handler_id > 0 && g_signal_handler_is_connected (preview, ctx.print_handler_id))
    {
      g_signal_handler_disconnect (preview, ctx.print_handler_id);
      ctx.print_handler_id = 0;
    }

    g_main_loop_unref (loop);
  }
  else if (marker_string_ends_with (outfile, ".tex")) {
    marker_markdown_to_latex_file (markdown, len, base_folder,
                                   (marker_prefs_get_use_mathjs ())
                                     ? MATHJS_NET
                                     : MATHJS_OFF,
                                   (marker_prefs_get_use_highlight ())
                                     ? HIGHLIGHT_NET
                                     : HIGHLIGHT_OFF,
                                   (marker_prefs_get_use_mermaid ())
                                     ? MERMAID_NET
                                     : MERMAID_OFF,
                                   outfile);
  }
  else {
    marker_exporter_export_pandoc (markdown, stylesheet, outfile);
  }
}
