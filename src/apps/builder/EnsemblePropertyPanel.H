#ifndef ENSEMBLEPROPERTYPANEL_H_
#define ENSEMBLEPROPERTYPANEL_H_

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include "inv/SbLinear.H"

#include "GeomTracePropertyPanel.H"

class wxButton;
class wxListEvent;
class wxImageList;
class SoSeparator;

// The members of a conformer, protonation-site, tautomer, cluster or pose
// ensemble ranked by energy: table, population, energy-against-rank plot.
// One panel class, five Properties entries (PropertyPanelDescriptor.xml);
// the entry's name is the run type's noun and isRelevant() lets only the
// matching one through.
class EnsemblePropertyPanel : public GeomTracePropertyPanel
{
  DECLARE_DYNAMIC_CLASS(EnsemblePropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    EnsemblePropertyPanel();
    EnsemblePropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="EnsemblePropertyPanel");
    virtual ~EnsemblePropertyPanel();

    virtual bool isRelevant(IPropCalculation *propCalc) const;
    virtual wxWindow * GetTearableContent();
    virtual void refresh();
    virtual void saveSettings() {}
    virtual void restoreSettings() {}

    // The Properties entry name the calculation's ensemble belongs under.
    static std::string kindOf(IPropCalculation *calc);
    static std::string kindFor(const std::string& runtype, bool scored);

    // "rank | dE | population % | pattern" per row, in the order shown.
    std::vector<std::string> tableRows() const;
    // Show member `rank` (1 = lowest) in the table, plot and 3-D view.
    bool selectRank(int rank);
    bool selectRanks(const std::vector<int>& ranks);
    // Selection order, with heavy-atom RMSD in Angstrom and HTML colour.
    std::vector<std::string> superpositionRows() const;
    bool superpositionVisible() const { return p_overlay; }
    bool exportPng(const std::string& path);
    std::vector<int> thumbnailRanks() const;
    void sortTable(int column, bool ascending);
    int selectedRank() const { return p_selRank; }
    // Ranks of the selected rows, ascending.
    std::vector<int> selectedRanks() const;
    // The codes offered for a new calculation (WorkflowSteps codes with a
    // Geometry run type) as "label|resource type" and the default's index.
    static std::vector<std::string> codeChoices(int& dflt);
    // "New calculation from selected" (#708): per rank a Geometry
    // Optimization calculation of code `typeName` (a resource type such as
    // orca_es) with that structure and this calculation's charge and
    // multiplicity: a linked workflow step if this is a workflow step, else a
    // calculation beside it.  The URLs created.
    std::vector<std::string> newCalcsFromRanks(const std::vector<int>& ranks,
                                               const std::string& typeName,
                                               std::string& why);
    bool exportCsv(const std::string& path) const;
    std::string kind() const;
    const std::string& energyLabel() const { return p_deName; }
    // A Vina calculation (code or run type): its score is empirical, so the
    // panel shows no Boltzmann population for it.
    static bool isVina(IPropCalculation *calc);
    bool hasPopulation() const { return p_population; }
    // Docking poses: the receptor's residues within this distance of the
    // pose's ligand are drawn as sticks, the rest of the receptor hidden.
    static constexpr double pocketRadius = 6.0;
    void setPocketView(bool on);
    bool pocketView() const { return p_pocket; }
    // "ligand=<n> pocket=<n> hidden=<n>" for the pose shown, for tests.
    std::string pocketInfo() const;

  protected:
    virtual void initialize();
    virtual void processStep(int step);
    virtual void receiveFocus();
    virtual void loseFocus();

    struct Member { int rank; double de; double pop; std::string pattern; };

    std::vector<int> ligandAtoms();
    void showPocket();

    void load();
    void fillTable();
    void fillPlot();
    void OnRowSelected(wxListEvent& event);
    void OnColumn(wxListEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnNewCalcs(wxCommandEvent& event);
    void updateSelection();
    void clearOverlay();
    struct Geometry {
      std::vector<double> xyz;
      std::vector<int> elements;
      std::vector<std::pair<int,int> > bonds;
    };
    const Geometry& geometry(int rank);
    SoSeparator *molecule(const Geometry& g, const std::vector<double>& xyz,
                           const wxColour *colour = NULL);
    bool makeThumbnail(int rank);
    void thumbnailBatch();

    std::vector<Member> p_members;   // as shown (sorted by the user's column)
    std::string p_deName;            // energy column header
    bool p_hasPatterns;
    bool p_isSites = false;         // MLP site screen: eV, environment column
    bool p_inSelect;
    int p_selRank;
    int p_sortCol;
    bool p_sortUp;
    wxListCtrl *p_table;
    wxButton *p_export;
    wxButton *p_newCalcs;
    std::vector<int> p_selection;
    std::vector<std::string> p_fits;
    std::map<int, Geometry> p_geometry;
    std::map<int, wxBitmap> p_thumbnails;
    bool p_population = true;
    bool p_pocket = true;
    int p_pocketCounts[3] = {0, 0, 0};
    int p_pocketBonds[2] = {0, 0};  // bonds drawn in the Ligand/Pocket styles, for the hook
    bool p_pocketFitted = false;   // camera framed since the view was switched on
    float p_pocketFit = 0;         // diagonal of the framed box, for the hook
    // What the pocket view last drew: stepping a pose moves the ligand but
    // leaves the same atoms shown, and restyling all of them again cost most
    // of a second per step on a protein (#966).
    vector<char> p_pocketRole;
    vector<char> p_pocketBondRole;
    const void *p_pocketFragment = 0;
    void fitPocketCamera(const std::vector<const double*>& atoms);
    std::map<int, int> p_imageIndex;
    wxImageList *p_images = NULL;
    wxListCtrl *p_key = NULL;
    wxTimer p_thumbTimer, p_selectTimer;
    SbRotation p_thumbOrientation;
    bool p_haveOrientation = false;
    float p_thumbBg[3] = {-1.f, -1.f, -1.f};   // viewer background of the thumbnails
    bool p_overlay = false;
    bool p_updatingSelection = false;
    int p_savedCS = 0;
};

#endif
