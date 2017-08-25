#ifndef _EWXBUTTON_H
#define _EWXBUTTON_H

#include "wx/button.h"
#include "wx/colour.h"
#include "wxgui/ewxStyledWindow.H"


class ewxDisabler;


class ewxButton : public wxButton, public ewxStyledWindow
{
   public:
      // constructors
      ewxButton();
      ewxButton(wxWindow* parent, wxWindowID id, const wxString& caption,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = "ewxButton");
      bool Create(wxWindow* parent, wxWindowID id, const wxString& caption,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = 0,
                  const wxValidator& validator = wxDefaultValidator,
                  const wxString& name = "ewxButton");
      virtual ~ewxButton();
      //bool ProcessEvent(wxEvent& event);

      void SetForegroundColour(wxColour& color);

      virtual void setCustomDisabledStyle(bool disabled);

   protected:
      ewxDisabler *p_disabler;
      wxColour p_fg;
};

#endif
