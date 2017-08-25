#ifndef EWXLISTVIEW_H_
#define EWXLISTVIEW_H_

#include <wx/listctrl.h>

#include "wxgui/ewxGenericValidator.H"
#include "wxgui/ewxStyledWindow.H"

class ewxListView : public wxListView, public ewxStyledWindow
{
  DECLARE_DYNAMIC_CLASS(ewxListView)

  public:
    // constructors
    ewxListView();
  
    ewxListView(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxLC_ICON,
            const wxValidator& validator = ewxDefaultValidator,
            const wxString& name = "ewxListView");

    virtual ~ewxListView();
  
    virtual bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxLC_ICON,
            const wxValidator& validator = ewxDefaultValidator,
            const wxString& name = "ewxListView");
};

#endif // EWXLISTVIEW_H_
