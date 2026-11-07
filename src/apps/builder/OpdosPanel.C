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
#include <wx/image.h>
#include <wx/link.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/wrapsizer.h>

#include "dsm/IPropCalculation.H"
#include "tdat/Fragment.H"
#include "tdat/ResultsFromCalc.H"
#include "tdat/ResultsH5.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/SGViewer.H"
#include "OpdosPanel.H"
#include "XYPlotCanvas.H"

namespace {

wxString U(const char *utf8) { return wxString::FromUTF8(utf8); }

const char *SCHEME =
  "Population scheme: Mulliken. OPDOS(A,B) = sum_i 2 sum(mu in A, nu in B) c_mu,i c_nu,i S_mu,nu g(E - e_i): "
  "positive is bonding between A and B, negative antibonding. PDOS(X) = Mulliken gross population of X in each MO. "
  "States per eV; each closed-shell MO counts twice.";
const char *DIFFUSE_NOTE =
  "Virtual MOs in a diffuse basis have large Mulliken overlap populations of both signs that cancel; "
  "read the empty region with care or tick Occupied only.";
const char *SEMIEMPIRICAL_NOTE =
  "This calculation is semiempirical: its basis is treated as orthonormal (S = I), so the Mulliken overlap "
  "population between atoms is zero for every orbital and an OPDOS would be empty. A bond-order DOS is not "
  "available yet.";

bool parseAtoms(const std::string& s, int natoms, std::vector<int>& out)
{
  out.clear();
  size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && (s[i] == ' ' || s[i] == ',')) i++;
    if (i >= s.size()) break;
    char *end = 0;
    long a = std::strtol(s.c_str() + i, &end, 10), b = a;
    if (end == s.c_str() + i) return false;
    i = end - s.c_str();
    if (i < s.size() && s[i] == '-') {
      const char *p = s.c_str() + i + 1;
      b = std::strtol(p, &end, 10);
      if (end == p) return false;
      i = end - s.c_str();
    }
    if (a < 1 || b < a || b > natoms) return false;
    for (long x = a; x <= b; x++) out.push_back(int(x) - 1);
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return !out.empty();
}

}  // namespace

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
  auto atomsBox = [&](wxTextCtrl *&box, const char *label, int which) {
    row1->Add(new wxStaticText(this, wxID_ANY, label), 0, RIGHT, 4);
    box = new wxTextCtrl(this, wxID_ANY, atomText(which ? p_b : p_a), wxDefaultPosition,
                         wxSize(110, -1), wxTE_PROCESS_ENTER);
    box->SetToolTip("Atom numbers of this fragment as in the structure, e.g. 1,2,5-8; Enter applies");
    row1->Add(box, 0, RIGHT, 4);
    wxButton *use = new wxButton(this, wxID_ANY, "Use selection");
    use->SetToolTip("Take the atoms selected in the viewer");
    row1->Add(use, 0, RIGHT, 12);
    use->Bind(wxEVT_BUTTON, [this, which](wxCommandEvent&) { useSelection(which); });
    box->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
      setAtoms(p_atomsA->GetValue().ToStdString(), p_atomsB->GetValue().ToStdString());
    });
  };
  atomsBox(p_atomsA, "A", 0);
  atomsBox(p_atomsB, "B", 1);

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
  p_plot->SetMinSize(wxSize(240, 200));
  p_intPlot = new XYPlotCanvas(this);
  p_intPlot->SetMinSize(wxSize(240, 110));
  p_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 110),
                          wxTE_MULTILINE | wxTE_READONLY);
  top->Add(p_plot, 3, wxEXPAND | wxALL, 4);
  top->Add(p_intPlot, 1, wxEXPAND | wxALL, 4);
  top->Add(p_text, 1, wxEXPAND | wxALL, 4);
  auto handler = [this](int c, int i) {
    XYPlotCanvas *src = p_plot->curveCount() > c && p_plot->pointCount(c) > i ? p_plot : p_intPlot;
    listMos(src->xAt(c, i));
  };
  p_plot->setPointHandler(handler);
  p_intPlot->setPointHandler([this](int c, int i) { listMos(p_intPlot->xAt(c, i)); });
  p_plot->setZeroLine(true);
  p_intPlot->setZeroLine(true);
  recomputeOrbitals();
  Layout();
}

void OpdosPanel::setText(const wxString& head)
{
  p_head = head;
  if (!p_text) return;
  p_text->SetValue(head + (p_clicked.empty() ? wxString() : "\n" + p_clicked));
  p_text->ShowPosition(0);
}

void OpdosPanel::recomputeOrbitals()
{
  p_haveAll = false;
  p_clicked.clear();
  if (!p_ok) return;
  wxBusyCursor busy;
  fragorb::DosOptions opt;
  fragorb::AtomSet a, b;
  a.name = "A"; a.atoms = p_a;
  b.name = "B"; b.atoms = p_b;
  opt.sets.push_back(a);
  opt.sets.push_back(b);
  opt.pairs.push_back(std::make_pair(0, 1));
  std::string err;
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
  p_plot->setShadeX(e0, homo);
  p_intPlot->setShadeX(e0, homo);
  p_plot->addVLine(homo, "HOMO");
  p_intPlot->addVLine(homo, "HOMO");
  p_plot->setXAxisLabel("Energy (eV)");
  p_intPlot->setXAxisLabel("Energy (eV)");
  p_plot->setYAxisLabel(p_unres ? U("states / eV (alpha up, beta down)") : U("states / eV"));
  p_intPlot->setYAxisLabel("integrated OPDOS");
  p_plot->setTitle("Density of states, Mulliken");
  p_intPlot->SetMinSize(wxSize(240, p_int->GetValue() ? 110 : 1));
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
  (void)e1;
  Layout();

  //  Header: scheme, the totals and the caveats.
  double occOp = 0;
  for (int s = 0; s < nch; s++) occOp += p_part[s].occupiedOverlap[0];
  wxString head = U(SCHEME);
  head += wxString::Format("\nA = atoms %s, B = atoms %s.  Occupied A-B overlap population (sum n_i OP_i) %.4f",
                           atomText(p_a), atomText(p_b), occOp);
  head += wxString::Format("; max |cTSc - 1| %.1e.", p_all.normError);
  head += "\n" + U(DIFFUSE_NOTE);
  if (p_unres) head += U("\nOpen shell: alpha channel up, beta channel down.");
  setText(head);
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
  if (!p_ok) return false;
  std::vector<int> va, vb;
  const int n = int(p_model->natoms());
  if (!parseAtoms(a, n, va) || !parseAtoms(b, n, vb)) {
    setText("Atom lists are numbers and ranges such as 1,2,5-8, each between 1 and the number of atoms.");
    return false;
  }
  for (size_t i = 0; i < va.size(); i++)
    if (std::binary_search(vb.begin(), vb.end(), va[i])) {
      setText(wxString::Format("Atom %d is in both A and B.", va[i] + 1));
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
  if (!p_ok || !p_vizfw) return false;
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
  char buf[64];
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
  s += "text " + std::string(p_text ? p_text->GetValue().ToUTF8().data() : "") + "\n";
  return s;
}
