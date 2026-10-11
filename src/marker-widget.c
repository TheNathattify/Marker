/*
 * marker-widget.c
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

#include "marker-widget.h"

void
marker_widget_combo_box_set_string_model (GtkComboBox  *combo_box,
                                          GtkTreeModel *model)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_TREE_MODEL (model));

  gtk_combo_box_set_model (combo_box, model);
  GtkCellRenderer *cell_renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell_renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
                                  cell_renderer,
                                  "text", 0,
                                  NULL);
  gtk_combo_box_set_active (combo_box, 0);
}

void
marker_widget_populate_combo_box_with_strings (GtkComboBox *combo_box,
                                               GList       *list)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  GtkListStore *list_store = gtk_list_store_new (1, G_TYPE_STRING);
  GtkTreeIter iter;

  for (GList *item = list; item != NULL; item = item->next)
  {
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter, 0, item->data, -1);
  }

  marker_widget_combo_box_set_string_model (combo_box, GTK_TREE_MODEL (list_store));
  g_object_unref (list_store);
}

gchar *
marker_widget_combo_box_get_active_str (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter (combo_box, &iter))
  {
    GtkTreeModel *model = gtk_combo_box_get_model (combo_box);
    if (model != NULL)
    {
      gchar *str = NULL;
      gtk_tree_model_get (model, &iter, 0, &str, -1);
      return str;
    }
  }

  return NULL;
}

void
marker_widget_combo_box_set_active_str (GtkComboBox *combo_box,
                                        const gchar *str,
                                        int          model_len G_GNUC_UNUSED)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  if (str == NULL)
    return;

  GtkTreeModel *model = gtk_combo_box_get_model (combo_box);
  if (model == NULL)
    return;

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_first (model, &iter))
    return;

  gboolean found = FALSE;
  do
  {
    g_autofree gchar *item_str = NULL;
    gtk_tree_model_get (model, &iter, 0, &item_str, -1);
    if (item_str != NULL && g_strcmp0 (item_str, str) == 0)
    {
      gtk_combo_box_set_active_iter (combo_box, &iter);
      found = TRUE;
      break;
    }
  } while (gtk_tree_model_iter_next (model, &iter));

  if (!found)
  {
    gtk_combo_box_set_active (combo_box, 0);
  }
}

