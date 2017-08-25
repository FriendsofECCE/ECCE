#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/BrowserHelp.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxWindowUtils.H"

#define ID_MENU_HELP 50000
#define ID_MENU_FEEDBACK 50001

BEGIN_EVENT_TABLE(ewxHelpHandler, wxEvtHandler)
   //EVT_MENU(ID_MENU_HELP, ewxHelpHandler::OnHelpClicked)
   EVT_KEY_UP(ewxHelpHandler::OnKeyUp)
END_EVENT_TABLE()


ewxHelpHandler::ewxHelpHandler(wxWindow *win)
{
   // Our help keys are always on the top window, not its subcontrols
   p_win = ewxWindowUtils::getTopWindow(win);
}

ewxHelpHandler::~ewxHelpHandler()
{
   p_win = 0;
}

void ewxHelpHandler::OnKeyUp(wxKeyEvent& event)
{
   int code = event.GetKeyCode();
   if (code == WXK_HELP || code == WXK_F1) {
     showHelp();
   }
   event.Skip();
}

/**
 * My attempt to connect help and feedback automatically didn't work...
 */
void ewxHelpHandler::OnHelpClicked(wxCommandEvent& event)
{
  //   cout << "help clicked " << endl;
   showHelp();
   event.Skip();

}

void ewxHelpHandler::showHelp() const
{
   string name = "HomeFallback";
   if (p_win) {
      name = p_win->GetName();
   }
   //   cout << p_win->GetTitle()<<endl;
   //   cout << "help key " << name << endl;
   BrowserHelp help;
   help.showPage(help.URL(name));
}
