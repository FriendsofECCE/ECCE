/**
 * @file
 * @author Ken Swanson
 *
 */


#include <algorithm>
    using std::min;
    using std::max;

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStaticText.H"

#include "wxgui/WxTimeSpanEdit.H"


#define MINUTES_PER_HOUR    60
#define HOURS_PER_DAY       24
#define MINUTES_PER_DAY     (HOURS_PER_DAY * MINUTES_PER_HOUR) // = 1440

IMPLEMENT_CLASS(WxTimeSpanEdit, ewxPanel)

BEGIN_EVENT_TABLE(WxTimeSpanEdit, ewxPanel)
    EVT_SIZE(WxTimeSpanEdit::windowResizeCB)
    EVT_SPINCTRL( ID_SPINCTRL_WXTIMESPANEDIT_DAYS, WxTimeSpanEdit::timeSpanDaysSpinCtrlUpdatedCB )
    EVT_SPINCTRL( ID_SPINCTRL_WXTIMESPANEDIT_HOURS, WxTimeSpanEdit::timeSpanHoursSpinCtrlUpdatedCB )
    EVT_SPINCTRL( ID_SPINCTRL_WXTIMESPANEDIT_MINUTES, WxTimeSpanEdit::timeSpanMinutesSpinCtrlUpdatedCB )
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(ewxEVT_TIMESPAN_VALUE_CHANGED)

string dfltcptns[3] = { "minutes", "hours", "days" };


WxTimeSpanEdit::WxTimeSpanEdit(      wxWindow* parent,
                                     wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                                        long style,
                               const wxString& name)
        : ewxPanel(parent, id, pos, size, style, name)
{
    this->createControls();
    this->setMaxValue(21620);
    this->setValue(0);
    this->setRangeEnforced(true);
}


void WxTimeSpanEdit::createControls()
{
    wxFlexGridSizer* szrMain = new wxFlexGridSizer(2, 3, 0, 5);

    p_daysSpinCtrl = new ewxSpinCtrl(this, ID_SPINCTRL_WXTIMESPANEDIT_DAYS, _("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 14, 0 );
    szrMain->Add(p_daysSpinCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

    p_hoursSpinCtrl = new ewxSpinCtrl(this, ID_SPINCTRL_WXTIMESPANEDIT_HOURS, _("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 23, 0 );
    szrMain->Add(p_hoursSpinCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

    p_minutesSpinCtrl = new ewxSpinCtrl(this, ID_SPINCTRL_WXTIMESPANEDIT_MINUTES, _("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 59, 0 );
    szrMain->Add(p_minutesSpinCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

    p_daysLabelStaticText = new ewxStaticText( this, ID_STATIC_WXTIMESPANEDIT_DAYSLABEL, _("days"), wxDefaultPosition, wxDefaultSize, 0 );
    szrMain->Add(p_daysLabelStaticText, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxADJUST_MINSIZE, 0);

    p_hoursLabelStaticText = new ewxStaticText( this, ID_STATIC_WXTIMESPANEDIT_HOURSLABEL, _("hours"), wxDefaultPosition, wxDefaultSize, 0 );
    szrMain->Add(p_hoursLabelStaticText, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxADJUST_MINSIZE, 0);

    p_minutesLabelStaticText = new ewxStaticText( this, ID_STATIC_WXTIMESPANEDIT_MINUTESLABEL, _("minutes"), wxDefaultPosition, wxDefaultSize, 0 );
    szrMain->Add(p_minutesLabelStaticText, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxADJUST_MINSIZE, 0);

    this->SetSizer(szrMain);
    this->Layout();
    this->Fit();
}


WxTimeSpanEdit::~WxTimeSpanEdit()
{
}


wxSize WxTimeSpanEdit::DoGetBestSize() const
{
    return (this->GetSizer()->GetMinSize());
}


void WxTimeSpanEdit::windowResizeCB(wxSizeEvent& event)
{
    this->Layout();
}


void WxTimeSpanEdit::timeSpanDaysSpinCtrlUpdatedCB(wxSpinEvent& event)
{
    long timeRmng;
    int maxHours, maxMinutes;
    int days, hours;

    if (!p_inCtrlUpdate)
    {
        if (p_rangeEnfrc)
        {
            p_inCtrlUpdate = true;

            days = p_daysSpinCtrl->GetValue();
            timeRmng = p_maxValue - (days * MINUTES_PER_DAY);
            maxHours = min((timeRmng / MINUTES_PER_HOUR), (long)(HOURS_PER_DAY - 1));
            p_hoursSpinCtrl->SetRange(0, maxHours);     //  Should adjust the value as needed

            hours = p_hoursSpinCtrl->GetValue();
            timeRmng = timeRmng - (hours * MINUTES_PER_HOUR);
            maxMinutes = min(timeRmng, (long)(MINUTES_PER_HOUR - 1));
            p_minutesSpinCtrl->SetRange(0, maxMinutes);  //  Should adjust the value as needed

            p_inCtrlUpdate = false;
        }

        this->fireValueChanged();
    }
}


void WxTimeSpanEdit::timeSpanHoursSpinCtrlUpdatedCB(wxSpinEvent& event)
{
    long timeRmng;
    int days, hours;
    int maxMinutes;

    if (!p_inCtrlUpdate)
    {
        if (p_rangeEnfrc)
        {
            p_inCtrlUpdate = true;

            days = p_daysSpinCtrl->GetValue();
            timeRmng = p_maxValue - (days * MINUTES_PER_DAY);

            hours = p_hoursSpinCtrl->GetValue();
            timeRmng = timeRmng - (hours * MINUTES_PER_HOUR);
            maxMinutes = min(timeRmng, (long)(MINUTES_PER_HOUR - 1));
            p_minutesSpinCtrl->SetRange(0, maxMinutes);  //  Should adjust the value as needed

            p_inCtrlUpdate = false;
        }

        this->fireValueChanged();
    }
}


void WxTimeSpanEdit::fireValueChanged()
{
    wxCommandEvent event(ewxEVT_TIMESPAN_VALUE_CHANGED, this->GetId());

    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent( event );
}


void WxTimeSpanEdit::timeSpanMinutesSpinCtrlUpdatedCB(wxSpinEvent& event)
{
    if (!p_inCtrlUpdate)
        this->fireValueChanged();
}


void WxTimeSpanEdit::splitTimeMinutes(long value, int *days, int *hrs, int *mins)
{
    *days = value / (MINUTES_PER_HOUR * HOURS_PER_DAY);

    value = value % (MINUTES_PER_HOUR * HOURS_PER_DAY);
    *hrs = value / MINUTES_PER_HOUR;

    *mins = (value % MINUTES_PER_HOUR);
}


long WxTimeSpanEdit::calcTimeMinutes(int days, int hrs, int mins)
{
    return (mins + (hrs + (days * HOURS_PER_DAY)) * MINUTES_PER_HOUR);
}


string WxTimeSpanEdit::formatTime(long value)
{
    int days, hours, minutes;

    splitTimeMinutes(value, &days, &hours, &minutes);

    return formatTime(days, hours, minutes);
}


string WxTimeSpanEdit::formatTime(int days, int hrs, int mins)
{
    string result;
    char buf[10];

    sprintf(buf, "%dd " , days);
    result = buf;

    sprintf(buf, "%02dh ", hrs);
    result += buf;

    sprintf(buf,"%02dm", mins);
    result += buf;

    return result;
}

string WxTimeSpanEdit::toString(long value)
{
    string result;
    char buf[32];
    int days, hours, minutes;

    WxTimeSpanEdit::splitTimeMinutes(value, &days, &hours, &minutes);

    sprintf(buf, "%d %d:%d:00", days, hours, minutes);
    result = buf;

    return buf;
}


void WxTimeSpanEdit::setMaxValue(long maxval)
{
    //int maxDays = 0, maxHours = 0, maxMinutes = 0;
    //long timeRmng;

    p_maxValue = maxval;
}


bool WxTimeSpanEdit::getRangeEnforced()
{
    return p_rangeEnfrc;
}


void WxTimeSpanEdit::setRangeEnforced(bool enfrc)
{
    int maxDays = 365;
    int maxHours = (HOURS_PER_DAY - 1);
    int maxMinutes = (MINUTES_PER_HOUR - 1);

    p_rangeEnfrc = enfrc;

    if (p_rangeEnfrc)
    {
        splitTimeMinutes(p_maxValue, &maxDays, &maxHours, &maxMinutes);
    }

    p_daysSpinCtrl->SetRange(0, maxDays);
    p_hoursSpinCtrl->SetRange(0, maxHours);
    p_minutesSpinCtrl->SetRange(0, maxMinutes);

}

bool WxTimeSpanEdit::getEnabled()
{
    return p_enbld;
}


void WxTimeSpanEdit::setEnabled(bool enbld)
{
    if (p_enbld != enbld)
    {
        p_enbld = enbld;

        p_daysSpinCtrl->Enable(p_enbld);
        p_hoursSpinCtrl->Enable(p_enbld);
        p_minutesSpinCtrl->Enable(p_enbld);

        p_daysLabelStaticText->Enable(p_enbld);
        p_hoursLabelStaticText->Enable(p_enbld);
        p_minutesLabelStaticText->Enable(p_enbld);
    }
}


long WxTimeSpanEdit::getValue()
{
    int days, hours, minutes;
    //long value;

    days = p_daysSpinCtrl->GetValue();
    hours = p_hoursSpinCtrl->GetValue();
    minutes = p_minutesSpinCtrl->GetValue();

    return calcTimeMinutes(days, hours, minutes);
}


void WxTimeSpanEdit::setValue(long value)
{
    int days = 0, hours = 0, minutes = 0;
    //int maxDays;
    int maxHours, maxMinutes;
    long timeRmng;

    if (p_rangeEnfrc && (value > p_maxValue))
        value = p_maxValue;

    splitTimeMinutes(value, &days, &hours, &minutes);

    p_inCtrlUpdate = true;

    if (p_rangeEnfrc)
    {
        timeRmng = p_maxValue;

        timeRmng = timeRmng - (days * MINUTES_PER_DAY);
        maxHours = min((timeRmng / MINUTES_PER_HOUR), (long)(HOURS_PER_DAY - 1));
        p_hoursSpinCtrl->SetRange(0, maxHours);     //  Should adjust the value as needed

        timeRmng = timeRmng - (hours * MINUTES_PER_HOUR);
        maxMinutes = min(timeRmng, (long)(MINUTES_PER_HOUR - 1));
        p_minutesSpinCtrl->SetRange(0, maxMinutes);  //  Should adjust the value as needed
    }

    p_daysSpinCtrl->SetValue(days);
    p_hoursSpinCtrl->SetValue(hours);
    p_minutesSpinCtrl->SetValue(minutes);

    p_inCtrlUpdate = false;
}
