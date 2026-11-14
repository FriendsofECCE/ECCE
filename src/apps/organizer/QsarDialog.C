#include "QsarDialog.H"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <limits>
#include <fstream>
#include <memory>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/dcbuffer.h>
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
#include "tdat/VibSpectrum.H"
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


namespace {
wxString utf(const string& s) { return wxString::FromUTF8(s.c_str()); }
string readText(const fs::path& path)
{
  std::ifstream f(path);
  return string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}
vector<vector<string>> readCsv(const fs::path& path) { return parseCsv(readText(path)); }
const char *scalings[] = {"uv", "pareto", "ctr", "none"};

SpectrumTicks enclosingTicks(double& lo, double& hi)
{
  const double step = niceTicks(lo, hi, 4).step;
  lo = std::floor(lo / step) * step;
  hi = std::ceil(hi / step) * step;
  return niceTicks(lo, hi, int(std::lround((hi - lo) / step)));
}

wxString tickLabel(double value, const SpectrumTicks& ticks)
{
  return wxString::Format("%.*f", ticks.decimals, value);
}


wxString gridText(wxGrid *g, bool whole)
{
  wxString text;
  int r0 = 0, r1 = g->GetNumberRows() - 1, c0 = 0, c1 = g->GetNumberCols() - 1;
  if (!whole) {
    r0 = r1 + 1; c0 = c1 + 1; r1 = c1 = -1;
    for (int r = 0; r < g->GetNumberRows(); ++r)
      for (int c = 0; c < g->GetNumberCols(); ++c)
        if (g->IsInSelection(r, c)) {
          r0 = std::min(r0, r); r1 = std::max(r1, r);
          c0 = std::min(c0, c); c1 = std::max(c1, c);
        }
    if (r1 < 0) { r0 = r1 = g->GetGridCursorRow(); c0 = c1 = g->GetGridCursorCol(); }
  }
  if (r0 < 0 || c0 < 0 || c1 < c0) return text;
  if (whole) {
    for (int c = c0; c <= c1; ++c) { if (c != c0) text += "\t"; text += g->GetColLabelValue(c); }
    text += "\n";
  }
  for (int r = r0; r <= r1; ++r) {
    for (int c = c0; c <= c1; ++c) {
      if (c != c0) text += "\t";
      if (whole || !g->IsSelection() || g->IsInSelection(r, c)) text += g->GetCellValue(r, c);
    }
    text += "\n";
  }
  return text;
}
bool copyGrid(wxGrid *g, bool whole)
{
  if (!wxTheClipboard->Open()) return false;
  const bool ok = wxTheClipboard->SetData(new wxTextDataObject(gridText(g, whole)));
  wxTheClipboard->Close();
  return ok;
}
void fitDescription(wxGrid *g)
{
  if (g->GetNumberCols() < 3 || g->GetColLabelValue(2) != "Description") return;
  const int width = g->GetClientSize().x - g->GetRowLabelSize() -
                    g->GetColSize(0) - g->GetColSize(1) - 25;
  if (width > 150) g->SetColSize(2, width);
  g->AutoSizeRows(false);
}
void fillGrid(wxGrid *g, const vector<vector<string>>& data)
{
  if (g->GetNumberRows()) g->DeleteRows(0, g->GetNumberRows());
  if (g->GetNumberCols()) g->DeleteCols(0, g->GetNumberCols());
  if (data.empty()) return;
  g->AppendCols(data[0].size());
  if (data.size() > 1) g->AppendRows(data.size() - 1);
  for (size_t c = 0; c < data[0].size(); ++c) g->SetColLabelValue(c, utf(data[0][c]));
  for (size_t r = 1; r < data.size(); ++r)
    for (size_t c = 0; c < data[r].size() && c < data[0].size(); ++c)
      g->SetCellValue(r - 1, c, utf(data[r][c]));
  g->AutoSizeColumns(false);
  for (int c = 0; c < g->GetNumberCols(); ++c)
    g->SetColSize(c, std::min(600, std::max(90, g->GetColSize(c) + 12)));
  if (g->GetNumberCols() >= 3 && g->GetColLabelValue(2) == "Description") {
    auto *attr = new wxGridCellAttr;
    attr->SetRenderer(new wxGridCellAutoWrapStringRenderer);
    g->SetColAttr(2, attr);
    fitDescription(g);
  }
}
wxGrid *gridPage(wxNotebook *book, const wxString& title)
{
  auto *page = new wxPanel(book);
  auto *sizer = new wxBoxSizer(wxVERTICAL);
  auto *copy = new wxButton(page, wxID_ANY, "Copy table");
  sizer->Add(copy, 0, wxALL, 5);
  auto *grid = new wxGrid(page, wxID_ANY);
  grid->CreateGrid(0, 0); grid->EnableEditing(false); grid->SetColLabelSize(48);
  grid->SetDefaultCellOverflow(false);
  grid->Bind(wxEVT_SIZE, [grid](wxSizeEvent& e) {
    e.Skip(); grid->CallAfter([grid]() { fitDescription(grid); });
  });
  grid->GetGridWindow()->Bind(wxEVT_MOTION, [grid](wxMouseEvent& e) {
    int x, y; grid->CalcUnscrolledPosition(e.GetX(), e.GetY(), &x, &y);
    int row = grid->YToRow(y), col = grid->XToCol(x);
    if (row >= 0 && col >= 0) grid->GetGridWindow()->SetToolTip(grid->GetCellValue(row, col));
    else grid->GetGridWindow()->UnsetToolTip();
    e.Skip();
  });
  sizer->Add(grid, 1, wxEXPAND | wxALL, 5);
  page->SetSizer(sizer); book->AddPage(page, title);
  copy->Bind(wxEVT_BUTTON, [grid](wxCommandEvent&) { copyGrid(grid, true); });
  grid->Bind(wxEVT_CHAR_HOOK, [grid](wxKeyEvent& e) {
    if (e.GetModifiers() == wxMOD_CONTROL && (e.GetKeyCode() == 'C' || e.GetKeyCode() == 'c'))
      copyGrid(grid, false);
    else e.Skip();
  });
  return grid;
}
}

class QsarPlot : public wxPanel
{
public:
  struct Point { string name, tip; double x, y; int row, kind; };
  vector<Point> points;
  string xTitle, yTitle, xLabels, yLabels;
  bool parity = false;
  std::function<void(int)> selected;
  QsarPlot(wxWindow *parent) : wxPanel(parent)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &QsarPlot::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) { hover(e.GetPosition()); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { UnsetToolTip(); });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { click(e.GetPosition()); });
  }
  void reset() { points.clear(); p_selected = -1; UnsetToolTip(); }
  wxPoint position(size_t i) const
  {
    const auto& p = points[i];
    return wxPoint(80 + (p.x - p_x0) / (p_x1 - p_x0) * (GetClientSize().x - 105),
                   45 + (p_y1 - p.y) / (p_y1 - p_y0) * (GetClientSize().y - 125));
  }
  int hit(wxPoint pos) const
  {
    int best = -1; double distance = 65;
    for (size_t i = 0; i < points.size(); ++i) {
      wxPoint d = position(i) - pos;
      double dd = d.x * d.x + d.y * d.y;
      if (dd < distance) { distance = dd; best = i; }
    }
    return best;
  }
  string hover(wxPoint pos)
  {
    int i = hit(pos);
    if (i < 0) { UnsetToolTip(); return ""; }
    SetToolTip(utf(points[i].tip)); return points[i].tip;
  }
  int click(wxPoint pos)
  {
    p_selected = hit(pos);
    if (p_selected >= 0 && selected) selected(points[p_selected].row);
    Refresh(); return p_selected;
  }
private:
  void OnPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this); dc.SetBackground(*wxWHITE_BRUSH); dc.Clear();
    if (points.empty()) return;
    const wxSize sz = GetClientSize();
    if (sz.x < 180 || sz.y < 180) return;
    p_x0 = p_y0 = std::numeric_limits<double>::infinity(); p_x1 = p_y1 = -p_x0;
    for (const auto& p : points) {
      p_x0 = std::min(p_x0, p.x); p_x1 = std::max(p_x1, p.x);
      p_y0 = std::min(p_y0, p.y); p_y1 = std::max(p_y1, p.y);
    }
    if (parity) { p_x0 = p_y0 = std::min(p_x0, p_y0); p_x1 = p_y1 = std::max(p_x1, p_y1); }
    double dx = std::max(1e-6, p_x1 - p_x0), dy = std::max(1e-6, p_y1 - p_y0);
    p_x0 -= .07 * dx; p_x1 += .07 * dx; p_y0 -= .07 * dy; p_y1 += .07 * dy;
    const SpectrumTicks xt = enclosingTicks(p_x0, p_x1), yt = enclosingTicks(p_y0, p_y1);
    auto X = [&](double v) { return 80 + int((v - p_x0) / (p_x1 - p_x0) * (sz.x - 105)); };
    auto Y = [&](double v) { return 45 + int((p_y1 - v) / (p_y1 - p_y0) * (sz.y - 125)); };
    dc.SetFont(wxFont(wxFontInfo(9)));
    dc.SetPen(*wxLIGHT_GREY_PEN);
    dc.DrawRectangle(80, 45, sz.x - 105, sz.y - 125);
    xLabels.clear(); yLabels.clear();
    for (double v : xt.values) {
      dc.DrawLine(X(v), 45, X(v), sz.y - 80);
      const wxString label = tickLabel(v, xt);
      dc.DrawText(label, X(v) - dc.GetTextExtent(label).x / 2, sz.y - 74);
      xLabels += label.ToStdString() + " ";
    }
    for (double v : yt.values) {
      dc.DrawLine(80, Y(v), sz.x - 25, Y(v));
      const wxString label = tickLabel(v, yt);
      dc.DrawText(label, 74 - dc.GetTextExtent(label).x, Y(v) - 6);
      yLabels += label.ToStdString() + " ";
    }
    dc.SetTextForeground(*wxBLACK);
    dc.DrawText(utf(xTitle), std::max(80, (sz.x - dc.GetTextExtent(utf(xTitle)).x) / 2), sz.y - 48);
    dc.DrawRotatedText(utf(yTitle), 8, (sz.y + dc.GetTextExtent(utf(yTitle)).x) / 2, 90);
    if (parity) {
      dc.SetPen(wxPen(wxColour(110, 110, 110), 1, wxPENSTYLE_SHORT_DASH));
      dc.DrawLine(X(p_x0), Y(p_x0), X(p_x1), Y(p_x1));
      dc.DrawText("y = x", X(p_x0 + .75 * (p_x1 - p_x0)), Y(p_y0 + .75 * (p_y1 - p_y0)) - 18);
    }
    const wxColour colours[] = {wxColour(30, 90, 170), wxColour(200, 50, 40), wxColour(25, 140, 90)};
    for (size_t i = 0; i < points.size(); ++i) {
      dc.SetPen(wxPen(colours[points[i].kind], 1)); dc.SetBrush(wxBrush(colours[points[i].kind]));
      wxPoint pos = position(i); dc.DrawCircle(pos, 4);
      if (int(i) == p_selected) { dc.SetPen(wxPen(*wxBLACK, 2)); dc.SetBrush(*wxTRANSPARENT_BRUSH); dc.DrawCircle(pos, 8); }
    }
    const char *labels[] = {"Fitted", "Test set", "Cross-validated"};
    for (int i = 0; i < (parity ? 3 : 1); ++i) {
      int x = 80 + i * (sz.x - 105) / 3;
      dc.SetBrush(wxBrush(colours[i])); dc.SetPen(wxPen(colours[i])); dc.DrawCircle(x, 20, 4);
      dc.DrawText(labels[i], x + 8, 13);
    }
  }
  double p_x0 = 0, p_x1 = 1, p_y0 = 0, p_y1 = 1;
  int p_selected = -1;
};

class QsarScree : public wxPanel
{
public:
  vector<vector<double>> bars, lines;
  string yLabels;
  QsarScree(wxWindow *parent) : wxPanel(parent)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &QsarScree::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
  }
private:
  void OnPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this); dc.SetBackground(*wxWHITE_BRUSH); dc.Clear();
    const auto sz = GetClientSize();
    if (bars.empty() || bars[0].empty() || sz.x < 180 || sz.y < 180) return;
    const int n = std::min(size_t(10), bars[0].size()), left = 65, top = 78;
    const double w = sz.x - left - 25, h = sz.y - top - 60;
    double lo = 0, hi = 100;
    for (const auto& series : bars) for (int i = 0; i < n && i < int(series.size()); ++i)
      if (std::isfinite(series[i])) lo = std::min(lo, series[i] * 100);
    for (const auto& series : lines) for (int i = 0; i < n && i < int(series.size()); ++i)
      if (std::isfinite(series[i])) lo = std::min(lo, series[i] * 100);
    const SpectrumTicks yt = enclosingTicks(lo, hi);
    auto Y = [&](double percent) { return top + int((hi - percent) / (hi - lo) * h); };
    auto X = [&](int i) { return left + int((i + .5) * w / n); };
    dc.SetFont(wxFont(wxFontInfo(9)));
    dc.DrawText("Variance / prediction explained (%)", left, 52);
    yLabels.clear();
    for (double v : yt.values) {
      dc.SetPen(*wxLIGHT_GREY_PEN); dc.DrawLine(left, Y(v), left + w, Y(v));
      const wxString label = tickLabel(v, yt);
      dc.DrawText(label, 12, Y(v) - 7);
      yLabels += label.ToStdString() + " ";
    }
    const wxColour colours[] = {wxColour(30, 90, 170), wxColour(200, 110, 20), wxColour(25, 140, 90)};
    const char *labels[] = {"R²X", "R²Y", "Q²"};
    const int count = bars.size();
    for (int s = 0; s < count; ++s) {
      const double bw = std::min(22.0, .7 * w / n / count);
      dc.SetPen(wxPen(colours[s])); dc.SetBrush(wxBrush(colours[s]));
      for (int i = 0; i < n; ++i) {
        if (i >= int(bars[s].size()) || !std::isfinite(bars[s][i])) continue;
        const int y = Y(bars[s][i] * 100), zero = Y(0);
        dc.DrawRectangle(X(i) + int((s - count / 2.0) * bw), std::min(y, zero), std::max(1,int(bw - 1)), std::max(1,std::abs(zero - y)));
      }
      dc.SetPen(wxPen(colours[s], 2));
      for (int i = 1; i < n && i < int(lines[s].size()); ++i)
        if (std::isfinite(lines[s][i]) && std::isfinite(lines[s][i-1]))
          dc.DrawLine(X(i-1), Y(lines[s][i-1] * 100), X(i), Y(lines[s][i] * 100));
      for (int i = 0; i < n && i < int(lines[s].size()); ++i)
        if (std::isfinite(lines[s][i])) dc.DrawCircle(X(i), Y(lines[s][i] * 100), 3);
      const int lx = left + s * int(w / count);
      dc.DrawRectangle(lx, 12, 9, 9); dc.DrawText(utf(string(labels[s]) + " per component"), lx + 14, 8);
      dc.DrawLine(lx, 37, lx + 10, 37); dc.DrawText(utf(string("Cumulative ") + labels[s]), lx + 14, 30);
    }
    dc.SetTextForeground(*wxBLACK);
    for (int i = 0; i < n; ++i) dc.DrawText(wxString::Format("%d", i+1), X(i)-4, top+h+5);
    dc.DrawText("Component", left + w / 2 - 30, sz.y - 27);
  }
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
  : wxDialog(parent, wxID_ANY, "QSAR Analysis", wxDefaultPosition, wxSize(1200, 680),
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
  for (const char *s : {"Unit variance (autoscaling)", "Pareto", "Centred only", "None"}) p_scale->Append(s);
  const wxString scalingHelp =
      "Unit variance (autoscaling): centre, divide by the standard deviation; every descriptor weighs the same. "
      "The usual choice when descriptors have different units.\n"
      "Pareto: centre, divide by the square root of the standard deviation.\n"
      "Centred only: keeps the original variances; for columns in the same unit.\n"
      "None: no centring or scaling.";
  p_scale->SetToolTip(scalingHelp);
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
  wxBoxSizer *row2 = new wxBoxSizer(wxHORIZONTAL);
  row2->Add(new wxStaticText(this, wxID_ANY, "PLS: measured value:"), 0,
            wxALIGN_CENTER_VERTICAL | wxALL, 5);
  p_y = new wxChoice(this, wxID_ANY);
  for (const string& y : p_measured) p_y->Append(wxString::FromUTF8(y.c_str()));
  if (p_y->GetCount()) p_y->SetSelection(0);
  row2->Add(p_y, 0, wxALL, 5);
  row2->Add(new wxStaticText(this, wxID_ANY, "Test set %:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  p_test = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                          wxSP_ARROW_KEYS, 0, 50, 0);
  row2->Add(p_test, 0, wxALL, 5);
  row2->Add(new wxStaticText(this, wxID_ANY, "Permutations:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  p_perm = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                          wxSP_ARROW_KEYS, 0, 500, 0);
  row2->Add(p_perm, 0, wxALL, 5);
  wxButton *runPls = new wxButton(this, wxID_ANY, "Run PLS");
  runPls->Enable(p_y->GetCount() > 0);
  row2->Add(runPls, 0, wxALL, 5);
  top->Add(row2, 0, wxEXPAND);
  p_pages = new wxNotebook(this, wxID_ANY);
  auto *report = new wxPanel(p_pages);
  auto *mid = new wxBoxSizer(wxHORIZONTAL);
  p_text = new wxTextCtrl(report, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                          wxTE_MULTILINE | wxTE_READONLY);
  mid->Add(p_text, 9, wxEXPAND | wxALL, 5);
  p_plots = new wxNotebook(report, wxID_ANY);
  p_plot = new QsarPlot(p_plots);
  p_scree = new QsarScree(p_plots);
  p_plots->AddPage(p_plot, "Molecule plot");
  p_plots->AddPage(p_scree, "Variance explained");
  mid->Add(p_plots, 11, wxEXPAND | wxALL, 5);
  report->SetSizer(mid);
  p_pages->AddPage(report, "Summary and plots");
  p_components = gridPage(p_pages, "Components");
  p_molecules = gridPage(p_pages, "Molecules");
  p_vip = gridPage(p_pages, "VIP (PLS)");
  p_plot->selected = [this](int row) {
    p_molecules->ClearSelection();
    if (row >= 0 && row < p_molecules->GetNumberRows()) {
      p_molecules->SetGridCursor(row, 0); p_molecules->SelectRow(row);
      if (p_molecules->IsShownOnScreen()) p_molecules->MakeCellVisible(row, 0);
    }
  };
  top->Add(p_pages, 1, wxEXPAND | wxALL, 5);
  top->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALIGN_RIGHT | wxALL, 5);
  SetSizer(top);
  run->Bind(wxEVT_BUTTON, &QsarDialog::OnRun, this);
  runPls->Bind(wxEVT_BUTTON, &QsarDialog::OnRunPls, this);
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
  for (int i = 0; i < 4; ++i) if (scaling == scalings[i]) p_scale->SetSelection(i);
  p_comps->SetValue(components);
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
  loadPresentation(false);
  return true;
}

bool QsarDialog::runPls(const string& y, const string& scaling, int components,
                        int testPercent, int permutations, string& why)
{
  if (!p_ok) { why = p_why; return false; }
  for (int i = 0; i < 4; ++i) if (scaling == scalings[i]) p_scale->SetSelection(i);
  p_comps->SetValue(components);
  p_y->SetStringSelection(utf(y)); p_test->SetValue(testPercent); p_perm->SetValue(permutations);
  string args = "pls " + quote(p_table) + " --y " + quote(y) + " --scale " + scaling +
                " --outdir " + quote(p_dir) + " --save-model " + quote(p_dir + "/pls_model.json");
  if (components > 0) args += " --components " + std::to_string(components);
  if (testPercent > 0) args += " --test-fraction " + std::to_string(testPercent / 100.0);
  if (permutations > 0) args += " --permutations " + std::to_string(permutations);
  int st = 0;
  p_summary = run(args, st);
  if (p_text) p_text->SetValue(wxString::FromUTF8(p_summary.c_str()));
  if (st != 0) {
    const size_t e = p_summary.find("error: ");
    why = e == string::npos ? "ecce-qsar pls failed" :
        p_summary.substr(e + 7, p_summary.find('\n', e) - e - 7);
    return false;
  }
  loadPresentation(true, y);
  return true;
}

void QsarDialog::loadPresentation(bool pls, const string& measured)
{
  p_summary = readText(fs::path(p_dir) / "analysis_summary.txt");
  p_text->SetValue(utf(p_summary));
  fillGrid(p_components, readCsv(fs::path(p_dir) / "analysis_components.csv"));
  fillGrid(p_vip, pls ? readCsv(fs::path(p_dir) / "analysis_vip.csv") : vector<vector<string>>());
  auto data = readCsv(fs::path(p_dir) / (pls ? "pls_predictions.csv" : "pca_scores.csv"));
  auto comp = readCsv(fs::path(p_dir) / (pls ? "pls_components.csv" : "pca_components.csv"));
  p_plot->reset(); p_plot->parity = pls;
  p_scree->bars.assign(pls ? 3 : 1, {}); p_scree->lines.assign(pls ? 3 : 1, {});
  for (size_t i = 1; i < comp.size() && i <= 10; ++i) {
    for (int j = 0; j < (pls ? 3 : 1); ++j) {
      const int col = pls ? 1 + 2*j : 2;
      if (col + 1 >= int(comp[i].size())) continue;
      p_scree->bars[j].push_back(comp[i][col].empty() ? NAN : atof(comp[i][col].c_str()));
      p_scree->lines[j].push_back(comp[i][col+1].empty() ? NAN : atof(comp[i][col+1].c_str()));
    }
  }
  if (!data.empty()) {
    auto raw = data;
    if (pls) {
      data[0] = {"Molecule", "Set", "Observed " + measured, "Predicted " + measured,
                 "Cross-validated " + measured, "Residual " + measured};
      for (size_t i = 1; i < data.size(); ++i)
        if (data[i].size() >= 5)
          data[i].push_back(wxString::Format("%.6g", atof(data[i][2].c_str()) - atof(data[i][3].c_str())).ToStdString());
      auto slashTitle = measured;
      auto unit = slashTitle.find_last_of('(');
      if (unit != string::npos && slashTitle.back() == ')') {
        string u = slashTitle.substr(unit+1, slashTitle.size()-unit-2);
        slashTitle = slashTitle.substr(0, unit);
        while (!slashTitle.empty() && slashTitle.back() == ' ') slashTitle.pop_back();
        slashTitle += " / " + u;
      }
      p_plot->xTitle = "observed " + slashTitle; p_plot->yTitle = "predicted " + slashTitle;
      for (size_t i = 1; i < raw.size(); ++i) {
        if (raw[i].size() < 5) continue;
        double obs = atof(raw[i][2].c_str()), pred = atof(raw[i][3].c_str());
        string tip = raw[i][0] + "\nObserved " + measured + ": " + raw[i][2] +
          "\nPredicted " + measured + ": " + raw[i][3] +
          "\nResidual (observed - predicted): " + data[i][5];
        int kind = raw[i][1] == "test" ? 1 : 0;
        p_plot->points.push_back({raw[i][0], tip + (kind ? "\nTest set" : "\nFitted"), obs, pred, int(i-1), kind});
        if (!raw[i][4].empty()) {
          const double cv = atof(raw[i][4].c_str());
          const string cvtip = raw[i][0] + "\nObserved " + measured + ": " + raw[i][2] +
            "\nPredicted " + measured + ": " + raw[i][4] +
            "\nResidual (observed - predicted): " + wxString::Format("%.6g", obs-cv).ToStdString() + "\nCross-validated";
          p_plot->points.push_back({raw[i][0], cvtip, obs, cv, int(i-1), 2});
        }
      }
    } else {
      data[0][0] = "Molecule"; data[0][data[0].size()-2] = "T²";
      const bool second = raw[0].size() > 4;
      auto pct = [&](size_t i) {
        return i < p_scree->bars[0].size() ? wxString::Format("%.0f", p_scree->bars[0][i]*100).ToStdString() : string("0");
      };
      p_plot->xTitle = "t1 (R²X " + pct(0) + " %)";
      p_plot->yTitle = second ? "t2 (R²X " + pct(1) + " %)" : "t2 (not computed)";
      for (size_t i = 1; i < raw.size(); ++i) {
        if (raw[i].size() < 4) continue;
        string tip = raw[i][0] + "\nt1: " + raw[i][1] +
          "\nt2: " + (second ? raw[i][2] : string("not computed")) +
          "\nT²: " + raw[i][raw[i].size()-2] + "\nDModX: " + raw[i].back();
        p_plot->points.push_back({raw[i][0], tip, atof(raw[i][1].c_str()),
                                  second ? atof(raw[i][2].c_str()) : 0, int(i-1), 0});
      }
    }
  }
  if (pls && !data.empty()) {
    const auto scores = readCsv(fs::path(p_dir) / "pls_scores.csv");
    data[0].push_back("T²"); data[0].push_back("DModX");
    for (size_t i = 1; i < data.size(); ++i) {
      string t2, dmodx;
      for (size_t j = 1; j < scores.size(); ++j)
        if (scores[j].size() >= 4 && scores[j][0] == data[i][0]) {
          t2 = scores[j][scores[j].size()-2]; dmodx = scores[j].back(); break;
        }
      data[i].push_back(t2); data[i].push_back(dmodx);
    }
  }
  fillGrid(p_molecules, data);
  p_plot->Refresh(); p_scree->Refresh();
}

void QsarDialog::selectPresentationPage(int page)
{
  p_pages->SetSelection(page == 4 ? 0 : std::max(0, std::min(page, 3)));
  if (page == 0 || page == 4) p_plots->SetSelection(page == 4 ? 1 : 0);
  Layout();
}

bool QsarDialog::verifyPresentation(const string& dir, string& why)
{
  if (!p_components->GetNumberRows() || p_plot->points.empty()) {
    why = "no presentation data"; return false;
  }
  auto clipboard = []() {
    wxTextDataObject text;
    if (wxTheClipboard->Open()) { wxTheClipboard->GetData(text); wxTheClipboard->Close(); }
    return text.GetText();
  };
  bool copied = copyGrid(p_components, true);
  wxString whole = clipboard();
  bool ok = copied && whole == gridText(p_components, true);
  std::ofstream(fs::path(dir) / "presentation.tsv") << whole.ToStdString(wxConvUTF8);
  p_components->ClearSelection();
  p_components->SelectBlock(0, 0, 0, std::min(1, p_components->GetNumberCols()-1));
  wxKeyEvent key(wxEVT_CHAR_HOOK); key.m_keyCode = 'C'; key.SetControlDown(true);
  p_components->GetEventHandler()->ProcessEvent(key);
  wxString block = clipboard();
  const bool selection = block == gridText(p_components, false) && !block.empty();
  std::ofstream(fs::path(dir) / "selection.tsv") << block.ToStdString(wxConvUTF8);
  p_components->ClearSelection();
  selectPresentationPage(0); p_plot->Refresh(); p_plot->Update();
  std::ofstream hoverFile(fs::path(dir) / "hover.txt");
  for (const auto& p : p_plot->points) hoverFile << p.tip << "\n\n";
  const wxPoint pos = p_plot->position(0);
  const int hit = p_plot->hit(pos);
  const bool hover = hit >= 0 && p_plot->hover(pos) == p_plot->points[hit].tip;
  const int clicked = p_plot->click(pos);
  const bool click = clicked >= 0 && p_molecules->IsInSelection(p_plot->points[clicked].row, 0);
  selectPresentationPage(4); p_scree->Refresh(); p_scree->Update();
  selectPresentationPage(0);
  auto verdict = [](bool pass) { return pass ? "PASS" : "FAIL"; };
  std::ofstream(fs::path(dir) / "presentation-check.txt") <<
    "copy=" << verdict(ok) << "\nselection=" << verdict(selection) <<
    "\nhover=" << verdict(hover) << "\nclick=" << verdict(click) <<
    "\nplot-x=" << p_plot->xLabels << "\nplot-y=" << p_plot->yLabels <<
    "\nscree-y=" << p_scree->yLabels << "\n";
  ok = ok && selection && hover && click;
  if (!ok) why = "presentation clipboard, hover or click check failed";
  return ok;
}

void QsarDialog::OnRunPls(wxCommandEvent&)
{
  string why;
  if (!runPls(p_y->GetStringSelection().ToStdString(wxConvUTF8),
              scalings[std::max(0, p_scale->GetSelection())], p_comps->GetValue(),
              p_test->GetValue(), p_perm->GetValue(), why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "QSAR Analysis", wxOK | wxICON_ERROR, this);
}

bool QsarDialog::saveResults(const string& dir, string& why) const
{
  std::error_code ec;
  fs::create_directories(dir, ec);
  for (const char *f : {"table.csv", "pca_scores.csv", "pca_loadings.csv", "pca_components.csv",
                        "pls_components.csv", "pls_scores.csv", "pls_vip.csv",
                        "pls_predictions.csv", "pls_permutation.csv", "pls_model.json",
                        "analysis_components.csv", "analysis_vip.csv", "analysis_summary.txt"}) {
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
  if (!runPca(scalings[std::max(0, p_scale->GetSelection())], p_comps->GetValue(), why))
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
