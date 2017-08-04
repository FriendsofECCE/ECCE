#include "wxgui/StringListCtrl.H"

IMPLEMENT_DYNAMIC_CLASS(StringListCtrl, ewxListView)


StringListCtrl::StringListCtrl()
  : ewxListView()
{
}


StringListCtrl::StringListCtrl(wxWindow * parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxValidator& validator, const wxString& name)
  : ewxListView()
{
  Create(parent, id, pos, size, style, validator, name);
}


StringListCtrl::~StringListCtrl()
{
}


bool StringListCtrl::Create(wxWindow * parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxValidator& validator, const wxString& name)
{
  if (!ewxListView::Create(parent, id, pos, size, style, validator, name)) {
    wxFAIL_MSG( wxT("StringListCtrl creation failed") );
    return false;
  }

  InsertColumn(0, "String");
  SetColumnWidth(0, wxLIST_AUTOSIZE);

  return true;
}


long StringListCtrl::Append(const wxString& label)
{
  long ret = InsertItem(GetItemCount(), label);
  SetColumnWidth(0, wxLIST_AUTOSIZE);
  EnsureVisible(ret);
  return ret;
}


long StringListCtrl::Insert(long index, const wxString& label)
{
  long ret = InsertItem(index, label);
  SetColumnWidth(0, wxLIST_AUTOSIZE);
  EnsureVisible(ret);
  return ret;
}


bool StringListCtrl::Delete(long index)
{
  bool ret = DeleteItem(index);
  SetColumnWidth(0, wxLIST_AUTOSIZE);
  return ret;
}


long StringListCtrl::Find(const wxString& label)
{
  return FindItem(-1, label);
}


wxString StringListCtrl::GetSelection() const
{
  long item = GetFirstSelected();
  if (item != -1) {
    return GetItemText(item);
  }
  return "";
}


void StringListCtrl::SetSelection(const wxString& label)
{
  long index = Find(label);
  if (index != wxNOT_FOUND) {
    SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    EnsureVisible(index);
  }
}


void StringListCtrl::Change(long index, const wxString& label)
{
  SetItemText(index, label);
  SetColumnWidth(0, wxLIST_AUTOSIZE);
}

