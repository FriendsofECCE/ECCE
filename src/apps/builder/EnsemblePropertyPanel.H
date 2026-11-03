#ifndef ENSEMBLEPROPERTYPANEL_H_
#define ENSEMBLEPROPERTYPANEL_H_

#include <string>
#include <vector>

#include "GeomTracePropertyPanel.H"

class wxButton;
class wxListEvent;

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
    int selectedRank() const { return p_selRank; }
    // Ranks of the selected rows, ascending.
    std::vector<int> selectedRanks() const;
    // "New calculation from selected" (#708): one calculation per rank in the
    // CREST calculation's folder, <calc>_<noun><rank>; the URLs created.
    std::vector<std::string> newCalcsFromRanks(const std::vector<int>& ranks,
                                               std::string& why);
    bool exportCsv(const std::string& path) const;
    std::string kind() const;

  protected:
    virtual void initialize();
    virtual void processStep(int step);
    virtual void receiveFocus();
    virtual void loseFocus();

    struct Member { int rank; double de; double pop; std::string pattern; };

    void load();
    void fillTable();
    void fillPlot();
    void OnRowSelected(wxListEvent& event);
    void OnColumn(wxListEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnNewCalcs(wxCommandEvent& event);

    std::vector<Member> p_members;   // as shown (sorted by the user's column)
    std::string p_deName;            // energy column header
    bool p_hasPatterns;
    bool p_inSelect;
    int p_selRank;
    int p_sortCol;
    bool p_sortUp;
    wxListCtrl *p_table;
    wxButton *p_export;
    wxButton *p_newCalcs;
};

#endif
