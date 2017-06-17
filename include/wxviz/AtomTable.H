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

    /** map atom indices to table indices to deal with the fact that the
       table does not display all atoms. -1 = notdisplayed */
    vector<int> p_atommap;
};

#endif
