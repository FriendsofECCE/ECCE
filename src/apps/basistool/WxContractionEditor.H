#ifndef WXCONTRACTIONEDITOR_HH
#define WXCONTRACTIONEDITOR_HH

#include <wx/event.h>

#include "util/Preferences.H"

#include "dsm/TGBSConfig.H"

#include "WxContractionEditorGUI.H"
#include "GBSContractionGridModel.H"
#include "GBSSummaryGridModel.H"

class ewxChoice;
class ewxGrid;
class ewxPanel;

#define WXCONTRACTIONEDITOR_WINDOW_MINWIDTH     250
#define WXCONTRACTIONEDITOR_WINDOW_MINHEIGHT    200
#define WXCONTRACTIONEDITOR_WINDOW_DFLTLEFT     200
#define WXCONTRACTIONEDITOR_WINDOW_DFLTTOP      200
#define WXCONTRACTIONEDITOR_WINDOW_DFLTWIDTH    WXCONTRACTIONEDITOR_WINDOW_MINWIDTH
#define WXCONTRACTIONEDITOR_WINDOW_DFLTHEIGHT   WXCONTRACTIONEDITOR_WINDOW_MINHEIGHT

//#define WXCONTRACTIONEDITOR_DEBUG





class WxContractionEditor: public WxContractionEditorGUI //, public wxEvtHandler
{
    DECLARE_CLASS(WxContractionEditor)

    public:
        /// Constructors, Destructor
        WxContractionEditor(      wxWindow* parent,
                                  wxWindowID id = SYMBOL_WXCONTRACTIONEDITORGUI_IDNAME,
                            const wxString& caption = SYMBOL_WXCONTRACTIONEDITORGUI_TITLE,
                            const wxPoint& pos = SYMBOL_WXCONTRACTIONEDITORGUI_POSITION,
                            const wxSize& size = SYMBOL_WXCONTRACTIONEDITORGUI_SIZE,
                                  long style = SYMBOL_WXCONTRACTIONEDITORGUI_STYLE);
        virtual ~WxContractionEditor();

       // void contractionsGridSelectCellCB(wxGridEvent& event);

        void elementsChoiceSelectedCB(wxCommandEvent& event);
        void contractGridEditorHiddenCB(wxGridEvent& event);
        void summaryGridEditorHiddenCB(wxGridEvent& event);
        void closeButtonClickCB(wxCommandEvent& event);
        void contractGridCellSelectedCB( wxGridEvent& event );
        void summaryGridCellSelectedCB( wxGridEvent& event );
        void contractGridCellChangedCB( wxGridEvent& event );
        void summaryGridCellChangedCB( wxGridEvent& event );

        void setConfiguration(TGBSConfig* cnfg);
        void selectElement(int idx);
        void diffuseParamEditValueChangedCB(wxCommandEvent& event); //Widget, XtPointer, XtPointer);
        void updateTables();

        void saveSettings(Preferences *prefs);
        void loadSettings(Preferences *prefs);

    private:
        void initControls();
        void updateSummaryValue(const int& row,
                                const int& col,
                                const int& value);
        string getSelectionSymbol();
        void updateSummaryTable(const string& tag,
                                const int& shell,
                                const bool& recurse);

        bool        p_inCtrlUpdate;

        wxGridCellAttr      *p_cntrctsClmnAttrbs[4];
        wxGridCellAttr      *p_summaryClmnAttrbs[5];

        ewxGrid                             *p_cntrctsGrid;
        ewxGrid                             *p_summaryGrid;
        ewxPanel                            *p_elmtSlctnPanel;
        vector<const TGaussianBasisSet*>     p_slctnBasisSets;

        ewxChoice                           *p_elmtsChoice;

        WxParameterEdit                     *p_diffFnctnsEdit;
        wxWindow                            *p_prntWindow;


        vector<GBSContractionGridModel *>    *p_cntrcGridModels;
        vector<GBSSummaryGridModel *>        *p_smmryGridModels;

        int                                  p_elmtSlctnIdx;


        TGBSConfig                          *p_gbscnfg;
};

#endif          //  WXCONTRACTIONEDITOR_HH
