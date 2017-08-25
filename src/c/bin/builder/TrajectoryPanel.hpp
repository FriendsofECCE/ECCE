#ifndef TRAJECTORY_PANEL_H_
#define TRAJECTORY_PANEL_H_

#include <istream>

#include "VizPropertyPanel.H"

class MdTask;
class PlaybackControl;
class TaskJob;
class TrajectoryPanel;


class TrajectoryStreamProvider
{
  public:
    TrajectoryStreamProvider(IPropCalculation * calc);
    vector<string> getFileSequence();
    istream * getStream(const string& url);

  private:
    IPropCalculation * p_calc;
    TaskJob * p_job;
    MdTask * p_mdtask;
};



class TrajectoryPanelModel
{
  public:
    TrajectoryPanelModel();

    long fileIndexMin;
    long fileIndexMax;
    long fileIndexFrom;
    long fileIndexTo;
    long fileIndexCurrent;
    long stepIndexMin;
    long stepIndexMax;
    long stepCurrent;
    long stepSize;
    int delay;

    bool recomputeBonds;
    bool showSolvent;
    
    wxString units;

    wxString outputFormat;
    wxString outputFileSize;
    wxString povrayFinish;
    wxString povrayBonds;
    wxString povraySurfaces;
    int outputWidth;
    int outputHeight;
    wxString outputDirectory;
    wxString outputFilePrefix;

    bool showVelocities;
    double velocityScale;
    bool showForces;
    double forceScale;

    void restoreSettings();
    void saveSettings();
};



class TrajectoryOutputPanel : public ewxPanel
{
  public:
    static const wxWindowID ID_CHOICE_OUTPUT_FORMAT;

    static const wxWindowID ID_PANEL_POVRAY;
    static const wxWindowID ID_CHOICE_POVRAY_FINISH;
    static const wxWindowID ID_CHOICE_POVRAY_BONDS;
    static const wxWindowID ID_CHOICE_POVRAY_SURFACES;

    static const wxWindowID ID_PANEL_SIZE;
    static const wxWindowID ID_CHOICE_OUTPUT_SIZE;
    static const wxWindowID ID_TEXT_OUTPUT_WIDTH;
    static const wxWindowID ID_LABEL_OUTPUT_WIDTH;
    static const wxWindowID ID_TEXT_OUTPUT_HEIGHT;
    static const wxWindowID ID_LABEL_OUTPUT_HEIGHT;

    static const wxWindowID ID_TEXT_OUTPUT_DIRECTORY;
    static const wxWindowID ID_TEXT_OUTPUT_FILE_PREFIX;

    TrajectoryOutputPanel();
    TrajectoryOutputPanel(TrajectoryPanelModel * model,
            wxWindow * parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=0, const wxString& name="TrajectoryOutputPanel");
    virtual ~TrajectoryOutputPanel();

    bool Create(TrajectoryPanelModel * model,
            wxWindow * parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=0, const wxString& name="TrajectoryOutputPanel");

    
  protected:
    void CreateOutputOptions();

    void OnValidation(wxCommandEvent& event);
    
    void UpdateFormatChoice();
    void UpdateSizeChoice();

    TrajectoryPanelModel * p_model;
    
  private:
    DECLARE_DYNAMIC_CLASS(TrajectoryOutputPanel)
    DECLARE_EVENT_TABLE()
};



/**
 * Simply wraps the TrajectoryOutputPanel in a dialog.
 */
class TrajectoryOutputDialog : public ewxDialog
{
  public:
    TrajectoryOutputDialog();
    TrajectoryOutputDialog(TrajectoryPanelModel * model,
            wxWindow * parent, wxWindowID id=wxID_ANY,
            const wxString& title="Output Options",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxDEFAULT_DIALOG_STYLE,
            const wxString& name="TrajectoryOutputDialog");
    virtual ~TrajectoryOutputDialog();

    bool Create(TrajectoryPanelModel * model,
            wxWindow * parent, wxWindowID id=wxID_ANY,
            const wxString& title="Output Options",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxDEFAULT_DIALOG_STYLE,
            const wxString& name="TrajectoryOutputDialog");

  protected:
    void OnValidation(wxCommandEvent& event);

  private:
    DECLARE_DYNAMIC_CLASS(TrajectoryOutputDialog)
    DECLARE_EVENT_TABLE()
};



class TrajectoryPanel : public VizPropertyPanel
{
  public:
    static const wxWindowID ID_BUTTON_REFRESH;
    static const wxWindowID ID_TEXT_FILE_NAME;
    static const wxWindowID ID_PANEL_FILE_CURRENT;
    static const wxWindowID ID_TEXT_FILE_CURRENT;

    //static const wxWindowID ID_BUTTON_EXPORT_MEASURES;
    //static const wxWindowID ID_BUTTON_SELECT_ATOMS;

    static const wxWindowID ID_TEXT_STEP_CURRENT;
    static const wxWindowID ID_TEXT_STEP_SIZE;
    static const wxWindowID ID_TEXT_DELAY;
    static const wxWindowID ID_PLAYBACK;

    static const wxWindowID ID_CHECK_RECOMPUTE_BONDS;
    static const wxWindowID ID_CHECK_SHOW_SOLVENT;

    static const wxWindowID ID_CHECK_VELOCITIES;
    static const wxWindowID ID_SCALE_VELOCITIES;
    static const wxWindowID ID_CHECK_FORCES;
    static const wxWindowID ID_SCALE_FORCES;

    TrajectoryPanel();
    TrajectoryPanel(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="TrajectoryPanel");

    virtual bool Create(IPropCalculation *calculation,
            wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="TrajectoryPanel");

    virtual ~TrajectoryPanel();

    virtual void refresh();

  protected:
    virtual void receiveFocus();
    virtual void loseFocus();

    virtual void initialize();

    void ReadStep(const long& step);

    void OnPlaybackTick(wxCommandEvent& event);
    void OnPlaybackLoop(wxCommandEvent& event);
    void OnValidation(wxCommandEvent& event);
    void OnRefreshClick(wxCommandEvent& event);
    void doRefreshClick();
    void OnRecordClick(wxCommandEvent& event);

    void UpdatePlayback(wxUpdateUIEvent& event);
    void UpdateRefresh(wxUpdateUIEvent& event);
    void UpdateRecomputeBonds(wxUpdateUIEvent& event);
    void UpdateShowSolvent(wxUpdateUIEvent& event);
    void UpdateShowVelocities(wxUpdateUIEvent& event);
    void UpdateShowForces(wxUpdateUIEvent& event);

    static int ExtractSequenceNumber(const string& filename);

    void LoadFiles();
    void CaptureMeasures();
    void SaveIt();
    DisplayStyle QueryDisplayStyle(const int& numSoluteAtoms);
    void SetDisplayStyle(const DisplayStyle& style, const int& numSoluteAtoms);

  private:
    TrajectoryPanelModel p_model;

    TrajectoryReader * p_reader;
    TrajectoryStreamProvider * p_streamProvider;
    vector<string> p_files;

    TaskJob * p_job;

    ewxTextCtrl     * p_fileName;
    PlaybackControl * p_playback;

    DECLARE_DYNAMIC_CLASS(TrajectoryPanel)
    DECLARE_EVENT_TABLE()
};

#endif // TRAJECTORY_PANEL_H_

