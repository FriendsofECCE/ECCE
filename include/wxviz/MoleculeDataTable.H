/**
 * @file
 *
 *
 */

#ifndef MOLECULEDATATABLE_HH
#define MOLECULEDATATABLE_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/grid.h" // for wxGridRangeSelectEvent

#include "util/Event.H"
#include "util/Listener.H"
  using namespace ecce;
#include "util/InvalidException.H"

#include "wxgui/ewxPanel.H"
#include "wxviz/WxVizTool.H"


class AtomView;
class WxGridView;
class WxVizToolFW;
class GridAtomElement;


/**
 * Base class for molecule data table information.
 * Each table is a WxGrid while each cell of 
 * the grid contains a GridAtomElement.
 */
class MoleculeDataTable : public ewxPanel, 
                          public WxVizTool,
                          public Listener
{
DECLARE_DYNAMIC_CLASS(MoleculeDataTable)

  public:

    MoleculeDataTable();

    MoleculeDataTable(const string& name, wxWindow *parent, 
              wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
              const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL);
    bool Create(const string& name, wxWindow *parent,
                wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
                const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL);
    ~MoleculeDataTable();

    virtual void setVisibleRows(int rows, bool applySetting=true);
    void eventMCB(const Event& event);
    void refresh();

    virtual void gridRangeSelect(wxGridRangeSelectEvent& event);

  protected:

    virtual void setSelections();
    virtual void connectEventHandlers();
    virtual void gridCellChange(wxGridEvent& event);

    virtual void fillTable() {;}
    virtual void updateFragment(GridAtomElement *element) {;}
    virtual void notifySelections(vector<GridAtomElement*> *selections, bool sendNotification=false) {;}
    virtual void saveSortSettings() {;}
    virtual void restoreSortSettings() {;}
    virtual vector<GridAtomElement*> *getCurrentSelections();
    virtual bool isVisible();
    static bool sortRows(const GridAtomElement *e1, const GridAtomElement *e2);


    WxGridView 
      *p_table;          /**< Pointer to a WxGridView object, that is responsible 
                              for presenting the atom data */

    int p_visibleRows;   /**< The number of rows visible in the WxGridView. */

    AtomView 
      *p_atomView;       /**< Pointer to current atom view being used in display */
    int p_viewId;        /** id of currently selected view */

};

#endif

