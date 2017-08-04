/**
 *  @file
 *  @author Ken Swanson
 *
 *  Graphical component for editing a numeric parameter.  This class combines a
 *  wxSpinCtrl with labels for units and range information.  Both the units and
 *  range labels can be hidden as desired.  Changes to the value, via user
 *  interaction, invoke an event which can be caught by a containing window.
 *
 */

#include <algorithm>
    using std::min;
    using std::max;
#include <stdlib.h>
#include <math.h>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxGenericValidator.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxSmallLabel.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/WxParameterEdit.H"

IMPLEMENT_CLASS(WxParameterEdit, ewxPanel)

BEGIN_EVENT_TABLE(WxParameterEdit, wxEvtHandler)
    EVT_SIZE(WxParameterEdit::windowResizeCB)
    EVT_SPINCTRL(ID_SPINCTRL_WXPARAMETEREDIT_VALUE, WxParameterEdit::valueSpinCtrlUpdatedCB)
    EVT_TEXT_ENTER(ID_SPINCTRL_WXPARAMETEREDIT_VALUE, WxParameterEdit::valueSpinCtrlValidateCB)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(ewxEVT_PARAMETER_VALUE_CHANGED)

WxParameterEdit::WxParameterEdit()
  : ewxPanel()
{
  
}


WxParameterEdit::WxParameterEdit(        wxWindow *parent,
                                       wxWindowID  id,
                                 const    wxPoint &pos,
                                 const     wxSize &size,
                                             long  style,
                                 const   wxString &name)
  : ewxPanel()
{
    Create(parent, id, pos, size, style, name);
}


bool WxParameterEdit::Create(wxWindow *parent,
                             wxWindowID  id,
                             const    wxPoint &pos,
                             const     wxSize &size,
                             long  style,
                             const   wxString &name)
{
    if (!ewxPanel::Create(parent, id, pos, size, style, name)) {
        wxFAIL_MSG( wxT("WxParameterEdit creation failed") );
        return false;
    }

    this->createControls();

    PushEventHandler(new ewxHelpHandler(this));

    p_disabler = new ewxDisabler();
    PushEventHandler(p_disabler);

    setStyles(this, true);

    return true;
}


WxParameterEdit::~WxParameterEdit()
{
    PopEventHandler(true);
    PopEventHandler(true);
}


void WxParameterEdit::setCustomDisabledStyle(bool enabled)
{
    if (p_disabler)
        p_disabler->setEnabled(enabled);

    this->setEnabled(enabled);
    setStyles(this, true);
}


void WxParameterEdit::createControls()
{
    wxBoxSizer* szrMain = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* szrRight = new wxBoxSizer(wxVERTICAL);

    p_valueSpinCtrl = new ewxSpinCtrl( this, ID_SPINCTRL_WXPARAMETEREDIT_VALUE, _("0"), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
    szrMain->Add(p_valueSpinCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5);

    p_unitsStaticText = new ewxSmallLabel( this, ID_STATIC_WXPARAMETEREDIT_UNITSLABEL, _(""), wxDefaultPosition, wxDefaultSize, 0 );
    szrRight->Add(p_unitsStaticText, 0, wxALIGN_BOTTOM|wxADJUST_MINSIZE, 5);

    p_rangeStaticText = new ewxSmallLabel( this, ID_STATIC_WXPARAMETEREDIT_UNITSLABEL, _(""), wxDefaultPosition, wxDefaultSize, 0 );
    szrRight->Add(p_rangeStaticText, 0, wxALIGN_BOTTOM|wxADJUST_MINSIZE, 5);

    szrMain->Add(szrRight, 0, wxALIGN_CENTER_VERTICAL, 5);

    this->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_PROCESS_UI_UPDATES);
    this->SetSizer(szrMain);
    this->Layout();
    this->Fit();
}

wxSize WxParameterEdit::DoGetBestSize() const
{
    return (this->GetSizer()->GetMinSize());
}


void WxParameterEdit::windowResizeCB(wxSizeEvent& event)
{
    this->Layout();
}

void WxParameterEdit::valueSpinCtrlUpdatedCB(wxSpinEvent& event)
{
    this->fireValueEdited();
}


void WxParameterEdit::valueSpinCtrlValidateCB(wxCommandEvent& event)
{
    this->fireValueEdited();
}


void WxParameterEdit::fireValueEdited()
{
    wxCommandEvent event(ewxEVT_PARAMETER_VALUE_CHANGED, this->GetId());

    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent( event );
}


int WxParameterEdit::getValue()
{
    return (p_valueSpinCtrl->GetValue());
}


void WxParameterEdit::setValue(int value)
{
    p_valueSpinCtrl->SetValue(value);
}


int WxParameterEdit::getMinimum()
{
    return (p_valueSpinCtrl->GetMin());
}


int WxParameterEdit::getMaximum()
{
    return (p_valueSpinCtrl->GetMax());
}


wxSize WxParameterEdit::GetMinSize() const
{
    return (this->GetSizer()->GetMinSize());
}


void WxParameterEdit::setUnitsVisible(bool vsbl)
{
    p_unitsStaticText->Show(vsbl);
}


bool WxParameterEdit::getUnitsVisible()
{
    return p_unitsStaticText->IsShown();
}


void WxParameterEdit::setRange(int vmin, int vmax)
{
    int w, h;
    string s;

    p_valueSpinCtrl->SetRange(vmin, vmax);
    p_valueSpinCtrl->Refresh();

    s = "[" + StringConverter::toString(vmin) + ".." + StringConverter::toString(vmax) + "]";
    p_rangeStaticText->SetLabel(s);
    p_rangeStaticText->GetTextExtent(s, &w, &h);
    p_rangeStaticText->SetMinSize(wxSize(w, -1));

    int numChars = (int)(ceil(log10((double)(vmax))));
    numChars = max(numChars, 3);

    p_valueSpinCtrl->GetTextExtent("0", &w, &h);
    p_valueSpinCtrl->SetMinSize(wxSize(numChars * w + 25, -1));

    this->Layout();
    this->Fit();
}


void WxParameterEdit::setMinimum(int vmin)
{
    this->setRange(vmin, this->getMaximum());
}


void WxParameterEdit::setMaximum(int vmax)
{
    this->setRange(this->getMinimum(), vmax);
}


bool WxParameterEdit::getRangeVisible()
{
    return (p_rangeStaticText->IsShown());
}


void WxParameterEdit::setRangeVisible(bool vsbl)
{
    p_rangeStaticText->Show(vsbl);
    this->Layout();
    this->Fit();
}


bool WxParameterEdit::setEnabled(bool enbld)
{
    return p_valueSpinCtrl->Enable(enbld);
}


string WxParameterEdit::getUnitsLabel()
{
    return ((string)(p_unitsStaticText->GetLabel()));
}


void WxParameterEdit::setUnitsLabel(string units)
{
    p_unitsStaticText->SetLabel(units);
    this->Layout();
    this->Fit();
}
