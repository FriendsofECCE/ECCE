#include "AnotherCodeDialog.H"

#include <algorithm>
#include <cctype>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

using std::string;

namespace {
wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }
string lower(string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
  return s;
}
}

AnotherCodeDialog::AnotherCodeDialog(wxWindow *parent, int count,
    const std::vector<std::pair<string, string> >& codes, const EcceURL& folder,
    std::function<bool(EcceURL&)> chooser)
  : wxDialog(parent, wxID_ANY, _("New from This in Another Code")),
    p_codes(codes), p_folder(folder), p_chooser(chooser)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(this, wxID_ANY, U(count == 1
      ? "One new calculation is made from the selected one, with its final "
        "geometry, charge and multiplicity."
      : std::to_string(count) + " new calculations are made from the selected "
        "ones, each with its final geometry, charge and multiplicity.")),
      0, wxALL, 10);
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 8, 8);
  grid->AddGrowableCol(1);
  grid->Add(new wxStaticText(this, wxID_ANY, _("Code")), 0, wxALIGN_CENTER_VERTICAL);
  p_code = new wxChoice(this, wxID_ANY);
  for (const auto& c : p_codes) p_code->Append(U(c.first));
  int sel = 0;
  for (size_t i = 0; i < p_codes.size(); i++)
    if (p_codes[i].first == "ORCA") sel = (int)i;
  p_code->SetSelection(sel);
  grid->Add(p_code, 1, wxEXPAND);
  grid->Add(new wxStaticText(this, wxID_ANY, _("End of the names")), 0, wxALIGN_CENTER_VERTICAL);
  p_suffix = new wxTextCtrl(this, wxID_ANY);
  grid->Add(p_suffix, 1, wxEXPAND);
  grid->Add(new wxStaticText(this, wxID_ANY, _("Folder")), 0, wxALIGN_CENTER_VERTICAL);
  wxBoxSizer *where = new wxBoxSizer(wxHORIZONTAL);
  p_where = new wxStaticText(this, wxID_ANY, U(p_folder.getFilePathTail()));
  where->Add(p_where, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  wxButton *choose = new wxButton(this, wxID_ANY, _("Choose..."));
  where->Add(choose, 0);
  grid->Add(where, 1, wxEXPAND);
  top->Add(grid, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
  top->Add(new wxStaticText(this, wxID_ANY, _("The first one opens in the Calculation "
      "Editor. Set up the theory, run type, basis set and launch settings there; "
      "the others are made with the same setup.")), 0, wxALL, 10);
  top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
  SetSizerAndFit(top);
  p_suffix->SetValue(U("_" + lower(codeLabel())));
  p_suffix->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
    if (p_suffix->IsModified()) p_suffixEdited = true;
  });
  p_code->Bind(wxEVT_CHOICE, &AnotherCodeDialog::OnCode, this);
  choose->Bind(wxEVT_BUTTON, &AnotherCodeDialog::OnFolder, this);
}

string AnotherCodeDialog::typeName() const
{
  int i = p_code->GetSelection();
  return i >= 0 ? p_codes[i].second : string();
}

string AnotherCodeDialog::codeLabel() const
{
  int i = p_code->GetSelection();
  return i >= 0 ? p_codes[i].first : string();
}

string AnotherCodeDialog::suffix() const
{
  return string(p_suffix->GetValue().ToUTF8());
}

void AnotherCodeDialog::OnCode(wxCommandEvent&)
{
  if (!p_suffixEdited) p_suffix->ChangeValue(U("_" + lower(codeLabel())));
}

void AnotherCodeDialog::OnFolder(wxCommandEvent&)
{
  EcceURL chosen = p_folder;
  if (p_chooser && p_chooser(chosen)) {
    p_folder = chosen;
    p_where->SetLabel(U(p_folder.getFilePathTail()));
    Layout();
  }
}
