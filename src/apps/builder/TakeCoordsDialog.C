#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "TakeCoordsDialog.H"

TakeCoordsDialog::TakeCoordsDialog(wxWindow *parent,
                                   const std::vector<std::string>& names,
                                   const std::string& atoms)
  : wxDialog(parent, wxID_ANY, _("Take Coordinates From"))
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(this, wxID_ANY,
      _("Structure with the same atoms, in the same order:")), 0, wxALL, 8);
  p_calc = new wxChoice(this, wxID_ANY);
  for (const std::string& n : names) p_calc->Append(wxString::FromUTF8(n.c_str()));
  p_calc->SetSelection(0);
  top->Add(p_calc, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
  top->Add(new wxStaticText(this, wxID_ANY,
      _("Atoms to take from it (numbers from 1, e.g. 1-12,40):")), 0, wxALL, 8);
  p_atoms = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(atoms.c_str()),
                           wxDefaultPosition, wxSize(260, -1));
  top->Add(p_atoms, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
  p_align = new wxCheckBox(this, wxID_ANY,
      _("Align first (fit it onto this structure over the other atoms)"));
  top->Add(p_align, 0, wxALL, 8);
  top->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
  SetSizerAndFit(top);
}

int TakeCoordsDialog::source() const { return p_calc->GetSelection(); }

std::string TakeCoordsDialog::atoms() const
{
  return std::string(p_atoms->GetValue().utf8_str());
}

bool TakeCoordsDialog::align() const { return p_align->GetValue(); }
