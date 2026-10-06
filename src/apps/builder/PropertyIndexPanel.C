#include <map>
#include <set>

#include <wx/sizer.h>

#include "PropertyIndexPanel.H"

namespace {

// Tree items carry the panel name; group headings carry nothing.
class NameData : public wxTreeItemData
{
  public:
    explicit NameData(const std::string& n) : name(n) {}
    std::string name;
};

}


PropertyIndexPanel::PropertyIndexPanel(wxWindow *parent)
  : wxPanel(parent, wxID_ANY),
    p_title(0), p_codeTheory(0), p_energy(0), p_tree(0), p_quiet(false)
{
  p_title = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition,
                             wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  wxFont bold = p_title->GetFont();
  bold.SetWeight(wxFONTWEIGHT_BOLD);
  p_title->SetFont(bold);
  p_codeTheory = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition,
                                  wxDefaultSize, wxST_ELLIPSIZE_END);
  p_energy = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition,
                              wxDefaultSize, wxST_ELLIPSIZE_END);
  p_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                          wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES |
                          wxTR_HAS_BUTTONS | wxBORDER_THEME);

  wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
  col->Add(p_title, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
  col->Add(p_codeTheory, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
  col->Add(p_energy, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  col->Add(p_tree, 1, wxEXPAND);
  SetSizer(col);
  SetMinSize(wxSize(200, 150));

  p_tree->Bind(wxEVT_TREE_SEL_CHANGED, &PropertyIndexPanel::OnSelChanged,
               this);
}


void PropertyIndexPanel::setHeader(const wxString& title,
                                   const wxString& codeTheory,
                                   const wxString& energy)
{
  p_title->SetLabel(title);
  p_codeTheory->SetLabel(codeTheory);
  p_energy->SetLabel(energy);
  p_codeTheory->Show(!codeTheory.IsEmpty());
  p_energy->Show(!energy.IsEmpty());
  Layout();
}


/**
 * One row per panel; a group with a single panel is just that panel, so
 * the list is not mostly headings.  The groups keep the order the
 * descriptor file first names them.
 */
void PropertyIndexPanel::setEntries(const std::vector<Entry>& entries)
{
  if (entries.size() == p_entries.size()) {
    bool same = true;
    for (size_t i = 0; i < entries.size() && same; ++i) {
      same = entries[i].name == p_entries[i].name &&
             entries[i].group == p_entries[i].group;
    }
    if (same) {
      return;
    }
  }
  p_entries = entries;

  std::string selected;
  wxTreeItemId sel = p_tree->GetSelection();
  if (sel.IsOk()) {
    NameData *d = dynamic_cast<NameData*>(p_tree->GetItemData(sel));
    if (d) selected = d->name;
  }

  p_quiet = true;
  p_tree->Freeze();
  p_tree->DeleteAllItems();
  wxTreeItemId root = p_tree->AddRoot("");

  std::vector<std::string> groupOrder;
  std::map<std::string, std::vector<const Entry*> > byGroup;
  for (size_t i = 0; i < p_entries.size(); ++i) {
    if (byGroup.find(p_entries[i].group) == byGroup.end()) {
      groupOrder.push_back(p_entries[i].group);
    }
    byGroup[p_entries[i].group].push_back(&p_entries[i]);
  }
  for (size_t g = 0; g < groupOrder.size(); ++g) {
    const std::vector<const Entry*>& list = byGroup[groupOrder[g]];
    if (list.size() == 1) {
      p_tree->AppendItem(root, list[0]->name, -1, -1,
                         new NameData(list[0]->name));
      continue;
    }
    wxTreeItemId head = p_tree->AppendItem(root, groupOrder[g]);
    p_tree->SetItemBold(head, true);
    for (size_t i = 0; i < list.size(); ++i) {
      p_tree->AppendItem(head, list[i]->name, -1, -1,
                         new NameData(list[i]->name));
    }
    p_tree->Expand(head);
  }
  p_tree->Thaw();
  p_quiet = false;
  if (!selected.empty()) {
    setSelected(selected);
  }
}


void PropertyIndexPanel::setSelected(const std::string& name)
{
  p_quiet = true;
  wxTreeItemId root = p_tree->GetRootItem();
  bool found = false;
  wxTreeItemIdValue c1;
  for (wxTreeItemId it = p_tree->GetFirstChild(root, c1);
       it.IsOk() && !found; it = p_tree->GetNextChild(root, c1)) {
    NameData *d = dynamic_cast<NameData*>(p_tree->GetItemData(it));
    if (d && d->name == name) {
      p_tree->SelectItem(it);
      p_tree->EnsureVisible(it);
      found = true;
      break;
    }
    wxTreeItemIdValue c2;
    for (wxTreeItemId ch = p_tree->GetFirstChild(it, c2); ch.IsOk();
         ch = p_tree->GetNextChild(it, c2)) {
      NameData *cd = dynamic_cast<NameData*>(p_tree->GetItemData(ch));
      if (cd && cd->name == name) {
        p_tree->SelectItem(ch);
        p_tree->EnsureVisible(ch);
        found = true;
        break;
      }
    }
  }
  if (!found) {
    p_tree->UnselectAll();
  }
  p_quiet = false;
}


void PropertyIndexPanel::OnSelChanged(wxTreeEvent& event)
{
  if (p_quiet || !p_onSelect) {
    return;
  }
  wxTreeItemId item = event.GetItem();
  if (!item.IsOk()) {
    return;
  }
  NameData *d = dynamic_cast<NameData*>(p_tree->GetItemData(item));
  if (d) {
    p_onSelect(d->name);
  } else {
    // A group heading: open or close it.
    p_tree->Toggle(item);
  }
}


std::vector<std::string> PropertyIndexPanel::listing() const
{
  std::vector<std::string> out;
  for (size_t i = 0; i < p_entries.size(); ++i) {
    out.push_back(p_entries[i].group + "/" + p_entries[i].name);
  }
  return out;
}
