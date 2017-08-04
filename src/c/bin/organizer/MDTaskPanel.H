/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#ifndef MDTASKPANEL_HH
#define MDTASKPANEL_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "MDTaskPanelGUI.H"


class Resource;
class WxState;
class SessionContextPanel;

class MDTaskPanel : public MDTaskPanelGUI
{
  public:
    enum STATE
      {UNSELECTED, SELECTED, LOSTFOCUS};


    MDTaskPanel( );

    MDTaskPanel( Resource *task,
                 SessionContextPanel * sessionPanel,
                 wxWindow * parent,
                 wxWindowID id = SYMBOL_MDTASKPANELGUI_IDNAME,
                 const wxPoint& pos = SYMBOL_MDTASKPANELGUI_POSITION,
                 const wxSize &size = SYMBOL_MDTASKPANELGUI_SIZE,
                 long style = SYMBOL_MDTASKPANELGUI_STYLE );

    ~MDTaskPanel();

    Resource* getTask();
    WxState* getState();

    void setBackground(STATE state);

  protected:

    void createControls();

    // Event Handlers
    void connectEventHandlers();
    void OnPaint( wxPaintEvent& event );
    void OnLeftClick( wxMouseEvent& event);
    void OnLeftDClick( wxMouseEvent& event);
    void OnRightClick( wxMouseEvent& event );

  private:

    Resource *p_task;     /** A MDTask. */

    SessionContextPanel * p_parent;

    WxState *p_state; /** A WxState object. */

};

#endif

