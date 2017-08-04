/**
 * @file
 * @author Lisong Sun
 *
 * State Button Class
 *
 * This is a homemade widget that mimic a button but can be turned off
 * as flat if needed. It also got customized drawing on it using WxState.
 * It is used in GatewayPref dialog to set or reset state colors.
 *
 */

#ifndef _STATEBUTTON_H_
#define _STATEBUTTON_H_

#include "wx/wxprec.h"

// For compilers that support precompilation, includes "wx/wx.h".
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <string.h>
#include "dsm/ResourceDescriptor.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/WxState.H"

#define STATEBUTTON_STYLE  wxNO_BORDER|wxTAB_TRAVERSAL
#define STATEBUTTON_SIZE wxSize(50,30)
#define STATEBUTTON_POSITION wxDefaultPosition
#define ID_STATEBUTTON wxID_ANY


class WxState;
class GatewayPrefs;

class StateButton: public ewxPanel {

  DECLARE_EVENT_TABLE()

public:

  enum BUTTONSTATUS { FLATBUTTON, SUNKENBUTTON, RAISENBUTTON };

  StateButton(ResourceDescriptor::RUNSTATE state, bool isDefault,
              GatewayPrefs* parent, wxWindowID id = ID_STATEBUTTON,
              const wxPoint& pos = STATEBUTTON_POSITION,
              const wxSize& size = STATEBUTTON_SIZE,
              long style = STATEBUTTON_STYLE);
  ~StateButton();

  void SetSibling(StateButton * sibling);
  void SetStatus(BUTTONSTATUS status);
  bool CanBeReset();

protected:

  StateButton();

  void Create(ResourceDescriptor::RUNSTATE state, bool isDefault,
              GatewayPrefs* parent, wxWindowID id = ID_STATEBUTTON,
              const wxPoint& pos = STATEBUTTON_POSITION,
              const wxSize& size = STATEBUTTON_SIZE,
              long style = STATEBUTTON_STYLE);


  void OnMouseLeftDown( wxMouseEvent& event );
  void OnMouseLeftUp( wxMouseEvent& event );
  void OnMouseEnterWindow( wxMouseEvent& event );
  void OnMouseLeaveWindow( wxMouseEvent& event );
  void OnPaint( wxPaintEvent& event );

  void ToolActive();
  bool Enable(bool enable);
  void SiblingClicked();

  GatewayPrefs * p_parent;
  StateButton * p_sibling;
  WxState * p_stateIcon;
  ResourceDescriptor::RUNSTATE p_state;
  BUTTONSTATUS p_butStatus;
  bool p_isDefault;
};

#endif
