#include "CompareEnergiesDialog.H"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "CompareEnergies.H"

using std::string;

CompareEnergiesDialog::CompareEnergiesDialog(wxWindow *parent,
    const std::vector<std::pair<string, string> >& calcs,
    const string& a, const string& b)
  : wxDialog(parent, wxID_ANY, _("Compare Energies"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_calcs(calcs)
{
  p_a = new wxChoice(this, wxID_ANY);
  p_b = new wxChoice(this, wxID_ANY);
  int sa = wxNOT_FOUND, sb = wxNOT_FOUND;
  for (size_t i = 0; i < p_calcs.size(); i++) {
    p_a->Append(wxString::FromUTF8(p_calcs[i].first.c_str()));
    p_b->Append(wxString::FromUTF8(p_calcs[i].first.c_str()));
    if (p_calcs[i].second == a) sa = i;
    if (p_calcs[i].second == b) sb = i;
  }
  if (sa == wxNOT_FOUND && !p_calcs.empty()) sa = 0;
  if (sb == wxNOT_FOUND && !p_calcs.empty()) sb = p_calcs.size() > 1 ? 1 : 0;
  p_a->SetSelection(sa);
  p_b->SetSelection(sb);

  p_results = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                             wxSize(480, 150),
                             wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_notes = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                           wxSize(480, 90), wxTE_MULTILINE | wxTE_READONLY);

  wxFlexGridSizer *pick = new wxFlexGridSizer(2, 5, 8);
  pick->AddGrowableCol(1);
  pick->Add(new wxStaticText(this, wxID_ANY, _("Calculation A")), 0,
            wxALIGN_CENTER_VERTICAL);
  pick->Add(p_a, 1, wxEXPAND);
  pick->Add(new wxStaticText(this, wxID_ANY, _("Calculation B")), 0,
            wxALIGN_CENTER_VERTICAL);
  pick->Add(p_b, 1, wxEXPAND);

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(pick, 0, wxEXPAND | wxALL, 10);
  top->Add(new wxStaticText(this, wxID_ANY, _("Energies")), 0,
           wxLEFT | wxRIGHT, 10);
  top->Add(p_results, 1, wxEXPAND | wxALL, 10);
  top->Add(new wxStaticText(this, wxID_ANY, _("Notes")), 0,
           wxLEFT | wxRIGHT, 10);
  top->Add(p_notes, 1, wxEXPAND | wxALL, 10);
  top->Add(CreateStdDialogButtonSizer(wxCLOSE), 0, wxEXPAND | wxALL, 10);
  SetSizerAndFit(top);
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (IsModal()) EndModal(wxID_CLOSE); else Hide();
  }, wxID_CLOSE);

  p_a->Bind(wxEVT_CHOICE, &CompareEnergiesDialog::OnChoice, this);
  p_b->Bind(wxEVT_CHOICE, &CompareEnergiesDialog::OnChoice, this);
  update();
}

void CompareEnergiesDialog::update()
{
  const int ia = p_a->GetSelection(), ib = p_b->GetSelection();
  if (ia < 0 || ib < 0) {
    p_results->SetValue("");
    p_notes->SetValue(_("This project has no calculations to compare."));
    return;
  }
  const EnergyEntry a = gatherEnergyEntry(p_calcs[ia].second);
  const EnergyEntry b = gatherEnergyEntry(p_calcs[ib].second);
  const EnergyComparison c = compareEnergies(a, b);
  p_results->SetValue(wxString::FromUTF8(comparisonResults(a, b, c).c_str()));
  p_notes->SetValue(wxString::FromUTF8(comparisonNotes(c).c_str()));
}

string CompareEnergiesDialog::resultText() const
{
  return string(p_results->GetValue().ToUTF8());
}

string CompareEnergiesDialog::notesText() const
{
  return string(p_notes->GetValue().ToUTF8());
}
