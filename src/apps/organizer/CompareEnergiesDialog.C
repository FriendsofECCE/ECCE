#include "CompareEnergiesDialog.H"

#include <algorithm>
#include <cstdlib>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/font.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "CompareEnergies.H"

using std::string;

CompareEnergiesDialog::CompareEnergiesDialog(wxWindow *parent,
    const std::vector<std::pair<string, string> >& calcs,
    const Side& a, const Side& b)
  : wxDialog(parent, wxID_ANY, _("Compare Energies"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_calcs(calcs)
{
  std::stable_sort(p_calcs.begin(), p_calcs.end(),
      [](const std::pair<string, string>& x, const std::pair<string, string>& y) {
        return calcNameLess(x.first, y.first); });

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  const char *const title[2] = {"Side A", "Side B"};
  for (int s = 0; s < 2; s++) {
    wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, wxGetTranslation(title[s]));
    p_side[s].host = box->GetStaticBox();
    p_side[s].rows = new wxBoxSizer(wxVERTICAL);
    box->Add(p_side[s].rows, 0, wxEXPAND);
    wxButton *add = new wxButton(box->GetStaticBox(), wxID_ANY,
                                 _("Add calculation"));
    box->Add(add, 0, wxALL, 4);
    add->Bind(wxEVT_BUTTON, [this, s](wxCommandEvent&) {
      addRow(s, string(), 1);
      update();
    });
    top->Add(box, 0, wxEXPAND | wxALL, 6);
    const Side& init = s == 0 ? a : b;
    for (size_t i = 0; i < init.size(); i++) addRow(s, init[i].first, init[i].second);
    if (init.empty()) addRow(s, string(), 1);
  }

  p_results = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                             wxSize(640, 140),
                             wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_results->SetFont(wxFont(wxFontInfo(wxSystemSettings::GetFont(
      wxSYS_DEFAULT_GUI_FONT).GetPointSize()).Family(wxFONTFAMILY_TELETYPE)));
  p_notes = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                           wxSize(640, 90), wxTE_MULTILINE | wxTE_READONLY);
  top->Add(new wxStaticText(this, wxID_ANY, _("Energies")), 0,
           wxLEFT | wxRIGHT, 10);
  top->Add(p_results, 1, wxEXPAND | wxALL, 10);
  top->Add(new wxStaticText(this, wxID_ANY, _("Notes")), 0,
           wxLEFT | wxRIGHT, 10);
  top->Add(p_notes, 1, wxEXPAND | wxALL, 10);

  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *copy = new wxButton(this, wxID_ANY, _("Copy"));
  copy->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (wxTheClipboard->Open()) {
      wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(p_copy.c_str())));
      wxTheClipboard->Close();
    }
  });
  buttons->Add(copy, 0, wxRIGHT, 8);
  buttons->AddStretchSpacer();
  buttons->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  top->Add(buttons, 0, wxEXPAND | wxALL, 10);
  SetSizerAndFit(top);
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (IsModal()) EndModal(wxID_CLOSE); else Hide();
  }, wxID_CLOSE);
  update();
}

void CompareEnergiesDialog::addRow(int side, const string& url, int coeff)
{
  Row r;
  r.panel = new wxPanel(p_side[side].host);
  r.sign = new wxChoice(r.panel, wxID_ANY, wxDefaultPosition, wxSize(54, -1));
  r.sign->Append("+");
  r.sign->Append(wxString::FromUTF8("\xE2\x88\x92"));
  r.sign->SetSelection(coeff < 0 ? 1 : 0);
  r.count = new wxSpinCtrl(r.panel, wxID_ANY, "", wxDefaultPosition,
                           wxSize(70, -1), wxSP_ARROW_KEYS, 1, 99,
                           std::max(1, std::abs(coeff)));
  r.calc = new wxChoice(r.panel, wxID_ANY);
  int sel = wxNOT_FOUND;
  for (size_t i = 0; i < p_calcs.size(); i++) {
    r.calc->Append(wxString::FromUTF8(p_calcs[i].first.c_str()));
    if (p_calcs[i].second == url) sel = (int)i;
  }
  if (sel == wxNOT_FOUND && !p_calcs.empty())
    sel = std::min<int>((int)p_side[side].items.size() + side, (int)p_calcs.size() - 1);
  r.calc->SetSelection(sel);
  wxButton *rm = new wxButton(r.panel, wxID_ANY, wxString::FromUTF8("\xC3\x97"),
                              wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
  wxBoxSizer *h = new wxBoxSizer(wxHORIZONTAL);
  h->Add(r.sign, 0, wxRIGHT, 4);
  h->Add(r.count, 0, wxRIGHT, 4);
  h->Add(r.calc, 1, wxRIGHT, 4);
  h->Add(rm, 0);
  r.panel->SetSizer(h);
  wxPanel *panel = r.panel;
  rm->Bind(wxEVT_BUTTON, [this, side, panel](wxCommandEvent&) {
    removeRow(side, panel);
  });
  auto changed = [this](wxCommandEvent&) { update(); };
  r.sign->Bind(wxEVT_CHOICE, changed);
  r.calc->Bind(wxEVT_CHOICE, changed);
  r.count->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { update(); });
  p_side[side].rows->Add(r.panel, 0, wxEXPAND | wxALL, 2);
  p_side[side].items.push_back(r);
  if (GetSizer()) {
    Layout();
    Fit();
  }
}

void CompareEnergiesDialog::removeRow(int side, wxPanel *panel)
{
  std::vector<Row>& v = p_side[side].items;
  if (v.size() < 2) return;
  for (size_t i = 0; i < v.size(); i++)
    if (v[i].panel == panel) {
      v.erase(v.begin() + i);
      break;
    }
  p_side[side].rows->Detach(panel);
  CallAfter([panel]() { panel->Destroy(); });
  Layout();
  Fit();
  update();
}

CompareEnergiesDialog::Side CompareEnergiesDialog::termsOf(int side) const
{
  Side out;
  for (const Row& r : p_side[side].items) {
    const int i = r.calc->GetSelection();
    if (i < 0) continue;
    out.push_back(std::make_pair(p_calcs[i].second,
                                 (r.sign->GetSelection() == 1 ? -1 : 1) * r.count->GetValue()));
  }
  return out;
}

void CompareEnergiesDialog::update()
{
  std::vector<EnergyTerm> t[2];
  for (int s = 0; s < 2; s++) {
    const Side terms = termsOf(s);
    for (size_t i = 0; i < terms.size(); i++)
      t[s].push_back(gatherEnergyTerm(terms[i].first, terms[i].second));
  }
  if (t[0].empty() || t[1].empty()) {
    p_results->SetValue("");
    p_notes->SetValue(_("This project has no calculations to compare."));
    p_copy.clear();
    return;
  }
  const SumResult r = sumEnergies(t[0], t[1]);
  p_results->SetValue(wxString::FromUTF8(sumTable(r, false).c_str()));
  p_notes->SetValue(wxString::FromUTF8(sumNotes(r).c_str()));
  p_copy = sumText(r, true);
}

string CompareEnergiesDialog::resultText() const
{
  return string(p_results->GetValue().ToUTF8());
}

string CompareEnergiesDialog::notesText() const
{
  return string(p_notes->GetValue().ToUTF8());
}

string CompareEnergiesDialog::copyText() const { return p_copy; }
