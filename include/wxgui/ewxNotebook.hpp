#ifndef _EWXNOTEBOOK_H
#define _EWXNOTEBOOK_H

#include "wx/notebook.h"
#include "wxgui/ewxStyledWindow.H"

class ewxNotebook : public wxNotebook, public ewxStyledWindow
{
  public:
    ewxNotebook();

    ewxNotebook(wxWindow* parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0, const wxString& name = "ewxNotebook");

    bool Create(wxWindow* parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0, const wxString& name = "ewxNotebook");

    wxSize computeSize();
};

#endif
