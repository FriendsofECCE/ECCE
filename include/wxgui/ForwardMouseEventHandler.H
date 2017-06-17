#ifndef FORWARDMOUSEEVENTHANDLER_H_
#define FORWARDMOUSEEVENTHANDLER_H_

#include <wx/event.h>

class ForwardMouseEventHandler : public wxEvtHandler
{
  DECLARE_EVENT_TABLE()

  public:
    ForwardMouseEventHandler(wxEvtHandler * target);
    ~ForwardMouseEventHandler();

    void OnMouseEvent( wxMouseEvent& event );

  protected:
    wxEvtHandler * p_target;
};

#endif // FORWARDMOUSEEVENTHANDLER_H_
