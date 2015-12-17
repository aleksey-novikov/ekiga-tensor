/*
 * Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>
 *
 * This program is free software; you can  redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version. This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Ekiga is licensed under the GPL license and as a special exception, you
 * have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination, without
 * applying the requirements of the GNU GPL to the OPAL, OpenH323 and PWLIB
 * programs, as long as you do follow the requirements of the GNU GPL for all
 * the rest of the software thus combined.
 */


/*
 *                         form-dialog.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007-2008 by Julien Puydt
 *                          (c) 2014 by Damien Sandras
 *   description          : implementation of a gtk+ representation of forms
 *
 */


#include <glib/gi18n.h>
#include <ptlib.h>

#include "platform.h"
#include "form-dialog-gtk.h"
#include "gm-entry.h"

/*
 * Declarations : GTK+ Callbacks
 */

/** Called when a GmEntry validity has changed.
 *
 * The GmEntry is considered valid iff:
 *   - The GmEntry content is empty and it is allowed.
 *   - The GmEntry regex matches
 *
 * The "OK" button will be sensitive iff all Form elements
 * are considered valid.
 *
 * @param: data is a pointer to the FormDialog object.
 */
static void
text_entry_validity_changed_cb (GtkEntry *entry,
                                gpointer data);


/** Called when a choice has been toggled in the
 * GtkListStore.
 *
 * Toggle the choice.
 *
 * @param: data is a pointer to the GtkListStore representing
 * the list of choices.
 */
static void
multiple_choice_choice_toggled_cb (GtkCellRendererToggle *cell,
				   gchar *path_str,
				   gpointer data);


/** Called when the GtkEntry aiming at adding a new
 * value has been activated.
 *
 * Checks if the proposed value is not already in
 * the list, add it to the values if it is not the
 * case.
 *
 * @param: data is a pointer to the GtkListStore presenting
 * the list of values.
 */
static void
editable_list_add_value_activated_cb (GtkWidget *entry,
				     gpointer data);


/** Called when an entry in the GtkListStore has been
 * edited.
 *
 * Checks if the proposed value is not already in
 * the list, modify the edited value if it is not the
 * case.
 *
 * @param: data is a pointer to the GtkListStore presenting
 * the list of values.
 */
static void
editable_list_edit_value_cb (GtkCellRendererText *cell,
                            gchar *path_string,
                            gchar *value,
                            gpointer data);


/** Called when the GtkButton to add a value
 * has been clicked.
 *
 * Emit the 'activated' signal on the GtkEntry
 * to trigger editable_list_add_value_activated_cb.
 *
 * @param: data is a pointer to the GtkEntry containing
 * the new value.
 */
static void
editable_list_add_value_clicked_cb (GtkWidget *button,
				   gpointer data);


/** Called when a choice has been toggled in the
 * GtkListStore.
 *
 * Toggle the choice.
 *
 * @param: data is a pointer to the GtkListStore representing
 * the list of choices.
 */
static void
editable_list_choice_toggled_cb (GtkCellRendererToggle *cell,
				gchar *path_str,
				gpointer data);


/** Called when a link in a Form is clicked.
 * Open the URI.
 *
 * @param: The URI to open.
 */
static void
link_clicked_cb (GtkWidget *widget,
                 gpointer data);


/* Declarations and implementation : the various submitters
 * of a Form
 */
class TitleSubmitter: public Submitter
{
public:

  TitleSubmitter (const std::string _title):
    title (_title)
  { }

  ~TitleSubmitter ()
  { }

  void submit (Ekiga::FormBuilder &builder)
  {
    builder.title (title);
  }

private:

  const std::string title;
};


class ActionSubmitter: public Submitter
{
public:

  ActionSubmitter (const std::string _action):
    action (_action)
  { }

  ~ActionSubmitter ()
  { }

  void submit (Ekiga::FormBuilder &builder)
  {
    builder.action (action);
  }

private:

  const std::string action;
};


class InstructionsSubmitter: public Submitter
{
public:

  InstructionsSubmitter (const std::string _instructions): instructions (_instructions)
  { }

  ~InstructionsSubmitter ()
  { }

  void submit (Ekiga::FormBuilder &builder)
  {
    builder.instructions (instructions);
  }

private:

  const std::string instructions;
};


class HiddenSubmitter: public Submitter
{
public:

  HiddenSubmitter (const std::string _name,
		   const std::string _value): name(_name), value(_value)
  {}

  ~HiddenSubmitter ()
  {}

  void submit (Ekiga::FormBuilder &builder)
  {
    builder.hidden (name, value);
  }

private:

  const std::string name;
  const std::string value;
};


class BooleanSubmitter: public Submitter
{
public:

  BooleanSubmitter (const std::string _name,
		    const std::string _description,
		    bool _advanced,
		    GtkWidget *_widget): name(_name),
					 description(_description),
					 advanced(_advanced),
					 widget(_widget)
  { }

  ~BooleanSubmitter ()
  { }

  void submit (Ekiga::FormBuilder &builder)
  {
    builder.boolean (name, description,
		     gtk_switch_get_active (GTK_SWITCH (widget)),
		     advanced);
  }

private:

  const std::string name;
  const std::string description;
  bool advanced;
  GtkWidget *widget;
};


class TextSubmitter: public Submitter
{
public:

  TextSubmitter (const std::string _name,
		 const std::string _description,
		 const std::string _placeholder_text,
                 const Ekiga::FormVisitor::FormTextType _type,
		 bool _advanced,
		 bool _allow_empty,
		 GtkWidget *_widget): name(_name),
				      description(_description),
				      placeholder_text(_placeholder_text),
				      type(_type),
				      advanced(_advanced),
				      allow_empty(_allow_empty),
                                      submit_ok (false),
				      widget(_widget)
  { }

  ~TextSubmitter ()
  { }

  bool can_submit ()
  {
    return gm_entry_text_is_valid (GM_ENTRY (widget));
  }

  void submit (Ekiga::FormBuilder &builder)
  {
    builder.text (name,
                  description,
                  gtk_entry_get_text (GTK_ENTRY (widget)),
                  placeholder_text,
                  type,
                  advanced,
                  allow_empty);
  }

private:

  const std::string name;
  const std::string description;
  const std::string placeholder_text;
  const Ekiga::FormVisitor::FormTextType type;
  bool advanced;
  bool allow_empty;
  bool submit_ok;
  GtkWidget *widget;
};


class MultiTextSubmitter: public Submitter
{
public:

  MultiTextSubmitter (const std::string _name,
		      const std::string _description,
		      bool _advanced,
		      GtkTextBuffer *_buffer): name(_name),
					       description(_description),
					       advanced(_advanced),
					       buffer(_buffer)
  { }

  ~MultiTextSubmitter ()
  { }

  void submit (Ekiga::FormBuilder &builder)
  {
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_start_iter (buffer, &start);
    gtk_text_buffer_get_end_iter (buffer, &end);
    builder.multi_text (name, description,
			gtk_text_buffer_get_text (buffer,
						  &start, &end, FALSE),
			advanced);
  }

private:

  const std::string name;
  const std::string description;
  bool advanced;
  GtkTextBuffer *buffer;
};


class SingleChoiceSubmitter: public Submitter
{
public:

  SingleChoiceSubmitter (const std::string _name,
			 const std::string _description,
			 const std::map<std::string, std::string> _choices,
			 bool _advanced,
			 GtkWidget *_combo): name(_name),
					     description(_description),
					     choices(_choices),
					     advanced(_advanced),
					     combo(_combo)
  { }

  ~SingleChoiceSubmitter ()
  { }

  enum {

    COLUMN_VALUE,
    COLUMN_NAME,
    COLUMN_NUMBER
  };

  void submit (Ekiga::FormBuilder &builder)
  {
    gchar *cvalue = NULL;
    GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
    GtkTreeIter iter;

    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

    gtk_tree_model_get (model, &iter, COLUMN_VALUE, &cvalue, -1);

    builder.single_choice (name, description,
			   std::string (cvalue), choices,
			   advanced);

    g_free (cvalue);
  }

private:

  const std::string name;
  const std::string description;
  const std::map<std::string, std::string> choices;
  bool advanced;
  GtkWidget *combo;
};


class MultipleChoiceSubmitter: public Submitter
{
public:

  MultipleChoiceSubmitter (const std::string _name,
			   const std::string _description,
			   const std::map<std::string, std::string> _choices,
			   bool _advanced,
			   GtkWidget *_tree_view):
    name(_name), description(_description),
    choices(_choices), advanced(_advanced), tree_view (_tree_view)
  { }

  ~MultipleChoiceSubmitter ()
  { }

  enum {

    COLUMN_ACTIVE,
    COLUMN_NAME,
    COLUMN_NUMBER
  };

  void submit (Ekiga::FormBuilder &builder)
  {
    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    gboolean active = FALSE;

    std::set<std::string> values;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {

      do {

	gchar *gname = NULL;

        gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                            COLUMN_ACTIVE, &active,
                            COLUMN_NAME, &gname,
                            -1);

        if (active && gname) {

          values.insert (gname);

          std::map <std::string, std::string>::const_iterator mit;
          mit = choices.find (gname);
          if (mit == choices.end ())
            choices [gname] = gname;
        }

	g_free (gname);
      } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
    }

    builder.multiple_choice (name, description, values, choices, advanced);
  }

private:

  const std::string name;
  const std::string description;
  std::map<std::string, std::string> choices;
  bool advanced;
  GtkWidget *tree_view;
};


class EditableListSubmitter: public Submitter
{
public:

  EditableListSubmitter (const std::string _name,
			const std::string _description,
			bool _advanced,
			GtkWidget *_tree_view):
    name(_name), description(_description), advanced(_advanced), tree_view(_tree_view)
  { }

  ~EditableListSubmitter ()
  { }

  enum {

    COLUMN_ACTIVE,
    COLUMN_VALUE,
    COLUMN_NUMBER
  };

  void submit (Ekiga::FormBuilder &builder)
  {
    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    std::list<std::string> values;
    std::list<std::string> proposed_values;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {

      do {

	gboolean active = FALSE;
	gchar *value = NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
			    COLUMN_ACTIVE, &active,
			    COLUMN_VALUE, &value,
			    -1);

	if (value) {

	  if (active)
	    values.push_back (value);
	  else
	    proposed_values.push_back (value);
	  g_free (value);
	}
      } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
    }

    builder.editable_list (name, description, values, proposed_values, advanced);
  }

private:

  const std::string name;
  const std::string description;
  bool advanced;
  GtkWidget *tree_view;
};


/*
 * GTK+ Callbacks
 */
static void
editable_list_add_value_activated_cb (GtkWidget *entry,
				     gpointer data)
{
  GtkTreeModel *model = NULL;

  const char *value = NULL;
  gchar *tree_value = NULL;

  GtkTreeIter iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));

  value = gtk_entry_get_text (GTK_ENTRY (entry));
  if (!g_strcmp0 (value, ""))
    return;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {

    do {

      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                          EditableListSubmitter::COLUMN_VALUE, &tree_value,
                          -1);
      if (tree_value && !g_strcmp0 (tree_value, value)) {
        g_free (tree_value);
        return;
      }
      g_free (tree_value);

    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
  }

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);

  gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      EditableListSubmitter::COLUMN_ACTIVE, TRUE,
                      EditableListSubmitter::COLUMN_VALUE, gtk_entry_get_text (GTK_ENTRY (entry)),
                      -1);

  gtk_entry_set_text (GTK_ENTRY (entry), "");
}


static void
editable_list_edit_value_cb (G_GNUC_UNUSED GtkCellRendererText *cell,
                             gchar *path_string,
                             gchar *value,
                             gpointer data)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;

  gchar *tree_value = NULL;

  if (!g_strcmp0 (value, ""))
    return;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {

    do {

      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                          EditableListSubmitter::COLUMN_VALUE, &tree_value,
                          -1);
      if (tree_value && !g_strcmp0 (tree_value, value)) {
        g_free (tree_value);
        return;
      }
      g_free (tree_value);

    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
  }

  gtk_tree_model_get_iter_from_string (model, &iter, path_string); 
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      EditableListSubmitter::COLUMN_VALUE, value, -1);
}


static void
editable_list_add_value_clicked_cb (G_GNUC_UNUSED GtkWidget *button,
				   gpointer data)
{
  if (g_strcmp0 (gtk_entry_get_text (GTK_ENTRY (data)), ""))
    gtk_widget_activate (GTK_WIDGET (data));
}


static void
editable_list_choice_toggled_cb (G_GNUC_UNUSED GtkCellRendererToggle *cell,
				gchar *path_str,
				gpointer data)
{
  GtkTreeModel *model = NULL;
  GtkTreePath *path = NULL;

  GtkTreeIter iter;
  gboolean fixed = false;

  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_str);

  /* Update the tree model */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter,
		      EditableListSubmitter::COLUMN_ACTIVE, &fixed,
		      -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      EditableListSubmitter::COLUMN_ACTIVE, fixed^1,
		      -1);
  gtk_tree_path_free (path);
}


static void
text_entry_validity_changed_cb (G_GNUC_UNUSED GtkEntry *entry,
                                gpointer data)
{
  g_return_if_fail (data);

  FormDialog *form_dialog = (FormDialog *) data;
  GtkWidget *dialog = form_dialog->get_dialog ();
  GtkWidget *ok_button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  gtk_widget_set_sensitive (ok_button, form_dialog->can_submit ());
}


static void
multiple_choice_choice_toggled_cb (G_GNUC_UNUSED GtkCellRendererToggle *cell,
				   gchar *path_str,
				   gpointer data)
{
  GtkTreeModel *model = NULL;
  GtkTreePath *path = NULL;

  GtkTreeIter iter;
  gboolean fixed = false;

  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_str);

  /* Update the tree model */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter,
		      MultipleChoiceSubmitter::COLUMN_ACTIVE, &fixed,
		      -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      MultipleChoiceSubmitter::COLUMN_ACTIVE, fixed^1,
		      -1);
  gtk_tree_path_free (path);
}


static void
link_clicked_cb (GtkWidget * /*widget*/,
                 gpointer data)
{
  gm_platform_open_uri ((gchar *) data);
}


FormDialog::FormDialog (Ekiga::FormRequestPtr _request,
			GtkWidget *parent): request(_request)
{
  GtkWidget *vbox = NULL;
  bool use_header = false;
  has_preamble = false;
  rows = 0;
  advanced_rows = 0;

  g_object_get (gtk_settings_get_default (), "gtk-dialogs-use-header", &use_header, NULL);
  window = GTK_WIDGET (g_object_new (GTK_TYPE_DIALOG,
                                     "use-header-bar", use_header,
                                     NULL));
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
  if (GTK_IS_WINDOW (parent))
    gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
  gtk_dialog_add_button (GTK_DIALOG (window),
                         _("Cancel"),
                         GTK_RESPONSE_CANCEL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 18);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 18);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  preamble = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), preamble, FALSE, FALSE, 0);

  fields = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (fields), 6);
  gtk_grid_set_column_spacing (GTK_GRID (fields), 12);
  gtk_box_pack_start (GTK_BOX (vbox), fields, FALSE, FALSE, 0);

  advanced_fields = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (advanced_fields), 6);
  gtk_grid_set_column_spacing (GTK_GRID (advanced_fields), 12);
  expander = gtk_expander_new (_("Advanced"));
  gtk_container_add (GTK_CONTAINER (expander), advanced_fields);
  gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);

  request->visit (*this);
}


FormDialog::~FormDialog ()
{
  gtk_widget_destroy (GTK_WIDGET (window));
  for (std::list<Submitter *>::iterator iter = submitters.begin ();
       iter != submitters.end ();
       iter++)
    delete (*iter);
  submitters.clear ();
}


void
FormDialog::run ()
{
  bool ok = false;

  if (has_preamble)
    gtk_widget_show_all (preamble);
  gtk_widget_show_all (fields);
  if (advanced_rows > 0)
    gtk_widget_show_all (expander);
  gtk_widget_show (window);

  while (!ok) {
    switch (gtk_dialog_run (GTK_DIALOG (window))) {

    case GTK_RESPONSE_ACCEPT:
      ok = submit();
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
    default:
      cancel();
      ok = true;
      break;
    }
  }
}


void
FormDialog::title (const std::string _title)
{
  TitleSubmitter *submitter = NULL;

  gtk_window_set_title (GTK_WINDOW (window), _title.c_str ());

  submitter = new TitleSubmitter (_title);
  submitters.push_back (submitter);
}


void
FormDialog::action (const std::string _action)
{
  ActionSubmitter *submitter = NULL;

  gtk_dialog_add_button (GTK_DIALOG (window),
                         _action.empty () ? _("Done") : _action.c_str (),
                         GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_ACCEPT);

  submitter = new ActionSubmitter (_action);
  submitters.push_back (submitter);
}


void
FormDialog::instructions (const std::string _instructions)
{
  GtkWidget *widget = NULL;
  InstructionsSubmitter *submitter = NULL;

  if (_instructions.empty ())
    return;

  has_preamble = true;

  widget = gtk_label_new_with_mnemonic (_instructions.c_str ());
  gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (widget), PANGO_WRAP_WORD);
  gtk_box_pack_start (GTK_BOX (preamble), widget, FALSE, FALSE, 0);

  submitter = new InstructionsSubmitter (_instructions);
  submitters.push_back (submitter);
}


void
FormDialog::link (const std::string _link,
                  const std::string _uri)
{
  GtkWidget *widget = NULL;
  GtkWidget *label = NULL;
  gchar *label_text = NULL;

  widget = gtk_button_new ();
  label = gtk_label_new (NULL);
  label_text = g_strdup_printf ("<span foreground=\"blue\"><u>%s</u></span>",
                                _link.c_str ());
  gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), label_text);
  g_free (label_text);

  gtk_container_add (GTK_CONTAINER (widget), label);

  gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (preamble), widget, FALSE, FALSE, 0);

  has_preamble = true;

  g_signal_connect_data (widget, "clicked",
                         G_CALLBACK (link_clicked_cb), (gpointer) g_strdup (_uri.c_str ()),
                         (GClosureNotify) g_free, (GConnectFlags) 0);
}


void
FormDialog::error (const std::string _error)
{
  GtkWidget *widget = NULL;

  if (!_error.empty ()) {

    widget = gtk_label_new (NULL);
    gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
    gtk_label_set_line_wrap_mode (GTK_LABEL (widget), PANGO_WRAP_WORD);
    gtk_label_set_markup_with_mnemonic (GTK_LABEL (widget),
					("<span foreground=\"red\">" + _error + "</span>").c_str ());
    gtk_container_add (GTK_CONTAINER (preamble), widget);
    has_preamble = true;
    gtk_widget_show_all (preamble);
  }
}


void
FormDialog::hidden (const std::string name,
		    const std::string value)
{
  HiddenSubmitter *submitter = NULL;

  submitter = new HiddenSubmitter (name, value);
  submitters.push_back (submitter);
}


void
FormDialog::boolean (const std::string name,
		     const std::string description,
		     bool value,
		     bool advanced,
                     bool in_header_bar)
{
  GtkWidget *label = NULL;
  GtkWidget *widget = NULL;
  GtkWidget *header_bar = NULL;

  BooleanSubmitter *submitter = NULL;

  header_bar = gtk_dialog_get_header_bar (GTK_DIALOG (window));

  widget = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (widget), value);
  gtk_widget_set_vexpand (GTK_WIDGET (widget), FALSE);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), FALSE);
  gtk_widget_set_halign (GTK_WIDGET (widget), GTK_ALIGN_START);
  gtk_widget_show (widget);

  if (header_bar && in_header_bar) {
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), widget);
  }
  else {
    label = gtk_label_new_with_mnemonic (description.c_str ());
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);

    grow_fields (advanced);
    if (advanced) {

      gtk_grid_attach (GTK_GRID (advanced_fields), label,
                       0, advanced_rows - 1,
                       1, 1);
      gtk_grid_attach (GTK_GRID (advanced_fields), widget,
                       1, advanced_rows - 1,
                       1, 1);
    } else {

      gtk_grid_attach (GTK_GRID (fields), label,
                       0, rows - 1,
                       1, 1);
      gtk_grid_attach (GTK_GRID (fields), widget,
                       1, rows - 1,
                       1, 1);
    }
  }

  submitter = new BooleanSubmitter (name, description, advanced, widget);
  submitters.push_back (submitter);
}


void
FormDialog::text (const std::string name,
		  const std::string description,
		  const std::string value,
		  const std::string placeholder_text,
                  const Ekiga::FormVisitor::FormTextType type,
		  bool advanced,
                  bool allow_empty)
{
  GtkWidget *label = NULL;
  GtkWidget *entry = NULL;

  TextSubmitter *submitter = NULL;

  grow_fields (advanced);

  label = gtk_label_new_with_mnemonic (description.c_str ());
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);

  switch (type) {
    case PASSWORD:
      entry = gm_entry_new (NULL);
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
      gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_PASSWORD);
      break;
    case PHONE_NUMBER:
      entry = gm_entry_new (PHONE_NUMBER_REGEX);
      gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_PHONE);
      break;
    case NUMBER:
      entry = gm_entry_new (NUMBER_REGEX);
      gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_NUMBER);
      break;
    case URI:
      entry = gm_entry_new (BASIC_URI_REGEX);
      gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_URL);
      break;
    case EKIGA_URI:
      entry = gm_entry_new (EKIGA_URI_REGEX);
      gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_URL);
      break;
    case STANDARD:
    default:
      entry = gm_entry_new (NULL);
      break;
  };

  gtk_entry_set_placeholder_text (GTK_ENTRY (entry), placeholder_text.c_str ());
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), true);
  gtk_entry_set_text (GTK_ENTRY (entry), value.c_str ());
  g_object_set (G_OBJECT (entry), "expand", TRUE,
                "allow-empty", allow_empty,
                "width-chars", 30, NULL);

  submitter = new TextSubmitter (name, description, placeholder_text, type, advanced, allow_empty, entry);
  submitters.push_back (submitter);

  g_object_set_data (G_OBJECT (entry), "submitter", submitter);
  text_entry_validity_changed_cb (GTK_ENTRY (entry), (gpointer) this);
  g_signal_connect (entry, "validity-changed",
                    G_CALLBACK (text_entry_validity_changed_cb), this);

  if (advanced) {

    gtk_grid_attach (GTK_GRID (advanced_fields), label,
		     0, advanced_rows - 1,
		     1, 1);
    gtk_grid_attach (GTK_GRID (advanced_fields), entry,
		     1, advanced_rows - 1,
		     1, 1);
  } else {

    gtk_grid_attach (GTK_GRID (fields), label,
		     0, rows - 1,
		     1, 1);
    gtk_grid_attach (GTK_GRID (fields), entry,
		     1, rows - 1,
		     1, 1);
  }
}


void
FormDialog::multi_text (const std::string name,
			const std::string description,
			const std::string value,
			bool advanced)
{
  GtkWidget *label = NULL;
  GtkWidget *scroller = NULL;
  GtkWidget *widget = NULL;
  GtkTextBuffer *buffer = NULL;
  MultiTextSubmitter *submitter = NULL;

  grow_fields (advanced);

  label = gtk_label_new_with_mnemonic (description.c_str ());
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
  if (advanced) {

    gtk_grid_attach (GTK_GRID (advanced_fields), label,
		     0, advanced_rows - 1,
		     2, 1);
  } else {

    gtk_grid_attach (GTK_GRID (fields), label,
		     0, rows - 1,
		     2, 1);
  }

  grow_fields (advanced);

  widget = gtk_text_view_new ();
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);
  gtk_text_buffer_set_text (buffer, value.c_str (), -1);
  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroller), widget);

  if (advanced) {

    gtk_grid_attach (GTK_GRID (advanced_fields), scroller,
		     0, advanced_rows - 1,
		     2, 1);
  } else {

    gtk_grid_attach (GTK_GRID (fields), scroller,
		     0, rows - 1,
		     2, 1);
  }

  submitter = new MultiTextSubmitter (name, description, advanced, buffer);
  submitters.push_back (submitter);
}


void
FormDialog::single_choice (const std::string name,
			   const std::string description,
			   const std::string value,
			   const std::map<std::string, std::string> choices,
			   bool advanced)
{
  GtkWidget *label = NULL;
  GtkListStore *model = NULL;
  GtkWidget *widget = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkTreeIter iter;
  SingleChoiceSubmitter *submitter = NULL;

  grow_fields (advanced);

  label = gtk_label_new_with_mnemonic (description.c_str ());
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);

  model = gtk_list_store_new (SingleChoiceSubmitter::COLUMN_NUMBER,
			      G_TYPE_STRING, G_TYPE_STRING);
  widget = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer,
				  "text", SingleChoiceSubmitter::COLUMN_NAME,
                                  NULL);
  g_object_set (G_OBJECT (widget), "expand", TRUE, NULL);

  for (std::map<std::string, std::string>::const_iterator map_iter
	 = choices.begin ();
       map_iter != choices.end ();
       map_iter++) {

    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter,
			SingleChoiceSubmitter::COLUMN_VALUE, map_iter->first.c_str (),
			SingleChoiceSubmitter::COLUMN_NAME, map_iter->second.c_str (),
			-1);
    if (map_iter->first == value)
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
  }

  if (advanced) {

    gtk_grid_attach (GTK_GRID (advanced_fields), label,
		     0, advanced_rows - 1,
		     1, 1);
    gtk_grid_attach (GTK_GRID (advanced_fields), widget,
		     1, advanced_rows - 1,
		     1, 1);
  } else {

    gtk_grid_attach (GTK_GRID (fields), label,
		     0, rows - 1,
		     1, 1);
    gtk_grid_attach (GTK_GRID (fields), widget,
		     1, rows - 1,
		     1, 1);
  }

  submitter = new SingleChoiceSubmitter (name, description, choices,
					 advanced, widget);
  submitters.push_back (submitter);
}


void
FormDialog::multiple_choice (const std::string name,
			     const std::string description,
			     const std::set<std::string> values,
			     const std::map<std::string, std::string> choices,
			     bool advanced)
{
  GtkWidget *label = NULL;
  GtkWidget *scroll = NULL;
  GtkWidget *tree_view = NULL;
  GtkWidget *frame = NULL;

  GtkListStore *list_store = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkTreeIter iter;

  MultipleChoiceSubmitter *submitter = NULL;

  grow_fields (advanced);

  /* The label */
  label = gtk_label_new_with_mnemonic (description.c_str ());
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);

  /* The GtkListStore containing the choices */
  tree_view = gtk_tree_view_new ();
  list_store = gtk_list_store_new (MultipleChoiceSubmitter::COLUMN_NUMBER,
                                   G_TYPE_BOOLEAN, G_TYPE_STRING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  frame = gtk_frame_new (NULL);
  g_object_set (G_OBJECT (frame), "expand", TRUE, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (frame), -1, 125);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scroll);
  gtk_container_add (GTK_CONTAINER (scroll), tree_view);

  renderer = gtk_cell_renderer_toggle_new ();
  column =
    gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                              "active", MultipleChoiceSubmitter::COLUMN_ACTIVE,
                                              NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  g_signal_connect (renderer, "toggled",
                    G_CALLBACK (multiple_choice_choice_toggled_cb), list_store);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                              "text", MultipleChoiceSubmitter::COLUMN_NAME,
                                              NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);


  for (std::map<std::string, std::string>::const_iterator map_iter
	 = choices.begin ();
       map_iter != choices.end ();
       map_iter++) {

    bool active = (std::find (values.begin (), values.end (), map_iter->first) != values.end ());

    gtk_list_store_append (GTK_LIST_STORE (list_store), &iter);
    gtk_list_store_set (GTK_LIST_STORE (list_store), &iter,
                        MultipleChoiceSubmitter::COLUMN_ACTIVE, active,
                        MultipleChoiceSubmitter::COLUMN_NAME, map_iter->second.c_str (),
                        -1);
  }

  if (advanced) {

    gtk_grid_attach (GTK_GRID (advanced_fields), label,
		     0, advanced_rows - 1,
		     1, 1);
    grow_fields (advanced);
    gtk_grid_attach (GTK_GRID (advanced_fields), frame,
		     0, advanced_rows - 1,
		     2, 1);
  } else {

    gtk_grid_attach (GTK_GRID (fields), label,
		     0, rows - 1,
		     1, 1);
    grow_fields (advanced);
    gtk_grid_attach (GTK_GRID (fields), frame,
		     0, rows - 1,
		     2, 1);
  }

  submitter = new MultipleChoiceSubmitter (name, description,
					   choices, advanced, tree_view);
  submitters.push_back (submitter);
}


void
FormDialog::editable_list (const std::string name,
                           const std::string description,
                           const std::list<std::string> values,
                           const std::list<std::string> proposed_values,
                           bool advanced,
                           bool rename_only)
{
  GtkWidget *label = NULL;
  GtkWidget *scroll = NULL;
  GtkWidget *button = NULL;
  GtkWidget *tree_view = NULL;
  GtkWidget *frame = NULL;
  GtkWidget *hbox = NULL;
  GtkWidget *entry = NULL;

  GtkListStore *list_store = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkTreeIter iter;

  EditableListSubmitter *submitter = NULL;

  /* The label */
  if (!description.empty ()) {
    label = gtk_label_new_with_mnemonic (description.c_str ());
    gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  }

  /* The GtkListStore containing the values */
  list_store = gtk_list_store_new (EditableListSubmitter::COLUMN_NUMBER,
				   G_TYPE_BOOLEAN, G_TYPE_STRING);
  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  frame = gtk_frame_new (NULL);
  g_object_set (G_OBJECT (frame), "expand", TRUE, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (frame), -1, 125);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scroll);
  gtk_container_add (GTK_CONTAINER (scroll), tree_view);

  if (!rename_only) {
    renderer = gtk_cell_renderer_toggle_new ();
    column =
      gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                                "active", EditableListSubmitter::COLUMN_ACTIVE,
                                                NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
    g_signal_connect (renderer, "toggled",
                      G_CALLBACK (editable_list_choice_toggled_cb), list_store);
  }
  renderer = gtk_cell_renderer_text_new ();
  if (rename_only)
    g_object_set (renderer, "editable", TRUE, NULL);
  column =
    gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                              "text", EditableListSubmitter::COLUMN_VALUE,
                                              NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  g_signal_connect (renderer, "edited", (GCallback) editable_list_edit_value_cb, tree_view);

  for (std::list<std::string>::const_iterator list_iter = values.begin ();
       list_iter != values.end ();
       list_iter++) {

    gtk_list_store_append (GTK_LIST_STORE (list_store), &iter);
    gtk_list_store_set (GTK_LIST_STORE (list_store), &iter,
			EditableListSubmitter::COLUMN_ACTIVE, TRUE,
                        EditableListSubmitter::COLUMN_VALUE, list_iter->c_str (),
                        -1);
  }
  for (std::list<std::string>::const_iterator list_iter
	 = proposed_values.begin ();
       list_iter != proposed_values.end ();
       list_iter++) {

    if (std::find(values.begin(), values.end(), *list_iter) == values.end ()) {

      gtk_list_store_append (GTK_LIST_STORE (list_store), &iter);
      gtk_list_store_set (GTK_LIST_STORE (list_store), &iter,
			  EditableListSubmitter::COLUMN_ACTIVE, FALSE,
			  EditableListSubmitter::COLUMN_VALUE, list_iter->c_str (),
			  -1);
    }
  }

  if (advanced) {

    if (label) {
      grow_fields (advanced);
      gtk_grid_attach (GTK_GRID (advanced_fields), label,
                       0, advanced_rows - 1,
                       1, 1);
    }
    grow_fields (advanced);
    gtk_grid_attach (GTK_GRID (advanced_fields), frame,
		     0, advanced_rows - 1,
		     2, 1);
  } else {

    if (label) {
      grow_fields (advanced);
      gtk_grid_attach (GTK_GRID (fields), label,
                       0, rows - 1,
                       1, 1);
    }
    grow_fields (advanced);
    gtk_grid_attach (GTK_GRID (fields), frame,
		     0, rows - 1,
		     2, 1);
  }

  if (!rename_only) {

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    entry = gtk_entry_new ();
    button = gtk_button_new_with_label (_("Add"));
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    g_signal_connect (entry, "activate",
                      (GCallback) editable_list_add_value_activated_cb,
                      (gpointer) tree_view);

    g_signal_connect (button, "clicked",
                      (GCallback) editable_list_add_value_clicked_cb,
                      (gpointer) entry);

    grow_fields (advanced);
    if (advanced) {

      gtk_grid_attach (GTK_GRID (advanced_fields), hbox,
                       0, advanced_rows - 1,
                       2, 1);
    } else {

      gtk_grid_attach (GTK_GRID (fields), hbox,
                       0, rows - 1,
                       2, 1);
    }
  }

  submitter = new EditableListSubmitter (name, description, advanced, tree_view);
  submitters.push_back (submitter);
}


bool
FormDialog::can_submit ()
{
  for (std::list<Submitter *>::iterator iter = submitters.begin ();
       iter != submitters.end ();
       iter++)
    if (!(*iter)->can_submit ())
      return false;

  return true;
}


bool
FormDialog::submit ()
{
  bool ok = false;
  std::string error_msg;
  Ekiga::FormBuilder builder;

  for (std::list<Submitter *>::iterator iter = submitters.begin ();
       iter != submitters.end ();
       iter++)
    (*iter)->submit (builder);

  ok = request->submit (builder, error_msg);
  if (!ok)
    error (error_msg);
  return ok;
}


GtkWidget *
FormDialog::get_dialog ()
{
  return window;
}

void
FormDialog::cancel ()
{
  request->cancel ();
}


void
FormDialog::grow_fields (bool advanced)
{
  if (advanced)
    advanced_rows++;
  else
    rows++;
}
