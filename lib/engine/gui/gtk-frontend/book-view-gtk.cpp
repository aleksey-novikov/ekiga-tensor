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
 *                         book-view-gtk.cpp -  description
 *                         ------------------------------------------
 *   begin                : written in 2006 by Julien Puydt
 *   copyright            : (c) 2006 by Julien Puydt
 *   description          : implementation of an addressbook view
 *
 */

#include <glib/gi18n.h>
#include <boost/assign/ptr_list_of.hpp>

#include "book-view-gtk.h"

#include "gm-info-bar.h"

#include "filterable.h"

#include "gactor-menu.h"
#include "scoped-connections.h"

/*
 * The Book View
 */
struct _BookViewGtkPrivate
{
  _BookViewGtkPrivate (Ekiga::BookPtr book_) : book (book_) { }

  GtkTreeView *tree_view;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *search_button;
  GtkWidget *searchbar;
  GtkWidget *info_bar;
  GtkWidget *scrolled_window;

  Ekiga::GActorMenuPtr book_menu;
  Ekiga::GActorMenuPtr contact_menu;

  Ekiga::BookPtr book;
  Ekiga::scoped_connections connections;
};


enum {

  COLUMN_CONTACT_POINTER,
  COLUMN_PIXBUF,
  COLUMN_NAME,
  COLUMN_NUMBER
};


enum {
  ACTIONS_CHANGED_SIGNAL,
  LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (BookViewGtk, book_view_gtk, GTK_TYPE_FRAME);

/*
 * Callbacks
 */

/* DESCRIPTION  : Called when the a contact has been added in a Book.
 * BEHAVIOR     : Update the BookView.
 * PRE          : The gpointer must point to the BookViewGtk GObject.
 */
static bool on_visit_contacts (Ekiga::ContactPtr contact,
			       gpointer data);

/* DESCRIPTION  : Called when the a contact has been added in a Book.
 * BEHAVIOR     : Update the BookView.
 * PRE          : The gpointer must point to the BookViewGtk GObject.
 */
static void on_contact_added (Ekiga::ContactPtr contact,
			      gpointer data);


/* DESCRIPTION  : Called when the a contact has been updated in a Book.
 * BEHAVIOR     : Update the BookView.
 * PRE          : The gpointer must point to the BookViewGtk GObject.
 */
static void on_contact_updated (Ekiga::ContactPtr contact,
				gpointer data);


/* DESCRIPTION  : Called when the a contact has been removed from a Book.
 * BEHAVIOR     : Update the BookView.
 * PRE          : The gpointer must point to the BookViewGtk GObject.
 */
static void on_contact_removed (Ekiga::ContactPtr contact,
				gpointer data);


/* DESCRIPTION  : Called when the user selects a contact.
 * BEHAVIOR     : Rebuilds menus and emit the actions_changed signal.
 * PRE          : The gpointer must point to the BookViewGtk GObject.
 */
static void on_selection_changed (GtkTreeSelection *selection,
                                  gpointer data);


/* DESCRIPTION  : Called when the user activates the filter GtkEntry.
 * BEHAVIOR     : Updates the Book search filter, which triggers
 *                a refresh.
 * PRE          : A valid pointer to the BookViewGtk.
 */
static void on_search_entry_activated_cb (GtkWidget *entry,
                                          gpointer data);


/* DESCRIPTION  : Called when the a contact is clicked.
 * BEHAVIOR     : Displays a popup menu.
 * PRE          : The gpointer must point to a BookViewGtk GObject.
 */
static gint on_contact_clicked (GtkWidget *tree_view,
                                GdkEventButton *event,
                                gpointer data);


/* DESCRIPTION  : Called when the BookViewGtk widget becomes visible.
 * BEHAVIOR     : Add its own search action (if it is filterable).
 *                Calls on_selection_changed to update actions.
 * PRE          : /
 */
static void on_map_cb (G_GNUC_UNUSED GtkWidget *widget,
                       gpointer data);


/* DESCRIPTION  : Called when the BookViewGtk widget becomes invisible.
 * BEHAVIOR     : Remove Actions. They will be mapped again when we
 *                are mapped again.
 * PRE          : /
 */
static void on_unmap_cb (G_GNUC_UNUSED GtkWidget *widget,
                         gpointer data);


/* DESCRIPTION  : Called when the user triggers the search action.
 * BEHAVIOR     : Toggle the search bar.
 * PRE          : /
 */
static void on_search_cb (G_GNUC_UNUSED GSimpleAction *action,
                          G_GNUC_UNUSED GVariant *parameter,
                          gpointer data);


/* Static functions */

/* DESCRIPTION  : /
 * BEHAVIOR     : Add the contact to the BookViewGtk.
 * PRE          : /
 */
static void
book_view_gtk_add_contact (BookViewGtk *self,
                           Ekiga::ContactPtr contact);


/* DESCRIPTION  : /
 * BEHAVIOR     : Updated the contact in the BookViewGtk.
 * PRE          : /
 */
static void
book_view_gtk_update_contact (BookViewGtk *self,
                              Ekiga::ContactPtr contact,
                              GtkTreeIter *iter);


/* DESCRIPTION  : /
 * BEHAVIOR     : Remove the contact from the BookViewGtk.
 * PRE          : /
 */
static void
book_view_gtk_remove_contact (BookViewGtk *self,
                              Ekiga::ContactPtr contact);


/* DESCRIPTION  : /
 * BEHAVIOR     : Create and return the window GtkSearchBar.
 * PRE          : /
 */
static GtkWidget *
book_view_build_searchbar (BookViewGtk *self);


/* DESCRIPTION  : /
 * BEHAVIOR     : Return TRUE and update the GtkTreeIter if we found
 *                the iter corresponding to the Contact in the BookViewGtk.
 * PRE          : /
 */
static gboolean
book_view_gtk_find_iter_for_contact (BookViewGtk *view,
                                     Ekiga::ContactPtr contact,
                                     GtkTreeIter *iter);


static GActionEntry win_entries[] =
{
    { "search", on_search_cb, NULL, NULL, NULL, 0 }
};



/* Implementation of the callbacks */
static bool
on_visit_contacts (Ekiga::ContactPtr contact,
		   gpointer data)
{
  on_contact_added (contact, data);
  return true;
}


static void
on_contact_added (Ekiga::ContactPtr contact,
		  gpointer data)
{
  book_view_gtk_add_contact (BOOK_VIEW_GTK (data), contact);
}


static void
on_contact_updated (Ekiga::ContactPtr contact,
		    gpointer data)
{
  BookViewGtk *view = NULL;
  GtkTreeIter iter;

  view = BOOK_VIEW_GTK (data);

  if (book_view_gtk_find_iter_for_contact (view, contact, &iter)) {
    book_view_gtk_update_contact (view, contact, &iter);
  }
}


static void
on_contact_removed (Ekiga::ContactPtr contact,
                    gpointer data)
{
  BookViewGtk *view = NULL;

  view = BOOK_VIEW_GTK (data);
  book_view_gtk_remove_contact (view, contact);
}


static void
on_selection_changed (GtkTreeSelection *selection,
		      gpointer data)
{
  BookViewGtk* self = NULL;
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;

  Ekiga::Contact *contact = NULL;

  self = BOOK_VIEW_GTK (data);
  model = gtk_tree_view_get_model (self->priv->tree_view);

  /* Reset old data. This also ensures GIO actions are
   * properly removed before adding new ones.
   */
  if (self->priv->contact_menu)
    self->priv->contact_menu.reset ();

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get (model, &iter,
                        COLUMN_CONTACT_POINTER, &contact,
                        -1);

    if (contact != NULL) {
      self->priv->contact_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*contact,
                                                                              contact->get_name ()));
      g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0,
                     self->priv->contact_menu->get_model (boost::assign::list_of (self->priv->book_menu)));
    }
  }
  else
    g_signal_emit (self, signals[ACTIONS_CHANGED_SIGNAL], 0,
                   self->priv->book_menu->get_model ());
}


static void
on_search_entry_activated_cb (G_GNUC_UNUSED GtkWidget *entry,
                              gpointer data)
{
  g_return_if_fail (IS_BOOK_VIEW_GTK (data));
  BookViewGtk *self = BOOK_VIEW_GTK (data);

  const char *entry_text = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
  boost::shared_ptr<Ekiga::Filterable> filtered = boost::dynamic_pointer_cast<Ekiga::Filterable>(BOOK_VIEW_GTK (data)->priv->book);
  filtered->set_search_filter (entry_text);
}


static gint
on_contact_clicked (G_GNUC_UNUSED GtkWidget *tree_view,
		    GdkEventButton *event,
		    gpointer data)
{
  g_return_val_if_fail (IS_BOOK_VIEW_GTK (data), FALSE);
  BookViewGtk *self = BOOK_VIEW_GTK (data);

  if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
    gtk_menu_popup (GTK_MENU (self->priv->contact_menu->get_menu ()),
                    NULL, NULL, NULL, NULL, event->button, event->time);
  }

  return TRUE;
}


static void
on_map_cb (G_GNUC_UNUSED GtkWidget *widget,
           gpointer data)
{
  GtkTreeSelection *selection = NULL;

  g_return_if_fail (IS_BOOK_VIEW_GTK (data));
  BookViewGtk *self = BOOK_VIEW_GTK (data);

  g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                              "search");
  if (self->priv->searchbar)
    g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                     win_entries, G_N_ELEMENTS (win_entries),
                                     self);
  selection = gtk_tree_view_get_selection (self->priv->tree_view);
  on_selection_changed (selection, self);
}


static void
on_unmap_cb (G_GNUC_UNUSED GtkWidget *widget,
             gpointer data)
{
  g_return_if_fail (IS_BOOK_VIEW_GTK (data));
  BookViewGtk *self = BOOK_VIEW_GTK (data);

  g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                              "search");

  /* Reset old data. This also ensures GIO actions are
   * properly removed when we are going out of scope.
   */
  if (self->priv->contact_menu)
    self->priv->contact_menu.reset ();
}



static void
on_search_cb (G_GNUC_UNUSED GSimpleAction *action,
              G_GNUC_UNUSED GVariant *parameter,
              gpointer data)
{
  g_return_if_fail (IS_BOOK_VIEW_GTK (data));
  BookViewGtk *self = BOOK_VIEW_GTK (data);

  gboolean toggle = gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (self->priv->searchbar));

  gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (self->priv->searchbar),
                                  !toggle);
}


/* Implementation of the static functions */
static void
book_view_gtk_add_contact (BookViewGtk *self,
                           Ekiga::ContactPtr contact)
{
  GtkTreeModel *model = NULL;
  GtkListStore *store = NULL;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (self->priv->tree_view);
  store = GTK_LIST_STORE (model);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, COLUMN_CONTACT_POINTER, contact.get (), -1);
  book_view_gtk_update_contact (self, contact, &iter);
}


static void
book_view_gtk_update_contact (BookViewGtk *self,
			      Ekiga::ContactPtr contact,
			      GtkTreeIter *iter)
{
  GtkListStore *store = NULL;
  GdkPixbuf *pixbuf = NULL;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (self->priv->tree_view));
  pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     "avatar-default",
                                     GTK_ICON_SIZE_MENU, (GtkIconLookupFlags) 0, NULL);
  gtk_list_store_set (store, iter,
                      COLUMN_PIXBUF, pixbuf,
		      COLUMN_NAME, contact->get_name ().c_str (),
		      -1);
  if (pixbuf)
    g_object_unref (pixbuf);
}


static void
book_view_gtk_remove_contact (BookViewGtk *self,
                              Ekiga::ContactPtr contact)
{
  GtkTreeModel *model = NULL;
  GtkListStore *store = NULL;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (self->priv->tree_view);
  store = GTK_LIST_STORE (model);

  while (book_view_gtk_find_iter_for_contact (self, contact, &iter))
    gtk_list_store_remove (store, &iter);
}


static GtkWidget *
book_view_build_searchbar (BookViewGtk *self)
{
  GtkWidget *box = NULL;
  GtkWidget *label = NULL;
  GtkWidget *image = NULL;
  GtkWidget *searchbar = NULL;

  searchbar = gtk_search_bar_new ();
  gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (searchbar), TRUE);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (searchbar), box);
  gtk_widget_show (box);

  label = gtk_label_new_with_mnemonic (_("_Search Filter:"));
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  self->priv->entry = gtk_search_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), self->priv->entry, TRUE, TRUE, 0);
  gtk_widget_show (self->priv->entry);

  self->priv->search_button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (self->priv->search_button), image);
  gtk_box_pack_start (GTK_BOX (box), self->priv->search_button, TRUE, TRUE, 0);
  gtk_widget_show (self->priv->search_button);

  g_signal_connect (self->priv->entry, "activate",
                    G_CALLBACK (on_search_entry_activated_cb), self);
  g_signal_connect (self->priv->search_button, "clicked",
                    G_CALLBACK (on_search_entry_activated_cb), self);

  gtk_search_bar_connect_entry (GTK_SEARCH_BAR (searchbar), GTK_ENTRY (self->priv->entry));

  return searchbar;
}



static gboolean
book_view_gtk_find_iter_for_contact (BookViewGtk *view,
                                     Ekiga::ContactPtr contact,
                                     GtkTreeIter *iter)
{
  GtkTreeModel *model = NULL;
  gboolean found = FALSE;

  model = gtk_tree_view_get_model (view->priv->tree_view);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), iter)) {

    do {

      Ekiga::Contact *iter_contact = NULL;
      gtk_tree_model_get (model, iter,
                          COLUMN_CONTACT_POINTER, &iter_contact,
                          -1);
      if (iter_contact == contact.get ())
        found = TRUE;

    } while (!found && gtk_tree_model_iter_next (model, iter));
  }

  return found;
}


/* GObject boilerplate code */
static void
book_view_gtk_dispose (GObject *obj)
{
  BookViewGtk *self = NULL;

  self = BOOK_VIEW_GTK (obj);

  if (self->priv) {
    delete self->priv;
    self->priv = NULL;
  }

  G_OBJECT_CLASS (book_view_gtk_parent_class)->dispose (obj);
}


static void
book_view_gtk_init (G_GNUC_UNUSED BookViewGtk* self)
{
  /* can't do anything here... waiting for a core :-/ */
}


static void
book_view_gtk_class_init (BookViewGtkClass* klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = book_view_gtk_dispose;

  signals[ACTIONS_CHANGED_SIGNAL] =
    g_signal_new ("actions-changed",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BookViewGtkClass, selection_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, G_TYPE_MENU_MODEL);
}


/* public methods implementation */
GtkWidget *
book_view_gtk_new (Ekiga::BookPtr book)
{
  BookViewGtk *self = NULL;

  GtkTreeSelection *selection = NULL;
  GtkListStore *store = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkCellRenderer *renderer = NULL;

  self = (BookViewGtk *) g_object_new (BOOK_VIEW_GTK_TYPE, NULL);

  self->priv = new _BookViewGtkPrivate (book);
  self->priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (self), GTK_SHADOW_NONE);
  gtk_widget_show (self->priv->vbox);

  /* populate book actions */
  self->priv->book_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*book,
                                                                       book->get_name ()));

  /* The Search Box */
  boost::shared_ptr<Ekiga::Filterable> filtered = boost::dynamic_pointer_cast<Ekiga::Filterable> (book);
  self->priv->entry = NULL;
  self->priv->search_button = NULL;
  self->priv->searchbar = NULL;
  if (filtered) {
    /* Search bar */
    self->priv->searchbar = book_view_build_searchbar (self);
    gtk_box_pack_start (GTK_BOX (self->priv->vbox),
                        self->priv->searchbar, FALSE, FALSE, 0);
    gtk_widget_show (self->priv->searchbar);
  }

  /* The info bar */
  self->priv->info_bar = gm_info_bar_new ();
  gtk_box_pack_start (GTK_BOX (self->priv->vbox),
                      self->priv->info_bar, FALSE, FALSE, 0);

  /* The List Store */
  self->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
				  (self->priv->scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  self->priv->tree_view = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_tree_view_set_headers_visible (self->priv->tree_view, FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (self->priv->tree_view), FALSE);
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->priv->vbox));
  gtk_box_pack_start (GTK_BOX (self->priv->vbox),
		     GTK_WIDGET (self->priv->scrolled_window), TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (self->priv->scrolled_window),
		     GTK_WIDGET (self->priv->tree_view));

  selection = gtk_tree_view_get_selection (self->priv->tree_view);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (on_selection_changed), self);

  g_signal_connect (self->priv->tree_view, "event-after",
                    G_CALLBACK (on_contact_clicked), self);

  store = gtk_list_store_new (COLUMN_NUMBER,
			      G_TYPE_POINTER,
                              GDK_TYPE_PIXBUF,
                              G_TYPE_STRING);

  gtk_tree_view_set_model (self->priv->tree_view, GTK_TREE_MODEL (store));
  g_object_unref (store);

  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "pixbuf", COLUMN_PIXBUF, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_NAME,
                                       NULL);

  gtk_tree_view_column_set_title (column, _("Full Name"));
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_resizable (column, true);
  gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view), column);

  gtk_widget_show_all (self->priv->scrolled_window);

  /* connect to the signals */
  self->priv->connections.add (book->contact_added.connect (boost::bind (&on_contact_added, _1, (gpointer)self)));
  self->priv->connections.add (book->contact_updated.connect (boost::bind (&on_contact_updated, _1, (gpointer)self)));
  self->priv->connections.add (book->contact_removed.connect (boost::bind (&on_contact_removed, _1, (gpointer)self)));

  /* populate */
  book->visit_contacts (boost::bind (&on_visit_contacts, _1, (gpointer)self));

  g_signal_connect (GTK_WIDGET (self), "map",
                    G_CALLBACK (on_map_cb), self);
  g_signal_connect (GTK_WIDGET (self), "unmap",
                    G_CALLBACK (on_unmap_cb), self);

  return GTK_WIDGET (self);
}


gboolean
book_view_gtk_handle_event (BookViewGtk *self,
                            GdkEvent *event)
{
  if (self->priv->searchbar)
    return gtk_search_bar_handle_event (GTK_SEARCH_BAR (self->priv->searchbar),
                                        event);

  return FALSE;
}
