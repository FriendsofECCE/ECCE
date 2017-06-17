#include "wxgui/ewxChoice.H"
#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxHelpHandler.H"


ewxChoice::ewxChoice()
  : wxChoice(),
    ewxStyledWindow(),
    p_editable(true),
    p_disabler(NULL)
{
}


ewxChoice::ewxChoice(wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, int n, const wxString choices[], long style,
        const wxValidator& validator, const wxString& name)
  : wxChoice(),
    ewxStyledWindow(),
    p_editable(true),
    p_disabler(NULL)
{
  Create(parent, id, pos, size, n, choices, style, validator, name);
}


ewxChoice::ewxChoice(wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, const wxArrayString& choices, long style,
        const wxValidator& validator, const wxString& name)
  : wxChoice(),
    ewxStyledWindow(),
    p_editable(true),
    p_disabler(NULL)
{
  Create(parent, id, pos, size, choices, style, validator, name);
}


ewxChoice::~ewxChoice()
{
  PopEventHandler(true);
  PopEventHandler(true);
}


bool ewxChoice::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, int n, const wxString choices[], long style,
        const wxValidator& validator, const wxString& name)
{
  if (!wxChoice::Create(parent, id, pos, size, n, choices, style, validator,
                        name)) {
    wxFAIL_MSG( wxT("ewxChoice creation failed") );
    return false;
  }

  return Create();
}


bool ewxChoice::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, const wxArrayString& choices, long style,
        const wxValidator& validator, const wxString& name)
{
  if (!wxChoice::Create(parent, id, pos, size, choices, style, validator,
                        name)) {
    wxFAIL_MSG( wxT("ewxChoice creation failed") );
    return false;
  }

  return Create();
}


bool ewxChoice::Create()
{
  PushEventHandler(new ewxHelpHandler(this));
  p_disabler = new ewxDisabler();
  PushEventHandler(p_disabler);
  p_editable = true;
  setStyles(this, false);

  /*
  // Hack fix for selection highlight missing bug.
  // Do not add choices in constructor and add them after setStyles.
  for (int i=0; i<n; i++) {
    Append(choices[i]);
  }
  */

  return true;
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxChoice::setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
   SetEditable(enabled);
}


/**
 * Set widget to editable or not.  This is NOT
 * a function provided by wxwidgets but we use their method name
 * style since it should be and is less confusing.
 */
void ewxChoice::SetEditable(bool editable)
{
   p_editable = editable;
   setStyles(this,false);
}


bool ewxChoice::IsEditable()
{
   return p_editable;
}

