#include <algorithm>
#include <functional>
#include <cmath>
#include <cstdio>
#include <memory>

#include <wx/button.h>
#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/toolbar.h>
#include <wx/dcmemory.h>
#include <wx/imaglist.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <set>
#include <sstream>

#include "inv/SoOffscreenRenderer.H"
#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoLightModel.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoCamera.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "wxviz/SGViewer.H"
#include "viz/SGContainer.H"
#include "tdat/AtomMap.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"
#include "tdat/PropTSVecTable.H"
#include "wxgui/PlotStyle.H"
#include "wxviz/WxVizToolFW.H"

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/ChemistryTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "tdat/Fragment.H"
#include "tdat/TRunType.H"
#include "tdat/GUIValues.H"
#include "tdat/DisplayDescriptor.H"
#include "tdat/DisplayStyle.H"
#include "dsm/TaskJob.H"
#include "dsm/JCode.H"
#include "viz/CSStyleCmd.H"
#include "viz/DisplayAtomsCmd.H"
#include "viz/SGFragment.H"
#include "WorkflowSteps.H"
#include "dsm/IPropCalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/ScanPoint.H"
#include "wxgui/ewxStaticText.H"

#include "EnsemblePropertyPanel.H"
#include "PropertyPanel.H"
#include "PropertyPanelFactory.H"
#include "XYPlotCanvas.H"
#include "util/EcceURL.H"

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

// Short tag of the member in a calculation name: crest_conf3, crest_site2.
std::string tag(const std::string& kind)
{
  if (kind == "Protonation Sites") return "site";
  if (kind == "Tautomers") return "taut";
  if (kind == "Clusters") return "cluster";
  if (kind == "Poses") return "pose";
  return "conf";
}

}  // namespace


EnsemblePropertyPanel::EnsemblePropertyPanel()
  : GeomTracePropertyPanel(), p_hasPatterns(false), p_inSelect(false),
    p_selRank(0), p_sortCol(0), p_sortUp(true), p_table(NULL), p_export(NULL), p_newCalcs(NULL)
{
}


EnsemblePropertyPanel::EnsemblePropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : GeomTracePropertyPanel(), p_hasPatterns(false), p_inSelect(false),
    p_selRank(0), p_sortCol(0), p_sortUp(true), p_table(NULL), p_export(NULL), p_newCalcs(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


EnsemblePropertyPanel::~EnsemblePropertyPanel()
{
  p_thumbTimer.Stop();
  p_selectTimer.Stop();
  if (p_overlay) clearOverlay();
}


// Run type first; an ensemble from a run type not listed is Poses if it is
// scored (POSESCORE), else Conformers.
std::string EnsemblePropertyPanel::kindFor(const std::string& rt, bool scored)
{
  if (rt == "Protonation Sites") return "Protonation Sites";
  if (rt == "Tautomers") return "Tautomers";
  if (rt == "Solvation Shell") return "Clusters";
  if (rt == "Docking (aISS)" || rt == "Docking (Vina)") return "Poses";
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
  const bool vina = isVina(getCalculation());
  p_population = !vina;
  // Vina's score is an empirical estimate, not an interaction energy.
  p_deName = vina ? "Vina score (kcal mol⁻¹)"
           : ScanPoint::ensembleIsPoses(task)
                 ? "Interaction energy (kcal mol⁻¹)" : "ΔE (kcal mol⁻¹)";

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

  GetSizer()->Add(new wxStaticText(this, wxID_ANY,
      "Compare structures and choose members for further calculations."),
      0, wxEXPAND | wxALL, 5);

  p_table = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 220),
                           wxLC_REPORT);
  p_images = new wxImageList(72, 56, false);
  p_table->AssignImageList(p_images, wxIMAGE_LIST_SMALL);
  p_table->AppendColumn("Structure", wxLIST_FORMAT_LEFT, 86);
  p_table->AppendColumn("Rank", wxLIST_FORMAT_RIGHT, 60);
  const wxString energyLabel = wxString::FromUTF8(p_deName.c_str());
  p_table->AppendColumn(energyLabel, wxLIST_FORMAT_RIGHT,
      std::max(150, p_table->GetTextExtent(energyLabel).x + 24));
  if (p_population)
    p_table->AppendColumn("Population (%)", wxLIST_FORMAT_RIGHT, 110);
  if (p_hasPatterns)
    p_table->AppendColumn("Coordination pattern", wxLIST_FORMAT_LEFT, 240);
  p_table->SetToolTip(p_population
      ? "One row per member, lowest energy first; click a column "
        "heading to sort and a row to show that structure; Ctrl/Shift selects "
        "several members to superpose. Population is the "
        "Boltzmann weight at 298.15 K of the energy shown, within this method "
        "only: it is not a measured or cross-method population."
      : "One row per pose, best score first; click a column heading to sort "
        "and a row to show that pose; Ctrl/Shift selects several poses to "
        "superpose. The Vina score is an empirical estimate of the binding "
        "affinity, not an energy, so no Boltzmann population is given.");
  p_table->Bind(wxEVT_LIST_ITEM_SELECTED, &EnsemblePropertyPanel::OnRowSelected, this);
  p_table->Bind(wxEVT_LIST_ITEM_DESELECTED, &EnsemblePropertyPanel::OnRowSelected, this);
  p_table->Bind(wxEVT_LIST_COL_CLICK, &EnsemblePropertyPanel::OnColumn, this);
  GetSizer()->Add(p_table, 1, wxEXPAND|wxALL, 2);

  p_key = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 90), wxLC_REPORT);
  p_key->AppendColumn("Member", wxLIST_FORMAT_RIGHT, 75);
  p_key->AppendColumn("Colour", wxLIST_FORMAT_LEFT, 85);
  p_key->AppendColumn(wxString::FromUTF8("RMSD (Å)"), wxLIST_FORMAT_RIGHT, 100);
  p_key->AssignImageList(new wxImageList(12, 12, false), wxIMAGE_LIST_SMALL);
  p_key->SetToolTip("Heavy atoms, matching atom indices; RMSD to the first selected member.");
  GetSizer()->Add(p_key, 0, wxEXPAND | wxALL, 5);

  p_plotCtrl = new XYPlotCanvas(this);
  p_plotCtrl->setPointHandler([this](int, int index) { selectRank(index + 1); });
  // A floor: in a short pane the tables above would otherwise squeeze the
  // plot until its axis labels are cut off.
  p_plotCtrl->SetMinSize(wxSize(-1, 220));
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  p_export = new wxButton(this, wxID_ANY, "Export .csv...");
  p_export->SetToolTip("Save the table (rank, energy, population, pattern) as "
                       "a CSV file");
  p_export->Bind(wxEVT_BUTTON, &EnsemblePropertyPanel::OnExport, this);
  p_newCalcs = new wxButton(this, wxID_ANY, "New Calculation from Selected...");
  p_newCalcs->SetToolTip("Create one new calculation for each selected row, "
      "named <calculation>_<kind><rank>, in the same folder, with the code you "
      "choose, a Geometry Optimization run type, this calculation's charge and "
      "multiplicity and the structure of that row. Select several rows with Ctrl or Shift.");
  p_newCalcs->Bind(wxEVT_BUTTON, &EnsemblePropertyPanel::OnNewCalcs, this);
  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  buttons->Add(p_export, 0, wxALL, 4);
  buttons->Add(p_newCalcs, 0, wxALL, 4);
  wxButton *png = new wxButton(this, wxID_ANY, "Export .png...");
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export table with thumbnails", "", "ensemble.png",
        "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !exportPng(std::string(dlg.GetPath().utf8_str())))
      wxMessageBox("The table image could not be written.", "Export ensemble table",
                    wxOK | wxICON_ERROR, this);
  });
  buttons->Add(png, 0, wxALL, 4);
  if (kind() == "Poses") {
    wxCheckBox *pocket = new wxCheckBox(this, wxID_ANY, "Pocket view");
    pocket->SetValue(p_pocket);
    pocket->SetToolTip(wxString::FromUTF8("Show the selected pose with the "
        "receptor's residues within 6 Å of its ligand as sticks and hide the "
        "rest of the receptor; untick to show the whole receptor."));
    pocket->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) { setPocketView(e.IsChecked()); });
    buttons->Add(pocket, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
  }
  GetSizer()->Add(buttons, 0);

  p_selectTimer.SetOwner(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { updateSelection(); }, p_selectTimer.GetId());
  p_thumbTimer.SetOwner(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { thumbnailBatch(); }, p_thumbTimer.GetId());
  p_thumbTimer.Start(20);

  fillTable();
  fillPlot();
  addViewerToggle();
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


void EnsemblePropertyPanel::fillTable()
{
  p_inSelect = true;
  p_table->DeleteAllItems();
  for (size_t i = 0; i < p_members.size(); i++) {
    const Member& m = p_members[i];
    const auto image = p_imageIndex.find(m.rank);
    long row = p_table->InsertItem((long)i, "", image == p_imageIndex.end() ? -1 : image->second);
    p_table->SetItem(row, 1, wxString::Format("%d", m.rank));
    p_table->SetItem(row, 2, wxString::Format("%.3f", m.de));
    if (p_population)
      p_table->SetItem(row, 3, wxString::Format("%.1f", m.pop));
    if (p_hasPatterns)
      p_table->SetItem(row, p_population ? 4 : 3, wxString::FromUTF8(m.pattern.c_str()));
    p_table->SetItemData(row, m.rank);
    if (std::find(p_selection.begin(), p_selection.end(), m.rank) != p_selection.end())
      p_table->SetItemState(row, wxLIST_STATE_SELECTED,
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
    p_plotCtrl->setPointLabel(curve, (int)i, p_population
        ? wxString::Format("%s %d: %.3f kcal/mol, %.1f %%",
              wxString::FromUTF8(noun(kind()).c_str()), byRank[i].rank,
              byRank[i].de, byRank[i].pop)
        : wxString::Format("%s %d: %.3f kcal/mol",
              wxString::FromUTF8(noun(kind()).c_str()), byRank[i].rank,
              byRank[i].de));
  p_plotCtrl->setXAxisLabel(wxString::FromUTF8((noun(kind()) + " (lowest first)").c_str()));
  p_plotCtrl->setYAxisLabel(wxString::FromUTF8(p_deName.c_str()));
  if (p_selRank > 0) p_plotCtrl->setSelectedPoint(0, p_selRank - 1);
}


void EnsemblePropertyPanel::refresh()
{
  if (!p_table) return;
  clearOverlay();
  p_geometry.clear();
  p_thumbnails.clear();
  p_imageIndex.clear();
  p_images->RemoveAll();
  load();
  p_selection.erase(std::remove_if(p_selection.begin(), p_selection.end(),
      [this](int k) { return k > (int)p_members.size(); }), p_selection.end());
  fillTable();
  fillPlot();
  p_thumbTimer.Start(20);
  if (hasFocus()) updateSelection();
}


void EnsemblePropertyPanel::OnRowSelected(wxListEvent& event)
{
  if (p_inSelect) return;
  const int rank = (int)event.GetData();
  p_selection.erase(std::remove(p_selection.begin(), p_selection.end(), rank), p_selection.end());
  if (event.GetEventType() == wxEVT_LIST_ITEM_SELECTED) p_selection.push_back(rank);
  // A Ctrl/Shift gesture sends several events; fit once after the gesture.
  p_selectTimer.StartOnce(30);
}


void EnsemblePropertyPanel::OnColumn(wxListEvent& event)
{
  int col = event.GetColumn() - 1;
  if (col < 0) return;
  if (!p_population && col >= 2) col++;   // no population column
  p_sortUp = col == p_sortCol ? !p_sortUp : true;
  p_sortCol = col;
  sortTable(col, p_sortUp);
}

void EnsemblePropertyPanel::sortTable(int col, bool ascending)
{
  p_sortCol = col;
  p_sortUp = ascending;
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
  return selectRanks(vector<int>(1, rank));
}

bool EnsemblePropertyPanel::selectRanks(const vector<int>& ranks)
{
  for (int rank : ranks)
    if (rank < 1 || rank > (int)p_members.size()) return false;
  p_selectTimer.Stop();
  p_selection.clear();
  for (int rank : ranks)
    if (std::find(p_selection.begin(), p_selection.end(), rank) == p_selection.end())
      p_selection.push_back(rank);
  p_inSelect = true;
  for (long i = 0; i < p_table->GetItemCount(); i++)
    p_table->SetItemState(i, std::find(p_selection.begin(), p_selection.end(),
        (int)p_table->GetItemData(i)) != p_selection.end() ?
        wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
  p_inSelect = false;
  updateSelection();
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
  updateSelection();
}


void EnsemblePropertyPanel::loseFocus() { p_selectTimer.Stop(); clearOverlay(); }


vector<string> EnsemblePropertyPanel::tableRows() const
{
  vector<string> rows;
  for (size_t i = 0; i < p_members.size(); i++) {
    const Member& m = p_members[i];
    char buf[96];
    if (p_population)
      snprintf(buf, sizeof buf, "%d | %.3f | %.1f", m.rank, m.de, m.pop);
    else
      snprintf(buf, sizeof buf, "%d | %.3f", m.rank, m.de);
    rows.push_back(string(buf) + (p_hasPatterns ? " | " + m.pattern : string()));
  }
  return rows;
}


bool EnsemblePropertyPanel::exportCsv(const std::string& path) const
{
  FILE *out = fopen(path.c_str(), "wb");
  if (!out) return false;
  fprintf(out, "rank,%s%s%s\n",
          p_population ? "energy_kcal_per_mol,population_percent"
                       : "vina_score_kcal_per_mol", p_hasPatterns ? ",pattern" : "", p_fits.empty() ? "" :
          ",reference_rank,heavy_atom_rmsd_angstrom,colour");
  for (size_t i = 0; i < p_members.size(); i++) {
    const Member& m = p_members[i];
    if (p_population) fprintf(out, "%d,%.3f,%.2f", m.rank, m.de, m.pop);
    else fprintf(out, "%d,%.3f", m.rank, m.de);
    if (p_hasPatterns) fprintf(out, ",\"%s\"", m.pattern.c_str());
    if (!p_fits.empty()) {
      bool selected = false;
      for (const string& row : p_fits) {
        int rank;
        double rmsd;
        char colour[16];
        if (sscanf(row.c_str(), "%d | %lf | %15s", &rank, &rmsd, colour) == 3 && rank == m.rank) {
          fprintf(out, ",%d,%.8f,%s", p_selection.front(), rmsd, colour);
          selected = true;
          break;
        }
      }
      if (!selected) fprintf(out, ",,,");
    }
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


vector<int> EnsemblePropertyPanel::selectedRanks() const
{
  vector<int> ranks;
  for (long i = p_table ? p_table->GetNextItem(-1, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED) : -1;
       i >= 0; i = p_table->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
    ranks.push_back((int)p_table->GetItemData(i));
  std::sort(ranks.begin(), ranks.end());
  return ranks;
}


vector<string> EnsemblePropertyPanel::codeChoices(int& dflt)
{
  vector<string> out;
  dflt = 0;
  ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
  vector<ResourceType*> types = rd.getResourceTypes();
  const vector<WorkflowSteps::Code>& codes = WorkflowSteps::codes();
  for (size_t i = 0; i < codes.size(); i++) {
    // CREST and Vina have no Geometry run type; a conformer or pose is for a
    // further calculation.
    if (string(codes[i].typeName) == "crest_es" ||
        string(codes[i].typeName) == "vina_es") continue;
    bool installed = false;
    for (size_t t = 0; t < types.size(); t++)
      if (types[t]->getName() == codes[i].typeName) installed = true;
    if (!installed) continue;
    if (string(codes[i].label) == "ORCA") dflt = (int)out.size();
    out.push_back(string(codes[i].label) + "|" + codes[i].typeName);
  }
  return out;
}


// A workflow step goes through WorkflowSteps (link geometry.cK, annotation);
// otherwise the calculation is made beside this one.  The structure is the
// frame of GEOMTRACE for the rank, found with ScanPoint so a sorted table and
// the energy order cannot disagree.
vector<string> EnsemblePropertyPanel::newCalcsFromRanks(const vector<int>& ranks,
                                                        const string& typeName,
                                                        string& why)
{
  vector<string> urls;
  PropertyTask *task = dynamic_cast<PropertyTask*>(getCalculation());
  ICalculation *from = dynamic_cast<ICalculation*>(getCalculation());
  ResourceType *rt = 0;
  vector<ResourceType*> types =
      ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (size_t t = 0; t < types.size() && !rt; t++)
    if (types[t]->getName() == typeName) rt = types[t];
  if (!task || !from || !rt || ranks.empty()) {
    why = "No such code or calculation.";
    return urls;
  }
  const EcceURL here = task->getURL();
  Resource *parent = EDSIFactory::getResource(here.getParent());
  Session *wf = dynamic_cast<Session*>(parent);
  vector<Resource*> made;
  if (wf) {
    made = WorkflowSteps::createRanks(wf, rt, here.getFilePathTail(), task,
                                      ranks, WorkflowSteps::Carry(), why);
  } else if (parent) {
    for (size_t i = 0; i < ranks.size(); i++) {
      string err;
      Fragment *frag = ScanPoint::fragment(task, "c" + std::to_string(ranks[i]), err);
      Resource *r = frag ? parent->createChild(here.getFilePathTail() + "_" +
          tag(kind()) + std::to_string(ranks[i]), rt) : 0;
      ChemistryTask *ct = dynamic_cast<ChemistryTask*>(r);
      ICalculation *to = dynamic_cast<ICalculation*>(r);
      if (!ct || !to) {
        why = "Rank " + std::to_string(ranks[i]) + ": " +
              (err.empty() ? "the calculation could not be created." : err);
        delete frag;
        break;
      }
      ct->fragment(frag);
      delete frag;
      to->spinMultiplicity(from->spinMultiplicity());
      to->openShells(from->openShells());
      made.push_back(r);
    }
  }
  for (size_t i = 0; i < made.size(); i++) {
    ICalculation *to = dynamic_cast<ICalculation*>(made[i]);
    if (to) {
      TRunType rtype;
      rtype.name("Geometry");
      to->runtype(rtype);
    }
    EDSIFactory::notifyCreate(made[i]->getURL().toString(), true);
    urls.push_back(made[i]->getURL().toString());
  }
  return urls;
}


void EnsemblePropertyPanel::OnNewCalcs(wxCommandEvent&)
{
  const vector<int> ranks = selectedRanks();
  if (ranks.empty()) {
    wxMessageBox("Select one or more rows first (Ctrl or Shift for several).",
                 "New calculation", wxOK | wxICON_INFORMATION, this);
    return;
  }
  int dflt = 0;
  const vector<string> choices = codeChoices(dflt);
  if (choices.empty()) {
    wxMessageBox("No code with a Geometry run type is installed.",
                 "New calculation", wxOK | wxICON_WARNING, this);
    return;
  }
  wxArrayString labels;
  for (size_t i = 0; i < choices.size(); i++)
    labels.Add(wxString::FromUTF8(choices[i].substr(0, choices[i].find('|')).c_str()));
  wxSingleChoiceDialog dlg(this, "Code for the new calculations (Geometry "
      "Optimization run type):", "New calculation", labels);
  dlg.SetSelection(dflt);
  if (dlg.ShowModal() != wxID_OK) return;
  const string typeName = choices[dlg.GetSelection()].substr(
      choices[dlg.GetSelection()].find('|') + 1);
  string why;
  const vector<string> urls = newCalcsFromRanks(ranks, typeName, why);
  wxString text = wxString::Format("Created %d of %d calculation(s) in %s.",
      (int)urls.size(), (int)ranks.size(),
      urls.empty() ? wxString() : wxString::FromUTF8(
          EcceURL(urls[0]).getParent().getFilePathTail().c_str()));
  if (!why.empty()) text += "\n" + wxString::FromUTF8(why.c_str());
  else text += "\nThey are in the Organizer, ready to edit and launch.";
  wxMessageBox(text, "New calculation", wxOK |
               (why.empty() ? wxICON_INFORMATION : wxICON_WARNING), this);
}

namespace {
wxColour memberColour(size_t i)
{
  const PlotPalette p = PlotPalette::system();
  // Keep the shared series hues; additional cycles differ in lightness.
  return i < 6 ? p.series(i) : PlotPalette::mix(p.series(i), p.text,
                                               std::min(0.7, 0.18 * (i / 6)));
}
const int thumbWidth = 72, thumbHeight = 56;
}

const EnsemblePropertyPanel::Geometry& EnsemblePropertyPanel::geometry(int rank)
{
  auto found = p_geometry.find(rank);
  if (found != p_geometry.end()) return found->second;
  Geometry g;
  string why;
  PropertyTask *task = dynamic_cast<PropertyTask*>(getCalculation());
  const string spec = "c" + std::to_string(rank);
  const int frame = ScanPoint::frame(task, spec, why);
  PropTSVecTable *trace = dynamic_cast<PropTSVecTable*>(getCalculation()->getProperty("GEOMTRACE"));
  std::unique_ptr<Fragment> f;
  // Row labels carry the ensemble's own elements (including added protons).
  // Use cached property data so generating 200 pictures does not fetch the
  // calculation's input fragment from the data server 200 times.
  if (trace && frame >= 0 && trace->rowLabels() &&
      trace->rowLabels()->size() * 3 == trace->values(frame).size()) {
    TPerTab pt;
    f.reset(new Fragment);
    const vector<double>& xyz = trace->values(frame);
    const vector<string>& labels = *trace->rowLabels();
    for (size_t i = 0; i < labels.size(); i++) {
      const size_t dash = labels[i].find('-');
      const string el = dash == string::npos ? labels[i] : labels[i].substr(dash+1);
      if (!pt.isValid(el)) { f.reset(); break; }
      f->addAtom(el, &xyz[3*i]);
    }
  }
  if (!f) f.reset(ScanPoint::fragment(task, spec, why));
  if (f) {
    f->deleteAllBonds();
    f->addCovalentBonds();
    for (int i = 0; i < f->numAtoms(); i++) {
      const TAtm *a = f->atomRef(i);
      g.elements.push_back(a->atomicNumber());
      for (int k = 0; k < 3; k++) g.xyz.push_back(a->coordinates()[k]);
    }
    std::unique_ptr<vector<TBond*> > bonds(f->bonds());
    for (const TBond *b : *bonds)
      g.bonds.push_back(std::make_pair(b->atom1()->index(), b->atom2()->index()));
  }
  return p_geometry.emplace(rank, std::move(g)).first->second;
}

SoSeparator *EnsemblePropertyPanel::molecule(const Geometry& g,
    const vector<double>& xyz, const wxColour *colour)
{
  SoSeparator *root = new SoSeparator;
  SoComplexity *quality = new SoComplexity;
  quality->value = 0.2f;
  root->addChild(quality);
  SoLightModel *light = new SoLightModel;
  light->model = SoLightModel::BASE_COLOR;
  root->addChild(light);
  TPerTab pt;
  for (size_t i = 0; i < g.elements.size(); i++) {
    if (!g.elements[i]) continue;
    SoSeparator *atom = new SoSeparator;
    SoMaterial *mat = new SoMaterial;
    const wxColour c = colour ? *colour : wxColour(pt.colorName(g.elements[i]));
    mat->diffuseColor.setValue(c.Red()/255.f, c.Green()/255.f, c.Blue()/255.f);
    atom->addChild(mat);
    SoTranslation *at = new SoTranslation;
    at->translation.setValue(xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
    atom->addChild(at);
    SoSphere *sphere = new SoSphere;
    sphere->radius = g.elements[i] == 1 ? 0.12f : 0.23f;
    atom->addChild(sphere);
    root->addChild(atom);
  }
  SoMaterial *mat = new SoMaterial;
  const wxColour c = colour ? *colour : wxColour(110,110,110);
  mat->diffuseColor.setValue(c.Red()/255.f, c.Green()/255.f, c.Blue()/255.f);
  root->addChild(mat);
  SoDrawStyle *style = new SoDrawStyle;
  style->lineWidth = 2;
  root->addChild(style);
  SoCoordinate3 *points = new SoCoordinate3;
  SoLineSet *lines = new SoLineSet;
  int n = 0;
  for (const auto& b : g.bonds) {
    if (!g.elements[b.first] || !g.elements[b.second]) continue;
    for (int i : {b.first, b.second})
      points->point.set1Value(n++, xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
    lines->numVertices.set1Value(n/2 - 1, 2);
  }
  root->addChild(points);
  if (n) root->addChild(lines);
  else { lines->ref(); lines->unref(); }
  return root;
}

void EnsemblePropertyPanel::clearOverlay()
{
  if (!p_overlay) return;
  SGContainer& sg = getFW().getSceneGraph();
  SoSwitch *sw = sg.getPropertySwitch("EnsembleSuperposition");
  sw->whichChild = SO_SWITCH_NONE;
  sw->removeAllChildren();
  sg.getcsSwitch()->whichChild = p_savedCS;
  p_overlay = false;
  getFW().getViewer().refreshRenderArea();
}

void EnsemblePropertyPanel::updateSelection()
{
  // Stepping the base trace yields to wx; a second selection must wait
  // until the first has finished changing the scene graph.
  if (p_updatingSelection) { p_selectTimer.StartOnce(30); return; }
  struct Guard {
    bool& active;
    explicit Guard(bool& value) : active(value) { active = true; }
    ~Guard() { active = false; }
  } guard(p_updatingSelection);
  const vector<int> requested = p_selection;
  const bool wasFocused = hasFocus();
  clearOverlay();
  p_fits.clear();
  if (p_key) p_key->DeleteAllItems();
  if (p_key) p_key->GetImageList(wxIMAGE_LIST_SMALL)->RemoveAll();
  if (p_selection.empty()) return;
  p_selRank = p_selection.front();
  string why;
  int frame = ScanPoint::frame(dynamic_cast<PropertyTask*>(getCalculation()),
                             "c" + std::to_string(p_selRank), why);
  if (frame < 0) return;
  processStep(frame);
  showPocket();
  if (wasFocused && !hasFocus()) return;
  if (requested != p_selection) { p_selectTimer.StartOnce(30); return; }
  if (p_selection.size() < 2) return;

  const Geometry& ref = geometry(p_selRank);
  if (ref.xyz.empty()) return;
  vector<double> heavyRef;
  for (size_t i = 0; i < ref.elements.size(); i++)
    if (ref.elements[i] > 1)
      for (int k = 0; k < 3; k++) heavyRef.push_back(ref.xyz[3*i+k]);
  // All-hydrogen ensembles have no heavy atoms: fit all real atoms instead.
  const bool all = heavyRef.empty();
  if (all) heavyRef = ref.xyz;
  vector<int> identity(heavyRef.size()/3);
  for (size_t i = 0; i < identity.size(); i++) identity[i] = i;

  SGContainer& sg = getFW().getSceneGraph();
  SoSwitch *sw = sg.getPropertySwitch("EnsembleSuperposition");
  for (size_t m = 0; m < p_selection.size(); m++) {
    const int rank = p_selection[m];
    const Geometry& g = geometry(rank);
    if (g.elements != ref.elements) {
      clearOverlay();
      sw->removeAllChildren();
      p_fits.clear();
      p_key->DeleteAllItems();
      getFW().showMessage("Cannot align members with different atoms at matching indices.", true);
      return;
    }
    vector<double> heavy;
    for (size_t i = 0; i < g.elements.size(); i++)
      if (all || g.elements[i] > 1)
        for (int k = 0; k < 3; k++) heavy.push_back(g.xyz[3*i+k]);
    AtomAlignment fit;
    if (m != 0) fit = AtomMap::align(heavyRef, heavy, identity);
    vector<double> xyz = g.xyz;
    AtomMap::apply(fit, xyz);
    const wxColour c = memberColour(m);
    sw->addChild(molecule(g, xyz, &c));
    const wxString hex = c.GetAsString(wxC2S_HTML_SYNTAX);
    p_fits.push_back(std::string(wxString::Format("%d | %.8f | %s",
        rank, m == 0 ? 0.0 : fit.rmsdAfter, hex).utf8_str()));
    wxBitmap swatch(12, 12);
    wxMemoryDC dc(swatch);
    dc.SetBackground(wxBrush(c));
    dc.Clear();
    dc.SelectObject(wxNullBitmap);
    const int image = p_key->GetImageList(wxIMAGE_LIST_SMALL)->Add(swatch);
    long row = p_key->InsertItem(m, wxString::Format("%d", rank), image);
    p_key->SetItem(row, 1, hex);
    p_key->SetItem(row, 2, wxString::Format("%.4f", m == 0 ? 0.0 : fit.rmsdAfter));
  }
  p_savedCS = sg.getcsSwitch()->whichChild.getValue();
  sg.getcsSwitch()->whichChild = SO_SWITCH_NONE;
  sw->whichChild = SO_SWITCH_ALL;
  p_overlay = true;
  getFW().getViewer().refreshRenderArea();
}

vector<string> EnsemblePropertyPanel::superpositionRows() const { return p_fits; }

bool EnsemblePropertyPanel::makeThumbnail(int rank)
{
  if (p_thumbnails.count(rank)) return true;
  const Geometry& g = geometry(rank);
  if (g.xyz.empty()) return false;
  SoSeparator *root = new SoSeparator;
  root->ref();
  SoOrthographicCamera *cam = new SoOrthographicCamera;
  if (!p_haveOrientation) {
    if (getFW().getViewer().getCamera())
      p_thumbOrientation = getFW().getViewer().getCamera()->orientation.getValue();
    p_haveOrientation = true;
  }
  cam->orientation = p_thumbOrientation;
  root->addChild(cam);
  // In the pocket view a pose's thumbnail is its ligand: the whole receptor
  // fills a 72 px image with the same grey ball for every pose.
  Geometry ligandOnly;
  const Geometry *shown = &g;
  const int n = (int)g.elements.size();
  const vector<int> lig = (p_pocket && kind() == "Poses") ? ligandAtoms() : vector<int>();
  if (!lig.empty() && (int)lig.size() < n) {
    vector<char> keep(n, 0);
    for (int i : lig) if (i >= 0 && i < n) keep[i] = 1;
    ligandOnly = g;
    for (int i = 0; i < n; i++) if (!keep[i]) ligandOnly.elements[i] = 0;
    ligandOnly.bonds.clear();
    for (const auto& b : g.bonds)
      if (b.first >= 0 && b.first < n && b.second >= 0 && b.second < n &&
          keep[b.first] && keep[b.second])
        ligandOnly.bonds.push_back(b);
    shown = &ligandOnly;
  }
  root->addChild(molecule(*shown, g.xyz));
  SbViewportRegion vp;
  vp.setWindowSize(SbVec2s(thumbWidth, thumbHeight));
  cam->viewAll(root, vp, 1.1f);
  // Reuse the context, as the existing calculation thumbnail renderer does.
  static SoOffscreenRenderer *renderer = new SoOffscreenRenderer(vp);
  renderer->setComponents(SoOffscreenRenderer::RGB);
  const SbColor& bg = getFW().getViewer().getBackgroundColor();
  renderer->setBackgroundColor(bg);
  for (int k = 0; k < 3; k++) p_thumbBg[k] = bg[k];
  const bool ok = renderer->render(root) && renderer->getBuffer();
  if (ok) {
    wxImage image(thumbWidth, thumbHeight);
    const unsigned char *pixels = renderer->getBuffer();
    for (int y = 0; y < thumbHeight; y++)
      std::copy(pixels + 3*thumbWidth*(thumbHeight-1-y),
                pixels + 3*thumbWidth*(thumbHeight-y), image.GetData() + 3*thumbWidth*y);
    wxBitmap bitmap(image);
    p_thumbnails[rank] = bitmap;
    const int idx = p_images->Add(bitmap);
    p_imageIndex[rank] = idx;
    for (long row = 0; row < p_table->GetItemCount(); row++)
      if ((int)p_table->GetItemData(row) == rank) p_table->SetItemImage(row, idx);
  }
  root->unref();
  return ok;
}

// The timer keeps running slowly once all are made: the thumbnails are drawn
// on the viewer's background, and a later change of it would leave them on
// the old one (black thumbnails on a white viewer).
void EnsemblePropertyPanel::thumbnailBatch()
{
  if (!p_thumbnails.empty()) {
    const SbColor& bg = getFW().getViewer().getBackgroundColor();
    if (bg[0] != p_thumbBg[0] || bg[1] != p_thumbBg[1] || bg[2] != p_thumbBg[2]) {
      p_thumbnails.clear();
      p_imageIndex.clear();
      p_images->RemoveAll();
      for (long row = 0; row < p_table->GetItemCount(); row++)
        p_table->SetItemImage(row, -1);
    }
  }
  int made = 0;
  for (const Member& m : p_members) {
    if (p_thumbnails.count(m.rank)) continue;
    if (!makeThumbnail(m.rank)) { p_thumbTimer.Stop(); return; }
    if (++made == 2) { p_thumbTimer.Start(20); return; }
  }
  p_thumbTimer.Start(500);
}

vector<int> EnsemblePropertyPanel::thumbnailRanks() const
{
  vector<int> out;
  for (const auto& t : p_thumbnails) out.push_back(t.first);
  return out;
}

bool EnsemblePropertyPanel::exportPng(const string& path)
{
  for (const Member& m : p_members) if (!makeThumbnail(m.rank)) return false;
  const int rowHeight = 62;
  wxMemoryDC dc;
  dc.SetFont(p_table->GetFont());
  vector<int> widths = {86, 60, 160};
  if (p_population) widths.push_back(120);
  widths[2] = std::max(widths[2], dc.GetTextExtent(wxString::FromUTF8(p_deName.c_str())).x + 16);
  int patternWidth = 200;
  for (const Member& m : p_members)
    patternWidth = std::max(patternWidth, dc.GetTextExtent(wxString::FromUTF8(m.pattern.c_str())).x + 16);
  if (p_hasPatterns) widths.push_back(patternWidth);
  int width = 0;
  for (int w : widths) width += w;
  const int header = 32;
  wxBitmap bitmap(width, header + rowHeight * p_members.size());
  if (!bitmap.IsOk()) return false;
  dc.SelectObject(bitmap);
  dc.SetBackground(*wxWHITE_BRUSH);
  dc.Clear();
  dc.SetTextForeground(*wxBLACK);
  dc.SetPen(wxPen(wxColour(210,210,210)));
  vector<wxString> labels = {"Structure", "Rank", wxString::FromUTF8(p_deName.c_str())};
  if (p_population) labels.push_back("Population (%)");
  if (p_hasPatterns) labels.push_back("Coordination pattern");
  int x = 0;
  for (size_t i = 0; i < widths.size(); i++) {
    dc.DrawText(labels[i], x + 6, 7);
    x += widths[i];
  }
  int y = header;
  for (const Member& m : p_members) {
    dc.DrawLine(0, y, width, y);
    dc.DrawBitmap(p_thumbnails.at(m.rank), 5, y + 3);
    vector<wxString> text = {"", wxString::Format("%d", m.rank),
        wxString::Format("%.3f", m.de)};
    if (p_population) text.push_back(wxString::Format("%.1f", m.pop));
    text.push_back(wxString::FromUTF8(m.pattern.c_str()));
    x = widths[0];
    for (size_t i = 1; i < widths.size(); i++) {
      dc.DrawText(text[i], x + 6, y + (rowHeight - dc.GetCharHeight())/2);
      x += widths[i];
    }
    y += rowHeight;
  }
  dc.SelectObject(wxNullBitmap);
  return bitmap.SaveFile(wxString::FromUTF8(path.c_str()), wxBITMAP_TYPE_PNG);
}

bool EnsemblePropertyPanel::isVina(IPropCalculation *calc)
{
  // The code as well as the run type: a calculation stored without its run
  // type (an import, a script) is still a Vina one.
  TaskJob *job = dynamic_cast<TaskJob*>(calc);
  if (job && job->application() && job->application()->name() == "Vina")
    return true;
  ICalculation *ic = dynamic_cast<ICalculation*>(calc);
  return ic && ic->runtype().name() == "Docking (Vina)";
}

// The ligand as the docking split defines it (ES.Runtype.Docking.LigandAtoms,
// 1-based ranges); empty means the smallest molecule, as docking.pl takes it.
vector<int> EnsemblePropertyPanel::ligandAtoms()
{
  vector<int> out;
  ICalculation *ic = dynamic_cast<ICalculation*>(getCalculation());
  GUIValues *g = ic ? ic->guiparams() : 0;
  GUIValue *v = g ? g->get("ES.Runtype.Docking.LigandAtoms") : 0;
  string spec = v ? v->getValueAsString() : "";
  for (char& c : spec) if (c == ',') c = ' ';
  std::istringstream in(spec);
  string part;
  while (in >> part) {
    int a = 0, b = 0;
    if (sscanf(part.c_str(), "%d-%d", &a, &b) == 2) {
      for (int i = a; i <= b; i++) out.push_back(i - 1);
    } else if (sscanf(part.c_str(), "%d", &a) == 1) {
      out.push_back(a - 1);
    }
  }
  delete g;
  if (!out.empty() || p_members.empty()) return out;
  const Geometry& geo = geometry(p_members.front().rank);
  const int n = (int)geo.elements.size();
  vector<int> root(n);
  for (int i = 0; i < n; i++) root[i] = i;
  std::function<int(int)> find = [&](int i) { return root[i] == i ? i : root[i] = find(root[i]); };
  for (const auto& b : geo.bonds) root[find(b.first)] = find(b.second);
  std::map<int, vector<int> > parts;
  for (int i = 0; i < n; i++) parts[find(i)].push_back(i);
  if (parts.size() < 2) return out;
  for (const auto& p : parts)
    if (out.empty() || p.second.size() < out.size()) out = p.second;
  return out;
}

void EnsemblePropertyPanel::setPocketView(bool on)
{
  p_thumbBg[0] = -1.f;   // redraw the thumbnails: ligand only, or whole
  p_pocket = on;
  if (!on) {
    SGContainer& sg = getFW().getSceneGraph();
    Command *cmd = new DisplayAtomsCmd("DisplayAtoms", &sg);
    cmd->getParameter("all")->setBoolean(true);
    cmd->getParameter("show")->setBoolean(true);
    getFW().execute(cmd);
    for (int& c : p_pocketCounts) c = 0;
    p_pocketFit = 0;
    getFW().getViewer().viewAll();
    getFW().getViewer().refreshRenderArea();
  } else {
    p_pocketFitted = false;
    showPocket();
  }
}

std::string EnsemblePropertyPanel::pocketInfo() const
{
  char buf[128];
  snprintf(buf, sizeof buf, "ligand=%d pocket=%d hidden=%d fit=%.1f",
           p_pocketCounts[0], p_pocketCounts[1], p_pocketCounts[2], p_pocketFit);
  return buf;
}

// The receptor's whole residues with any atom within pocketRadius of the
// ligand as sticks, the ligand as balls and sticks, the rest hidden: a
// 2000-atom receptor drawn in full hides the pose it surrounds.
void EnsemblePropertyPanel::showPocket()
{
  for (int& c : p_pocketCounts) c = 0;
  if (!p_pocket || kind() != "Poses") return;
  SGContainer& sg = getFW().getSceneGraph();
  SGFragment *frag = sg.getFragment();
  const int n = frag ? frag->numAtoms() : 0;
  const vector<int> lig = ligandAtoms();
  if (lig.empty() || (int)lig.size() >= n) return;
  vector<char> role(n, 0);   // 1 ligand, 2 pocket, 0 hidden
  for (int i : lig) if (i >= 0 && i < n) role[i] = 1;
  const double r2 = pocketRadius * pocketRadius;
  std::set<const void*> residues;
  for (int a = 0; a < n; a++) {
    if (role[a] == 1) continue;
    const double *p = frag->atomRef(a)->coordinates();
    for (int l : lig) {
      if (l < 0 || l >= n) continue;
      const double *q = frag->atomRef(l)->coordinates();
      const double d = (p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) +
                       (p[2]-q[2])*(p[2]-q[2]);
      if (d < r2) { role[a] = 2; break; }
    }
    if (role[a] == 2 && frag->atomRef(a)->getResidue())
      residues.insert(frag->atomRef(a)->getResidue());
  }
  for (int a = 0; a < n; a++)
    if (!role[a] && frag->atomRef(a)->getResidue() &&
        residues.count(frag->atomRef(a)->getResidue()))
      role[a] = 2;

  auto selectRole = [&](int which) {
    frag->m_atomHighLight.clear();
    frag->m_bondHighLight.clear();
    for (int a = 0; a < n; a++)
      if (role[a] == which) frag->m_atomHighLight.push_back(a);
    p_pocketCounts[which == 1 ? 0 : which == 2 ? 1 : 2] = (int)frag->m_atomHighLight.size();
  };
  auto style = [&](DisplayStyle::Style st, const string& name) {
    const string s = DisplayStyle::fromStyle(st);
    DisplayDescriptor dd(name, s, "Element");
    dd.setColorScheme(dd.getDefaultColorScheme(s));
    Command *cmd = new CSStyleCmd("Style", (Receiver*)&sg);
    cmd->getParameter("descriptor")->setString(dd.toString());
    cmd->getParameter("all")->setBoolean(false);
    getFW().execute(cmd);
  };
  // Show everything first: the previous pose's pocket may differ.
  Command *show = new DisplayAtomsCmd("DisplayAtoms", &sg);
  show->getParameter("all")->setBoolean(true);
  show->getParameter("show")->setBoolean(true);
  getFW().execute(show);
  selectRole(2);
  style(DisplayStyle::STICK, "Pocket");
  selectRole(1);
  style(DisplayStyle::BALLSTICK, "Ligand");
  selectRole(0);
  if (!frag->m_atomHighLight.empty()) {
    Command *hide = new DisplayAtomsCmd("DisplayAtoms", &sg);
    hide->getParameter("selected")->setBoolean(true);
    getFW().execute(hide);
  }
  frag->m_atomHighLight.clear();
  frag->m_bondHighLight.clear();
  sg.fixAllNubs();
  // Frame once per switch-on, so stepping through poses keeps the user's zoom.
  if (!p_pocketFitted) {
    vector<const double*> shown;
    for (int a = 0; a < n; a++)
      if (role[a]) shown.push_back(frag->atomRef(a)->coordinates());
    fitPocketCamera(shown);
    p_pocketFitted = true;
  }
  getFW().getViewer().refreshRenderArea();
}

// viewAll() frames hidden atoms too, so the pocket would stay a speck in the
// middle of the whole receptor. Frame the shown atoms and the search box.
void EnsemblePropertyPanel::fitPocketCamera(const vector<const double*>& atoms)
{
  SGViewer& viewer = getFW().getViewer();
  SoCamera *cam = viewer.getCamera();
  if (!cam || atoms.empty()) return;
  SbBox3f bbox;
  for (const double *p : atoms) bbox.extendBy(SbVec3f((float)p[0], (float)p[1], (float)p[2]));
  SoSwitch *sw = getFW().getSceneGraph().getPropertySwitch("VinaSearchBox");
  if (sw && sw->whichChild.getValue() != SO_SWITCH_NONE) {
    SoGetBoundingBoxAction bba(viewer.getViewportRegion());
    bba.apply(sw);
    if (!bba.getBoundingBox().isEmpty()) bbox.extendBy(bba.getBoundingBox());
  }
  // Fit the box's corners in the camera's own frame: viewAll's bounding
  // sphere leaves a box drawn at a third of a tall, narrow viewer.
  const SbVec3f centre = bbox.getCenter();
  const SbRotation rot = cam->orientation.getValue();
  SbVec3f right, up, back;
  rot.multVec(SbVec3f(1, 0, 0), right);
  rot.multVec(SbVec3f(0, 1, 0), up);
  rot.multVec(SbVec3f(0, 0, 1), back);
  const SbVec3f lo = bbox.getMin(), hi = bbox.getMax();
  float hx = 0, hy = 0, hz = 0;
  for (int i = 0; i < 8; i++) {
    const SbVec3f d = SbVec3f((i & 1) ? hi[0] : lo[0], (i & 2) ? hi[1] : lo[1],
                              (i & 4) ? hi[2] : lo[2]) - centre;
    hx = std::max(hx, std::fabs(d.dot(right)));
    hy = std::max(hy, std::fabs(d.dot(up)));
    hz = std::max(hz, std::fabs(d.dot(back)));
  }
  const float aspect = viewer.getViewportRegion().getViewportAspectRatio();
  // ADJUST_CAMERA maps the height angle onto the shorter side of the viewport.
  const float half = 1.05f * (aspect < 1 ? std::max(hx, hy * aspect)
                                         : std::max(hy, hx / aspect));
  float dist = hz + 1.0f;
  if (cam->isOfType(SoPerspectiveCamera::getClassTypeId())) {
    const float angle = ((SoPerspectiveCamera*)cam)->heightAngle.getValue();
    dist = hz + half / std::tan(angle / 2);
  } else if (cam->isOfType(SoOrthographicCamera::getClassTypeId())) {
    ((SoOrthographicCamera*)cam)->height = 2 * half;
  }
  cam->position = centre + back * dist;
  cam->focalDistance = dist;
  cam->nearDistance = std::max(0.1f, dist - 2 * hz);
  cam->farDistance = dist + 2 * hz;
  float dx, dy, dz;
  bbox.getSize(dx, dy, dz);
  p_pocketFit = std::sqrt(dx * dx + dy * dy + dz * dz);
}
