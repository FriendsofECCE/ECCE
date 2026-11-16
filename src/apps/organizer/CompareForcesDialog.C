#include "CompareForcesDialog.H"
#include "CompareForces.H"

#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

CompareForcesDialog::CompareForcesDialog(wxWindow *parent,
    const std::string& a, const std::string& b)
  : wxDialog(parent, wxID_ANY, _("Compare Forces"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  const ForceComparison r = compareForces(gatherForceInput(a), gatherForceInput(b));
  p_text = "A\t" + a + "\nB\t" + b + "\n" + forceComparisonText(r);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxStaticText *names = new wxStaticText(this, wxID_ANY,
      wxString::FromUTF8(("A: " + a + "\nB: " + b).c_str()));
  names->Wrap(700);
  top->Add(names, 0, wxEXPAND | wxALL, 10);
  wxListCtrl *table = new wxListCtrl(this, wxID_ANY, wxDefaultPosition,
      wxSize(700, 180), wxLC_REPORT);
  p_table = table;
  table->InsertColumn(0, _("Element"));
  table->InsertColumn(1, _("n atoms"));
  table->InsertColumn(2, wxString::FromUTF8("max |Delta F| (eV/Å)"));
  table->InsertColumn(3, wxString::FromUTF8("rms |Delta F| (eV/Å)"));
  for (const ForceRow& row : r.rows) {
    const long i = table->InsertItem(table->GetItemCount(), wxString::FromUTF8(row.element.c_str()));
    table->SetItem(i, 1, wxString::Format("%lu", (unsigned long)row.count));
    table->SetItem(i, 2, wxString::Format("%.10g", row.maximum));
    table->SetItem(i, 3, wxString::Format("%.10g", row.rms));
  }
  for (int i = 0; i < 4; ++i) table->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
  top->Add(table, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
  std::string notes = r.problem;
  if (r.hasEnergy)
    notes += std::string(wxString::Format("Delta E = E(B) - E(A): %.12g eV/cell; %.12g meV/atom\n",
        r.deltaEV, r.deltaMeVPerAtom).ToUTF8());
  for (const auto& note : r.notes) notes += note + "\n";
  wxStaticText *message = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(notes.c_str()));
  p_notes = message;
  message->Wrap(700);
  top->Add(message, 0, wxEXPAND | wxALL, 10);
  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *copy = new wxButton(this, wxID_ANY, _("Copy"));
  copy->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (wxTheClipboard->Open()) {
      wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(p_text.c_str())));
      wxTheClipboard->Close();
    }
  });
  buttons->Add(copy);
  buttons->AddStretchSpacer();
  buttons->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  top->Add(buttons, 0, wxEXPAND | wxALL, 10);
  SetSizerAndFit(top);
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (IsModal()) EndModal(wxID_CLOSE); else Hide();
  }, wxID_CLOSE);
}

std::string CompareForcesDialog::resultText() const
{
  std::string out;
  for (long row = 0; row < p_table->GetItemCount(); ++row) {
    for (int column = 0; column < 4; ++column) {
      if (column) out += '\t';
      out += std::string(p_table->GetItemText(row, column).ToUTF8());
    }
    out += '\n';
  }
  return out + std::string(p_notes->GetLabel().ToUTF8());
}
