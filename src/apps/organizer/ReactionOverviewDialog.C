#include "ReactionOverviewDialog.H"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "XYPlotCanvas.H"
#include "util/ThermoRun.H"

using std::string;
using std::vector;

namespace {

// One CSV line; fields may be double-quoted with "" for a quote.
vector<string> csvFields(const string& line)
{
  vector<string> f;
  string cur;
  bool q = false;
  for (size_t i = 0; i < line.size(); i++) {
    const char c = line[i];
    if (q) {
      if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; i++; }
      else if (c == '"') q = false;
      else cur += c;
    } else if (c == '"') q = true;
    else if (c == ',') { f.push_back(cur); cur.clear(); }
    else cur += c;
  }
  f.push_back(cur);
  return f;
}

string num(double v, const char *fmt = "%.3f")
{
  char b[64];
  snprintf(b, sizeof b, fmt, v);
  return b;
}

}

ReactionOverviewDialog::ReactionOverviewDialog(wxWindow *parent, const string& lines)
  : wxDialog(parent, wxID_ANY, _("Reaction overview"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(this, wxID_ANY,
      _("Reactions, one per line: reactants -> products (calculation URLs, N* for a coefficient)")),
      0, wxALL, 8);
  p_input = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(lines.c_str()),
                           wxDefaultPosition, wxSize(720, 90), wxTE_MULTILINE | wxTE_DONTWRAP);
  top->Add(p_input, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(this, wxID_ANY, _("G treatment:")), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_treatment = new wxChoice(this, wxID_ANY);
  for (const ThermoTreatmentInfo& t : thermoTreatments())
    p_treatment->Append(wxString::FromUTF8(t.label));
  p_treatment->SetSelection(2);   // Grimme qRRHO, 100 cm-1
  p_treatment->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { update(); });
  bar->Add(p_treatment, 0, wxRIGHT, 12);
  wxButton *run = new wxButton(this, wxID_ANY, _("Update"));
  run->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { update(); });
  bar->Add(run, 0);
  top->Add(bar, 0, wxALL, 8);

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(720, 300));
  top->Add(p_plot, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
  p_detail = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(720, 150),
                            wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  top->Add(p_detail, 1, wxEXPAND | wxALL, 8);

  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *csv = new wxButton(this, wxID_ANY, _("Export CSV..."));
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog d(this, _("Export chart data"), "", "reactions.csv", "*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (d.ShowModal() == wxID_OK) saveCsv(string(d.GetPath().ToUTF8()));
  });
  wxButton *png = new wxButton(this, wxID_ANY, _("Export PNG..."));
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog d(this, _("Export chart"), "", "reactions.png", "*.png",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (d.ShowModal() == wxID_OK) savePng(string(d.GetPath().ToUTF8()));
  });
  buttons->Add(csv, 0, wxRIGHT, 8);
  buttons->Add(png, 0);
  buttons->AddStretchSpacer();
  buttons->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  top->Add(buttons, 0, wxEXPAND | wxALL, 8);
  SetSizerAndFit(top);
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (IsModal()) EndModal(wxID_CLOSE); else Hide();
  }, wxID_CLOSE);
  p_plot->setPointHandler([this](int, int idx) {
    if (idx >= 0 && idx < (int)p_rowOfPoint.size()) showReaction(p_rowOfPoint[idx] + 1);
  });
  update();
}

void ReactionOverviewDialog::setTreatment(const string& cli)
{
  const vector<ThermoTreatmentInfo>& t = thermoTreatments();
  for (size_t i = 0; i < t.size(); i++)
    if (cli == t[i].cli) p_treatment->SetSelection((int)i);
  update();
}

void ReactionOverviewDialog::update()
{
  p_rows.clear();
  p_rowOfPoint.clear();
  p_plot->clearCurves();
  p_csv.clear();
  const string tmp = string(wxFileName::CreateTempFileName("reactions").ToUTF8());
  { std::ofstream(tmp.c_str()) << string(p_input->GetValue().ToUTF8()) << "\n"; }
  const int sel = p_treatment->GetSelection();
  string csv, err;
  const bool ok = runThermoReactions(tmp, thermoTreatments()[sel < 0 ? 2 : sel].cli, csv, err);
  std::remove(tmp.c_str());
  if (!ok) {
    p_error = err;
    p_detail->SetValue(wxString::FromUTF8(err.c_str()));
    return;
  }
  p_error.clear();
  const size_t split = csv.find("\n\n");
  p_csv = csv.substr(0, split);
  std::istringstream in(csv);
  string line;
  bool modes = false;
  std::getline(in, line);   // header
  while (std::getline(in, line)) {
    if (line.empty()) { modes = true; std::getline(in, line); continue; }
    const vector<string> f = csvFields(line);
    if (!modes && f.size() >= 8) {
      Row r;
      r.text = f[0]; r.status = f[1]; r.reason = f[7];
      if (r.status == "ok") {
        r.dE = atof(f[2].c_str()); r.dG = atof(f[3].c_str()); r.spread = atof(f[4].c_str());
        r.lo = atof(f[5].c_str()); r.hi = atof(f[6].c_str());
      }
      p_rows.push_back(r);
    } else if (modes && f.size() >= 7) {
      for (Row& r : p_rows)
        if (r.text == f[0] && r.status == "ok") {
          r.modes.push_back({f[1], atof(f[2].c_str()), atof(f[3].c_str()), atof(f[4].c_str()),
                             atof(f[5].c_str()), atof(f[6].c_str())});
          break;
        }
    }
  }
  vector<double> x0, e, x1, g, lo, hi;
  string refused;
  for (size_t i = 0; i < p_rows.size(); i++) {
    const Row& r = p_rows[i];
    if (r.status != "ok") {
      refused += wxString::Format("%d: refused: %s\n", (int)i + 1, r.reason.c_str()).ToStdString();
      continue;
    }
    p_rowOfPoint.push_back((int)i);
    x0.push_back(i + 0.85); e.push_back(r.dE);
    x1.push_back(i + 1.15); g.push_back(r.dG);
    lo.push_back(r.lo); hi.push_back(r.hi);
  }
  const int ce = p_plot->addCurve(x0, e, _("dE (electronic)"), 0, XYPlotCanvas::MARKERS);
  const int cg = p_plot->addCurve(x1, g, _("dG (bar: spread over treatments)"), 1,
                                  XYPlotCanvas::MARKERS);
  p_plot->setCurveMarker(ce, 1);
  p_plot->setCurveErrorBars(cg, lo, hi);
  p_plot->setShowKey(true);
  p_plot->setIntegerX(true);
  p_plot->setXAxisLabel(_("Reaction (numbered as in the list)"));
  p_plot->setYAxisLabel(_("kJ/mol"));
  string d;
  for (size_t i = 0; i < p_rows.size(); i++)
    d += std::to_string(i + 1) + ": " + p_rows[i].text + "\n";
  p_detail->SetValue(wxString::FromUTF8((d + refused + "\nClick a point to list the soft modes.").c_str()));
}

string ReactionOverviewDialog::chartText() const
{
  string out = p_error.empty() ? string() : "error: " + p_error + "\n";
  const size_t n = p_rowOfPoint.size();
  for (size_t k = 0; k < n; k++) {
    const Row& r = p_rows[p_rowOfPoint[k]];
    const int idx = p_rowOfPoint[k] + 1;
    out += "0 " + std::to_string(k) + " " + std::to_string(idx) + " " + num(idx - 0.15) + " " +
           num(r.dE) + " - -\n";
    out += "1 " + std::to_string(k) + " " + std::to_string(idx) + " " + num(idx + 0.15) + " " +
           num(r.dG) + " " + num(r.lo) + " " + num(r.hi) + "\n";
  }
  return out;
}

void ReactionOverviewDialog::showReaction(int n)
{
  string d;
  if (n < 1 || n > (int)p_rows.size()) return;
  const Row& r = p_rows[n - 1];
  d += std::to_string(n) + ": " + r.text + "\n";
  d += "dE_el " + num(r.dE, "%.2f") + "   dG " + num(r.dG, "%.2f") + "   spread " +
       num(r.spread, "%.2f") + " kJ/mol\n";
  d += "Soft modes (< 100 cm-1), share of the species' S and of its G correction:\n";
  if (r.modes.empty()) d += "  none\n";
  for (const Mode& m : r.modes)
    d += "  " + m.species + "  " + num(m.freq, "%.1f") + " cm-1  S " + num(m.S) + " J/mol/K (" +
         num(100 * m.Sshare, "%.1f") + "%)  G " + num(m.G) + " kJ/mol (" +
         num(100 * m.Gshare, "%.1f") + "%)\n";
  p_detail->SetValue(wxString::FromUTF8(d.c_str()));
}

string ReactionOverviewDialog::clickText(int n)
{
  showReaction(n);
  return string(p_detail->GetValue().ToUTF8());
}

bool ReactionOverviewDialog::savePng(const string& file) const
{
  const wxSize size(900, 480);
  wxBitmap bmp(size.x, size.y);
  wxMemoryDC dc(bmp);
  p_plot->paintOnto(dc, size);
  dc.SelectObject(wxNullBitmap);
  return bmp.SaveFile(wxString::FromUTF8(file.c_str()), wxBITMAP_TYPE_PNG);
}

bool ReactionOverviewDialog::saveCsv(const string& file) const
{
  std::ofstream f(file.c_str());
  if (!f) return false;
  f << p_csv << "\n";
  return (bool)f;
}
