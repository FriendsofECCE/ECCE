/**
 * @file
 * @author Ken Swanson
 *
 */


#ifndef WXTIMESPANEDIT_HH
#define WXTIMESPANEDIT_HH

#include <string>
    using std::string;

#include "wx/event.h"
#include "wx/frame.h"
#include "wx/spinctrl.h"

#include "wxgui/ewxPanel.H"

#include "util/StringConverter.H"

class ewxSpinCtrl;
class ewxStaticText;

#define ID_WXTIMESPANEDIT_BASE 62600

#define ID_STATIC_WXTIMESPANEDIT_CAPTION        (ID_WXTIMESPANEDIT_BASE + 18)
#define ID_SPINCTRL_WXTIMESPANEDIT_DAYS         (ID_WXTIMESPANEDIT_BASE + 31)
#define ID_SPINCTRL_WXTIMESPANEDIT_HOURS        (ID_WXTIMESPANEDIT_BASE + 39)
#define ID_SPINCTRL_WXTIMESPANEDIT_MINUTES      (ID_WXTIMESPANEDIT_BASE + 40)
#define ID_STATIC_WXTIMESPANEDIT_DAYSLABEL      (ID_WXTIMESPANEDIT_BASE + 19)
#define ID_STATIC_WXTIMESPANEDIT_HOURSLABEL     (ID_WXTIMESPANEDIT_BASE + 37)
#define ID_STATIC_WXTIMESPANEDIT_MINUTESLABEL   (ID_WXTIMESPANEDIT_BASE + 38)
#define ID_STATIC_WXTIMESPANEDIT_MAXVALUELABEL  (ID_WXTIMESPANEDIT_BASE + 41)
#define ID_STATIC_WXTIMESPANEDIT_MAXVALUE       (ID_WXTIMESPANEDIT_BASE + 32)

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(ewxEVT_TIMESPAN_VALUE_CHANGED, ID_WXTIMESPANEDIT_BASE + 50)
END_DECLARE_EVENT_TYPES()


class WxTimeSpanEdit: public ewxPanel
{
    DECLARE_CLASS( WxTimeSpanEdit )
    DECLARE_EVENT_TABLE()

    public:
        //  Constructor(s), Destructor
        WxTimeSpanEdit(     wxWindow* parent,
                            wxWindowID id,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                            long style = 0,
                      const wxString& name = wxPanelNameStr);

        ~WxTimeSpanEdit();

        static long calcTimeMinutes(int days, int hrs, int mins);

        wxSize DoGetBestSize() const;
        void windowResizeCB(wxSizeEvent& event);

        void timeSpanDaysSpinCtrlUpdatedCB(wxSpinEvent& event);
        void timeSpanHoursSpinCtrlUpdatedCB(wxSpinEvent& event);
        void timeSpanMinutesSpinCtrlUpdatedCB(wxSpinEvent& event);

        long getMaxValue();
        void setMaxValue(long vmax);

        long getValue();
        void setValue(long v);

        bool getRangeEnforced();
        void setRangeEnforced(bool enfrc);

        bool getEnabled();
        void setEnabled(bool enbld);

        static string formatTime(long value);
        static string formatTime(int days, int hrs, int mins);
        static string toString(long value);

    protected:

    private:
        static void splitTimeMinutes(long minutes, int *days, int *hrs, int *mins);
        void createControls();
        void fireValueChanged();

        long                 p_value;
        long                 p_maxValue;
        bool                 p_enbld;
        bool                 p_rangeEnfrc;

        bool                 p_inCtrlUpdate;

        ewxSpinCtrl         *p_daysSpinCtrl;
        ewxSpinCtrl         *p_hoursSpinCtrl;
        ewxSpinCtrl         *p_minutesSpinCtrl;

        ewxStaticText       *p_daysLabelStaticText;
        ewxStaticText       *p_hoursLabelStaticText;
        ewxStaticText       *p_minutesLabelStaticText;
};

#endif  //  WXTIMESPANEDIT_HH
