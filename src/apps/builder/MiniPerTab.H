#ifndef _MINIPERTABGUI_H_
#define _MINIPERTABGUI_H_

#include <string>
using std::string;
#include <vector>
using std::vector;

#include "util/Listener.H"
#include "util/Event.H"
using namespace ecce;

#include "wxviz/WxVizTool.H"

class wxBoxSizer;
class wxGridBagSizer;
class wxToggleButton;
class ImageDropDown;
class wxConfig;

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_MINIPERTABGUI_TITLE _("Dialog")
#define SYMBOL_MINIPERTABGUI_IDNAME ID_DIALOG
////@end control identifiers

enum {
  NUM_ELT_BTN = 12,
  ID_ELT_BASE = 8000,  // Need to reserve at least a range of 12
  ID_SHOW_TABLE = 8100,
  ID_BOND_SEL,
  ID_SHAPE_SEL
};
  

class MiniPerTab : public ewxPanel,
                   public WxVizTool,
                   public Listener
{    
DECLARE_DYNAMIC_CLASS( MiniPerTab)
DECLARE_EVENT_TABLE()

public:
  MiniPerTab( );
  MiniPerTab( wxWindow* parent, wxWindowID id = -1,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxSUNKEN_BORDER );
  ~MiniPerTab();
  void setElements();
  void addRow();

  vector<string> getElements();

  //virtual wxWindow *getExtensionPanel();

  // Listener virtual(s)
  void eventMCB(const Event& event);

protected:
  //void OnOpenCloseClick( wxCommandEvent& event );

  /// Creation
  bool Create( wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxSUNKEN_BORDER );
  /* common interface for initializing and saving preferences.  */
  virtual void restoreSettings(wxConfig * config);
  virtual void saveSettings(wxConfig * config);

  void OnAddhClick(wxCommandEvent &event);
  void OnRemovehClick(wxCommandEvent &event);
  void OnCleanClick(wxCommandEvent &event);
  void OnRemoveClick(wxCommandEvent &event);
  void OnMoreClick(wxCommandEvent &event);
  void OnBondSel(wxCommandEvent &event);
  void OnShapeSel(wxCommandEvent &event);
  void OnElementClick(wxCommandEvent &event);
  void onElementChanged(string elt);
  //  void OnChildFocus(wxChildFocusEvent &event);
  /// Creates the controls and sizers
  void CreateControls();

  /// Should we show tooltips?
  static bool ShowToolTips();

  ewxPanel *p_extensionPanel;
  wxSizer* p_sizer;
  wxGridBagSizer *p_gridSizer;

  string p_elements[NUM_ELT_BTN];
  wxToggleButton * p_eltBtns[NUM_ELT_BTN];
  int p_replaceIndex;

  ImageDropDown * p_bonds;
  ImageDropDown * p_shapes;
};

#endif
