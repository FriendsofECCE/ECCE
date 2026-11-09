#include <vector>

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "FolderChooserDialog.H"
#include "dsm/ResourceType.H"
#include "dsm/Resource.H"
#include "wxgui/WxResourceTreeCtrl.H"
#include "wxgui/WxResourceTreeItemData.H"

namespace {

class FolderItem : public wxTreeItemData
{
public:
  explicit FolderItem(const EcceURL& u) : url(u), filled(false) {}
  EcceURL url;
  bool filled;
};

//  A server root that is not connected has no resource, so is not a folder.
bool isFolder(WxResourceTreeItemData *data)
{
  if (!data || !data->getResource()) return false;
  ResourceType *type = data->getResource()->getDescriptor();
  return type && type->getIsExpandable();
}

}


FolderChooserDialog::FolderChooserDialog(wxWindow *parent,
    const wxString& title, const wxString& prompt, WxResourceTreeCtrl *source,
    const EcceURL& current, RefusalFn refusal)
  : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_source(source), p_refusal(refusal)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxStaticText *label = new wxStaticText(this, wxID_ANY, prompt);
  label->Wrap(FromDIP(380));
  top->Add(label, 0, wxALL | wxEXPAND, FromDIP(8));

  p_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition,
      FromDIP(wxSize(380, 300)),
      wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES);
  top->Add(p_tree, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(8));

  p_note = new wxStaticText(this, wxID_ANY, wxEmptyString);
  top->Add(p_note, 0, wxALL | wxEXPAND, FromDIP(8));

  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
  p_ok = new wxButton(this, wxID_OK);
  buttons->AddButton(p_ok);
  buttons->AddButton(new wxButton(this, wxID_CANCEL));
  buttons->Realize();
  top->Add(buttons, 0, wxALL | wxALIGN_RIGHT, FromDIP(8));
  SetSizerAndFit(top);
  SetMinSize(FromDIP(wxSize(320, 360)));

  p_tree->AddRoot(wxEmptyString, -1, -1, new FolderItem(EcceURL()));
  fill(p_tree->GetRootItem());
  p_tree->Bind(wxEVT_TREE_ITEM_EXPANDING, &FolderChooserDialog::onExpanding, this);
  p_tree->Bind(wxEVT_TREE_SEL_CHANGED, &FolderChooserDialog::onSelChanged, this);
  p_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &FolderChooserDialog::onActivated, this);
  expandTo(current);
  update();
}


EcceURL FolderChooserDialog::selectedUrl() const
{
  wxTreeItemId id = p_tree->GetSelection();
  FolderItem *item = id.IsOk()
      ? dynamic_cast<FolderItem*>(p_tree->GetItemData(id)) : 0;
  return item ? item->url : EcceURL();
}


//  Adds the folder children of `item`, taken from the Organizer's own tree
//  so both show the same servers and names.
void FolderChooserDialog::fill(const wxTreeItemId& item)
{
  FolderItem *parent = dynamic_cast<FolderItem*>(p_tree->GetItemData(item));
  if (!parent || parent->filled) return;
  parent->filled = true;

  wxTreeItemId from;
  if (item == p_tree->GetRootItem()) {
    from = p_source->GetRootItem();
  } else {
    WxResourceTreeItemData *node = p_source->findNode(parent->url, false, true);
    if (!node) return;
    if (p_source->GetChildrenCount(node->GetId(), false) == 0)
      p_source->loadChildren(node);
    from = node->GetId();
  }
  wxTreeItemIdValue cookie;
  for (wxTreeItemId c = p_source->GetFirstChild(from, cookie); c.IsOk();
       c = p_source->GetNextChild(from, cookie)) {
    WxResourceTreeItemData *data =
        dynamic_cast<WxResourceTreeItemData*>(p_source->GetItemData(c));
    if (!isFolder(data)) continue;
    wxTreeItemId made = p_tree->AppendItem(item, p_source->GetItemText(c), -1, -1,
                                           new FolderItem(data->getUrl()));
    p_tree->SetItemHasChildren(made, true);
  }
}


void FolderChooserDialog::expandTo(const EcceURL& url)
{
  std::vector<std::string> path;
  EcceURL up = url;
  for (int i = 0; i < 32 && !up.toString().empty(); i++) {
    path.insert(path.begin(), up.toString());
    EcceURL next = up.getParent();
    if (next.toString() == up.toString()) break;
    up = next;
  }
  wxTreeItemId at = p_tree->GetRootItem();
  for (const std::string& step : path) {
    fill(at);
    wxTreeItemIdValue cookie;
    wxTreeItemId found;
    for (wxTreeItemId c = p_tree->GetFirstChild(at, cookie); c.IsOk();
         c = p_tree->GetNextChild(at, cookie)) {
      FolderItem *item = dynamic_cast<FolderItem*>(p_tree->GetItemData(c));
      if (item && item->url.toString() == step) { found = c; break; }
    }
    if (!found.IsOk()) continue;
    at = found;
    if (step != url.toString()) p_tree->Expand(at);
  }
  if (at.IsOk() && at != p_tree->GetRootItem()) {
    p_tree->SelectItem(at);
    p_tree->EnsureVisible(at);
  }
}


void FolderChooserDialog::onExpanding(wxTreeEvent& e)
{
  fill(e.GetItem());
}


void FolderChooserDialog::onSelChanged(wxTreeEvent&)
{
  update();
}


void FolderChooserDialog::onActivated(wxTreeEvent& e)
{
  if (p_ok->IsEnabled()) EndModal(wxID_OK);
  else e.Skip();
}


void FolderChooserDialog::update()
{
  const EcceURL url = selectedUrl();
  std::string why = url.toString().empty() ? "Choose a folder." : p_refusal(url);
  p_ok->Enable(why.empty());
  p_note->SetLabel(wxString::FromUTF8(why.c_str()));
  p_note->Wrap(FromDIP(380));
  Layout();
}
