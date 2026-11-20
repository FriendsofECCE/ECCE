// Density of States (OPDOS) panel (#171): TDOS, PDOS(A), PDOS(B), OPDOS(A,B).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dcmemory.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/filedlg.h>
#include <wx/html/htmlwin.h>
#include <wx/image.h>
#include <wx/link.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/stopwatch.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/wrapsizer.h>

#include "dsm/IPropCalculation.H"
#include "tdat/Fragment.H"
#include "tdat/ResultsFromCalc.H"
#include "tdat/ResultsH5.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/WindowShot.H"
#include "wxviz/SGViewer.H"
#include "MoPanel.H"
#include "OpdosPanel.H"
#include "XYPlotCanvas.H"

namespace {

wxString U(const char *utf8) { return wxString::FromUTF8(utf8); }

const char *SEMIEMPIRICAL_NOTE =
  "This calculation is semiempirical: its basis is treated as orthonormal (S = I), so the Mulliken overlap "
  "population between atoms is zero for every orbital and an OPDOS would be empty. A bond-order DOS is not "
  "available yet.";

}  // namespace

//  Dialog shown only once the work has taken half a second, so quick cases
//  never flash it; the message names the stage.
class OpdosProgress
{
  public:
    explicit OpdosProgress(wxWindow *parent) : p_parent(parent) {}
    ~OpdosProgress() { if (p_dlg) p_dlg->Destroy(); }
    void stage(const wxString& msg, double frac)
    {
      p_msg = msg;
      if (!p_dlg) {
        if (p_watch.Time() < 500) return;
        p_dlg = new ewxProgressDialog("Density of States", msg, 100, p_parent,
                                      wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH);
        p_dlg->Show();
      }
      p_dlg->Update(std::min(99, int(frac * 100)), msg);
    }
  private:
    wxWindow *p_parent;
    ewxProgressDialog *p_dlg = nullptr;
    wxString p_msg;
    wxStopWatch p_watch;
};

IMPLEMENT_DYNAMIC_CLASS(OpdosPanel, VizPropertyPanel)
wxFORCE_LINK_THIS_MODULE(OpdosPanel)

OpdosPanel::OpdosPanel() : VizPropertyPanel() {}

OpdosPanel::OpdosPanel(IPropCalculation *calculation, wxWindow *parent, wxWindowID id,
      const wxPoint& pos, const wxSize& size, long style, const wxString& name)
  : OpdosPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}

OpdosPanel::~OpdosPanel()
{
  unsubscribe();
  if (p_winFrame) {
    p_winFrame->DestroyChildren();
    p_winFrame->Destroy();
  }
}

bool OpdosPanel::Create(IPropCalculation *calculation, wxWindow *parent, wxWindowID id,
      const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name)) return false;
  SetSizer(new wxBoxSizer(wxVERTICAL));
  subscribe();
  return true;
}

bool OpdosPanel::isRelevant(IPropCalculation *propCalc) const
{
  return propCalc != 0 && propCalc->getProperty("MO") != 0 && propCalc->getProperty("ORBOCC") != 0;
}

void OpdosPanel::initialize() { build(); }

std::string OpdosPanel::atomText(const std::vector<int>& atoms)
{
  std::string s;
  for (size_t i = 0; i < atoms.size();) {
    size_t j = i;
    while (j + 1 < atoms.size() && atoms[j + 1] == atoms[j] + 1) j++;
    if (!s.empty()) s += ",";
    s += std::to_string(atoms[i] + 1);
    if (j > i) s += "-" + std::to_string(atoms[j] + 1);
    i = j + 1;
  }
  return s;
}

void OpdosPanel::build()
{
  wxSizer *top = GetSizer();
  OpdosProgress progress(this);
  progress.stage("Reading the stored results (results.h5)", 0.0);
  p_model = ResultsFromCalc::stored(getCalculation());
  auto refuse = [&](const wxString& msg) {
    wxStaticText *t = new wxStaticText(this, wxID_ANY, msg);
    t->Wrap(380);
    top->Add(t, 0, wxALL, 8);
  };
  if (!p_model || p_model->mos.empty()) {
    refuse("This calculation has no stored orbitals (results.h5), so the density of states cannot be drawn.");
    return;
  }
  if (p_model->basis.type != "GTO" || p_model->basis.orthonormal) {
    refuse(U(SEMIEMPIRICAL_NOTE));
    return;
  }
  p_ok = true;
  p_unres = p_model->mos.size() > 1;
  fragorb::defaultSplit(*p_model, p_a, p_b);

  const int RIGHT = wxALIGN_CENTER_VERTICAL | wxRIGHT;
  wxWrapSizer *row1 = new wxWrapSizer(wxHORIZONTAL), *rowMore = new wxWrapSizer(wxHORIZONTAL);
  //  Only the groups, Options, the window button and the plots stay in view.
  p_more = new wxPanel(this);
  p_more->SetSizer(rowMore);
  //  Each fragment is one column: label and text box, its Use selection button below.
  auto atomsBox = [&](wxTextCtrl *&box, wxButton *&use, const char *label, int which) {
    wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *line = new wxBoxSizer(wxHORIZONTAL);
    line->Add(new wxStaticText(this, wxID_ANY, label), 0, RIGHT, 4);
    box = new wxTextCtrl(this, wxID_ANY, atomText(which ? p_b : p_a), wxDefaultPosition,
                         wxSize(130, -1), wxTE_PROCESS_ENTER);
    box->SetToolTip("Atom numbers of this fragment as in the structure, e.g. 1,2,5-8; Enter applies");
    line->Add(box, 0, RIGHT, 0);
    use = new wxButton(this, wxID_ANY, "Use selection");
    use->SetToolTip("Take the atoms selected in the viewer");
    col->Add(line, 0, wxBOTTOM, 3);
    col->Add(use, 0, wxALIGN_RIGHT, 0);
    row1->Add(col, 0, wxRIGHT, 16);
    use->Bind(wxEVT_BUTTON, [this, which](wxCommandEvent&) { useSelection(which); });
    box->Bind(wxEVT_TEXT_ENTER, [this, which](wxCommandEvent&) {
      setAtoms(p_atomsA->GetValue().ToStdString(), p_atomsB->GetValue().ToStdString(), which);
    });
  };
  atomsBox(p_atomsA, p_useA, "A", 0);
  atomsBox(p_atomsB, p_useB, "B", 1);

  rowMore->Add(new wxStaticText(p_more, wxID_ANY, "Width (eV)"), 0, RIGHT, 4);
  p_width = new wxSpinCtrlDouble(p_more, wxID_ANY, "0.30", wxDefaultPosition, wxSize(130, -1),
                                 wxSP_ARROW_KEYS, 0.02, 3.0, 0.3, 0.05);
  p_width->SetMinSize(wxSize(130, -1));
  p_width->SetToolTip("Full width at half maximum of the line drawn for each orbital");
  rowMore->Add(p_width, 0, RIGHT, 12);
  rowMore->Add(new wxStaticText(p_more, wxID_ANY, "Shape"), 0, RIGHT, 4);
  const wxString shapes[] = {"Gaussian", "Lorentzian"};
  p_shape = new wxChoice(p_more, wxID_ANY, wxDefaultPosition, wxDefaultSize, 2, shapes);
  p_shape->SetSelection(0);
  p_shape->SetToolTip("Line shape of each orbital's peak; Lorentzian tails are long, so neighbouring peaks overlap more");
  rowMore->Add(p_shape, 0, RIGHT, 12);
  p_shape->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { redraw(); });
  p_width->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { redraw(); });
  p_width->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { redraw(); });
  auto check = [&](wxCheckBox *&c, const char *label, const char *tip, bool on) {
    c = new wxCheckBox(p_more, wxID_ANY, label);
    c->SetValue(on);
    c->SetToolTip(tip);
    rowMore->Add(c, 0, RIGHT, 12);
    return c;
  };
  check(p_sticks, "Sticks", "One line per orbital at its energy, height OPDOS contribution", false);
  check(p_int, "Integrated OPDOS", "Running integral of OPDOS in a second plot; at the HOMO it is the total A-B overlap population", true);
  check(p_occ, "Occupied only", "Leave out the empty orbitals (diffuse virtuals give large, cancelling Mulliken terms)", false);
  p_sticks->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { redraw(); });
  p_int->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { redraw(); });
  p_occ->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { redraw(); });
  rowMore->Add(new wxStaticText(p_more, wxID_ANY, "From (eV)"), 0, RIGHT, 4);
  p_from = new wxSpinCtrlDouble(p_more, wxID_ANY, "0", wxDefaultPosition, wxSize(110, -1),
                                wxSP_ARROW_KEYS, -2000, 2000, 0, 0.5);
  rowMore->Add(p_from, 0, RIGHT, 8);
  rowMore->Add(new wxStaticText(p_more, wxID_ANY, "To"), 0, RIGHT, 4);
  p_to = new wxSpinCtrlDouble(p_more, wxID_ANY, "0", wxDefaultPosition, wxSize(110, -1),
                              wxSP_ARROW_KEYS, -2000, 2000, 0, 0.5);
  p_from->SetToolTip("Lowest energy shown in both plots");
  p_to->SetToolTip("Highest energy shown in both plots");
  rowMore->Add(p_to, 0, RIGHT, 6);
  wxButton *reset = new wxButton(p_more, wxID_ANY, "Reset");
  reset->SetToolTip("Show the whole energy range again");
  rowMore->Add(reset, 0, RIGHT, 12);
  for (wxSpinCtrlDouble *w : {p_from, p_to}) {
    w->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { applyWindow(true); });
    w->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { applyWindow(true); });
  }
  reset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { resetWindow(); });
  wxButton *csv = new wxButton(p_more, wxID_ANY, "Export CSV");
  wxButton *png = new wxButton(p_more, wxID_ANY, "Export PNG");
  rowMore->Add(csv, 0, RIGHT, 6);
  rowMore->Add(png, 0, RIGHT, 6);
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    exportCsvDialog();
  });
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    exportPngDialog(p_plot, p_intPlot);
  });

  {
    wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
    p_moreButton = new wxButton(this, wxID_ANY, U("Options \xE2\x96\xB8"));
    p_moreButton->SetToolTip("Width, shape, sticks, integrated OPDOS, occupied only, energy range and export");
    p_windowButton = new wxButton(this, wxID_ANY, "Open in window...");
    p_windowButton->SetToolTip("Both plots in a window of their own that follows these settings");
    col->Add(p_moreButton, 0, wxBOTTOM, 3);
    col->Add(p_windowButton, 0, 0, 0);
    row1->Add(col, 0, wxRIGHT, 0);
    p_moreButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { setMoreOpen(!p_moreOpen); });
    p_windowButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { openWindow(); });
    ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
    if (cfg) cfg->Read("/Opdos/MoreOpen", &p_moreOpen, false);
  }
  top->Add(row1, 0, wxEXPAND | wxALL, 4);
  top->Add(p_more, 0, wxEXPAND | wxALL, 4);
  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(240, 170));
  //  The axis titles need 140 px of plot height.
  p_intPlot = new XYPlotCanvas(this);
  p_intPlot->SetMinSize(wxSize(240, 150));
  p_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 150),
                            wxHW_SCROLLBAR_AUTO | wxBORDER_SUNKEN);
  p_html->SetMinSize(wxSize(240, 110));
  p_html->SetBorders(6);
  p_html->SetBackgroundColour(*wxWHITE);
  p_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 52),
                          wxTE_MULTILINE | wxTE_READONLY);
  top->Add(p_plot, 3, wxEXPAND | wxALL, 4);
  top->Add(p_intPlot, 2, wxEXPAND | wxALL, 4);
  //  The MOs under a clicked peak; hidden until the first click.
  p_moList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 110),
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
  const char *cols[] = {"MO", "Spin", "E (eV)", "Occ.", "OP(A,B)", "A", "B"};
  for (int c = 0; c < 7; c++) p_moList->AppendColumn(cols[c], c > 1 ? wxLIST_FORMAT_RIGHT : wxLIST_FORMAT_LEFT, c == 1 ? 50 : 72);
  p_moList->SetToolTip("Orbitals under the clicked peak; double-click one to draw its isosurface");
  p_moList->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) { showListedMo(int(e.GetIndex())); });
  wxButton *draw = new wxButton(this, wxID_ANY, "Draw orbital");
  draw->SetToolTip("Draw the selected orbital's isosurface in the viewer, as the MOs panel does");
  draw->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    const long row = p_moList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    showListedMo(row >= 0 ? int(row) : 0);
  });
  p_listRow = new wxBoxSizer(wxHORIZONTAL);
  p_listRow->Add(p_moList, 1, wxEXPAND | wxRIGHT, 6);
  p_listRow->Add(draw, 0, wxALIGN_TOP, 0);
  top->Add(p_listRow, 0, wxEXPAND | wxALL, 4);
  top->Show(p_listRow, false);
  top->Add(p_html, 2, wxEXPAND | wxALL, 4);
  top->Add(p_text, 0, wxEXPAND | wxALL, 4);
  auto handler = [this](int c, int i) {
    XYPlotCanvas *src = p_plot->curveCount() > c && p_plot->pointCount(c) > i ? p_plot : p_intPlot;
    listMos(src->xAt(c, i));
  };
  p_plot->setPointHandler(handler);
  p_intPlot->setPointHandler([this](int c, int i) { listMos(p_intPlot->xAt(c, i)); });
  p_plot->setZeroLine(true);
  p_intPlot->setZeroLine(true);
  {
    ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
    bool on = false;
    double from = 0, to = 0;
    if (cfg) {
      cfg->Read("/Opdos/WindowOn", &on, false);
      cfg->Read("/Opdos/From", &from, 0.0);
      cfg->Read("/Opdos/To", &to, 0.0);
    }
    if (on && to > from) {
      p_winOn = true;
      p_from->SetValue(from);
      p_to->SetValue(to);
    }
  }
  syncMore();
  recomputeOrbitals(&progress);
  Layout();
}

void OpdosPanel::syncMore()
{
  p_moreButton->SetLabel(p_moreOpen ? U("Options \xE2\x96\xBE") : U("Options \xE2\x96\xB8"));
  p_more->Show(p_moreOpen);
  Layout();
}

void OpdosPanel::remember() const
{
  ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
  if (!cfg) return;
  cfg->Write("/Opdos/MoreOpen", p_moreOpen);
  cfg->Flush();
}

bool OpdosPanel::setMoreOpen(bool open)
{
  if (!p_ok) return false;
  p_moreOpen = open;
  syncMore();
  remember();
  return true;
}

void OpdosPanel::plotMessage(const wxString& msg)
{
  for (XYPlotCanvas *p : {p_plot, p_intPlot, p_winPlot, p_winInt}) {
    if (!p) continue;
    p->clearCurves();
    p->setMessage(p == p_plot || p == p_winPlot ? msg : wxString());
  }
}

void OpdosPanel::setText(const wxString& head)
{
  p_head = head;
  if (!p_text) return;
  p_text->SetValue(head + (head.empty() || p_clicked.empty() ? wxString() : "\n") + p_clicked);
  p_text->ShowPosition(0);
}

void OpdosPanel::recomputeOrbitals(OpdosProgress *progress)
{
  p_haveAll = false;
  p_clicked.clear();
  if (!p_ok || p_busy) return;
  //  The progress dialog pumps events: a click on a control must not start a second run.
  struct Busy { bool& f; Busy(bool& b) : f(b) { f = true; } ~Busy() { f = false; } } busyGuard(p_busy);
  OpdosProgress own(this);
  if (!progress) progress = &own;
  wxBusyCursor busy;
  std::string err;
  if (!p_haveS) {
    progress->stage("Building the orbitals: overlap matrix of the basis", 0.0);
    if (!fragorb::dosOverlap(*p_model, p_S, err)) {
      p_error = err;
      plotMessage(U(err.c_str()));
      setText(U(err.c_str()));
      return;
    }
    p_haveS = true;
  }
  fragorb::DosOptions opt;
  fragorb::AtomSet a, b;
  a.name = "A"; a.atoms = p_a;
  b.name = "B"; b.atoms = p_b;
  opt.sets.push_back(a);
  opt.sets.push_back(b);
  opt.pairs.push_back(std::make_pair(0, 1));
  opt.overlap = &p_S;
  opt.progress = [progress](double f) { progress->stage("Computing the overlap populations of the orbitals", f); };
  if (!fragorb::dosOrbitals(*p_model, opt, p_all, err)) {
    p_error = err;
    plotMessage(U(err.c_str()));
    setText(U(err.c_str()));
    return;
  }
  //  cTSc = 1 for every MO, as the MO diagram checks: a wrong AO order or
  //  normalisation for a code would make every number here wrong.
  const char *worstSpin = p_unres ? (p_all.normWorstSpin ? " beta" : " alpha") : "";
  if (p_all.normError > 1e-3) {
    char buf[300];
    snprintf(buf, sizeof buf, "The orbital coefficients do not match this basis (max |cTSc - 1| = %.3g, at MO %d%s), "
             "so no overlap populations are drawn.", p_all.normError, p_all.normWorstMo, worstSpin);
    p_error = buf;
    plotMessage(U(buf));
    setText(U(buf));
    return;
  }
  p_error.clear();
  p_haveAll = true;
  //  Small but above rounding of printed coefficients: drawn, with a warning.
  p_normNote.clear();
  if (p_all.normError > 1e-4)
    p_normNote = wxString::Format("Warning: the orbitals are not quite normalised in this basis "
                                  "(max |cTSc - 1| = %.2g, at MO %d%s); populations are approximate.",
                                  p_all.normError, p_all.normWorstMo, worstSpin);
  setText(p_moveNote.empty() ? p_normNote : p_moveNote + (p_normNote.empty() ? wxString() : "\n" + p_normNote));
  redraw();
}

void OpdosPanel::redraw()
{
  if (!p_ok || !p_haveAll) return;
  const bool occOnly = p_occ->GetValue();
  fragorb::DosOptions opt;
  opt.fwhmEv = p_width->GetValue();
  opt.lorentzian = p_shape->GetSelection() == 1;
  opt.emin = p_all.homoEv - 20;
  opt.emax = std::max(p_all.lumoEv, p_all.homoEv) + 10;
  opt.step = 0.05;
  const int nch = p_unres ? 2 : 1;
  std::string err;
  for (int s = 0; s < nch; s++)
    if (!fragorb::dosSpinPart(p_all, p_unres ? s : -1, occOnly, p_part[s], err) ||
        !fragorb::dosCurves(opt, p_part[s], err)) {
      p_plot->setMessage(U(err.c_str()));
      return;
    }
  drawCurves(p_plot, p_intPlot);
  if (p_winFrame) {
    drawCurves(p_winPlot, p_winInt);
    p_winFrame->Layout();
  }
  applyWindow(false);
  Layout();

  double occOp = 0;
  for (int s = 0; s < nch; s++) occOp += p_part[s].occupiedOverlap[0];
  showExplanation(occOp);
}

void OpdosPanel::drawCurves(XYPlotCanvas *plot, XYPlotCanvas *ip)
{
  const int nch = p_unres ? 2 : 1;
  plot->setMessage(wxString());
  plot->clearCurves();
  ip->clearCurves();
  plot->clearVLines();
  ip->clearVLines();
  const double homo = p_unres ? std::max(p_part[0].homoEv, p_part[1].homoEv) : p_part[0].homoEv;
  const double e0 = p_part[0].energy.front(), e1 = p_part[0].energy.back();
  //  Occupied region shaded, HOMO marked.
  if (plot == plot && !p_winOn) {
    p_from->SetValue(std::floor(e0 * 10) / 10);
    p_to->SetValue(std::ceil(e1 * 10) / 10);
  }
  plot->setShadeX(e0, homo);
  ip->setShadeX(e0, homo);
  plot->addVLine(homo, "HOMO");
  ip->addVLine(homo, "HOMO");
  plot->setXAxisLabel("Energy (eV)");
  ip->setXAxisLabel("Energy (eV)");
  plot->setYAxisLabel(p_unres ? U("states / eV (alpha up, beta down)") : U("states / eV"));
  ip->setYAxisLabel("Integrated OPDOS (electrons)");
  ip->setTitle(wxString());
  plot->setTitle("Density of states, Mulliken");
  if (plot == plot) ip->SetMinSize(wxSize(240, p_int->GetValue() ? 150 : 1));
  ip->Show(p_int->GetValue());
  plot->setReadout("Energy", "E", "eV");

  const size_t np = 1, ns = 2;
  int series = 0;
  const char *names[4] = {"TDOS", "PDOS(A)", "PDOS(B)", "OPDOS(A,B)"};
  for (int s = 0; s < nch; s++) {
    const fragorb::DosResult& r = p_part[s];
    const double sign = s ? -1.0 : 1.0;
    const size_t n = r.energy.size();
    std::vector<double> y(n);
    for (int k = 0; k < 4; k++) {
      for (size_t g = 0; g < n; g++)
        y[g] = sign * (k == 0 ? r.tdos[g] : k < 3 ? r.pdos[g * ns + (k - 1)] : r.opdos[g * np]);
      wxString nm = names[k];
      if (p_unres) nm += s ? " beta" : " alpha";
      const int c = plot->addCurve(r.energy, y, nm, k + (s ? 0 : 0), XYPlotCanvas::LINE);
      if (s) plot->setCurveStyle(c, true, 1);
      if (k == 0) plot->setCurveColour(c, wxColour(0, 0, 0));
      series++;
    }
    if (p_sticks->GetValue()) {
      std::vector<double> sx, sy;
      for (size_t i = 0; i < r.moEnergy.size(); i++) {
        const double v = sign * r.moWeight[i] * r.moOverlap[i * np];
        sx.push_back(r.moEnergy[i]); sy.push_back(0);
        sx.push_back(r.moEnergy[i]); sy.push_back(v);
        sx.push_back(r.moEnergy[i]); sy.push_back(0);
      }
      const int c = plot->addCurve(sx, sy, wxString(), 3, XYPlotCanvas::LINE);
      plot->setCurveThin(c, true);
      if (s) plot->setCurveStyle(c, false, 1);
    }
    if (p_int->GetValue()) {
      std::vector<double> yi(n);
      for (size_t g = 0; g < n; g++) yi[g] = sign * r.intOpdos[g * np];
      const int c = ip->addCurve(r.energy, yi, wxString(p_unres ? (s ? "beta" : "alpha") : "integrated OPDOS(A,B)"), 3, XYPlotCanvas::LINE);
      if (s) ip->setCurveStyle(c, true, 1);
    }
  }
  plot->setShowKey(true);

}

void OpdosPanel::applyWindow(bool remember)
{
  if (!p_from || !p_plot) return;
  const double lo = p_from->GetValue(), hi = p_to->GetValue();
  if (remember) {
    if (!(hi > lo)) { setText("To must be above From."); return; }
    p_winOn = true;
  }
  const bool on = p_winOn && hi > lo;
  for (XYPlotCanvas *p : {p_plot, p_intPlot, p_winPlot, p_winInt})
    if (p) p->setXWindow(on ? lo : 0, on ? hi : 0);
  if (remember) {
    ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
    if (cfg) {
      cfg->Write("/Opdos/WindowOn", p_winOn);
      cfg->Write("/Opdos/From", lo);
      cfg->Write("/Opdos/To", hi);
      cfg->Flush();
    }
  }
}

bool OpdosPanel::setWindow(double from, double to)
{
  if (!p_ok || !p_haveAll || !(to > from)) return false;
  p_from->SetValue(from);
  p_to->SetValue(to);
  applyWindow(true);
  return true;
}

bool OpdosPanel::resetWindow()
{
  if (!p_ok) return false;
  p_winOn = false;
  if (p_haveAll) {
    p_from->SetValue(std::floor(p_part[0].energy.front() * 10) / 10);
    p_to->SetValue(std::ceil(p_part[0].energy.back() * 10) / 10);
  }
  applyWindow(false);
  ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
  if (cfg) { cfg->Write("/Opdos/WindowOn", false); cfg->Flush(); }
  return true;
}

//  The explanation: formula, definitions, the numbers of this run, the caution.
void OpdosPanel::showExplanation(double occOp)
{
  char num[64];
  snprintf(num, sizeof num, "%.4f", occOp);
  char err[64];
  snprintf(err, sizeof err, "%.1e", p_all.normError);
  const std::string a = atomText(p_a), b = atomText(p_b);
  std::string h =
    "<p><b>Population scheme: Mulliken</b>, for OPDOS and PDOS.</p>"
    "<p>OPDOS(A,B) = &#931;<sub>i</sub> 2 &#931;<sub>&#956;&#8712;A, &#957;&#8712;B</sub> "
    "c<sub>&#956;i</sub> c<sub>&#957;i</sub> S<sub>&#956;&#957;</sub> g(E &#8722; &#949;<sub>i</sub>)</p>"
    "<p>i: molecular orbital, &#949;<sub>i</sub> its energy<br>"
    "c<sub>&#956;i</sub>: coefficient of basis function &#956; in orbital i<br>"
    "S<sub>&#956;&#957;</sub>: overlap of basis functions &#956; and &#957;<br>"
    "g: " + std::string(p_shape->GetSelection() == 1 ? "Lorentzian" : "Gaussian") + " of the chosen width (FWHM), area 1<br>"
    "PDOS(X): Mulliken gross population of X in each orbital<br>"
    "Unit: states per eV; each closed-shell orbital counts twice. "
    "Positive OPDOS is bonding between A and B, negative antibonding.</p>"
    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">"
    "<tr><td>A</td><td>atoms " + a + "</td></tr>"
    "<tr><td>B</td><td>atoms " + b + "</td></tr>"
    "<tr><td>Occupied A&#8211;B overlap population</td><td>" + num + " electrons</td></tr>"
    "<tr><td>max |C<sup>T</sup>SC &#8722; 1|</td><td>" + err + "</td></tr></table>";
  if (p_unres) h += "<p>Open shell: alpha channel up, beta channel down.</p>";
  h += "<table width=\"100%\" bgcolor=\"#fff1c2\" cellpadding=\"4\"><tr><td><b>Caution:</b> "
       "virtual orbitals in a diffuse basis have large Mulliken overlap populations of both signs that cancel; "
       "read the empty region with care or tick Occupied only.</td></tr></table>";
  p_html->SetPage("<html><body>" + wxString::FromUTF8(h.c_str()) + "</body></html>");
  p_headPlain = "Population scheme: Mulliken. OPDOS(A,B) = sum_i 2 sum(mu in A, nu in B) c S g(E - e_i); "
                "A = atoms " + wxString(a) + ", B = atoms " + wxString(b) + "; occupied A-B overlap population " + num +
                "; max |CTSC - 1| " + err + ". Caution: diffuse virtual orbitals give large cancelling overlap populations.";
}

void OpdosPanel::listMos(double ev)
{
  if (!p_ok || !p_haveAll) return;
  const double w = p_width->GetValue();
  struct Row { double d; int idx, ch; };
  std::vector<Row> rows;
  const int nch = p_unres ? 2 : 1;
  for (int s = 0; s < nch; s++) {
    const fragorb::DosResult& r = p_part[s];
    for (size_t i = 0; i < r.moEnergy.size(); i++)
      if (std::fabs(r.moEnergy[i] - ev) <= w) {
        Row x = {std::fabs(r.moEnergy[i] - ev), int(i), s};
        rows.push_back(x);
      }
  }
  std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.d < b.d; });
  p_listed.clear();
  p_moList->DeleteAllItems();
  for (size_t k = 0; k < rows.size() && k < 40; k++) {
    const fragorb::DosResult& r = p_part[rows[k].ch];
    const int i = rows[k].idx;
    const long it = p_moList->InsertItem(long(k), wxString::Format("%d", r.moIndex[i]));
    p_moList->SetItem(it, 1, p_unres ? (rows[k].ch ? "beta" : "alpha") : "");
    p_moList->SetItem(it, 2, wxString::Format("%.3f", r.moEnergy[i]));
    p_moList->SetItem(it, 3, wxString::Format("%.2f", r.moOccupation[i]));
    p_moList->SetItem(it, 4, wxString::Format("%+.4f", r.moOverlap[i]));
    p_moList->SetItem(it, 5, wxString::Format("%.3f", r.moGross[i * 2]));
    p_moList->SetItem(it, 6, wxString::Format("%.3f", r.moGross[i * 2 + 1]));
    ListedMo m = {r.moIndex[i], p_unres ? rows[k].ch : 0};
    p_listed.push_back(m);
  }
  if (!p_listed.empty()) p_moList->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  GetSizer()->Show(p_listRow, !p_listed.empty());
  Layout();
  p_clicked = wxString::Format("MOs within %.2f eV of %.3f eV: %s", w, ev,
      p_listed.empty() ? wxString("none") : wxString::Format("%zu, listed below; double-click one to draw it", p_listed.size()));
  setText(p_head);
}

bool OpdosPanel::showListedMo(int row)
{
  if (!p_ok || row < 0 || row >= int(p_listed.size())) return false;
  const ListedMo m = p_listed[row];
  const wxString name = wxString::Format("MO %d%s", m.mo, p_unres ? (m.spin ? " beta" : " alpha") : "");
  //  The same path as a click on an MO-diagram level: the MOs panel computes the surface.
  MoPanel *mo = nullptr;
  std::set<PropertyPanel*> panels = PropertyPanel::getPanels(getCalculation()->getURL().toString(), "MOs", getOwner());
  for (std::set<PropertyPanel*>::iterator it = panels.begin(); it != panels.end() && !mo; ++it)
    mo = dynamic_cast<MoPanel*>(*it);
  p_drawn.clear();
  if (!mo) {
    p_clicked = "Open the MOs panel (Properties menu) to draw " + name + ".";
  } else if (!mo->showOrbital(m.mo - 1, m.spin)) {
    p_clicked = name + " cannot be drawn: the MOs panel has no basis set or no coefficients for it.";
  } else {
    p_drawn = name;
    p_clicked = name + " drawn in the viewer (MOs panel).";
  }
  setText(p_head);
  return !p_drawn.empty();
}

bool OpdosPanel::clickEnergy(double ev)
{
  if (!p_ok || !p_haveAll) return false;
  listMos(ev);
  return true;
}

bool OpdosPanel::setAtoms(const std::string& a, const std::string& b, int winner)
{
  if (!p_ok || p_busy) return false;
  std::vector<int> v[2];
  const std::string text[2] = {a, b};
  const int n = int(p_model->natoms());
  for (int g = 0; g < 2; g++) {
    //  An emptied box is an empty group, reported below; only junk is refused.
    if (text[g].find_first_not_of(" ,") == std::string::npos) continue;
    if (!fragorb::parseAtomList(text[g], n, v[g])) {
      setText("Atom lists are numbers and ranges such as 1,2,5-8, each between 1 and the number of atoms.");
      return false;
    }
  }
  //  The group that was just set keeps an atom both name; the other loses it.
  const int win = winner ? 1 : 0, lose = 1 - win;
  std::vector<int> moved, kept;
  for (int x : v[lose]) (std::binary_search(v[win].begin(), v[win].end(), x) ? moved : kept).push_back(x);
  v[lose] = kept;
  p_a = v[0];
  p_b = v[1];
  p_atomsA->SetValue(atomText(p_a));
  p_atomsB->SetValue(atomText(p_b));
  p_moveNote.clear();
  if (!moved.empty())
    p_moveNote = wxString::Format("Atoms %s moved from %s to %s.", atomText(moved), lose ? "B" : "A", win ? "B" : "A");
  if (p_a.empty() || p_b.empty()) {
    const wxString which = p_a.empty() && p_b.empty() ? "Groups A and B are" : p_a.empty() ? "Group A is" : "Group B is";
    p_haveAll = false;
    p_clicked.clear();
    p_error = "empty group";
    plotMessage(which + " empty; put atoms in it to draw the density of states.");
    setText((p_moveNote.empty() ? wxString() : p_moveNote + "\n") + which + " empty.");
    return true;
  }
  recomputeOrbitals();
  return p_haveAll;
}

bool OpdosPanel::useSelection(int which)
{
  if (!p_ok || !p_vizfw || p_busy) return false;
  SGFragment *frag = getFW().getSceneGraph().getFragment();
  if (!frag) return false;
  std::vector<int> picked;
  for (size_t i = 0; i < frag->m_atomHighLight.size(); i++) {
    const int a = frag->m_atomHighLight[i];
    if (a >= 0 && a < int(p_model->natoms()) && frag->atomRef(a)->atomicSymbol() != "Nub") picked.push_back(a);
  }
  if (picked.empty()) {
    setText("Select atoms in the viewer first.");
    return false;
  }
  std::sort(picked.begin(), picked.end());
  //  The other group as typed in its box, not as last applied: "A = 1" typed
  //  without Enter, then Use selection for B, collided with A's old atoms.
  const std::string ta = p_atomsA->GetValue().ToStdString();
  const std::string tb = p_atomsB->GetValue().ToStdString();
  return setAtoms(which ? ta : atomText(picked), which ? atomText(picked) : tb, which);
}

bool OpdosPanel::setWidth(double ev)
{
  if (!p_ok || ev < 0.02 || ev > 3.0) return false;
  p_width->SetValue(ev);
  redraw();
  return true;
}
bool OpdosPanel::setSticks(bool on) { if (!p_ok) return false; p_sticks->SetValue(on); redraw(); return true; }
bool OpdosPanel::setOccupiedOnly(bool on) { if (!p_ok) return false; p_occ->SetValue(on); redraw(); return true; }
bool OpdosPanel::setIntegrated(bool on) { if (!p_ok) return false; p_int->SetValue(on); redraw(); return true; }
bool OpdosPanel::setLineShape(bool lorentzian) { if (!p_ok) return false; p_shape->SetSelection(lorentzian ? 1 : 0); redraw(); return true; }

bool OpdosPanel::savePng(const wxString& path, int width, int height)
{
  return savePngOf(p_plot, p_intPlot, path, width, height);
}

bool OpdosPanel::savePngOf(XYPlotCanvas *plot, XYPlotCanvas *intPlot, const wxString& path, int width, int height)
{
  if (!p_ok || !p_haveAll) return false;
  static bool handlers = false;
  if (!handlers) { wxInitAllImageHandlers(); handlers = true; }
  const bool both = p_int->GetValue();
  const int h1 = both ? height * 3 / 4 : height;
  wxBitmap bmp(width, height);
  {
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    plot->paintOnto(dc, wxSize(width, h1));
    if (both) {
      wxBitmap lower(width, height - h1);
      {
        wxMemoryDC d2(lower);
        d2.SetBackground(*wxWHITE_BRUSH);
        d2.Clear();
        intPlot->paintOnto(d2, wxSize(width, height - h1));
      }
      dc.DrawBitmap(lower, 0, h1);
    }
  }
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

bool OpdosPanel::exportCsv(const wxString& path)
{
  if (!p_ok || !p_haveAll) return false;
  std::ofstream out(path.ToStdString());
  if (!out) return false;
  const int nch = p_unres ? 2 : 1;
  out << "# Mulliken populations; A=" << atomText(p_a) << " B=" << atomText(p_b)
      << "; width " << p_width->GetValue() << " eV (FWHM, " << (p_shape->GetSelection() == 1 ? "Lorentzian" : "Gaussian")
      << "); states/eV\nE_eV";
  for (int s = 0; s < nch; s++) {
    const char *tag = p_unres ? (s ? "_beta" : "_alpha") : "";
    out << ",TDOS" << tag << ",PDOS_A" << tag << ",PDOS_B" << tag << ",OPDOS_AB" << tag << ",intOPDOS_AB" << tag;
  }
  out << "\n";
  char buf[200];
  for (size_t g = 0; g < p_part[0].energy.size(); g++) {
    snprintf(buf, sizeof buf, "%.4f", p_part[0].energy[g]);
    out << buf;
    for (int s = 0; s < nch; s++) {
      const fragorb::DosResult& r = p_part[s];
      snprintf(buf, sizeof buf, ",%.6g,%.6g,%.6g,%.6g,%.6g", r.tdos[g], r.pdos[g * 2], r.pdos[g * 2 + 1], r.opdos[g], r.intOpdos[g]);
      out << buf;
    }
    out << "\n";
  }
  return true;
}

void OpdosPanel::exportCsvDialog()
{
  wxFileDialog d(this, "Export curves", "", "opdos.csv", "CSV (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (d.ShowModal() == wxID_OK) exportCsv(d.GetPath());
}

void OpdosPanel::exportPngDialog(XYPlotCanvas *plot, XYPlotCanvas *intPlot)
{
  wxFileDialog d(this, "Export plot", "", "opdos.png", "PNG (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (d.ShowModal() == wxID_OK) savePngOf(plot, intPlot, d.GetPath(), 1200, 800);
}

//  Second canvases drawn by the same code as the panel's, so the window
//  always shows the panel's settings; a child of the Builder's frame.
bool OpdosPanel::openWindow()
{
  if (!p_ok) return false;
  if (p_winFrame) {
    p_winFrame->Raise();
    return true;
  }
  wxFrame *frame = new wxFrame(wxGetTopLevelParent(this), wxID_ANY, "Density of States (OPDOS)",
                               wxDefaultPosition, wxSize(1000, 700));
  wxPanel *panel = new wxPanel(frame);
  p_winPlot = new XYPlotCanvas(panel);
  p_winPlot->SetMinSize(wxSize(240, 170));
  p_winInt = new XYPlotCanvas(panel);
  p_winInt->SetMinSize(wxSize(240, 150));
  for (XYPlotCanvas *p : {p_winPlot, p_winInt}) p->setZeroLine(true);
  p_winPlot->setPointHandler([this](int c, int i) { listMos(p_winPlot->xAt(c, i)); });
  p_winInt->setPointHandler([this](int c, int i) { listMos(p_winInt->xAt(c, i)); });
  wxButton *csv = new wxButton(panel, wxID_ANY, "Export CSV");
  wxButton *png = new wxButton(panel, wxID_ANY, "Export PNG");
  wxButton *close = new wxButton(panel, wxID_ANY, "Close");
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportCsvDialog(); });
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportPngDialog(p_winPlot, p_winInt); });
  close->Bind(wxEVT_BUTTON, [frame](wxCommandEvent&) { frame->Close(); });
  wxBoxSizer *foot = new wxBoxSizer(wxHORIZONTAL);
  foot->AddStretchSpacer(1);
  foot->Add(csv, 0, wxALL, 4);
  foot->Add(png, 0, wxALL, 4);
  foot->Add(close, 0, wxALL, 4);
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(p_winPlot, 3, wxEXPAND | wxALL, 4);
  sizer->Add(p_winInt, 2, wxEXPAND | wxALL, 4);
  sizer->Add(foot, 0, wxEXPAND);
  panel->SetSizer(sizer);
  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  outer->Add(panel, 1, wxEXPAND);
  frame->SetSizer(outer);
  //  The canvases go with the frame; the pointers must not outlive it.
  frame->Bind(wxEVT_CLOSE_WINDOW, [this, frame](wxCloseEvent&) {
    p_winPlot = p_winInt = nullptr;
    frame->Destroy();
  });
  p_winFrame = frame;
  frame->Show(true);
  if (p_haveAll) {
    drawCurves(p_winPlot, p_winInt);
    applyWindow(false);
  } else {
    p_winPlot->setMessage(p_plot->message());
  }
  frame->Layout();
  return true;
}

void OpdosPanel::closeWindow()
{
  if (!p_winFrame) return;
  p_winFrame->Close();
}

bool OpdosPanel::saveWindowShot(const wxString& path)
{
  if (!p_winFrame) return false;
  //  With no window manager a new frame is not yet mapped on top: let the
  //  events through before the grab.
  p_winFrame->Raise();
  p_winFrame->Layout();
  for (int i = 0; i < 5; i++) {
    wxMilliSleep(100);
    wxYield();
  }
  p_winFrame->Refresh();
  p_winFrame->Update();
  return ecceClientShot(p_winFrame, path);
}

//  The visible controls and their rectangles at a panel width, for the layout test.
std::string OpdosPanel::layoutReport(int width)
{
  if (!p_ok) return "layout none\n";
  ensureInitialized();
  GetSizer()->SetDimension(0, 0, width, 760);
  const std::pair<const char*, wxWindow*> items[] = {
    {"atomsA", p_atomsA}, {"useA", p_useA}, {"atomsB", p_atomsB}, {"useB", p_useB},
    {"options", p_moreButton}, {"window", p_windowButton}, {"width", p_width}, {"shape", p_shape},
    {"sticks", p_sticks}, {"integrated", p_int}, {"occupied", p_occ}, {"from", p_from}, {"to", p_to},
    {"plot", p_plot}, {"intplot", p_intPlot}};
  std::string out = "layout width " + std::to_string(width) + "\n", rows;
  bool clipped = false;
  for (const auto& it : items) {
    wxWindow *w = it.second;
    const bool inMore = w->GetParent() == p_more;
    if (!w->IsShown() || (inMore && !p_more->IsShown())) continue;
    wxRect r = w->GetRect();
    if (inMore) r.Offset(p_more->GetPosition());
    const bool bad = r.x < 0 || r.GetRight() >= width;
    clipped = clipped || bad;
    char buf[160];
    snprintf(buf, sizeof buf, "ctl %s x=%d y=%d w=%d h=%d %s\n", it.first, r.x, r.y, r.width, r.height, bad ? "clipped" : "ok");
    out += buf;
    rows += std::string(rows.empty() ? "" : ",") + it.first;
  }
  out += "visible " + rows + "\n";
  out += clipped ? "layout clipped\n" : "layout ok\n";
  Layout();
  return out;
}

std::string OpdosPanel::state() const
{
  if (!p_ok) return "ok 0\n";
  std::string s = "ok 1\n";
  char buf[512];
  snprintf(buf, sizeof buf, "ready %d\nerror %s\nnatoms %zu\nA %s\nB %s\nunrestricted %d\n",
           p_haveAll ? 1 : 0, p_error.empty() ? "-" : p_error.c_str(), p_model->natoms(),
           atomText(p_a).c_str(), atomText(p_b).c_str(), p_unres ? 1 : 0);
  s += buf;
  snprintf(buf, sizeof buf, "options_open %d\nwindow_open %d\n", p_moreOpen ? 1 : 0, p_winFrame ? 1 : 0);
  s += buf;
  if (p_winFrame && p_winPlot) {
    snprintf(buf, sizeof buf, "window_plots 2\nwindow_size %d %d\nwindow_curves %d\nwindow_intcurves %d\nwindow_int_shown %d\n",
             p_winFrame->GetSize().x, p_winFrame->GetSize().y, p_winPlot->curveCount(), p_winInt->curveCount(),
             p_winInt->IsShown() ? 1 : 0);
    s += buf;
  }
  if (!p_haveAll) return s + "text " + std::string(p_text ? p_text->GetValue().ToUTF8().data() : "") + "\n";
  {
    //  Positions inside the panel, for layout checks.
    auto where = [this](const char *name, const wxWindow *w) {
      const wxPoint q = ScreenToClient(w->GetScreenPosition());
      return wxString::Format("%s %d %d %d %d\n", name, q.x, q.y, w->GetSize().x, w->GetSize().y).ToStdString();
    };
    s += where("atomsA_box", p_atomsA) + where("atomsB_box", p_atomsB) + where("useA_button", p_useA) +
         where("useB_button", p_useB) + where("plot_box", p_plot) + where("intplot_box", p_intPlot) +
         where("explanation_box", p_html) + where("status_box", p_text) + where("from_box", p_from);
    double lo = 0, hi = 0;
    snprintf(buf, sizeof buf, "window_on %d\nfrom_ev %.3f\nto_ev %.3f\n", p_winOn ? 1 : 0, p_from->GetValue(), p_to->GetValue());
    s += buf;
    const bool w1 = p_plot->xWindow(&lo, &hi);
    double lo2 = 0, hi2 = 0;
    const bool w2 = p_intPlot->xWindow(&lo2, &hi2);
    snprintf(buf, sizeof buf, "plot_window %d %.3f %.3f\nintplot_window %d %.3f %.3f\n", w1 ? 1 : 0, lo, hi, w2 ? 1 : 0, lo2, hi2);
    s += buf;
    s += "plot_xlabel " + std::string(p_plot->xAxisLabel().ToUTF8().data()) + "\n";
    s += "plot_ylabel " + std::string(p_plot->yAxisLabel().ToUTF8().data()) + "\n";
    s += "intplot_xlabel " + std::string(p_intPlot->xAxisLabel().ToUTF8().data()) + "\n";
    s += "intplot_ylabel " + std::string(p_intPlot->yAxisLabel().ToUTF8().data()) + "\n";
  }
  const int nch = p_unres ? 2 : 1;
  const double step = p_part[0].energy.size() > 1 ? p_part[0].energy[1] - p_part[0].energy[0] : 0;
  snprintf(buf, sizeof buf, "width %.3f\nshape %s\nsticks %d\noccupied_only %d\nnorm_error %.3e\nnorm_worst %d %d\ncurves %d\n",
           p_width->GetValue(), p_shape->GetSelection() == 1 ? "lorentzian" : "gaussian", p_sticks->GetValue() ? 1 : 0,
           p_occ->GetValue() ? 1 : 0, p_all.normError, p_all.normWorstMo, p_all.normWorstSpin, p_plot->curveCount());
  s += buf;
  //  Each named curve's range: beta drawn downwards shows as max <= 0.
  for (XYPlotCanvas *pl : {p_plot, p_intPlot})
    for (int c = 0; c < pl->curveCount(); c++) {
      if (pl->curveName(c).empty()) continue;
      double lo = 0, hi = 0;
      for (int i = 0; i < pl->pointCount(c); i++) { lo = std::min(lo, pl->yAt(c, i)); hi = std::max(hi, pl->yAt(c, i)); }
      snprintf(buf, sizeof buf, "%s %s|%.4f|%.4f\n", pl == p_plot ? "curve" : "intcurve",
               pl->curveName(c).ToStdString().c_str(), lo, hi);
      s += buf;
    }
  snprintf(buf, sizeof buf, "list_rows %zu\nlist_shown %d\ndrawn %s\n", p_listed.size(),
           p_moList && p_moList->IsShown() ? 1 : 0, p_drawn.empty() ? "-" : p_drawn.ToStdString().c_str());
  s += buf;
  for (size_t k = 0; k < p_listed.size(); k++) {
    snprintf(buf, sizeof buf, "list_row%zu %s|%s|%s|%s|%s\n", k,
             p_moList->GetItemText(long(k), 0).ToStdString().c_str(), p_moList->GetItemText(long(k), 1).ToStdString().c_str(),
             p_moList->GetItemText(long(k), 2).ToStdString().c_str(), p_moList->GetItemText(long(k), 3).ToStdString().c_str(),
             p_moList->GetItemText(long(k), 4).ToStdString().c_str());
    s += buf;
  }
  for (int c = 0; c < nch; c++) {
    const fragorb::DosResult& r = p_part[c];
    double it = 0, ia = 0, ib = 0, io = 0;
    for (size_t g = 0; g < r.energy.size(); g++) {
      it += r.tdos[g] * step; ia += r.pdos[g * 2] * step; ib += r.pdos[g * 2 + 1] * step; io += r.opdos[g] * step;
    }
    const double e0 = r.energy.front();
    size_t gh = size_t(std::floor((r.homoEv + 0.5 * (r.lumoEv - r.homoEv) - e0) / step + 0.5));
    if (gh >= r.energy.size()) gh = r.energy.size() - 1;
    snprintf(buf, sizeof buf,
             "channel%d tdos_int %.4f pdosA_int %.4f pdosB_int %.4f opdos_int %.6f intopdos_at_homo %.6f occupied_op %.6f nmo %zu homo_ev %.4f\n",
             c, it, ia, ib, io, r.intOpdos[gh], r.occupiedOverlap[0], r.moEnergy.size(), r.homoEv);
    s += buf;
  }
  s += "text " + std::string(p_headPlain.ToUTF8().data()) + "\n" + std::string(p_text ? p_text->GetValue().ToUTF8().data() : "") + "\n";
  return s;
}
