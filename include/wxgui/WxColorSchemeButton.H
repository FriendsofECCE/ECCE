#ifndef _WXCOLORSCHEMEBUTTON_H_
#define _WXCOLORSCHEMEBUTTON_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxColorSchemeButton.C"
#endif

#include "wxgui/ewxPanel.H"


#define SYMBOL_WXSTATEGUI_SIZE wxSize(15, 15)
#define SYMBOL_WXSTATEGUI_POSITION wxDefaultPosition


#include <vector>
using std::vector;

#include <string>
using std::string;


const static int SCHEME_BUTTON_BASE = 102200;


class wxBoxSizer;


/*!
 * WxColorSchemeButton class declaration
 */
class WxColorSchemeButton: public ewxPanel
{
  DECLARE_DYNAMIC_CLASS( WxColorSchemeButton )
  DECLARE_EVENT_TABLE()

public:
  /// Constructors
  WxColorSchemeButton( );
  WxColorSchemeButton( wxWindow* parent,
                       wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxSize(40, 30),
                       long style = wxNO_BORDER|wxTAB_TRAVERSAL );
  
  virtual void OnPaint( wxPaintEvent& event );
  virtual void OnMouseLeftDown( wxMouseEvent& event );
  virtual void setSelected( bool selected );
  virtual void addColor( const wxColour & color );
  virtual void addColors( const vector<wxColour> & colors );
  virtual void addColors( const vector<string> & colors );
  virtual void addColors( const vector<char *> & colors );
  virtual void setColors( const vector<wxColour> & colors );
  virtual void setColors( const vector<string> & colors );
  virtual vector<wxColour> getColors() const;

protected:

  bool p_selected;
  wxBoxSizer * p_colorBandSizer;
  vector<wxColour> p_colors;

};

#endif
