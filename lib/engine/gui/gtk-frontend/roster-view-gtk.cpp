
/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2016 Damien Sandras <dsandras@seconix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * Ekiga is licensed under the GPL license and as a special exception,
 * you have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination,
 * without applying the requirements of the GNU GPL to the OPAL, OpenH323
 * and PWLIB programs, as long as you do follow the requirements of the
 * GNU GPL for all the rest of the software thus combined.
 */


/*
 *                         roster-view-gtk.cpp -  description
 *                         ------------------------------------------
 *   begin                : written in 2006 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *                          (c) 2014 by Damien Sandras
 *   description          : implementation of the roster view
 *
 */

#include <iostream>
#include <ctime>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <boost/assign/ptr_list_of.hpp>

#include "ekiga-settings.h"

#include "account.h"

#include "gm-cell-renderer-bitext.h"
#include "gm-cell-renderer-expander.h"
#include "roster-view-gtk.h"
#include "form-dialog-gtk.h"
#include "scoped-connections.h"
#include "gactor-menu.h"

#define SPINNER_PULSE_INTERVAL (750 / 18)


/*
 * The Roster
 *
 * This is a RosterView in GTK.
 *
 * It will display Ekiga::Heaps and their Ekiga::Presentities.
 *
 * If a Heap is also an Ekiga::Account, both will be merged. Each
 * Ekiga::Account being an Ekiga::Heap will display the underlying
 * Ekiga::Presentities.
 *
 * Ekiga::Accounts that are not Ekiga::Heaps will not be displayed
 * by the RosterViewGtk. They should be handled elsewhere.
 */
struct _RosterViewGtkPrivate
{
  Ekiga::scoped_connections connections;
  Ekiga::Settings *settings;
  GtkTreeStore *store;
  GtkTreeView *tree_view;
  GSList *folded_groups;
  gboolean show_offline_contacts;

  int pulse_timeout_id;
  unsigned int pulse_value;

  Ekiga::GActorMenuPtr presentity_menu;
  Ekiga::GActorMenuPtr heap_menu;
};

/* the different type of things which will appear in the view */
enum {

  TYPE_HEAP,
  TYPE_GROUP,
  TYPE_PRESENTITY
};

enum {
  ACTIONS_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (RosterViewGtk, roster_view_gtk, GTK_TYPE_FRAME);

/* This is how things will be stored roughly :
 * - the heaps are at the top ;
 * - under each heap come the groups ;
 * - under each group come the presentities.
 *
 * For the heaps, we show the name.
 *
 * For the groups, we show the name.
 *
 * For the presentities, we show the name, the status and the presence.
 *
 * This means we can share and put the name in a column.
 *
 */
enum {

  COLUMN_TYPE,
  COLUMN_HEAP,
  COLUMN_ACCOUNT,
  COLUMN_PRESENTITY,
  COLUMN_NAME,
  COLUMN_STATUS,
  COLUMN_PRESENCE_ICON,
  COLUMN_AVATAR_PIXBUF,
  COLUMN_ACCOUNT_STATUS_ICON,
  COLUMN_ACCOUNT_STATUS_ICON_VISIBLE,
  COLUMN_ACCOUNT_SPINNER_PULSE,
  COLUMN_ACCOUNT_SPINNER_VISIBLE,
  COLUMN_FOREGROUND_COLOR,
  COLUMN_GROUP_NAME,
  COLUMN_PRESENCE,
  COLUMN_OFFLINE,
  COLUMN_NUMBER
};

/* Callbacks */

/* DESCRIPTION : Called when the user clicks in a view.
 * BEHAVIOUR   : Folds/unfolds.
 */
static void on_clicked_fold (RosterViewGtk* self,
                             GtkTreePath* path,
                             const gchar* name);


/* DESCRIPTION : Called when the user changes the preference for offline
 * BEHAVIOUR   : Updates things.
 * PRE         : The gpointer must be a RosterViewGtk
 */
static void on_show_offline_contacts_changed_cb (GSettings *settings,
                                                 gchar *key,
                                                 gpointer data);


/* DESCRIPTION  : Called when the user selects a presentity.
 * BEHAVIOR     : Rebuilds menus and emit the actions_changed signal.
 * PRE          : The gpointer must point to the RosterViewGtk GObject.
 */
static void on_selection_changed (GtkTreeSelection* actions,
                                  gpointer data);


/* DESCRIPTION  : Called when the user clicks or presses Enter
 *                on a heap, group or presentity.
 * BEHAVIOR     : Update the menu and displays it as a popup.
 * PRE          : The gpointer must point to the RosterViewGtk GObject.
 */
static gint on_view_event_after (GtkWidget *tree_view,
                                 GdkEventButton *event,
                                 gpointer data);


/* DESCRIPTION  : Called when the RosterViewGtk widget becomes visible.
 * BEHAVIOR     : Calls on_selection_changed to update actions.
 * PRE          : /
 */
static void on_map_cb (G_GNUC_UNUSED GtkWidget *widget,
                       gpointer data);


/* DESCRIPTION : Called to decide whether to show a line ; used to hide/show
 *               offline contacts on demand.
 * BEHAVIOUR   : Returns TRUE if the line should be shown.
 * PRE         : The gpointer must point to a RosterViewGtk object.
 */
static gboolean presentity_hide_show_offline (RosterViewGtk* self,
                                              GtkTreeModel* model,
                                              GtkTreeIter* iter);
static gboolean group_hide_show_offline (RosterViewGtk* self,
                                         GtkTreeModel* model,
                                         GtkTreeIter* iter);
static gboolean tree_model_filter_hide_show_offline (GtkTreeModel *model,
                                                     GtkTreeIter *iter,
                                                     gpointer data);


/* DESCRIPTION  : Called for a given renderer in order to show or hide it.
 * BEHAVIOR     : Only show the renderer if current iter points to a line of
 *                type GPOINTER_TO_INT (data).
 * PRE          : The gpointer must be TYPE_HEAP, TYPE_CLUSTER or TYPE_PRESENTITY
 *                once casted using GPOINTER_TO_INT.
 */
static void show_cell_data_func (GtkTreeViewColumn *column,
                                 GtkCellRenderer *renderer,
                                 GtkTreeModel *model,
                                 GtkTreeIter *iter,
                                 gpointer data);


/* DESCRIPTION  : Called for a given renderer in order to modify properties.
 * BEHAVIOR     : Expand the expander renderer if required.
 *                Hide the expander renderer for Presentity.
 *                and Heap.
 * PRE          : /
 */
static void expand_cell_data_func (GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer data);


/**************** Account Methods ****************/

/* DESCRIPTION  : /
 * BEHAVIOR     : Visits the Banks containing the Accounts.
 *                We are only interested in Accounts which are also Heaps.
 *                Other types of Accounts are not displayed in the Roster.
 *
 *                For this reason, we will only connect to the account_updated
 *                Bank signal to handle Account updates.
 *
 *                Account removal, and newly added Accounts are handled through
 *                the Heap methods.
 * PRE          : /
 */
static bool on_visit_banks (RosterViewGtk* self,
                            Ekiga::BankPtr bank);


/* DESCRIPTION  : Called when a new bank has been added
 * BEHAVIOR     : Connect to the account_updated signal.
 * PRE          : /
 */
static void on_bank_added (RosterViewGtk* self,
                           Ekiga::BankPtr bank);


/* DESCRIPTION  : Called when the account updated signal has been emitted.
 * BEHAVIOR     : Update the GtkTreeView.
 * PRE          : /
 */
static void on_account_updated (RosterViewGtk* self,
                                Ekiga::AccountPtr account);


/**************** Presence Methods ****************/

/* DESCRIPTION  : Called when a new cluster has been added
 * BEHAVIOR     : Visits the cluster's heaps, and add them to the view
 * PRE          : /
 */
static bool on_visit_clusters (RosterViewGtk* self,
                               Ekiga::ClusterPtr cluster);


/* DESCRIPTION  : Called when a new cluster has been added
 * BEHAVIOR     : Visits the cluster's heaps, and add them to the view
 * PRE          : /
 */
static void on_cluster_added (RosterViewGtk* self,
                              Ekiga::ClusterPtr cluster);


/* DESCRIPTION  : Called when visiting a new cluster
 * BEHAVIOR     : Adds in the cluster heaps
 * PRE          : /
 */
static bool visit_heaps (RosterViewGtk* self,
                         Ekiga::HeapPtr heap);


/* DESCRIPTION  : Called when the or heap_added signal has been emitted
 * BEHAVIOR     : Add the Heap in the GtkTreeView.
 * PRE          : /
 */
static void on_heap_added (RosterViewGtk* self,
                           Ekiga::HeapPtr heap);


/* DESCRIPTION  : Called when the heap_updated signal has been emitted
 * BEHAVIOR     : Update the Heap in the GtkTreeView.
 * PRE          : /
 */
static void on_heap_updated (RosterViewGtk* self,
                             Ekiga::HeapPtr heap);


/* DESCRIPTION  : Called when the heap_removed signal has been emitted.
 * BEHAVIOR     : Removes the Heap from the GtkTreeView. All children,
 *                ie associated Presentity entities are also removed from
 *                the view.
 * PRE          : /
 */
static void on_heap_removed (RosterViewGtk* self,
                             Ekiga::HeapPtr heap);


/* DESCRIPTION  : Called when visiting a new heap
 * BEHAVIOR     : Adds in the heap presentities
 * PRE          : /
 */
static bool on_visit_presentities (RosterViewGtk* self,
                                   GtkTreeIter heap_iter,
                                   Ekiga::PresentityPtr presentity);


/* DESCRIPTION  : Called when the presentity_added signal has been emitted.
 * BEHAVIOR     : Add the given Presentity into the Heap on which it was
 *                added.
 * PRE          : A valid Heap.
 */
static void on_presentity_added (RosterViewGtk* self,
                                 GtkTreeIter heap_iter,
                                 Ekiga::PresentityPtr presentity);


/* DESCRIPTION  : Called when the presentity_updated signal has been emitted.
 * BEHAVIOR     : Update the given Presentity into the Heap.
 * PRE          : A valid Heap.
 */
static void on_presentity_updated (RosterViewGtk* self,
                                   GtkTreeIter heap_iter,
                                   Ekiga::PresentityPtr presentity);


/* DESCRIPTION  : Called when the presentity_removed signal has been emitted.
 * BEHAVIOR     : Remove the given Presentity from the given Heap.
 * PRE          : A valid Heap.
 */
static void on_presentity_removed (RosterViewGtk* self,
                                   GtkTreeIter heap_iter,
                                   Ekiga::PresentityPtr presentity);


/* DESCRIPTION  : Called when the PresenceCore has a form request to handle.
 * BEHAVIOR     : Runs the form request.
 * PRE          : The given pointer is the roster view widget.
 */
static bool on_handle_questions (RosterViewGtk* self,
                                 Ekiga::FormRequestPtr request);


/*
 * Static helper functions
 */

/* DESCRIPTION  : /
 * BEHAVIOR     : Update the iter parameter so that it points to
 *                the GtkTreeIter corresponding to the given Heap.
 *                Return true if the Heap was found, false otherwise.
 * PRE          : /
 */
static bool roster_view_gtk_get_iter_for_heap (RosterViewGtk *view,
                                               Ekiga::HeapPtr heap,
                                               GtkTreeIter *iter);


/* DESCRIPTION  : /
 * BEHAVIOR     : Update the iter parameter so that it points to
 *                the GtkTreeIter corresponding to the given Account.
 *                Return true if the Account was found, false otherwise.
 * PRE          : /
 */
static bool roster_view_gtk_get_iter_for_account (RosterViewGtk *view,
                                                  Ekiga::AccountPtr account,
                                                  GtkTreeIter *iter);


/* DESCRIPTION  : /
 * BEHAVIOR     : Update the iter parameter so that it points to
 *                the GtkTreeIter corresponding to the given group name
 *                in the given Heap.
 *                Return true if the group was found, false otherwise.
 * PRE          : /
 */
static bool roster_view_gtk_get_iter_for_group (RosterViewGtk *view,
                                                GtkTreeIter heap_iter,
                                                const std::string name,
                                                GtkTreeIter *iter);


/* DESCRIPTION  : /
 * BEHAVIOR     : Update the iter parameter so that it points to
 *                the GtkTreeIter corresponding to the given presentity
 *                in the given group.
 *                Return true if iter was found, false otherwise.
 * PRE          : /
 */
static bool roster_view_gtk_find_iter_for_presentity (RosterViewGtk *view,
                                                      GtkTreeIter *group_iter,
                                                      Ekiga::PresentityPtr presentity,
                                                      GtkTreeIter *iter);


/* DESCRIPTION : /
 * BEHAVIOUR   : Updates online/offline counters.
 * PRE         : Both arguments have to be correct
 */
static void roster_view_gtk_update_counters (RosterViewGtk* self,
                                             GtkTreeIter* iter);


/* DESCRIPTION  : /
 * BEHAVIOR     : Do a clean up in the RosterViewGtk to clean all empty groups
 *                from the view. It also folds or unfolds groups following
 *                the value of the appropriate GMConf key.
 * PRE          : /
 */
static void roster_view_gtk_update_groups (RosterViewGtk *view,
                                           GtkTreeIter *heap_iter);


/* DESCRIPTION  : /
 * BEHAVIOR     : Update the presentity in the Roster.
 * PRE          : /
 */
static void roster_view_gtk_update_presentity (RosterViewGtk* self,
                                               GtkTreeIter iter,
                                               Ekiga::PresentityPtr presentity);


/* DESCRIPTION  : /
 * BEHAVIOR     : Remove presentity from the Roster.
 *                Update iter to the next row. Return true if iter is still
 *                valid, false otherwise.
 * PRE          : /
 */
static bool roster_view_gtk_remove_presentity (RosterViewGtk* self,
                                               GtkTreeIter iter,
                                               Ekiga::PresentityPtr presentity);


/* DESCRIPTION  : /
 * BEHAVIOR     : Update the given Heap.
 * PRE          : /
 */
static void roster_view_gtk_update_heap (RosterViewGtk* self,
                                         GtkTreeIter iter,
                                         Ekiga::HeapPtr heap);

/* DESCRIPTION  : /
 * BEHAVIOR     : Remove the given Heap.
 * PRE          : /
 */
static void roster_view_gtk_remove_heap (RosterViewGtk* self,
                                         GtkTreeIter iter,
                                         Ekiga::HeapPtr heap);


/* DESCRIPTION  : /
 * BEHAVIOR     : Update the given Account.
 * PRE          : /
 */
static void roster_view_gtk_update_account (RosterViewGtk *self,
                                            GtkTreeIter heap_iter,
                                            Ekiga::AccountPtr heap);



/* Implementation of the helpers */
static void
on_clicked_fold (RosterViewGtk* self,
                 GtkTreePath* path,
                 const gchar* name)
{
  gboolean row_expanded = TRUE;
  GSList* existing_group = NULL;

  row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (self->priv->tree_view), path);

  existing_group = g_slist_find_custom (self->priv->folded_groups,
                                        name,
                                        (GCompareFunc) g_ascii_strcasecmp);
  if (!row_expanded) {

    if (existing_group == NULL) {
      self->priv->folded_groups = g_slist_append (self->priv->folded_groups,
                                                  g_strdup (name));
    }
  }
  else {

    if (existing_group != NULL) {

      self->priv->folded_groups
        = g_slist_remove_link (self->priv->folded_groups, existing_group);

      g_free ((gchar *) existing_group->data);
      g_slist_free_1 (existing_group);
    }
  }

  /* Update gsettings */
  self->priv->settings->set_slist ("roster-folded-groups", self->priv->folded_groups);
}


static gboolean
spinner_pulse_timeout_cb (gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter loop_iter;
  gint column_type;
  gboolean active = FALSE;
  gboolean keep_pulsing = FALSE;
  unsigned int pulse = 0;

  RosterViewGtk *self = ROSTER_VIEW_GTK (data);

  model = GTK_TREE_MODEL (self->priv->store);

  if (gtk_tree_model_get_iter_first (model, &loop_iter)) {

    do {

      gtk_tree_model_get (model, &loop_iter,
                          COLUMN_TYPE, &column_type,
                          COLUMN_ACCOUNT_SPINNER_PULSE, &pulse,
                          COLUMN_ACCOUNT_SPINNER_VISIBLE, &active,
                          -1);

      if (column_type == TYPE_HEAP) {
        if (active) {

          pulse++;
          if (pulse == G_MAXUINT)
            pulse = 0;

          gtk_tree_store_set (GTK_TREE_STORE (model), &loop_iter,
                              COLUMN_ACCOUNT_SPINNER_PULSE, pulse, -1);

          keep_pulsing = TRUE;
        }
      }

    } while (gtk_tree_model_iter_next (model, &loop_iter));
  }

  if (!keep_pulsing)
    self->priv->pulse_timeout_id = -1;

  return keep_pulsing;
}


static void
on_show_offline_contacts_changed_cb (GSettings *settings,
                                     gchar *key,
                                     gpointer data)
{
  RosterViewGtk *self = NULL;
  GtkTreeModel *model = NULL;

  g_return_if_fail (data != NULL);

  self = ROSTER_VIEW_GTK (data);

  self->priv->show_offline_contacts = g_settings_get_boolean (settings, key);

  /* beware: model is filtered here */
  model = gtk_tree_view_get_model (self->priv->tree_view);
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));

  /* beware: we want the unfiltered model now */
  model = GTK_TREE_MODEL (self->priv->store);

  /* there's an interesting problem there : hiding makes the rows
   * unexpanded... so they don't come back as they should! */
  GtkTreeIter heaps;
  GtkTreePath* path = NULL;
  if (gtk_tree_model_get_iter_first (model, &heaps)) {

    do {

      path = gtk_tree_model_get_path (model, &heaps);
      gtk_tree_view_expand_row (self->priv->tree_view, path, FALSE);
      gtk_tree_path_free (path);

      roster_view_gtk_update_groups (self, &heaps);
    } while (gtk_tree_model_iter_next (model, &heaps));
  }

  // Clean up
  model = gtk_tree_view_get_model (self->priv->tree_view);
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
}


static void
on_selection_changed (GtkTreeSelection* selection,
                      gpointer data)
{
  RosterViewGtk* self = NULL;
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;

  gint column_type;
  Ekiga::Heap *heap = NULL;
  Ekiga::Presentity *presentity = NULL;

  gchar *name = NULL;
  gchar *group_name = NULL;

  self = ROSTER_VIEW_GTK (data);
  model = gtk_tree_view_get_model (self->priv->tree_view);

  /* Reset old data. This also ensures GIO actions are
   * properly removed before adding new ones.
   */
  self->priv->presentity_menu.reset ();
  self->priv->heap_menu.reset ();

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get (model, &iter,
                        COLUMN_NAME, &name,
                        COLUMN_GROUP_NAME, &group_name,
                        COLUMN_TYPE, &column_type,
                        COLUMN_HEAP, &heap,
                        COLUMN_PRESENTITY, &presentity,
                        -1);

    if (heap != NULL)
      self->priv->heap_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*heap, heap->get_name ()));
    if (presentity != NULL)
      self->priv->presentity_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*presentity, presentity->get_name ()));

    if (heap && presentity)
      g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0,
                     self->priv->presentity_menu->get_model (boost::assign::list_of (self->priv->heap_menu)));
    else if (heap)
      g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0,
                     self->priv->heap_menu->get_model ());
    else if (presentity)
      g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0,
                     self->priv->presentity_menu->get_model ());
    else
      g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0, NULL);

    g_free (group_name);
    g_free (name);
  }
  else
    g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0, NULL);
}


static gint
on_view_event_after (GtkWidget *tree_view,
                     GdkEventButton *event,
                     gpointer data)
{
  RosterViewGtk *self = NULL;
  GtkTreeModel *model = NULL;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;

  // take into account only clicks and Enter keys
  if (event->type != GDK_BUTTON_PRESS && event->type != GDK_2BUTTON_PRESS && event->type != GDK_KEY_PRESS)
    return FALSE;
  if (event->type == GDK_KEY_PRESS && ((GdkEventKey*)event)->keyval != GDK_KEY_Return && ((GdkEventKey*)event)->keyval != GDK_KEY_KP_Enter)
    return FALSE;

  self = ROSTER_VIEW_GTK (data);
  model = gtk_tree_view_get_model (self->priv->tree_view);

  // get the line clicked or currently selected
  gboolean ret = true;
  if (event->type == GDK_KEY_PRESS)
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (tree_view), &path, NULL);
  else
    ret = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree_view),
                                         (gint) event->x, (gint) event->y,
                                         &path, NULL, NULL, NULL);

  if (!ret)
    return TRUE;  // click on an empty line

  if (gtk_tree_model_get_iter (model, &iter, path)) {

    gint column_type;
    gchar *name = NULL;
    gchar *group_name = NULL;
    Ekiga::Presentity *presentity = NULL;
    gtk_tree_model_get (model, &iter,
                        COLUMN_NAME, &name,
                        COLUMN_GROUP_NAME, &group_name,
                        COLUMN_TYPE, &column_type,
                        COLUMN_PRESENTITY, &presentity,
                        -1);

    switch (column_type) {

    case TYPE_HEAP:
      if (event->type == GDK_BUTTON_PRESS && event->button == 1 && name)
        on_clicked_fold (self, path, name);
      else if (event->type == GDK_BUTTON_PRESS && event->button == 3 && self->priv->heap_menu->get_menu ())
        gtk_menu_popup (GTK_MENU (self->priv->heap_menu->get_menu ()),
                        NULL, NULL, NULL, NULL, event->button, event->time);
      break;
    case TYPE_GROUP:
      if (event->type == GDK_BUTTON_PRESS && event->button == 1 && group_name)
        on_clicked_fold (self, path, group_name);
      break;
    case TYPE_PRESENTITY:
      if (event->type == GDK_BUTTON_PRESS && event->button == 3 && self->priv->presentity_menu->get_menu ())
        gtk_menu_popup (GTK_MENU (self->priv->presentity_menu->get_menu ()),
                        NULL, NULL, NULL, NULL, event->button, event->time);
      if ((event->type == GDK_2BUTTON_PRESS && event->button == 1) || (event->type == GDK_KEY_PRESS))
        g_action_group_activate_action (G_ACTION_GROUP (g_application_get_default ()), "call", NULL);
      break;
    default:

      g_assert_not_reached ();
      break; // shouldn't happen
    }
    g_free (name);
    g_free (group_name);
  }
  gtk_tree_path_free (path);

  return TRUE;
}


static void
on_map_cb (G_GNUC_UNUSED GtkWidget *widget,
           gpointer data)
{
  GtkTreeSelection *selection = NULL;

  g_return_if_fail (IS_ROSTER_VIEW_GTK (data));
  RosterViewGtk *self = ROSTER_VIEW_GTK (data);

  selection = gtk_tree_view_get_selection (self->priv->tree_view);
  on_selection_changed (selection, self);
}


static gboolean
presentity_hide_show_offline (RosterViewGtk* self,
                              GtkTreeModel* model,
                              GtkTreeIter* iter)
{
  gboolean result = FALSE;

  if (self->priv->show_offline_contacts)
    result = TRUE;
  else
    gtk_tree_model_get (model, iter,
                        COLUMN_OFFLINE, &result,
                        -1);

  return result;
}

static gboolean
group_hide_show_offline (RosterViewGtk* self,
                         GtkTreeModel* model,
                         GtkTreeIter* iter)
{
  gboolean result = FALSE;

  if (self->priv->show_offline_contacts)
    result = TRUE;
  else {

    GtkTreeIter child_iter;
    if (gtk_tree_model_iter_nth_child (model, &child_iter, iter, 0)) {

      do {

        result = presentity_hide_show_offline (self, model, &child_iter);
      } while (!result && gtk_tree_model_iter_next (model, &child_iter));
    }
  }

  return result;
}


static gboolean
tree_model_filter_hide_show_offline (GtkTreeModel *model,
                                     GtkTreeIter *iter,
                                     gpointer data)
{
  gboolean result = FALSE;
  RosterViewGtk *self = NULL;
  gint column_type;

  self = ROSTER_VIEW_GTK (data);

  gtk_tree_model_get (model, iter,
                      COLUMN_TYPE, &column_type,
                      -1);

  switch (column_type) {

  case TYPE_PRESENTITY:

    result = presentity_hide_show_offline (self, model, iter);
    break;

  case TYPE_GROUP:

    result = group_hide_show_offline (self, model, iter);
    break;

  case TYPE_HEAP:
  default:
    result = TRUE;
  }

  return result;
}


static void
show_cell_data_func (GtkTreeViewColumn * /*column*/,
                     GtkCellRenderer *renderer,
                     GtkTreeModel *model,
                     GtkTreeIter *iter,
                     gpointer data)
{
  gint column_type;

  gtk_tree_model_get (model, iter, COLUMN_TYPE, &column_type, -1);

  if (column_type == GPOINTER_TO_INT (data))
    g_object_set (renderer, "visible", TRUE, NULL);
  else
    g_object_set (renderer, "visible", FALSE, NULL);
}


static void
expand_cell_data_func (GtkTreeViewColumn *column,
                       GtkCellRenderer *renderer,
                       GtkTreeModel *model,
                       GtkTreeIter *iter,
                       gpointer /*data*/)
{
  GtkTreePath *path = NULL;
  gint column_type;
  gboolean row_expanded = FALSE;

  path = gtk_tree_model_get_path (model, iter);
  row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (gtk_tree_view_column_get_tree_view (column)), path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, iter, COLUMN_TYPE, &column_type, -1);

  if (column_type == TYPE_PRESENTITY || column_type == TYPE_HEAP)
    g_object_set (renderer, "visible", FALSE, NULL);
  else
    g_object_set (renderer, "visible", TRUE, NULL);

  g_object_set (renderer,
                "is-expanded", row_expanded,
                NULL);
}


static bool
on_visit_banks (RosterViewGtk* self,
                Ekiga::BankPtr bank)
{
  on_bank_added (self, bank);

  return true;
}


static void
on_bank_added (RosterViewGtk* self,
               Ekiga::BankPtr bank)
{
  boost::signals2::connection conn;
  conn = bank->account_updated.connect (boost::bind (&on_account_updated, self, _1));
  self->priv->connections.add (conn);
}


static void
on_account_updated (RosterViewGtk* self,
                    Ekiga::AccountPtr account)
{
  GtkTreeIter iter;

  if (roster_view_gtk_get_iter_for_account (self, account, &iter))
    roster_view_gtk_update_account (self, iter, account);
}


static bool
on_visit_clusters (RosterViewGtk* self,
                   Ekiga::ClusterPtr cluster)
{
  on_cluster_added (self, cluster);

  return true;
}


static void
on_cluster_added (RosterViewGtk* self,
                  Ekiga::ClusterPtr cluster)
{
  boost::signals2::connection conn;
  cluster->visit_heaps (boost::bind (&visit_heaps, self, _1));

  conn = cluster->heap_added.connect (boost::bind (&on_heap_added, self, _1));
  self->priv->connections.add (conn);

  conn = cluster->heap_updated.connect (boost::bind (&on_heap_updated, self, _1));
  self->priv->connections.add (conn);

  conn = cluster->heap_removed.connect (boost::bind (&on_heap_removed, self, _1));
  self->priv->connections.add (conn);
}

static bool
visit_heaps (RosterViewGtk* self,
             Ekiga::HeapPtr heap)
{
  on_heap_added (self, heap);

  return true;
}


static void
on_heap_added (RosterViewGtk* self,
               Ekiga::HeapPtr heap)
{
  GtkTreeIter heap_iter;
  boost::signals2::connection conn;

  gtk_tree_store_append (self->priv->store, &heap_iter, NULL);
  roster_view_gtk_update_heap (self, heap_iter, heap);

  Ekiga::AccountPtr account = boost::dynamic_pointer_cast <Ekiga::Account> (heap);
  if (account)
    roster_view_gtk_update_account (self, heap_iter, account);

  conn = heap->presentity_added.connect (boost::bind (&on_presentity_added, self, heap_iter, _1));
  self->priv->connections.add (conn);

  conn = heap->presentity_updated.connect (boost::bind (&on_presentity_updated, self, heap_iter, _1));
  self->priv->connections.add (conn);

  conn = heap->presentity_removed.connect (boost::bind (&on_presentity_removed, self, heap_iter, _1));
  self->priv->connections.add (conn);

  heap->visit_presentities (boost::bind (&on_visit_presentities, self, heap_iter, _1));
}


static void
on_heap_updated (RosterViewGtk* self,
                 Ekiga::HeapPtr heap)
{
  GtkTreeIter iter;

  if (roster_view_gtk_get_iter_for_heap (self, heap, &iter))
    roster_view_gtk_update_heap (self, iter, heap);
}


static void
on_heap_removed (RosterViewGtk* self,
                 Ekiga::HeapPtr heap)
{
  GtkTreeIter iter;

  if (roster_view_gtk_get_iter_for_heap (self, heap, &iter))
    roster_view_gtk_remove_heap (self, iter, heap);
}


static bool
on_visit_presentities (RosterViewGtk* self,
                       GtkTreeIter heap_iter,
                       Ekiga::PresentityPtr presentity)
{
  on_presentity_added (self, heap_iter, presentity);

  return true;
}


static void
on_presentity_added (RosterViewGtk* self,
                     GtkTreeIter heap_iter,
                     Ekiga::PresentityPtr presentity)
{
  std::list<std::string> groups = presentity->get_groups ();
  GtkTreeIter group_iter;
  GtkTreeIter iter;

  if (groups.empty ())
    groups.push_back (_("Unsorted"));

  /* Add the presentity to all heap groups it belongs to */
  for (std::list<std::string>::const_iterator group = groups.begin ();
       group != groups.end ();
       group++) {

    if (!roster_view_gtk_get_iter_for_group (self, heap_iter, *group, &group_iter)) {
      // Group not found, add it to the roster
      gtk_tree_store_append (self->priv->store, &group_iter, &heap_iter);
      gtk_tree_store_set (self->priv->store, &group_iter,
                          COLUMN_TYPE, TYPE_GROUP,
                          COLUMN_NAME, (*group).c_str (),
                          COLUMN_GROUP_NAME, (*group).c_str (),
                          -1);
    }

    // Now add the presentity in the group
    gtk_tree_store_append (self->priv->store, &iter, &group_iter);
    roster_view_gtk_update_presentity (self, iter, presentity);
  }

  /* Clean up */
  roster_view_gtk_update_groups (self, &heap_iter);
}


static void
on_presentity_updated (RosterViewGtk* self,
                       GtkTreeIter heap_iter,
                       Ekiga::PresentityPtr presentity)
{
  GtkTreeModel *model;
  GtkTreeIter group_iter;
  GtkTreeIter iter;
  gchar *group_name = NULL;
  Ekiga::Presentity *presentity_pointer = NULL;
  std::list<std::string> groups = presentity->get_groups ();

  model = GTK_TREE_MODEL (self->priv->store);

  if (groups.empty ())
    groups.push_back (_("Unsorted"));

  if (gtk_tree_model_iter_nth_child (model, &group_iter, &heap_iter, 0)) {

    // Loop through all groups in the Heap
    do {

      bool in_group = false;
      bool found_presentity = false;

      gtk_tree_model_get (model, &group_iter, COLUMN_GROUP_NAME, &group_name, -1);
      if (std::find (groups.begin (), groups.end (), (const char*) group_name) != groups.end ())
        in_group = true;

      if (gtk_tree_model_iter_nth_child (model, &iter, &group_iter, 0)) {

        // Loop through all presentities in the group
        // When we find the presentity, we remove it from the group if
        // it does not belong to it. Or we update it.
        // A presentity can only be present once in a group.
        do {

          gtk_tree_model_get (model, &iter, COLUMN_PRESENTITY, &presentity_pointer, -1);

          if (presentity_pointer == presentity.get ()) {
            found_presentity = true;
            if (!in_group)
              roster_view_gtk_remove_presentity (self, iter, presentity);
            else
              roster_view_gtk_update_presentity (self, iter, presentity);
            groups.remove (group_name);
          }
        } while (!found_presentity && gtk_tree_model_iter_next (model, &iter));
      }

      g_free (group_name);

    } while (gtk_tree_model_iter_next (model, &group_iter));
  }

  // Now add the presentity to all new groups it belongs
  for (std::list<std::string>::const_iterator group = groups.begin ();
       group != groups.end ();
       group++) {

    if (!roster_view_gtk_get_iter_for_group (self, heap_iter, *group, &group_iter)) {
      // Group not found, add it to the roster
      gtk_tree_store_append (self->priv->store, &group_iter, &heap_iter);
      gtk_tree_store_set (self->priv->store, &group_iter,
                          COLUMN_TYPE, TYPE_GROUP,
                          COLUMN_NAME, (*group).c_str (),
                          COLUMN_GROUP_NAME, (*group).c_str (),
                          -1);
    }
    gtk_tree_store_append (self->priv->store, &iter, &group_iter);
    roster_view_gtk_update_presentity (self, iter, presentity);
  }

  // Clean up
  model = gtk_tree_view_get_model (self->priv->tree_view);
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
  roster_view_gtk_update_groups (self, &heap_iter);
}


static void
on_presentity_removed (RosterViewGtk* self,
                       GtkTreeIter heap_iter,
                       Ekiga::PresentityPtr presentity)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter group_iter;
  GtkTreeIter iter;

  model = GTK_TREE_MODEL (self->priv->store);

  if (gtk_tree_model_iter_nth_child (model, &group_iter, &heap_iter, 0)) {

    bool found_presentity = false;

    do {

      if (roster_view_gtk_find_iter_for_presentity (self, &group_iter, presentity, &iter)) {
        found_presentity = true;
        gtk_tree_store_remove (self->priv->store, &iter);
      }
    } while (!found_presentity && gtk_tree_model_iter_next (model, &group_iter));
  }

  roster_view_gtk_update_groups (self, &heap_iter);
}


static bool
on_handle_questions (RosterViewGtk* self,
                     Ekiga::FormRequestPtr request)
{
  GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  FormDialog dialog (request, parent);

  dialog.run ();

  return true;
}


/*
 * Implementation of the static helpers.
 */
static bool
roster_view_gtk_get_iter_for_heap (RosterViewGtk *view,
                                   Ekiga::HeapPtr heap,
                                   GtkTreeIter *iter)
{
  GtkTreeModel *model = NULL;
  Ekiga::Heap *iter_heap = NULL;
  gboolean found = FALSE;

  model = GTK_TREE_MODEL (view->priv->store);

  if (gtk_tree_model_get_iter_first (model, iter)) {

    do {

      gtk_tree_model_get (model, iter, COLUMN_HEAP, &iter_heap, -1);
      if (iter_heap == heap.get ())
        found = TRUE;
    } while (!found && gtk_tree_model_iter_next (model, iter));
  }

  return found;
}


static bool
roster_view_gtk_get_iter_for_account (RosterViewGtk *view,
                                      Ekiga::AccountPtr account,
                                      GtkTreeIter *iter)
{
  GtkTreeModel *model = NULL;
  Ekiga::Account *iter_account = NULL;
  gboolean found = FALSE;

  model = GTK_TREE_MODEL (view->priv->store);

  if (gtk_tree_model_get_iter_first (model, iter)) {

    do {

      gtk_tree_model_get (model, iter, COLUMN_ACCOUNT, &iter_account, -1);
      if (iter_account == account.get ())
        found = TRUE;
    } while (!found && gtk_tree_model_iter_next (model, iter));
  }

  return found;
}


static bool
roster_view_gtk_get_iter_for_group (RosterViewGtk *view,
                                    GtkTreeIter heap_iter,
                                    const std::string name,
                                    GtkTreeIter *group_iter)
{
  GtkTreeModel *model = NULL;
  gchar *group_name = NULL;
  gboolean found = FALSE;

  model = GTK_TREE_MODEL (view->priv->store);

  if (gtk_tree_model_iter_nth_child (model, group_iter, &heap_iter, 0)) {

    do {

      gtk_tree_model_get (model, group_iter, COLUMN_GROUP_NAME, &group_name, -1);
      if (group_name != NULL && name == group_name)
        found = TRUE;
      if (group_name != NULL)
        g_free (group_name);
    } while (!found && gtk_tree_model_iter_next (model, group_iter));
  }

  return found;
}


static bool
roster_view_gtk_find_iter_for_presentity (RosterViewGtk *view,
                                          GtkTreeIter *group_iter,
                                          Ekiga::PresentityPtr presentity,
                                          GtkTreeIter *iter)
{
  GtkTreeModel *model = NULL;
  Ekiga::Presentity *iter_presentity = NULL;
  gboolean found = FALSE;

  model = GTK_TREE_MODEL (view->priv->store);

  if (gtk_tree_model_iter_nth_child (model, iter, group_iter, 0)) {

    do {

      gtk_tree_model_get (model, iter, COLUMN_PRESENTITY, &iter_presentity, -1);
      if (iter_presentity == presentity.get ())
        found = TRUE;
    } while (!found && gtk_tree_model_iter_next (model, iter));
  }

  return found;
}


static void
roster_view_gtk_update_counters (RosterViewGtk* self,
                                 GtkTreeIter* iter)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter loop_iter;
  gint total = 0;
  gint offline_count = 0;
  gint column_type;
  Ekiga::Presentity* presentity = NULL;
  gchar *name = NULL;
  gchar *name_with_count = NULL;

  model = GTK_TREE_MODEL (self->priv->store);

  if (gtk_tree_model_iter_nth_child (model, &loop_iter, iter, 0)) {

    do {

      gtk_tree_model_get (model, &loop_iter,
                          COLUMN_TYPE, &column_type,
                          COLUMN_PRESENTITY, &presentity,
                          -1);
      if (column_type == TYPE_PRESENTITY
          && (presentity->get_presence () == "offline"
              || presentity->get_presence () == "unknown"))
        offline_count++;
    } while (gtk_tree_model_iter_next (model, &loop_iter));
  }

  total = gtk_tree_model_iter_n_children (model, iter);
  gtk_tree_model_get (model, iter, COLUMN_GROUP_NAME, &name, -1);
  name_with_count = g_strdup_printf ("%s - (%d/%d)", name, total - offline_count, total);
  gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                      COLUMN_NAME, name_with_count, -1);
  g_free (name);
  g_free (name_with_count);
}


static void
roster_view_gtk_update_groups (RosterViewGtk *view,
                               GtkTreeIter *heap_iter)
{
  GtkTreeModel *model = NULL;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;

  GSList *existing_group = NULL;

  gboolean go_on = FALSE;
  gchar *name = NULL;

  model = GTK_TREE_MODEL (view->priv->store);

  if (gtk_tree_model_iter_nth_child (model, &iter, heap_iter, 0)) {

    do {

      // If this node has children, see if it must be
      // folded or unfolded
      if (gtk_tree_model_iter_has_child (model, &iter)) {

        roster_view_gtk_update_counters (view, &iter);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GROUP_NAME, &name, -1);
        if (name) {

          if (view->priv->folded_groups)
            existing_group = g_slist_find_custom (view->priv->folded_groups,
                                                  name,
                                                  (GCompareFunc) g_ascii_strcasecmp);

          path = gtk_tree_model_get_path (model, heap_iter);
          if (path) {
            gtk_tree_view_expand_row (view->priv->tree_view, path, FALSE);
            gtk_tree_path_free (path);
          }

          path = gtk_tree_model_get_path (model, &iter);
          if (path) {

            if (existing_group == NULL) {
              if (!gtk_tree_view_row_expanded (view->priv->tree_view, path)) {
                gtk_tree_view_expand_row (view->priv->tree_view, path, TRUE);
              }
            }
            else {
              if (gtk_tree_view_row_expanded (view->priv->tree_view, path)) {
                gtk_tree_view_collapse_row (view->priv->tree_view, path);
              }
            }

            gtk_tree_path_free (path);
          }

          go_on = gtk_tree_model_iter_next (model, &iter);
        }

        g_free (name);
      }
      // else remove the node (no children)
      else
        go_on = gtk_tree_store_remove (view->priv->store, &iter);
    } while (go_on);
  }
}


static void
roster_view_gtk_update_presentity (RosterViewGtk* self,
                                   GtkTreeIter iter,
                                   Ekiga::PresentityPtr presentity)
{
  GdkRGBA color;
  GdkPixbuf *pixbuf = NULL;
  bool active = false;
  std::string icon = "phone";
  std::string presence;

  // Refer to what we know
  if (presentity->get_presence () == "available"
      || presentity->get_presence () == "busy"
      || presentity->get_presence () == "away"
      || presentity->get_presence () == "inacall"
      || presentity->get_presence () == "offline") {
    presence = presentity->get_presence ();
    icon = "user-" + presence;
  }
  else
    presence = "unknown";

  active = (presence != "offline");

  gtk_style_context_get_color (gtk_widget_get_style_context (GTK_WIDGET (self->priv->tree_view)),
                               (presence != "available" && presence != "unknown")?GTK_STATE_FLAG_INSENSITIVE:GTK_STATE_FLAG_NORMAL,
                               &color);

  pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     "avatar-default-symbolic",
                                     48,
                                     GTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                     NULL);

  gtk_tree_store_set (self->priv->store, &iter,
                      COLUMN_TYPE, TYPE_PRESENTITY,
                      COLUMN_OFFLINE, active,
                      COLUMN_PRESENTITY, presentity.get (),
                      COLUMN_NAME, presentity->get_name ().c_str (),
                      COLUMN_AVATAR_PIXBUF, pixbuf,
                      COLUMN_STATUS, presentity->get_status ().c_str (),
                      COLUMN_PRESENCE_ICON, icon.c_str (),
                      COLUMN_FOREGROUND_COLOR, &color, -1);

  g_object_unref (pixbuf);
}


static bool
roster_view_gtk_remove_presentity (RosterViewGtk* self,
                                   GtkTreeIter iter,
                                   G_GNUC_UNUSED Ekiga::PresentityPtr presentity)
{
  return gtk_tree_store_remove (self->priv->store, &iter);
}


static void
roster_view_gtk_update_heap (RosterViewGtk* self,
                             GtkTreeIter iter,
                             Ekiga::HeapPtr heap)
{
  gchar *heap_name = NULL;

  heap_name = g_strdup_printf ("<span weight=\"bold\" size=\"larger\" variant=\"smallcaps\">%s</span>",
                               heap->get_name ().c_str ());

  gtk_tree_store_set (self->priv->store, &iter,
                      COLUMN_TYPE, TYPE_HEAP,
                      COLUMN_HEAP, heap.get (),
                      COLUMN_NAME, heap_name, -1);

  g_free (heap_name);
}


static void
roster_view_gtk_remove_heap (RosterViewGtk* self,
                             GtkTreeIter heap_iter,
                             G_GNUC_UNUSED Ekiga::HeapPtr heap)
{
  GtkTreeSelection* selection = NULL;

  selection = gtk_tree_view_get_selection (self->priv->tree_view);
  gtk_tree_selection_unselect_all (selection);

  gtk_tree_store_remove (self->priv->store, &heap_iter);
}


static void
roster_view_gtk_update_account (RosterViewGtk *self,
                                GtkTreeIter heap_iter,
                                Ekiga::AccountPtr account)
{
  gchar *icon_name = NULL;
  GdkPixbuf *pixbuf = NULL;

  gtk_tree_store_set (self->priv->store, &heap_iter,
                      COLUMN_ACCOUNT, account.get (), -1);

  switch (account->get_state ()) {
  case Ekiga::Account::Processing:
    gtk_tree_store_set (self->priv->store, &heap_iter,
                        COLUMN_ACCOUNT_STATUS_ICON_VISIBLE, FALSE,
                        COLUMN_ACCOUNT_SPINNER_VISIBLE, TRUE, -1);

    if (self->priv->pulse_timeout_id == -1)
      self->priv->pulse_timeout_id =
        g_timeout_add (SPINNER_PULSE_INTERVAL,
                       spinner_pulse_timeout_cb,
                       self);
    break;
  case Ekiga::Account::Registered:
    icon_name = g_strdup ("network-idle-symbolic");
    break;
  case Ekiga::Account::Unregistered:
    icon_name = g_strdup ("network-offline-symbolic");
    break;
  case Ekiga::Account::RegistrationFailed:
  case Ekiga::Account::UnregistrationFailed:
  default:
    icon_name = g_strdup ("network-error-symbolic");
  }

  if (icon_name) {
    pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                       icon_name,
                                       16,
                                       GTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                       NULL);
    gtk_tree_store_set (self->priv->store, &heap_iter,
                        COLUMN_ACCOUNT_STATUS_ICON, pixbuf,
                        COLUMN_ACCOUNT_STATUS_ICON_VISIBLE, TRUE,
                        COLUMN_ACCOUNT_SPINNER_VISIBLE, FALSE, -1);
    g_object_unref (pixbuf);
    g_free (icon_name);
  }
}


/*
 * GObject stuff
 */
static void
roster_view_gtk_dispose (GObject *obj)
{
  RosterViewGtk *view = NULL;

  view = (RosterViewGtk *)obj;

  if (view->priv->pulse_timeout_id > -1)
    g_source_remove (view->priv->pulse_timeout_id);
  view->priv->pulse_timeout_id = -1;

  G_OBJECT_CLASS (roster_view_gtk_parent_class)->dispose (obj);
}


static void
roster_view_gtk_finalize (GObject *obj)
{
  RosterViewGtk *view = NULL;

  view = (RosterViewGtk *)obj;

  delete view->priv->settings;

  g_slist_foreach (view->priv->folded_groups, (GFunc) g_free, NULL);
  g_slist_free (view->priv->folded_groups);
  view->priv->folded_groups = NULL;
  delete view->priv;


  G_OBJECT_CLASS (roster_view_gtk_parent_class)->finalize (obj);
}


static void
roster_view_gtk_init (RosterViewGtk* self)
{
  GtkWidget *scrolled_window;
  GtkWidget *vbox = NULL;
  GtkTreeModel *filtered = NULL;
  GtkTreeSelection *selection = NULL;
  GtkTreeViewColumn *col = NULL;
  GtkCellRenderer *renderer = NULL;

  self->priv = new RosterViewGtkPrivate;

  self->priv->settings = new Ekiga::Settings (CONTACTS_SCHEMA);
  self->priv->folded_groups = self->priv->settings->get_slist ("roster-folded-groups");
  self->priv->show_offline_contacts = self->priv->settings->get_bool ("show-offline-contacts");
  self->priv->pulse_timeout_id = -1;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 0);
  gtk_frame_set_shadow_type (GTK_FRAME (self), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  self->priv->store = gtk_tree_store_new (COLUMN_NUMBER,
                                          G_TYPE_INT,         // type
                                          G_TYPE_POINTER,     // heap
                                          G_TYPE_POINTER,     // account
                                          G_TYPE_POINTER,     // presentity
                                          G_TYPE_STRING,      // name
                                          G_TYPE_STRING,      // status
                                          G_TYPE_STRING,      // presence
                                          GDK_TYPE_PIXBUF,    // Avatar
                                          GDK_TYPE_PIXBUF,    // Account status icon
                                          G_TYPE_BOOLEAN,     // Account status icon visible
                                          G_TYPE_UINT,        // Account spinner pulse
                                          G_TYPE_BOOLEAN,     // Account spinner visible
                                          GDK_TYPE_RGBA,      // cell foreground color
                                          G_TYPE_STRING,      // group name (invisible)
                                          G_TYPE_STRING,      // presence
                                          G_TYPE_BOOLEAN);    // offline

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->store),
                                        COLUMN_NAME, GTK_SORT_ASCENDING);
  filtered = gtk_tree_model_filter_new (GTK_TREE_MODEL (self->priv->store),
                                        NULL);
  g_object_unref (self->priv->store);
  self->priv->tree_view =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model (filtered));
  g_object_unref (filtered);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filtered),
                                          tree_model_filter_hide_show_offline,
                                          self, NULL);

  gtk_tree_view_set_headers_visible (self->priv->tree_view, FALSE);
  gtk_tree_view_set_grid_lines (self->priv->tree_view, GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox),
                      GTK_WIDGET (scrolled_window), TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     GTK_WIDGET (self->priv->tree_view));

  /* Build the GtkTreeView */
  // We hide the normal GTK+ expanders and use our own
  col = gtk_tree_view_column_new ();
  g_object_set (col, "visible", FALSE, NULL);
  gtk_tree_view_append_column (self->priv->tree_view, col);
  gtk_tree_view_set_expander_column (self->priv->tree_view, col);

  col = gtk_tree_view_column_new ();
  renderer = gm_cell_renderer_expander_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  g_object_set (renderer,
                "xalign", 0.0,
                "xpad", 0,
                "ypad", 0,
                "visible", TRUE,
                "is-expander", TRUE,
                "is-expanded", FALSE,
                NULL);
  gtk_tree_view_column_set_cell_data_func (col, renderer, expand_cell_data_func, NULL, NULL);
  gtk_tree_view_append_column (self->priv->tree_view, col);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_set_spacing (col, 0);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "markup", COLUMN_NAME);
  gtk_tree_view_column_add_attribute (col, renderer, "foreground-rgba", COLUMN_FOREGROUND_COLOR);
  gtk_tree_view_column_set_alignment (col, 0.0);
  g_object_set (renderer, "xalign", 0.0, NULL);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           show_cell_data_func, GINT_TO_POINTER (TYPE_HEAP), NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "text", COLUMN_NAME);
  gtk_tree_view_column_add_attribute (col, renderer, "foreground-rgba", COLUMN_FOREGROUND_COLOR);
  g_object_set (renderer, "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           show_cell_data_func, GINT_TO_POINTER (TYPE_GROUP), NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "xalign", 0.5, "xpad", 6, "stock-size", 1, NULL);
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "icon-name",
                                      COLUMN_PRESENCE_ICON);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           show_cell_data_func, GINT_TO_POINTER (TYPE_PRESENTITY), NULL);

  renderer = gm_cell_renderer_bitext_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, "width-chars", 30, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "primary-text", COLUMN_NAME);
  gtk_tree_view_column_add_attribute (col, renderer, "secondary-text", COLUMN_STATUS);
  gtk_tree_view_column_add_attribute (col, renderer, "foreground-rgba", COLUMN_FOREGROUND_COLOR);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           show_cell_data_func, GINT_TO_POINTER (TYPE_PRESENTITY), NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "xalign", 1.0, "xpad", 0, NULL);
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "pixbuf",
                                      COLUMN_AVATAR_PIXBUF);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           show_cell_data_func, GINT_TO_POINTER (TYPE_PRESENTITY), NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "xalign", 1.0, "xpad", 6, NULL);
  gtk_tree_view_column_pack_end (col, renderer, FALSE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "pixbuf",
                                      COLUMN_ACCOUNT_STATUS_ICON);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "visible",
                                      COLUMN_ACCOUNT_STATUS_ICON_VISIBLE);

  renderer = gtk_cell_renderer_spinner_new ();
  g_object_set (renderer, "xalign", 1.0,
                "size", GTK_ICON_SIZE_LARGE_TOOLBAR,
                "xpad", 6, NULL);
  gtk_tree_view_column_pack_end (col, renderer, FALSE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "pulse",
                                      COLUMN_ACCOUNT_SPINNER_PULSE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "active",
                                      COLUMN_ACCOUNT_SPINNER_VISIBLE);
  gtk_tree_view_column_add_attribute (col, renderer,
                                      "visible",
                                      COLUMN_ACCOUNT_SPINNER_VISIBLE);

  /* Callback when the selection has been changed */
  selection = gtk_tree_view_get_selection (self->priv->tree_view);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (on_selection_changed), self);
  g_signal_connect (self->priv->tree_view, "event-after",
                    G_CALLBACK (on_view_event_after), self);
  g_signal_connect (GTK_WIDGET (self), "map",
                    G_CALLBACK (on_map_cb), self);

  /* Other signals */
  g_signal_connect (self->priv->settings->get_g_settings (), "changed::show-offline-contacts",
                    G_CALLBACK (&on_show_offline_contacts_changed_cb), self);
}


static void
roster_view_gtk_class_init (RosterViewGtkClass* klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = roster_view_gtk_dispose;
  gobject_class->finalize = roster_view_gtk_finalize;

  signals[ACTIONS_CHANGED_SIGNAL] =
    g_signal_new ("actions-changed",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (RosterViewGtkClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_MENU_MODEL);
}


/*
 * Public API
 */
GtkWidget *
roster_view_gtk_new (boost::shared_ptr<Ekiga::PresenceCore> pcore,
                     boost::shared_ptr<Ekiga::AccountCore> acore)
{
  RosterViewGtk* self = NULL;
  boost::signals2::connection conn;

  self = (RosterViewGtk *) g_object_new (ROSTER_VIEW_GTK_TYPE, NULL);

  /* Account */
  acore->visit_banks (boost::bind (&on_visit_banks, self, _1));

  /* Presence */
  conn = pcore->cluster_added.connect (boost::bind (&on_cluster_added, self, _1));
  self->priv->connections.add (conn);

  conn = pcore->questions.connect (boost::bind (&on_handle_questions, self, _1));
  self->priv->connections.add (conn);

  pcore->visit_clusters (boost::bind (&on_visit_clusters, self, _1));

  return (GtkWidget *) self;
}
