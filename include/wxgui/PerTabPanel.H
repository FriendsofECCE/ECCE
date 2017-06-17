/**
 * @file
 *
 * Periodic table panel class.
 *
 *
 */

#ifndef _PERTABPANEL_H_
#define _PERTABPANEL_H_

#include "wx/event.h"
#include "wx/wxprec.h"
#include "wx/gbsizer.h"

// For compilers that support precompilation, includes "wx/wx.h".
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <vector>
  using std::vector;

#include "util/Listener.H"
using namespace ecce;

#include "wxgui/ewxColor.H"
#include "wxgui/ewxPanel.H"

class ewxMenu;
class PerTabPanel;
class PTDataFrame;
class Preferences;
class TPerTab;

// Default parameters for the GUI components
#define ELEMENTBUTTON_STYLE  wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE
#define ELEMENTBUTTON_SIZE wxDefaultSize
#define ELEMENTBUTTON_POSITION wxDefaultPosition

#define PERTABPANEL_STYLE wxNO_BORDER|wxTAB_TRAVERSAL
#define PERTABPANEL_SIZE wxDefaultSize
#define PERTABPANEL_POSITION wxDefaultPosition

// ID's
enum { ID_ELEMENTBASE = 2000,
       ID_PANEL_PERTAB = 3000,
       ID_ITEM_PHYS,     ID_ITEM_METAL,
       ID_ITEM_DEFAULT,  ID_ITEM_USER,
       ID_ITEM_SHOWNUM,  ID_ITEM_SHOWDATA,
       ID_ITEM_FEEDBACK};


// ElementButton Class
class ElementButton: public ewxPanel
{

  DECLARE_EVENT_TABLE()

public:

  virtual ~ElementButton();
  ElementButton(int atomicNum, PerTabPanel* parent, wxWindowID id,
                const wxPoint& pos = ELEMENTBUTTON_POSITION,
                const wxSize& size = ELEMENTBUTTON_SIZE,
                long style = ELEMENTBUTTON_STYLE);

  void setStatus(bool isSunken);
  bool getStatus() const;
  void clearSizeInfo();

  void OnMouseLeftDown( wxMouseEvent& event );
  void OnMouseLeftUp( wxMouseEvent& event );
  void OnContextMenu( wxContextMenuEvent& event );
  void OnMouseEnterWindow( wxMouseEvent& event );
  void OnMouseLeaveWindow( wxMouseEvent& event );
  void OnPaint( wxPaintEvent& event );
//  void OnEraseBackground( wxEraseEvent& event);

protected:

  ElementButton();

  int p_atomicNum;
  string p_atomicNumStr;
  string p_symbol;
  wxCoord p_numWidth;
  wxCoord p_numHeight;
  wxCoord p_symbolWidth;
  wxCoord p_symbolHeight;

  PerTabPanel * p_parent;

  bool p_isSunken;
  bool p_drawTriangle;
};



// PerTabPanel Class
class PerTabPanel: public ewxPanel,
                   public Listener
{

DECLARE_EVENT_TABLE()

public:

  PerTabPanel();
  virtual ~PerTabPanel();
  PerTabPanel(wxWindow* parent, bool isMultiSelect = true,
              int colorScheme = ID_ITEM_DEFAULT,
              bool showNumber = true,
              bool showGhost = true,
              bool isMini = false,
              wxWindowID id = ID_PANEL_PERTAB,
              const wxPoint& pos = PERTABPANEL_POSITION,
              const wxSize& size = PERTABPANEL_SIZE,
              long style = PERTABPANEL_STYLE );

  PerTabPanel(wxWindow* parent,
              wxWindowID id = ID_PANEL_PERTAB,
              const wxPoint& pos = PERTABPANEL_POSITION,
              const wxSize& size = PERTABPANEL_SIZE,
              long style = PERTABPANEL_STYLE );

  void createControls(bool isMultiSelect = true,
                      int colorScheme = ID_ITEM_DEFAULT,
                      bool showNumber = true,
                      bool showGhost = true,
                      bool isMini = false);

  void close();
  ewxMenu * getLongViewMenu() const;
  ewxMenu * getDataMenu() const;
  wxColour getElementColor(int atomicNum, bool isSunken);
  wxColour getContextColor(bool isSunken);
  TPerTab * getTPerTab() const;
  TPerTab * getRefTPerTab() const;

  wxColour getDefaultElementColor(int atomicNum) const;
  wxColour getUserElementColor(int atomicNum) const;
  double getDefaultCovRad(int atomicNum) const;
  double getDefaultVdwRad(int atomicNum) const;
  double getDefaultBft(int atomicNum) const;
  double getUserCovRad(int atomicNum) const;
  double getUserVdwRad(int atomicNum) const;
  double getUserBft(int atomicNum) const;

  void resetElementRadii(int atomicNum);
  void setElementRadii(int atomicNum, double covRadInput,
                       double vdwRadInput, double bftInput);

  void resetAllElementColor();
  void resetAllElementRadii();

  bool isShowNumber();
  bool isMini();
  bool isShowMark(int atomicNum);
  void showMark(int atomicNum, bool show=true);
  void showMarks(vector<const char*> *symbols);
  void resetMarks();

  void setCurrentElement(int atomicNum);
  void selectElement(int atomicNum, bool isSelected);
  vector<string> * getSelectionSymbols();
  void selectElement(int atomicNum);
  void clearSelection();


  void OnContextMenu( wxContextMenuEvent& event );
  void OnMenuClick(wxCommandEvent& event);

  void recolorTable();
  void saveSettings(Preferences& prefs);
  void restoreSettings(const Preferences& prefs);

  void resetPreference();

  void setElementColor(int atomicNum, const ewxColor& color);
  void resetUserPreferences();
  void mergeUserElementPreferences();
  void writeRadiiOverrides();

  bool colorChanged();
  bool radiiChanged();

  void notifyPrefChanged();
  void fireSelectionChanged(int atomicNumber);

  void eventMCB(const Event& event);

protected:

  struct ElementInfo{
    bool cornerMark;
    string defaultColor;
    string userColor;
    double covalentRadius;
    double vanderWaalsRadius;
    double bondFormationTolerance;
  };

  vector<ElementInfo *> p_elementInfo;

  bool equal( double a, double b);
  wxColour offsetColor(wxColour color, float offset);

  TPerTab * p_tPerTab;
  TPerTab * p_refTPerTab;
  int p_numAtoms;
  
  int p_colorScheme;
  bool p_showNumber;
  bool p_isMini;
  bool p_isMultiSelect;
  int p_selectCount;
  int p_currentElement;

  /*  
  bool * p_cornerMark;
  string * p_defaultColors;
  string * p_userColors;

  double * p_covalentRadius;
  double * p_vwr;
  double * p_covalentRadiusTolerance;
  */

  wxBoxSizer * p_topSizer;
  wxGridBagSizer * p_elementSizer;
  ElementButton ** p_elements;
  ewxMenu * p_longViewMenu;
  ewxMenu * p_shortViewMenu;
  ewxMenu * p_dataMenu;
  PTDataFrame * p_dataFrame;

  Preferences *p_prefs;
  
  static string p_prefFile;
  
  static wxColour p_physColor[4];
  static wxColour p_metalColor[3];
  static wxColour p_contextColor;
};


#endif
// _PERTABPANEL_H_
