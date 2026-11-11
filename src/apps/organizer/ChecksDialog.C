#include "ChecksDialog.H"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/settings.h>
#include <wx/sizer.h>

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/TaskJob.H"
#include "SiExport.H"

using std::string;
using std::vector;

// One CSV record, quotes undoubled; the script quotes only what needs it.
vector<vector<string>> parseCsv(const string& t)
{
  vector<vector<string>> out(1);
  string cur;
  bool q = false;
  for (size_t i = 0; i < t.size(); i++) {
    const char c = t[i];
    if (q) {
      if (c == '"' && i + 1 < t.size() && t[i+1] == '"') { cur += '"'; i++; }
      else if (c == '"') q = false;
      else cur += c;
    } else if (c == '"') q = true;
    else if (c == ',') { out.back().push_back(cur); cur.clear(); }
    else if (c == '\n') {
      out.back().push_back(cur); cur.clear();
      out.emplace_back();
    } else cur += c;
  }
  if (out.back().empty()) out.pop_back();
  return out;
}

namespace {

string quoteCsv(const string& v)
{
  if (v.find_first_of("\",\n") == string::npos) return v;
  string o = "\"";
  for (char c : v) { if (c == '"') o += '"'; o += c; }
  return o + "\"";
}

void collect(Resource *r, vector<EcceURL>& out)
{
  if (!r) return;
  if (dynamic_cast<TaskJob*>(r)) { out.push_back(r->getURL()); return; }
  if (vector<Resource*> *kids = r->getChildren(true))
    for (Resource *k : vector<Resource*>(*kids)) collect(k, out);
}

}  // namespace

vector<EcceURL> calculationsUnder(const vector<EcceURL>& roots)
{
  vector<EcceURL> out;
  for (const EcceURL& u : roots) collect(EDSIFactory::getResource(u), out);
  return out;
}

bool ChecksDialog::load(wxWindow *parent, bool withProgress)
{
  string text;
  // More than a few calculations take a while to gather: show n of N.
  std::unique_ptr<wxProgressDialog> pd;
  GatherProgress progress;
  if ((p_rank || p_qsar) && withProgress && p_calcs.size() > 3) {
    pd.reset(new wxProgressDialog(p_qsar ? "QSAR table" : "Compare Energies", "Starting...",
        (int)p_calcs.size(), parent, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
    progress = [&pd](size_t i, size_t n, const string& name) {
      return pd->Update((int)i, wxString::Format("Reading %zu of %zu: %s",
          i + 1, n, wxString::FromUTF8(name.c_str())));
    };
  }
  if (p_qsar) {
    if (!qsarTable(p_calcs, p_groups, text, p_why, progress)) return false;
  } else if (!(p_rank ? rankTable(p_calcs, "csv", text, p_why, progress, p_ref)
                      : checksTable(p_calcs, "csv", text, p_why))) return false;
  vector<vector<string>> all = parseCsv(text);
  if (all.empty()) { p_why = "The table is empty."; return false; }
  p_head = all[0];
  p_rows.assign(all.begin() + 1, all.end());
  return true;
}

bool ChecksDialog::setReference(const string& name, string& why)
{
  const string old = p_ref;
  p_ref = name;
  if (!load(this, true)) {
    why = p_why;
    p_ref = old;
    syncReference();
    return false;
  }
  syncReference();
  p_sortCol = -1;
  if (p_grid) fill();
  return true;
}

ChecksDialog::ChecksDialog(wxWindow *parent, const vector<EcceURL>& calcs, bool rank,
                           const string *qsarGroups)
  : wxDialog(parent, wxID_ANY, qsarGroups ? "QSAR table" : rank ? "Compare Energies" : "Checks table",
             wxDefaultPosition, wxSize(1100, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_calcs(calcs), p_rank(rank), p_qsar(qsarGroups != nullptr)
{
  if (qsarGroups) p_groups = *qsarGroups;
  if (!load(parent, true)) return;
  p_ok = true;

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  if (p_rank) {
    wxBoxSizer *refRow = new wxBoxSizer(wxHORIZONTAL);
    refRow->Add(new wxStaticText(this, wxID_ANY, "Reference:"), 0,
                wxALIGN_CENTER_VERTICAL | wxALL, 5);
    p_reference = new wxChoice(this, wxID_ANY);
    syncReference();
    refRow->Add(p_reference, 0, wxALL, 5);
    top->Add(refRow, 0, wxEXPAND);
    p_reference->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
      const int k = p_reference->GetSelection();
      const string name = k <= 0 ? string() :
          p_reference->GetString(k).ToStdString(wxConvUTF8);
      string why;
      if (!setReference(name, why) && why != "Cancelled.")
        wxMessageBox(wxString::FromUTF8(why.c_str()), "Compare Energies",
                     wxOK | wxICON_ERROR, this);
    });
  }
  p_grid = new wxGrid(this, wxID_ANY);
  p_grid->CreateGrid(0, (int)p_head.size());
  p_grid->EnableEditing(false);
  top->Add(p_grid, 1, wxEXPAND | wxALL, 5);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  wxButton *c = new wxButton(this, wxID_ANY, "Export CSV...");
  wxButton *l = new wxButton(this, wxID_ANY, "Export LaTeX...");
  row->Add(c, 0, wxALL, 5);
  if (p_qsar) l->Hide();
  else row->Add(l, 0, wxALL, 5);
  row->AddStretchSpacer();
  row->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALL, 5);
  top->Add(row, 0, wxEXPAND);
  SetSizer(top);
  c->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportAs("csv"); });
  l->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportAs("latex"); });
  FindWindow(wxID_CLOSE)->Bind(wxEVT_BUTTON,
      [this](wxCommandEvent&) { EndModal(wxID_CLOSE); });
  p_grid->Bind(wxEVT_GRID_LABEL_LEFT_CLICK, &ChecksDialog::onSort, this);
  fill();
}

vector<string> ChecksDialog::referenceChoices() const
{
  vector<string> names;
  if (p_reference)
    for (unsigned i = 0; i < p_reference->GetCount(); ++i)
      names.push_back(p_reference->GetString(i).ToStdString(wxConvUTF8));
  return names;
}

string ChecksDialog::selectedReference() const
{
  return p_reference ? p_reference->GetStringSelection().ToStdString(wxConvUTF8) : "";
}

void ChecksDialog::syncReference()
{
  if (!p_reference) return;
  p_reference->Clear();
  p_reference->Append("Lowest");
  const auto name = std::find(p_head.begin(), p_head.end(), "name");
  const auto energy = std::find(p_head.begin(), p_head.end(), "E_el");
  const auto excluded = std::find(p_head.begin(), p_head.end(), "excluded");
  if (name != p_head.end() && energy != p_head.end() && excluded != p_head.end())
    for (const auto& row : p_rows) {
      const string& e = row[energy - p_head.begin()];
      char *end;
      strtod(e.c_str(), &end);
      // Use the gathered rows so the chooser and exported table agree.
      if (!e.empty() && end != e.c_str() && !*end &&
          row[excluded - p_head.begin()].empty())
        p_reference->Append(wxString::FromUTF8(row[name - p_head.begin()].c_str()));
    }
  p_reference->SetSelection(p_ref.empty() ? 0 :
      p_reference->FindString(wxString::FromUTF8(p_ref.c_str()), true));
}

// Rows with a problem are red text on a light red ground; excluded rows grey.
void ChecksDialog::fill()
{
  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  p_grid->AppendRows((int)p_rows.size());
  int excl = -1, prob = -1;
  for (size_t j = 0; j < p_head.size(); j++) {
    p_grid->SetColLabelValue((int)j, wxString::FromUTF8(p_head[j].c_str()) +
        ((int)j == p_sortCol ? (p_ascending ? " ▲" : " ▼") : ""));
    if (p_head[j] == "excluded") excl = (int)j;
    if (p_head[j] == "problems" || p_head[j] == "flags") prob = (int)j;
  }
  for (size_t i = 0; i < p_rows.size(); i++) {
    const bool x = excl >= 0 && !p_rows[i][excl].empty();
    const bool bad = prob >= 0 && !p_rows[i][prob].empty();
    for (size_t j = 0; j < p_head.size() && j < p_rows[i].size(); j++) {
      p_grid->SetCellValue((int)i, (int)j, wxString::FromUTF8(p_rows[i][j].c_str()));
      if (x) p_grid->SetCellTextColour((int)i, (int)j,
                    wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
      else if (bad) {
        p_grid->SetCellBackgroundColour((int)i, (int)j, wxColour(255, 220, 220));
        p_grid->SetCellTextColour((int)i, (int)j, wxColour(150, 0, 0));
      }
    }
  }
  p_grid->AutoSizeColumns();
}

void ChecksDialog::onSort(wxGridEvent& e)
{
  const int col = e.GetCol();
  if (col < 0) return;
  p_ascending = col == p_sortCol ? !p_ascending : true;
  p_sortCol = col;
  // "-" (not stored) sorts last in either direction; numbers by value.
  std::stable_sort(p_rows.begin(), p_rows.end(),
      [&](const vector<string>& a, const vector<string>& b) {
        const string &x = a[col], &y = b[col];
        if ((x == "-") != (y == "-")) return y == "-";
        char *ex, *ey;
        const double dx = strtod(x.c_str(), &ex), dy = strtod(y.c_str(), &ey);
        const bool num = !x.empty() && !y.empty() && !*ex && !*ey;
        const bool less = num ? dx < dy : x < y;
        const bool more = num ? dx > dy : x > y;
        return p_ascending ? less : more;
      });
  fill();
}

string ChecksDialog::csv() const
{
  string o;
  auto line = [&o](const vector<string>& r) {
    for (size_t i = 0; i < r.size(); i++) o += (i ? "," : "") + quoteCsv(r[i]);
    o += "\n";
  };
  line(p_head);
  for (const auto& r : p_rows) line(r);
  return o;
}

void ChecksDialog::exportAs(const string& format)
{
  wxFileDialog dlg(this, p_qsar ? "Write the QSAR table to" :
                   p_rank ? "Write the Compare Energies table to" : "Write the Checks table to", "",
                   p_qsar ? "qsar.csv" :
                   format == "csv" ? (p_rank ? "rank.csv" : "checks.csv") : (p_rank ? "rank.tex" : "checks.tex"), "*",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  string text, why;
  if (format == "csv") text = csv();
  else if (!(p_rank ? rankTable(p_calcs, format, text, why, GatherProgress(), p_ref)
                    : checksTable(p_calcs, format, text, why))) {
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Checks table", wxOK | wxICON_ERROR, this);
    return;
  }
  std::ofstream(dlg.GetPath().ToStdString()) << text;
}
