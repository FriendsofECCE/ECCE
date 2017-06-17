#ifndef _GEOMCONSTRAINTS_H_
#define _GEOMCONSTRAINTS_H_

#include "wx/grid.h" // for wxGridEvent

#include "wxgui/WxJMSListener.H"
#include "wxgui/WxDavAuth.H"

#include "MetaPotentialsGUI.H"


class MetaPotentialModel;
class MetaPotential;
class WxFeedback;
class WxGridView;
class WxJMSMessageDispatch;
class MetaEd;



class MetaPotentials: public MetaPotentialsGUI,
                      public WxDavAuth,
                      public WxJMSListener
{

  DECLARE_CLASS(MetaPotentials)

  public:

    MetaPotentials(MetaEd *metaEd,
                    wxWindow* parent,
                    wxWindowID id = SYMBOL_METAPOTENTIALSGUI_IDNAME,
                    const wxString& caption = SYMBOL_METAPOTENTIALSGUI_TITLE,
                    const wxPoint& pos = SYMBOL_METAPOTENTIALSGUI_POSITION,
                    const wxSize& size = SYMBOL_METAPOTENTIALSGUI_SIZE,
                    long style = SYMBOL_METAPOTENTIALSGUI_STYLE );

    virtual ~MetaPotentials( );

    void initDisplay(bool isEnabled=true);

  protected:

    MetaPotentials();
    void clearTables();

    void createTables();
    void hideTable(WxGridView *table, wxSizer *container);
    void initConstraintTable(WxGridView *table);
    void setTableSize(WxGridView *table);
    void createBondTables();
    void createAngleTables();
    void notify();
    MetaPotentialModel *getModel();
    void displayAngleConstraints(MetaPotentialModel *model);
    void displayBondMetaDynamicsPotential(MetaPotentialModel *model);
    void displayAngleMetaDynamicsPotential(MetaPotentialModel *model);
    void displayPotentials(wxSizer *container, 
                           WxGridView *table, 
                           vector<MetaPotential*>& potentials);
    void filterConstraints(vector<MetaPotential*>& constraints);
    void filterRestraints(vector<MetaPotential*>& constraints);
    void requestBuilder(const string& actionTopic="");
    vector<MetaPotential*> *getSelectedTableConstraints();


    virtual void OnCloseWindow(wxCloseEvent& event);
    virtual void OnPotentialButtonClick(wxCommandEvent& event);
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
    MetaEd *p_metaEd;
    WxGridView *p_bondMetaDynamicsPotential;
    WxGridView *p_angleMetaDynamicsPotential;
    WxGridView *p_selectedTable;
    int p_visibleRows;

};

#endif


