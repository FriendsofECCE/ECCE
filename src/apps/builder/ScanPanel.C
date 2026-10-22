#include <algorithm>
#include <cstdio>
#include <locale>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/event.h>

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

#include "ScanPanel.H"

namespace {
const double HARTREE_KJ = 2625.4996;
const double HARTREE_KCAL = 627.5095;
const wxWindowID ID_UNIT = wxNewId();
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_NEWCALC = wxNewId();
const wxWindowID ID_PLAYBACK = wxNewId();

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
  : VizPropertyPanel(), p_selected(-1), p_unit(NULL), p_plot(NULL), p_grid(NULL),
    p_status(NULL), p_playback(NULL), p_delay(500), p_syncing(false)
{
}

ScanPanel::ScanPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(), p_selected(-1), p_unit(NULL), p_plot(NULL), p_grid(NULL),
    p_status(NULL), p_playback(NULL), p_delay(500), p_syncing(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}

ScanPanel::~ScanPanel()
{
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

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(this, wxID_ANY, "Energy unit:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_unit = new wxChoice(this, ID_UNIT);
  p_unit->Append("kJ/mol");
  p_unit->Append("kcal/mol");
  p_unit->SetSelection(0);
  bar->Add(p_unit, 0, wxRIGHT, 12);
  bar->AddStretchSpacer();
  bar->Add(new wxButton(this, ID_NEWCALC, "New calculation from this point"), 0,
           wxRIGHT, 6);
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(300, 180));
  p_plot->setPointHandler([this](int, int index) { selectPoint(index); });
  GetSizer()->Add(p_plot, 3, wxEXPAND);

  wxBoxSizer *play = new wxBoxSizer(wxHORIZONTAL);
  p_playback = new PlaybackControl(this, ID_PLAYBACK);
  p_playback->SetDelay(&p_delay);
  p_playback->SetStopIndex(0L);
  if (wxWindow *rec = FindWindow(PlaybackControl::ID_RECORD)) rec->Hide();
  play->Add(p_playback, 0, wxALIGN_CENTER_VERTICAL);
  p_status = new wxStaticText(this, wxID_ANY, "");
  GetSizer()->Add(play, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  GetSizer()->Add(p_status, 0, wxEXPAND | wxALL, 4);

  p_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 110));
  p_grid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
  p_grid->EnableEditing(false);
  p_grid->SetRowLabelSize(40);
  p_grid->SetColLabelValue(0, "Coordinate");
  p_grid->SetColLabelValue(1, "Energy (Eh)");
  p_grid->SetColLabelValue(2, "Relative");
  GetSizer()->Add(p_grid, 2, wxEXPAND | wxALL, 3);

  p_grid->Bind(wxEVT_GRID_SELECT_CELL, &ScanPanel::OnRow, this);
  Bind(wxEVT_CHOICE, &ScanPanel::OnUnit, this, ID_UNIT);
  Bind(wxEVT_BUTTON, &ScanPanel::OnExport, this, ID_EXPORT);
  Bind(wxEVT_BUTTON, &ScanPanel::OnNewCalc, this, ID_NEWCALC);

  fill();
  return true;
}

void ScanPanel::fill()
{
  p_coord.clear();
  p_hartree.clear();
  p_frame.clear();
  IPropCalculation *calc = getCalculation();
  PropVector *c = calc ? dynamic_cast<PropVector*>(calc->getProperty("SCANCOORD")) : 0;
  PropVector *e = calc ? dynamic_cast<PropVector*>(calc->getProperty("SCANENERGY")) : 0;
  if (!c || !e) return;
  const size_t n = std::min(c->values().size(), e->values().size());
  p_coord.assign(c->values().begin(), c->values().begin() + n);
  p_hartree.assign(e->values().begin(), e->values().begin() + n);
  p_coordUnit = c->units() == "Degree" ? "degree" : "angstrom";

  // One structure per point, or none: a partial set cannot be matched up.
  PropTSVecTable *geom =
      calc ? dynamic_cast<PropTSVecTable*>(calc->getProperty("SCANGEOM")) : 0;
  p_frame.assign(n, -1);
  if (geom && geom->tables() == n)
    for (size_t i = 0; i < n; i++) p_frame[i] = (int)i;

  if (p_playback) p_playback->SetStopIndex(n ? (long)n - 1 : 0);
  const bool kcal = p_unit->GetSelection() == 1;
  const wxString unit = kcal ? "kcal/mol" : "kJ/mol";
  const double lo = n ? *std::min_element(p_hartree.begin(), p_hartree.end()) : 0;

  p_plot->clearCurves();
  if (n) {
    std::vector<double> y(n);
    for (size_t i = 0; i < n; i++) y[i] = (p_hartree[i] - lo) * factor(kcal);
    p_plot->addCurve(p_coord, y);
  }
  p_plot->setXAxisLabel(p_coordUnit == "degree" ? "Scanned coordinate (degrees)"
                                                 : "Scanned coordinate (Angstrom)");
  p_plot->setYAxisLabel("Relative energy (" + unit + ")");

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

int ScanPanel::showPoint(int index)
{
  if (index < 0 || index >= (int)p_frame.size() || p_frame[index] < 0) return -1;
  IPropCalculation *calc = getCalculation();
  if (!calc) return -1;
  p_selected = index;
  SGContainer& sg = getFW().getSceneGraph();
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
  ecce::EventDispatcher::getDispatcher().publish(ecce::Event("StepChange"));
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

void ScanPanel::OnPlayback(wxCommandEvent& event)
{
  selectPoint((int)event.GetExtraLong());
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
  newCalcFromPoint();
}

void ScanPanel::OnUnit(wxCommandEvent&)
{
  fill();
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
