#ifndef _EWXDISABLER_H
#define _EWXDISABLER_H

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/spinctrl.h"
#endif

/**
 * Class to catch and not pass on events.
 * Used to disable buttons action without graying them out.
 *
 * Each widget must create an instance of this in its constructor
 * and use PushEventHandler.  It must also implement the virtual
 * setCustomDisabledStyle.  See ewxButton.
 *
 */
class ewxDisabler : public wxEvtHandler
{
   public:

      ewxDisabler();
      ~ewxDisabler();

      void setEnabled(bool);
      bool getEnabled() const;

      void filter( wxCommandEvent& event );
      void filterMouse( wxMouseEvent& event );
      void filterKey( wxKeyEvent& event );
      void filterSpin(wxSpinEvent& event);

   private:
      DECLARE_EVENT_TABLE()

      bool p_isEnabled;


};

#endif
