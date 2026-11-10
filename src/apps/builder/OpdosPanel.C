// Density of States (OPDOS) panel (#171): TDOS, PDOS(A), PDOS(B), OPDOS(A,B).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/html/htmlwin.h>
#include <wx/image.h>
#include <wx/link.h>
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
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxviz/SGViewer.H"
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

OpdosPanel::~OpdosPanel() { unsubscribe(); }

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
  wxWrapSizer *row1 = new wxWrapSizer(wxHORIZONTAL), *row2 = new wxWrapSizer(wxHORIZONTAL);
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
    //  Typing one group against the other's applied atoms would collide: the
    //  untouched group gives up the atoms the edited one takes.
    box->Bind(wxEVT_TEXT_ENTER, [this, which](wxCommandEvent&) {
      const std::string ta = p_atomsA->GetValue().ToStdString(), tb = p_atomsB->GetValue().ToStdString();
      std::vector<int> va, vb;
      const size_t n = p_model->natoms();
      if (fragorb::parseAtomList(ta, n, va) && fragorb::parseAtomList(tb, n, vb)) {
        std::vector<int>& other = which ? va : vb;
        const std::vector<int>& mine = which ? vb : va;
        const bool otherUntouched = which ? ta == atomText(p_a) : tb == atomText(p_b);
        std::vector<int> keep;
        for (size_t i = 0; i < other.size(); i++)
          if (!std::binary_search(mine.begin(), mine.end(), other[i])) keep.push_back(other[i]);
        if (otherUntouched && !keep.empty() && keep.size() < other.size()) {
          other = keep;
          const bool ok = setAtoms(atomText(va), atomText(vb));
          if (ok) setText(wxString::Format("Atoms taken by %s were removed from %s.", which ? "B" : "A", which ? "A" : "B"));
          return;
        }
      }
      setAtoms(ta, tb);
    });
  };
  atomsBox(p_atomsA, p_useA, "A", 0);
  atomsBox(p_atomsB, p_useB, "B", 1);

  row2->Add(new wxStaticText(this, wxID_ANY, "Width (eV)"), 0, RIGHT, 4);
  p_width = new wxSpinCtrlDouble(this, wxID_ANY, "0.30", wxDefaultPosition, wxSize(130, -1),
                                 wxSP_ARROW_KEYS, 0.02, 3.0, 0.3, 0.05);
  p_width->SetMinSize(wxSize(130, -1));
  p_width->SetToolTip("Full width at half maximum of the Gaussian drawn for each orbital");
  row2->Add(p_width, 0, RIGHT, 12);
  p_width->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { redraw(); });
  p_width->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { redraw(); });
  auto check = [&](wxCheckBox *&c, const char *label, const char *tip, bool on) {
    c = new wxCheckBox(this, wxID_ANY, label);
    c->SetValue(on);
    c->SetToolTip(tip);
    row2->Add(c, 0, RIGHT, 12);
    return c;
  };
  check(p_sticks, "Sticks", "One line per orbital at its energy, height OPDOS contribution", false);
  check(p_int, "Integrated OPDOS", "Running integral of OPDOS in a second plot; at the HOMO it is the total A-B overlap population", true);
  check(p_occ, "Occupied only", "Leave out the empty orbitals (diffuse virtuals give large, cancelling Mulliken terms)", false);
  p_sticks->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { redraw(); });
  p_int->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { redraw(); });
  p_occ->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { redraw(); });
  row2->Add(new wxStaticText(this, wxID_ANY, "From (eV)"), 0, RIGHT, 4);
  p_from = new wxSpinCtrlDouble(this, wxID_ANY, "0", wxDefaultPosition, wxSize(110, -1),
                                wxSP_ARROW_KEYS, -2000, 2000, 0, 0.5);
  row2->Add(p_from, 0, RIGHT, 8);
  row2->Add(new wxStaticText(this, wxID_ANY, "To"), 0, RIGHT, 4);
  p_to = new wxSpinCtrlDouble(this, wxID_ANY, "0", wxDefaultPosition, wxSize(110, -1),
                              wxSP_ARROW_KEYS, -2000, 2000, 0, 0.5);
  p_from->SetToolTip("Lowest energy shown in both plots");
  p_to->SetToolTip("Highest energy shown in both plots");
  row2->Add(p_to, 0, RIGHT, 6);
  wxButton *reset = new wxButton(this, wxID_ANY, "Reset");
  reset->SetToolTip("Show the whole energy range again");
  row2->Add(reset, 0, RIGHT, 12);
  for (wxSpinCtrlDouble *w : {p_from, p_to}) {
    w->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { applyWindow(true); });
    w->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { applyWindow(true); });
  }
  reset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { resetWindow(); });
  wxButton *csv = new wxButton(this, wxID_ANY, "Export CSV");
  wxButton *png = new wxButton(this, wxID_ANY, "Export PNG");
  row2->Add(csv, 0, RIGHT, 6);
  row2->Add(png, 0, RIGHT, 6);
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog d(this, "Export curves", "", "opdos.csv", "CSV (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (d.ShowModal() == wxID_OK) exportCsv(d.GetPath());
  });
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog d(this, "Export plot", "", "opdos.png", "PNG (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (d.ShowModal() == wxID_OK) savePng(d.GetPath(), 1200, 800);
  });

  top->Add(row1, 0, wxEXPAND | wxALL, 4);
  top->Add(row2, 0, wxEXPAND | wxALL, 4);
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
  recomputeOrbitals(&progress);
  Layout();
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
      p_plot->clearCurves();
      p_intPlot->clearCurves();
      p_plot->setMessage(U(err.c_str()));
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
    p_plot->clearCurves();
    p_intPlot->clearCurves();
    p_plot->setMessage(U(err.c_str()));
    setText(U(err.c_str()));
    return;
  }
  //  cTSc = 1 for every MO, as the MO diagram checks: a wrong AO order or
  //  normalisation for a code would make every number here wrong.
  if (p_all.normError > 1e-3) {
    char buf[300];
    snprintf(buf, sizeof buf, "The orbital coefficients do not match this basis (max |cTSc - 1| = %.3g), "
             "so no overlap populations are drawn.", p_all.normError);
    p_error = buf;
    p_plot->clearCurves();
    p_intPlot->clearCurves();
    p_plot->setMessage(U(buf));
    setText(U(buf));
    return;
  }
  p_error.clear();
  p_haveAll = true;
  setText(wxString());
  redraw();
}

void OpdosPanel::redraw()
{
  if (!p_ok || !p_haveAll) return;
  const bool occOnly = p_occ->GetValue();
  fragorb::DosOptions opt;
  opt.fwhmEv = p_width->GetValue();
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
  p_plot->setMessage(wxString());
  p_plot->clearCurves();
  p_intPlot->clearCurves();
  p_plot->clearVLines();
  p_intPlot->clearVLines();
  const double homo = p_unres ? std::max(p_part[0].homoEv, p_part[1].homoEv) : p_part[0].homoEv;
  const double e0 = p_part[0].energy.front(), e1 = p_part[0].energy.back();
  //  Occupied region shaded, HOMO marked.
  if (!p_winOn) {
    p_from->SetValue(std::floor(e0 * 10) / 10);
    p_to->SetValue(std::ceil(e1 * 10) / 10);
  }
  p_plot->setShadeX(e0, homo);
  p_intPlot->setShadeX(e0, homo);
  p_plot->addVLine(homo, "HOMO");
  p_intPlot->addVLine(homo, "HOMO");
  p_plot->setXAxisLabel("Energy (eV)");
  p_intPlot->setXAxisLabel("Energy (eV)");
  p_plot->setYAxisLabel(p_unres ? U("states / eV (alpha up, beta down)") : U("states / eV"));
  p_intPlot->setYAxisLabel("Integrated OPDOS (electrons)");
  p_intPlot->setTitle(wxString());
  p_plot->setTitle("Density of states, Mulliken");
  p_intPlot->SetMinSize(wxSize(240, p_int->GetValue() ? 150 : 1));
  p_intPlot->Show(p_int->GetValue());
  p_plot->setReadout("Energy", "E", "eV");

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
      const int c = p_plot->addCurve(r.energy, y, nm, k + (s ? 0 : 0), XYPlotCanvas::LINE);
      if (s) p_plot->setCurveStyle(c, true, 1);
      if (k == 0) p_plot->setCurveColour(c, wxColour(0, 0, 0));
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
      const int c = p_plot->addCurve(sx, sy, wxString(), 3, XYPlotCanvas::LINE);
      p_plot->setCurveThin(c, true);
      if (s) p_plot->setCurveStyle(c, false, 1);
    }
    if (p_int->GetValue()) {
      std::vector<double> yi(n);
      for (size_t g = 0; g < n; g++) yi[g] = sign * r.intOpdos[g * np];
      const int c = p_intPlot->addCurve(r.energy, yi, wxString(p_unres ? (s ? "beta" : "alpha") : "integrated OPDOS(A,B)"), 3, XYPlotCanvas::LINE);
      if (s) p_intPlot->setCurveStyle(c, true, 1);
    }
  }
  p_plot->setShowKey(true);
  applyWindow(false);
  Layout();

  double occOp = 0;
  for (int s = 0; s < nch; s++) occOp += p_part[s].occupiedOverlap[0];
  showExplanation(occOp);
}

void OpdosPanel::applyWindow(bool remember)
{
  if (!p_from || !p_plot) return;
  const double lo = p_from->GetValue(), hi = p_to->GetValue();
  if (remember) {
    if (!(hi > lo)) { setText("To must be above From."); return; }
    p_winOn = true;
  }
  if (p_winOn && hi > lo) {
    p_plot->setXWindow(lo, hi);
    p_intPlot->setXWindow(lo, hi);
  } else {
    p_plot->setXWindow(0, 0);
    p_intPlot->setXWindow(0, 0);
  }
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
    "g: Gaussian of the chosen width, area 1<br>"
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
  wxString t = wxString::Format("MOs within %.2f eV of %.3f eV:", w, ev);
  if (rows.empty()) t += " none";
  for (size_t k = 0; k < rows.size() && k < 12; k++) {
    const fragorb::DosResult& r = p_part[rows[k].ch];
    const int i = rows[k].idx;
    t += wxString::Format("\n  MO %d%s  E %.3f eV  occ %.1f  gross A %.3f  B %.3f  OP(A,B) %+.4f",
                          r.moIndex[i], p_unres ? (rows[k].ch ? " beta" : " alpha") : "", r.moEnergy[i],
                          r.moOccupation[i], r.moGross[i * 2], r.moGross[i * 2 + 1], r.moOverlap[i]);
  }
  p_clicked = t;
  setText(p_head);
}

bool OpdosPanel::clickEnergy(double ev)
{
  if (!p_ok || !p_haveAll) return false;
  listMos(ev);
  return true;
}

bool OpdosPanel::setAtoms(const std::string& a, const std::string& b)
{
  if (!p_ok || p_busy) return false;
  std::vector<int> va, vb;
  const int n = int(p_model->natoms());
  if (!fragorb::parseAtomList(a, n, va) || !fragorb::parseAtomList(b, n, vb)) {
    setText("Atom lists are numbers and ranges such as 1,2,5-8, each between 1 and the number of atoms.");
    return false;
  }
  for (size_t i = 0; i < va.size(); i++)
    if (std::binary_search(vb.begin(), vb.end(), va[i])) {
      setText(wxString::Format("Atom %d is in both A (%s) and B (%s); an atom belongs to one group.",
                               va[i] + 1, atomText(va), atomText(vb)));
      return false;
    }
  p_a = va;
  p_b = vb;
  p_atomsA->SetValue(atomText(p_a));
  p_atomsB->SetValue(atomText(p_b));
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
  return setAtoms(which ? atomText(p_a) : atomText(picked), which ? atomText(picked) : atomText(p_b));
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

bool OpdosPanel::savePng(const wxString& path, int width, int height)
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
    p_plot->paintOnto(dc, wxSize(width, h1));
    if (both) {
      wxBitmap lower(width, height - h1);
      {
        wxMemoryDC d2(lower);
        d2.SetBackground(*wxWHITE_BRUSH);
        d2.Clear();
        p_intPlot->paintOnto(d2, wxSize(width, height - h1));
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
      << "; width " << p_width->GetValue() << " eV (FWHM); states/eV\nE_eV";
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

std::string OpdosPanel::state() const
{
  if (!p_ok) return "ok 0\n";
  std::string s = "ok 1\n";
  char buf[512];
  snprintf(buf, sizeof buf, "ready %d\nerror %s\nnatoms %zu\nA %s\nB %s\nunrestricted %d\n",
           p_haveAll ? 1 : 0, p_error.empty() ? "-" : p_error.c_str(), p_model->natoms(),
           atomText(p_a).c_str(), atomText(p_b).c_str(), p_unres ? 1 : 0);
  s += buf;
  if (!p_haveAll) return s;
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
  snprintf(buf, sizeof buf, "width %.3f\nsticks %d\noccupied_only %d\nnorm_error %.3e\ncurves %d\n",
           p_width->GetValue(), p_sticks->GetValue() ? 1 : 0, p_occ->GetValue() ? 1 : 0, p_all.normError,
           p_plot->curveCount());
  s += buf;
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
