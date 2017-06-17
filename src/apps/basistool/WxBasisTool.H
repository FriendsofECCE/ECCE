#ifndef WXBASISTOOL_HH
#define WXBASISTOOL_HH

#include "util/JMSPublisher.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSConfig.H"

#include "wxgui/CalcDropHandler.H"
#include "wxgui/FeedbackSaveHandler.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxFeedback.H"

#include "WxBasisToolGUI.H"

class EDSIGaussianBasisSetLibrary;
class ewxButton;
class ewxCheckBox;
class ewxGrid;
class ewxListBox;
class ewxMenuItem;
class ewxRadioButton;
class ewxStaticText;
class Fragment;
class GridSelectionBlock;
class ewxNotebook;
class JCode;
class PerTabPanel;
class Preferences;
class TCalculation;
class TTheory;
class WxBasisSetCoefficients;
class WxBasisSetDetail;
class WxContractionEditor;
class WxFeedback;

//#define WXBASISTOOL_SPHERICAL  0
//#define WXBASISTOOL_CARTESIAN  1

#define WXBASISTOOL_JMSNAME   "Basistool"

//#define WXBASISTOOL_DEBUG

#define WXBASISTOOL_TOTAL_BASISSET_TYPES    12

#define WXBASISTOOL_WINDOW_MINWIDTH     720
#define WXBASISTOOL_WINDOW_MINHEIGHT    600
#define WXBASISTOOL_WINDOW_DFLTLEFT     200
#define WXBASISTOOL_WINDOW_DFLTTOP      200
#define WXBASISTOOL_WINDOW_DFLTWIDTH    WXBASISTOOL_WINDOW_MINWIDTH
#define WXBASISTOOL_WINDOW_DFLTHEIGHT   WXBASISTOOL_WINDOW_MINHEIGHT

#define ID_PERTAB_WXBASISTOOL_ELEMENTS      34101


class WxBasisTool: public WxBasisToolGUI,
                   public WxDavAuth,
                   public JMSPublisher,
                   public FeedbackSaveHandler,
                   public CalcDropHandler
{
    DECLARE_CLASS(WxBasisTool)
    DECLARE_EVENT_TABLE()

    public:
        /// Constructors, Destructor
        WxBasisTool(      wxWindow* parent,
                             wxWindowID id = SYMBOL_WXBASISTOOLGUI_IDNAME,
                       const wxString& caption = SYMBOL_WXBASISTOOLGUI_TITLE,
                       const wxPoint& pos = SYMBOL_WXBASISTOOLGUI_POSITION,
                       const wxSize& size = SYMBOL_WXBASISTOOLGUI_SIZE,
                             long style = SYMBOL_WXBASISTOOLGUI_STYLE);
        virtual ~WxBasisTool();

        void programSaveMenuItemClickCB(wxCommandEvent& event);
        void programCoefficientsMenuItemClickCB(wxCommandEvent& event);
        void editClearSelectionMenuItemClickCB(wxCommandEvent& event);
        void editClearAllMenuItemClickCB(wxCommandEvent& event);
        void programQuitMenuItemClickCB(wxCommandEvent& event);
        void helpProgramMenuItemClickCB(wxCommandEvent& event);
        void helpSupportMenuItemClickCB(wxCommandEvent& event);

        void windowSplitterSashPosChangingCB( wxSplitterEvent& event );
        void windowSplitterSashPosChangedCB( wxSplitterEvent& event );
        void windowSplitterSizeCB(wxSizeEvent& event);

        void basisSetsNotebookPageChangingCB(wxNotebookEvent& event);
        void basisSetsNotebookPageChangedCB(wxNotebookEvent& event);

        void basisSetCategoryNotebookPageChangingCB(wxNotebookEvent& event);
        void basisSetCategoryNotebookPageChangedCB(wxNotebookEvent& event);

        void basisSetTypeListboxSelectedCB(wxCommandEvent& event);
        void basisSetTypeListboxDoubleClickedCB(wxCommandEvent& event);

        void detailsButtonClickCB( wxCommandEvent& event );

        void elementSelectionChangedCB(wxCommandEvent& event);

        void contextBasisSetsNotebookPageChangingCB(wxNotebookEvent& event);
        void contextBasisSetsNotebookPageChangedCB(wxNotebookEvent& event);

        void contextAddButtonClickCB(wxCommandEvent& event);
        void contextBasisSetsGridRangeSelectCB(wxGridRangeSelectEvent& event);
        void contextBasisSetsGridCellSelectCB(wxGridEvent& event);

        void contextPolarizationRadioButtonSelectedCB(wxCommandEvent& event);
        void contextOptimizeCheckboxClickCB(wxCommandEvent& event);

       /// /void contextClearSelectionButtonClickCB(wxCommandEvent& event);
      // / void contextClearTableButtonClickCB(wxCommandEvent& event);
       void showEditorButtonClickCB(wxCommandEvent& event);
       // void contextSaveButtonClickCB(wxCommandEvent& event);

        void contextMultipleAllowedCheckboxClickCB(wxCommandEvent& event);
        void windowSizeCB( wxSizeEvent& event );
        void windowCloseCB(wxCloseEvent& event);

        void loadSettings(string fname);
        void saveSettings();

        void reloadPerTabPrefs();

        PerTabPanel * getPeriodicTable();
        WxFeedback * getMessagesFeedback();

        bool verifyExit();

        enum contextChangeType
        {
            cntxtNewCalculation,
            cntxtFragmentChange,
            cntxtBasisChange,
            cntxtTheoryChange
        };

        enum basisColumnType
        {
            clmnOrbital,
            clmnPolarization,
            clmnDiffuse,
            clmnRydberg,
            clmnECP,
            clmnDFTCharge,
            clmnDFTExchange
        };

        EcceURL * getContext();
        void openLibraryAccess(EcceURL url);

        bool isSameContext(EcceURL *url, const string& theoryType);
        void setContext(EcceURL *url, contextChangeType changeType,
                        const string& changeMsg, const string& theory);

        void updateContextURL(EcceURL *urlPrev, EcceURL *urlCurr);
        void refreshPeriodicTable();
        void refreshFeedbackPanel();
        void refreshContextFormula();
        void refreshContextStatus(ResourceDescriptor::RUNSTATE state);
        void refreshBasisSetsDisplay();

        void updateNumbers();
        void displayEC();

        void saveContext(EcceURL *urlTrgt);

        //  Implementation of FeedbackSaveHandler interface
        virtual void processSave();

        //  Implementation of CalcDropHandler interface
        virtual void processDrop(const string& url);

        void showErrorMessage(string mesg);
        void showContextChanged(bool chgd);
        bool promptContextSave(const bool& allowCancel = true);

    private:
        void selectElements(Fragment *frag);
        void selectElements(string elmts);
        void initControls();
        void updateBasisSets(int index, bool slctFirst = true);
     //   void displayBasisList(wxListBox *list, const vector<const char*>& items);
        void setEditingAllowed(bool enbld);

        bool saveToFile(const char *fpath);
        void saveContext();

        bool checkCodeRules(bool verifyAll);
        bool reportGBSRuleViolations();
        void showCoverage(string bsName,
                               TGaussianBasisSet::GBSType bsType);
//        void outOfBrowseMode(bool autoOptimize);
        void resetDFTObjects(const JCode *code, TTheory *thry);
        void resetECPObjects(const JCode* code);
        void resetCodeObjects(bool autoOptimize);
        void setGbsToTable(ewxGrid* grid, int row, TGBSGroup* group);
        void setPolarizationAndOptimize(TGBSConfig* cnfg);
        void showFormula(TGBSConfig* cnfg);
        void indicateGBSConfig();
       // void indicateChemSys();
        void updateNumbers(TGBSConfig *cnfg, Fragment *frag);
        void updateNumbers(TGBSConfig *cnfg);
        void resetBSTable(TGBSConfig* cnfg, Fragment *frag);
    //    void showColumn(int c);
    //    void hideColumn(int c);
        void setGridColumnVisible(int col, bool vsbl);
     //   void resizeHack();
     //   bool addAllowed() const;
        void addBasisSet(string name);
        TGBSGroup* updateGroup(TGBSConfig* config, string& tag, string& bsName);
//        void displayContractionEditor();
        bool confirmCollapse(TGBSConfig::ConfigType destType);

        void notifyChange();

        wxSplitterWindow                *p_mainSplitWindow;

        ewxNotebook                     *p_basisSetsNotebook;
        ewxPanel                        *p_basisSetCategoryPanel[4];

        ewxNotebook                     *p_basisSetCategoryNotebook[4];

        ewxPanel                        *p_basisSetTypePanel[12];
        ewxListBox                      *p_basisSetTypeListBox[12];
        TGaussianBasisSet::GBSType       p_basisSetTypes[12];       //  Map that relates the index of the basisSet List
                                                                    //  to the GBSType that it represents.
        int                              p_basisSetCtgys[12];       //  Map that relates the index of the basisSet List
                                                                    //  to the category that encompasses the type represented

        bool                             p_calcChgd;
//        ewxStaticText                   *p_contextCodeStatic;

        WxFeedback                      *p_messagesFeedback;
        EDSIGaussianBasisSetLibrary     *p_gbsFactory;

        ewxStaticText                   *p_contextFormulaStatic;

        ewxNotebook                     *p_contextBasisSetsNotebook;
        ewxPanel                        *p_contextBasisSetsPanel[3];
        ewxGrid                         *p_contextBasisSetsGrid[3];
        GridSelectionBlock              *p_gridSlctnBlock[3];

        ewxCheckBox                     *p_contextAllowMultipleCheckbox;

        TCalculation                    *p_tcalc;
        EcceURL                         *p_urlCalc;
        TCalculation                    *p_tcalcUndo;

//        ewxPanel                        *p_contextPanel;

        bool                             p_editAllwd;           //  Whether to allow editing on the context
        ewxPanel                        *p_dividerPanel;

        string                           p_theoryDscpn;
        static const char               *p_columnLabels[];

 //       BasisToolColumn                 *p_gridClmns[7];

        ewxRadioButton                  *p_polarizationRadioButton[2];
        ewxCheckBox                     *p_optimizeCheckBox;

    //    EcceURL                         *p_calcURL;
        //string                           p_prefsFile;

        ewxStaticText                   *p_functionsStatic;
        ewxStaticText                   *p_primativesStatic;
        ewxStaticText                   *p_effPrimativesStatic;

        ewxMenuItem                     *p_editClearSlctnMenuItem;
        ewxMenuItem                     *p_editClearAllMenuItem;
        ewxMenuItem                     *p_programSaveMenuItem;
        ewxMenuItem                     *p_programCoeffsMenuItem;
        ewxButton                       *p_showEditorButton;

        ewxButton                       *p_contextAddButton;
        ewxButton                       *p_detailsButton;

        string                           p_lastSlctn;

        bool                             p_multSpptd;       //  Whether or not option to allow
                                                            //      multiple orbitals is supported
                                                            //      (depends on calculation code)
        bool                             p_multAllwd;       //  Whether or not multiple orbitals
                                                            //      are allowed (determined by user)

        bool                             p_inCtrlUpdate;

        TGBSConfig                       p_cnfgMode;

        int                              p_typeSlctn;   //  Index of the selected BasisSetType
                                                        //  Use p_basisSetTypes[] array to get
                                                        //      actual BasisSetType.

        WxContractionEditor             *p_cntrcEdit;
        WxBasisSetDetail                *p_bsetDetail;
        WxBasisSetCoefficients          *p_bsetCoeffs;

        ewxPanel                        *p_cntxtOprtnsPanel;
        ewxPanel                        *p_cntxtMoleculePanel;
        ewxPanel                        *p_elementsPanel;
        PerTabPanel                     *p_elementsTable;

        int                             p_splitPostnMin;
        int                             p_splitPostnMax;

        Preferences                     *p_prefs;

        int WXBASISTOOL_BASISSET_CATEGORY_PANEL_ITEMS[4];
        int WXBASISTOOL_BASISSET_TYPE_PANEL_ITEMS[12];
        int WXBASISTOOL_BASISSET_TYPE_LISTBOX_ITEMS[12];
        int WXBASISTOOL_POLARIZATION_RADIOBUTTON_ITEMS[2];
        int WXBASISTOOL_CONTEXT_BASISSETS_GRID_ITEMS[3];
        int WXBASISTOOL_CONTEXT_BASISSETS_PANEL_ITEMS[3];


        static int p_indentLevel;
        static void DebugEnterMethod(string dscpn);
        static void DebugWriteMessage(string mesg);
        static void DebugLeaveMethod();

};


#endif  //  WXBASISTOOL_HH



