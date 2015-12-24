/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>
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
 *                         call-history-view-gtk.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Julien Puydt
 *   copyright            : (c) 2008 by Julien Puydt
 *   description          : implementation of a call history view widget
 *
 */

#include <sstream>
#include <glib/gi18n.h>
#include <boost/assign/ptr_list_of.hpp>

#include "call-history-view-gtk.h"

#include "gm-cell-renderer-bitext.h"
#include "gactor-menu.h"
#include "scoped-connections.h"


struct null_deleter
{
  void operator()(void const *) const
  {
  }
};


struct _CallHistoryViewGtkPrivate
{
  _CallHistoryViewGtkPrivate (boost::shared_ptr<History::Book> book_)
    : book(book_)
  {}

  boost::shared_ptr<History::Book> book;

  Ekiga::GActorMenuPtr menu;
  Ekiga::GActorMenuPtr contact_menu;

  GtkTreeView* tree;
  Ekiga::scoped_connections conns;
};


/* this is what we put in the view */
enum {
  COLUMN_CONTACT,
  COLUMN_ERROR_PIXBUF,
  COLUMN_PIXBUF,
  COLUMN_NAME,
  COLUMN_INFO,
  COLUMN_NUMBER
};

/* and this is the list of signals supported */
enum {
  ACTIONS_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CallHistoryViewGtk, call_history_view_gtk, GTK_TYPE_SCROLLED_WINDOW);


/* react to a new call being inserted in history */
static void
on_contact_added (Ekiga::ContactPtr contact,
                  GtkListStore *store)
{
  time_t t;
  struct tm *timeinfo = NULL;
  char buffer [80];
  std::stringstream info;
  std::string id;

  boost::shared_ptr<History::Contact> hcontact = boost::dynamic_pointer_cast<History::Contact> (contact);
  GtkTreeIter iter;

  if (hcontact) {

    switch (hcontact->get_type ()) {

    case History::RECEIVED:
      id = "go-previous-symbolic";
      break;

    case History::PLACED:
      id = "go-next-symbolic";
      break;

    case History::MISSED:
      id = "call-missed-symbolic";
      break;

    default:
      break;
    }
  }

  gtk_list_store_prepend (store, &iter);
  t = hcontact->get_call_start ();
  timeinfo = localtime (&t);
  if (timeinfo != NULL) {
    strftime (buffer, 80, "%d.%m.%y %H:%M", timeinfo);
    info << buffer;
    if (!hcontact->get_call_duration ().empty ())
      info << " (" << hcontact->get_call_duration () << ")";
    else
      gtk_list_store_set (store, &iter,
                          COLUMN_ERROR_PIXBUF, "error",
                          -1);
  }
  else
    info << hcontact->get_call_duration ();

  gtk_list_store_set (store, &iter,
                      COLUMN_CONTACT, contact.get (),
                      COLUMN_PIXBUF, id.c_str (),
                      COLUMN_NAME, contact->get_name ().c_str (),
                      COLUMN_INFO, info.str ().c_str (),
                      -1);
}


static void
on_selection_changed (G_GNUC_UNUSED GtkTreeSelection* selection,
                      gpointer data)
{
  CallHistoryViewGtk* self = NULL;
  History::Contact *contact = NULL;

  self = CALL_HISTORY_VIEW_GTK (data);

  /* Reset old data. This also ensures GIO actions are
   * properly removed before adding new ones.
   */
  self->priv->contact_menu.reset ();

  /* Set or reset ContactActor data */
  call_history_view_gtk_get_selected (self, &contact);

  if (contact != NULL) {
    self->priv->contact_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*contact));
    g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0,
                   self->priv->contact_menu->get_model (boost::assign::list_of (self->priv->menu)));
  }
  else
    g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0, NULL);
}


static bool
on_visit_contacts (Ekiga::ContactPtr contact,
                   GtkListStore *store)
{
  on_contact_added (contact, store);
  return true;
}


static void
on_book_contact_added (Ekiga::ContactPtr contact,
                       CallHistoryViewGtk* self)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (self->priv->tree));

  on_contact_added (contact, store);
}


static void
on_book_cleared (CallHistoryViewGtk* data)
{
  GtkListStore *store = NULL;
  GtkTreeSelection *selection = NULL;

  g_return_if_fail (IS_CALL_HISTORY_VIEW_GTK (data));
  CallHistoryViewGtk *self = CALL_HISTORY_VIEW_GTK (data);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (self->priv->tree));
  selection = gtk_tree_view_get_selection (self->priv->tree);

  /* Reset old data. This also ensures GIO actions are
   * properly removed before adding new ones.
   */
  self->priv->contact_menu.reset ();

  if (selection)
    g_signal_handlers_block_by_func (selection, (gpointer) on_selection_changed, self);
  gtk_list_store_clear (store);
  if (selection)
    g_signal_handlers_unblock_by_func (selection, (gpointer) on_selection_changed, self);
}


/* react to user clicks */
static gint
on_clicked (G_GNUC_UNUSED GtkWidget *tree,
            GdkEventButton *event,
            gpointer data)
{
  CallHistoryViewGtk *self = CALL_HISTORY_VIEW_GTK (data);

  // take into account only clicks and Enter keys
  if (event->type != GDK_BUTTON_PRESS && event->type != GDK_2BUTTON_PRESS && event->type != GDK_KEY_PRESS)
    return FALSE;
  if (event->type == GDK_KEY_PRESS && ((GdkEventKey*)event)->keyval != GDK_KEY_Return && ((GdkEventKey*)event)->keyval != GDK_KEY_KP_Enter)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 3 && self->priv->contact_menu)
    gtk_menu_popup (GTK_MENU (self->priv->contact_menu->get_menu (boost::assign::list_of (self->priv->menu))),
                    NULL, NULL, NULL, NULL, event->button, event->time);

  if ((event->type == GDK_2BUTTON_PRESS && event->button == 1) || (event->type == GDK_KEY_PRESS))
    g_action_group_activate_action (G_ACTION_GROUP (g_application_get_default ()), "call", NULL);

  return TRUE;
}


static void
on_map_cb (G_GNUC_UNUSED GtkWidget *widget,
           gpointer data)
{
  GtkTreeSelection *selection = NULL;

  g_return_if_fail (IS_CALL_HISTORY_VIEW_GTK (data));
  CallHistoryViewGtk *self = CALL_HISTORY_VIEW_GTK (data);

  selection = gtk_tree_view_get_selection (self->priv->tree);
  on_selection_changed (selection, self);
}


/* GObject stuff */
static void
call_history_view_gtk_finalize (GObject* obj)
{
  CallHistoryViewGtk* view = NULL;

  view = CALL_HISTORY_VIEW_GTK (obj);

  delete view->priv;

  G_OBJECT_CLASS (call_history_view_gtk_parent_class)->finalize (obj);
}

static void
call_history_view_gtk_init (G_GNUC_UNUSED CallHistoryViewGtk* self)
{
  /* empty because we don't have the core */
}

static void
call_history_view_gtk_class_init (CallHistoryViewGtkClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = call_history_view_gtk_finalize;

  signals[ACTIONS_CHANGED_SIGNAL] =
    g_signal_new ("actions-changed",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CallHistoryViewGtkClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_MENU_MODEL);
}

/* public api */

GtkWidget *
call_history_view_gtk_new (boost::shared_ptr<History::Book> book,
                           G_GNUC_UNUSED boost::shared_ptr<Ekiga::CallCore> call_core,
                           G_GNUC_UNUSED boost::shared_ptr<Ekiga::ContactCore> contact_core)
{
  CallHistoryViewGtk* self = NULL;

  GtkListStore *store = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkTreeSelection *selection = NULL;

  g_return_val_if_fail (book, (GtkWidget*)NULL);

  self = (CallHistoryViewGtk*)g_object_new (CALL_HISTORY_VIEW_GTK_TYPE, NULL);

  self->priv = new _CallHistoryViewGtkPrivate (book);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /* build the store then the tree */
  store = gtk_list_store_new (COLUMN_NUMBER,
                              G_TYPE_POINTER,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  self->priv->tree = (GtkTreeView*)gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->tree), FALSE);
  gtk_tree_view_set_grid_lines (self->priv->tree, GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->priv->tree));
  g_object_unref (store);

  /* one column should be enough for everyone */
  column = gtk_tree_view_column_new ();

  /* show icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "icon-name", COLUMN_ERROR_PIXBUF);
  g_object_set (renderer, "xalign", 0.0, "yalign", 0.5, "xpad", 6, "stock-size", 1, NULL);

  /* show name and text */
  renderer = gm_cell_renderer_bitext_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "primary-text", COLUMN_NAME);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "secondary-text", COLUMN_INFO);
  gtk_tree_view_append_column (self->priv->tree, column);

  /* show icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_end (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "icon-name", COLUMN_PIXBUF);
  g_object_set (renderer, "xalign", 1.0, "yalign", 0.5, "xpad", 6, "stock-size", 2, NULL);

  /* react to user clicks */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (on_selection_changed), self);
  g_signal_connect (self->priv->tree, "event-after",
                    G_CALLBACK (on_clicked), self);
  g_signal_connect (GTK_WIDGET (self), "map",
                    G_CALLBACK (on_map_cb), self);

  /* connect to the signals */
  self->priv->conns.add (book->contact_added.connect (boost::bind (&on_book_contact_added, _1, self)));
  self->priv->conns.add (book->cleared.connect (boost::bind (&on_book_cleared, self)));

  /* initial populate */
  self->priv->book->visit_contacts (boost::bind (&on_visit_contacts, _1, GTK_LIST_STORE (store)));

  /* register book actions */
  self->priv->menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*book));

  return GTK_WIDGET (self);
}

void
call_history_view_gtk_get_selected (CallHistoryViewGtk* self,
                                    History::Contact** contact)
{
  g_return_if_fail (IS_CALL_HISTORY_VIEW_GTK (self) && contact != NULL);

  GtkTreeSelection* selection = NULL;
  GtkTreeModel* model = NULL;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (self->priv->tree);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    gtk_tree_model_get (model, &iter,
                        COLUMN_CONTACT, contact,
                        -1);
  else
    *contact = NULL;
}
