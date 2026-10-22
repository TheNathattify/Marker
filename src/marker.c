/*
 * marker.c
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

#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "marker-prefs.h"
#include "marker-window.h"
#include "marker-exporter.h"
#include "marker.h"

static gboolean editor_mode_arg = FALSE;
static gboolean preview_mode_arg = FALSE;
static gboolean dual_pane_mode_arg = FALSE;
static gboolean dual_window_mode_arg = FALSE;
static gchar *outfile_arg = NULL;

static const GOptionEntry CLI_OPTIONS[] =
{
  { "editor", 'e', 0, G_OPTION_ARG_NONE, &editor_mode_arg, "Open in editor-only mode", NULL },
  { "preview", 'p', 0, G_OPTION_ARG_NONE, &preview_mode_arg, "Open in preview-only mode", NULL },
  { "dual-pane", 'd', 0, G_OPTION_ARG_NONE, &dual_pane_mode_arg, "Open in dual-pane mode", NULL },
  { "dual-window", 'w', 0, G_OPTION_ARG_NONE, &dual_window_mode_arg, "Open in dual-window mode", NULL },
  { "output", 'o', 0, G_OPTION_ARG_STRING, &outfile_arg, "Export the given markdown document as the given output file", NULL },
  { NULL }
};

const int APP_MENU_ACTION_ENTRIES_LEN = 6;

const GActionEntry APP_MENU_ACTION_ENTRIES[] =
{
  { "new", new_cb, NULL, NULL, NULL },
  { "prefs", marker_prefs_cb, NULL, NULL, NULL },
  { "shortcuts", marker_shortcuts_cb, NULL, NULL, NULL },
  { "help", marker_help_cb, NULL, NULL, NULL },
  { "about", marker_about_cb, NULL, NULL, NULL },
  { "quit", marker_quit_cb, NULL, NULL, NULL }
};

GtkApplication *
marker_get_app (void)
{
  return GTK_APPLICATION (g_application_get_default ());
}

static void
marker_init (GtkApplication *app)
{
  marker_prefs_load ();

  g_set_application_name ("Marker");
  g_set_prgname ("com.github.adrighem.marker");
  gtk_window_set_default_icon_name ("com.github.adrighem.marker");

  const gchar *quit_accels[] = { "<Ctrl>q", NULL };
  gtk_application_set_accels_for_action (app, "app.quit", quit_accels);

  AdwStyleManager *style_manager = adw_style_manager_get_default ();
  adw_style_manager_set_color_scheme (style_manager,
                                      marker_prefs_get_use_dark_theme () ? ADW_COLOR_SCHEME_PREFER_DARK : ADW_COLOR_SCHEME_DEFAULT);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   APP_MENU_ACTION_ENTRIES,
                                   APP_MENU_ACTION_ENTRIES_LEN,
                                   NULL);
}

static void
activate (GtkApplication *app)
{
  marker_init (app);
  marker_create_new_window ();
}

static void
marker_open (GtkApplication  *app,
             GFile          **files,
             gint             num_files,
             const gchar     *hint)
{
  (void) hint;
  g_application_hold (G_APPLICATION (app));
  marker_init (app);

  if (outfile_arg != NULL && num_files > 0)
  {
    g_autoptr(GFile) outfile = g_file_new_for_commandline_arg (outfile_arg);
    g_autofree gchar *outfile_path = g_file_get_path (outfile);
    g_autofree gchar *infile_path = g_file_get_path (files[0]);
    marker_exporter_export (infile_path, outfile_path);
    g_application_release (G_APPLICATION (app));
    return;
  }

  for (int i = 0; i < num_files; ++i)
  {
    marker_open_file (files[i]);
  }
  g_application_release (G_APPLICATION (app));
}

void
new_cb (GSimpleAction *action,
        GVariant      *parameter,
        gpointer       user_data)
{
  (void) action;
  (void) parameter;
  (void) user_data;
  marker_create_new_window ();
}

void
marker_prefs_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  (void) action;
  (void) parameter;
  (void) user_data;
  marker_prefs_show_window ();
}

void
marker_help_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  (void) action;
  (void) parameter;
  (void) user_data;
  GtkApplication *app = marker_get_app ();
  GtkWindow *parent = gtk_application_get_active_window (app);

  gtk_show_uri (parent, "help:Marker", GDK_CURRENT_TIME);
}

void
marker_about_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  (void) action;
  (void) parameter;
  (void) user_data;
  const gchar *authors[] = {
    "Vincent van Adrighem",
    "Fabio Colacio",
    "Martino Ferrari",
    NULL
  };

  const gchar *artists[] = {
    "Fabio Colacio",
    NULL
  };

  GtkApplication *app = marker_get_app ();
  GtkWindow *parent = gtk_application_get_active_window (app);

  adw_show_about_dialog (GTK_WIDGET (parent),
                         "application-name", "Marker",
                         "application-icon", "com.github.adrighem.marker",
                         "version", MARKER_VERSION,
                         "comments", _("A markdown editor for GNOME"),
                         "website", "https://github.com/adrighem/Marker",
                         "issue-url", "https://github.com/adrighem/Marker/issues",
                         "copyright", "Copyright 2017-2026 Marker Developers",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "developers", authors,
                         "artists", artists,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

void
marker_quit_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  (void) action;
  (void) parameter;
  (void) user_data;
  marker_quit ();
}

void
marker_shortcuts_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  (void) action;
  (void) parameter;
  (void) user_data;
  g_autoptr(GtkBuilder) builder =
    gtk_builder_new_from_resource ("/com/github/adrighem/marker/ui/marker-shortcuts-window.ui");

  GtkWidget *dialog = GTK_WIDGET (gtk_builder_get_object (builder, "shortcuts"));
  GtkApplication *app = marker_get_app ();
  GtkWindow *parent = gtk_application_get_active_window (app);

  if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (dialog)))
  {
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
  }

  gtk_window_present (GTK_WINDOW (dialog));
}

gboolean
marker_has_app_menu (void)
{
  return FALSE;
}

void
marker_create_new_window (void)
{
  GtkApplication *app = marker_get_app ();
  MarkerWindow *window = marker_window_new (app);
  gtk_window_present (GTK_WINDOW (window));
}

void
marker_create_new_window_from_file (GFile *file)
{
  GtkApplication *app = marker_get_app ();
  MarkerWindow *window = marker_window_new_from_file (app, file);
  gtk_window_present (GTK_WINDOW (window));

  if (preview_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, PREVIEW_ONLY_MODE);
  }
  else if (editor_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, EDITOR_ONLY_MODE);
  }
  else if (dual_pane_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, DUAL_PANE_MODE);
  }
  else if (dual_window_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, DUAL_WINDOW_MODE);
  }
}

void
marker_open_file (GFile *file)
{
  GtkApplication *app = marker_get_app ();
  GList *windows = gtk_application_get_windows (app);
  if (windows && MARKER_IS_WINDOW (windows->data))
  {
    MarkerWindow *window = MARKER_WINDOW (windows->data);
    marker_window_new_editor_from_file (window, file);
    return;
  }

  marker_create_new_window_from_file (file);
}

void
marker_quit (void)
{
  GtkApplication *app = marker_get_app ();
  GList *windows = gtk_application_get_windows (app);
  for (GList *item = windows; item != NULL; item = item->next)
  {
    if (MARKER_IS_WINDOW (item->data))
    {
      MarkerWindow *window = item->data;
      marker_window_try_close (window);
    }
  }
}

int
main (int    argc,
      char **argv)
{
  setlocale (LC_ALL, "");
  bindtextdomain ("marker", LOCALE_DIR);
  bind_textdomain_codeset ("marker", "UTF-8");
  textdomain ("marker");

  adw_init ();
  gtk_source_init ();

  g_autoptr(GtkApplication) app = gtk_application_new ("com.github.adrighem.marker",
                                                        G_APPLICATION_HANDLES_OPEN);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "open", G_CALLBACK (marker_open), NULL);

  g_application_add_main_option_entries (G_APPLICATION (app), CLI_OPTIONS);

  int status = g_application_run (G_APPLICATION (app), argc, argv);

  gtk_source_finalize ();

  return status;
}
