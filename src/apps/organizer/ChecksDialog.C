#include "ChecksDialog.H"

#include <algorithm>
#include <cstdlib>
#include <fstream>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/sizer.h>

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/TaskJob.H"
#include "SiExport.H"

using std::string;
using std::vector;

namespace {

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

ChecksDialog::ChecksDialog(wxWindow *parent, const vector<EcceURL>& calcs, bool rank)
  : wxDialog(parent, wxID_ANY, rank ? "Rank" : "Checks table", wxDefaultPosition,
             wxSize(1100, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_calcs(calcs), p_rank(rank)
{
  string text;
  if (!(p_rank ? rankTable(p_calcs, "csv", text, p_why)
                : checksTable(p_calcs, "csv", text, p_why))) return;
  vector<vector<string>> all = parseCsv(text);
  if (all.empty()) { p_why = "The table is empty."; return; }
  p_head = all[0];
  p_rows.assign(all.begin() + 1, all.end());
  p_ok = true;

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  p_grid = new wxGrid(this, wxID_ANY);
  p_grid->CreateGrid(0, (int)p_head.size());
  p_grid->EnableEditing(false);
  top->Add(p_grid, 1, wxEXPAND | wxALL, 5);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  wxButton *c = new wxButton(this, wxID_ANY, "Export CSV...");
  wxButton *l = new wxButton(this, wxID_ANY, "Export LaTeX...");
  row->Add(c, 0, wxALL, 5);
  row->Add(l, 0, wxALL, 5);
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
  wxFileDialog dlg(this, p_rank ? "Write the Rank table to" : "Write the Checks table to", "",
                   format == "csv" ? (p_rank ? "rank.csv" : "checks.csv") : (p_rank ? "rank.tex" : "checks.tex"), "*",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  string text, why;
  if (format == "csv") text = csv();
  else if (!(p_rank ? rankTable(p_calcs, format, text, why)
                    : checksTable(p_calcs, format, text, why))) {
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Checks table", wxOK | wxICON_ERROR, this);
    return;
  }
  std::ofstream(dlg.GetPath().ToStdString()) << text;
}
