/**
 * @file
 *
 * See BasisPanel.H.
 */
#include "wx/wx.h"
#include "wx/listctrl.h"

#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "util/StringConverter.H"

#include "BasisPanel.H"
#include "CalcEd.H"

using std::string;
using std::vector;

namespace {

struct Section { TGaussianBasisSet::GBSType type; const char *label; };
const Section SECTIONS[] = {
  {TGaussianBasisSet::pople, "Pople"},
  {TGaussianBasisSet::other_segmented, "Segmented"},
  {TGaussianBasisSet::correlation_consistent, "Correlation consistent"},
  {TGaussianBasisSet::other_generally_contracted, "Generally contracted"},
  {TGaussianBasisSet::ECPOrbital, "With ECP"},
  {TGaussianBasisSet::DFTOrbital, "DFT orbital"}};

const char *sectionLabel(TGaussianBasisSet::GBSType t)
{
  for (const Section& s : SECTIONS) if (s.type == t) return s.label;
  return "";
}

string join(const vector<string>& v, const char *sep = ", ")
{
  string out;
  for (const string& s : v) out += (out.empty() ? "" : sep) + s;
  return out;
}

}  // namespace


BasisPanel::BasisPanel(wxWindow *parent, CalcEd *editor)
  : wxPanel(parent, wxID_ANY), p_ed(editor)
{
  EDSIServerCentral central;
  p_lib.reset(new EDSIGaussianBasisSetLibrary(
      central.getDefaultBasisSetLibrary()));

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  p_filter = new wxTextCtrl(this, wxID_ANY);
  p_filter->SetHint("Filter the basis sets");
  top->Add(p_filter, 0, wxEXPAND | wxALL, 4);
  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(300, 180),
                          wxLC_REPORT | wxLC_SINGLE_SEL);
  p_list->AppendColumn("Basis set", wxLIST_FORMAT_LEFT, 150);
  p_list->AppendColumn("Type", wxLIST_FORMAT_LEFT, 150);
  top->Add(p_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
  p_details = new wxStaticText(this, wxID_ANY, "");
  p_cover = new wxStaticText(this, wxID_ANY, "");
  p_message = new wxStaticText(this, wxID_ANY, "");
  p_message->SetForegroundColour(*wxRED);
  top->Add(p_details, 0, wxEXPAND | wxALL, 4);
  top->Add(p_cover, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  top->Add(p_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  p_use = new wxButton(this, wxID_ANY, "Use for all atoms");
  wxButton *full = new wxButton(this, wxID_ANY, "Full Basis Set Tool...");
  row->Add(p_use, 0, wxRIGHT, 6);
  row->Add(full, 0);
  top->Add(row, 0, wxALL, 4);
  top->Add(new wxStaticText(this, wxID_ANY, "Current assignment"), 0,
           wxLEFT | wxTOP, 4);
  p_current = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_current, 0, wxEXPAND | wxALL, 4);
  SetSizer(top);
  p_use->Enable(false);

  for (const Section& s : SECTIONS) {
    vector<const char*> *names = p_lib->gbsNameList(s.type);
    if (!names) continue;
    for (const char *n : *names) if (n) p_all.push_back({n, s.type});
    delete names;
  }
  p_filter->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { fill(); });
  p_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& e) {
    p_picked = e.GetIndex();
    chosen();
  });
  p_use->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { useForAll(); });
  full->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    p_ed->openBasisSetTool();
  });
  fill();
  refresh();
}


BasisPanel::~BasisPanel() {}


void BasisPanel::fill()
{
  wxString f = p_filter->GetValue().Lower();
  p_shown.clear();
  p_list->DeleteAllItems();
  for (const Entry& e : p_all) {
    if (!f.empty() && wxString::FromUTF8(e.name.c_str()).Lower().Find(f) ==
        wxNOT_FOUND) continue;
    long i = p_list->InsertItem(p_list->GetItemCount(),
                                wxString::FromUTF8(e.name.c_str()));
    p_list->SetItem(i, 1, sectionLabel(e.type));
    p_shown.push_back(e);
  }
  p_picked = -1;
  chosen();
}


void BasisPanel::setFilter(const string& text)
{
  p_filter->ChangeValue(wxString::FromUTF8(text.c_str()));
  fill();
}


bool BasisPanel::pick(const string& name)
{
  for (size_t i = 0; i < p_shown.size(); i++) {
    if (p_shown[i].name != name) continue;
    p_picked = (int)i;
    p_list->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    chosen();
    return true;
  }
  return false;
}


void BasisPanel::chosen()
{
  p_covered.clear();
  p_uncovered.clear();
  p_message->SetLabel("");
  if (p_picked < 0 || p_picked >= (int)p_shown.size()) {
    p_details->SetLabel("");
    p_cover->SetLabel("");
    p_use->Enable(false);
    Layout();
    return;
  }
  const Entry& e = p_shown[p_picked];
  string nice = p_lib->getNiceName(e.name);
  p_details->SetLabel(wxString::FromUTF8((e.name + (nice.empty() ||
      nice == e.name ? "" : " (" + nice + ")") + ", " +
      sectionLabel(e.type)).c_str()));
  for (const string& el : p_ed->systemElements()) {
    (p_lib->isSupported(e.name, e.type, el) ? p_covered : p_uncovered)
        .push_back(el);
  }
  wxString text = "Covers: " + wxString::FromUTF8(join(p_covered).c_str());
  if (!p_uncovered.empty()) {
    text += "\nNot covered: " + wxString::FromUTF8(join(p_uncovered).c_str());
    p_cover->SetForegroundColour(*wxRED);
  } else {
    p_cover->SetForegroundColour(wxNullColour);
  }
  p_cover->SetLabel(text);
  p_use->Enable(p_uncovered.empty() && !p_covered.empty());
  Layout();
}


bool BasisPanel::useForAll()
{
  if (p_picked < 0 || p_picked >= (int)p_shown.size()) return false;
  string err;
  bool ok = p_ed->useBasisForAll(p_shown[p_picked].name,
                                 p_shown[p_picked].type, err);
  p_message->SetLabel(wxString::FromUTF8(err.c_str()));
  p_message->Wrap(std::max(200, GetSize().x - 20));
  Layout();
  return ok;
}


void BasisPanel::refresh()
{
  vector<string> lines = p_ed->basisAssignmentLines();
  p_current->SetLabel(wxString::FromUTF8(
      lines.empty() ? "None" : [&]() {
        string out;
        for (const string& l : lines) out += (out.empty() ? "" : "\n") + l;
        return out;
      }().c_str()));
  Layout();
}


string BasisPanel::describe() const
{
  string cur;
  for (const string& l : p_ed->basisAssignmentLines())
    cur += (cur.empty() ? "" : "|") + l;
  for (char& c : cur) if (c == ' ') c = '_';
  return "list=" + StringConverter::toString((int)p_shown.size()) +
         " of " + StringConverter::toString((int)p_all.size()) +
         " picked=" + (p_picked >= 0 ? p_shown[p_picked].name : "-") +
         " covered=" + join(p_covered, ",") + " uncovered=" + join(p_uncovered, ",") +
         " use=" + (p_use->IsEnabled() ? "on" : "off") +
         " assigned=" + cur;
}
