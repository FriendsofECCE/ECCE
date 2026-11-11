#include "QsarDialog.H"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dcclient.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "ChecksDialog.H"
#include "SiExport.H"
#include "util/Ecce.H"
#include "util/Filesystem.H"

using std::string;
using std::vector;
namespace fs = ecce::fs;

namespace {
string quote(const string& s)
{
  string q;
  for (char c : s) if (c != '\'' && c != '"') q += c;
  return "'" + q + "'";
}

string tool()
{
  const char *home = getenv("ECCE_HOME");
  return home && *home ? string(home) + "/bin/ecce-qsar" : string("ecce-qsar");
}
}  // namespace


// Scores t1 against t2 with the names beside the points, and the R2X of each
// component as bars below.
class QsarPlot : public wxPanel
{
public:
  QsarPlot(wxWindow *parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(460, 420))
  {
    Bind(wxEVT_PAINT, &QsarPlot::OnPaint, this);
  }
  void set(const vector<string>& names, const vector<double>& x, const vector<double>& y,
           const vector<double>& r2x)
  {
    p_names = names; p_x = x; p_y = y; p_r2 = r2x;
    Refresh();
  }

private:
  void OnPaint(wxPaintEvent&)
  {
    wxPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    wxSize sz = GetClientSize();
    const int m = 36, barH = 70;
    const int w = sz.x - 2 * m, h = sz.y - 2 * m - barH;
    if (p_x.empty() || w < 20 || h < 20) return;
    double x0 = *std::min_element(p_x.begin(), p_x.end()), x1 = *std::max_element(p_x.begin(), p_x.end());
    double y0 = p_y.empty() ? -1 : *std::min_element(p_y.begin(), p_y.end());
    double y1 = p_y.empty() ? 1 : *std::max_element(p_y.begin(), p_y.end());
    if (x1 - x0 < 1e-12) { x0 -= 1; x1 += 1; }
    if (y1 - y0 < 1e-12) { y0 -= 1; y1 += 1; }
    const double px = 0.05 * (x1 - x0), py = 0.05 * (y1 - y0);
    x0 -= px; x1 += px; y0 -= py; y1 += py;
    auto X = [&](double v) { return m + (int)((v - x0) / (x1 - x0) * w); };
    auto Y = [&](double v) { return m + h - (int)((v - y0) / (y1 - y0) * h); };
    dc.SetPen(*wxLIGHT_GREY_PEN);
    dc.DrawRectangle(m, m, w, h);
    if (x0 < 0 && x1 > 0) dc.DrawLine(X(0), m, X(0), m + h);
    if (y0 < 0 && y1 > 0) dc.DrawLine(m, Y(0), m + w, Y(0));
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(wxBrush(wxColour(30, 90, 170)));
    dc.SetFont(*wxSMALL_FONT);
    for (size_t i = 0; i < p_x.size() && i < p_y.size(); i++) {
      dc.DrawCircle(X(p_x[i]), Y(p_y[i]), 3);
      if (p_x.size() <= 60)
        dc.DrawText(wxString::FromUTF8(p_names[i].c_str()), X(p_x[i]) + 4, Y(p_y[i]) - 6);
    }
    dc.DrawText("t1", m + w - 12, m + h + 2);
    dc.DrawText("t2", 4, m - 2);
    const int by = sz.y - barH + 8;
    dc.DrawText("R2X per component", m, by - 8);
    const size_t nb = p_r2.size();
    if (nb) {
      const int bw = std::max(8, std::min(30, w / (int)nb - 4));
      for (size_t i = 0; i < nb; i++) {
        int bh = (int)(p_r2[i] * (barH - 30));
        dc.SetBrush(wxBrush(wxColour(150, 150, 150)));
        dc.DrawRectangle(m + (int)i * (bw + 4), by + barH - 30 - bh, bw, bh);
      }
    }
  }
  vector<string> p_names;
  vector<double> p_x, p_y, p_r2;
};


string QsarDialog::run(const string& args, int& status) const
{
  string out;
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  string pyExe = fs::exists(py) ? py.string() : string("python.exe");
  status = Ecce::readCommand("\"" + pyExe + "\" \"" + tool() + "\" " + args, out);
#else
  status = Ecce::readCommand(quote(tool()) + " " + args + " 2>&1", out);
#endif
  return out;
}

QsarDialog::QsarDialog(wxWindow *parent, const vector<EcceURL>& calcs)
  : wxDialog(parent, wxID_ANY, "QSAR Analysis", wxDefaultPosition, wxSize(1000, 560),
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_calcs(calcs)
{
  static int serial = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-qsar-" +
      std::to_string((long)fs::file_time_type::clock::now().time_since_epoch().count()) +
      "-" + std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);

  string text;
  std::unique_ptr<wxProgressDialog> pd;
  GatherProgress progress;
  if (calcs.size() > 3) {
    pd.reset(new wxProgressDialog("QSAR Analysis", "Starting...", (int)calcs.size(), parent,
        wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
    progress = [&pd](size_t i, size_t n, const string& name) {
      return pd->Update((int)i, wxString::Format("Reading %zu of %zu: %s", i + 1, n,
                                                 wxString::FromUTF8(name.c_str())));
    };
  }
  if (!qsarTable(calcs, "counts,topology,models,charges,vsa,qm", text, p_why, progress)) return;
  p_table = (fs::path(p_dir) / "table.csv").string();
  std::ofstream(p_table) << text;
  int st = 0;
  const string cols = run("columns " + quote(p_table), st);
  if (st == 0) {
    std::istringstream is(cols);
    for (string l; std::getline(is, l); ) if (!l.empty()) p_measured.push_back(l);
  }
  p_ok = true;

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(new wxStaticText(this, wxID_ANY, "Scaling:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  p_scale = new wxChoice(this, wxID_ANY);
  for (const char *s : {"uv", "pareto", "ctr", "none"}) p_scale->Append(s);
  p_scale->SetSelection(0);
  row->Add(p_scale, 0, wxALL, 5);
  row->Add(new wxStaticText(this, wxID_ANY, "Components (0 = by cross-validation):"), 0,
           wxALIGN_CENTER_VERTICAL | wxALL, 5);
  p_comps = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 20, 0);
  row->Add(p_comps, 0, wxALL, 5);
  wxButton *run = new wxButton(this, wxID_ANY, "Run PCA");
  row->Add(run, 0, wxALL, 5);
  wxButton *save = new wxButton(this, wxID_ANY, "Save Results...");
  row->Add(save, 0, wxALL, 5);
  top->Add(row, 0, wxEXPAND);
  wxBoxSizer *mid = new wxBoxSizer(wxHORIZONTAL);
  p_text = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(500, 380),
                          wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_text->SetFont(wxFont(wxFontInfo(9).Family(wxFONTFAMILY_TELETYPE)));
  mid->Add(p_text, 1, wxEXPAND | wxALL, 5);
  p_plot = new QsarPlot(this);
  mid->Add(p_plot, 1, wxEXPAND | wxALL, 5);
  top->Add(mid, 1, wxEXPAND);
  top->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALIGN_RIGHT | wxALL, 5);
  SetSizer(top);
  run->Bind(wxEVT_BUTTON, &QsarDialog::OnRun, this);
  save->Bind(wxEVT_BUTTON, &QsarDialog::OnSave, this);
  FindWindow(wxID_CLOSE)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); });
}

QsarDialog::~QsarDialog()
{
  std::error_code ec;
  if (!p_dir.empty()) fs::remove_all(p_dir, ec);
}

bool QsarDialog::runPca(const string& scaling, int components, string& why)
{
  if (!p_ok) { why = p_why; return false; }
  string args = "pca " + quote(p_table) + " --scale " + scaling +
                " --outdir " + quote(p_dir);
  if (components > 0) args += " --components " + std::to_string(components);
  for (const string& y : p_measured) args += " --y " + quote(y);
  int st = 0;
  p_summary = run(args, st);
  if (p_text) p_text->SetValue(wxString::FromUTF8(p_summary.c_str()));
  if (st != 0) {
    const size_t e = p_summary.find("error: ");
    why = e == string::npos ? "ecce-qsar pca failed" :
        p_summary.substr(e + 7, p_summary.find('\n', e) - e - 7);
    return false;
  }
  vector<vector<string>> sc, comp;
  { std::ifstream f(fs::path(p_dir) / "pca_scores.csv");
    sc = parseCsv(string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>())); }
  { std::ifstream f(fs::path(p_dir) / "pca_components.csv");
    comp = parseCsv(string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>())); }
  vector<string> names;
  vector<double> x, y, r2;
  for (size_t i = 1; i < sc.size(); i++) {
    if (sc[i].size() < 3) continue;
    names.push_back(sc[i][0]);
    x.push_back(atof(sc[i][1].c_str()));
    y.push_back(atof(sc[i][2].c_str()));
  }
  for (size_t i = 1; i < comp.size(); i++)
    if (comp[i].size() > 2) r2.push_back(atof(comp[i][2].c_str()));
  if (p_plot) p_plot->set(names, x, y, r2);
  return true;
}

bool QsarDialog::saveResults(const string& dir, string& why) const
{
  std::error_code ec;
  fs::create_directories(dir, ec);
  for (const char *f : {"table.csv", "pca_scores.csv", "pca_loadings.csv", "pca_components.csv"}) {
    const fs::path src = fs::path(p_dir) / f;
    if (!fs::exists(src)) continue;
    fs::copy_file(src, fs::path(dir) / f, fs::copy_options::overwrite_existing, ec);
    if (ec) { why = string("cannot write ") + f + ": " + ec.message(); return false; }
  }
  return true;
}

void QsarDialog::OnRun(wxCommandEvent&)
{
  string why;
  if (!runPca(p_scale->GetStringSelection().ToStdString(), p_comps->GetValue(), why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "QSAR Analysis", wxOK | wxICON_ERROR, this);
}

void QsarDialog::OnSave(wxCommandEvent&)
{
  wxDirDialog dlg(this, "Write the table and the results to");
  if (dlg.ShowModal() != wxID_OK) return;
  string why;
  if (!saveResults(dlg.GetPath().ToStdString(wxConvUTF8), why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "QSAR Analysis", wxOK | wxICON_ERROR, this);
}
