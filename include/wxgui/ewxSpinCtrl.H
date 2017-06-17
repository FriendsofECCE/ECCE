#ifndef _EWXSPINCTRL_H
#define _EWXSPINCTRL_H

#include "wx/spinctrl.h"
#include "wxgui/ewxStyledWindow.H"

class ewxDisabler;

class ewxSpinCtrl : public wxSpinCtrl, public ewxStyledWindow
{
    public:
        // constructors
        ewxSpinCtrl();
        ewxSpinCtrl(wxWindow* parent, wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSP_ARROW_KEYS, int min = 0, int max = 100,
                int initial = 0, const wxString& name = "ewxSpinCtrl");
        bool Create(wxWindow* parent, wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSP_ARROW_KEYS, int min = 0, int max = 100,
                int initial = 0, const wxString& name = "ewxSpinCtrl");
        virtual ~ewxSpinCtrl();
        void setCustomDisabledStyle(bool enabled);

    protected:
        ewxDisabler *p_disabler;

    private:
        void Init();
};

#endif
