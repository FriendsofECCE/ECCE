#ifndef GEOMTRACEPROPERTYPANEL_H_
#define GEOMTRACEPROPERTYPANEL_H_

#include <map>
  using std::map;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include <wx/listctrl.h>
#include <wx/collpane.h>
#include "wxgui/TearableContentProvider.H"

#include "VizPropertyPanel.H"

class wxCheckBox;
class PlaybackControl;
class XYPlotCanvas;
class wxChoice;
class wxCollapsiblePane;
class wxCollapsiblePaneEvent;
class wxStaticText;
class wxTextCtrl;
class wxButton;
class wxPanel;
class SbColor;


class GeomTracePropertyPanel : public VizPropertyPanel,
                               public TearableContentProvider
{
  DECLARE_DYNAMIC_CLASS(GeomTracePropertyPanel)
  DECLARE_EVENT_TABLE()

  static const wxWindowID ID_CHECKBOX_BONDS;
  static const wxWindowID ID_CHECKBOX_FOLD;
  static const wxWindowID ID_DELAY;
  static const wxWindowID ID_PLAYBACK;

  public:
    GeomTracePropertyPanel();
    GeomTracePropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="GeomTracePropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="GeomTracePropertyPanel");
    virtual ~GeomTracePropertyPanel();

    virtual wxWindow * GetTearableContent();

    virtual void refresh();

    //  A click on point `index` of a plot of `points` points: the nearest
    //  stored structure is shown.
    void showPlotPoint(int index, int points);
    int frameCount() const { return numSteps(); }
    //  Plot the series `key` (as the Plot choice does); false if unknown.
    bool showSeries(const string& key);
    //  The coordination-pattern table of a CREST ensemble, one string per row
    //  ("pattern | count | lowest kcal/mol | conformer"); empty if none shown.
    vector<string> patternRows() const;
    //  Displacement from the first frame (one calculation) or, after
    //  compareWith(), the aligned difference to a second one (#701).  The
    //  numbers are those of ecce-displacement; the rows are its CSV fields.
    vector<string> displacementRows();
    //  "aligned RMSD ..." for a comparison, else the trace summary.
    string displacementSummary() const { return p_dispSummary; }
    bool setDisplacementSelection(const string& spec);   // --select syntax
    bool compareWith(const string& name);                // "" ends it
    vector<string> comparableNames();
    void colourByDisplacement(bool on);
    bool exportDisplacement(const string& path) const;
    //  Atom colours now in the viewer, "binding r g b" per atom.
    vector<string> atomColourRows() const;
    virtual void saveSettings();
    virtual void restoreSettings();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

  protected:
    virtual void initialize();
    void buildPatternTable();     // CREST ensembles around an ion

    virtual void fillPlot();

    void OnToolClick(wxCommandEvent& event);
    void OnPropChoice(wxCommandEvent& event);
    void selectProp(const string& key);
    void OnPlaybackTick(wxCommandEvent& event);
    void OnPlaybackLoop(wxCommandEvent& event);
    void OnPointClick(int curve, int index);
    int frameOfPlotPoint(int index, int points) const;
    int plotPointOfFrame(int frame, int points) const;
    void OnValidation(wxCommandEvent& event);

    virtual void processStep(int step);

    virtual int numSteps() const;
    virtual string getTracePropKey() const;

    virtual void receiveFocus();
    virtual void loseFocus();

    void buildDisplacementPane();
    vector<IPropCalculation*> comparable(vector<string> *labels);
    bool runDisplacement();
    void showDisplacement();
    void paintDisplacement(bool on);
    void OnDispExpand(wxCollapsiblePaneEvent& event);
    void OnDispColour(wxCommandEvent& event);
    void OnDispCompare(wxCommandEvent& event);
    void OnDispSelect(wxCommandEvent& event);
    void OnDispExport(wxCommandEvent& event);

    wxCollapsiblePane *p_dispPane;
    wxListCtrl      *p_dispList;
    wxCheckBox      *p_dispColour;
    wxTextCtrl      *p_dispSelect;
    wxStaticText    *p_dispInfo;
    wxStaticText    *p_dispLegend;
    wxPanel         *p_dispBar;
    string p_dispCompare;                  // second calculation's name
    string p_dispSelection;
    string p_dispSummary;
    vector<string> p_dispCols;             // CSV header of the table
    vector<vector<string> > p_dispTable;   // CSV rows
    vector<float> p_dispSaved;             // atom colours before painting
    int p_dispSavedBinding;
    bool p_dispPainted;

    wxListCtrl      *p_patterns;     // CREST ensembles: pattern table
    XYPlotCanvas     *p_plotCtrl;
    PlaybackControl *p_playback;
    wxChoice        *p_propChoice;   // every logged series, above the plot

    int p_delay;
    wxCheckBox *p_loop;
    bool p_recompute;
    bool p_fold;

    string p_currentProp;
    bool p_propChosen;     // the user picked the series (so it is remembered)

    vector<string> p_importantProps;
    map<string,wxWindowID> p_keyToId;
    map<wxWindowID,string> p_idToKey;
};

#endif // GEOMTRACEPROPERTYPANEL_H_
