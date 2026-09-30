#ifndef _CALCED_H_
#define _CALCED_H_

#include <string>
#include <utility>
#include <vector>
using std::pair;
using std::vector;
  using std::string;

#include "wx/wx.h"
#include "wx/socket.h"

#include "util/JMSPublisher.H"
#include "util/EditListener.H"

#include "tdat/SpinMult.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"
#include "tdat/ESPConstraintModel.H"

#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/FeedbackSaveHandler.H"
#include "wxgui/WxDavAuth.H"

#include "comm/RCommand.H"

#include "CalcEdGUI.H"
#include "InputVerifier.H"

#define wxID_THEORY_CHANGE 10998
#define wxID_RUNTYPE_CHANGE 10999

class EcceTool;
class Fragment;
class GUIValues;
class ICalculation;
class JCode;
class SFile;
class TGBSConfig;
class GeomConstraints;
class PartialCharge;

class CalcEd: public CalcEdGUI,
              public WxDavAuth,
              public JMSPublisher,
              public FeedbackSaveHandler,
              public CalcDropHandler,
              public EditListener
{
  DECLARE_CLASS( CalcEd )
  DECLARE_EVENT_TABLE()

  public:
    CalcEd( );
    CalcEd( wxWindow* parent, wxWindowID id = SYMBOL_CALCEDGUI_IDNAME,
            const wxString& caption = SYMBOL_CALCEDGUI_TITLE,
            const wxPoint& pos = SYMBOL_CALCEDGUI_POSITION,
            const wxSize& size = SYMBOL_CALCEDGUI_SIZE,
            long style = SYMBOL_CALCEDGUI_STYLE );
    ~CalcEd( );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CALCEDGUI_IDNAME,
                 const wxString& caption = SYMBOL_CALCEDGUI_TITLE,
                 const wxPoint& pos = SYMBOL_CALCEDGUI_POSITION,
                 const wxSize& size = SYMBOL_CALCEDGUI_SIZE,
                 long style = SYMBOL_CALCEDGUI_STYLE );

    void setContext(const string& url, const string& codeName="");
    string getContext() const;
    void setEditStatus(WxFeedback::EditStatus status);
    Fragment *getFrag() const;
    WxFeedback *getFeedback();
    ESPConstraintModel *getESPConstraintModel();
    string getCodeName() const;

    // FeedbackSaveHandler virtuals
    virtual void processSave();

    // CalcDropHandler virtuals
    virtual void processDrop(const string& url);

    // EditListener virtuals
    virtual void processEditCompletion(const EditEvent& ee);

    // Event handlers
    virtual void OnButtonCalcedCodeClick( wxCommandEvent& event );
    virtual void OnToolClick(wxCommandEvent &event);
    virtual void OnCloseWindow( wxCloseEvent& event );
    virtual void OnSaveClick( wxCommandEvent& event );
    virtual void OnMenuCalcedSavePrefClick( wxCommandEvent& event );
    virtual void OnMenuCalcedRegenInputClick( wxCommandEvent& event );
    virtual void OnExitClick( wxCommandEvent& event );
    virtual void OnHelpClick( wxCommandEvent& event );
    virtual void OnMenuFeedbackClick( wxCommandEvent& event );
    virtual void OnTextctrlCalcedNameEnter( wxCommandEvent& event);
    virtual void OnComboboxCalcedChargeSelected( wxCommandEvent& event );
    virtual void OnComboboxCalcedChargeEnter( wxCommandEvent& event );
    virtual void OnComboboxCalcedSpinMultSelected( wxCommandEvent& event );
    virtual void OnComboboxCalcedSpinMultEnter( wxCommandEvent& event );
    virtual void OnCheckboxCalcedIrreducibleClick( wxCommandEvent& event );
    virtual void OnCheckboxCalcedUseSymmetryClick( wxCommandEvent& event );

    /** Read/write the "use symmetry" choice on the calculation. */
    bool getUseSymmetry() const;
    void setUseSymmetry(const bool& value);
    void storeUseSymmetry(const bool& value);
    virtual void OnButtonCalcedBasisQuickClick( wxCommandEvent& event );
    virtual void OnChoiceCalcedTheorySelected( wxCommandEvent& event );
    virtual void OnChoiceCalcedRuntypeSelected( wxCommandEvent& event );
            void OnMenuCalcedBasisSetSelected( wxCommandEvent& event );
    virtual void OnCheckboxCalcedUseExponentsClick( wxCommandEvent& event );
    virtual void OnButtonCalcedTheoryClick( wxCommandEvent& event );
    virtual void OnButtonCalcedRuntypeClick( wxCommandEvent& event );
    virtual void OnButtonCalcedPartialClick( wxCommandEvent& event );
    virtual void OnButtonCalcedConstraintClick( wxCommandEvent& event );
    virtual void OnButtonCalcedFinalEditClick( wxCommandEvent& event );
    virtual void OnButtonCalcedVerifyClick( wxCommandEvent& event );
    virtual void OnButtonCalcedLaunchClick( wxCommandEvent& event );

    // Calc/Project change message/event callbacks (MCB)
    void moveMCB(wxCommandEvent& event);
    void deleteMCB(wxCommandEvent& event);
    void basisMCB(wxCommandEvent& event);
    void codeMCB(wxCommandEvent& event);
    void detailsMCB(wxCommandEvent& event);
    void launchMCB(wxCommandEvent& event);
    void runtypeMCB(wxCommandEvent& event);
    void spinMCB(wxCommandEvent& event);
    void stateMCB(wxCommandEvent& event);
    void subjectMCB(wxCommandEvent& event);
    void theoryMCB(wxCommandEvent& event);
    void propertyMCB(wxCommandEvent& event);

    // Sub-dialogs need to be able to call this
    void enableSave(const bool& enable=true);

  protected:
    void CreateControls();
    void replaceBuilderButton();
    void replaceBasisSetButton();
    void fixBuilderButtonId();
    void createCodeButtons();
    void setCurrentCodeButton(const string& currentCode);

    void doSetContext(const string& codeName);
    void setContextTheoryRuntype();
    void freeContext();

    // getters for calc values persisted by the GUI
    wxString               getTheoryCategory() const;
    wxString               getTheoryName() const;
    TTheory                getTheory() const;
    wxString               getRuntypeName() const;
    bool                   getRuntypeNoSpherical() const;
    TRunType               getRuntype() const;
    int                    getOpenShells() const;
    bool                   getUseExpCoeff() const;
    bool                   getUseIrreducible() const;
    SpinMult::SpinMultEnum getSpinMult() const;

    // setters for calc values persisted by the GUI
    bool setTheory(const TTheory* value, const bool& overrideFlag);
    bool setRuntype(const TRunType& value);
    void setOpenShells(const int& value);
    void setUseExpCoeff(const bool& value);
    void setUseIrreducible(const bool& value);
    bool setSpinMult(const SpinMult::SpinMultEnum& value,
                     const bool& reportFlag);
    
    // parts of the GUI must be rebuilt based on GUI or calc properties
    void populateTheories();
    void populateRuntypes();
    void populateSpinMultiplicities();
    void populateSummaryFields();
    void populateSummaryField(const string& summaryType);

    void resetTheoryDetails();
    void resetRuntypeDetails();
    void resetMinorDetails();

    void restrictSpinByCharge(const bool& force=false);
    void restrictTheoriesBySpin(const bool& force=false);

    bool isValidCode(const JCode* code);
    bool isDetailsReady() const;
    bool theoryNeedsBasis() const;
    bool isReady() const;
    bool requiresMissingCell() const;
    bool isGbsValid();

    // refresh & enable/disable & show/hide indicated field sets
    void updateAllFields();
    void updateChemSysFields();
    void updateBasisSetFields();
    void updateDetailsFields();
    
    // refresh indicated field values
    void refreshAllFields();
    void refreshChemSysFields();
    void refreshChemSysThumb();
    void refreshBasisSetFields();
    void refreshDetailsFields();

    // enable/disable indicated field sets
    void enableAllFields();
    void enableChemSysFields();
    void enableBasisSetFields();
    void enableDetailsFields();
    void enableLaunch();
    void enableGeomConstraints();

    // show/hide indicated field sets
    void showAllFields();
    void showChemSysFields();
    void showBasisSetFields();
    void showDetailsFields();
    void showGeomEditor();
    void showPartialEditor();

    void makeRuntypeNoSphericalConsistent();
    void urlChangeNotify(const string& topic) const;
    bool generateInput(const bool& paramFlag);

    wxWindowID doClose(const bool& allowCancel=true);
    void doSave();
    void doTheoryChange();
    void doRuntypeChange();

    void saveSettings();
    void restoreSettings();
    void updateGeomModel();
    void updateESPModel();
    void clearGeomModel();
    void clearESPModel();

    void startApp(int id, int force, const string& url) const;
    void startApp(const string& app, int force, const string& url) const;
    
    string buildTheoryRuntypeArgs(const bool& isTheory) const;
    void getOrbitalParams(int &electrons, int &spin, int &frozenOrbs,
                       int &occupiedOrbs, int &virtualOrbs,
                       int &normalModes) const;
    unsigned long getCoreElectrons(const unsigned long atomicNumber) const;

    // wxPython IPC functions
    bool launchDetachedApp(const string& cmd);
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
    bool write_esp(const string& output_file, bool& file_flag);
    bool write_constraints(const string& output_file, bool& file_flag);
    bool write_gbsconfig(const string& output_file, const bool& useExpCoeff,
                         bool& file_flag, bool& ecp_flag, bool& spherical_flag);
    bool write_setup(const string& output_file);
    bool write_template(RCommand& localconn, const string& templateFile,
                        const string& output_file);
    bool input_controller(const bool& saveParamFlag, const bool& useExpCoeff,
                          string& message);

    /**
     * Read the stored input file, run the checker over it, and set the
     * lamp (GitHub #148).
     *
     * @param findings  filled in, when the caller wants to show them
     * @return false if the checker could not be run, which leaves the
     *         lamp BLANK rather than green: a light that cannot tell
     *         the difference between "checked and fine" and "not
     *         checked" is worse than no light, because it is the first
     *         one the user will believe.
     */
    bool verifyInput(vector<VerifyFinding>* findings = 0);

    /** Set the lamp, or clear it when `checked` is false. */
    void setVerifyLight(bool checked, VerifyFinding::Level level,
                        const string& tip);

    /**
     * The structure can change out from under an already-open CalcEd --
     * edited in the Builder viewer while this window stays open on the
     * same calculation (#GitHub, reported live 2026-09-27).  doSave()
     * only regenerates the deck when ITS OWN fields are marked
     * MODIFIED, which a purely-external structure edit never sets, so
     * the stale deck was what got launched.
     *
     * Call this at every point that is about to hand the deck to
     * something else (Launch, switching to another EcceTool).  It asks
     * the calculation directly (ICalculation::isFragmentNew(), a real
     * WebDAV timestamp comparison) rather than trusting this window's
     * own edit-status bookkeeping, and regenerates when the structure
     * is newer than the input file.
     *
     * If the current input file is a Final Edit hand edit, regenerating
     * would silently destroy it, so this asks first (Yes = regenerate
     * and lose the hand edit / No = launch the hand-edited file as is /
     * Cancel = do nothing).
     *
     * @return false only on Cancel -- the caller must not proceed
     *         (launch, tool switch) when this returns false.
     */
    bool regenerateIfStructureChanged();

  private:
    WxFeedback *p_feedback;
    EcceTool   *p_builderTool;
    EcceTool   *p_basisSetTool;

    string                  p_codeName;
    const JCode            *p_code;
    SpinMult::SpinMultEnum  p_spinMult;
    GUIValues              *p_GUIValues;

    ICalculation *p_iCalc;
    Fragment     *p_frag;
    Fragment     *p_fullFrag;

    TGBSConfig *p_basis;

    TTheory  p_lastTheory;
    TRunType p_lastRuntype;

    /** the geom constraints dialog. */
    GeomConstraints *p_geomConstraints;

    /** the partial charge dialog. */
    PartialCharge   *p_partialCharge;

    ESPConstraintModel *p_ESPCnstrnt;

    string p_context;
    bool   p_startUp;

    /**
     * True once Final Edit has written a hand-edited deck back to DAV
     * (processEditCompletion()), and cleared again the next time
     * generateInput() writes a fresh, editor-generated deck.  Tracks
     * whether the CURRENT input file on disk is a hand edit that
     * regenerateIfStructureChanged() must not silently overwrite.
     */
    bool   p_handEdited;

    /**
     * True when the last generateInput() ran the generator and it failed:
     * the settings describe a deck that cannot be written, and any deck
     * on DAV (if there is one at all) is an older one.  Feeds the Verify
     * lamp/dialog (#148) as a BAD finding -- it does NOT gate Launch or
     * Final Edit (Andy, 2026-09-29: Verify is what should flag a bad
     * deck; Launch should let people do what they want with it).
     */
    bool   p_inputGenFailed;

    /** The generator's own diagnostic, cleaned of shell/session noise,
     *  from the last failed generateInput().  Empty when the last
     *  attempt succeeded or none has run yet. */
    string p_inputGenError;

    /** "ai.<code> warning [anchor]: message" lines from the last
     *  SUCCESSFUL generation, as (anchor, message).  The deck was
     *  written; Verify shows these, anchored on the first deck line
     *  containing the anchor.  Cleared on every generation. */
    vector<pair<string, string> > p_inputGenWarnings;

    /** Persist/restore p_inputGenWarnings as a DAV property on the
     *  calculation, so the lamp describes the deck on the server and
     *  not just this session.  Store never writes to a calculation
     *  that is read-only. */
    void storeInputGenWarnings();
    void loadInputGenWarnings();

    /**
     * Whether this calculation has an input file at all, as of the last
     * time verifyInput() checked (context open, a generation attempt,
     * a hand edit).  Launch needs a deck to submit -- this is cheaper
     * than a DAV fetch on every enableLaunch(), which runs on nearly
     * every edit.
     */
    bool   p_hasInputFile;

    const static char* p_BASIS_QUICK_PICKS[];

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

#endif // _CALCED_H_
