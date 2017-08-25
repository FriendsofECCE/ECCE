/**
 * @file
 *
 *
 */
#ifndef _WXSTATE_H_
#define _WXSTATE_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxStateGUI.C"
#endif

#include <string>
using std::string;
#include <vector>
using std::vector;


#include "dsm/ResourceDescriptor.H"

#include "wxgui/WxStateGUI.H"

class wxPen;
class wxBrush;
class Preferences;

/**
 * A class that draws a symbol representing and Ecce run state
 * such as running, complete...
 */
class WxState : public WxStateGUI
{    
public:
  WxState( );
  WxState( wxWindow* parent, wxWindowID id = SYMBOL_WXSTATEGUI_IDNAME,
           const wxPoint& pos = SYMBOL_WXSTATEGUI_POSITION,
           const wxSize& size = SYMBOL_WXSTATEGUI_SIZE,
           long style = SYMBOL_WXSTATEGUI_STYLE );
  virtual ~WxState( );

  void refresh();

  static void resetToSystemDefault();
  // Need to call this function when state preference message is received
  // then refresh the frame or panel
  static void resetToUserDefault();
  static void resetUserPreferences();

  static void finalize();

  void setRunState(int state);

  void setRunState(ResourceDescriptor::RUNSTATE state,
                   bool useSystemColor = false);
  ResourceDescriptor::RUNSTATE getRunState() const;

  bool canBeReset();
  string getPrefString();
  string getName(bool capital = false);
  string getColorString();

  static string getName(ResourceDescriptor::RUNSTATE state,
                        bool capital = false);
  static string getPrefString(int state);
  static void draw(wxDC & dc, ResourceDescriptor::RUNSTATE currentState,
                   bool useSystemColor = false, const wxRect& rect = wxRect());

protected:

  void OnPaint( wxPaintEvent& event );

  static void initializeColors();
  static void createBrushesAndPens();

  static void resetFromPreferences(const Preferences& prefs);

  /** The current run state to draw */
  ResourceDescriptor::RUNSTATE p_currentState;

  bool p_useSystemColor;

  /** The color string name - for editing purposes. */
  //string p_color;

  /** default colors for states - one per ResourceDescriptor::RUNSTATE */
  static vector<string> p_defaultColors;

  /** Brushes (fill color) one per ResourceDescriptor::RUNSTATE */
  static vector<wxBrush*> p_brushes[2];
  /** Pens (border color) one per ResourceDescriptor::RUNSTATE */
  static vector<wxPen*>   p_pens[2];

};

#endif
