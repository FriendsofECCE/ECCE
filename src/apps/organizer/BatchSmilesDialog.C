#include <fstream>
#include <iterator>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "BatchSmilesDialog.H"
#include <wx/settings.h>

using std::string;
using std::vector;

namespace {
wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }
string S(const wxString& s) { return string(s.ToUTF8()); }
}


BatchSmilesDialog::BatchSmilesDialog(wxWindow *parent, const string& folderName,
    const vector<std::pair<string, string> >& templates)
  : wxDialog(parent, wxID_ANY, _("Batch Add from SMILES"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_templates(templates)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxStaticText *intro = new wxStaticText(this, wxID_ANY, U(
      "One calculation per row in " + folderName + ". The table needs a name "
      "and a SMILES column; the columns ticked under Measured values are stored "
      "with each calculation."));
  intro->Wrap(700);
  top->Add(intro, 0, wxALL, 8);

  wxBoxSizer *file = new wxBoxSizer(wxHORIZONTAL);
  file->Add(new wxStaticText(this, wxID_ANY, _("Table (CSV or TSV), or paste it:")),
            1, wxALIGN_CENTER_VERTICAL);
  wxButton *open = new wxButton(this, wxID_OPEN, _("Open File..."));
  file->Add(open, 0);
  top->Add(file, 0, wxLEFT | wxRIGHT | wxEXPAND, 8);
  p_text = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(640, 110),
                          wxTE_MULTILINE | wxTE_DONTWRAP);
  top->Add(p_text, 0, wxALL | wxEXPAND, 8);

  wxFlexGridSizer *cols = new wxFlexGridSizer(2, 4, 8);
  cols->AddGrowableCol(1);
  cols->Add(new wxStaticText(this, wxID_ANY, _("Name column:")), 0, wxALIGN_CENTER_VERTICAL);
  p_name = new wxChoice(this, wxID_ANY);
  cols->Add(p_name, 1, wxEXPAND);
  cols->Add(new wxStaticText(this, wxID_ANY, _("SMILES column:")), 0, wxALIGN_CENTER_VERTICAL);
  p_smiles = new wxChoice(this, wxID_ANY);
  cols->Add(p_smiles, 1, wxEXPAND);
  cols->Add(new wxStaticText(this, wxID_ANY, _("Measured values:")), 0, wxALIGN_TOP);
  p_responses = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 70));
  cols->Add(p_responses, 1, wxEXPAND);
  cols->Add(new wxStaticText(this, wxID_ANY, _("Setup:")), 0, wxALIGN_CENTER_VERTICAL);
  p_template = new wxChoice(this, wxID_ANY);
  p_template->Append(_("xTB GFN2-xTB optimisation on this computer"));
  for (const auto& t : p_templates) p_template->Append(U("Same as " + t.first));
  p_template->SetSelection(0);
  for (size_t i = 0; i < p_templates.size(); i++)
    if (p_templates[i].first == "QSAR template") p_template->SetSelection((int)i + 1);
  cols->Add(p_template, 1, wxEXPAND);
  top->Add(cols, 0, wxLEFT | wxRIGHT | wxEXPAND, 8);
  p_replace = new wxCheckBox(this, wxID_ANY, _(
      "Rebuild the existing calculations that have not been run, with this setup"));
  top->Add(p_replace, 0, wxLEFT | wxRIGHT | wxTOP, 8);
  wxStaticText *replaceNote = new wxStaticText(this, wxID_ANY, _(
      "Created and Ready calculations of the same name are deleted and made "
      "again; submitted, running and finished ones are kept."));
  replaceNote->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
  top->Add(replaceNote, 0, wxLEFT | wxRIGHT, 30);

  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(640, 200),
                          wxLC_REPORT | wxLC_SINGLE_SEL);
  top->Add(p_list, 1, wxALL | wxEXPAND, 8);
  p_summary = new wxStaticText(this, wxID_ANY, _("No table yet."));
  top->Add(p_summary, 0, wxLEFT | wxRIGHT | wxEXPAND, 8);

  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer;
  p_create = new wxButton(this, wxID_OK, _("Create"));
  buttons->AddButton(p_create);
  buttons->AddButton(new wxButton(this, wxID_CANCEL));
  buttons->Realize();
  top->Add(buttons, 0, wxALL | wxEXPAND, 8);
  SetSizerAndFit(top);
  p_create->Enable(false);

  open->Bind(wxEVT_BUTTON, &BatchSmilesDialog::OnOpen, this);
  p_text->Bind(wxEVT_TEXT, &BatchSmilesDialog::OnTextChanged, this);
  p_name->Bind(wxEVT_CHOICE, &BatchSmilesDialog::OnColumnChanged, this);
  p_smiles->Bind(wxEVT_CHOICE, &BatchSmilesDialog::OnColumnChanged, this);
  p_responses->Bind(wxEVT_CHECKLISTBOX, &BatchSmilesDialog::OnColumnChanged, this);
}


bool BatchSmilesDialog::loadFile(const string& path, string& why)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in) { why = "cannot read " + path; return false; }
  string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (text.compare(0, 2, "PK") == 0) {
    why = "This is a spreadsheet file. Save it as CSV (comma or tab separated) first.";
    return false;
  }
  setText(text);
  return true;
}


void BatchSmilesDialog::setText(const string& text)
{
  p_text->ChangeValue(U(text));
  parse();
}


void BatchSmilesDialog::OnOpen(wxCommandEvent&)
{
  wxFileDialog fd(this, _("Table of names and SMILES"), "", "",
                  _("Tables (*.csv;*.tsv;*.txt;*.smi)|*.csv;*.tsv;*.txt;*.smi|All files|*"),
                  wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (fd.ShowModal() != wxID_OK) return;
  string why;
  if (!loadFile(S(fd.GetPath()), why)) p_summary->SetLabel(U(why));
}


void BatchSmilesDialog::OnTextChanged(wxCommandEvent&) { parse(); }


void BatchSmilesDialog::OnColumnChanged(wxCommandEvent&) { refreshPreview(); }


void BatchSmilesDialog::parse()
{
  string why;
  p_parsed = SmilesBatch::parseTable(S(p_text->GetValue()), p_table, why);
  fillColumnChoices();
  if (!p_parsed) {
    p_rows.clear();
    p_list->ClearAll();
    p_summary->SetLabel(U(why.empty() ? "No table yet." : "Not a table: " + why));
    p_create->Enable(false);
    return;
  }
  refreshPreview();
}


void BatchSmilesDialog::fillColumnChoices()
{
  p_name->Clear();
  p_smiles->Clear();
  p_responses->Clear();
  if (!p_parsed) return;
  p_name->Append(_("(none: mol-001, ...)"));
  for (const string& h : p_table.header) {
    p_name->Append(U(h));
    p_smiles->Append(U(h));
    p_responses->Append(U(h));
  }
  SmilesBatch::Columns g = SmilesBatch::guessColumns(p_table);
  p_name->SetSelection(g.name + 1);
  if (g.smiles >= 0) p_smiles->SetSelection(g.smiles);
  for (int k : g.responses) p_responses->Check(k);
}


SmilesBatch::Columns BatchSmilesDialog::columns() const
{
  SmilesBatch::Columns c;
  c.name = p_name->GetSelection() - 1;
  c.smiles = p_smiles->GetSelection();
  for (unsigned i = 0; i < p_responses->GetCount(); i++)
    if (p_responses->IsChecked(i) && (int)i != c.smiles && (int)i != c.name)
      c.responses.push_back((int)i);
  return c;
}


void BatchSmilesDialog::refreshPreview()
{
  p_rows = SmilesBatch::preview(p_table, columns());
  const SmilesBatch::Columns c = columns();
  p_list->ClearAll();
  int col = 0;
  p_list->InsertColumn(col++, _("Name"), wxLIST_FORMAT_LEFT, 130);
  p_list->InsertColumn(col++, _("SMILES"), wxLIST_FORMAT_LEFT, 200);
  p_list->InsertColumn(col++, _("Charge"), wxLIST_FORMAT_RIGHT, 60);
  p_list->InsertColumn(col++, _("Atoms"), wxLIST_FORMAT_RIGHT, 55);
  for (int k : c.responses)
    p_list->InsertColumn(col++, U(p_table.header[k]), wxLIST_FORMAT_RIGHT, 90);
  p_list->InsertColumn(col, _("Status"), wxLIST_FORMAT_LEFT, 220);
  int good = 0;
  for (size_t i = 0; i < p_rows.size(); i++) {
    const SmilesBatch::Row& r = p_rows[i];
    long item = p_list->InsertItem((long)i, U(r.name));
    p_list->SetItem(item, 1, U(r.smiles));
    const bool ok = r.problem.empty();
    good += ok;
    p_list->SetItem(item, 2, ok ? wxString::Format("%+d", r.charge).Mid(r.charge == 0) : "");
    p_list->SetItem(item, 3, ok ? wxString::Format("%d", r.atoms) : "");
    int k = 4;
    for (const auto& m : r.measured) p_list->SetItem(item, k++, U(m.second));
    p_list->SetItem(item, col, ok ? _("ready") : U(r.problem));
    if (!ok) p_list->SetItemTextColour(item, wxColour(170, 0, 0));
  }
  const int bad = (int)p_rows.size() - good;
  p_summary->SetLabel(wxString::Format(
      "%d rows: %d will be made%s.", (int)p_rows.size(), good,
      bad ? wxString::Format(", %d will not (reason in Status)", bad) : wxString()));
  p_create->Enable(good > 0);
}


string BatchSmilesDialog::templateUrl() const
{
  const int k = p_template->GetSelection();
  return k >= 1 && k <= (int)p_templates.size() ? p_templates[k - 1].second : string();
}


bool BatchSmilesDialog::replace() const { return p_replace->GetValue(); }


string BatchSmilesDialog::describe() const
{
  const SmilesBatch::Columns c = columns();
  string s = "name=" + (c.name >= 0 ? p_table.header[c.name] : string("-")) +
             " smiles=" + (c.smiles >= 0 ? p_table.header[c.smiles] : string("-")) +
             " responses=";
  for (size_t i = 0; i < c.responses.size(); i++)
    s += (i ? "," : "") + p_table.header[c.responses[i]];
  int bad = 0;
  for (const auto& r : p_rows) bad += !r.problem.empty();
  s += " rows=" + std::to_string(p_rows.size()) + " problems=" + std::to_string(bad) +
       " setup=" + S(p_template->GetStringSelection());
  return s;
}
