#include <algorithm>
#include <cstdio>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/ResourceDescriptor.H"
#include "tdat/GUIValues.H"
#include "tdat/PropString.H"
#include "tdat/PropValue.H"
#include "tdat/SpinMult.H"
#include "dsm/TGBSConfig.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"
#include "util/EcceURL.H"
#include "XYPlotCanvas.H"

#include "ReactionPanel.H"
#include "tdat/EnergyFormat.H"

namespace {
const wxWindowID ID_UNIT = wxNewId();
const wxWindowID ID_QUANTITY = wxNewId();
const wxWindowID ID_EXPORT = wxNewId();

bool isTsRuntype(IPropCalculation *calc)
{
  ICalculation *c = dynamic_cast<ICalculation*>(calc);
  if (!c) return false;
  const std::string rt = c->runtype().name();
  return rt.compare(0, 22, "Transition State (QST)") == 0 ||
         rt.compare(0, 24, "Transition State (NEB-TS") == 0;
}

bool hasValue(IPropCalculation *c, const char *key, double *v)
{
  PropValue *p = dynamic_cast<PropValue*>(c->getProperty(key));
  if (!p) return false;
  *v = p->value();
  return true;
}

// The energies and the settings they were computed with, or the reason
// there are none.  "Converged optimisation" is: the job completed and
// stored a geometry trace.
RxnState gather(IPropCalculation *calc, const std::string& name)
{
  RxnState s;
  s.name = name;
  ICalculation *c = dynamic_cast<ICalculation*>(calc);
  if (!calc || !c) {
    s.problem = "the calculation cannot be read.";
    return s;
  }
  const ResourceDescriptor::RUNSTATE st = c->getState();
  if (st != ResourceDescriptor::STATE_COMPLETED &&
      st != ResourceDescriptor::STATE_LOADED) {
    s.problem = "the calculation is not complete.";
    return s;
  }
  if (!calc->getProperty("GEOMTRACE")) {
    s.problem = "it has no optimised geometry (only an input structure), so "
                "there is no final energy to use.";
    return s;
  }
  if (!hasValue(calc, "TE", &s.energy)) {
    s.problem = "it has no total energy.";
    return s;
  }
  double h = 0, g = 0, z = 0, t = 0;
  if (hasValue(calc, "EZEROPT", &z) && hasValue(calc, "ENTHALPY", &h) &&
      hasValue(calc, "EGIBBS", &g)) {
    s.thermo = true;
    s.zpe = z;
    s.enthalpy = h;
    s.gibbs = g;
    if (hasValue(calc, "THERMOTEMP", &t)) s.temperature = t;
  }
  TTheory *th = c->theory();
  if (th) {
    s.method = th->toConciseString();
    delete th;
  }
  TGBSConfig *gbs = c->gbsConfig();
  if (gbs) {
    s.basis = gbs->name();
    delete gbs;
  }
  PropString *sm = dynamic_cast<PropString*>(calc->getProperty("SOLVMODEL"));
  if (sm) s.solvent = sm->value();
  s.multiplicity = SpinMult::toString(c->spinMultiplicity());
  Fragment *frag = c->fragment();
  if (frag) {
    s.charge = frag->charge();
    delete frag;
  }
  return s;
}

std::string pickOf(GUIValues *g, const char *a, const char *b)
{
  GUIValue *v = g ? g->get(a) : 0;
  if (!v) v = g ? g->get(b) : 0;
  return v ? v->getValueAsString() : "";
}

RxnState gatherPick(const std::string& url, const char *role)
{
  if (url.empty()) {
    RxnState s;
    s.name = "none picked";
    s.problem = std::string("no ") + role + " calculation is picked; pick it in "
                "the Calculation Editor.";
    return s;
  }
  PropertyTask *t = EDSIFactory::getPropertyTask(EcceURL(url.c_str()));
  if (!t) {
    RxnState s;
    s.name = url;
    s.problem = "the calculation no longer exists.";
    return s;
  }
  return gather(t, t->getName());
}
}

IMPLEMENT_DYNAMIC_CLASS(ReactionPanel, PropertyPanel)

ReactionPanel::ReactionPanel()
  : PropertyPanel(), p_unit(NULL), p_quantity(NULL), p_text(NULL),
    p_plot(NULL), p_grid(NULL)
{
}

ReactionPanel::ReactionPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_unit(NULL), p_quantity(NULL), p_text(NULL),
    p_plot(NULL), p_grid(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}

ReactionPanel::~ReactionPanel()
{
}

bool ReactionPanel::isRelevant(IPropCalculation *propCalc) const
{
  return isTsRuntype(propCalc);
}

bool ReactionPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ReactionPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));

  // A wrap sizer: in a narrow panel the controls go onto a second line.
  wxWrapSizer *bar = new wxWrapSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(this, wxID_ANY, "Energy unit:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_unit = new wxChoice(this, ID_UNIT);
  p_unit->Append("kJ/mol");
  p_unit->Append("kcal/mol");
  p_unit->SetSelection(0);
  bar->Add(p_unit, 0, wxRIGHT | wxBOTTOM, 12);
  bar->Add(new wxStaticText(this, wxID_ANY, "Diagram:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_quantity = new wxChoice(this, ID_QUANTITY);
  bar->Add(p_quantity, 0, wxRIGHT | wxBOTTOM, 12);
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0, wxBOTTOM, 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_text = new wxStaticText(this, wxID_ANY, wxEmptyString);
  GetSizer()->Add(p_text, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);

  p_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(1, 120));
  p_grid->CreateGrid(0, 5);
  p_grid->EnableEditing(false);
  p_grid->SetDefaultCellFont(p_grid->GetDefaultCellFont().Smaller());
  p_grid->SetLabelFont(p_grid->GetLabelFont().Smaller());
  p_grid->SetRowLabelSize(wxGRID_AUTOSIZE);
  GetSizer()->Add(p_grid, 0, wxEXPAND | wxALL, 3);

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(1, 180));
  p_plot->setShowXAxis(false);
  GetSizer()->Add(p_plot, 1, wxEXPAND);

  Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
    e.Skip();
    if (p_text && p_text->IsShown()) {
      p_text->SetLabel(p_message);
      p_text->Wrap(std::max(100, GetClientSize().x - 16));
    }
  });
  Bind(wxEVT_CHOICE, &ReactionPanel::OnChange, this, ID_UNIT);
  Bind(wxEVT_CHOICE, &ReactionPanel::OnChange, this, ID_QUANTITY);
  Bind(wxEVT_BUTTON, &ReactionPanel::OnExport, this, ID_EXPORT);

  // The picks are read once: they are fixed when the job runs.
  IPropCalculation *self = calculation;
  ICalculation *c = dynamic_cast<ICalculation*>(self);
  GUIValues *g = c ? c->guiparams() : 0;
  const std::string reactant = pickOf(g, "ES.Runtype.GeomOpt.QSTReactant",
                                      "ES.Runtype.NEB.Reactant");
  const std::string product = pickOf(g, "ES.Runtype.GeomOpt.QSTProduct",
                                     "ES.Runtype.NEB.Product");
  delete g;
  RxnState s[3];
  s[RXN_REACTANT] = gatherPick(reactant, "reactant");
  s[RXN_TS] = gather(self, self ? self->getURL().getFilePathTail() : "");
  s[RXN_PRODUCT] = gatherPick(product, "product");
  p_result = rxnAnalyse(s);

  fill();
  return true;
}

void ReactionPanel::fill()
{
  const bool kcal = p_unit->GetSelection() == 1;
  const wxString unit = kcal ? "kcal/mol" : "kJ/mol";

  int sel = p_quantity->GetSelection();
  if (p_quantity->GetCount() != p_result.levels.size()) {
    p_quantity->Clear();
    for (const RxnLevel& l : p_result.levels)
      p_quantity->Append(wxString::FromUTF8(l.label.c_str()));
    sel = p_result.levels.empty() ? -1 : (int)p_result.levels.size() - 1;
    p_quantity->SetSelection(sel);
  }

  wxString text;
  for (const std::string& s : p_result.problems)
    text += "No energies: " + wxString::FromUTF8(s.c_str()) + "\n";
  for (const std::string& s : p_result.warnings)
    text += "Warning: " + wxString::FromUTF8(s.c_str()) + "\n";
  for (const std::string& s : p_result.notes)
    text += wxString::FromUTF8(s.c_str()) + "\n";
  text.Trim();
  p_message = text;
  p_text->SetLabel(text);
  p_text->SetForegroundColour(p_result.problems.empty() && p_result.warnings.empty()
                              ? GetForegroundColour() : wxColour(160, 60, 0));
  p_text->Show(!text.empty());
  p_text->Wrap(std::max(100, GetClientSize().x - 16));
  const bool numbers = p_result.ok();
  p_quantity->Enable(numbers);
  p_grid->Show(numbers);
  p_plot->Show(numbers);

  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  p_grid->SetColLabelValue(0, "Reactant");
  p_grid->SetColLabelValue(1, "TS");
  p_grid->SetColLabelValue(2, "Product");
  p_grid->SetColLabelValue(3, wxString::FromUTF8("\xCE\x94\xE2\x80\xA1\n(") + unit + ")");
  p_grid->SetColLabelValue(4, wxString::FromUTF8("\xCE\x94rxn\n(") + unit + ")");
  p_grid->AppendRows((int)p_result.levels.size());
  for (size_t i = 0; i < p_result.levels.size(); i++) {
    const RxnLevel& l = p_result.levels[i];
    p_grid->SetRowLabelValue((int)i, wxString::FromUTF8(l.label.c_str()));
    for (int k = 0; k < 3; k++)
      p_grid->SetCellValue((int)i, k, wxString::FromUTF8(fmtAbsolute(l.value[k]).c_str()));
    p_grid->SetCellValue((int)i, 3, wxString::FromUTF8(fmtDifference(l.barrier(), kcal ? EU_KCAL : EU_KJ).c_str()));
    p_grid->SetCellValue((int)i, 4, wxString::FromUTF8(fmtDifference(l.reaction(), kcal ? EU_KCAL : EU_KJ).c_str()));
  }
  p_grid->SetColLabelSize(wxGRID_AUTOSIZE);
  p_grid->SetRowLabelSize(wxGRID_AUTOSIZE);
  p_grid->AutoSizeColumns();
  p_grid->SetMinSize(wxSize(1, p_grid->GetColLabelSize() +
      (int)p_result.levels.size() * p_grid->GetDefaultRowSize() + 24));

  p_plot->clearCurves();
  if (sel >= 0 && sel < (int)p_result.levels.size()) {
    const RxnLevel& l = p_result.levels[sel];
    const char *role[3] = {"Reactant", "Transition state", "Product"};
    double x[3], y[3];
    for (int k = 0; k < 3; k++) {
      x[k] = k;
      y[k] = rxnToUnit(l.value[k] - l.value[0], kcal);
    }
    p_plot->addCurve(x, y, 3, wxString(), 3, XYPlotCanvas::LINE);
    for (int k = 0; k < 3; k++) {
      const int c = p_plot->addCurve(x + k, y + k, 1, role[k], k, XYPlotCanvas::LEVELS);
      p_plot->setPointLabel(c, 0, wxString::Format("%s: %.1f %s", role[k], y[k], unit));
    }
    p_plot->setXPadding(0.6);
    p_plot->setTitle(wxString::FromUTF8((p_result.names[0] + "  >  " + p_result.names[1] +
                                         "  >  " + p_result.names[2]).c_str()));
    p_plot->setYAxisLabel("Relative " + wxString::FromUTF8(l.label.c_str()) +
                          " (" + unit + ")");
  }
  Layout();
}

void ReactionPanel::OnChange(wxCommandEvent&)
{
  fill();
}

bool ReactionPanel::exportCsv(const wxString& path, bool kcal)
{
  IPropCalculation *calc = getCalculation();
  const std::string text = rxnCsv(p_result, kcal,
      calc ? calc->getURL().getFilePathTail() : std::string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void ReactionPanel::OnExport(wxCommandEvent&)
{
  IPropCalculation *calc = getCalculation();
  wxFileDialog dlg(this, "Export reaction energies", wxEmptyString,
                   wxString::FromUTF8(calc ? calc->getURL().getFilePathTail().c_str() : "reaction") +
                   "-energies.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath(), p_unit->GetSelection() == 1))
    wxMessageBox("The file could not be written.", "Export reaction energies",
                 wxOK | wxICON_ERROR, this);
}
