#include "wxgui/GenericValidator.H" // for wxEVT_VALIDATION_EVENT
#include "wxgui/SliderCombo.H"


///////////////////////////////////////////////////////////////////////////////
// SliderComboPopup
///////////////////////////////////////////////////////////////////////////////

SliderComboPopup::SliderComboPopup()
  : ewxSlider(),
    wxComboPopup(),
    p_isInt(false)
{
}


SliderComboPopup::~SliderComboPopup()
{
}


void SliderComboPopup::Init()
{
}


bool SliderComboPopup::Create(wxWindow *parent)
{
  wxSize size(20,150);
  bool ret = ewxSlider::Create(parent, wxID_ANY, 50, 0, 100,
          wxDefaultPosition, size, wxSL_VERTICAL);
  SetBackgroundColour(ewxStyledWindow::getInputColor());
  SetMaxSize(size);
  // This causes clicking in the slider to move by one increment
  // and behaves more like the old calcviewer.  Seems better...
  // See SliderCombo class also - must be reset when the range is changed.
  SetPageSize(GetLineSize());
  return ret;
}


wxWindow* SliderComboPopup::GetControl()
{
  return this;
}


void SliderComboPopup::SetStringValue(const wxString& value)
{
  if (p_isInt) {
    long ival;
    value.ToLong(&ival);
    SetValue(static_cast<int>(ival));
  } else {
    double dval;
    value.ToDouble(&dval);
    SetValue(static_cast<int>(dval * 100.0f));
  }
}


wxString SliderComboPopup::GetStringValue() const
{
  wxString ret;

  if (p_isInt) {
    ret = wxString::Format("%d", GetValue());
  } else {
    ret = wxString::Format("%2.2f", GetValue() / 100.0f);
  }

  return ret;
}


void SliderComboPopup::OnComboKeyEvent(wxKeyEvent &event)
{
  if (event.GetKeyCode() == WXK_RETURN) {
    SetStringValue(m_combo->GetTextCtrl()->GetValue());
  }
  event.Skip();
}



///////////////////////////////////////////////////////////////////////////////
// SliderCombo
///////////////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE(SliderCombo, wxComboCtrl)
  EVT_SCROLL(SliderCombo::OnScroll)
  EVT_COMMAND(wxID_ANY, wxEVT_VALIDATION_EVENT, SliderCombo::OnValidate)
END_EVENT_TABLE()


SliderCombo::SliderCombo()
  : wxComboCtrl(),
    p_slider(NULL)
{
}


SliderCombo::SliderCombo(wxWindow *parent, wxWindowID id,
        const wxString& value, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
  : wxComboCtrl(),
    p_slider(NULL)
{
  Create(parent, id, value, pos, size, style, validator, name);
}


SliderCombo::~SliderCombo()
{
}


bool SliderCombo::Create(wxWindow *parent, wxWindowID id,
        const wxString& value, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
{
  if (!wxComboCtrl::Create(parent, id, value, pos, size, style,
                           validator, name)) {
    wxFAIL_MSG( wxT("SliderCombo creation failed") );
    return false;
  }

  SetPopupAnchor(wxRIGHT);

  SliderComboPopup *popupCtrl = new SliderComboPopup();
  p_slider = popupCtrl;

  SetPopupControl(popupCtrl);

  wxComboCtrl::SetValue(value);
  SetText(GetPopupControl()->GetStringValue());

  return true;
}


void SliderCombo::SetValue(int value)
{
  wxComboCtrl::SetValue(wxString::Format("%d", value));
}


void SliderCombo::SetValue(double value)
{
  wxComboCtrl::SetValue(wxString::Format("%2.2f", value));
}


void SliderCombo::SetRange(int min, int max)
{
  p_slider->p_isInt = true;
  p_slider->SetRange(min,max);

  // Have to reset this when the range changes
  p_slider->SetPageSize(p_slider->GetLineSize());
}


void SliderCombo::SetRange(double min, double max)
{
  p_slider->p_isInt = false;
  p_slider->SetRange(static_cast<int>(min*100), static_cast<int>(max*100));

  // Have to reset this when the range changes
  p_slider->SetPageSize(p_slider->GetLineSize());
}


int SliderCombo::GetIntValue() const
{
  long val = 0;
  GetValue().ToLong(&val);
  return (int)val;
}


float SliderCombo::GetFloatValue() const
{
  double val = 0;
  GetValue().ToDouble(&val);
  return val;
}


void SliderCombo::OnScroll(wxScrollEvent &event)
{

  SetText(GetPopupControl()->GetStringValue());
  // Change event id, object before relaying it forward
  event.SetId(GetId());
  event.SetEventObject(this);

  Validate();

  event.Skip();
}


void SliderCombo::OnValidate(wxCommandEvent &event)
{
  // Change event id, object before relaying it forward
  event.SetId(GetId());
  event.SetEventObject(this);

  event.Skip();
}

