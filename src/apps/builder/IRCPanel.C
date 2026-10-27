#include <algorithm>
#include <cmath>
#include <cstdio>
#include <locale>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/wrapsizer.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TProperty.H"
#include "dsm/IPropCalculation.H"
#include "viz/GTStepCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "util/EcceURL.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "wxgui/PlaybackControl.H"
#include "XYPlotCanvas.H"
#include "PointCalc.H"

#include "PlaybackExtras.H"
#include "PlaybackLoop.H"
#include "PointNote.H"
#include "IRCPanel.H"

namespace {
const double HARTREE_KJ = 2625.4996;
const double HARTREE_KCAL = 627.5095;
const wxWindowID ID_UNIT = wxNewId();
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_NEWCALC = wxNewId();
const wxWindowID ID_VIEW = wxNewId();
const wxWindowID ID_PLAYBACK = wxNewId();

double factor(bool kcal) { return kcal ? HARTREE_KCAL : HARTREE_KJ; }

// The TS is the point at coordinate 0 (the one nearest to it).
size_t tsIndex(const std::vector<double>& coord)
{
  size_t ts = 0;
  for (size_t i = 1; i < coord.size(); i++)
    if (std::fabs(coord[i]) < std::fabs(coord[ts])) ts = i;
  return ts;
}
}

std::string ircCsv(const std::vector<double>& coord,
                   const std::vector<double>& hartree,
                   const std::string& coordUnit, bool kcal,
                   const std::string& calculation)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(10);
  out << "# ECCE IRC path\n";
  out << "# calculation: " << calculation << "\n";
  out << "# coordinate unit: " << coordUnit << "\n";
  out << "# relative energy is to the first point of the path (the reactant end)\n";
  out << "point,coordinate_" << coordUnit << ",energy_hartree,relative_energy_"
      << (kcal ? "kcal/mol" : "kJ/mol") << "\n";
  const size_t n = std::min(coord.size(), hartree.size());
  const double ref = n ? hartree[0] : 0;
  for (size_t i = 0; i < n; i++)
    out << i + 1 << "," << coord[i] << "," << hartree[i] << ","
        << (hartree[i] - ref) * factor(kcal) << "\n";
  return out.str();
}

std::vector<int> ircFrameMap(const std::vector<double>& coord, int frames)
{
  const int n = (int)coord.size();
  std::vector<int> map(n, -1);
  if (n == 0) return map;
  const size_t ts = tsIndex(coord);
  int nF = 0, nR = 0;
  for (int i = 0; i < n; i++) {
    if ((size_t)i == ts) continue;
    if (coord[i] > 0) nF++; else nR++;
  }
  // ORCA stores one structure per summary row, TS included, in path order.
  if (frames == n) {
    for (int i = 0; i < n; i++) map[i] = i;
    return map;
  }
  if (frames != nF + nR) return map;
  // Points are in path order, so counting away from the TS gives the rank.
  int rank = 0;
  for (int i = (int)ts + 1; i < n; i++) map[i] = rank++;
  rank = 0;
  for (int i = (int)ts - 1; i >= 0; i--) map[i] = nF + rank++;
  return map;
}

IMPLEMENT_DYNAMIC_CLASS(IRCPanel, VizPropertyPanel)

BEGIN_EVENT_TABLE(IRCPanel, VizPropertyPanel)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_TICK_EVENT, IRCPanel::OnPlayback)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_LOOP_EVENT, IRCPanel::OnPlayback)
END_EVENT_TABLE()

IRCPanel::IRCPanel()
  : VizPropertyPanel(), p_unit(NULL), p_plot(NULL), p_grid(NULL), p_view(NULL), p_status(NULL), p_playback(NULL), p_extras(NULL), p_delay(500)
{
}

IRCPanel::IRCPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(), p_unit(NULL), p_plot(NULL), p_grid(NULL), p_view(NULL), p_status(NULL), p_playback(NULL), p_extras(NULL), p_delay(500)
{
  Create(calculation, parent, id, pos, size, style, name);
}

IRCPanel::~IRCPanel()
{
  delete p_extras;
}

bool IRCPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("IRCPanel creation failed") );
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
  wxButton *newCalc = new wxButton(this, ID_NEWCALC, "New calculation...");
  newCalc->SetToolTip("New calculation from the selected point of the path");
  bar->Add(newCalc, 0, wxRIGHT, 6);
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(300, 180));
  p_plot->setPointHandler([this](int, int index) { selectPoint(index); });
  GetSizer()->Add(p_plot, 3, wxEXPAND);

  // Reverse end, TS, forward end: the table is already in that order.
  p_playback = new PlaybackControl(this, ID_PLAYBACK);
  p_playback->SetDelay(&p_delay);
  p_playback->SetStopIndex(0L);
  if (wxWindow *rec = FindWindow(PlaybackControl::ID_RECORD)) rec->Hide();
  GetSizer()->Add(p_playback, 0, wxLEFT | wxRIGHT, 4);
  p_extras = new PlaybackExtras(this, GetSizer(), &p_delay);

  p_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 110));
  p_grid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
  p_grid->EnableEditing(false);
  p_grid->SetRowLabelSize(40);
  p_grid->SetColLabelValue(0, "Coordinate");
  p_grid->SetColLabelValue(1, "Energy (Eh)");
  p_grid->SetColLabelValue(2, "Relative");
  p_grid->Bind(wxEVT_GRID_SELECT_CELL, &IRCPanel::OnRow, this);
  GetSizer()->Add(p_grid, 2, wxEXPAND | wxALL, 3);

  // Messages go below all the controls.
  //  It describes the selected point, so it gets a thin-bordered box of its
  //  own rather than loose text among the controls.
  wxPanel *statusBox = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxBORDER_SIMPLE);
  statusBox->SetBackgroundColour(GetBackgroundColour().ChangeLightness(106));
  p_status = new wxStaticText(statusBox, wxID_ANY, "");
  p_status->SetMinSize(wxSize(50, p_status->GetCharHeight() + 2));
  wxBoxSizer *statusSizer = new wxBoxSizer(wxVERTICAL);
  statusSizer->Add(p_status, 1, wxEXPAND | wxALL, 4);
  statusBox->SetSizer(statusSizer);
  GetSizer()->Add(statusBox, 0, wxEXPAND | wxALL, 4);

  Bind(wxEVT_CHOICE, &IRCPanel::OnUnit, this, ID_UNIT);
  Bind(wxEVT_BUTTON, &IRCPanel::OnExport, this, ID_EXPORT);
  Bind(wxEVT_BUTTON, &IRCPanel::OnNewCalc, this, ID_NEWCALC);
  Bind(wxEVT_RADIOBOX, &IRCPanel::OnView, this, ID_VIEW);

  showView(false);
  fill();
  return true;
}

void IRCPanel::fill()
{
  p_coord.clear();
  p_hartree.clear();
  p_frame.clear();
  if (p_playback) p_playback->SetStopIndex(0L);
  IPropCalculation *calc = getCalculation();
  PropVector *c = calc ? dynamic_cast<PropVector*>(calc->getProperty("IRCCOORD")) : 0;
  PropVector *e = calc ? dynamic_cast<PropVector*>(calc->getProperty("IRCENERGY")) : 0;
  if (!c || !e) return;
  const size_t n = std::min(c->values().size(), e->values().size());
  p_coord.assign(c->values().begin(), c->values().begin() + n);
  p_hartree.assign(e->values().begin(), e->values().begin() + n);
  // The unit stays ASCII in the CSV; the Unicode form is for display only.
  p_coordUnit = c->units() == "Point" ? "point" : "sqrt(amu)*bohr";
  const bool mw = p_coordUnit != "point";
  const wxString xUnit = wxString::FromUTF8("amu\xC2\xBD\xC2\xB7" "bohr");

  PropTSVecTable *geom =
      calc ? dynamic_cast<PropTSVecTable*>(calc->getProperty("IRCGEOM")) : 0;
  p_frame = ircFrameMap(p_coord, geom ? (int)geom->tables() : 0);
  if (p_playback) p_playback->SetStopIndex(n ? (long)n - 1 : 0);

  const bool kcal = p_unit->GetSelection() == 1;
  const wxString unit = kcal ? "kcal/mol" : "kJ/mol";
  const double ref = n ? p_hartree[0] : 0;

  p_plot->clearCurves();
  if (n) {
    std::vector<double> y(n);
    for (size_t i = 0; i < n; i++) y[i] = (p_hartree[i] - ref) * factor(kcal);
    p_plot->addCurve(p_coord, y);
  }
  p_plot->setXAxisLabel(mw ? "Reaction coordinate (" + xUnit + ")"
                           : wxString("Point from the TS"));
  p_plot->setYAxisLabel("Relative energy (" + unit + ")");
  p_plot->setReadout("Point", mw ? "Coordinate" : "Offset from the TS",
                     mw ? xUnit : wxString(), "Relative energy", unit);

  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  p_grid->AppendRows((int)n);
  p_grid->SetColLabelValue(0, mw ? "Coordinate (" + xUnit + ")"
                                 : wxString("Coordinate"));
  p_grid->SetColLabelValue(2, unit);
  for (size_t i = 0; i < n; i++) {
    p_grid->SetRowLabelValue((int)i, wxString::Format("%d", (int)i + 1));
    p_grid->SetCellValue((int)i, 0, wxString::Format("%.4f", p_coord[i]));
    p_grid->SetCellValue((int)i, 1, wxString::Format("%.6f", p_hartree[i]));
    p_grid->SetCellValue((int)i, 2,
        wxString::Format("%.3f", (p_hartree[i] - ref) * factor(kcal)));
  }
  p_grid->AutoSizeColumns();
}

// As in ScanPanel: the structures arrive after the energies on a live job.
void IRCPanel::refresh()
{
  fill();
  if (p_selected >= (int)p_coord.size()) p_selected = -1;
  if (p_selected >= 0 && hasFocus()) showPoint(p_selected);
}

// The orbital panels hold the last computed point's orbitals, so leaving this
// panel puts that point's structure back (as the Relaxed Scan panel does).
void IRCPanel::receiveFocus()
{
  if (p_selected >= 0) showPoint(p_selected);
}

void IRCPanel::loseFocus()
{
  IPropCalculation *calc = getCalculation();
  // With per-point orbitals the selected point's structure stays with them.
  if (calc && calc->hasPointOrbitals()) return;
  PropTSVecTable *trace =
      calc ? dynamic_cast<PropTSVecTable*>(calc->getProperty("GEOMTRACE")) : 0;
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

std::string IRCPanel::axisText(int index) const
{
  wxString t = p_plot->xAxisLabel() + " | " + p_plot->yAxisLabel() + " | " +
               p_grid->GetColLabelValue(0) + " | " + p_grid->GetColLabelValue(2) +
               " | " + p_plot->readoutText(0, index);
  return std::string(t.utf8_str());
}

std::string IRCPanel::statusText() const
{
  return p_status ? std::string(p_status->GetLabel().utf8_str()) : std::string();
}

// One of plot and table at a time, as in the Vibrations panel.
void IRCPanel::showView(bool table)
{
  p_view->SetSelection(table ? 1 : 0);
  p_plot->Show(!table);
  p_grid->Show(table);
  Layout();
}

void IRCPanel::OnView(wxCommandEvent&)
{
  showView(p_view->GetSelection() == 1);
}

int IRCPanel::selectPoint(int index)
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

void IRCPanel::OnRow(wxGridEvent& event)
{
  event.Skip();
  if (!p_syncing) selectPoint(event.GetRow());
}

// With Loop on, the control's wrap from one end to the other is turned into
// a reversal, so play runs forward and back until stopped.
void IRCPanel::OnPlayback(wxCommandEvent& event)
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

void IRCPanel::setPlaybackSpeed(const std::string& name)
{
  if (p_extras) p_extras->setSpeed(name);
}

std::string IRCPanel::playbackSpeed() const
{
  return p_extras ? p_extras->speed() : std::string();
}

void IRCPanel::setPlaybackLoop(bool on)
{
  if (p_extras) p_extras->setLoop(on);
}

bool IRCPanel::playbackLoop() const
{
  return p_extras && p_extras->loop();
}

std::vector<int> IRCPanel::playOrder(int ticks)
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

int IRCPanel::step(const std::string& which)
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

void IRCPanel::selectRow(int index)
{
  if (index >= 0 && index < points()) p_grid->GoToCell(index, 0);
}

int IRCPanel::selectedRow() const
{
  wxArrayInt rows = p_grid->GetSelectedRows();
  return rows.GetCount() ? rows[0] : -1;
}

int IRCPanel::markerPoint() const
{
  return p_plot->selectedIndex();
}

int IRCPanel::showPoint(int index)
{
  if (index < 0 || index >= (int)p_frame.size()) return -1;
  if (p_frame[index] < 0) {
    p_status->SetLabel(wxString::Format(
        "Point %d (coordinate %.4f) has no stored structure.",
        index + 1, p_coord[index]));
    return -1;
  }
  IPropCalculation *calc = getCalculation();
  if (!calc) return -1;
  p_selected = index;
  p_status->SetLabel(wxString::Format("Showing point %d (coordinate %.4f).",
                                      index + 1, p_coord[index]));
  SGContainer& sg = getFW().getSceneGraph();
  Command *cmd = new GTStepCmd("IRC Step", &sg, calc);
  cmd->getParameter("Index")->setInteger(p_frame[index]);
  cmd->getParameter("PropKey")->setString("IRCGEOM");
  cmd->execute();
  delete cmd;
  // Bonds differ along the path, so redo them from the new geometry.
  SGFragment *frag = sg.getFragment();
  frag->deleteAllBonds();
  frag->addCovalentBonds();
  frag->touchNumbers();
  sg.touchChemDisplay();
  ecce::EventDispatcher::getDispatcher().publish(ecce::Event("StepChange"));
  selectOrbitalPoint(calc, p_frame[index], index);
  return p_frame[index];
}

std::string IRCPanel::newCalcFromPoint(int index)
{
  if (index < 0) index = p_selected;
  std::string why;
  std::string url;
  if (index < 0 || index >= (int)p_frame.size() || p_frame[index] < 0)
    why = "Click a point in the plot first; the transition state itself has no "
          "stored structure in a Gaussian IRC (use its own calculation).";
  else
    url = ::newCalcFromPoint(getCalculation(), "IRCGEOM", p_frame[index],
                             index + 1, why);
  if (url.empty() && !why.empty())
    wxMessageBox(wxString::FromUTF8(why.c_str()), "New calculation",
                 wxOK | wxICON_WARNING, this);
  return url;
}

void IRCPanel::OnNewCalc(wxCommandEvent&)
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

void IRCPanel::OnUnit(wxCommandEvent&)
{
  fill();
}

bool IRCPanel::exportCsv(const wxString& path, bool kcal)
{
  IPropCalculation *calc = getCalculation();
  const std::string text = ircCsv(p_coord, p_hartree, p_coordUnit, kcal,
      calc ? calc->getURL().getFilePathTail() : std::string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void IRCPanel::OnExport(wxCommandEvent&)
{
  IPropCalculation *calc = getCalculation();
  wxFileDialog dlg(this, "Export IRC path", wxEmptyString,
                   wxString::FromUTF8(calc ? calc->getURL().getFilePathTail().c_str() : "irc") +
                   "-irc.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath(), p_unit->GetSelection() == 1))
    wxMessageBox("The file could not be written.", "Export IRC path",
                 wxOK | wxICON_ERROR, this);
}
