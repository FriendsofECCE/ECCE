#include "AnotherCodeDialog.H"
#include "dsm/CodeChoice.H"

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
    std::function<bool(EcceURL&)> chooser, int hiddenCodes)
  : wxDialog(parent, wxID_ANY, count == 1 ? _("New Calculation in Another Code")
                                          : _("New Calculations in Another Code")),
    p_codes(codes), p_folder(folder), p_chooser(chooser), p_hidden(hiddenCodes)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  //  Wrapped, so the longest sentence does not set the dialog's width.
  const int wrap = FromDIP(420);
  wxStaticText *intro = new wxStaticText(this, wxID_ANY, U(count == 1
      ? "A new calculation is made with the final geometry, charge and "
        "multiplicity of the selected one."
      : std::to_string(count) + " new calculations are made, one from each "
        "selected calculation, with its final geometry, charge and multiplicity."));
  intro->Wrap(wrap);
  top->Add(intro, 0, wxALL, 10);
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 8, 8);
  grid->AddGrowableCol(1);
  grid->Add(new wxStaticText(this, wxID_ANY, _("Code")), 0, wxALIGN_CENTER_VERTICAL);
  p_code = new wxChoice(this, wxID_ANY);
  for (const auto& c : p_codes) p_code->Append(U(c.first));
  if (p_hidden > 0) p_code->Append(U(CodeChoice::moreLabel(p_hidden)));
  int sel = 0;
  for (size_t i = 0; i < p_codes.size(); i++)
    if (p_codes[i].first == "ORCA") sel = (int)i;
  p_code->SetSelection(sel);
  grid->Add(p_code, 1, wxEXPAND);
  grid->Add(new wxStaticText(this, wxID_ANY, _("Name ending")), 0, wxALIGN_CENTER_VERTICAL);
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
  wxStaticText *next = new wxStaticText(this, wxID_ANY, U(count == 1
      ? "It opens in the Calculation Editor, where you set up the theory, run "
        "type, basis set and launch settings."
      : "The first one opens in the Calculation Editor, where you set up the "
        "theory, run type, basis set and launch settings; the other " +
        std::to_string(count - 1) + " get the same setup."));
  next->Wrap(wrap);
  top->Add(next, 0, wxALL, 10);
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
  return i >= 0 && i < (int)p_codes.size() ? p_codes[i].second : string();
}

string AnotherCodeDialog::codeLabel() const
{
  int i = p_code->GetSelection();
  return i >= 0 && i < (int)p_codes.size() ? p_codes[i].first : string();
}

std::vector<string> AnotherCodeDialog::labels() const
{
  std::vector<string> out;
  for (unsigned i = 0; i < p_code->GetCount(); i++)
    out.push_back(string(p_code->GetString(i).ToUTF8()));
  return out;
}

bool AnotherCodeDialog::wantsMoreCodes() const
{
  return p_hidden > 0 && p_code->GetSelection() == (int)p_codes.size();
}

string AnotherCodeDialog::suffix() const
{
  return string(p_suffix->GetValue().ToUTF8());
}

void AnotherCodeDialog::OnCode(wxCommandEvent&)
{
  if (!p_suffixEdited && !wantsMoreCodes()) p_suffix->ChangeValue(U("_" + lower(codeLabel())));
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
