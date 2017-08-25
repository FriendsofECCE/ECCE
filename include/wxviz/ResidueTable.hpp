/**
 * @file
 *
 *
 */

#ifndef RESIDUETABLE_HH
#define RESIDUETABLE_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Event.H"
#include "util/Listener.H"
  using namespace ecce;

#include "wxgui/TearableContentProvider.H"
#include "wxviz/MoleculeDataTable.H"


class GridAtomElement;
class wxConfig;

/**
 * Residue table view.
 */
class ResidueTable : public MoleculeDataTable,
                     public TearableContentProvider
{
   DECLARE_DYNAMIC_CLASS(ResidueTable)
   DECLARE_EVENT_TABLE()

   enum MenuIds {MAKERES=9300, RENUMBER, DELETERES, INSERTRES, APPENDRES,
                 VALIDATE, ADDH, ASSIGNRESATOMS, 
                 CONFIGURE, WRITESEG, WRITEFRAG};

  public:

    static const int ID_DIALOG_RESIDUE_TABLE;

    ResidueTable();

    ResidueTable(wxWindow* parent, 
              wxWindowID id = ID_DIALOG_RESIDUE_TABLE,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxTAB_TRAVERSAL);
    bool Create(wxWindow* parent, 
                wxWindowID id = ID_DIALOG_RESIDUE_TABLE,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTAB_TRAVERSAL);

    ~ResidueTable();

    virtual void refresh();
    int resIndexToRow(int resIndex);
    int rowToResIndex(int row);
    static bool sortResidueRows(const GridAtomElement *e1, const GridAtomElement *e2);
    void setResEditIndex(int resIndex, bool onOnly=true);

    // Listener virtual
    void eventMCB(const Event& event);

    virtual void restoreSettings(wxConfig * config);
    virtual void saveSettings(wxConfig * config);

  protected:

    void fillTable();
    void updateFragment(GridAtomElement *element);
    virtual void notifySelections(vector<GridAtomElement*> *selections, bool sendNotification=false);
    void setSelections();
    void OnSelectCell( wxGridEvent& event);
    void OnGridLabelLeftClick( wxGridEvent& event);

    virtual wxWindow* GetTearableContent();

    void OnMenuClick( wxCommandEvent& event );

    /** directory for save/restore of segment file save dialog */
    wxString p_segdir;

    /** directory for save/restore of fragment file save dialog */
    wxString p_fragdir;

    /** map residue indices to table indices to deal with the fact that the
       table does not display all residues. -1 = notdisplayed */
    vector<int> p_residuemap;
};

#endif

