#ifndef _METAED_H_
#define _METAED_H_

#include <string>
  using std::string;

#include "wx/wx.h"
#include "wx/socket.h"

#include "util/JMSPublisher.H"
#include "util/EditListener.H"

#include "tdat/SpinMult.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"

#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/FeedbackSaveHandler.H"
#include "wxgui/WxDavAuth.H"

#include "comm/RCommand.H"

#include "MetaEdGUI.H"

#define wxID_THEORY_CHANGE 10998
#define wxID_RUNTYPE_CHANGE 10999

class EcceTool;
class Fragment;
class GUIValues;
class ICalculation;
class JCode;
class SFile;
class MetaPotentials;
class QMMMPotentials;

class MetaEd: public MetaEdGUI,
              public WxDavAuth,
              public JMSPublisher,
              public FeedbackSaveHandler,
              public CalcDropHandler,
              public EditListener
{
  DECLARE_CLASS( MetaEd )
  DECLARE_EVENT_TABLE()

  public:
    MetaEd( );
    MetaEd( wxWindow* parent, wxWindowID id = SYMBOL_METAEDGUI_IDNAME,
            const wxString& caption = SYMBOL_METAEDGUI_TITLE,
            const wxPoint& pos = SYMBOL_METAEDGUI_POSITION,
            const wxSize& size = SYMBOL_METAEDGUI_SIZE,
            long style = SYMBOL_METAEDGUI_STYLE );
    ~MetaEd( );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_METAEDGUI_IDNAME,
                 const wxString& caption = SYMBOL_METAEDGUI_TITLE,
                 const wxPoint& pos = SYMBOL_METAEDGUI_POSITION,
                 const wxSize& size = SYMBOL_METAEDGUI_SIZE,
                 long style = SYMBOL_METAEDGUI_STYLE );

    void setContext(const string& url);
    string getContext() const;
    void setEditStatus(WxFeedback::EditStatus status);
    Fragment *getFrag() const;
    WxFeedback *getFeedback();

    // FeedbackSaveHandler virtuals
    virtual void processSave();

    // CalcDropHandler virtuals
    virtual void processDrop(const string& url);

    // EditListener virtuals
    virtual void processEditCompletion(const EditEvent& ee);

    // Event handlers
    virtual void OnToolClick(wxCommandEvent &event);
    virtual void OnCloseWindow( wxCloseEvent& event );
    virtual void OnSaveClick( wxCommandEvent& event );
    virtual void OnExitClick( wxCommandEvent& event );
    virtual void OnHelpClick( wxCommandEvent& event );
    virtual void OnMenuFeedbackClick( wxCommandEvent& event );
    virtual void OnComboboxMetaedChargeSelected( wxCommandEvent& event );
    virtual void OnComboboxMetaedChargeEnter( wxCommandEvent& event );
    virtual void OnComboboxMetaedSpinMultSelected( wxCommandEvent& event );
    virtual void OnComboboxMetaedSpinMultEnter( wxCommandEvent& event );
    virtual void OnButtonMetaedTheoryClick( wxCommandEvent& event );
    virtual void OnButtonMetaedRuntypeClick( wxCommandEvent& event );
    virtual void OnButtonMetaedQmmmClick( wxCommandEvent& event );
    virtual void OnButtonMetaedPotentialClick( wxCommandEvent& event );
    virtual void OnButtonMetaedFinalEditClick( wxCommandEvent& event );
    virtual void OnButtonMetaedLaunchClick( wxCommandEvent& event );

    // Calc/Project change message/event callbacks (MCB)
    void moveMCB(wxCommandEvent& event);
    void deleteMCB(wxCommandEvent& event);
    void launchMCB(wxCommandEvent& event);
    void stateMCB(wxCommandEvent& event);
    void subjectMCB(wxCommandEvent& event);
    void propertyMCB(wxCommandEvent& event);

    // Sub-dialogs need to be able to call this
    void enableSave(const bool& enable=true);

  protected:
    void CreateControls();
    void replaceBuilderButton();
    void fixBuilderButtonId();

    void doSetContext();
    void freeContext();

    // getters for calc values persisted by the GUI
    TTheory                getTheory() const;
    bool                   getRuntypeNoSpherical() const;
    TRunType               getRuntype() const;
    int                    getOpenShells() const;
    SpinMult::SpinMultEnum getSpinMult() const;

    // setters for calc values persisted by the GUI
    bool setTheory(const TTheory* value, const bool& overrideFlag);
    bool setRuntype(const TRunType& value);
    void setOpenShells(const int& value);
    bool setSpinMult(const SpinMult::SpinMultEnum& value,
                     const bool& reportFlag);
    
    // parts of the GUI must be rebuilt based on GUI or calc properties
    void populateSpinMultiplicities();
    void populateSummaryFields();
    void populateSummaryField(const string& summaryType);

    void resetTheoryDetails();
    void resetRuntypeDetails();

    void restrictSpinByCharge();

    bool isDetailsReady() const;
    bool isReady() const;

    // refresh & enable/disable & show/hide indicated field sets
    void updateAllFields();
    void updateChemSysFields();
    void updateDetailsFields();
    
    // refresh indicated field values
    void refreshAllFields();
    void refreshChemSysFields();
    void refreshChemSysThumb();
    void refreshDetailsFields();

    // enable/disable indicated field sets
    void enableAllFields();
    void enableChemSysFields();
    void enableDetailsFields();
    void enableLaunch();
    void enableMetaPotentials();

    // show/hide indicated field sets
    void showAllFields();
    void showChemSysFields();
    void showDetailsFields();
    void showPotentialEditor();
    void showQMMMEditor();

    void urlChangeNotify(const string& topic) const;
    bool generateInput(const bool& paramFlag);

    wxWindowID doClose(const bool& allowCancel=true);
    void doSave();
    void doTheoryChange();
    void doRuntypeChange();

    void saveSettings();
    void restoreSettings();
    void updatePotentialModel();
    void updateQMMMModel();
    void clearPotentialModel();
    void clearQMMMModel();

    void startApp(int id, int force, const string& url) const;
    void startApp(const string& app, int force, const string& url) const;
    
    string buildTheoryRuntypeArgs(const bool& isTheory) const;
    void getOrbitalParams(int &electrons, int &spin, int &frozenOrbs,
                       int &occupiedOrbs, int &virtualOrbs,
                       int &normalModes) const;
    unsigned long getCoreElectrons(const unsigned long atomicNumber) const;

    // wxPython IPC functions
    void startTheoryApp(const bool& localInitFlag);
    void startRuntypeApp(const bool& localInitFlag);
    void OnTheoryIPC(wxSocketEvent& event);
    void OnRuntypeIPC(wxSocketEvent& event);
    void processTheoryInput(const char* databuf);
    void processRuntypeInput(const char* databuf);
    void closeTheoryApp(const bool& sendTerm);
    void closeRuntypeApp(const bool& sendTerm);

    // input file generation, aka InputController
    string changeWD(const string& newDir);
    bool write_cs(const string& output_file);
    bool write_potentials(const string& output_file, bool& file_flag);
    bool write_qmmmterms(const string& output_file, bool& file_flag);
    bool write_gbsconfig(const string& output_file, const bool& useExpCoeff,
                         bool& file_flag, bool& ecp_flag, bool& spherical_flag);
    bool write_setup(const string& output_file);
    bool write_template(RCommand& localconn, const string& templateFile,
                        const string& output_file);
    bool input_controller(const bool& saveParamFlag, const bool& useExpCoeff,
                          string& message);

  private:
    WxFeedback *p_feedback;
    EcceTool   *p_builderTool;

    string                  p_codeName;
    const JCode            *p_code;
    SpinMult::SpinMultEnum  p_spinMult;
    GUIValues              *p_GUIValues;

    ICalculation *p_iCalc;
    Fragment     *p_frag;
    Fragment     *p_fullFrag;

    TTheory  p_lastTheory;
    TRunType p_lastRuntype;

    /** the geom constraints dialog. */
    MetaPotentials *p_metaPotentials;

    /** the atom types dialog. */
    QMMMPotentials   *p_qmmmPotentials;

    string p_context;
    bool   p_startUp;

    // wxPython IPC for theory dialog
    int               p_theoryPid;
    string            p_theoryInFilePath;
    SFile            *p_theoryOutFile;
    wxDatagramSocket *p_theoryInSocket;
    unsigned short    p_theoryInPort;
    bool              p_theoryInFlag;
    bool              p_theoryHoldFlag;
    bool              p_theoryInitFlag;

    // wxPython IPC for runtype dialog
    int               p_runtypePid;
    string            p_runtypeInFilePath;
    SFile            *p_runtypeOutFile;
    wxDatagramSocket *p_runtypeInSocket;
    unsigned short    p_runtypeInPort;
    bool              p_runtypeInFlag;
    bool              p_runtypeHoldFlag;
    bool              p_runtypeInitFlag;
};

#endif // _METAED_H_
