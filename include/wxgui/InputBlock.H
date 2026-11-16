/**
 * @file
 *
 * No input while a start or a submit runs, without greying the controls
 * (#1088 items 5, 6).
 */
#ifndef INPUTBLOCK_HH
#define INPUTBLOCK_HH

#include "wx/app.h"
#include "wx/dialog.h"
#include "wx/event.h"
#include "wx/toplevel.h"
#include "wx/window.h"

#include "util/MqttLink.H"

/**
 * In one-process mode (ECCE_ONE_PROCESS) swallows mouse and key events of
 * every window but dialogs, so nothing is disabled and nothing changes
 * colour while the busy cursor shows the wait.  Otherwise it is the
 * wxWindowDisabler it replaces: the separate windows of the gateway mode
 * keep their old behaviour.
 */
class InputBlock : public wxEventFilter
{
public:
  explicit InputBlock(wxWindow *except = 0)
    : p_disabler(0)
  {
    if (MqttLink::localDelivery()) wxTheApp->AddFilter(this);
    else p_disabler = new wxWindowDisabler(except);
  }

  ~InputBlock() override
  {
    if (p_disabler) delete p_disabler;
    else wxTheApp->RemoveFilter(this);
  }

  int FilterEvent(wxEvent& ev) override
  {
    const wxEventType t = ev.GetEventType();
    const bool input = ev.IsKindOf(wxCLASSINFO(wxMouseEvent)) ||
                       ev.IsKindOf(wxCLASSINFO(wxKeyEvent));
    if (!input || t == wxEVT_MOTION || t == wxEVT_ENTER_WINDOW ||
        t == wxEVT_LEAVE_WINDOW)
      return Event_Skip;
    wxWindow *w = wxDynamicCast(ev.GetEventObject(), wxWindow);
    // A prompt opened meanwhile must stay usable.
    if (w && wxDynamicCast(wxGetTopLevelParent(w), wxDialog)) return Event_Skip;
    return Event_Processed;
  }

private:
  wxWindowDisabler *p_disabler;
};

#endif
