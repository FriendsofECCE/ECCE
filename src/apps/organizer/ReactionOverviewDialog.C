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
#include "RateBox.H"
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

string trim(const string& s)
{
  const size_t b = s.find_first_not_of(" \t\r");
  return b == string::npos ? string() : s.substr(b, s.find_last_not_of(" \t\r") - b + 1);
}

string csvq(const string& s)
{
  if (s.find_first_of(",\"\n") == string::npos) return s;
  string o = "\"";
  for (char c : s) { if (c == '"') o += '"'; o += c; }
  return o + "\"";
}

// One side of a reaction as the calculation folders ecce-rate takes: no
// coefficients, each species once.  Empty with `why` set when it cannot.
vector<string> sidePaths(const string& side, string& why)
{
  vector<string> out;
  std::istringstream in(side);
  for (string item; std::getline(in, item, ','); ) {
    item = trim(item);
    if (item.empty()) continue;
    if (item.find('*') != string::npos) {
      why = "a reaction with a transition state takes each species once (no N* coefficient)";
      return vector<string>();
    }
    const string p = rateLocalPath(item);
    if (p.empty()) {
      why = "rate constants need stored local calculations (a file:// URL or a path): " + item;
      return vector<string>();
    }
    out.push_back(p);
  }
  return out;
}

// Reaction `a, b -> [ts] -> c` split into its parts; false when the line has
// brackets but not that form.
bool splitTs(const string& line, string& lhs, string& ts, string& rhs)
{
  const size_t a1 = line.find("->");
  const size_t a2 = a1 == string::npos ? string::npos : line.find("->", a1 + 2);
  if (a2 == string::npos || line.find("->", a2 + 2) != string::npos) return false;
  const string mid = trim(line.substr(a1 + 2, a2 - a1 - 2));
  if (mid.size() < 3 || mid[0] != '[' || mid.back() != ']') return false;
  lhs = trim(line.substr(0, a1));
  ts = trim(mid.substr(1, mid.size() - 2));
  rhs = trim(line.substr(a2 + 2));
  return !lhs.empty() && !ts.empty() && !rhs.empty();
}

}

ReactionOverviewDialog::ReactionOverviewDialog(wxWindow *parent, const string& lines)
  : wxDialog(parent, wxID_ANY, _("Reaction overview"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(this, wxID_ANY,
      _("Reactions, one per line: reactants -> products, or reactants -> [ts] -> products for a rate "
        "(calculation URLs, N* for a coefficient)")),
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
  bar->Add(new wxStaticText(this, wxID_ANY, _("T / K:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_T = new wxTextCtrl(this, wxID_ANY, "298.15", wxDefaultPosition, wxSize(80, -1),
                       wxTE_PROCESS_ENTER);
  p_T->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { update(); });
  bar->Add(p_T, 0, wxRIGHT, 12);
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

void ReactionOverviewDialog::setTemperature(const string& kelvin)
{
  p_T->SetValue(wxString::FromUTF8(kelvin.c_str()));
  update();
}

void ReactionOverviewDialog::update()
{
  p_rows.clear();
  p_rowOfPoint.clear();
  p_plot->clearCurves();
  p_csv.clear();
  p_anyTs = false;
  const int sel = p_treatment->GetSelection();
  const string treatment = thermoTreatments()[sel < 0 ? 2 : sel].cli;
  double kelvin = atof(string(p_T->GetValue().ToUTF8()).c_str());
  if (kelvin <= 0) kelvin = 298.15;
  const string tText = num(kelvin, "%.2f");

  // One entry per line.  A TS line goes to ecce-thermo twice: 'a -> c' for the
  // reaction and 'a -> ts' for the barrier, so both come from its numbers.
  struct Entry { string user, main, aux; int mainIdx = -1, auxIdx = -1; string refused; vector<string> re, ts, pr; };
  vector<Entry> entries;
  std::istringstream lines(string(p_input->GetValue().ToUTF8()));
  string thermoFile;
  int nth = 0;
  for (string raw; std::getline(lines, raw); ) {
    const string l = trim(raw);
    if (l.empty() || l[0] == '#') continue;
    Entry e;
    e.user = l;
    if (l.find('[') == string::npos) {
      e.main = l;
    } else {
      string lhs, ts, rhs;
      if (!splitTs(l, lhs, ts, rhs)) {
        e.refused = "cannot parse; the form is 'a, 2*b -> [ts] -> c'";
      } else {
        string why;
        e.re = sidePaths(lhs, why);
        e.ts = sidePaths(ts, why);
        e.pr = sidePaths(rhs, why);
        if (!why.empty()) e.refused = why;
        else if (e.ts.size() != 1) e.refused = "name one transition state in [ ]";
        e.main = lhs + " -> " + rhs;
        e.aux = lhs + " -> " + ts;
      }
    }
    if (e.refused.empty()) {
      e.mainIdx = nth++;
      thermoFile += e.main + "\n";
      if (!e.aux.empty()) { e.auxIdx = nth++; thermoFile += e.aux + "\n"; }
    }
    entries.push_back(e);
  }
  const string tmp = string(wxFileName::CreateTempFileName("reactions").ToUTF8());
  { std::ofstream(tmp.c_str()) << thermoFile; }
  string csv, err;
  const bool ok = thermoFile.empty() ? true : runThermoReactions(tmp, treatment, csv, err, kelvin);
  std::remove(tmp.c_str());
  if (!ok) {
    p_error = err;
    p_detail->SetValue(wxString::FromUTF8(err.c_str()));
    return;
  }
  p_error.clear();

  vector<Row> thermo;
  vector<string> rawLines;
  string header;
  {
    std::istringstream in(csv);
    string line;
    std::getline(in, header);
    bool modes = false;
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
        thermo.push_back(r);
        rawLines.push_back(line);
      } else if (modes && f.size() >= 7) {
        for (Row& r : thermo)
          if (r.text == f[0] && r.status == "ok") {
            r.modes.push_back({f[1], atof(f[2].c_str()), atof(f[3].c_str()), atof(f[4].c_str()),
                               atof(f[5].c_str()), atof(f[6].c_str())});
            break;
          }
      }
    }
  }
  for (const Entry& e : entries) {
    Row r;
    if (e.mainIdx >= 0 && e.mainIdx < (int)thermo.size()) r = thermo[e.mainIdx];
    else { r.status = "refused"; r.reason = e.refused.empty() ? "no result" : e.refused; }
    r.thermoText = r.text;
    r.text = e.user;
    if (e.mainIdx >= 0 && e.mainIdx < (int)rawLines.size()) r.rawCsv = rawLines[e.mainIdx];
    if (!e.aux.empty() && r.status == "ok") {
      p_anyTs = true;
      r.hasTs = true;
      const Row *a = e.auxIdx < (int)thermo.size() ? &thermo[e.auxIdx] : 0;
      if (!a || a->status != "ok") {
        r.rateError = "barrier: " + (a ? a->reason : string("no result"));
      } else {
        r.dEact = a->dE;
        r.dGact = a->dG;
        RateRequest req;
        req.reactants = e.re; req.ts = e.ts[0]; req.products = e.pr;
        req.temperatures = tText;
        req.treatment = treatment;
        r.rate = runRate(req);
        if (!r.rate.ok) r.rateError = r.rate.error;
      }
    }
    p_rows.push_back(r);
  }
  // The exported table: ecce-thermo's rows, as before, plus rate columns when a reaction has a TS.
  string table = header;
  if (p_anyTs)
    table += ",dE_act_kJ_mol,dG_act_kJ_mol,T_K,kappa_wigner,kappa_eckart,k_tst,k_wigner,k_eckart,k_unit";
  for (const Row& r : p_rows) {
    string line = r.rawCsv;
    if (line.empty()) line = csvq(r.text) + ",refused,,,,,," + csvq(r.reason);
    else if (r.text != r.thermoText) line = csvq(r.text) + line.substr(csvq(r.thermoText).size());
    if (p_anyTs) {
      if (r.hasTs && r.rate.ok) {
        const RateRow& w = r.rate.rows[0];
        line += "," + num(r.dEact) + "," + num(r.dGact) + "," + num(w.T, "%.2f") + "," +
                num(w.kappaWigner) + "," + (w.kappaEckart != w.kappaEckart ? string() : num(w.kappaEckart)) + "," +
                num(w.k, "%.4e") + "," + num(w.kWigner, "%.4e") + "," +
                (w.kEckart != w.kEckart ? string() : num(w.kEckart, "%.4e")) + "," + csvq(r.rate.unit);
      } else if (r.hasTs) {
        line += "," + num(r.dEact) + "," + num(r.dGact) + "," + num(kelvin, "%.2f") + ",,,,,," + csvq(r.rateError);
      } else line += ",,,,,,,,,";
    }
    table += "\n" + line;
  }
  p_csv = table;

  vector<double> x0, e, x1, g, lo, hi, xe, ye, xg, yg;
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
    if (r.hasTs && r.rateError.compare(0, 8, "barrier:") != 0) {
      xe.push_back(i + 0.7); ye.push_back(r.dEact);
      xg.push_back(i + 1.3); yg.push_back(r.dGact);
    }
  }
  const int ce = p_plot->addCurve(x0, e, _("dE (electronic)"), 0, XYPlotCanvas::MARKERS);
  const int cg = p_plot->addCurve(x1, g, _("dG (bar: spread over treatments)"), 1,
                                  XYPlotCanvas::MARKERS);
  p_plot->setCurveMarker(ce, 1);
  p_plot->setCurveErrorBars(cg, lo, hi);
  if (p_anyTs) {
    const int cte = p_plot->addCurve(xe, ye, _("dE\xE2\x80\xA1 (barrier, electronic)"), 2, XYPlotCanvas::MARKERS);
    p_plot->addCurve(xg, yg, _("dG\xE2\x80\xA1 (barrier)"), 3, XYPlotCanvas::MARKERS);
    p_plot->setCurveMarker(cte, 1);
  }
  p_plot->setShowKey(true);
  p_plot->setIntegerX(true);
  p_plot->setXAxisLabel(_("Reaction (numbered as in the list)"));
  p_plot->setYAxisLabel(_("kJ/mol"));
  string d;
  for (size_t i = 0; i < p_rows.size(); i++)
    d += std::to_string(i + 1) + ": " + p_rows[i].text + "\n";
  d += refused;
  if (p_anyTs) d += "\nRate constants at " + tText + " K (" + treatment + ", 1 M)\n" + rateText();
  p_detail->SetValue(wxString::FromUTF8((d + "\nClick a point to list the soft modes.").c_str()));
}

string ReactionOverviewDialog::rateLines(int n) const
{
  const Row& r = p_rows[n - 1];
  if (!r.hasTs) return string();
  const string head = std::to_string(n) + ": ";
  if (!r.rateError.empty()) return head + "no rate: " + r.rateError + "\n";
  const RateRow& w = r.rate.rows[0];
  const string u = r.rate.unit.empty() ? string() : " " + r.rate.unit;
  string o = head + "dE\xE2\x80\xA1 " + num(r.dEact, "%.2f") + "  dG\xE2\x80\xA1 " + num(r.dGact, "%.2f") +
             " kJ/mol  T " + num(w.T, "%.2f") + " K\n";
  o += "   k TST " + rateNum(w.k) + u + "\n";
  o += "   kappa Wigner " + num(w.kappaWigner) + "  k " + rateNum(w.kWigner) + u + "\n";
  o += "   kappa Eckart " + (w.kappaEckart != w.kappaEckart ? string("-") : num(w.kappaEckart)) +
       "  k " + rateNum(w.kEckart) + u + "\n";
  for (const string& note : r.rate.notes) o += "   note: " + note + "\n";
  return o;
}

string ReactionOverviewDialog::rateText() const
{
  string o;
  for (size_t i = 0; i < p_rows.size(); i++)
    if (p_rows[i].hasTs) o += rateLines((int)i + 1);
  return o;
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
  for (size_t k = 0; k < n; k++) {
    const Row& r = p_rows[p_rowOfPoint[k]];
    const int idx = p_rowOfPoint[k] + 1;
    if (!r.hasTs || r.rateError.compare(0, 8, "barrier:") == 0) continue;
    out += "2 " + std::to_string(k) + " " + std::to_string(idx) + " " + num(idx - 0.3) + " " +
           num(r.dEact) + " - -\n";
    out += "3 " + std::to_string(k) + " " + std::to_string(idx) + " " + num(idx + 0.3) + " " +
           num(r.dGact) + " - -\n";
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
  d += rateLines(n);
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
