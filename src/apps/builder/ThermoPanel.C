#include <cstdio>
#include <cstdlib>

#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/wrapsizer.h>

#include "dsm/PropertyTask.H"
#include "dsm/VDoc.H"
#include "tdat/PropValue.H"
#include "util/Ecce.H"

#include "ThermoPanel.H"

namespace {
const wxWindowID ID_TREATMENT = wxNewId();
const wxWindowID ID_STATE = wxNewId();
const wxWindowID ID_TEMP = wxNewId();
const wxWindowID ID_SIGMA = wxNewId();
const wxWindowID ID_ROTOR = wxNewId();

bool stored(IPropCalculation *c, const char *key, double *v)
{
  PropValue *p = dynamic_cast<PropValue*>(c->getProperty(key));
  if (!p) return false;
  *v = p->value();
  return true;
}
}

IMPLEMENT_DYNAMIC_CLASS(ThermoPanel, PropertyPanel)

ThermoPanel::ThermoPanel()
  : PropertyPanel(), p_treatment(NULL), p_state(NULL), p_rotor(NULL), p_temp(NULL), p_sigma(NULL),
    p_text(NULL), p_grid(NULL), p_have(false)
{
}

ThermoPanel::ThermoPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_treatment(NULL), p_state(NULL), p_rotor(NULL), p_temp(NULL), p_sigma(NULL),
    p_text(NULL), p_grid(NULL), p_have(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}

ThermoPanel::~ThermoPanel()
{
}

bool ThermoPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ThermoPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
  wxWrapSizer *bar = new wxWrapSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(this, wxID_ANY, "Treatment:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_treatment = new wxChoice(this, ID_TREATMENT);
  for (const ThermoTreatmentInfo& t : thermoTreatments())
    p_treatment->Append(wxString::FromUTF8(t.label));
  p_treatment->SetSelection(2);   // Grimme, 100 cm-1: what ORCA stores
  bar->Add(p_treatment, 0, wxRIGHT | wxBOTTOM, 12);
  bar->Add(new wxStaticText(this, wxID_ANY, "Temperature (K):"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  double t0 = 298.15;
  stored(calculation, "THERMOTEMP", &t0);
  p_temp = new wxTextCtrl(this, ID_TEMP, wxString::Format("%.2f", t0),
                          wxDefaultPosition, wxSize(80, -1),
                          wxTE_PROCESS_ENTER);
  bar->Add(p_temp, 0, wxRIGHT | wxBOTTOM, 12);
  bar->Add(new wxStaticText(this, wxID_ANY, "Standard state:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_state = new wxChoice(this, ID_STATE);
  p_state->Append("1 atm");
  p_state->Append("1 M");
  p_state->SetSelection(0);
  bar->Add(p_state, 0, wxRIGHT | wxBOTTOM, 12);
  bar->Add(new wxStaticText(this, wxID_ANY, "Symmetry number:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_sigma = new wxTextCtrl(this, ID_SIGMA, wxEmptyString, wxDefaultPosition,
                           wxSize(50, -1), wxTE_PROCESS_ENTER);
  p_sigma->SetHint("as used");
  bar->Add(p_sigma, 0, wxRIGHT | wxBOTTOM, 12);
  bar->Add(new wxStaticText(this, wxID_ANY, "Rotor:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_rotor = new wxChoice(this, ID_ROTOR);
  p_rotor->Append("As found");
  p_rotor->Append("Linear");
  p_rotor->Append("Non-linear");
  p_rotor->SetSelection(0);
  bar->Add(p_rotor, 0, wxBOTTOM, 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_text = new wxStaticText(this, wxID_ANY, wxEmptyString);
  // No width of its own: the note takes the panel's width and wraps to it.
  p_text->SetMinSize(wxSize(1, -1));
  GetSizer()->Add(p_text, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
  p_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(1, 110));
  p_grid->CreateGrid(3, 2);
  p_grid->EnableEditing(false);
  GetSizer()->Add(p_grid, 0, wxEXPAND | wxALL, 3);

  // The panel has no real width while it is built, so the note is wrapped
  // from its own text again whenever the panel is sized.
  Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
    e.Skip();
    if (e.GetSize().x != p_wrapWidth) CallAfter(&ThermoPanel::rewrap);
  });
  Bind(wxEVT_CHOICE, &ThermoPanel::OnChange, this, ID_TREATMENT);
  Bind(wxEVT_CHOICE, &ThermoPanel::OnChange, this, ID_STATE);
  Bind(wxEVT_TEXT_ENTER, &ThermoPanel::OnChange, this, ID_TEMP);
  Bind(wxEVT_CHOICE, &ThermoPanel::OnChange, this, ID_ROTOR);
  Bind(wxEVT_TEXT_ENTER, &ThermoPanel::OnChange, this, ID_SIGMA);
  p_sigma->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
    e.Skip();
    wxCommandEvent c;
    OnChange(c);
  });
  p_temp->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
    e.Skip();
    wxCommandEvent c;
    OnChange(c);
  });
  fill();
  return true;
}

void ThermoPanel::fill()
{
  IPropCalculation *calc = getCalculation();
  double kelvin = atof(p_temp->GetValue().utf8_str());
  if (kelvin <= 0) kelvin = 298.15;
  const int sel = p_treatment->GetSelection();
  const std::string cli = sel >= 0 ? thermoTreatments()[sel].cli : "rrho";
  const int sigma = atoi(p_sigma->GetValue().utf8_str());
  const ThermoRunResult r = calc ? runThermo(calc->getURL().toString(), kelvin,
                                             p_state->GetSelection() == 1,
                                             sigma > 0 ? sigma : 0,
                                             p_rotor->GetSelection())
                                 : ThermoRunResult();
  p_have = false;
  p_error.clear();
  wxString msg;
  const ThermoValue *v = r.ok ? r.find(cli) : 0;
  if (!v) {
    p_error = r.error.empty() ? "no values" : r.error;
    msg = "No recomputed values: " + wxString::FromUTF8(p_error.c_str());
  } else {
    p_have = true;
    p_value = *v;
    if (!r.symmetry.empty()) msg += wxString::FromUTF8(r.symmetry.c_str()) + "\n";
    for (const std::string& n : r.notes) msg += wxString::FromUTF8(n.c_str()) + "\n";
  }
  msg.Trim();
  p_note = msg;
  p_text->Show(!msg.empty());
  rewrap();

  PropertyTask *task = dynamic_cast<PropertyTask*>(calc);
  const std::string code = task ?
      task->getProp(VDoc::getEcceNamespace() + ":application") : std::string();
  p_grid->SetColLabelValue(0, "Recomputed (ecce-thermo)");
  p_grid->SetColLabelValue(1, wxString::FromUTF8(
      ((code.empty() ? std::string("The code") : code) + "'s own").c_str()));
  p_grid->SetRowLabelValue(0, "H (Hartree)");
  p_grid->SetRowLabelValue(1, "S (J/mol/K)");
  p_grid->SetRowLabelValue(2, "G (Hartree)");
  double h = 0, ts = 0, g = 0, tt = 0;
  const bool haveStored = calc && stored(calc, "ENTHALPY", &h) &&
                          stored(calc, "ENTROPY", &ts) && stored(calc, "EGIBBS", &g);
  stored(calc, "THERMOTEMP", &tt);
  for (int row = 0; row < 3; row++) {
    const double mine[3] = {p_value.H, p_value.S, p_value.G};
    // ENTROPY is stored as T*S in Hartree.
    const double theirs[3] = {h, tt > 0 ? ts / tt * 4.3597447222071e-18 * 6.02214076e23 : 0, g};
    p_grid->SetCellValue(row, 0, p_have ?
        wxString::Format(row == 1 ? "%.3f" : "%.6f", mine[row]) : wxString("-"));
    p_grid->SetCellValue(row, 1, haveStored && (row != 1 || tt > 0) ?
        wxString::Format(row == 1 ? "%.3f" : "%.6f", theirs[row]) : wxString("-"));
  }
  // Measure the descriptive labels, not CreateGrid's initial row numbers.
  // Keep these widths when the viewport narrows; wxGrid scrolls the cells.
  p_grid->SetRowLabelSize(wxGRID_AUTOSIZE);
  p_grid->AutoSizeColumns();
  // Reserve the horizontal scrollbar's height so all three rows stay visible.
  int height = p_grid->GetColLabelSize() +
      wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y, p_grid) +
      p_grid->GetSize().y - p_grid->GetClientSize().y;
  for (int row = 0; row < p_grid->GetNumberRows(); ++row)
    height += p_grid->GetRowSize(row);
  p_grid->SetMinSize(wxSize(1, height));
  Layout();
}

void ThermoPanel::rewrap()
{
  const int width = GetClientSize().x;
  if (width < 40) return;   // not laid out yet
  p_wrapWidth = GetSize().x;
  p_text->SetLabel(p_note);   // drops the line breaks of the previous wrap
  p_text->Wrap(width - 12);
  p_text->InvalidateBestSize();
  Layout();
}

void ThermoPanel::OnChange(wxCommandEvent&)
{
  fill();
}

bool ThermoPanel::setChoice(const std::string& treatment, double kelvin, bool oneMolar)
{
  const std::vector<ThermoTreatmentInfo>& t = thermoTreatments();
  for (size_t i = 0; i < t.size(); i++)
    if (treatment == t[i].cli) {
      p_treatment->SetSelection((int)i);
      p_temp->SetValue(wxString::Format("%.2f", kelvin));
      p_state->SetSelection(oneMolar ? 1 : 0);
      fill();
      return true;
    }
  return false;
}

void ThermoPanel::setSymmetry(int sigma, int rotor)
{
  p_sigma->SetValue(sigma > 0 ? wxString::Format("%d", sigma) : wxString());
  p_rotor->SetSelection(rotor);
  fill();
}

bool ThermoPanel::shown(ThermoValue *v) const
{
  if (p_have && v) *v = p_value;
  return p_have;
}
