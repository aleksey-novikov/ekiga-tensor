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
 *                          assistant.cpp  -  description
 *                          -----------------------------
 *   begin                : Mon May 1 2002
 *   copyright            : (C) 2000-2008 by Damien Sandras
 *                          (C) 2008 by Steve Frécinaux
 *   description          : This file contains all the functions needed to
 *                          build the assistant.
 */

#include <glib/gi18n.h>

#include "services.h"
#include "account.h"

#include "gm-entry.h"
#include "platform.h"
#include "assistant-window.h"
#include "ekiga-app.h"
#include "opal-bank.h"

#include "ekiga-settings.h"

#include <gdk/gdkkeysyms.h>

G_DEFINE_TYPE (AssistantWindow, assistant_window, GTK_TYPE_ASSISTANT);

struct _AssistantWindowPrivate
{
  boost::shared_ptr<Opal::Bank> bank;
  GdkPixbuf *icon;

  GtkWidget *welcome_page;
  GtkWidget *info_page;
  GtkWidget *ekiga_net_page;
  GtkWidget *ekiga_out_page;
  GtkWidget *summary_page;

  GtkWidget *name;

  GtkWidget *username;
  GtkWidget *password;
  GtkWidget *skip_ekiga_net;

  GtkWidget *dusername;
  GtkWidget *dpassword;
  GtkWidget *skip_ekiga_out;

  gint last_active_page;

  GtkListStore *summary_model;

  boost::shared_ptr<Ekiga::Settings> personal_data_settings;
};

enum {
  SUMMARY_KEY_COLUMN,
  SUMMARY_VALUE_COLUMN
};


static GtkWidget *
create_page (AssistantWindow       *assistant,
             const gchar          *title,
             GtkAssistantPageType  page_type)
{
  GtkWidget *vbox;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  gtk_assistant_append_page (GTK_ASSISTANT (assistant), vbox);
  gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), vbox, title);
  gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), vbox, page_type);

  if (page_type != GTK_ASSISTANT_PAGE_CONTENT)
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), vbox, TRUE);

  return vbox;
}

static void
set_current_page_complete (GtkAssistant *assistant,
                           gboolean      complete)
{
  gint page_number;
  GtkWidget *current_page;

  page_number = gtk_assistant_get_current_page (assistant);
  current_page = gtk_assistant_get_nth_page (assistant, page_number);
  gtk_assistant_set_page_complete (assistant, current_page, complete);
}

static void
create_welcome_page (AssistantWindow *assistant)
{
  GtkWidget *label;

  label = gtk_label_new (_("This is the Ekiga general configuration assistant. "
                           "The following steps will set up Ekiga by asking "
                           "a few simple questions.\n\nOnce you have completed "
                           "these steps, you can always change them later by "
                           "selecting Preferences in the Edit menu."));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_widget_show (label);
  gtk_assistant_append_page (GTK_ASSISTANT (assistant), label);
  gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), label, _("Welcome to Ekiga"));
  gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), label, GTK_ASSISTANT_PAGE_INTRO);
  gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), label, TRUE);

  assistant->priv->welcome_page = label;
}


static void
create_info_page (AssistantWindow *assistant)
{
  GtkWidget *label;

  label = gtk_label_new (_("You usually need a SIP account in order to use Ekiga.\n"
                           "Many services allow you creating such accounts.\n\n"
                           "We suggest that you use a free ekiga.im account, "
                           "which allows you to be called by any person with a SIP account.\n\n"
                           "If you want to call regular phone lines too, we suggest "
                           "that you purchase an inexpensive call out account."
                           "\n\nThe following two pages allow you creating "
                           "such accounts."));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_widget_show (label);
  gtk_assistant_append_page (GTK_ASSISTANT (assistant), label);
  gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), label, _("Introduction to Accounts"));
  gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), label, GTK_ASSISTANT_PAGE_CONTENT);
  gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), label, TRUE);

  assistant->priv->info_page = label;
}


static void
ekiga_net_button_clicked_cb (G_GNUC_UNUSED GtkWidget *button,
                             G_GNUC_UNUSED gpointer data)
{
  gm_platform_open_uri ("http://www.ekiga.im");
}


static void
ekiga_out_new_clicked_cb (G_GNUC_UNUSED GtkWidget *widget,
                          gpointer data)
{
  AssistantWindow *assistant = NULL;

  const char *account = NULL;
  const char *password = NULL;

  assistant = ASSISTANT_WINDOW (data);

  account = gtk_entry_get_text (GTK_ENTRY (assistant->priv->dusername));
  password = gtk_entry_get_text (GTK_ENTRY (assistant->priv->dpassword));

  if (account == NULL || password == NULL)
    return; /* no account configured yet */

  gm_platform_open_uri ("https://www.diamondcard.us/exec/voip-login?act=sgn&spo=ekiga");
}


static void
ekiga_net_info_changed_cb (G_GNUC_UNUSED GtkWidget *w,
                           AssistantWindow *assistant)
{
  gboolean complete = FALSE;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_net)))
    complete = TRUE;
  else
    complete = gm_entry_text_is_valid (GM_ENTRY (assistant->priv->username))
      && gm_entry_text_is_valid (GM_ENTRY (assistant->priv->password));

  set_current_page_complete (GTK_ASSISTANT (assistant), complete);
}


static void
ekiga_out_info_changed_cb (G_GNUC_UNUSED GtkWidget *w,
                           AssistantWindow *assistant)
{
  gboolean complete = FALSE;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_out)))
    complete = TRUE;
  else
    complete = gm_entry_text_is_valid (GM_ENTRY (assistant->priv->dusername))
      && gm_entry_text_is_valid (GM_ENTRY (assistant->priv->dpassword));
  std::cout << gm_entry_text_is_valid (GM_ENTRY (assistant->priv->dusername)) << std::endl << std::flush;

  set_current_page_complete (GTK_ASSISTANT (assistant), complete);
}


static void
create_ekiga_net_page (AssistantWindow *assistant)
{
  GtkWidget *vbox;
  GtkWidget *label;
  gchar *text;
  GtkWidget *button;

  vbox = create_page (assistant, _("Ekiga.im Account"), GTK_ASSISTANT_PAGE_CONTENT);

  label = gtk_label_new (_("Please enter your ekiga.im SIP address:"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  assistant->priv->username = gm_entry_new (EKIGA_URI_REGEX);
  gm_entry_set_allow_empty (GM_ENTRY (assistant->priv->username), FALSE);
  gtk_entry_set_activates_default (GTK_ENTRY (assistant->priv->username), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), assistant->priv->username, FALSE, FALSE, 0);

  label = gtk_label_new (_("Please enter your password:"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  assistant->priv->password = gm_entry_new (NULL);
  gm_entry_set_allow_empty (GM_ENTRY (assistant->priv->password), FALSE);
  gtk_entry_set_activates_default (GTK_ENTRY (assistant->priv->password), TRUE);
  gtk_entry_set_visibility (GTK_ENTRY (assistant->priv->password), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), assistant->priv->password, FALSE, FALSE, 0);

  label = gtk_label_new (NULL);
  text = g_strdup_printf ("<i>%s</i>", _("The ekiga.im SIP address and password are used "
					 "to login to your existing account at the ekiga.im "
					 "free SIP service. If you do not have an ekiga.im "
					 "SIP address yet, you may first create an account "
					 "below. This will provide a SIP address that allows "
					 "people to call you.\n\nYou may skip this step if "
					 "you use an alternative SIP service, or if you "
					 "would prefer to specify the login details later."));
  gtk_label_set_markup (GTK_LABEL (label), text);
  g_free (text);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

  button = gtk_button_new ();
  label = gtk_label_new (NULL);
  text = g_strdup_printf ("<span foreground=\"blue\"><u>%s</u></span>",
                          _("Get an Ekiga.im SIP account"));
  gtk_label_set_markup (GTK_LABEL (label), text);
  g_free (text);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (button), FALSE, FALSE, 10);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (ekiga_net_button_clicked_cb), NULL);

  assistant->priv->skip_ekiga_net = gtk_check_button_new_with_label (_("I do not want to sign up for the ekiga.im free service"));
  gtk_box_pack_start (GTK_BOX (vbox), assistant->priv->skip_ekiga_net, TRUE, TRUE, 0);

  g_signal_connect (assistant->priv->username, "changed",
                    G_CALLBACK (ekiga_net_info_changed_cb), assistant);
  g_signal_connect (assistant->priv->password, "changed",
                    G_CALLBACK (ekiga_net_info_changed_cb), assistant);
  g_signal_connect (assistant->priv->skip_ekiga_net, "toggled",
                    G_CALLBACK (ekiga_net_info_changed_cb), assistant);

  assistant->priv->ekiga_net_page = vbox;
  gtk_widget_show_all (vbox);
}


static void
prepare_ekiga_net_page (AssistantWindow *assistant)
{
  Opal::AccountPtr account = assistant->priv->bank->find_account ("ekiga.net");
  gboolean complete = FALSE;

  if (account && !account->get_username ().empty ())
    gtk_entry_set_text (GTK_ENTRY (assistant->priv->username), (account->get_username () + "@ekiga.net").c_str ());
  if (account && !account->get_password ().empty ())
    gtk_entry_set_text (GTK_ENTRY (assistant->priv->password), account->get_password ().c_str ());

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_net)))
    complete = TRUE;
  else
    complete = gm_entry_text_is_valid (GM_ENTRY (assistant->priv->username))
      && gm_entry_text_is_valid (GM_ENTRY (assistant->priv->password));

  set_current_page_complete (GTK_ASSISTANT (assistant), complete);
}


static void
apply_ekiga_net_page (AssistantWindow *assistant)
{
  Opal::AccountPtr account = assistant->priv->bank->find_account ("ekiga.net");
  bool new_account = !account;
  const char *address = gtk_entry_get_text (GTK_ENTRY (assistant->priv->username));
  gchar **split = g_strsplit (address, "@", 1);
  if (!split || !split[0]) {
    g_strfreev (split);
    return;
  }

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_net))) {
    if (new_account)
      assistant->priv->bank->new_account (Opal::Account::Ekiga,
					  split[0],
					  gtk_entry_get_text (GTK_ENTRY (assistant->priv->password)));
    else
      account->set_authentication_settings (split[0],
					    gtk_entry_get_text (GTK_ENTRY (assistant->priv->password)));
  }

  g_strfreev (split);
}


static void
create_ekiga_out_page (AssistantWindow *assistant)
{
  GtkWidget *vbox;
  GtkWidget *label;
  gchar *text;
  GtkWidget *button;

  vbox = create_page (assistant, _("Ekiga Call Out Account"), GTK_ASSISTANT_PAGE_CONTENT);

  label = gtk_label_new (_("Please enter your account ID:"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  assistant->priv->dusername = gm_entry_new (NUMBER_REGEX);
  gm_entry_set_allow_empty (GM_ENTRY (assistant->priv->dusername), FALSE);
  gtk_entry_set_activates_default (GTK_ENTRY (assistant->priv->dusername), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), assistant->priv->dusername, FALSE, FALSE, 0);

  label = gtk_label_new (_("Please enter your PIN code:"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  assistant->priv->dpassword = gm_entry_new (NUMBER_REGEX);
  gm_entry_set_allow_empty (GM_ENTRY (assistant->priv->dusername), FALSE);
  gtk_entry_set_activates_default (GTK_ENTRY (assistant->priv->dpassword), TRUE);
  gtk_entry_set_visibility (GTK_ENTRY (assistant->priv->dpassword), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), assistant->priv->dpassword, FALSE, FALSE, 0);

  label = gtk_label_new (NULL);
  text = g_strdup_printf ("<i>%s</i>",
                          _("You can make calls to regular phones and cell numbers worldwide using Ekiga. "
                            "To enable this, you need to do two things:\n"
                            "- First buy an account at the URL below.\n"
                            "- Then enter your account ID and PIN code.\n"
                            "The service will work only if your account is created using the URL in this dialog.\n"));
  gtk_label_set_markup (GTK_LABEL (label), text);
  g_free (text);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

  button = gtk_button_new ();
  label = gtk_label_new (NULL);
  text = g_strdup_printf ("<span foreground=\"blue\"><u>%s</u></span>",
                          _("Get an Ekiga Call Out account"));
  gtk_label_set_markup (GTK_LABEL (label), text);
  g_free (text);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (button), FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (ekiga_out_new_clicked_cb), assistant);

  assistant->priv->skip_ekiga_out = gtk_check_button_new_with_label (_("I do not want to sign up for the Ekiga Call Out service"));
  gtk_box_pack_start (GTK_BOX (vbox), assistant->priv->skip_ekiga_out, TRUE, TRUE, 0);

  g_signal_connect (assistant->priv->dusername, "changed",
                    G_CALLBACK (ekiga_out_info_changed_cb), assistant);
  g_signal_connect (assistant->priv->dpassword, "changed",
                    G_CALLBACK (ekiga_out_info_changed_cb), assistant);
  g_signal_connect (assistant->priv->skip_ekiga_out, "toggled",
                    G_CALLBACK (ekiga_out_info_changed_cb), assistant);

  assistant->priv->ekiga_out_page = vbox;
  gtk_widget_show_all (vbox);
}


static void
prepare_ekiga_out_page (AssistantWindow *assistant)
{
  Opal::AccountPtr account = assistant->priv->bank->find_account ("sip.diamondcard.us");
  gboolean complete = FALSE;

  if (account && !account->get_username ().empty ())
    gtk_entry_set_text (GTK_ENTRY (assistant->priv->dusername), account->get_username ().c_str ());
  if (account && !account->get_password ().empty ())
    gtk_entry_set_text (GTK_ENTRY (assistant->priv->dpassword), account->get_password ().c_str ());

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_out)))
    complete = TRUE;
  else
    complete = gm_entry_text_is_valid (GM_ENTRY (assistant->priv->dusername))
      && gm_entry_text_is_valid (GM_ENTRY (assistant->priv->dpassword));

  set_current_page_complete (GTK_ASSISTANT (assistant), complete);
}


static void
apply_ekiga_out_page (AssistantWindow *assistant)
{
  /* Some specific Opal stuff for the Ekiga.net account */
  Opal::AccountPtr account = assistant->priv->bank->find_account ("sip.diamondcard.us");
  bool new_account = !account;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_out))) {
    if (new_account)
      assistant->priv->bank->new_account (Opal::Account::DiamondCard,
					  gtk_entry_get_text (GTK_ENTRY (assistant->priv->dusername)),
					  gtk_entry_get_text (GTK_ENTRY (assistant->priv->dpassword)));
    else
      account->set_authentication_settings (gtk_entry_get_text (GTK_ENTRY (assistant->priv->dusername)),
					    gtk_entry_get_text (GTK_ENTRY (assistant->priv->dpassword)));
  }
}


static void
create_summary_page (AssistantWindow *assistant)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *sw;
  GtkWidget *tree;
  GtkTreeViewColumn *column;
  GtkCellRenderer *cell;

  vbox = create_page (assistant, _("Configuration Complete"), GTK_ASSISTANT_PAGE_CONFIRM);

  label = gtk_label_new (_("You have now finished the Ekiga configuration. All "
			   "the settings can be changed in the Ekiga preferences. "
			   "Enjoy!"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new (_("Configuration summary:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);


  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

  assistant->priv->summary_model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  tree = gtk_tree_view_new ();
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                           GTK_TREE_MODEL (assistant->priv->summary_model));
  gtk_container_add (GTK_CONTAINER (sw), tree);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Option", cell,
                                                     "text", SUMMARY_KEY_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Value", cell,
                                                     "text", SUMMARY_VALUE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  assistant->priv->summary_page = vbox;
  gtk_widget_show_all (vbox);
}


static void
prepare_summary_page (AssistantWindow *assistant)
{
  GtkListStore *model = assistant->priv->summary_model;
  GtkTreeIter iter;

  gtk_list_store_clear (model);

  /* The full name */
  gtk_list_store_append (model, &iter);
  gtk_list_store_set (model, &iter,
                      SUMMARY_KEY_COLUMN, _("Full Name"),
                      SUMMARY_VALUE_COLUMN, gtk_entry_get_text (GTK_ENTRY (assistant->priv->name)),
                      -1);

  /* The ekiga.net account */
  {
    gchar* value = NULL;
    gtk_list_store_append (model, &iter);
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_net)))
      value = g_strdup_printf ("sip:%s@ekiga.net",
			       gtk_entry_get_text (GTK_ENTRY (assistant->priv->username)));
    else
      value = g_strdup ("None");
    gtk_list_store_set (model, &iter,
			SUMMARY_KEY_COLUMN, _("SIP URI"),
			SUMMARY_VALUE_COLUMN, value,
			-1);
    g_free (value);
  }

  /* Ekiga Call Out */
  {
    gchar *value = NULL;
    gtk_list_store_append (model, &iter);
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (assistant->priv->skip_ekiga_out)))
      value = g_strdup (gtk_entry_get_text (GTK_ENTRY (assistant->priv->dusername)));
    else
      value = g_strdup ("None");
    gtk_list_store_set (model, &iter,
			SUMMARY_KEY_COLUMN, _("Ekiga Call Out"),
			SUMMARY_VALUE_COLUMN, value,
			-1);
    g_free (value);
  }
}


static void
assistant_window_init (AssistantWindow *assistant)
{
  assistant->priv = new AssistantWindowPrivate;

  gtk_window_set_default_size (GTK_WINDOW (assistant), 500, 300);
  gtk_window_set_position (GTK_WINDOW (assistant), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width (GTK_CONTAINER (assistant), 12);

  assistant->priv->last_active_page = 0;
  assistant->priv->personal_data_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (PERSONAL_DATA_SCHEMA));

  create_welcome_page (assistant);
  create_info_page (assistant);
  create_ekiga_net_page (assistant);
  create_ekiga_out_page (assistant);
  create_summary_page (assistant);
}


static void
assistant_window_prepare (GtkAssistant *gtkassistant,
			  GtkWidget    *page)
{
  AssistantWindow *assistant = ASSISTANT_WINDOW (gtkassistant);
  gchar *title = NULL;
  bool forward = false;

  title = g_strdup_printf (_("Ekiga Configuration Assistant (%d of %d)"),
                           gtk_assistant_get_current_page (gtkassistant) + 1,
                           gtk_assistant_get_n_pages (gtkassistant));

  gtk_window_set_title (GTK_WINDOW (assistant), title);
  g_free (title);

  if (assistant->priv->last_active_page < gtk_assistant_get_current_page (gtkassistant))
    forward = true;
  assistant->priv->last_active_page = gtk_assistant_get_current_page (gtkassistant);

  if (!forward)
    return;

  if (page == assistant->priv->ekiga_net_page) {
    prepare_ekiga_net_page (assistant);
    return;
  }

  if (page == assistant->priv->ekiga_out_page) {
    prepare_ekiga_out_page (assistant);
    return;
  }

  if (page == assistant->priv->summary_page) {
    prepare_summary_page (assistant);
    return;
  }
}


static void
assistant_window_apply (GtkAssistant *gtkassistant)
{
  AssistantWindow *assistant = ASSISTANT_WINDOW (gtkassistant);

  apply_ekiga_net_page (assistant);
  apply_ekiga_out_page (assistant);
}


static void
assistant_window_close (GtkAssistant *gtkassistant)
{
  gtk_widget_destroy (GTK_WIDGET (gtkassistant));
}


static void
assistant_window_finalize (GObject *object)
{
  AssistantWindow *assistant = ASSISTANT_WINDOW (object);

  delete assistant->priv;
  assistant->priv = NULL;

  G_OBJECT_CLASS (assistant_window_parent_class)->finalize (object);
}

static void
assistant_window_class_init (AssistantWindowClass *klass)
{
  GtkAssistantClass *assistant_class = GTK_ASSISTANT_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  assistant_class->prepare = assistant_window_prepare;
  assistant_class->apply = assistant_window_apply;
  assistant_class->cancel = assistant_window_close;
  assistant_class->close = assistant_window_close;

  object_class->finalize = assistant_window_finalize;
}


static gboolean
assistant_window_key_press_cb (GtkWidget *widget,
			       GdkEventKey *event,
			       G_GNUC_UNUSED gpointer user_data)
{
  if (event->keyval == GDK_KEY_Escape) {

    gtk_widget_destroy (widget);
    return TRUE;  /* do not propagate the key to parent */
  }

  return FALSE; /* propagate what we don't treat */
}


GtkWidget *
assistant_window_new (GmApplication *app)
{
  AssistantWindow *assistant;

  assistant = ASSISTANT_WINDOW (g_object_new (ASSISTANT_WINDOW_TYPE, NULL));
  Ekiga::ServiceCore& core = gm_application_get_core (app);

  /* FIXME: move this into the caller */
  g_signal_connect (assistant, "key-press-event",
                    G_CALLBACK (assistant_window_key_press_cb), NULL);

  boost::signals2::connection conn;
  assistant->priv->bank = core.get<Opal::Bank> ("opal-account-store");

  return GTK_WIDGET (assistant);
}
