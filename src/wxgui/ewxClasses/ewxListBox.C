#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxListBox.H"
#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxHelpHandler.H"

// constructors
ewxListBox::ewxListBox()
  : wxListBox(),
    ewxStyledWindow(),
    p_editable(true),  
    p_disabler(NULL)  
{

}


// For backward compatibility
ewxListBox::ewxListBox(wxWindow* parent, wxWindowID id,
                       const wxPoint& pos, const wxSize& size, int n,
                       const wxString choices[], long style,
                       const wxValidator& validator, const wxString& name)
  : wxListBox(),
    ewxStyledWindow(),
    p_editable(true),
    p_disabler(NULL)
{
   Create(parent, id, pos, size, n, choices, style|wxWANTS_CHARS,
          validator, name);
}


bool ewxListBox::Create(wxWindow* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size, int n,
                        const wxString choices[], long style,
                        const wxValidator& validator, const wxString& name)
{
   if (!wxListBox::Create(parent, id, pos, size, n, choices, style, 
                          validator, name)) {
      wxFAIL_MSG( wxT("ewxListBox creation failed") );
      return false;
   }

   PushEventHandler(new ewxHelpHandler(this));
   p_disabler = new ewxDisabler();
   PushEventHandler(p_disabler);
   p_editable = true;
   setStyles(this);

   return true;
}


ewxListBox::ewxListBox(wxWindow* parent, wxWindowID id,
                       const wxPoint& pos, const wxSize& size,
                       const wxArrayString & choices, long style,
                       const wxValidator& validator, const wxString& name)
  : wxListBox(),
    ewxStyledWindow()
{
   Create(parent, id, pos, size, choices, style|wxWANTS_CHARS,
          validator, name);
}


bool ewxListBox::Create(wxWindow* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString & choices, long style,
                        const wxValidator& validator, const wxString& name)
{
   if (!wxListBox::Create(parent, id, pos, size, choices, style,
                          validator, name)) {
      wxFAIL_MSG( wxT("ewxListBox creation failed") );
      return false;
   }

   PushEventHandler(new ewxHelpHandler(this));
   p_disabler = new ewxDisabler();
   PushEventHandler(p_disabler);
   p_editable = true;
   setStyles(this);

   return true;
}


ewxListBox::~ewxListBox()
{
  PopEventHandler(true);
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxListBox::setCustomDisabledStyle(bool enabled)
{
   // The following line when commented out allows the user to select from
   // the listbox even though it is read-only mode.
   //if (p_disabler) p_disabler->setEnabled(enabled);
   SetEditable(enabled);
}


/**
 * Set widget to editable or not.  This is NOT
 * a function provided by wxwidgets but we use their method name
 * style since it should be and is less confusing.
 */
void ewxListBox::SetEditable(bool editable)
{
   p_editable = editable;
   setStyles(this,false);
}


bool ewxListBox::IsEditable()
{
   return p_editable;
}

