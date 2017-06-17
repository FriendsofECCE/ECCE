#ifndef _HELPHANDLER_H
#define _HELPHANDLER_H

#include "wx/event.h"


/**
 * A very small event handler whose sole purpose is to capture the
 * Help or F1 key and invoke help for the frame or dialog.
 * All widgets should add this to their event handler list with
 * PushEventHandler.  In addition, controls should have the style
 * wxWANTS_CHARS speciefied.  
 *
 * Note: the above requirements are met by ewx* classes.
 */
class ewxHelpHandler : public wxEvtHandler
{
   public:
      ewxHelpHandler(wxWindow *win);
      virtual ~ewxHelpHandler();


   protected:
      ewxHelpHandler();
      void showHelp() const;

      wxWindow *getTopWindow(wxWindow *win) const;

      void OnKeyUp(wxKeyEvent& event);
      void OnHelpClicked(wxCommandEvent& event);

      wxWindow *p_win;
      DECLARE_EVENT_TABLE()
};

#endif
