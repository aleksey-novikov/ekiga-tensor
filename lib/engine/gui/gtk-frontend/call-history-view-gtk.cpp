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

  GtkTreeView* tree;
  Ekiga::scoped_connections conns;
};


/* this is what we put in the view */
enum {
  COLUMN_CONTACT,
  COLUMN_PIXBUF,
  COLUMN_NAME,
  COLUMN_INFO,
  COLUMN_NUMBER
};

/* and this is the list of signals supported */
enum {
  CLICKED_SIGNAL,
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
//    else
//      gtk_list_store_set (store, &iter,
//                          COLUMN_ERROR_PIXBUF, "error",
//                          -1);
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


/* react to user clicks */
static gint
on_clicked (G_GNUC_UNUSED GtkWidget *tree,
            GdkEvent *event,
            gpointer data)
{
  CallHistoryViewGtk *self = CALL_HISTORY_VIEW_GTK (data);

  if ((event->type == GDK_2BUTTON_PRESS && ((GdkEventButton*)event)->button == 1) ||
      (event->type == GDK_KEY_RELEASE && (((GdkEventKey*)event)->keyval == GDK_KEY_Return || ((GdkEventKey*)event)->keyval == GDK_KEY_KP_Enter))) {
    g_signal_emit (self, signals[CLICKED_SIGNAL], 0, NULL);
    return TRUE;
  } else if (event->type == GDK_KEY_PRESS && ((GdkEventKey*)event)->state & GDK_CONTROL_MASK) {
    guint keyval = ((GdkEventKey*)event)->keyval;
    if (keyval == GDK_KEY_c || keyval == GDK_KEY_C || keyval == GDK_KEY_Cyrillic_es || keyval == GDK_KEY_Cyrillic_ES || keyval == GDK_KEY_Insert || keyval == GDK_KEY_KP_Insert) {
      History::Contact *contact = NULL;
      call_history_view_gtk_get_selected (self, &contact);

      if (contact && !contact->get_uri().empty()) {
        size_t pos = contact->get_uri().find(':');
        std::string number = contact->get_uri().substr(pos == std::string::npos ? 0 : pos + 1);
        number = number.substr(0, number.find('@'));

        if (!number.empty())
          gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), number.c_str(), -1);
      }

      return TRUE;
    }
  }

  return FALSE;
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

  signals[CLICKED_SIGNAL] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CallHistoryViewGtkClass, clicked),
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
                                      "icon-name", COLUMN_PIXBUF);
  g_object_set (renderer, "xalign", 0.0, "yalign", 0.5, "xpad", 6, "stock-size", 1, NULL);

  /* show name and text */
  renderer = gm_cell_renderer_bitext_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "primary-text", COLUMN_NAME);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "secondary-text", COLUMN_INFO);
  gtk_tree_view_append_column (self->priv->tree, column);

  /* react to user clicks */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (self->priv->tree, "event-after",
                    G_CALLBACK (on_clicked), self);

  /* connect to the signals */
  self->priv->conns.add (book->contact_added.connect (boost::bind (&on_book_contact_added, _1, self)));

  /* initial populate */
  self->priv->book->visit_contacts (boost::bind (&on_visit_contacts, _1, GTK_LIST_STORE (store)));

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
