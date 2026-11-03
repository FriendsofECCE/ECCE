#include <algorithm>
#include <cmath>
#include <cstdio>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/toolbar.h>

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/ScanPoint.H"
#include "wxgui/ewxStaticText.H"

#include "EnsemblePropertyPanel.H"
#include "PropertyPanel.H"
#include "PropertyPanelFactory.H"
#include "XYPlotCanvas.H"

wxFORCE_LINK_THIS_MODULE(EnsemblePropertyPanel)

IMPLEMENT_DYNAMIC_CLASS(EnsemblePropertyPanel, GeomTracePropertyPanel)

BEGIN_EVENT_TABLE(EnsemblePropertyPanel, GeomTracePropertyPanel)
END_EVENT_TABLE()

namespace {

const double kcalPerMolK = 0.0019872041;
const double kelvin = 298.15;

// The noun of one member, for the plot's x axis.
std::string noun(const std::string& kind)
{
  if (kind == "Protonation Sites") return "Protonation site";
  if (kind == "Tautomers") return "Tautomer";
  if (kind == "Clusters") return "Cluster";
  if (kind == "Poses") return "Pose";
  return "Conformer";
}

}  // namespace


EnsemblePropertyPanel::EnsemblePropertyPanel()
  : GeomTracePropertyPanel(), p_hasPatterns(false), p_inSelect(false),
    p_selRank(0), p_sortCol(0), p_sortUp(true), p_table(NULL), p_export(NULL)
{
}


EnsemblePropertyPanel::EnsemblePropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : GeomTracePropertyPanel(), p_hasPatterns(false), p_inSelect(false),
    p_selRank(0), p_sortCol(0), p_sortUp(true), p_table(NULL), p_export(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


EnsemblePropertyPanel::~EnsemblePropertyPanel() {}


// Run type first; an ensemble from a run type not listed is Poses if it is
// scored (POSESCORE), else Conformers.
std::string EnsemblePropertyPanel::kindFor(const std::string& rt, bool scored)
{
  if (rt == "Protonation Sites") return "Protonation Sites";
  if (rt == "Tautomers") return "Tautomers";
  if (rt == "Solvation Shell") return "Clusters";
  if (rt == "Docking (aISS)") return "Poses";
  if (rt == "Conformer Search" || rt == "Conformer search (simple)")
    return "Conformers";
  return scored ? "Poses" : "Conformers";
}


std::string EnsemblePropertyPanel::kindOf(IPropCalculation *calc)
{
  ICalculation *c = dynamic_cast<ICalculation*>(calc);
  return kindFor(c ? c->runtype().name() : std::string(),
                 ScanPoint::ensembleIsPoses(dynamic_cast<PropertyTask*>(calc)));
}


std::string EnsemblePropertyPanel::kind() const
{
  return kindOf(getCalculation());
}


bool EnsemblePropertyPanel::isRelevant(IPropCalculation *propCalc) const
{
  if (!propCalc || !getDescription()) return false;
  if (ScanPoint::ensembleSize(dynamic_cast<PropertyTask*>(propCalc)) < 1)
    return false;
  return kindOf(propCalc) == getDescription()->name;
}


wxWindow * EnsemblePropertyPanel::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxNO_BORDER|wxTB_TEXT|wxTB_NOICONS, getName() + " Menu");
  tb->Realize();
  return tb;
}


void EnsemblePropertyPanel::load()
{
  PropertyTask *task = dynamic_cast<PropertyTask*>(getCalculation());
  const int n = ScanPoint::ensembleSize(task);
  p_members.clear();
  p_deName = ScanPoint::ensembleIsPoses(task)
                 ? "Interaction energy (kcal/mol)" : "\xCE\x94" "E (kcal/mol)";

  vector<ScanPoint::Pattern> pats;
  string why;
  p_hasPatterns = ScanPoint::ensemblePatterns(task, pats, why);

  double lowest = n ? ScanPoint::conformerEnergy(task, 1) : 0;
  double z = 0;
  for (int k = 1; k <= n; k++) {
    Member m;
    m.rank = k;
    m.de = ScanPoint::conformerEnergy(task, k);
    m.pop = std::exp(-(m.de - lowest) / (kcalPerMolK * kelvin));
    z += m.pop;
    for (size_t g = 0; g < pats.size(); g++)
      if (std::find(pats[g].members.begin(), pats[g].members.end(), k) !=
          pats[g].members.end())
        m.pattern = pats[g].label;
    p_members.push_back(m);
  }
  for (size_t i = 0; i < p_members.size(); i++) p_members[i].pop *= 100.0 / z;
}


void EnsemblePropertyPanel::initialize()
{
  if (!GetSizer()) SetSizer(new wxBoxSizer(wxVERTICAL));
  load();

  p_table = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 220),
                           wxLC_REPORT | wxLC_SINGLE_SEL);
  p_table->AppendColumn("Rank", wxLIST_FORMAT_RIGHT, 60);
  p_table->AppendColumn(wxString::FromUTF8(p_deName.c_str()), wxLIST_FORMAT_RIGHT, 150);
  p_table->AppendColumn("Population (%)", wxLIST_FORMAT_RIGHT, 110);
  if (p_hasPatterns)
    p_table->AppendColumn("Coordination pattern", wxLIST_FORMAT_LEFT, 240);
  p_table->SetToolTip("One row per member, lowest energy first; click a column "
      "heading to sort and a row to show that structure. Population is the "
      "Boltzmann weight at 298.15 K of the energy shown, within this method "
      "only: it is not a measured or cross-method population.");
  p_table->Bind(wxEVT_LIST_ITEM_SELECTED, &EnsemblePropertyPanel::OnRowSelected, this);
  p_table->Bind(wxEVT_LIST_COL_CLICK, &EnsemblePropertyPanel::OnColumn, this);
  GetSizer()->Add(p_table, 1, wxEXPAND|wxALL, 2);

  p_plotCtrl = new XYPlotCanvas(this);
  p_plotCtrl->setPointHandler([this](int, int index) { selectRank(index + 1); });
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  p_export = new wxButton(this, wxID_ANY, "Export .csv...");
  p_export->SetToolTip("Save the table (rank, energy, population, pattern) as "
                       "a CSV file");
  p_export->Bind(wxEVT_BUTTON, &EnsemblePropertyPanel::OnExport, this);
  GetSizer()->Add(p_export, 0, wxALL, 4);

  fillTable();
  fillPlot();
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


void EnsemblePropertyPanel::fillTable()
{
  p_inSelect = true;
  p_table->DeleteAllItems();
  for (size_t i = 0; i < p_members.size(); i++) {
    const Member& m = p_members[i];
    long row = p_table->InsertItem((long)i, wxString::Format("%d", m.rank));
    p_table->SetItem(row, 1, wxString::Format("%.3f", m.de));
    p_table->SetItem(row, 2, wxString::Format("%.1f", m.pop));
    if (p_hasPatterns)
      p_table->SetItem(row, 3, wxString::FromUTF8(m.pattern.c_str()));
    p_table->SetItemData(row, m.rank);
    if (m.rank == p_selRank) p_table->SetItemState(row, wxLIST_STATE_SELECTED,
                                                   wxLIST_STATE_SELECTED);
  }
  p_inSelect = false;
}


void EnsemblePropertyPanel::fillPlot()
{
  p_plotCtrl->clearCurves();
  vector<double> x, y;
  vector<Member> byRank = p_members;
  std::sort(byRank.begin(), byRank.end(),
            [](const Member& a, const Member& b) { return a.rank < b.rank; });
  for (size_t i = 0; i < byRank.size(); i++) {
    x.push_back(byRank[i].rank);
    y.push_back(byRank[i].de);
  }
  p_plotCtrl->setShowKey(false);
  p_plotCtrl->setIntegerX(true);
  const int curve = p_plotCtrl->addCurve(x, y);
  for (size_t i = 0; i < byRank.size(); i++)
    p_plotCtrl->setPointLabel(curve, (int)i, wxString::Format("%s %d: %.3f kcal/mol, %.1f %%",
        wxString::FromUTF8(noun(kind()).c_str()), byRank[i].rank, byRank[i].de,
        byRank[i].pop));
  p_plotCtrl->setXAxisLabel(wxString::FromUTF8((noun(kind()) + " (lowest first)").c_str()));
  p_plotCtrl->setYAxisLabel(wxString::FromUTF8(p_deName.c_str()));
  if (p_selRank > 0) p_plotCtrl->setSelectedPoint(0, p_selRank - 1);
}


void EnsemblePropertyPanel::refresh()
{
  if (!p_table) return;
  load();
  fillTable();
  fillPlot();
}


void EnsemblePropertyPanel::OnRowSelected(wxListEvent& event)
{
  if (p_inSelect) return;
  selectRank((int)event.GetData());
}


void EnsemblePropertyPanel::OnColumn(wxListEvent& event)
{
  const int col = event.GetColumn();
  p_sortUp = col == p_sortCol ? !p_sortUp : true;
  p_sortCol = col;
  std::stable_sort(p_members.begin(), p_members.end(),
      [col, this](const Member& a, const Member& b) {
        int c = 0;
        if (col == 0) c = a.rank - b.rank;
        else if (col == 1) c = a.de < b.de ? -1 : a.de > b.de;
        else if (col == 2) c = a.pop < b.pop ? -1 : a.pop > b.pop;
        else c = a.pattern.compare(b.pattern);
        return p_sortUp ? c < 0 : c > 0;
      });
  fillTable();
}


bool EnsemblePropertyPanel::selectRank(int rank)
{
  PropertyTask *task = dynamic_cast<PropertyTask*>(getCalculation());
  string why;
  const int frame = ScanPoint::frame(task, "c" + std::to_string(rank), why);
  if (frame < 0) return false;
  p_selRank = rank;
  p_inSelect = true;
  for (long i = 0; i < p_table->GetItemCount(); i++)
    p_table->SetItemState(i, (int)p_table->GetItemData(i) == rank ?
        wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
  p_inSelect = false;
  processStep(frame);
  if (p_plotCtrl) p_plotCtrl->setSelectedPoint(0, rank - 1);
  return true;
}


// The base maps a frame to a plot point; this plot is in rank order.
void EnsemblePropertyPanel::processStep(int step)
{
  GeomTracePropertyPanel::processStep(step);
  if (p_plotCtrl && p_selRank > 0) p_plotCtrl->setSelectedPoint(0, p_selRank - 1);
}


void EnsemblePropertyPanel::receiveFocus()
{
  if (p_selRank > 0) selectRank(p_selRank);
}


void EnsemblePropertyPanel::loseFocus() {}


vector<string> EnsemblePropertyPanel::tableRows() const
{
  vector<string> rows;
  for (size_t i = 0; i < p_members.size(); i++) {
    const Member& m = p_members[i];
    char buf[96];
    snprintf(buf, sizeof buf, "%d | %.3f | %.1f", m.rank, m.de, m.pop);
    rows.push_back(string(buf) + (p_hasPatterns ? " | " + m.pattern : string()));
  }
  return rows;
}


bool EnsemblePropertyPanel::exportCsv(const std::string& path) const
{
  FILE *out = fopen(path.c_str(), "wb");
  if (!out) return false;
  fprintf(out, "rank,energy_kcal_per_mol,population_percent%s\n",
          p_hasPatterns ? ",pattern" : "");
  for (size_t i = 0; i < p_members.size(); i++) {
    const Member& m = p_members[i];
    fprintf(out, "%d,%.3f,%.2f", m.rank, m.de, m.pop);
    if (p_hasPatterns) fprintf(out, ",\"%s\"", m.pattern.c_str());
    fprintf(out, "\n");
  }
  return fclose(out) == 0;
}


void EnsemblePropertyPanel::OnExport(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Export ensemble table", wxEmptyString,
                   "ensemble.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(std::string(dlg.GetPath().utf8_str())))
    wxMessageBox("The file could not be written.", "Export ensemble table",
                 wxOK | wxICON_ERROR, this);
}
