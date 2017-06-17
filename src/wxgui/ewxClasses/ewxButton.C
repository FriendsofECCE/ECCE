#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxButton.H"
#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxHelpHandler.H"



class ewxHelpBtnHandler : public ewxHelpHandler
{
   public:
       ewxHelpBtnHandler(wxWindow *win) : ewxHelpHandler(win)  {;}

       ~ewxHelpBtnHandler()  {;}

       void OnHelp( wxMouseEvent& event)
       {
          showHelp();
          event.Skip();
       }


   protected:
       ewxHelpBtnHandler() {;}

       DECLARE_EVENT_TABLE()

};

BEGIN_EVENT_TABLE(ewxHelpBtnHandler, ewxHelpHandler)
    EVT_LEFT_UP(ewxHelpBtnHandler::OnHelp)
END_EVENT_TABLE()


ewxButton::ewxButton()
  : wxButton(),
    ewxStyledWindow()
{
   p_fg = wxColour(1,1,1);
}


ewxButton::ewxButton(wxWindow* parent, wxWindowID id, const wxString& caption,
                     const wxPoint& pos, const wxSize& size, long style,
                     const wxValidator& validator, const wxString& name)
  : wxButton(),
    ewxStyledWindow()
{
   p_fg = wxColour(1,1,1);
   Create(parent, id, caption, pos, size, style, validator, name);
}


bool ewxButton::Create(wxWindow* parent, wxWindowID id,
                       const wxString& caption, const wxPoint& pos,
                       const wxSize& size, long style,
                       const wxValidator& validator, const wxString& name)
{
   if (!wxButton::Create(parent, id, caption, pos, size, style, validator,
                         name)) {
     wxFAIL_MSG( wxT("ewxButton creation failed") );
     return false;
   }

   if (caption == "Help") {
      PushEventHandler(new ewxHelpBtnHandler(this));
   } else {
      PushEventHandler(new ewxHelpHandler(this));
   }
   p_disabler = new ewxDisabler();
   PushEventHandler(p_disabler);

   setStyles(this);

   return true;
}


ewxButton::~ewxButton()
{
  // Remove and delete help handler and disabler
  PopEventHandler(true);
  PopEventHandler(true);
}

void ewxButton::SetForegroundColour(wxColour& color)
{
   wxButton::SetForegroundColour(color);
   p_fg = color;
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxButton:: setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
   if (enabled) {
      wxButton::SetForegroundColour(p_fg);
   } else {
      wxButton::SetForegroundColour(wxColour(100,100,100));
   }
}
