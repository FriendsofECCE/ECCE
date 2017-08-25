#include "wxgui/ForwardMouseEventHandler.H"


BEGIN_EVENT_TABLE(ForwardMouseEventHandler, wxEvtHandler)
  EVT_MOUSE_EVENTS(ForwardMouseEventHandler::OnMouseEvent)
END_EVENT_TABLE()


ForwardMouseEventHandler::ForwardMouseEventHandler(wxEvtHandler * target)
  : wxEvtHandler()
  , p_target(target)
{
}


ForwardMouseEventHandler::~ForwardMouseEventHandler()
{
}


/**
 * Causes the mouse events to propagate.
 */
void ForwardMouseEventHandler::OnMouseEvent(wxMouseEvent& event)
{
  p_target->ProcessEvent(event);
}

