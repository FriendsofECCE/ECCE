/**
 * @file
 *
 * See BasisPanel.H.
 */
#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/checklst.h"

#include <algorithm>

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
  {TGaussianBasisSet::DFTOrbital, "DFT orbital"},
  {TGaussianBasisSet::ecp, "ECP"},
  {TGaussianBasisSet::charge, "Charge fitting"},
  {TGaussianBasisSet::exchange, "Exchange fitting"}};

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
  p_filter->SetHint("Filter");
  top->Add(p_filter, 0, wxEXPAND | wxALL, 4);
  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(300, 180),
                          wxLC_REPORT | wxLC_SINGLE_SEL);
  p_list->AppendColumn("Basis set", wxLIST_FORMAT_LEFT, 150);
  p_list->AppendColumn("Type", wxLIST_FORMAT_LEFT, 110);
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
  p_assign = new wxButton(this, wxID_ANY, "Assign to checked elements");
  wxButton *full = new wxButton(this, wxID_ANY, "Full Basis Set Tool...");
  p_elements = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition,
                                  wxSize(-1, 70));
  top->Add(new wxStaticText(this, wxID_ANY, "Elements"), 0, wxLEFT | wxTOP, 4);
  top->Add(p_elements, 0, wxEXPAND | wxALL, 4);
  row->Add(p_assign, 0, wxRIGHT, 6);
  row->Add(p_use, 0, wxRIGHT, 6);
  row->Add(full, 0);
  top->Add(row, 0, wxALL, 4);
  p_offer = new wxStaticText(this, wxID_ANY, "");
  p_offerButton = new wxButton(this, wxID_ANY, "Add ECP");
  top->Add(p_offer, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  top->Add(p_offerButton, 0, wxALL, 4);
  p_offer->Show(false);
  p_offerButton->Show(false);
  top->Add(new wxStaticText(this, wxID_ANY, "Current assignment"), 0,
           wxLEFT | wxTOP, 4);
  p_current = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_current, 0, wxEXPAND | wxALL, 4);
  SetSizer(top);
  p_use->Enable(false);
  p_assign->Enable(false);

  for (const Section& s : SECTIONS) {
    vector<const char*> *names = p_lib->gbsNameList(s.type);
    if (!names) continue;
    for (const char *n : *names) if (n) p_all.push_back({n, s.type});
    delete names;
  }
  p_filter->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { fill(); });
  // Basis set takes the width the Type column leaves.
  p_list->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
    int rest = p_list->GetClientSize().x - p_list->GetColumnWidth(1);
    if (rest > 100) p_list->SetColumnWidth(0, rest);
    e.Skip();
  });
  p_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& e) {
    p_picked = e.GetIndex();
    chosen();
  });
  p_use->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { useForAll(); });
  p_assign->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { assignChecked(); });
  p_offerButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { acceptEcp(); });
  p_elements->Bind(wxEVT_CHECKLISTBOX, [this](wxCommandEvent&) { chosen(); });
  full->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    p_ed->openBasisSetTool();
  });
  fillElements();
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
    p_assign->Enable(false);
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
  Kind kind = kindOf(e.type);
  p_use->Enable(kind == ORBITAL && p_uncovered.empty() && !p_covered.empty());
  // An orbital set must cover every checked element; an ECP or fitting set
  // goes on the ones it covers.
  size_t on = 0, onCovered = 0;
  for (const string& el : checked()) {
    on++;
    if (std::find(p_covered.begin(), p_covered.end(), el) != p_covered.end())
      onCovered++;
  }
  p_assign->SetLabel(kind == ORBITAL ? "Assign to checked elements"
                     : kind == ECP ? "Add ECP to checked elements"
                                   : "Add fitting set to checked elements");
  p_assign->Enable(on && (kind == ORBITAL ? onCovered == on : onCovered > 0));
  Layout();
}


BasisPanel::Kind BasisPanel::kindOf(TGaussianBasisSet::GBSType type) const
{
  return type == TGaussianBasisSet::ecp ? ECP
       : (type == TGaussianBasisSet::charge ||
          type == TGaussianBasisSet::exchange) ? FIT : ORBITAL;
}


void BasisPanel::fillElements()
{
  p_elementNames = p_ed->systemElements();
  p_elements->Clear();
  for (size_t i = 0; i < p_elementNames.size(); i++) {
    p_elements->Append(wxString::FromUTF8(p_elementNames[i].c_str()));
    p_elements->Check(i, true);
  }
}


vector<string> BasisPanel::checked() const
{
  vector<string> out;
  for (size_t i = 0; i < p_elementNames.size(); i++)
    if (p_elements->IsChecked(i)) out.push_back(p_elementNames[i]);
  return out;
}


void BasisPanel::say(const string& text)
{
  p_messageText = text;
  p_message->SetLabel(wxString::FromUTF8(text.c_str()));
  p_message->Wrap(std::max(200, GetSize().x - 20));
}


void BasisPanel::offerEcp(const string& basis, const vector<string>& els)
{
  p_ecpOffer = p_ed->matchingEcp(basis, els, p_ecpOfferFor);
  if (p_ecpOffer.empty()) p_ecpOfferFor.clear();
  p_offer->SetLabel(p_ecpOffer.empty() ? wxString() : wxString::FromUTF8(
      ("The library has a matching ECP, " + p_ecpOffer + ", for " +
       join(p_ecpOfferFor) + ". The set was built for it.").c_str()));
  p_offerButton->SetLabel(wxString::FromUTF8(("Add " + p_ecpOffer).c_str()));
  p_offer->Show(!p_ecpOffer.empty());
  p_offerButton->Show(!p_ecpOffer.empty());
  p_offer->Wrap(std::max(200, GetSize().x - 20));
  Layout();
}


bool BasisPanel::assignChecked()
{
  if (p_picked < 0 || p_picked >= (int)p_shown.size()) return false;
  const Entry e = p_shown[p_picked];
  vector<string> els = checked();
  if (els.empty()) {
    say("Check at least one element.");
    return false;
  }
  string err;
  bool ok = p_ed->assignBasis(e.name, e.type, els, err);
  say(err);
  if (ok && kindOf(e.type) != FIT)
    offerEcp(e.type == TGaussianBasisSet::ECPOrbital ? e.name : "", els);
  Layout();
  return ok;
}


bool BasisPanel::assignNamed(const string& name, Kind kind,
                             const vector<string>& elements)
{
  setFilter("");
  size_t at = p_shown.size();
  for (size_t i = 0; i < p_shown.size(); i++)
    if (p_shown[i].name == name && kindOf(p_shown[i].type) == kind) {
      at = i;
      break;
    }
  if (at == p_shown.size()) return false;
  p_picked = (int)at;
  p_list->SetItemState(at, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  for (size_t i = 0; i < p_elementNames.size(); i++)
    p_elements->Check(i, elements.empty() || std::find(elements.begin(),
        elements.end(), p_elementNames[i]) != elements.end());
  chosen();
  assignChecked();
  return true;
}


bool BasisPanel::acceptEcp()
{
  if (p_ecpOffer.empty()) return false;
  string err;
  bool ok = p_ed->assignBasis(p_ecpOffer, TGaussianBasisSet::ecp,
                              p_ecpOfferFor, err);
  say(err);
  if (ok) offerEcp("", p_ecpOfferFor);
  return ok;
}


bool BasisPanel::useForAll()
{
  if (p_picked < 0 || p_picked >= (int)p_shown.size()) return false;
  string err;
  const Entry e = p_shown[p_picked];
  bool ok = p_ed->useBasisForAll(e.name, e.type, err);
  say(err);
  if (ok) offerEcp(e.type == TGaussianBasisSet::ECPOrbital ? e.name : "",
                   vector<string>());
  Layout();
  return ok;
}


void BasisPanel::refresh()
{
  if (p_elementNames != p_ed->systemElements()) fillElements();
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
  string msgText = p_messageText.empty() ? "-" : p_messageText;
  for (char& c : msgText) c = c == ' ' ? '_' : c == '\n' ? '|' : c;
  return "list=" + StringConverter::toString((int)p_shown.size()) +
         " of " + StringConverter::toString((int)p_all.size()) +
         " picked=" + (p_picked >= 0 ? p_shown[p_picked].name : "-") +
         " covered=" + join(p_covered, ",") + " uncovered=" + join(p_uncovered, ",") +
         " use=" + (p_use->IsEnabled() ? "on" : "off") +
         " assign=" + (p_assign->IsEnabled() ? "on" : "off") +
         " checked=" + join(checked(), ",") +
         " ecpoffer=" + (p_ecpOffer.empty() ? "-" : p_ecpOffer + ":" +
                         join(p_ecpOfferFor, ",")) +
         " msg=" + msgText +
         " assigned=" + cur;
}
