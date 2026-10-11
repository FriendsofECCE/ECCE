#include <wx/sizer.h>
#include <wx/uri.h>

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
  return ContextAt(p_list->GetFirstSelected());
}


wxString ContextPanel::ContextAt(long index) const
{
  if (index < 0 || index >= (long)p_contexts.size()) return "";
  return p_contexts[index];
}


bool ContextPanel::SetContext(const wxString& context)
{
  if (context.IsEmpty()) {
    return false; // bail
  }

  long index = indexOf(context);
  if (index == wxNOT_FOUND) {
    p_contexts.push_back(context);
    index = p_list->Append(label(context));
    relabel();
  }
  p_list->SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  p_list->EnsureVisible(index);

  return true;
}


bool ContextPanel::RemoveContext(const wxString& context)
{
  long index = indexOf(context);
  if (index != wxNOT_FOUND) {
    p_list->Delete(index);
    p_contexts.erase(p_contexts.begin() + index);
    relabel();
    return true;
  }

  return false;
}


bool ContextPanel::RenameContext(const wxString& oldContext,
                                 const wxString& newContext)
{
  long index = indexOf(oldContext);
  if (index != wxNOT_FOUND) {
    p_contexts[index] = newContext;
    relabel();
    return true;
  }

  return false;
}


long ContextPanel::indexOf(const wxString& context) const
{
  for (size_t i = 0; i < p_contexts.size(); i++)
    if (p_contexts[i] == context) return (long)i;
  return wxNOT_FOUND;
}


//  A calculation is listed by its name, the last part of its URL; a name
//  that two open calculations share gets its folder added.
wxString ContextPanel::label(const wxString& context)
{
  if (context.Find("://") == wxNOT_FOUND) return context;
  wxString path = context.AfterFirst(':').Mid(2);
  while (path.EndsWith("/")) path.RemoveLast();
  return wxURI::Unescape(path.AfterLast('/'));
}


void ContextPanel::relabel()
{
  for (size_t i = 0; i < p_contexts.size(); i++) {
    wxString name = label(p_contexts[i]);
    bool shared = false;
    for (size_t j = 0; j < p_contexts.size() && !shared; j++)
      shared = j != i && label(p_contexts[j]) == name;
    if (shared) {
      wxString path = p_contexts[i].AfterFirst(':').Mid(2);
      while (path.EndsWith("/")) path.RemoveLast();
      wxString folder = wxURI::Unescape(path.BeforeLast('/').AfterLast('/'));
      if (!folder.IsEmpty()) name += " (" + folder + ")";
    }
    if (p_list->GetItemText(i) != name) p_list->Change(i, name);
  }
}
