/*
 * marker-preview.c
 *
 * Copyright (C) 2017-2020 - 2018 Fabio Colacio
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

#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#include "marker-markdown.h"
#include "marker-prefs.h"
#include "marker-string.h"
#include "marker-source-view.h"
#include "marker-preview.h"
#include "marker.h"

#define MAX_ZOOM  4.0
#define MIN_ZOOM  0.1

#define SCROLL_STEP 25
#define SCROLL_STEP_SCRIPT "window.scrollBy(%d,%d);"
#define SCROLL_SCRIPT "window.scrollTo(%d,%d);"

#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a < b) ? b : a)

struct _MarkerPreview
{
  WebKitWebView parent_instance;
};

G_DEFINE_TYPE (MarkerPreview, marker_preview, WEBKIT_TYPE_WEB_VIEW)

static gboolean
is_local_markdown_uri (const gchar *uri,
                       const gchar *current_uri)
{
  if (!uri || uri[0] == '\0')
    return FALSE;

  if (uri[0] == '#')
    return FALSE;

  if (g_str_has_prefix (uri, "http://") ||
      g_str_has_prefix (uri, "https://") ||
      g_str_has_prefix (uri, "mailto:") ||
      g_str_has_prefix (uri, "ftp://"))
    return FALSE;

  g_autofree gchar *clean_uri = g_strdup (uri);
  char *hash = strchr (clean_uri, '#');
  if (hash != NULL)
    *hash = '\0';
  char *query = strchr (clean_uri, '?');
  if (query != NULL)
    *query = '\0';

  if (current_uri != NULL)
  {
    g_autofree gchar *clean_current = g_strdup (current_uri);
    char *c_hash = strchr (clean_current, '#');
    if (c_hash != NULL)
      *c_hash = '\0';
    char *c_query = strchr (clean_current, '?');
    if (c_query != NULL)
      *c_query = '\0';

    if (g_strcmp0 (clean_uri, clean_current) == 0 && hash != NULL)
    {
      /* Same document with fragment: let WebKit navigate internally */
      return FALSE;
    }
  }

  g_autofree gchar *lower_uri = g_ascii_strdown (clean_uri, -1);
  if (g_str_has_suffix (lower_uri, ".md") ||
      g_str_has_suffix (lower_uri, ".markdown") ||
      g_str_has_suffix (lower_uri, ".mdown") ||
      g_str_has_suffix (lower_uri, ".mkdn"))
  {
    return TRUE;
  }

  return FALSE;
}

static gboolean
open_uri (MarkerPreview        *preview,
          WebKitPolicyDecision *decision)
{
  WebKitNavigationPolicyDecision *nav_dec = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
  WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action (nav_dec);
  WebKitURIRequest *request = webkit_navigation_action_get_request (action);

  /* Open only http requests in default browser */
  /* FIXME: Open also other request like ftp in default browser */
  if (webkit_uri_request_get_http_method (request) != NULL)
  {
    const gchar *uri = webkit_uri_request_get_uri (request);
    gtk_show_uri (GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (preview))), uri, GDK_CURRENT_TIME);
    webkit_policy_decision_ignore (decision);
    return TRUE;
  }
  return FALSE;
}

static gboolean
navigate (MarkerPreview        *preview,
          WebKitPolicyDecision *decision)
{
  WebKitNavigationPolicyDecision *nav_dec = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
  WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action (nav_dec);
  WebKitURIRequest *request = webkit_navigation_action_get_request (action);
  const gchar *uri = webkit_uri_request_get_uri (request);

  if (uri == NULL)
    return FALSE;

  const gchar *current_uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (preview));

  if (webkit_navigation_action_get_navigation_type (action) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED &&
      is_local_markdown_uri (uri, current_uri))
  {
    g_autofree gchar *clean_uri = g_strdup (uri);
    char *hash = strchr (clean_uri, '#');
    if (hash != NULL)
      *hash = '\0';
    char *query = strchr (clean_uri, '?');
    if (query != NULL)
      *query = '\0';

    g_signal_emit_by_name (preview, "open-file-requested", clean_uri);
    webkit_policy_decision_ignore (decision);
    return TRUE;
  }

  /* if request is http ignore default policy and open uri in default browser */
  return open_uri (preview, decision);
}

static gboolean
decide_policy_cb (WebKitWebView            *web_view,
                  WebKitPolicyDecision     *decision,
                  WebKitPolicyDecisionType  type,
                  gpointer                  user_data)
{
  MarkerPreview *preview = MARKER_PREVIEW (web_view);

  switch (type)
  {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
      return navigate (preview, decision);
    case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
      webkit_policy_decision_use (decision);
      break;
    case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
      return navigate (preview, decision);
    default:
      /* Making no decision results in webkit_policy_decision_use(). */
      return FALSE;
  }
  return TRUE;
}

static gboolean
context_menu_cb (WebKitWebView       *web_view,
                 WebKitContextMenu   *context_menu,
                 WebKitHitTestResult *hit_test_result,
                 gpointer             user_data)
{
  return TRUE;
}

static gboolean
key_pressed_cb (GtkEventControllerKey *controller,
                guint                  keyval,
                guint                  keycode,
                GdkModifierType        state,
                gpointer               user_data)
{
  g_return_val_if_fail (MARKER_IS_PREVIEW (user_data), FALSE);
  MarkerPreview *preview = MARKER_PREVIEW (user_data);

  if ((state & GDK_CONTROL_MASK) != 0)
  {
    switch (keyval)
    {
      case GDK_KEY_plus:
      case GDK_KEY_equal:
      case GDK_KEY_KP_Add:
        marker_preview_zoom_in (preview);
        return TRUE;

      case GDK_KEY_minus:
      case GDK_KEY_KP_Subtract:
        marker_preview_zoom_out (preview);
        return TRUE;

      case GDK_KEY_0:
      case GDK_KEY_KP_0:
        marker_preview_zoom_original (preview);
        return TRUE;
    }
  }
  else
  {
    switch (keyval)
    {
      case GDK_KEY_j:
        marker_preview_scroll_down (preview);
        return TRUE;

      case GDK_KEY_k:
        marker_preview_scroll_up (preview);
        return TRUE;

      case GDK_KEY_h:
        marker_preview_scroll_left (preview);
        return TRUE;

      case GDK_KEY_l:
        marker_preview_scroll_right (preview);
        return TRUE;

      case GDK_KEY_g:
        marker_preview_scroll_to_top (preview);
        return TRUE;

      case GDK_KEY_G:
        marker_preview_scroll_to_bottom (preview);
        return TRUE;
    }
  }

  return FALSE;
}

static gboolean
scroll_cb (GtkEventControllerScroll *controller,
           gdouble                   dx,
           gdouble                   dy,
           gpointer                  user_data)
{
  g_return_val_if_fail (MARKER_IS_PREVIEW (user_data), FALSE);
  MarkerPreview *preview = MARKER_PREVIEW (user_data);

  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  if ((state & GDK_CONTROL_MASK) != 0)
  {
    if (dy > 0)
    {
      marker_preview_zoom_out (preview);
      return TRUE;
    }
    else if (dy < 0)
    {
      marker_preview_zoom_in (preview);
      return TRUE;
    }
  }

  return FALSE;
}

static void
load_changed_cb (WebKitWebView   *preview,
                 WebKitLoadEvent  event)
{
  switch (event)
  {
    case WEBKIT_LOAD_STARTED:
      break;

    case WEBKIT_LOAD_REDIRECTED:
      break;

    case WEBKIT_LOAD_COMMITTED:
      break;

    case WEBKIT_LOAD_FINISHED:
      webkit_web_view_evaluate_javascript (preview,
                                           "document.getElementById('cursor_pos')?.scrollIntoView({behavior: 'instant', block: 'center'});",
                                           -1,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL);
      break;
  }
}

typedef struct {
  MarkerPreview          *preview;
  MarkerPrintPdfCallback  callback;
  gpointer                user_data;
  gboolean                notified;
} PrintCallbackData;

static void
pdf_dialog_print_failed_cb (WebKitPrintOperation *print_op,
                            GError               *err,
                            gpointer              user_data)
{
  (void) print_op;
  (void) user_data;
  g_printerr ("print failed with error: %s\n", err->message);
}

static void
pdf_print_finished_cb (WebKitPrintOperation *print_op,
                       gpointer              user_data)
{
  (void) print_op;
  PrintCallbackData *data = (PrintCallbackData *) user_data;
  if (data != NULL && !data->notified)
  {
    data->notified = TRUE;
    if (MARKER_IS_PREVIEW (data->preview))
    {
      g_signal_emit_by_name (data->preview, "print-finished", TRUE);
      if (data->callback != NULL)
      {
        data->callback (data->preview, TRUE, data->user_data);
      }
    }
  }
}

static void
pdf_print_failed_cb (WebKitPrintOperation *print_op,
                     GError               *err,
                     gpointer              user_data)
{
  (void) print_op;
  g_printerr ("print failed with error: %s\n", err->message);
  PrintCallbackData *data = (PrintCallbackData *) user_data;
  if (data != NULL && !data->notified)
  {
    data->notified = TRUE;
    if (MARKER_IS_PREVIEW (data->preview))
    {
      g_signal_emit_by_name (data->preview, "print-finished", FALSE);
      if (data->callback != NULL)
      {
        data->callback (data->preview, FALSE, data->user_data);
      }
    }
  }
}

void
marker_preview_set_zoom_level (MarkerPreview *preview,
                               gdouble        zoom_level)
{
  g_return_if_fail (MARKER_IS_PREVIEW (preview));
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (preview), zoom_level);
  marker_source_view_set_zoom_level (zoom_level);
  g_signal_emit_by_name (preview, "zoom-changed");
}

static void
marker_preview_init (MarkerPreview *preview)
{
  WebKitWebContext *context = webkit_web_view_get_context (WEBKIT_WEB_VIEW (preview));
  if (context != NULL)
  {
    webkit_web_context_set_cache_model (context, WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
  }

  WebKitSettings *settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (preview));
  if (settings)
  {
    webkit_settings_set_allow_file_access_from_file_urls (settings, TRUE);
    webkit_settings_set_allow_universal_access_from_file_urls (settings, FALSE);
    webkit_settings_set_enable_javascript (settings, TRUE);
    webkit_settings_set_enable_write_console_messages_to_stdout (settings, TRUE);
  }

  g_signal_connect (preview,
                    "decide-policy",
                    G_CALLBACK (decide_policy_cb),
                    NULL);

  g_signal_connect (preview,
                    "context-menu",
                    G_CALLBACK (context_menu_cb),
                    NULL);

  GtkEventController *key_controller = gtk_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed", G_CALLBACK (key_pressed_cb), preview);
  gtk_widget_add_controller (GTK_WIDGET (preview), key_controller);

  GtkEventController *scroll_controller =
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (scroll_controller, "scroll", G_CALLBACK (scroll_cb), preview);
  gtk_widget_add_controller (GTK_WIDGET (preview), scroll_controller);
}

static void
marker_preview_class_init (MarkerPreviewClass *class)
{
  g_signal_newv ("zoom-changed",
                 G_TYPE_FROM_CLASS (class),
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                 NULL, NULL, NULL, NULL,
                 G_TYPE_NONE, 0, NULL);

  g_signal_new ("open-file-requested",
                G_TYPE_FROM_CLASS (class),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 1, G_TYPE_STRING);

  g_signal_new ("print-finished",
                G_TYPE_FROM_CLASS (class),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  WEBKIT_WEB_VIEW_CLASS (class)->load_changed = load_changed_cb;
}

MarkerPreview *
marker_preview_new (void)
{
  MarkerPreview *obj = g_object_new (MARKER_TYPE_PREVIEW, NULL);
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (obj), marker_prefs_get_zoom_level ());

  return obj;
}

void
marker_preview_zoom_out (MarkerPreview *preview)
{
  g_return_if_fail (WEBKIT_IS_WEB_VIEW (preview));
  WebKitWebView *view = WEBKIT_WEB_VIEW (preview);

  gdouble val = webkit_web_view_get_zoom_level (view) - 0.1;
  val = max (val, MIN_ZOOM);

  marker_prefs_set_zoom_level (val);
  webkit_web_view_set_zoom_level (view, val);
  marker_source_view_set_zoom_level (val);

  g_signal_emit_by_name (preview, "zoom-changed");
}

void
marker_preview_zoom_original (MarkerPreview *preview)
{
  g_return_if_fail (WEBKIT_IS_WEB_VIEW (preview));
  WebKitWebView *view = WEBKIT_WEB_VIEW (preview);

  gdouble zoom = 1.0;

  marker_prefs_set_zoom_level (zoom);
  webkit_web_view_set_zoom_level (view, zoom);
  marker_source_view_set_zoom_level (zoom);

  g_signal_emit_by_name (preview, "zoom-changed");
}

void
marker_preview_zoom_in (MarkerPreview *preview)
{
  g_return_if_fail (WEBKIT_IS_WEB_VIEW (preview));
  WebKitWebView *view = WEBKIT_WEB_VIEW (preview);

  gdouble val = webkit_web_view_get_zoom_level (view) + 0.1;
  val = min (val, MAX_ZOOM);

  marker_prefs_set_zoom_level (val);
  webkit_web_view_set_zoom_level (view, val);
  marker_source_view_set_zoom_level (val);

  g_signal_emit_by_name (preview, "zoom-changed");
}

void
marker_preview_render_markdown (MarkerPreview *preview,
                                const char    *markdown,
                                const char    *css_theme,
                                const char    *base_uri,
                                int            cursor)
{
  MarkerMathJSMode katex_mode = MATHJS_OFF;
  if (marker_prefs_get_use_mathjs ())
  {
    katex_mode = MATHJS_LOCAL;
  }
  MarkerHighlightMode highlight_mode = HIGHLIGHT_OFF;
  if (marker_prefs_get_use_highlight ())
  {
    highlight_mode = HIGHLIGHT_LOCAL;
  }
  MarkerMermaidMode mermaid_mode = MERMAID_OFF;
  if (marker_prefs_get_use_mermaid ())
  {
    mermaid_mode = MERMAID_LOCAL;
  }

  g_autofree char *base_folder = NULL;
  if (base_uri)
  {
    if (g_file_test (base_uri, G_FILE_TEST_IS_DIR))
      base_folder = g_strdup (base_uri);
    else
      base_folder = marker_string_filename_get_path (base_uri);
  }
  char *html = marker_markdown_to_html (markdown,
                                        strlen (markdown),
                                        base_folder,
                                        katex_mode,
                                        highlight_mode,
                                        mermaid_mode,
                                        css_theme,
                                        cursor);

  WebKitWebView *web_view = WEBKIT_WEB_VIEW (preview);
  WebKitNetworkSession *session = webkit_web_view_get_network_session (web_view);
  if (session)
  {
    WebKitWebsiteDataManager *manager = webkit_network_session_get_website_data_manager (session);
    if (manager)
    {
      webkit_website_data_manager_clear (manager,
                                         WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL);
    }
  }

  g_autofree gchar *uri = NULL;
  if (base_uri)
  {
    g_autofree gchar *abs_path = NULL;
    if (!g_path_is_absolute (base_uri))
    {
      g_autofree gchar *current_dir = g_get_current_dir ();
      abs_path = g_build_filename (current_dir, base_uri, NULL);
    }
    const gchar *path_to_convert = abs_path ? abs_path : base_uri;
    g_autofree gchar *locale_uri = g_locale_from_utf8 (path_to_convert, strlen (path_to_convert), NULL, NULL, NULL);
    uri = g_filename_to_uri (locale_uri, NULL, NULL);
  }
  else
  {
    uri = g_strdup_printf ("file://%s", g_get_tmp_dir ());
  }
  if (uri == NULL)
  {
    uri = g_strdup_printf ("file://%s", g_get_tmp_dir ());
  }
  webkit_web_view_load_html (web_view,
                             html,
                             uri);

  free (html);
}

WebKitPrintOperationResponse
marker_preview_run_print_dialog (MarkerPreview *preview,
                                 GtkWindow     *parent)
{
  g_autoptr (WebKitPrintOperation) print_op =
    webkit_print_operation_new (WEBKIT_WEB_VIEW (preview));

  g_signal_connect (print_op, "failed", G_CALLBACK (pdf_dialog_print_failed_cb), NULL);

  return webkit_print_operation_run_dialog (print_op, parent);
}

/* GTK internal unix print symbols for virtual printer enumeration */
typedef gboolean (*GtkPrinterFunc) (gpointer printer, gpointer data);
void gtk_enumerate_printers (GtkPrinterFunc func, gpointer data, GDestroyNotify destroy, gboolean wait);
gboolean gtk_printer_is_virtual (gpointer printer);
const char *gtk_printer_get_name (gpointer printer);

static gboolean
find_virtual_printer_cb (gpointer printer,
                         gpointer data)
{
  if (gtk_printer_is_virtual (printer))
  {
    *(gchar **) data = g_strdup (gtk_printer_get_name (printer));
    return TRUE;
  }
  return FALSE;
}

static gchar *
get_file_printer_name (void)
{
  gchar *name = NULL;
  gtk_enumerate_printers (find_virtual_printer_cb, &name, NULL, TRUE);
  if (name == NULL)
  {
    const gchar *translated = dgettext ("gtk40", "Print to File");
    name = g_strdup (translated != NULL ? translated : "Print to File");
  }
  return name;
}

void
marker_preview_print_pdf_with_callback (MarkerPreview          *preview,
                                        const char             *outfile,
                                        enum scidown_paper_size paper_size,
                                        GtkPageOrientation      orientation,
                                        MarkerPrintPdfCallback  callback,
                                        gpointer                user_data)
{
  g_autoptr (WebKitPrintOperation) print_op = webkit_print_operation_new (WEBKIT_WEB_VIEW (preview));
  g_autoptr (GtkPrintSettings) print_s = gtk_print_settings_new ();
  g_autoptr (GtkPageSetup) gtk_page_setup = gtk_page_setup_new ();
  g_autoptr (GtkPaperSize) gtk_paper_size = NULL;
  g_autofree char *uri = g_strdup_printf ("file://%s", outfile);

  PrintCallbackData *data = g_new0 (PrintCallbackData, 1);
  data->preview = preview;
  data->callback = callback;
  data->user_data = user_data;
  data->notified = FALSE;

  g_object_set_data_full (G_OBJECT (print_op), "marker-print-callback-data", data, g_free);

  g_signal_connect (print_op, "finished", G_CALLBACK (pdf_print_finished_cb), data);
  g_signal_connect (print_op, "failed", G_CALLBACK (pdf_print_failed_cb), data);

  if (paper_size != B43 && paper_size != B169)
    gtk_paper_size = gtk_paper_size_new (paper_to_gtkstr (paper_size));
  else if (paper_size == B43)
    gtk_paper_size = gtk_paper_size_new_custom ("B43", "B43", 166, 221, GTK_UNIT_MM);
  else
    gtk_paper_size = gtk_paper_size_new_custom ("B43", "B43", 166, 294, GTK_UNIT_MM);

  g_autofree gchar *printer_name = get_file_printer_name ();
  gtk_print_settings_set (print_s, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");
  gtk_print_settings_set (print_s, GTK_PRINT_SETTINGS_OUTPUT_URI, uri);
  gtk_print_settings_set (print_s, GTK_PRINT_SETTINGS_PRINTER, printer_name);

  if (orientation == GTK_PAGE_ORIENTATION_PORTRAIT)
  {
    gtk_page_setup_set_paper_size (gtk_page_setup, gtk_paper_size);
    gtk_print_settings_set_paper_width (print_s, gtk_paper_size_get_width (gtk_paper_size, GTK_UNIT_MM), GTK_UNIT_MM);
    gtk_print_settings_set_paper_height (print_s, gtk_paper_size_get_height (gtk_paper_size, GTK_UNIT_MM), GTK_UNIT_MM);
  }
  else
  {
    gdouble width = gtk_paper_size_get_width (gtk_paper_size, GTK_UNIT_MM);
    gdouble height = gtk_paper_size_get_height (gtk_paper_size, GTK_UNIT_MM);
    g_autofree gchar *custom_name = g_strdup_printf ("%s_landscape", paper_to_string (paper_size));
    g_autoptr (GtkPaperSize) custom_size = gtk_paper_size_new_custom (custom_name,
                                                                      "pdf", height, width, GTK_UNIT_MM);
    gtk_page_setup_set_paper_size (gtk_page_setup, custom_size);

    gtk_print_settings_set_paper_width (print_s, height, GTK_UNIT_MM);
    gtk_print_settings_set_paper_height (print_s, width, GTK_UNIT_MM);
  }

  if (paper_size == B43 || paper_size == B169)
  {
    gtk_page_setup_set_left_margin (gtk_page_setup, 0, GTK_UNIT_POINTS);
    gtk_page_setup_set_right_margin (gtk_page_setup, 0, GTK_UNIT_POINTS);
    gtk_page_setup_set_top_margin (gtk_page_setup, 0, GTK_UNIT_POINTS);
    gtk_page_setup_set_bottom_margin (gtk_page_setup, 0, GTK_UNIT_POINTS);
  }
  gtk_print_settings_set_orientation (print_s, orientation);

  webkit_print_operation_set_print_settings (print_op, print_s);
  webkit_print_operation_set_page_setup (print_op, gtk_page_setup);

  webkit_print_operation_print (print_op);
}

void
marker_preview_print_pdf (MarkerPreview          *preview,
                          const char             *outfile,
                          enum scidown_paper_size paper_size,
                          GtkPageOrientation      orientation)
{
  marker_preview_print_pdf_with_callback (preview, outfile, paper_size, orientation, NULL, NULL);
}

void
marker_preview_scroll_left (MarkerPreview *preview)
{
  g_autofree gchar *script = g_strdup_printf (SCROLL_STEP_SCRIPT, -SCROLL_STEP, 0);
  webkit_web_view_evaluate_javascript (WEBKIT_WEB_VIEW (preview), script, -1, NULL, NULL, NULL, NULL, NULL);
}

void
marker_preview_scroll_right (MarkerPreview *preview)
{
  g_autofree gchar *script = g_strdup_printf (SCROLL_STEP_SCRIPT, SCROLL_STEP, 0);
  webkit_web_view_evaluate_javascript (WEBKIT_WEB_VIEW (preview), script, -1, NULL, NULL, NULL, NULL, NULL);
}

void
marker_preview_scroll_up (MarkerPreview *preview)
{
  g_autofree gchar *script = g_strdup_printf (SCROLL_STEP_SCRIPT, 0, -SCROLL_STEP);
  webkit_web_view_evaluate_javascript (WEBKIT_WEB_VIEW (preview), script, -1, NULL, NULL, NULL, NULL, NULL);
}

void
marker_preview_scroll_down (MarkerPreview *preview)
{
  g_autofree gchar *script = g_strdup_printf (SCROLL_STEP_SCRIPT, 0, SCROLL_STEP);
  webkit_web_view_evaluate_javascript (WEBKIT_WEB_VIEW (preview), script, -1, NULL, NULL, NULL, NULL, NULL);
}

void
marker_preview_scroll_to_top (MarkerPreview *preview)
{
  g_autofree gchar *script = g_strdup_printf (SCROLL_SCRIPT, 0, 0);
  webkit_web_view_evaluate_javascript (WEBKIT_WEB_VIEW (preview), script, -1, NULL, NULL, NULL, NULL, NULL);
}

void
marker_preview_scroll_to_bottom (MarkerPreview *preview)
{
  const gchar *script = "window.scrollTo(0,document.body.scrollHeight);";
  webkit_web_view_evaluate_javascript (WEBKIT_WEB_VIEW (preview), script, -1, NULL, NULL, NULL, NULL, NULL);
}
