#ifndef _GEOMCONSTRAINTS_H_
#define _GEOMCONSTRAINTS_H_

#include "wx/grid.h" // for wxGridEvent

#include "wxgui/WxJMSListener.H"
#include "wxgui/WxDavAuth.H"

#include "GeomConstraintsGUI.H"


class GeomConstraintModel;
class GeomConstraint;
class WxFeedback;
class WxGridView;
class WxJMSMessageDispatch;
class CalcEd;



class GeomConstraints: public GeomConstraintsGUI,
                       public WxDavAuth,
                       public WxJMSListener
{

  DECLARE_CLASS(GeomConstraints)

  public:

    GeomConstraints(CalcEd *calcEd,
                    wxWindow* parent,
                    wxWindowID id = SYMBOL_GEOMCONSTRAINTSGUI_IDNAME,
                    const wxString& caption = SYMBOL_GEOMCONSTRAINTSGUI_TITLE,
                    const wxPoint& pos = SYMBOL_GEOMCONSTRAINTSGUI_POSITION,
                    const wxSize& size = SYMBOL_GEOMCONSTRAINTSGUI_SIZE,
                    long style = SYMBOL_GEOMCONSTRAINTSGUI_STYLE );

    virtual ~GeomConstraints( );

    void initDisplay(bool isEnabled=true);

  protected:

    GeomConstraints();
    void clearTables();

    void createTables();
    void hideTable(WxGridView *table, wxSizer *container);
    void initConstraintTable(WxGridView *table);
    void initFixedAtomTable();
    void setTableSize(WxGridView *table);
    void createBondTables();
    void createAngleTables();
    void createTorsionTables();
    void createFixedAtomTable();
    void notify();
    GeomConstraintModel *getModel();
    void displayTorsionConstraints(GeomConstraintModel *model);
    void displayAngleConstraints(GeomConstraintModel *model);
    void displayBondConstraints(GeomConstraintModel *model);
    void displayTorsionRestraints(GeomConstraintModel *model);
    void displayAngleRestraints(GeomConstraintModel *model);
    void displayBondRestraints(GeomConstraintModel *model);
    void displayFixedAtoms(GeomConstraintModel *model);
    void displayConstraints(wxSizer *container, 
                            WxGridView *table, 
                            vector<GeomConstraint*>& constraints,
                            bool isRestraintTable,
                            bool isSupported);
    void filterConstraints(vector<GeomConstraint*>& constraints);
    void filterRestraints(vector<GeomConstraint*>& constraints);
    void requestBuilder(const string& actionTopic="");
    vector<GeomConstraint*> *getSelectedTableConstraints();


    enum SelectionType {CONSTRAINT, RESTRAINT, FIXED};

    virtual void OnCloseWindow(wxCloseEvent& event);
    virtual void OnConstrainButtonClick(wxCommandEvent& event);
    virtual void OnRestrainButtonClick(wxCommandEvent& event);
    virtual void OnFixAtomsButtonClick(wxCommandEvent& event);
    virtual void OnRemoveButtonClick(wxCommandEvent& event);
    virtual void OnClearAllButtonClick(wxCommandEvent& event);
    virtual void OnCloseButtonClick(wxCommandEvent& event);
    virtual void OnHelpButtonClick(wxCommandEvent& event);

    void connectEventHandlers();
    void gridCellLeftClick(wxGridEvent& event);
    void gridCellChange(wxGridEvent& event);

  private:

    void initMessaging();
    void selectionMCB(JMSMessage& msg);

    WxJMSMessageDispatch *p_msgDispatcher;
    CalcEd *p_calcEd;
    SelectionType p_selectionType; 
    WxGridView *p_torsionConstraints;
    WxGridView *p_angleConstraints;
    WxGridView *p_bondConstraints;
    WxGridView *p_torsionRestraints;
    WxGridView *p_angleRestraints;
    WxGridView *p_bondRestraints;
    WxGridView *p_fixedAtomConstraints;
    WxGridView *p_selectedTable;
    int p_visibleRows;

};

#endif


