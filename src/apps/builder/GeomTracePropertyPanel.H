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

class PlaybackControl;
class wxPlotCtrl;


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
    virtual void saveSettings();
    virtual void restoreSettings();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

  protected:
    virtual void initialize();

    virtual void fillPlot();

    void OnToolClick(wxCommandEvent& event);
    void OnPlaybackTick(wxCommandEvent& event);
    void OnPlaybackLoop(wxCommandEvent& event);
    void OnPointClick(wxPlotCtrlEvent& event);
    void OnValidation(wxCommandEvent& event);

    virtual void processStep(int step);

    virtual int numSteps() const;
    virtual string getTracePropKey() const;

    virtual void receiveFocus();
    virtual void loseFocus();

    wxPlotCtrl      *p_plotCtrl;
    PlaybackControl *p_playback;

    int p_delay;
    bool p_recompute;
    bool p_fold;

    string p_currentProp;

    vector<string> p_importantProps;
    map<string,wxWindowID> p_keyToId;
    map<wxWindowID,string> p_idToKey;
};

#endif // GEOMTRACEPROPERTYPANEL_H_
