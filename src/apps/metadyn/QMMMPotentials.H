#ifndef _ATOMTYPES_H_
#define _ATOMTYPES_H_

#include "wx/grid.h" // for wxGridEvent

#include "wxgui/WxDavAuth.H"

#include "QMMMPotentialsGUI.H"


class QMMMModel;
class QMMMParams;
class WxFeedback;
class WxGridView;
class WxJMSMessageDispatch;
class MetaEd;



class QMMMPotentials: public QMMMPotentialsGUI,
                      public WxDavAuth
{

  DECLARE_CLASS(QMMMPotentials)

  public:

    QMMMPotentials(MetaEd *metaEd,
              wxWindow* parent,
              wxWindowID id = SYMBOL_QMMMPOTENTIALSGUI_IDNAME,
              const wxString& caption = SYMBOL_QMMMPOTENTIALSGUI_TITLE,
              const wxPoint& pos = SYMBOL_QMMMPOTENTIALSGUI_POSITION,
              const wxSize& size = SYMBOL_QMMMPOTENTIALSGUI_SIZE,
              long style = SYMBOL_QMMMPOTENTIALSGUI_STYLE );

    virtual ~QMMMPotentials( );

    void initDisplay(bool isEnabled=true);

  protected:

    QMMMPotentials();
    void clearTables();

    void createTables();
    void initQMMMLists();
    void hideTable(WxGridView *table, wxSizer *container);
    void initQMMMTypesTable(WxGridView *table);
    void setTableSize(WxGridView *table);
    void createSoluteAtomTable();
    void createSolventAtomTable();
    void createBondStretchTable();
    void createAngleBendTable();
    void createPseudPotTable();
    void notify();
    QMMMModel *getModel();
    void displaySoluteAtoms(QMMMModel *model);
    void displaySolventAtoms(QMMMModel *model);
    void displayPseudPotAtoms(QMMMModel *model);
    void displayBonds(QMMMModel *model);
    void displayAngles(QMMMModel *model);
    void displayQMMMTerms(wxSizer *container, 
                          WxGridView *table,
			  vector<QMMMParams*> &terms);
    void requestBuilder(const string& actionTopic="");
    vector<QMMMParams*> *getSelectedTableConstraints();


    enum SelectionType {CONSTRAINT, RESTRAINT, FIXED};

    virtual void OnCloseWindow(wxCloseEvent& event);
    virtual void OnClearAllButtonClick(wxCommandEvent& event);
    virtual void OnCloseButtonClick(wxCommandEvent& event);
    virtual void OnHelpButtonClick(wxCommandEvent& event);
    virtual void OnAtmtUseShakeCheckboxClick( wxCommandEvent& event );
    void setShakeConfig( bool useShake );

    void connectEventHandlers();
    void gridCellLeftClick(wxGridEvent& event);
    void gridCellChange(wxGridEvent& event);


  private:

    void initMessaging();
    void selectionMCB(JMSMessage& msg);

    WxJMSMessageDispatch *p_msgDispatcher;
    MetaEd *p_metaEd;
    SelectionType p_selectionType; 
    WxGridView *p_soluteAtomsTable;
    WxGridView *p_solventAtomsTable;
    WxGridView *p_bondStretchTable;
    WxGridView *p_angleBendTable;
    WxGridView *p_solventPseudPotTable;
    WxGridView *p_selectedTable;
    int p_visibleRows;
    int p_solventNumAtoms;
    bool p_shakeEvent;
};

#endif


