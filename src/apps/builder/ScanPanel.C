#include <algorithm>
#include <cstdio>
#include <locale>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/radiobox.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/wrapsizer.h>
#include <wx/stattext.h>
#include <wx/event.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/utils.h>

#include "tdat/ResultsFromCalc.H"
#include "tdat/PropVector.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TProperty.H"
#include "dsm/IPropCalculation.H"
#include "viz/GTStepCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "XYPlotCanvas.H"
#include "PointCalc.H"
#include "wxgui/PlaybackControl.H"
#include "util/EcceURL.H"
#include "util/RateRun.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "PlaybackExtras.H"
#include "wxgui/ewxConfig.H"
#include "PlaybackLoop.H"
#include "PointNote.H"
#include "ScanPanel.H"
#include "wxgui/ewxBitmap.H"

namespace {
const double HARTREE_KJ = 2625.4996;
const double HARTREE_KCAL = 627.5095;
const wxWindowID ID_UNIT = wxNewId();
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_NEWCALC = wxNewId();
const wxWindowID ID_VIEW = wxNewId();
const wxWindowID ID_PLAYBACK = wxNewId();
const wxWindowID ID_MORSE = wxNewId();
const wxWindowID ID_WINDOW = wxNewId();
const wxWindowID ID_COMPARE = wxNewId();
const double KCAL_KJ = 4.184;
static const char *INIFILE = "wxbuilder.ini";

double factor(bool kcal) { return kcal ? HARTREE_KCAL : HARTREE_KJ; }
}

std::string scanCsv(const std::vector<double>& coord,
                    const std::vector<double>& hartree,
                    const std::string& coordUnit, bool kcal,
                    const std::string& calculation)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(10);
  out << "# ECCE relaxed scan\n";
  out << "# calculation: " << calculation << "\n";
  out << "# coordinate unit: " << coordUnit << "\n";
  out << "# relative energy is to the lowest scan point\n";
  out << "point,coordinate_" << coordUnit << ",energy_hartree,relative_energy_"
      << (kcal ? "kcal/mol" : "kJ/mol") << "\n";
  const size_t n = std::min(coord.size(), hartree.size());
  const double lo = n ? *std::min_element(hartree.begin(), hartree.begin() + n) : 0;
  for (size_t i = 0; i < n; i++)
    out << i + 1 << "," << coord[i] << "," << hartree[i] << ","
        << (hartree[i] - lo) * factor(kcal) << "\n";
  return out.str();
}

IMPLEMENT_DYNAMIC_CLASS(ScanPanel, VizPropertyPanel)

BEGIN_EVENT_TABLE(ScanPanel, VizPropertyPanel)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_TICK_EVENT, ScanPanel::OnPlayback)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_LOOP_EVENT, ScanPanel::OnPlayback)
END_EVENT_TABLE()

ScanPanel::ScanPanel()
  : VizPropertyPanel(), p_selected(-1), p_unit(NULL), p_plot(NULL), p_grid(NULL), p_view(NULL),
    p_status(NULL), p_playback(NULL), p_extras(NULL), p_delay(500), p_syncing(false),
    p_morse(NULL), p_window(NULL), p_windowUnit(NULL), p_morseText(NULL)
{
}

ScanPanel::ScanPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(), p_selected(-1), p_unit(NULL), p_plot(NULL), p_grid(NULL), p_view(NULL),
    p_status(NULL), p_playback(NULL), p_extras(NULL), p_delay(500), p_syncing(false),
    p_morse(NULL), p_window(NULL), p_windowUnit(NULL), p_morseText(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}

ScanPanel::~ScanPanel()
{
  delete p_extras;
}

bool ScanPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ScanPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));

  wxWrapSizer *bar = new wxWrapSizer(wxHORIZONTAL);   // wraps in a narrow dock
  bar->Add(new wxStaticText(this, wxID_ANY, "Energy unit:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_unit = new wxChoice(this, ID_UNIT);
  p_unit->Append("kJ/mol");
  p_unit->Append("kcal/mol");
  p_unit->SetSelection(0);
  bar->Add(p_unit, 0, wxRIGHT, 12);
  wxString views[] = {"Graph", "Table"};
  p_view = new wxRadioBox(this, ID_VIEW, "Show", wxDefaultPosition,
                          wxDefaultSize, 2, views, 1, wxRA_SPECIFY_ROWS);
  bar->Add(p_view, 0, wxRIGHT, 12);
  bar->Add(new wxButton(this, ID_NEWCALC, "New calculation from this point"), 0,
           wxRIGHT, 6);
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0, wxRIGHT, 12);
  bar->Add(new wxButton(this, ID_COMPARE, "Compare..."), 0, wxRIGHT, 12);
  p_morse = new wxCheckBox(this, ID_MORSE, "Fit Morse");
  p_morse->SetToolTip(wxString::FromUTF8(
      "Fit V(r) = De(1 - exp(-a(r - re)))\u00b2 to the points of a bond scan "
      "within the window above the lowest point"));
  bar->Add(p_morse, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  bar->Add(new wxStaticText(this, wxID_ANY, "Window:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  p_window = new wxTextCtrl(this, ID_WINDOW, "", wxDefaultPosition,
                            wxSize(ewxBitmap::scaled(70), -1), wxTE_PROCESS_ENTER);
  p_window->SetHint("all bound");
  p_window->SetToolTip("Energy above the lowest point; empty: every point "
                       "below the energy at the longest distance");
  bar->Add(p_window, 0, wxRIGHT, 4);
  p_windowUnit = new wxStaticText(this, wxID_ANY, "kJ/mol");
  bar->Add(p_windowUnit, 0, wxALIGN_CENTER_VERTICAL);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(300, 180));
  p_plot->setPointHandler([this](int curve, int index) {
    if (curve == 0) selectPoint(index);   // compared scans have no structures here
  });
  GetSizer()->Add(p_plot, 3, wxEXPAND);

  wxBoxSizer *play = new wxBoxSizer(wxHORIZONTAL);
  p_playback = new PlaybackControl(this, ID_PLAYBACK);
  p_playback->SetDelay(&p_delay);
  p_playback->SetStopIndex(0L);
  if (wxWindow *rec = FindWindow(PlaybackControl::ID_RECORD)) rec->Hide();
  play->Add(p_playback, 0, wxALIGN_CENTER_VERTICAL);
  p_status = new wxStaticText(this, wxID_ANY, "");
  GetSizer()->Add(play, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  p_extras = new PlaybackExtras(this, GetSizer(), &p_delay);
  GetSizer()->Add(p_status, 0, wxEXPAND | wxALL, 4);
  p_morseText = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                               wxSize(300, ewxBitmap::scaled(130)),
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_morseText->Hide();
  GetSizer()->Add(p_morseText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

  p_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 110));
  p_grid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
  p_grid->EnableEditing(false);
  p_grid->SetRowLabelSize(ewxBitmap::scaled(40));
  p_grid->SetColLabelValue(0, "Coordinate");
  p_grid->SetColLabelValue(1, "Energy (Eh)");
  p_grid->SetColLabelValue(2, "Relative");
  GetSizer()->Add(p_grid, 2, wxEXPAND | wxALL, 3);

  p_grid->Bind(wxEVT_GRID_SELECT_CELL, &ScanPanel::OnRow, this);
  Bind(wxEVT_CHOICE, &ScanPanel::OnUnit, this, ID_UNIT);
  Bind(wxEVT_BUTTON, &ScanPanel::OnExport, this, ID_EXPORT);
  Bind(wxEVT_BUTTON, &ScanPanel::OnNewCalc, this, ID_NEWCALC);
  Bind(wxEVT_RADIOBOX, &ScanPanel::OnView, this, ID_VIEW);
  Bind(wxEVT_CHECKBOX, &ScanPanel::OnMorse, this, ID_MORSE);
  Bind(wxEVT_TEXT_ENTER, &ScanPanel::OnMorse, this, ID_WINDOW);
  Bind(wxEVT_BUTTON, &ScanPanel::OnCompare, this, ID_COMPARE);

  bool table = false;
  ewxConfig::getConfig(INIFILE)->Read("ScanPanel/Table", &table, false);
  showView(table);

  fill();
  return true;
}

void ScanPanel::fill()
{
  p_coord.clear();
  p_hartree.clear();
  p_frame.clear();
  IPropCalculation *calc = getCalculation();
  PropVector *c = calc ? dynamic_cast<PropVector*>(ResultsFromCalc::property(calc, "SCANCOORD")) : 0;
  PropVector *e = calc ? dynamic_cast<PropVector*>(ResultsFromCalc::property(calc, "SCANENERGY")) : 0;
  if (!c || !e) return;
  const size_t n = std::min(c->values().size(), e->values().size());
  p_coord.assign(c->values().begin(), c->values().begin() + n);
  p_hartree.assign(e->values().begin(), e->values().begin() + n);
  p_coordUnit = c->units() == "Degree" ? "degree" : "angstrom";

  // One structure per point, or none: a partial set cannot be matched up.
  PropTSVecTable *geom =
      calc ? dynamic_cast<PropTSVecTable*>(ResultsFromCalc::property(calc, "SCANGEOM")) : 0;
  p_frame.assign(n, -1);
  if (geom && geom->tables() == n)
    for (size_t i = 0; i < n; i++) p_frame[i] = (int)i;

  if (p_playback) p_playback->SetStopIndex(n ? (long)n - 1 : 0);
  const bool kcal = p_unit->GetSelection() == 1;
  const wxString unit = kcal ? "kcal/mol" : "kJ/mol";
  const double lo = n ? *std::min_element(p_hartree.begin(), p_hartree.end()) : 0;

  const bool bond = p_coordUnit != "degree";
  p_morse->Show(bond);
  p_window->Show(bond);
  p_windowUnit->SetLabel(unit);
  p_windowUnit->Show(bond);
  if (!bond) p_morse->SetValue(false);

  p_plot->clearCurves();
  if (n) {
    std::vector<double> y(n);
    for (size_t i = 0; i < n; i++) y[i] = (p_hartree[i] - lo) * factor(kcal);
    p_plot->addCurve(p_coord, y, wxString::FromUTF8(
        calc->getURL().getFilePathTail().c_str()), 0);
  }
  // Each compared scan relative to its own lowest point: energies of
  // different methods are never put on one scale.
  for (size_t c = 0; c < p_compared.size(); c++) {
    const Compared& o = p_compared[c];
    if (o.hartree.empty()) continue;
    const double olo = *std::min_element(o.hartree.begin(), o.hartree.end());
    std::vector<double> y(o.hartree.size());
    for (size_t i = 0; i < y.size(); i++) y[i] = (o.hartree[i] - olo) * factor(kcal);
    p_plot->addCurve(o.coord, y, wxString::FromUTF8(o.name.c_str()), (int)c + 1);
  }
  if (p_morse->GetValue() && !p_fitR.empty()) {
    std::vector<double> y(p_fitE.size());
    for (size_t i = 0; i < y.size(); i++) y[i] = (p_fitE[i] - lo) * factor(kcal);
    const int f = p_plot->addCurve(p_fitR, y, "Morse fit", 0, XYPlotCanvas::LINE);
    p_plot->setCurveStyle(f, true, 2);
  }
  p_plot->setShowKey(!p_compared.empty() || (p_morse->GetValue() && !p_fitR.empty()));
  p_plot->setXAxisLabel(p_coordUnit == "degree" ? "Scanned coordinate (degrees)"
                                                 : wxString::FromUTF8("Scanned coordinate (Å)"));
  p_plot->setYAxisLabel(p_compared.empty()
      ? "Relative energy (" + unit + ")"
      : "Energy above each curve's lowest point (" + unit + ")");
  p_plot->setReadout("Point", "Coordinate",
                     p_coordUnit == "degree" ? wxString::FromUTF8("°")
                                             : wxString::FromUTF8("Å"),
                     "Relative energy", unit);

  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  p_grid->AppendRows((int)n);
  p_grid->SetColLabelValue(2, unit);
  for (size_t i = 0; i < n; i++) {
    p_grid->SetRowLabelValue((int)i, wxString::Format("%d", (int)i + 1));
    p_grid->SetCellValue((int)i, 0, wxString::Format("%.4f", p_coord[i]));
    p_grid->SetCellValue((int)i, 1, wxString::Format("%.6f", p_hartree[i]));
    p_grid->SetCellValue((int)i, 2,
        wxString::Format("%.3f", (p_hartree[i] - lo) * factor(kcal)));
  }
  p_grid->AutoSizeColumns();
}

// A running job delivers the energies before the structures (those are
// parsed from a separate file at the end), so the panel must refill when
// they arrive, or its points stay without a structure to show.
void ScanPanel::refresh()
{
  if (p_morse && p_morse->GetValue()) runFit();
  fill();
  if (p_selected >= (int)p_coord.size()) p_selected = -1;
  if (p_selected < 0) return;
  p_syncing = true;
  p_plot->setSelectedPoint(0, p_selected);
  p_grid->SelectRow(p_selected);
  p_syncing = false;
  if (p_playback) p_playback->SetCurrentStep((long)p_selected);
  if (hasFocus()) showPoint(p_selected);
}

// One of plot and table at a time, as in the IRC Path panel, so the shown one
// gets the full height; the choice is remembered for the next panel.
void ScanPanel::showView(bool table)
{
  p_view->SetSelection(table ? 1 : 0);
  p_plot->Show(!table);
  p_grid->Show(table);
  Layout();
}

std::string ScanPanel::viewState() const
{
  char buf[160];
  snprintf(buf, sizeof buf, "table=%d plotShown=%d gridShown=%d plotH=%d gridH=%d",
           p_view->GetSelection(), p_plot->IsShown(), p_grid->IsShown(),
           p_plot->GetSize().y, p_grid->GetSize().y);
  return buf;
}

void ScanPanel::OnView(wxCommandEvent&)
{
  setTableView(p_view->GetSelection() == 1);
}

void ScanPanel::setTableView(bool table)
{
  showView(table);
  ewxConfig *cfg = ewxConfig::getConfig(INIFILE);
  cfg->Write("ScanPanel/Table", table);
  cfg->Flush();   // the Builder may be closed without writing the file
}

int ScanPanel::showPoint(int index)
{
  if (index < 0 || index >= (int)p_frame.size() || p_frame[index] < 0) return -1;
  IPropCalculation *calc = getCalculation();
  if (!calc) return -1;
  p_selected = index;
  SGContainer& sg = getFW().getSceneGraph();
  // A view that already cuts atoms off was zoomed in by the user: keep it.
  const bool wholeInView = !getFW().atomsOutsideView();
  Command *cmd = new GTStepCmd("Scan Step", &sg, calc);
  cmd->getParameter("Index")->setInteger(p_frame[index]);
  cmd->getParameter("PropKey")->setString("SCANGEOM");
  cmd->execute();
  delete cmd;
  // Bonds differ between points, so redo them from the new geometry.
  SGFragment *frag = sg.getFragment();
  frag->deleteAllBonds();
  frag->addCovalentBonds();
  frag->touchNumbers();
  sg.touchChemDisplay();
  // The camera was fitted to the stored (last) point; the first point of a
  // bond scan can be far wider (#832).  Re-fit only when atoms leave a view
  // that showed them all.
  if (wholeInView && getFW().atomsOutsideView()) getFW().center();
  ecce::EventDispatcher::getDispatcher().publish(ecce::Event("StepChange"));
  selectOrbitalPoint(calc, p_frame[index], index);
  return p_frame[index];
}

int ScanPanel::selectPoint(int index)
{
  if (index < 0 || index >= (int)p_coord.size()) return -1;
  p_selected = index;
  p_syncing = true;
  p_plot->setSelectedPoint(0, index);
  p_grid->SelectRow(index);
  p_grid->GoToCell(index, 0);
  p_syncing = false;
  if (p_playback) p_playback->SetCurrentStep((long)index);
  return showPoint(index);
}

void ScanPanel::OnRow(wxGridEvent& event)
{
  event.Skip();
  if (!p_syncing) selectPoint(event.GetRow());
}

// With Loop on, the control's wrap from one end to the other is turned into
// a reversal, so play runs forward and back until stopped.
void ScanPanel::OnPlayback(wxCommandEvent& event)
{
  long at = event.GetExtraLong();
  if (event.GetEventType() == wxEVT_PLAYBACK_LOOP_EVENT && p_extras &&
      p_extras->loop() && points() > 0) {
    const bool wasForward = event.GetInt() != 0;
    const long n = points();
    const playbackloop::Pos p = playbackloop::next(
        wasForward ? playbackloop::Pos{n - 1, 1} : playbackloop::Pos{0, -1}, n);
    p_playback->SetStepSize(p.dir);
    p_playback->SetCurrentStep(p.cur);
    at = p.cur;
  }
  selectPoint((int)at);
}

void ScanPanel::setPlaybackSpeed(const std::string& name)
{
  if (p_extras) p_extras->setSpeed(name);
}

std::string ScanPanel::playbackSpeed() const
{
  return p_extras ? p_extras->speed() : std::string();
}

void ScanPanel::setPlaybackLoop(bool on)
{
  if (p_extras) p_extras->setLoop(on);
}

bool ScanPanel::playbackLoop() const
{
  return p_extras && p_extras->loop();
}

std::vector<int> ScanPanel::playOrder(int ticks)
{
  std::vector<int> out;
  const int n = points();
  playbackloop::Pos p{0, 1};
  if (!n) return out;
  const bool loop = playbackLoop();
  selectPoint(0);
  out.push_back(0);
  for (int t = 0; t < ticks; t++) {
    if (loop) p = playbackloop::next(p, n);
    else if (p.cur < n - 1) p.cur++;
    else break;
    selectPoint((int)p.cur);
    out.push_back((int)p.cur);
  }
  return out;
}

int ScanPanel::step(const std::string& which)
{
  const int n = points();
  if (!n) return -1;
  int i = p_selected < 0 ? 0 : p_selected;
  if (which == "first") i = 0;
  else if (which == "last") i = n - 1;
  else if (which == "prev") i = std::max(0, i - 1);
  else if (which == "next") i = std::min(n - 1, i + 1);
  selectPoint(i);
  return p_selected;
}

void ScanPanel::selectRow(int index)
{
  if (index >= 0 && index < points()) p_grid->GoToCell(index, 0);
}

int ScanPanel::markerPoint() const
{
  return p_plot->selectedIndex();
}

int ScanPanel::selectedRow() const
{
  wxArrayInt rows = p_grid->GetSelectedRows();
  return rows.GetCount() ? rows[0] : -1;
}

wxString ScanPanel::status() const
{
  return p_status ? p_status->GetLabel() : wxString();
}

void ScanPanel::setStatus(const wxString& text)
{
  if (p_status) p_status->SetLabel(text);
}

// The box shows the selected scan point; unticked, the calculation's own
// last structure comes back.
void ScanPanel::receiveFocus()
{
  if (p_selected >= 0) showPoint(p_selected);
}

void ScanPanel::loseFocus()
{
  IPropCalculation *calc = getCalculation();
  // With per-point orbitals the selected point's structure stays with them.
  if (calc && calc->hasPointOrbitals()) return;
  PropTSVecTable *trace =
      calc ? dynamic_cast<PropTSVecTable*>(ResultsFromCalc::property(calc, "GEOMTRACE")) : 0;
  if (!trace || trace->tables() == 0) return;
  SGContainer& sg = getFW().getSceneGraph();
  Command *cmd = new GTStepCmd("Trace Step", &sg, calc);
  cmd->getParameter("Index")->setInteger((int)trace->tables() - 1);
  cmd->execute();
  delete cmd;
  SGFragment *frag = sg.getFragment();
  frag->deleteAllBonds();
  frag->addCovalentBonds();
  frag->touchNumbers();
  sg.touchChemDisplay();
  ecce::EventDispatcher::getDispatcher().publish(ecce::Event("StepChange"));
}

std::string ScanPanel::newCalcFromPoint(int index)
{
  if (index < 0) index = p_selected;
  std::string why;
  std::string url;
  if (index < 0 || index >= (int)p_frame.size() || p_frame[index] < 0)
    why = "Click a point in the plot first; this scan has no structure for it.";
  else
    url = ::newCalcFromPoint(getCalculation(), "SCANGEOM", p_frame[index],
                             index + 1, why);
  if (url.empty() && !why.empty()) {
    setStatus(wxString::FromUTF8(why.c_str()));
    wxMessageBox(wxString::FromUTF8(why.c_str()), "New calculation",
                 wxOK | wxICON_WARNING, this);
  } else if (!url.empty()) {
    setStatus("Created " + wxString::FromUTF8(EcceURL(url).getFilePathTail().c_str()) +
              " from point " + wxString::Format("%d", index + 1));
  }
  return url;
}

void ScanPanel::OnNewCalc(wxCommandEvent&)
{
  const std::string url = newCalcFromPoint();
  if (url.empty()) return;
  // Silent success left users unsure anything happened; name the result.
  const EcceURL u(url);
  wxMessageBox("Created the calculation " +
               wxString::FromUTF8(u.getFilePathTail().c_str()) + " in " +
               wxString::FromUTF8(u.getParent().getFilePathTail().c_str()) +
               ".\nIt is in the Organizer, ready to edit and launch.",
               "New calculation", wxOK | wxICON_INFORMATION, this);
}

void ScanPanel::OnUnit(wxCommandEvent&)
{
  // The window field is in the shown unit, so its value changes with it.
  double w = 0;
  if (p_window->GetValue().ToCDouble(&w) && w > 0)
    p_window->ChangeValue(wxString::FromCDouble(
        p_unit->GetSelection() == 1 ? w / KCAL_KJ : w * KCAL_KJ, 3));
  fill();
}

// The fit is ecce-morse's (#858): the panel draws its curve and shows its
// report, so the numbers equal the command line's.
void ScanPanel::runFit()
{
  p_fitR.clear();
  p_fitE.clear();
  if (!p_morse->GetValue()) {
    p_morseState = "off";
    if (p_morseText->IsShown()) { p_morseText->Hide(); Layout(); }
    return;
  }
  IPropCalculation *calc = getCalculation();
  const std::string folder = calc ? rateLocalPath(calc->getURL().toString()) : "";
  wxString text;
  if (folder.empty()) {
    text = "The Morse fit reads calculations stored on this computer; this one "
           "is on a data server.";
    p_morseState = "error not local";
  } else {
    double w = 0;
    if (!p_window->GetValue().Strip(wxString::both).empty() &&
        (!p_window->GetValue().ToCDouble(&w) || w <= 0)) {
      text = "The window is an energy above the lowest point, a positive number "
             "(empty: every bound point).";
      p_morseState = "error window";
    } else {
      if (p_unit->GetSelection() == 1) w *= KCAL_KJ;
      wxBusyCursor busy;
      const MorseResult r = runMorse(folder, w);
      if (r.ok) {
        p_fitR = r.curveR;
        p_fitE = r.curveE;
        text = wxString::FromUTF8(r.report.c_str());
        char buf[400];
        snprintf(buf, sizeof buf, "ok re=%.10g De_eV=%.10g De_kJ=%.10g a=%.10g "
                 "omega=%.10g omegaX=%.10g D0_eV=%.10g D0_kJ=%.10g rms=%.10g "
                 "used=%d total=%d window=%.10g mode=%s", r.re, r.De_eV, r.De_kJ,
                 r.a, r.omega, r.omegaX, r.D0_eV, r.D0_kJ, r.rms, r.used, r.total,
                 r.window, r.windowMode.c_str());
        p_morseState = buf;
      } else {
        text = "No Morse fit: " + wxString::FromUTF8(r.error.c_str());
        p_morseState = "error " + r.error;
      }
    }
  }
  p_morseText->ChangeValue(text);
  if (!p_morseText->IsShown()) { p_morseText->Show(); Layout(); }
}

void ScanPanel::OnMorse(wxCommandEvent&)
{
  runFit();
  fill();
}

std::string ScanPanel::setMorse(bool on, double window)
{
  p_morse->SetValue(on);
  p_window->ChangeValue(window > 0 ? wxString::FromCDouble(window) : wxString());
  runFit();
  fill();
  return p_morseState;
}

wxString ScanPanel::morseText() const
{
  return p_morseText && p_morseText->IsShown() ? p_morseText->GetValue() : wxString();
}

namespace {
struct Candidate { std::string name; std::vector<double> coord, hartree; };

std::vector<Candidate> scansBeside(IPropCalculation *calc, const std::string& unit)
{
  std::vector<Candidate> out;
  if (!calc) return out;
  Resource *folder = EDSIFactory::getResource(calc->getURL().getParent());
  std::vector<Resource*> *kids = folder ? folder->getChildren() : 0;
  if (!kids) return out;
  const std::string own = calc->getURL().toString();
  for (Resource *r : *kids) {
    IPropCalculation *other = dynamic_cast<IPropCalculation*>(r);
    if (!other || other->getURL().toString() == own) continue;
    PropVector *c = dynamic_cast<PropVector*>(ResultsFromCalc::property(other, "SCANCOORD"));
    PropVector *e = dynamic_cast<PropVector*>(ResultsFromCalc::property(other, "SCANENERGY"));
    if (!c || !e || (c->units() == "Degree") != (unit == "degree")) continue;
    const size_t n = std::min(c->values().size(), e->values().size());
    if (n < 2) continue;
    Candidate k;
    k.name = other->getURL().getFilePathTail();
    k.coord.assign(c->values().begin(), c->values().begin() + n);
    k.hartree.assign(e->values().begin(), e->values().begin() + n);
    out.push_back(k);
  }
  std::sort(out.begin(), out.end(),
            [](const Candidate& a, const Candidate& b) { return a.name < b.name; });
  return out;
}
}

std::vector<std::string> ScanPanel::compareCandidates()
{
  std::vector<std::string> names;
  for (const Candidate& k : scansBeside(getCalculation(), p_coordUnit))
    names.push_back(k.name);
  return names;
}

bool ScanPanel::setCompared(const std::vector<std::string>& names, std::string& complaint)
{
  const std::vector<Candidate> all = scansBeside(getCalculation(), p_coordUnit);
  std::vector<Compared> chosen;
  for (const std::string& n : names) {
    auto it = std::find_if(all.begin(), all.end(),
                           [&](const Candidate& k) { return k.name == n; });
    if (it == all.end()) {
      complaint = "No scan named " + n + " beside this calculation.";
      return false;
    }
    chosen.push_back({it->name, it->coord, it->hartree});
  }
  p_compared = chosen;
  fill();
  return true;
}

void ScanPanel::OnCompare(wxCommandEvent&)
{
  const std::vector<std::string> names = compareCandidates();
  if (names.empty()) {
    wxMessageBox("No other scan of the same kind of coordinate is in this "
                 "calculation's folder.", "Compare scans", wxOK | wxICON_INFORMATION, this);
    return;
  }
  wxArrayString choices;
  wxArrayInt selected;
  for (size_t i = 0; i < names.size(); i++) {
    choices.Add(wxString::FromUTF8(names[i].c_str()));
    for (const Compared& c : p_compared)
      if (c.name == names[i]) selected.Add((int)i);
  }
  wxMultiChoiceDialog dlg(this, "Scans to draw beside this one, each relative to "
                          "its own lowest point:", "Compare scans", choices);
  dlg.SetSelections(selected);
  if (dlg.ShowModal() != wxID_OK) return;
  std::vector<std::string> picked;
  for (int i : dlg.GetSelections()) picked.push_back(names[i]);
  std::string why;
  setCompared(picked, why);
}

bool ScanPanel::savePng(const wxString& path, int width, int height)
{
  static bool handlers = false;
  if (!handlers) { wxInitAllImageHandlers(); handlers = true; }
  const PlotPalette was = p_plot->palette();
  p_plot->setPalette(PlotPalette::light());
  wxBitmap bmp(width, height);
  {
    wxMemoryDC dc(bmp);
    p_plot->paintOnto(dc, wxSize(width, height));
  }
  p_plot->setPalette(was);
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

bool ScanPanel::exportCsv(const wxString& path, bool kcal)
{
  IPropCalculation *calc = getCalculation();
  const std::string text = scanCsv(p_coord, p_hartree, p_coordUnit, kcal,
      calc ? calc->getURL().getFilePathTail() : std::string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void ScanPanel::OnExport(wxCommandEvent&)
{
  IPropCalculation *calc = getCalculation();
  wxFileDialog dlg(this, "Export scan", wxEmptyString,
                   wxString::FromUTF8(calc ? calc->getURL().getFilePathTail().c_str() : "scan") +
                   "-scan.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath(), p_unit->GetSelection() == 1))
    wxMessageBox("The file could not be written.", "Export scan",
                 wxOK | wxICON_ERROR, this);
}
