#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/BrowserHelp.H"
#include "wxgui/WxHelpViewer.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxWindowUtils.H"
#include "util/LiveCount.H"

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
   LiveCount::add("ewxHelpHandler", 1);
   //  Per owner class and name, so a leaked handler names its window.
   p_countKey = "ewxHelpHandler:";
   if (win)
     p_countKey += std::string(wxString(win->GetClassInfo()->GetClassName())
                               .ToStdString()) + "/" +
                   win->GetName().ToStdString();
   for (char& c : p_countKey) if (c == ' ' || c == '=') c = '_';
   LiveCount::add(p_countKey.c_str(), 1);
}

ewxHelpHandler::~ewxHelpHandler()
{
   LiveCount::add("ewxHelpHandler", -1);
   LiveCount::add(p_countKey.c_str(), -1);
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
   WxHelpViewer::showKey(name);
}
