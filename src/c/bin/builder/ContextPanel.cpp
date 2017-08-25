#include <wx/sizer.h>

#include "wxgui/ewxPanel.H"
#include "wxgui/StringListCtrl.H"

#include "ContextPanel.H"


IMPLEMENT_DYNAMIC_CLASS(ContextPanel, ewxPanel)


const wxWindowID ContextPanel::ID_LIST = wxNewId();


ContextPanel::ContextPanel()
  : ewxPanel()
  , p_list(NULL)
{
}


ContextPanel::ContextPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : ewxPanel()
  , p_list(NULL)
{
  Create(parent, id, pos, size, style, name);
}


ContextPanel::~ContextPanel()
{
}


bool ContextPanel::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!ewxPanel::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ContextPanel creation failed") );
    return false;
  }

  SetMinSize(wxSize(400,300));

  p_list = new StringListCtrl(this, ID_LIST);

  wxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(p_list, 1, wxGROW);

  SetSizerAndFit(sizer);

  return true;
}


wxString ContextPanel::GetContext() const
{
  return p_list->GetItemText(p_list->GetFirstSelected());
}


bool ContextPanel::SetContext(const wxString& context)
{
  if (context.IsEmpty() || context.IsSameAs("")) {
    return false; // bail
  }

  // Check we don't already have this file
  long index = p_list->Find(context);
  if (index == wxNOT_FOUND) {
    //index = p_list->Append(context);
    p_list->Append(context);
  }
  //p_list->Select(index);
  p_list->SetSelection(context);

  return true;
}


bool ContextPanel::RemoveContext(const wxString& context)
{
  long index = p_list->Find(context);
  if (index != wxNOT_FOUND) {
    p_list->Delete(index);
    return true;
  }

  return false;
}


bool ContextPanel::RenameContext(const wxString& oldContext,
                                 const wxString& newContext)
{
  long index = p_list->Find(oldContext);
  if (index != wxNOT_FOUND) {
    p_list->Change(index, newContext);
    return true;
  }

  return false;
}



