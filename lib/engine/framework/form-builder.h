/*
 * Ekiga -- A VoIP application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>

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
 *                         form-builder.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : declaration of an object able to build a form
 *
 */

#ifndef __FORM_BUILDER_H__
#define __FORM_BUILDER_H__

#include <list>

#include "form.h"

namespace Ekiga
{

/**
 * @addtogroup forms
 * @{
 */

  class FormBuilder: public virtual Form, public virtual FormVisitor
  {
  public:

    FormBuilder ();

    /* form part */

    void visit (FormVisitor &visitor) const;

    const std::string hidden (const std::string name) const;

    bool boolean (const std::string name) const;

    const std::string text (const std::string name) const;

    const std::string multi_text (const std::string name) const;

    const std::string single_choice (const std::string name) const;

    const std::set<std::string> multiple_choice (const std::string name) const;

    const std::list<std::string> editable_list (const std::string name) const;

    /* builder part */
    void title (const std::string title);

    void action (const std::string action);

    void instructions (const std::string instructions);

    void link (const std::string link,
               const std::string uri);

    void error (const std::string error);

    void hidden (const std::string name,
		 const std::string value);

    void boolean (const std::string name,
		  const std::string description,
		  bool value,
		  bool advanced = false,
                  bool in_header_bar = true);

    void text (const std::string text,
	       const std::string description,
	       const std::string value,
	       const std::string tooltip,
               const FormTextType type = STANDARD,
	       bool advanced = false,
               bool allow_empty = true);

    void multi_text (const std::string text,
		     const std::string description,
		     const std::string value,
		     bool advanced = false);

    void single_choice (const std::string name,
			const std::string description,
			const std::string value,
			const std::map<std::string, std::string> choices,
			bool advanced = false);

    void multiple_choice (const std::string name,
			  const std::string description,
			  const std::set<std::string> values,
			  const std::map<std::string, std::string> choices,
			  bool advanced = false);

    void editable_list (const std::string name,
                        const std::string description,
                        const std::list<std::string> values,
                        const std::list<std::string> proposed_values,
                        bool advanced = false,
                        bool rename_only = false);

  private:

    struct HiddenField
    {
      HiddenField (const std::string _name,
		   const std::string _value): name(_name), value(_value)
      {}

      const std::string name;
      const std::string value;
      bool advanced;
    };

    struct BooleanField
    {
      BooleanField (const std::string _name,
		    const std::string _description,
		    bool _value,
		    bool _advanced,
                    bool _in_header_bar): name(_name), description(_description),
                                          value(_value), advanced(_advanced),
                                          in_header_bar(_in_header_bar)
      {}

      const std::string name;
      const std::string description;
      bool value;
      bool advanced;
      bool in_header_bar;
    };

    struct TextField
    {
      TextField (const std::string _name,
		 const std::string _description,
		 const std::string _value,
		 const std::string _tooltip,
                 const FormTextType _type,
		 bool _advanced,
                 bool _allow_empty): name(_name),
                                     description(_description),
                                     value(_value),
                                     tooltip (_tooltip),
                                     type(_type),
                                     advanced(_advanced),
                                     allow_empty(_allow_empty)
      {}

      const std::string name;
      const std::string description;
      const std::string value;
      const std::string tooltip;
      const FormTextType type;
      bool advanced;
      bool allow_empty;
    };

    struct MultiTextField
    {
      MultiTextField (const std::string _name,
		      const std::string _description,
		      const std::string _value,
		      bool _advanced): name(_name),
				       description(_description),
				       value(_value), advanced(_advanced)
      {}

      const std::string name;
      const std::string description;
      const std::string value;
      bool advanced;
    };

    struct SingleChoiceField
    {
      SingleChoiceField (const std::string _name,
			 const std::string _description,
			 const std::string _value,
			 const std::map<std::string, std::string> _choices,
			 bool _advanced):
	name(_name), description(_description),
	value(_value), choices(_choices), advanced(_advanced)
      {}

      const std::string name;
      const std::string description;
      const std::string value;
      const std::map<std::string, std::string> choices;
      bool advanced;
    };


    struct MultipleChoiceField
    {
      MultipleChoiceField (const std::string _name,
			   const std::string _description,
			   const std::set<std::string> _values,
			   const std::map<std::string, std::string> _choices,
			   bool _advanced):
	name(_name), description(_description),
	values(_values), choices(_choices), advanced(_advanced)
      {}

      const std::string name;
      const std::string description;
      const std::set<std::string> values;
      const std::map<std::string, std::string> choices;
      bool advanced;
    };

    struct EditableListField
    {
      EditableListField (const std::string _name,
                         const std::string _description,
                         const std::list<std::string> _values,
                         const std::list<std::string> _proposed_values,
                         bool _advanced,
                         bool _rename_only):
	name(_name),
        description(_description),
	values(_values),
        proposed_values(_proposed_values),
        advanced(_advanced),
        rename_only(_rename_only)
      {}

      const std::string name;
      const std::string description;
      const std::list<std::string> values;
      const std::list<std::string> proposed_values;
      bool advanced;
      bool rename_only;
    };

    typedef enum {

      HIDDEN,
      BOOLEAN,
      TEXT,
      MULTI_TEXT,
      SINGLE_CHOICE,
      MULTIPLE_CHOICE,
      EDITABLE_SET
    } FieldType;

    std::string my_title;
    std::string my_action;
    std::string my_instructions;
    std::pair<std::string, std::string> my_link;
    std::string my_error;
    std::list<FieldType> ordering;
    std::list<struct HiddenField> hiddens;
    std::list<struct BooleanField> booleans;
    std::list<struct TextField> texts;
    std::list<struct MultiTextField> multi_texts;
    std::list<struct SingleChoiceField> single_choices;
    std::list<struct MultipleChoiceField> multiple_choices;
    std::list<struct EditableListField> editable_lists;
  };

/**
 * @}
 */

};
#endif
