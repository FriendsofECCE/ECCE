/**
 * @file
 * @author Ken Swanson
 *
 */


#ifndef WXPARAMETEREDIT_HH
#define WXPARAMETEREDIT_HH

#include "wx/event.h"
#include "wx/frame.h"
#include "wx/spinctrl.h"

#include "wxgui/ewxPanel.H"

#include "util/StringConverter.H"

class ewxDisabler;
class ewxSpinCtrl;
class ewxStaticText;

#define ID_WXPARAMETEREDIT_BASE                 127800

#define ID_STATIC_WXPARAMETEREDIT_CAPTION       (ID_WXPARAMETEREDIT_BASE + 1)
#define ID_SPINCTRL_WXPARAMETEREDIT_VALUE       (ID_WXPARAMETEREDIT_BASE + 2)
#define ID_STATIC_WXPARAMETEREDIT_RANGELABEL    (ID_WXPARAMETEREDIT_BASE + 3)
#define ID_STATIC_WXPARAMETEREDIT_UNITSLABEL    (ID_WXPARAMETEREDIT_BASE + 4)

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(ewxEVT_PARAMETER_VALUE_CHANGED, ID_WXPARAMETEREDIT_BASE + 50)
END_DECLARE_EVENT_TYPES()


class WxParameterEdit:  public ewxPanel
{
    DECLARE_CLASS(WxParameterEdit)
    DECLARE_EVENT_TABLE()

    public:
        //  Constructor(s), Destructor
        WxParameterEdit();
        WxParameterEdit(      wxWindow* parent,
                              wxWindowID id,
                        const   wxPoint& pos = wxDefaultPosition,
                        const   wxSize& size = wxDefaultSize,
                                  long style = wxTAB_TRAVERSAL,
                        const wxString& name = wxPanelNameStr);
        bool Create(wxWindow* parent,
                    wxWindowID id,
                    const   wxPoint& pos = wxDefaultPosition,
                    const   wxSize& size = wxDefaultSize,
                    long style = wxTAB_TRAVERSAL,
                    const wxString& name = wxPanelNameStr);
        ~WxParameterEdit();

        wxSize          DoGetBestSize() const;


        void            windowResizeCB(wxSizeEvent& event);

        bool            getRangeVisible();
        void            setRangeVisible(bool vsbl);

        int             getValue();
        int             getMinimum();
        int             getMaximum();

        wxSize          GetMinSize() const;

        void            setMaximum(int vmax);
        void            setMinimum(int vmin);

        string          getUnitsLabel();
        void            setUnitsLabel(string units);

        bool            getUnitsVisible();
        void            setUnitsVisible(bool visible);

        void            setValue(int value);
        void            setRange(int vmin, int vmax);
        bool            setEnabled(bool enbld);

        void            valueSpinCtrlUpdatedCB(wxSpinEvent& event);
        void            valueSpinCtrlValidateCB(wxCommandEvent& event);

        void            setCustomDisabledStyle(bool enabled);

    protected:
        ewxDisabler     *p_disabler;


    private:
        ewxSpinCtrl     *p_valueSpinCtrl;
        ewxStaticText   *p_unitsStaticText;
        ewxStaticText   *p_rangeStaticText;

        bool            p_captionVisible;
        bool            p_unitsVisible;
        bool            p_rangeVisible;

        void            createControls();
        void            fireValueEdited();
};

#endif      //  WXPARAMETEREDIT_HH
