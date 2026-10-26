#ifndef GEOMTRACEPROPERTYPANEL_H_
#define GEOMTRACEPROPERTYPANEL_H_

#include <map>
  using std::map;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "wxgui/TearableContentProvider.H"

#include "VizPropertyPanel.H"

class wxCheckBox;
class PlaybackControl;
class XYPlotCanvas;
class wxChoice;


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
    virtual void saveSettings();
    virtual void restoreSettings();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

  protected:
    virtual void initialize();

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
