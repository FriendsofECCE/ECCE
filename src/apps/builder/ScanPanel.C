#include <algorithm>
#include <cstdio>
#include <cmath>
#include <memory>
#include <numeric>
#include <locale>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/radiobox.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
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
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "OpenCalculationDialog.H"
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
#include "dsm/ICalculation.H"
#include "dsm/DeckWriter.H"
#include "tdat/Counterpoise.H"
#include "util/JMSPublisher.H"
#include "util/JMSMessage.H"

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
const wxWindowID ID_ZERO = wxNewId();
const wxWindowID ID_JOIN = wxNewId();
const wxWindowID ID_SMALL = wxNewId();
const wxWindowID ID_CLEAR = wxNewId();
const wxWindowID ID_CP = wxNewId();
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
  p_cpTimer.Stop();
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
  bar->Add(new wxButton(this, ID_CP, "Counterpoise single points"), 0, wxRIGHT, 6);
  bar->Add(new wxButton(this, ID_EXPORT, "Export..."), 0, wxRIGHT, 12);
  bar->Add(new wxButton(this, ID_COMPARE, "Compare..."), 0, wxRIGHT, 12);
  bar->Add(new wxButton(this, ID_CLEAR, "Clear comparisons"), 0, wxRIGHT, 6);
  bar->Add(new wxStaticText(this, wxID_ANY, "Zero:"), 0, wxALIGN_CENTER_VERTICAL);
  p_zero = new wxChoice(this, ID_ZERO);
  p_zero->Append("Lowest point");
  p_zero->Append("Largest coordinate");
  p_zero->Append("First point");
  p_zero->SetSelection(0);
  bar->Add(p_zero, 0, wxRIGHT, 6);
  p_join = new wxCheckBox(this, ID_JOIN, "Join");
  p_join->SetToolTip("Merge scans with matching first structures and energies, sorted by coordinate");
  bar->Add(p_join, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_small = new wxCheckBox(this, ID_SMALL, "Small multiples");
  bar->Add(p_small, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
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

  auto *area = new wxScrolledWindow(this);
  area->SetScrollRate(0, 10);
  p_plotArea = area;
  p_plotArea->SetSizer(new wxBoxSizer(wxVERTICAL));
  p_plot = new XYPlotCanvas(p_plotArea);
  p_plots.push_back(p_plot);
  p_plot->SetMinSize(wxSize(300, 180));
  p_plot->setPointHandler([this](int curve, int index) {
    if (curve >= 0 && curve < (int)p_curves.size() &&
        index >= 0 && index < (int)p_curves[curve].ownPoint.size() &&
        p_curves[curve].ownPoint[index] >= 0)
      selectPoint(p_curves[curve].ownPoint[index]);
  });
  p_plotArea->GetSizer()->Add(p_plot, 1, wxEXPAND);
  GetSizer()->Add(p_plotArea, 3, wxEXPAND);

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

  Bind(wxEVT_CHOICE, &ScanPanel::OnComparisonOptions, this, ID_ZERO);
  Bind(wxEVT_CHECKBOX, &ScanPanel::OnComparisonOptions, this, ID_JOIN);
  Bind(wxEVT_CHECKBOX, &ScanPanel::OnComparisonOptions, this, ID_SMALL);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    std::string why;
    setCompared({}, why);
  }, ID_CLEAR);

  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    std::string why;
    const std::string url = counterpoiseSinglePoints(true, why);
    const std::string message = url.empty() ? why : "Created " + url + "\n" +
        (why.empty() ? "Launch requested with the scan machine settings. Check progress in the Organizer." : why);
    wxMessageBox(wxString::FromUTF8(message.c_str()),
        "Counterpoise single points", wxOK | (url.empty() ? wxICON_WARNING : wxICON_INFORMATION), this);
  }, ID_CP);
  p_cpTimer.SetOwner(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { if (IsShownOnScreen()) refresh(); }, p_cpTimer.GetId());

  bool table = false;
  ewxConfig::getConfig(INIFILE)->Read("ScanPanel/Table", &table, false);
  showView(table);

  fill();
  auto *resource = dynamic_cast<Resource*>(calculation);
  if (p_cp.empty() && resource && !resource->getProp("ecce:scanCounterpoise").empty())
    p_cpTimer.Start(3000);
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

  readCounterpoise();
  rebuildCurves();
  drawCurves();
  const double zero = p_cp.empty()
      ? (p_curves.empty() ? lo : referenceEnergy(p_curves[0]))
      : referenceEnergy(readScan(getCalculation()));

  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  p_grid->AppendRows((int)n);
  const int columns = p_cp.empty() ? 3 : 5;
  if (p_grid->GetNumberCols() < columns) p_grid->AppendCols(columns - p_grid->GetNumberCols());
  if (p_grid->GetNumberCols() > columns) p_grid->DeleteCols(columns, p_grid->GetNumberCols() - columns);
  p_grid->SetColLabelValue(2, unit);
  if (!p_cp.empty()) {
    p_grid->SetColLabelValue(3, wxString::FromUTF8("ΔE (raw) (") + unit + ")");
    p_grid->SetColLabelValue(4, wxString::FromUTF8("ΔE (CP) (") + unit + ")");
  }
  for (size_t i = 0; i < n; i++) {
    p_grid->SetRowLabelValue((int)i, wxString::Format("%d", (int)i + 1));
    p_grid->SetCellValue((int)i, 0, wxString::Format("%.4f", p_coord[i]));
    p_grid->SetCellValue((int)i, 1, wxString::Format("%.6f", p_hartree[i]));
    p_grid->SetCellValue((int)i, 2,
        wxString::Format("%.3f", (p_hartree[i] - zero) * factor(kcal)));
    if (!p_cp.empty()) {
      p_grid->SetCellValue(i, 3, wxString::Format("%.3f", p_cpRaw[i] * factor(kcal)));
      p_grid->SetCellValue(i, 4, wxString::Format("%.3f", p_cp[i] * factor(kcal)));
    }
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
  markPoint(p_selected);
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
  p_plotArea->Show(!table);
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
  selectOrbitalPoint(calc, p_frame[index], index, getOwner());
  return p_frame[index];
}

int ScanPanel::selectPoint(int index)
{
  if (index < 0 || index >= (int)p_coord.size()) return -1;
  p_selected = index;
  p_syncing = true;
  markPoint(index);
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
  std::vector<Compared> chosen;
  const EcceURL own = getCalculation()->getURL();
  for (const std::string& name : names) {
    EcceURL url(name);
    if (name.find("://") == std::string::npos &&
        !wxFileName(wxString::FromUTF8(name.c_str())).IsAbsolute())
      url = own.getParent().getChild(name);
    IPropCalculation *calc = dynamic_cast<IPropCalculation*>(EDSIFactory::getResource(url));
    Compared c = readScan(calc);
    PropVector *coord = calc ? dynamic_cast<PropVector*>(ResultsFromCalc::property(calc, "SCANCOORD")) : nullptr;
    if (!coord || c.coord.size() < 2 ||
        (coord->units() == "Degree") != (p_coordUnit == "degree")) {
      complaint = "No scan named " + name + " with a compatible coordinate and energies.";
      return false;
    }
    if (EcceURL(c.url) == own) continue;
    if (std::none_of(chosen.begin(), chosen.end(), [&](const Compared& x) { return EcceURL(x.url) == EcceURL(c.url); }))
      chosen.push_back(c);
  }
  p_compared = chosen;
  fill();
  complaint.clear();
  return true;
}

void ScanPanel::OnCompare(wxCommandEvent&)
{
  OpenCalculationDialog dlg(this);
  dlg.SetTitle("Compare scans: add a calculation");
  dlg.setServerChoice(getCalculation()->getURL().getParent().toString());
  if (dlg.ShowModal() != wxID_OK) return;
  std::vector<std::string> picked;
  for (const Compared& c : p_compared) picked.push_back(c.url);
  picked.push_back(dlg.GetPath().ToStdString());
  std::string why;
  if (!setCompared(picked, why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Compare scans", wxOK | wxICON_WARNING, this);
}

bool ScanPanel::savePng(const wxString& path, int width, int height)
{
  static bool handlers = false;
  if (!handlers) { wxInitAllImageHandlers(); handlers = true; }
  if (width < 200 || height < 160 * (int)p_plots.size()) return false;
  wxBitmap bmp(width, height, 24);
  {
    wxMemoryDC dc(bmp);
    // Paint each panel separately so clearing one canvas cannot erase another.
    for (size_t i = 0; i < p_plots.size(); ++i) {
      const int top = height * i / p_plots.size();
      const int h = height * (i + 1) / p_plots.size() - top;
      XYPlotCanvas *plot = p_plots[i];
      const PlotPalette was = plot->palette();
      plot->setPalette(PlotPalette::light());
      wxBitmap part(width, h, 24);
      wxMemoryDC panel(part);
      plot->paintOnto(panel, wxSize(width, h));
      dc.Blit(0, top, width, h, &panel, 0, 0);
      plot->setPalette(was);
    }
  }
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

bool ScanPanel::exportCsv(const wxString& path, bool kcal)
{
  std::string text;
  for (const Compared& c : p_curves) {
    // Retain the single-scan CSV schema; each labelled block is one drawn curve.
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(12);
    out << "# ECCE relaxed scan\n# calculation: " << c.name
        << "\n# sources: " << c.url << "\n# coordinate unit: " << p_coordUnit << "\n# zero: "
        << (c.interaction ? "separated fragments at each geometry" : p_zero->GetStringSelection().ToStdString()) << "\n"
        << "point,coordinate_" << p_coordUnit << ",energy_hartree,"
        << (c.interaction ? "interaction_energy_" : "relative_energy_")
        << (kcal ? "kcal/mol" : "kJ/mol");
    if (c.interaction) out << ",scan_energy_hartree,cp_energy_hartree";
    out << "\n";
    const double zero = referenceEnergy(c);
    for (size_t i = 0; i < c.coord.size(); ++i) {
      out << i + 1 << ',' << c.coord[i] << ',' << c.hartree[i] << ','
          << (c.hartree[i] - zero) * factor(kcal);
      if (c.interaction) {
        const int point = c.ownPoint[i];
        out << ',' << p_hartree[point] << ',' << p_cp[point];
      }
      out << "\n";
    }
    text += out.str() + "\n";
  }
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
                   "-scan.csv", "CSV files (*.csv)|*.csv|PNG files (*.png)|*.png",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  const bool ok = dlg.GetFilterIndex() == 1
      ? savePng(dlg.GetPath(), 1000, std::max(560, 260 * (int)p_plots.size()))
      : exportCsv(dlg.GetPath(), p_unit->GetSelection() == 1);
  if (!ok)
    wxMessageBox("The file could not be written.", "Export scan",
                 wxOK | wxICON_ERROR, this);
}

ScanPanel::Compared ScanPanel::readScan(IPropCalculation *calc) const
{
  Compared c;
  if (!calc) return c;
  auto *x = dynamic_cast<PropVector*>(ResultsFromCalc::property(calc, "SCANCOORD"));
  auto *e = dynamic_cast<PropVector*>(ResultsFromCalc::property(calc, "SCANENERGY"));
  if (!x || !e || x->values().size() != e->values().size()) return c;
  for (size_t i = 0; i < x->values().size(); ++i)
    if (!std::isfinite(x->values()[i]) || !std::isfinite(e->values()[i])) return c;
  c.coord = x->values();
  c.hartree = e->values();
  c.url = calc->getURL().toString();
  c.name = calc->getURL().getFilePathTail();
  if (!c.hartree.empty()) c.firstEnergy = c.hartree.front();
  c.ownPoint.assign(c.coord.size(), -1);
  if (calc->getURL() == getCalculation()->getURL()) std::iota(c.ownPoint.begin(), c.ownPoint.end(), 0);
  auto *g = dynamic_cast<PropTSVecTable*>(ResultsFromCalc::property(calc, "SCANGEOM"));
  std::unique_ptr<Fragment> frag(calc->fragment());
  if (g && g->tables() > 0 && g->columns() == 3 && frag && g->rows() == frag->numAtoms()) {
    for (int i = 0; i < frag->numAtoms(); ++i)
      c.elements += frag->atomRef(i)->atomicSymbol() + " ";
    // Pair distances allow a translated or rotated copy of the same start.
    for (int i = 0; i < g->rows(); ++i)
      for (int j = 0; j < i; ++j) {
        double d2 = 0;
        for (int k = 0; k < 3; ++k) {
          const double d = g->value(0, i, k) - g->value(0, j, k);
          d2 += d * d;
        }
        c.start.push_back(std::sqrt(d2));
      }
  }
  return c;
}

double ScanPanel::referenceEnergy(const Compared& c) const
{
  if (c.hartree.empty() || c.interaction) return 0;
  if (p_zero->GetSelection() == 1)
    return c.hartree[std::max_element(c.coord.begin(), c.coord.end()) - c.coord.begin()];
  if (p_zero->GetSelection() == 2) return c.firstEnergy;
  return *std::min_element(c.hartree.begin(), c.hartree.end());
}

void ScanPanel::rebuildCurves()
{
  p_curves.clear();
  Compared own = readScan(getCalculation());
  if (!p_cp.empty()) {
    own.hartree = p_cpRaw;
    own.interaction = true;
    own.name += " ΔE (raw)";
  }
  if (!own.coord.empty()) p_curves.push_back(own);
  int merged = 0;
  for (const Compared& c : p_compared) {
    auto match = p_curves.end();
    if (p_join->GetValue() && !c.start.empty())
      match = std::find_if(p_curves.begin(), p_curves.end(), [&](const Compared& a) {
        if (a.interaction != c.interaction || a.elements != c.elements || a.start.size() != c.start.size() ||
            std::fabs(a.coord.front() - c.coord.front()) > 1e-5 ||
            std::fabs(a.firstEnergy - c.firstEnergy) > 1e-6) return false;
        for (size_t i = 0; i < a.start.size(); ++i)
          if (!std::isfinite(a.start[i]) || !std::isfinite(c.start[i]) ||
              std::fabs(a.start[i] - c.start[i]) > 1e-4) return false;
        return true;
      });
    if (match == p_curves.end()) { p_curves.push_back(c); continue; }
    // Keep one copy of the common first point. Other overlaps are retained:
    // hysteresis can give different energies at the same coordinate.
    match->name += " + " + c.name;
    match->url += " | " + c.url;
    match->coord.insert(match->coord.end(), c.coord.begin() + 1, c.coord.end());
    match->hartree.insert(match->hartree.end(), c.hartree.begin() + 1, c.hartree.end());
    match->ownPoint.insert(match->ownPoint.end(), c.ownPoint.begin() + 1, c.ownPoint.end());
    ++merged;
  }
  if (p_join->GetValue()) {
    for (Compared& c : p_curves) {
      std::vector<size_t> order(c.coord.size());
      std::iota(order.begin(), order.end(), 0);
      std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return c.coord[a] < c.coord[b];
      });
      Compared sorted = c;
      for (size_t i = 0; i < order.size(); ++i) {
        sorted.coord[i] = c.coord[order[i]];
        sorted.hartree[i] = c.hartree[order[i]];
        sorted.ownPoint[i] = c.ownPoint[order[i]];
      }
      c = sorted;
    }
    setStatus(wxString::Format("Join: %d scan(s) merged; %d curve(s). Unmatched starts stay separate.",
                               merged, (int)p_curves.size()));
  } else setStatus("");
  if (!p_cp.empty()) {
    Compared corrected = readScan(getCalculation());
    corrected.name += " ΔE (CP)";
    corrected.hartree = p_cp;
    corrected.interaction = true;
    p_curves.push_back(corrected);
  }
}

void ScanPanel::drawCurves()
{
  const bool small = p_small->GetValue();
  const size_t count = small ? std::max(size_t(1), p_curves.size()) : 1;
  while (p_plots.size() > count) {
    XYPlotCanvas *last = p_plots.back();
    p_plotArea->GetSizer()->Detach(last);
    last->Destroy();
    p_plots.pop_back();
  }
  while (p_plots.size() < count) {
    const size_t curve = p_plots.size();
    auto *plot = new XYPlotCanvas(p_plotArea);
    plot->SetMinSize(wxSize(300, 180));
    plot->setPointHandler([this, curve](int, int index) {
      const auto& points = p_curves[curve].ownPoint;
      if (index >= 0 && index < (int)points.size() && points[index] >= 0)
        selectPoint(points[index]);
    });
    p_plotArea->GetSizer()->Add(plot, 1, wxEXPAND);
    p_plots.push_back(plot);
  }
  double xlo = 0, xhi = 0;
  bool first = true;
  for (const Compared& c : p_curves)
    for (double x : c.coord) {
      if (first) { xlo = xhi = x; first = false; }
      xlo = std::min(xlo, x); xhi = std::max(xhi, x);
    }
  const double pad = std::max(1e-5, (xhi - xlo) * .04);
  const bool kcal = p_unit->GetSelection() == 1;
  const wxString unit = kcal ? "kcal/mol" : "kJ/mol";
  for (size_t i = 0; i < count; ++i) {
    auto *plot = p_plots[i];
    plot->clearCurves();
    plot->setTitle("");
    plot->setLeftMargin(small ? 110 : 0);
    plot->setLockX(small);
    plot->setXWindow(small ? xlo - pad : 0, small ? xhi + pad : 0);
    plot->setShowXAxis(i + 1 == count);
    plot->setXAxisLabel(p_coordUnit == "degree" ? "Scanned coordinate (degrees)"
        : wxString::FromUTF8("Scanned coordinate (Å)"));
    plot->setYAxisLabel((p_cp.empty() ? "Relative energy (" : "Energy difference (") + unit + ")");
    plot->setReadout("Point", "Coordinate", p_coordUnit == "degree" ? "degree" : "angstrom",
                     p_cp.empty() ? "Relative energy" : "Energy difference", unit);
    plot->setShowKey(true);
  }
  for (size_t i = 0; i < p_curves.size(); ++i) {
    const Compared& c = p_curves[i];
    const double zero = referenceEnergy(c);
    std::vector<double> y(c.coord.size());
    for (size_t j = 0; j < y.size(); ++j) y[j] = (c.hartree[j] - zero) * factor(kcal);
    // Full URLs disambiguate identical calculation names in different folders.
    const bool duplicate = std::count_if(p_curves.begin(), p_curves.end(),
        [&](const Compared& a) { return a.name == c.name; }) > 1;
    p_plots[small ? i : 0]->addCurve(c.coord, y,
        wxString::FromUTF8((duplicate ? c.url : c.name).c_str()), (int)i);
  }
  if (p_cp.empty() && p_morse->GetValue() && !p_fitR.empty() && !p_curves.empty()) {
    std::vector<double> y(p_fitE.size());
    const double zero = referenceEnergy(p_curves[0]);
    for (size_t i = 0; i < y.size(); ++i) y[i] = (p_fitE[i] - zero) * factor(kcal);
    const int fit = p_plot->addCurve(p_fitR, y, "Morse fit", 0, XYPlotCanvas::LINE);
    p_plot->setCurveStyle(fit, true, 2);
  }
  p_plotArea->Layout();
  p_plotArea->FitInside();
  Layout();
  markPoint(p_selected);
}

void ScanPanel::markPoint(int index)
{
  for (auto *plot : p_plots) plot->setSelectedPoint(-1, -1);
  if (index < 0 || p_curves.empty()) return;
  const auto& map = p_curves[0].ownPoint;
  auto pos = std::find(map.begin(), map.end(), index);
  if (pos != map.end()) p_plot->setSelectedPoint(0, (int)(pos - map.begin()));
}

bool ScanPanel::setZero(const std::string& mode)
{
  int selection = mode == "lowest" ? 0 : mode == "largest" ? 1 : mode == "first" ? 2 : -1;
  if (selection < 0) return false;
  p_zero->SetSelection(selection);
  fill();
  return true;
}

void ScanPanel::setJoin(bool on) { p_join->SetValue(on); fill(); }
void ScanPanel::setSmallMultiples(bool on) { p_small->SetValue(on); fill(); }
void ScanPanel::OnComparisonOptions(wxCommandEvent&) { fill(); }


std::string ScanPanel::scanSnapshot() const
{
  auto *calc = getCalculation();
  auto *geom = calc ? dynamic_cast<PropTSVecTable*>(ResultsFromCalc::property(calc, "SCANGEOM")) : nullptr;
  if (!geom || p_coord.empty() || geom->tables() != p_coord.size() || geom->columns() != 3) return "";
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(17);
  for (size_t point = 0; point < p_coord.size(); ++point) {
    if (point) out << ';';
    for (double v : geom->values(point)) {
      if (!std::isfinite(v)) return "";
      out << v << ' ';
    }
  }
  return out.str();
}

std::string ScanPanel::counterpoiseSinglePoints(bool launch, std::string& why)
{
  auto *source = dynamic_cast<ICalculation*>(getCalculation());
  if (!source || !source->application() ||
      (source->application()->getCodeName() != "ECCE-QM" && source->application()->getCodeName() != "ORCA")) {
    why = "Counterpoise scan single points require ECCE-QM or ORCA."; return "";
  }
  std::unique_ptr<Fragment> frag(source->fragment());
  if (!frag || frag->fragmentStates().size() < 2) {
    why = "Counterpoise needs at least two fragments in the stored scan. Define fragments in the Builder and save first."; return "";
  }
  if (!Counterpoise::validate(*frag, why)) return "";
  const std::string snapshot = scanSnapshot();
  if (snapshot.empty()) { why = "The scan needs a stored geometry for every point."; return ""; }
  if (source->getState() != ResourceDescriptor::STATE_COMPLETED) {
    why = "Wait until the relaxed scan is completed."; return "";
  }
  const std::string previous = source->getProp("ecce:scanCounterpoise");
  if (!previous.empty()) {
    why = "This scan already has a counterpoise calculation: " + previous; return "";
  }
  EcceURL target = source->getURL().getParent().getChild(source->getURL().getFilePathTail() + "_cp");
  if (!source->copy(target)) { why = source->messages(); return ""; }
  auto *copy = dynamic_cast<ICalculation*>(EDSIFactory::getResource(target));
  bool ok = copy && copy->resetForRerun(ResourceDescriptor::STATE_CREATED);
  if (ok) {
    copy->jobdata(Jobdata());
    copy->setGeomConstraintModel(nullptr);
    // Old stored constraints may not have a configuration entry.
    if (auto *file = EDSIFactory::getResource(target.getChild("Parameters").getChild("GeomConstraintModel.gcst")))
      file->remove();
    std::unique_ptr<GUIValues> gui(copy->guiparams());
    gui->deletePrefix("ES.Runtype.");
    for (const auto& item : std::vector<std::pair<std::string, std::string>>{
        {"ES.Runtype.Counterpoise", "1"}, {"ES.Runtype.CounterpoiseOwn", "1"},
        {"ES.Runtype.CPScanGeometry", snapshot}}) {
      auto *value = new GUIValue;
      value->m_key = item.first; value->m_type = "text_input";
      value->m_sensitive = value->m_write = true;
      value->setValue(item.second); gui->set(item.first, value);
    }
    ok = copy->runtype(TRunType("Energy", false)) && copy->guiparams(gui.get());
    if (ok) {
      const auto result = DeckWriter::generate(copy);
      ok = result.ok;
      if (!ok) why = result.message;
    }
    if (ok) ok = copy->setState(ResourceDescriptor::STATE_READY) &&
        copy->addProp("ecce:scanSource", source->getURL().toString()) &&
        source->addProp("ecce:scanCounterpoise", target.toString());
  }
  if (!ok) {
    if (why.empty()) why = "Could not prepare the counterpoise calculation.";
    if (copy) copy->remove();
    return "";
  }
  EDSIFactory::notifyCreate(target.toString(), true);
  if (launch) {
    JMSPublisher publisher("Builder");
    std::unique_ptr<JMSMessage> msg(publisher.newMessage());
    msg->addProperty("reply_id", "scan-cp-" + target.toString());
    msg->addIntProperty("n", 1); msg->addProperty("url0", target.toString());
    if (!publisher.publish("ecce_launch_request", *msg))
      why = "Created the calculation, but the launch request could not be sent. Launch it from the Organizer.";
  }
  p_cpTimer.Start(3000);
  setStatus("Counterpoise calculation: " + wxString::FromUTF8(target.toString().c_str()));
  return target.toString();
}

void ScanPanel::readCounterpoise()
{
  p_cp.clear(); p_cpRaw.clear();
  auto *source = dynamic_cast<ICalculation*>(getCalculation());
  if (!source) return;
  const std::string url = source->getProp("ecce:scanCounterpoise");
  if (url.empty()) return;
  auto *copy = dynamic_cast<ICalculation*>(EDSIFactory::getResource(EcceURL(url)));
  if (!copy) return;
  EDSIFactory::changePoolResource(copy->getURL());
  if (copy->getState() != ResourceDescriptor::STATE_COMPLETED) return;
  p_cpTimer.Stop();
  std::unique_ptr<GUIValues> gui(copy->guiparams());
  auto *snapshot = gui->get("ES.Runtype.CPScanGeometry");
  if (!snapshot || snapshot->getValueAsString() != scanSnapshot()) return;
  auto *cp = dynamic_cast<PropVector*>(copy->getProperty("SCANCP"));
  auto *raw = dynamic_cast<PropVector*>(copy->getProperty("SCANRAW"));
  if (!cp || !raw || cp->values().size() != p_coord.size() || raw->values().size() != p_coord.size()) return;
  for (double v : cp->values()) if (!std::isfinite(v)) return;
  for (double v : raw->values()) if (!std::isfinite(v)) return;
  p_cp = cp->values(); p_cpRaw = raw->values();
  p_cpTimer.Stop();
}

std::string ScanPanel::counterpoiseColumn()
{
  refresh();
  std::ostringstream out;
  out.imbue(std::locale::classic()); out.precision(12);
  out << "point coordinate raw_hartree cp_hartree\n";
  for (size_t i = 0; i < p_coord.size(); ++i) {
    out << i + 1 << ' ' << p_coord[i] << ' ';
    if (p_cp.empty()) out << "pending pending";
    else out << p_cpRaw[i] << ' ' << p_cp[i];
    out << '\n';
  }
  return out.str();
}
