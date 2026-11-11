#include <algorithm>
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
  p_tree->Bind(wxEVT_TREE_ITEM_GETTOOLTIP, [this](wxTreeEvent& event) {
    auto *data = dynamic_cast<NameData*>(p_tree->GetItemData(event.GetItem()));
    if (data) for (const auto& entry : p_entries)
      if (entry.name == data->name)
        event.SetToolTip(wxString::FromUTF8(entry.tooltip.c_str()));
  });
}


void PropertyIndexPanel::setHeader(const wxString& title,
                                   const wxString& codeTheory,
                                   const wxString& energy)
{
  p_title->SetLabel(title);
  p_codeTheory->SetLabel(codeTheory);
  p_energy->SetLabel(energy);
  p_title->Show(!title.IsEmpty());
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
    //  A category keeps its heading even with one entry, so the same
    //  property sits in the same place for every calculation.
    if (groupOrder[g].empty()) {
      for (size_t i = 0; i < list.size(); ++i)
        p_tree->AppendItem(root, list[i]->name, -1, -1,
                           new NameData(list[i]->name));
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


int PropertyIndexPanel::preferredHeight(int maxRows) const
{
  int rows = 0;
  wxTreeItemId root = p_tree->GetRootItem();
  if (root.IsOk()) {
    wxTreeItemIdValue c1;
    for (wxTreeItemId a = p_tree->GetFirstChild(root, c1); a.IsOk();
         a = p_tree->GetNextChild(root, c1)) {
      ++rows;
      if (p_tree->IsExpanded(a)) rows += (int)p_tree->GetChildrenCount(a, false);
    }
  }
  rows = std::max(1, rows);
  int rowHeight = p_tree->GetCharHeight() + 8;
  wxTreeItemIdValue c2;
  wxTreeItemId first = root.IsOk() ? p_tree->GetFirstChild(root, c2)
                                   : wxTreeItemId();
  wxRect r;
  if (first.IsOk() && p_tree->GetBoundingRect(first, r) && r.height > 0) {
    rowHeight = r.height;
  }
  int header = 6;
  wxStaticText *labels[3] = { p_title, p_codeTheory, p_energy };
  for (int i = 0; i < 3; ++i) {
    if (labels[i]->IsShown()) header += labels[i]->GetBestSize().y;
  }
  //  The cap counts rows, not pixels: the header above them is a fixed cost
  //  and a pixel cap left a short window one and a half rows.
  return header + 6 + std::min(rows, std::max(2, maxRows)) * rowHeight + 8;
}


void PropertyIndexPanel::setSelected(const std::string& name)
{
  selectEntry(name, true);
}


std::string PropertyIndexPanel::selected() const
{
  wxTreeItemId item = p_tree->GetSelection();
  if (!item.IsOk()) return "";
  NameData *d = dynamic_cast<NameData*>(p_tree->GetItemData(item));
  return d ? d->name : "";
}


bool PropertyIndexPanel::click(const std::string& name)
{
  p_tree->SetFocus();
  return selectEntry(name, false);
}


bool PropertyIndexPanel::selectEntry(const std::string& name, bool quiet)
{
  p_quiet = quiet;
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
  if (!found && quiet) {
    p_tree->UnselectAll();
  }
  p_quiet = false;
  return found;
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


std::vector<std::string> PropertyIndexPanel::rowRects() const
{
  std::vector<std::string> out;
  wxPoint org = p_tree->GetScreenPosition();
  wxSize sz = p_tree->GetClientSize();
  int sx = 0, sy = 0;
  p_tree->GetViewStart(&sx, &sy);
  int ux = 0, uy = 0;
  p_tree->GetScrollPixelsPerUnit(&ux, &uy);
  char buf[256];
  snprintf(buf, sizeof(buf), "treewin %d %d %d %d view %d %d unit %d %d",
           org.x, org.y, sz.x, sz.y, sx, sy, ux, uy);
  out.push_back(buf);
  wxTreeItemId root = p_tree->GetRootItem();
  if (!root.IsOk()) return out;
  wxTreeItemIdValue c1;
  for (wxTreeItemId it = p_tree->GetFirstChild(root, c1); it.IsOk();
       it = p_tree->GetNextChild(root, c1)) {
    wxTreeItemId items[1] = { it };
    for (int k = 0; k < 1; ++k) {
      wxRect r;
      if (p_tree->GetBoundingRect(items[k], r, true)) {
        snprintf(buf, sizeof(buf), "row %d %d %d %d %s", org.x + r.x,
                 org.y + r.y, r.width, r.height,
                 p_tree->GetItemText(items[k]).ToStdString().c_str());
        out.push_back(buf);
      }
    }
    wxTreeItemIdValue c2;
    for (wxTreeItemId ch = p_tree->GetFirstChild(it, c2); ch.IsOk();
         ch = p_tree->GetNextChild(it, c2)) {
      wxRect r;
      if (p_tree->GetBoundingRect(ch, r, true)) {
        snprintf(buf, sizeof(buf), "row %d %d %d %d %s", org.x + r.x,
                 org.y + r.y, r.width, r.height,
                 p_tree->GetItemText(ch).ToStdString().c_str());
        out.push_back(buf);
      }
    }
  }
  return out;
}


std::vector<std::string> PropertyIndexPanel::listing() const
{
  std::vector<std::string> out;
  for (size_t i = 0; i < p_entries.size(); ++i) {
    out.push_back(p_entries[i].group + "/" + p_entries[i].name);
  }
  return out;
}
