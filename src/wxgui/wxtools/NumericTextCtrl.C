#include <wx/sizer.h>

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/NumericTextCtrl.H"
#include "wxgui/NumericValidatorBase.H"


IMPLEMENT_DYNAMIC_CLASS(NumericTextCtrl, ewxPanel)


NumericTextCtrl::NumericTextCtrl()
  : ewxPanel(),
    p_text(NULL),
    p_units(NULL),
    p_range(NULL)
{
}


NumericTextCtrl::NumericTextCtrl(wxWindow* parent, wxWindowID id,
        const wxString& value, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
  : ewxPanel(),
    p_text(NULL),
    p_units(NULL),
    p_range(NULL)
{
  Create(parent, id, value, pos, size, style, validator, name);
}


NumericTextCtrl::~NumericTextCtrl()
{
}


bool NumericTextCtrl::Create(wxWindow* parent, wxWindowID id,
        const wxString& value, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
{
  if (!ewxPanel::Create(parent)) {
    wxFAIL_MSG( wxT("NumericTextCtrl creation failed") );
    return false;
  }

  p_text = new ewxTextCtrl(this, id, value, pos, size, style, validator, name);
  p_units = new ewxNonBoldLabel(this, wxID_ANY);
  p_range = new ewxNonBoldLabel(this, wxID_ANY);

  SetValidator(validator);

  wxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
  sizer->Add(p_text, 1, wxALIGN_CENTER_VERTICAL);

  wxSizer * label_sizer = new wxBoxSizer(wxVERTICAL);
  label_sizer->Add(p_units, 0);
  label_sizer->Add(p_range, 0);
  sizer->Add(label_sizer, 0, wxALIGN_CENTER_VERTICAL);

  SetSizer(sizer);
  //sizer->Fit(this);
  //sizer->SetSizeHints(this);

  setStyles(this, false);

  return true;
}


void NumericTextCtrl::SetValidator(const wxValidator& validator)
{
  if (!p_text) return;

  NumericValidatorBase * nv =
          dynamic_cast<NumericValidatorBase*>(validator.Clone());
  if (nv) {
    if (!nv->GetUnitsLabel() || !nv->GetRangeLabel()) {
      // so the user wants to use a NumericValidator(Base) but hasn't yet
      // assigned a units label or a range label
      if (!nv->GetUnitsLabel()) {
        nv->SetUnitsLabel(p_units);
      }
      if (!nv->GetRangeLabel()) {
        nv->SetRangeLabel(p_range);
      }
    }
    p_text->SetValidator(*nv);
  } else {
    p_text->SetValidator(validator);
  }
}

