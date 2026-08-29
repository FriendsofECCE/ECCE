/**
 * @file
 *
 *
 */

#ifndef ATOMTABLE_HH
#define ATOMTABLE_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Event.H"
#include "util/Listener.H"
  using namespace ecce;

#include "util/InvalidException.H"
#include "wxgui/TearableContentProvider.H"
#include "wxviz/MoleculeDataTable.H"
#include "wxviz/WxVizTool.H"


class GridAtomElement;
class Tearable;
class wxConfig;

/**
 * Atom table view.  Displays a view of the
 * atoms contained in a fragment.  The
 * set of properties displayed for the resources
 * is defined in the Resource Descriptor file.
 */
class AtomTable : public MoleculeDataTable,
                  public TearableContentProvider
{
   DECLARE_DYNAMIC_CLASS( AtomTable )
   DECLARE_EVENT_TABLE()

  public:

    static const wxWindowID ID_DIALOG_ATOM_TABLE;
    static const wxWindowID ID_SUMCHARGES;
    static const wxWindowID ID_H2DEUT;
    static const wxWindowID ID_RESETMASS;
    static const wxWindowID ID_BQXATOMS;
    static const wxWindowID ID_CLRBEHAV;

    AtomTable();

    AtomTable(wxWindow* parent, 
              wxWindowID id = ID_DIALOG_ATOM_TABLE, 
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxTAB_TRAVERSAL);
    bool Create(wxWindow* parent, 
                wxWindowID id = ID_DIALOG_ATOM_TABLE, 
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTAB_TRAVERSAL);

    ~AtomTable();

    // Listener virtual
    void eventMCB(const Event& event);

    virtual void restoreSettings(wxConfig * config);
    virtual void saveSettings(wxConfig * config);

  protected:

    void fillTable();
    void fastFillTable();
    void sumCharges();
    int rowToAtomIndex(int row);
    void updateFragment(GridAtomElement *element);
    void notifySelections(vector<GridAtomElement*> *selections,
                          bool sendNotification=false);
    void setSelections();

    wxWindow* GetTearableContent();

    void OnMenuClick( wxCommandEvent& event );
    void OnSelectCell( wxGridEvent& event);
    virtual void gridRangeSelect( wxGridRangeSelectEvent& event);

    /** map atom indices to table indices to deal with the fact that the
       table does not display all atoms. -1 = notdisplayed */
    vector<int> p_atommap;

    /** Guards OnSelectCell/gridRangeSelect against firing a real SelectCmd
        in response to our own programmatic grid updates (fillTable/
        fastFillTable/setSelections) rather than a genuine user click --
        including wxGrid's own default cell-cursor/range bookkeeping,
        which fires EVT_GRID_SELECT_CELL and EVT_GRID_RANGE_SELECT on its
        own once the grid is first populated and realized. setSelections()
        additionally Disconnects gridRangeSelect for its own internal row
        selection loop, but only when there's an existing highlight to
        restore (hl->size() > 0) -- a freshly opened structure has none,
        so that bracket never engages, leaving gridRangeSelect's base
        class implementation (MoleculeDataTable::gridRangeSelect(), which
        has no knowledge of this flag at all) fully live during the very
        first fillTable()/fastFillTable() population. This override closes
        that gap directly, the same way OnSelectCell already does. */
    bool p_internalSelect;
};

#endif
