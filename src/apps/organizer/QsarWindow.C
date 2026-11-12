#include "QsarWindow.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/graphics.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/settings.h>

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

wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }

string slurp(const fs::path& p)
{
  std::ifstream f(p);
  return string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

string scalingToken(int sel)
{
  static const char *const t[] = {"uv", "pareto", "ctr"};
  return t[sel < 0 || sel > 2 ? 0 : sel];
}

const char *const SCALING_TIPS[] = {
  "Autoscale: each descriptor is centred and divided by its standard deviation, so every "
  "descriptor counts equally whatever its unit.",
  "Pareto: centred and divided by the square root of the standard deviation; large "
  "descriptors count somewhat more than small ones.",
  "Centre only: the mean is subtracted but nothing is divided; descriptors with large "
  "numbers dominate."};

// "0.123" for a number, "-" for nothing.
string num3(const string& s)
{
  char *end;
  const double v = strtod(s.c_str(), &end);
  if (s.empty() || end == s.c_str()) return "-";
  char b[32];
  snprintf(b, sizeof b, "%.3f", v);
  return b;
}

string percent(double fraction)
{
  char b[32];
  snprintf(b, sizeof b, "%.0f", fraction * 100.0);
  return b;
}

// "bp (degC)" -> unit "°C"; header text with the degree sign set.
string niceHeader(string h)
{
  const size_t at = h.find("degC");
  if (at != string::npos) h.replace(at, 4, "\xC2\xB0" "C");
  return h;
}

// One non-overlapping-label scatter plot with axes, ticks and a hover readout.
const wxColour BLUE(30, 90, 170), RED(200, 50, 40), ORANGE(230, 140, 20), INK(40, 40, 40),
               GRIDC(225, 225, 225);

wxFont plotFont(double size = 1.0, bool bold = false)
{
  wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  f.SetPointSize((int)std::lround(f.GetPointSize() * size));
  if (bold) f.SetWeight(wxFONTWEIGHT_BOLD);
  return f;
}

double niceStep(double range, int want)
{
  const double raw = range / std::max(1, want);
  const double mag = std::pow(10.0, std::floor(std::log10(raw)));
  const double f = raw / mag;
  return mag * (f < 1.5 ? 1 : f < 3.5 ? 2 : f < 7.5 ? 5 : 10);
}

string tickText(double v, double step)
{
  char b[32];
  const int dec = step >= 1 ? 0 : (int)std::ceil(-std::log10(step));
  snprintf(b, sizeof b, "%.*f", std::min(dec, 6), std::fabs(v) < step * 1e-6 ? 0.0 : v);
  return b;
}
}  // namespace


class QsarPlot : public wxPanel
{
public:
  QsarPlot(wxWindow *parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(440, 420))
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &QsarPlot::OnPaint, this);
    Bind(wxEVT_MOTION, &QsarPlot::OnMove, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { p_hover = -1; Refresh(); });
  }

  /// flags: 0 normal, 1 outside the model limits, 2 kept out of the fit (test set).
  void set(const string& title, const string& xl, const string& yl, const vector<string>& names,
           const vector<double>& x, const vector<double>& y, const vector<int>& flag,
           bool parity)
  {
    p_title = title; p_xl = xl; p_yl = yl; p_names = names; p_x = x; p_y = y; p_flag = flag;
    p_parity = parity; p_hover = -1;
    Refresh();
  }

private:
  struct Frame { double x0, x1, y0, y1; int l, t, w, h; };

  Frame frame(wxGraphicsContext *gc) const
  {
    Frame f{};
    double x0 = *std::min_element(p_x.begin(), p_x.end()), x1 = *std::max_element(p_x.begin(), p_x.end());
    double y0 = *std::min_element(p_y.begin(), p_y.end()), y1 = *std::max_element(p_y.begin(), p_y.end());
    if (p_parity) { x0 = y0 = std::min(x0, y0); x1 = y1 = std::max(x1, y1); }
    if (x1 - x0 < 1e-12) { x0 -= 1; x1 += 1; }
    if (y1 - y0 < 1e-12) { y0 -= 1; y1 += 1; }
    const double px = 0.06 * (x1 - x0), py = 0.06 * (y1 - y0);
    f.x0 = x0 - px; f.x1 = x1 + px; f.y0 = y0 - py; f.y1 = y1 + py;
    const wxSize s = GetClientSize();
    f.l = 58; f.t = 34; f.w = std::max(20, s.x - f.l - 16); f.h = std::max(20, s.y - f.t - 70);
    (void)gc;
    return f;
  }
  double X(const Frame& f, double v) const { return f.l + (v - f.x0) / (f.x1 - f.x0) * f.w; }
  double Y(const Frame& f, double v) const { return f.t + f.h - (v - f.y0) / (f.y1 - f.y0) * f.h; }

  void text(wxGraphicsContext *gc, const string& s, double x, double y, const wxColour& c,
            double size = 1.0, bool bold = false, double angle = 0) const
  {
    gc->SetFont(plotFont(size, bold), c);
    if (angle == 0) gc->DrawText(U(s), x, y); else gc->DrawText(U(s), x, y, angle);
  }
  wxSize extent(wxGraphicsContext *gc, const string& s, double size = 1.0) const
  {
    gc->SetFont(plotFont(size), INK);
    double w, h;
    gc->GetTextExtent(U(s), &w, &h);
    return wxSize((int)std::ceil(w), (int)std::ceil(h));
  }

  void OnPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    text(gc.get(), p_title, 8, 6, INK, 1.1, true);
    if (p_x.empty() || p_x.size() != p_y.size()) return;
    const Frame f = frame(gc.get());
    // Grid and ticks.
    gc->SetPen(wxPen(GRIDC));
    const double sx = niceStep(f.x1 - f.x0, 6), sy = niceStep(f.y1 - f.y0, 6);
    for (double v = std::ceil(f.x0 / sx) * sx; v <= f.x1; v += sx) {
      gc->StrokeLine(X(f, v), f.t, X(f, v), f.t + f.h);
      const string t = tickText(v, sx);
      text(gc.get(), t, X(f, v) - extent(gc.get(), t).x / 2.0, f.t + f.h + 4, INK, 0.9);
    }
    for (double v = std::ceil(f.y0 / sy) * sy; v <= f.y1; v += sy) {
      gc->StrokeLine(f.l, Y(f, v), f.l + f.w, Y(f, v));
      const string t = tickText(v, sy);
      text(gc.get(), t, f.l - 6 - extent(gc.get(), t, 0.9).x, Y(f, v) - 8, INK, 0.9);
    }
    gc->SetPen(wxPen(wxColour(120, 120, 120)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(f.l, f.t, f.w, f.h);
    if (p_parity) {
      gc->SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_SHORT_DASH));
      gc->StrokeLine(X(f, f.x0), Y(f, f.x0), X(f, f.x1), Y(f, f.x1));
    } else {
      gc->SetPen(wxPen(wxColour(170, 170, 170)));
      if (f.x0 < 0 && f.x1 > 0) gc->StrokeLine(X(f, 0), f.t, X(f, 0), f.t + f.h);
      if (f.y0 < 0 && f.y1 > 0) gc->StrokeLine(f.l, Y(f, 0), f.l + f.w, Y(f, 0));
    }
    text(gc.get(), p_xl, f.l + f.w / 2.0 - extent(gc.get(), p_xl).x / 2.0, f.t + f.h + 24, INK);
    text(gc.get(), p_yl, 8, f.t + f.h / 2.0 + extent(gc.get(), p_yl).x / 2.0, INK, 1.0, false,
         M_PI / 2);
    // Points; the labels go in only where they do not overlap one already placed.
    vector<wxRect> placed;
    for (size_t i = 0; i < p_x.size(); i++) {
      const double px = X(f, p_x[i]), py = Y(f, p_y[i]);
      const int fl = p_flag[i];
      gc->SetPen(wxPen(fl == 1 ? ORANGE : fl == 2 ? RED : BLUE, fl == 1 ? 2 : 1));
      gc->SetBrush(fl == 1 ? *wxTRANSPARENT_BRUSH : wxBrush(fl == 2 ? RED : BLUE));
      if (fl == 1) gc->DrawRectangle(px - 4, py - 4, 8, 8);
      else gc->DrawEllipse(px - 3.5, py - 3.5, 7, 7);
      placed.push_back(wxRect((int)px - 4, (int)py - 4, 8, 8));
    }
    for (size_t i = 0; i < p_x.size(); i++) {
      const wxSize e = extent(gc.get(), p_names[i], 0.85);
      const double px = X(f, p_x[i]), py = Y(f, p_y[i]);
      const wxPoint cand[] = {{(int)px + 6, (int)py - e.y / 2}, {(int)px - 6 - e.x, (int)py - e.y / 2},
                              {(int)px - e.x / 2, (int)py - 6 - e.y}, {(int)px - e.x / 2, (int)py + 6}};
      for (const wxPoint& c : cand) {
        wxRect r(c.x, c.y, e.x, e.y);
        if (r.x < f.l || r.GetRight() > f.l + f.w || r.y < f.t || r.GetBottom() > f.t + f.h) continue;
        bool hit = false;
        for (const wxRect& q : placed) if (q.Intersects(r)) { hit = true; break; }
        if (hit) continue;
        placed.push_back(r);
        text(gc.get(), p_names[i], c.x, c.y, INK, 0.85);
        break;
      }
    }
    // Legend.
    int lx = f.l, ly = f.t + f.h + 44;
    auto key = [&](int fl, const string& what) {
      gc->SetPen(wxPen(fl == 1 ? ORANGE : fl == 2 ? RED : BLUE, fl == 1 ? 2 : 1));
      gc->SetBrush(fl == 1 ? *wxTRANSPARENT_BRUSH : wxBrush(fl == 2 ? RED : BLUE));
      if (fl == 1) gc->DrawRectangle(lx, ly + 4, 8, 8); else gc->DrawEllipse(lx, ly + 4, 8, 8);
      text(gc.get(), what, lx + 13, ly, INK, 0.9);
      lx += 13 + extent(gc.get(), what, 0.9).x + 14;
    };
    bool any1 = false, any2 = false;
    for (int fl : p_flag) { any1 |= fl == 1; any2 |= fl == 2; }
    key(0, p_parity ? "fitted molecules" : "molecules");
    if (any2) key(2, "test set (not in the fit)");
    if (any1) key(1, "outside the model limits (T2 or DModX)");
    // Hover readout.
    if (p_hover >= 0 && (size_t)p_hover < p_x.size()) {
      char b[160];
      snprintf(b, sizeof b, "%s: %.4g, %.4g", p_names[p_hover].c_str(), p_x[p_hover], p_y[p_hover]);
      const wxSize e = extent(gc.get(), b);
      double bx = std::min<double>(X(f, p_x[p_hover]) + 10, f.l + f.w - e.x - 8);
      double by = std::max<double>(f.t + 2, Y(f, p_y[p_hover]) - e.y - 10);
      gc->SetPen(wxPen(wxColour(90, 90, 90)));
      gc->SetBrush(wxBrush(wxColour(255, 255, 225)));
      gc->DrawRectangle(bx - 3, by - 2, e.x + 6, e.y + 4);
      text(gc.get(), b, bx, by, INK);
    }
  }

  void OnMove(wxMouseEvent& e)
  {
    int best = -1;
    if (!p_x.empty() && p_x.size() == p_y.size()) {
      const Frame f = frame(nullptr);
      double bd = 100;  // 10 px, squared
      for (size_t i = 0; i < p_x.size(); i++) {
        const double dx = X(f, p_x[i]) - e.GetX(), dy = Y(f, p_y[i]) - e.GetY();
        if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = (int)i; }
      }
    }
    if (best != p_hover) { p_hover = best; Refresh(); }
  }

  string p_title, p_xl, p_yl;
  vector<string> p_names;
  vector<double> p_x, p_y;
  vector<int> p_flag;
  bool p_parity = false;
  int p_hover = -1;
};


namespace {
// A results grid: worded headers, 3 decimals, rows up to `used` highlighted and
// the column `emph` in bold on a tint.
void fillGrid(wxGrid *g, const vector<string>& heads, const vector<vector<string>>& rows,
              size_t used, int emph)
{
  if (g->GetNumberRows()) g->DeleteRows(0, g->GetNumberRows());
  if (g->GetNumberCols()) g->DeleteCols(0, g->GetNumberCols());
  g->AppendCols((int)heads.size());
  g->AppendRows((int)rows.size());
  for (size_t j = 0; j < heads.size(); j++) g->SetColLabelValue((int)j, U(heads[j]));
  for (size_t i = 0; i < rows.size(); i++)
    for (size_t j = 0; j < heads.size() && j < rows[i].size(); j++) {
      g->SetCellValue((int)i, (int)j, U(rows[i][j]));
      g->SetReadOnly((int)i, (int)j);
      if (j > 0) g->SetCellAlignment((int)i, (int)j, wxALIGN_RIGHT, wxALIGN_CENTRE);
      if (i < used) g->SetCellBackgroundColour((int)i, (int)j, wxColour(225, 240, 255));
      if ((int)j == emph) {
        g->SetCellBackgroundColour((int)i, (int)j, i < used ? wxColour(190, 220, 250)
                                                            : wxColour(238, 238, 238));
        wxFont f = g->GetDefaultCellFont();
        f.SetWeight(wxFONTWEIGHT_BOLD);
        g->SetCellFont((int)i, (int)j, f);
      }
    }
  g->AutoSizeColumns();
  g->AutoSizeRows();
}

wxGrid *makeGrid(wxWindow *parent, const wxSize& size)
{
  wxGrid *g = new wxGrid(parent, wxID_ANY, wxDefaultPosition, size);
  g->CreateGrid(0, 0);
  g->EnableEditing(false);
  g->HideRowLabels();
  g->SetColLabelSize(wxGRID_AUTOSIZE);
  return g;
}

wxStaticText *wrapped(wxWindow *parent, const string& s, int width)
{
  wxStaticText *t = new wxStaticText(parent, wxID_ANY, U(s));
  t->Wrap(width);
  return t;
}
}  // namespace


string QsarWindow::run(const string& args, int& status) const
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

string QsarWindow::failure(const string& what) const
{
  const size_t e = p_summary.find("error: ");
  return e == string::npos ? what : p_summary.substr(e + 7, p_summary.find('\n', e) - e - 7);
}

bool QsarWindow::load(const vector<EcceURL>& calcs, const string& groups, string& why)
{
  std::unique_ptr<wxProgressDialog> pd;
  GatherProgress progress;
  if (calcs.size() > 3) {
    pd.reset(new wxProgressDialog("QSAR", "Starting...", (int)calcs.size(), this,
        wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
    progress = [&pd](size_t i, size_t n, const string& name) {
      return pd->Update((int)i, wxString::Format("Reading %zu of %zu: %s", i + 1, n, U(name)));
    };
  }
  string text;
  if (!qsarTable(calcs, groups, text, why, progress)) return false;
  vector<vector<string>> all = parseCsv(text);
  if (all.empty()) { why = "The table is empty."; return false; }
  p_calcs = calcs;
  p_groups = groups;
  p_csvText = text;
  p_head = all[0];
  p_rows.assign(all.begin() + 1, all.end());
  p_table = (fs::path(p_dir) / "table.csv").string();
  std::ofstream(p_table) << text;
  p_measured.clear();
  int st = 0;
  const string cols = run("columns " + quote(p_table), st);
  if (st == 0) {
    std::istringstream is(cols);
    for (string l; std::getline(is, l); ) if (!l.empty()) p_measured.push_back(l);
  }
  string path = calcs.empty() ? "" : calcs.front().getPath();
  while (path.size() > 1 && path.back() == '/') path.pop_back();
  const size_t cut = path.rfind('/');
  p_folder = cut == string::npos || cut == 0 ? path : path.substr(0, cut);
  const size_t nm = p_folder.rfind('/');
  p_name = nm == string::npos ? p_folder : p_folder.substr(nm + 1);
  return true;
}

QsarWindow::QsarWindow(wxWindow *parent, const vector<EcceURL>& calcs, const string& groups,
                       std::function<vector<EcceURL>()> chooseFolder,
                       std::function<void(QsarWindow *)> onDestroy)
  : wxFrame(parent, wxID_ANY, "QSAR", wxDefaultPosition, wxSize(1100, 700)),
    p_choose(chooseFolder), p_onDestroy(onDestroy)
{
  static int serial = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-qsar-" +
      std::to_string((long)fs::file_time_type::clock::now().time_since_epoch().count()) +
      "-" + std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);
  if (!load(calcs, groups, p_why)) return;
  p_ok = true;
  build();
  refreshHeader();
  fillTable();
}

QsarWindow::~QsarWindow()
{
  if (p_onDestroy) p_onDestroy(this);
  std::error_code ec;
  if (!p_dir.empty()) fs::remove_all(p_dir, ec);
}

void QsarWindow::refreshHeader()
{
  SetTitle(wxString::Format(wxString::FromUTF8("QSAR \xE2\x80\x93 %s (%zu calculations)"), U(p_name),
                            p_calcs.size()));
  p_header->SetLabel(wxString::Format("Data: %s, %zu calculations", U(p_folder), p_calcs.size()));
  p_plsY->Clear();
  for (const string& y : p_measured) p_plsY->Append(U(niceHeader(y)));
  if (p_plsY->GetCount()) p_plsY->SetSelection(0);
  p_plsRun->Enable(p_plsY->GetCount() > 0);
  updateCircular();
}

void QsarWindow::fillTable()
{
  if (p_tableGrid->GetNumberRows()) p_tableGrid->DeleteRows(0, p_tableGrid->GetNumberRows());
  if (p_tableGrid->GetNumberCols()) p_tableGrid->DeleteCols(0, p_tableGrid->GetNumberCols());
  p_tableGrid->AppendCols((int)p_head.size());
  p_tableGrid->AppendRows((int)p_rows.size());
  for (size_t j = 0; j < p_head.size(); j++) p_tableGrid->SetColLabelValue((int)j, U(niceHeader(p_head[j])));
  for (size_t i = 0; i < p_rows.size(); i++)
    for (size_t j = 0; j < p_head.size() && j < p_rows[i].size(); j++)
      p_tableGrid->SetCellValue((int)i, (int)j, U(p_rows[i][j]));
  p_tableGrid->AutoSizeColumns();
}

void QsarWindow::setStatus(const string& s) { if (p_status) p_status->SetLabel(U(s)); }

string QsarWindow::status() const { return p_plsStatus.empty() ? p_pcaStatus : p_plsStatus; }

void QsarWindow::showTab(int n) { if (p_book) p_book->SetSelection(n); }

bool QsarWindow::setGroups(const string& groups, string& why)
{
  if (!load(p_calcs, groups, why)) return false;
  p_summary.clear(); p_pcaStatus.clear(); p_plsStatus.clear();
  if (p_book) { refreshHeader(); fillTable(); }
  return true;
}

// A descriptor that models the same property as the one chosen makes the PLS circular.
void QsarWindow::updateCircular()
{
  p_circular.clear();
  string y = p_plsY ? p_plsY->GetStringSelection().ToStdString(wxConvUTF8) : "";
  std::transform(y.begin(), y.end(), y.begin(), ::tolower);
  struct Pair { const char *key, *descriptor, *what; };
  static const Pair pairs[] = {{"logp", "SlogP", "logP"}, {"refract", "SMR", "molar refractivity"},
                               {"tpsa", "TPSA", "polar surface area"}};
  for (const Pair& p : pairs)
    if (y.find(p.key) != string::npos &&
        std::find(p_head.begin(), p_head.end(), p.descriptor) != p_head.end()) {
      p_circular = p.descriptor;
      p_plsWarn->SetLabel(U(string(p.descriptor) + " is itself a model estimate of " + p.what +
          ", so a model of " + p_plsY->GetStringSelection().ToStdString(wxConvUTF8) +
          " that uses it is partly circular."));
      p_plsWarn->Wrap(560);
      p_plsLeave->SetLabel(U(string("Leave out ") + p.descriptor));
      break;
    }
  p_plsWarn->Show(!p_circular.empty());
  p_plsLeave->Show(!p_circular.empty());
  p_plsLeave->SetValue(true);
  if (p_book) p_book->Layout();
}

void QsarWindow::build()
{
  wxPanel *root = new wxPanel(this);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *hr = new wxBoxSizer(wxHORIZONTAL);
  p_header = new wxStaticText(root, wxID_ANY, "");
  hr->Add(p_header, 1, wxALIGN_CENTER_VERTICAL | wxALL, 6);
  wxButton *change = new wxButton(root, wxID_ANY, "Change folder...");
  change->SetToolTip("Use the folder (or calculations) now selected in the Organizer.");
  hr->Add(change, 0, wxALL, 4);
  top->Add(hr, 0, wxEXPAND);
  p_book = new wxNotebook(root, wxID_ANY);
  top->Add(p_book, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
  p_status = new wxStaticText(root, wxID_ANY, "Choose a tab and run the analysis.");
  top->Add(p_status, 0, wxEXPAND | wxALL, 6);
  root->SetSizer(top);

  // Table
  wxPanel *tp = new wxPanel(p_book);
  wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
  wxStaticBoxSizer *gb = new wxStaticBoxSizer(wxHORIZONTAL, tp, "Descriptors to include");
  static const struct { const char *name, *label, *tip; } groupDefs[] = {
    {"counts", "Counts", "atoms, bonds, rings, donors/acceptors, rotatable bonds"},
    {"topology", "Topological indices", "Wiener, Zagreb, chi, Kier, Balaban, information content"},
    {"models", "Property models", "SlogP, SMR, TPSA, surface area"},
    {"charges", "Gasteiger charges", "Q_PC+, Q_PC-, extremes"},
    {"vsa", "Surface areas (VSA)", "surface area by charge, SlogP and SMR"},
    {"qm", "From the calculations", "HOMO, LUMO, gap, dipole, charges"}};
  for (const auto& g : groupDefs) {
    wxCheckBox *c = new wxCheckBox(gb->GetStaticBox(), wxID_ANY, g.label);
    c->SetToolTip(g.tip);
    c->SetClientData((void *)g.name);
    c->SetValue(("," + p_groups + ",").find(string(",") + g.name + ",") != string::npos);
    gb->Add(c, 0, wxALL, 4);
    p_groupBoxes.push_back(c);
    c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
      string groups;
      for (wxCheckBox *b : p_groupBoxes)
        if (b->GetValue()) groups += (groups.empty() ? "" : ",") + string((const char *)b->GetClientData());
      if (groups.empty()) return;
      string why;
      if (!setGroups(groups, why) && why != "Cancelled.")
        wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
      else setStatus("Table updated: " + std::to_string(p_head.size()) + " columns.");
    });
  }
  ts->Add(gb, 0, wxEXPAND | wxALL, 4);
  p_tableGrid = makeGrid(tp, wxSize(600, 300));
  ts->Add(p_tableGrid, 1, wxEXPAND | wxALL, 4);
  wxButton *exp = new wxButton(tp, wxID_ANY, "Export CSV...");
  ts->Add(exp, 0, wxALL, 4);
  tp->SetSizer(ts);
  p_book->AddPage(tp, "Table");

  // PCA
  wxPanel *pp = new wxPanel(p_book);
  wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *pr = new wxBoxSizer(wxHORIZONTAL);
  pr->Add(new wxStaticText(pp, wxID_ANY, "Scaling:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_pcaScale = new wxChoice(pp, wxID_ANY);
  for (const char *s : {"Autoscale", "Pareto", "Centre only"}) p_pcaScale->Append(s);
  p_pcaScale->SetSelection(0);
  pr->Add(p_pcaScale, 0, wxALL, 4);
  pr->Add(new wxStaticText(pp, wxID_ANY, "Components:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_pcaComps = new wxChoice(pp, wxID_ANY);
  p_pcaComps->Append("Automatic (cross-validation)");
  for (int i = 1; i <= 10; i++) p_pcaComps->Append(wxString::Format("%d", i));
  p_pcaComps->SetSelection(0);
  pr->Add(p_pcaComps, 0, wxALL, 4);
  wxButton *runPca = new wxButton(pp, wxID_ANY, "Run PCA");
  pr->Add(runPca, 0, wxALL, 4);
  wxButton *savePca = new wxButton(pp, wxID_ANY, "Save Results...");
  pr->Add(savePca, 0, wxALL, 4);
  ps->Add(pr, 0, wxEXPAND);
  wxBoxSizer *pm = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *pl = new wxBoxSizer(wxVERTICAL);
  p_pcaSentence = wrapped(pp, "", 560);
  pl->Add(p_pcaSentence, 0, wxEXPAND | wxALL, 4);
  p_pcaGrid = makeGrid(pp, wxSize(560, 230));
  pl->Add(p_pcaGrid, 0, wxEXPAND | wxALL, 4);
  wxBoxSizer *pc = new wxBoxSizer(wxHORIZONTAL);
  p_pcaConst = new wxStaticText(pp, wxID_ANY, "");
  p_pcaShow = new wxButton(pp, wxID_ANY, "Show...");
  p_pcaShow->Hide();
  pc->Add(p_pcaConst, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  pc->Add(p_pcaShow, 0, wxALL, 2);
  pl->Add(pc, 0, wxEXPAND);
  pl->Add(wrapped(pp,
      "How to read this: each molecule is a point; t1 and t2 are the two directions along "
      "which the molecules differ most, and the percentages on the axes say how much of the "
      "differences each direction holds. Molecules close together have similar descriptors. "
      "Read the column 'Variation explained, total' to see how much the first components "
      "capture together; Q\xC2\xB2 says how much of it holds up when parts of the data are left out. "
      "Squares outside the limits are unlike the rest.", 560), 0, wxEXPAND | wxALL, 4);
  pm->Add(pl, 1, wxEXPAND);
  p_pcaPlot = new QsarPlot(pp);
  pm->Add(p_pcaPlot, 1, wxEXPAND | wxALL, 4);
  ps->Add(pm, 1, wxEXPAND);
  pp->SetSizer(ps);
  p_book->AddPage(pp, "PCA");

  // PLS
  wxPanel *lp = new wxPanel(p_book);
  wxBoxSizer *ls = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *lr = new wxBoxSizer(wxHORIZONTAL);
  lr->Add(new wxStaticText(lp, wxID_ANY, "Model:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsY = new wxChoice(lp, wxID_ANY);
  lr->Add(p_plsY, 0, wxALL, 4);
  lr->Add(new wxStaticText(lp, wxID_ANY, "Scaling:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsScale = new wxChoice(lp, wxID_ANY);
  for (const char *s : {"Autoscale", "Pareto", "Centre only"}) p_plsScale->Append(s);
  p_plsScale->SetSelection(0);
  lr->Add(p_plsScale, 0, wxALL, 4);
  lr->Add(new wxStaticText(lp, wxID_ANY, "Components:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsComps = new wxChoice(lp, wxID_ANY);
  p_plsComps->Append("Automatic (cross-validation)");
  for (int i = 1; i <= 10; i++) p_plsComps->Append(wxString::Format("%d", i));
  p_plsComps->SetSelection(0);
  lr->Add(p_plsComps, 0, wxALL, 4);
  ls->Add(lr, 0, wxEXPAND);
  wxBoxSizer *lr2 = new wxBoxSizer(wxHORIZONTAL);
  lr2->Add(new wxStaticText(lp, wxID_ANY, "Test set (% of molecules kept out):"), 0,
           wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsTest = new wxSpinCtrl(lp, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 0, 50, 0);
  lr2->Add(p_plsTest, 0, wxALL, 4);
  lr2->Add(new wxStaticText(lp, wxID_ANY, "Permutation runs:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsPerm = new wxSpinCtrl(lp, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 0, 500, 0);
  lr2->Add(p_plsPerm, 0, wxALL, 4);
  p_plsRun = new wxButton(lp, wxID_ANY, "Run PLS");
  lr2->Add(p_plsRun, 0, wxALL, 4);
  wxButton *savePls = new wxButton(lp, wxID_ANY, "Save Results...");
  lr2->Add(savePls, 0, wxALL, 4);
  ls->Add(lr2, 0, wxEXPAND);
  p_plsWarn = new wxStaticText(lp, wxID_ANY, "");
  p_plsWarn->SetForegroundColour(wxColour(150, 0, 0));
  p_plsLeave = new wxCheckBox(lp, wxID_ANY, "Leave out");
  ls->Add(p_plsWarn, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  ls->Add(p_plsLeave, 0, wxLEFT | wxBOTTOM, 6);
  wxBoxSizer *lm = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *ll = new wxBoxSizer(wxVERTICAL);
  p_plsSentence = wrapped(lp, "", 560);
  ll->Add(p_plsSentence, 0, wxEXPAND | wxALL, 4);
  p_plsGrid = makeGrid(lp, wxSize(560, 140));
  ll->Add(p_plsGrid, 0, wxEXPAND | wxALL, 4);
  ll->Add(new wxStaticText(lp, wxID_ANY, "Descriptors by importance (VIP)"), 0, wxLEFT, 6);
  p_vipGrid = makeGrid(lp, wxSize(560, 130));
  ll->Add(p_vipGrid, 0, wxEXPAND | wxALL, 4);
  wxBoxSizer *lc = new wxBoxSizer(wxHORIZONTAL);
  p_plsConst = new wxStaticText(lp, wxID_ANY, "");
  p_plsShow = new wxButton(lp, wxID_ANY, "Show...");
  p_plsShow->Hide();
  lc->Add(p_plsConst, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  lc->Add(p_plsShow, 0, wxALL, 2);
  ll->Add(lc, 0, wxEXPAND);
  ll->Add(wrapped(lp,
      "How to read this: the plot compares what was measured with what the model gives. "
      "R\xC2\xB2Y is how well the model reproduces the molecules it was built on; Q\xC2\xB2 is how well it "
      "predicts molecules left out, and is the number to judge the model by (close to 1 is "
      "good, near 0 or below is no better than the average). VIP says how much a descriptor "
      "matters: above 1 is more than average.", 560), 0, wxEXPAND | wxALL, 4);
  lm->Add(ll, 1, wxEXPAND);
  p_plsPlot = new QsarPlot(lp);
  lm->Add(p_plsPlot, 1, wxEXPAND | wxALL, 4);
  ls->Add(lm, 1, wxEXPAND);
  lp->SetSizer(ls);
  p_book->AddPage(lp, "PLS");

  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  outer->Add(root, 1, wxEXPAND);
  SetSizer(outer);

  p_pcaScale->SetToolTip(SCALING_TIPS[0]);
  p_plsScale->SetToolTip(SCALING_TIPS[0]);
  p_pcaScale->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    p_pcaScale->SetToolTip(SCALING_TIPS[std::max(0, p_pcaScale->GetSelection())]); });
  p_plsScale->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    p_plsScale->SetToolTip(SCALING_TIPS[std::max(0, p_plsScale->GetSelection())]); });
  p_plsY->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { updateCircular(); });

  change->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    vector<EcceURL> c = p_choose ? p_choose() : vector<EcceURL>();
    if (c.empty()) {
      wxMessageBox("Select the project folder (or calculations) in the Organizer first.", "QSAR",
                   wxOK | wxICON_INFORMATION, this);
      return;
    }
    string why;
    if (!load(c, p_groups, why)) {
      if (why != "Cancelled.") wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
      return;
    }
    p_summary.clear();
    refreshHeader();
    fillTable();
    setStatus("Folder changed.");
  });
  exp->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Write the QSAR table to", "", "qsar.csv", "CSV (*.csv)|*.csv",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) std::ofstream(dlg.GetPath().ToStdString(wxConvUTF8)) << p_csvText;
  });
  runPca->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    string why;
    if (!runPca(scalingToken(p_pcaScale->GetSelection()), p_pcaComps->GetSelection(), why))
      wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
  });
  p_plsRun->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    string why;
    const int k = p_plsY->GetSelection();
    vector<string> ex;
    if (!p_circular.empty() && p_plsLeave->GetValue()) ex.push_back(p_circular);
    if (k < 0 || !runPls(p_measured[k], scalingToken(p_plsScale->GetSelection()),
                         p_plsComps->GetSelection(), p_plsTest->GetValue(), p_plsPerm->GetValue(),
                         why, ex))
      wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
  });
  for (wxButton *b : {savePca, savePls})
    b->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
      wxFileDialog dlg(this, "Write the results to (a file name inside the folder to use)", "",
                       "results", "*", wxFD_SAVE);
      if (dlg.ShowModal() != wxID_OK) return;
      string why;
      if (!saveResults(dlg.GetPath().ToStdString(wxConvUTF8), why))
        wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
    });
  p_pcaShow->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { showNotes(); });
  p_plsShow->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { showNotes(); });
}

void QsarWindow::showNotes()
{
  string t;
  for (const string& n : p_notes) t += n + "\n";
  wxMessageBox(U(t.empty() ? "Nothing was left out." : t), "Descriptors left out", wxOK, this);
}

// The "note:" lines of the report; constant descriptors counted for the one-line message.
static void takeNotes(const string& summary, vector<string>& notes, int& constant)
{
  notes.clear();
  constant = 0;
  std::istringstream is(summary);
  for (string l; std::getline(is, l); )
    if (l.compare(0, 6, "note: ") == 0) {
      notes.push_back(l.substr(6));
      if (l.find("constant") != string::npos) constant++;
    }
}

bool QsarWindow::runPca(const string& scaling, int components, string& why)
{
  if (!p_ok) { why = p_why; return false; }
  string args = "pca " + quote(p_table) + " --scale " + scaling + " --outdir " + quote(p_dir);
  if (components > 0) args += " --components " + std::to_string(components);
  for (const string& y : p_measured) args += " --y " + quote(y);
  int st = 0;
  p_summary = run(args, st);
  if (st != 0) { why = failure("ecce-qsar pca failed"); return false; }
  vector<vector<string>> sc = parseCsv(slurp(fs::path(p_dir) / "pca_scores.csv"));
  vector<vector<string>> comp = parseCsv(slurp(fs::path(p_dir) / "pca_components.csv"));
  const size_t A = sc.empty() ? 0 : sc[0].size() - 3;
  double t2lim = 1e300, dmlim = 1e300;
  { const size_t a = p_summary.find("T2 limit (95%) ");
    if (a != string::npos) sscanf(p_summary.c_str() + a, "T2 limit (95%%) %lf, DModX limit (95%%) %lf", &t2lim, &dmlim); }
  vector<string> names;
  vector<double> x, y;
  vector<int> flag;
  for (size_t i = 1; i < sc.size(); i++) {
    if (sc[i].size() < 5 || A < 2) continue;
    names.push_back(sc[i][0]);
    x.push_back(atof(sc[i][1].c_str()));
    y.push_back(atof(sc[i][2].c_str()));
    flag.push_back(atof(sc[i][A + 1].c_str()) > t2lim || atof(sc[i][A + 2].c_str()) > dmlim ? 1 : 0);
  }
  vector<vector<string>> rows;
  double cum = 0, qcum = 0;
  for (size_t i = 1; i < comp.size(); i++) {
    if (comp[i].size() < 6) continue;
    rows.push_back({comp[i][0], num3(comp[i][1]), num3(comp[i][2]), num3(comp[i][3]),
                    num3(comp[i][4]), num3(comp[i][5])});
    if (i == A) { cum = atof(comp[i][3].c_str()); qcum = atof(comp[i][5].c_str()); }
  }
  fillGrid(p_pcaGrid, {"Component", "Eigenvalue", "Variation explained, this component (R\xC2\xB2X)",
                       "Variation explained, total (cum. R\xC2\xB2X)",
                       "Predicted, this component (Q\xC2\xB2)", "Predicted, total (cum. Q\xC2\xB2)"},
           rows, A, 3);
  char b[160];
  snprintf(b, sizeof b, "The first %zu component%s explain%s %s %% of the variation between the molecules.",
           A, A == 1 ? "" : "s", A == 1 ? "s" : "", percent(cum).c_str());
  p_pcaSentence->SetLabel(U(b));
  p_pcaSentence->Wrap(560);
  auto share = [&](size_t k) { return k < comp.size() && comp[k].size() > 2 ? atof(comp[k][2].c_str()) : 0.0; };
  p_pcaPlot->set("PCA scores: molecules by their first two components",
                 "t1 (" + percent(share(1)) + " %)", "t2 (" + percent(share(2)) + " %)", names, x, y, flag,
                 false);
  int constant = 0;
  takeNotes(p_summary, p_notes, constant);
  p_pcaConst->SetLabel(constant ? wxString::Format("%d descriptors are constant across these molecules "
                                                   "and were left out", constant) : wxString());
  p_pcaShow->Show(!p_notes.empty());
  p_pcaStatus = string("PCA done: ") + std::to_string(A) + " components explain " + percent(cum) +
                " % of the variation (cross-validated Q\xC2\xB2 " + num3(std::to_string(qcum)) + ")";
  setStatus(p_pcaStatus);
  p_plsStatus.clear();
  p_book->Layout();
  return true;
}

bool QsarWindow::runPls(const string& y, const string& scaling, int components, int testPercent,
                        int permutations, string& why, const vector<string>& exclude)
{
  if (!p_ok) { why = p_why; return false; }
  string args = "pls " + quote(p_table) + " --y " + quote(y) + " --scale " + scaling +
                " --outdir " + quote(p_dir) + " --save-model " + quote(p_dir + "/pls_model.json");
  if (components > 0) args += " --components " + std::to_string(components);
  if (testPercent > 0) args += " --test-fraction " + std::to_string(testPercent / 100.0);
  if (permutations > 0) args += " --permutations " + std::to_string(permutations);
  for (const string& e : exclude) args += " --exclude " + quote(e);
  int st = 0;
  p_summary = run(args, st);
  if (st != 0) { why = failure("ecce-qsar pls failed"); return false; }
  const string label = niceHeader(y);
  vector<vector<string>> pr = parseCsv(slurp(fs::path(p_dir) / "pls_predictions.csv"));
  vector<string> names;
  vector<double> obs, pred;
  vector<int> flag;
  for (size_t i = 1; i < pr.size(); i++) {
    if (pr[i].size() < 4) continue;
    names.push_back(pr[i][0]);
    flag.push_back(pr[i][1] == "test" ? 2 : 0);
    obs.push_back(atof(pr[i][2].c_str()));
    pred.push_back(atof(pr[i][3].c_str()));
  }
  p_plsPlot->set("PLS of " + label + ": predicted vs observed", "observed " + label,
                 "predicted " + label, names, obs, pred, flag, true);
  vector<vector<string>> comp = parseCsv(slurp(fs::path(p_dir) / "pls_components.csv"));
  const vector<vector<string>> sc = parseCsv(slurp(fs::path(p_dir) / "pls_scores.csv"));
  const size_t A = sc.empty() ? 0 : sc[0].size() - 3;
  vector<vector<string>> rows;
  for (size_t i = 1; i < comp.size(); i++) {
    if (comp[i].size() < 8) continue;
    rows.push_back({comp[i][0], num3(comp[i][1]), num3(comp[i][2]), num3(comp[i][3]),
                    num3(comp[i][4]), num3(comp[i][5]), num3(comp[i][6]), num3(comp[i][7])});
  }
  fillGrid(p_plsGrid, {"Component", "X variation, this (R\xC2\xB2X)", "X variation, total (cum. R\xC2\xB2X)",
                       "Y explained, this (R\xC2\xB2Y)", "Y explained, total (cum. R\xC2\xB2Y)",
                       "Y predicted, this (Q\xC2\xB2)", "Y predicted, total (cum. Q\xC2\xB2)",
                       "Prediction error (RMSECV)"}, rows, A, 6);
  // VIP, largest first.
  vector<vector<string>> vip = parseCsv(slurp(fs::path(p_dir) / "pls_vip.csv"));
  vector<vector<string>> vrows;
  for (size_t i = 1; i < vip.size(); i++) if (vip[i].size() >= 2) vrows.push_back({vip[i][0], vip[i][1]});
  std::stable_sort(vrows.begin(), vrows.end(), [](const vector<string>& a, const vector<string>& b) {
    return atof(a[1].c_str()) > atof(b[1].c_str()); });
  for (auto& r : vrows) r[1] = num3(r[1]);
  fillGrid(p_vipGrid, {"Descriptor", "VIP (importance)"}, vrows, 0, 1);
  double r2y = 0, q2 = 0, rmsee = 0, rmsecv = 0;
  const size_t a = p_summary.find("R2Y ");
  if (a != string::npos)
    sscanf(p_summary.c_str() + a, "R2Y %lf  Q2 %lf  RMSEE %lf  RMSECV %lf", &r2y, &q2, &rmsee, &rmsecv);
  char b[320];
  snprintf(b, sizeof b, "The model of %s uses %zu component%s. It reproduces %s %% of the variation in "
           "%s (R\xC2\xB2Y) and predicts left-out molecules with Q\xC2\xB2 = %.2f; judge it by Q\xC2\xB2.",
           label.c_str(), A, A == 1 ? "" : "s", percent(r2y).c_str(), label.c_str(), q2);
  p_plsSentence->SetLabel(U(b));
  p_plsSentence->Wrap(560);
  int constant = 0;
  takeNotes(p_summary, p_notes, constant);
  p_plsConst->SetLabel(constant ? wxString::Format("%d descriptors are constant across these molecules "
                                                   "and were left out", constant) : wxString());
  p_plsShow->Show(!p_notes.empty());
  snprintf(b, sizeof b, "PLS of %s done: R\xC2\xB2Y %.2f, Q\xC2\xB2 %.2f, RMSECV %.2f", label.c_str(), r2y, q2, rmsecv);
  p_plsStatus = b;
  setStatus(p_plsStatus);
  p_book->Layout();
  return true;
}

bool QsarWindow::saveResults(const string& dir, string& why) const
{
  std::error_code ec;
  fs::create_directories(dir, ec);
  for (const char *f : {"table.csv", "pca_scores.csv", "pca_loadings.csv", "pca_components.csv",
                        "pls_components.csv", "pls_scores.csv", "pls_vip.csv",
                        "pls_predictions.csv", "pls_permutation.csv", "pls_model.json"}) {
    const fs::path src = fs::path(p_dir) / f;
    if (!fs::exists(src)) continue;
    fs::copy_file(src, fs::path(dir) / f, fs::copy_options::overwrite_existing, ec);
    if (ec) { why = string("cannot write ") + f + ": " + ec.message(); return false; }
  }
  return true;
}
