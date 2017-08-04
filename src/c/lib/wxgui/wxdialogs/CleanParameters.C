#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"

#include "wxgui/CleanParameters.H"


const int CleanParameters::DEFAULT_MAX_ITERATIONS(100);
const double CleanParameters::DEFAULT_CONVERGENCE(0.0001);
const double CleanParameters::DEFAULT_FORCE_CUTOFF(5.0);


CleanParameters::CleanParameters(wxWindow *parent)
  : ewxDialog()
{
  if (!Create(parent)) {
    wxFAIL_MSG( wxT("CleanParameters creation failed") );
  }
}


bool CleanParameters::Create(wxWindow *parent)
{
  if (!ewxDialog::Create(parent, wxID_ANY, "ECCE Clean Parameters",
                         wxDefaultPosition, wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE)) {
    return false;
  }

  SetEscapeId(wxID_CLOSE);
  SetName("CleanParameters");
  SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

  ewxNumericValidator validator;

  SetSizer(new wxBoxSizer(wxVERTICAL));

  wxFlexGridSizer *sizer = new wxFlexGridSizer(3);
  sizer->AddGrowableCol(1, 1);
  GetSizer()->Add(sizer, 0, wxEXPAND);

  sizer->Add(new ewxStaticText(this, wxID_ANY, "Max Iterations:",
                               wxDefaultPosition, wxDefaultSize,
                               wxALIGN_RIGHT),
             0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
  p_maxIterations = new ewxSpinCtrl(this, wxID_ANY);
  p_maxIterations->SetRange(0,500);
  sizer->Add(p_maxIterations, 1,wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL,5);
  sizer->Add(createUnitLabel("", "[0..500]"));

  sizer->Add(new ewxStaticText(this, wxID_ANY, "Convergence:",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_RIGHT),
                  0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
  p_convergence = new ewxTextCtrl(this, wxID_ANY);
  validator.setHardRange(1E-7, 1E-2);
  validator.setValue(0.0001);
  p_convergence->SetValidator(validator);
  sizer->Add(p_convergence, 1, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5);
  sizer->Add(createUnitLabel("Hartree/Bohr", "[1E-7..1E-2]"));

  sizer->Add(new ewxStaticText(this, wxID_ANY, "Non-Bond Force Cutoff:",
                               wxDefaultPosition, wxDefaultSize,
                               wxALIGN_RIGHT),
             0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
  p_forceCutoff = new ewxTextCtrl(this, wxID_ANY);
  validator.setHardRange(0, 200);
  validator.setValue(5.0);
  p_forceCutoff->SetValidator(validator);
  sizer->Add(p_forceCutoff, 1, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5);
  sizer->Add(createUnitLabel("Angstroms", "[0..200]"));

  GetSizer()->Add(new ewxStaticLine(this, wxID_ANY), 0, wxALL|wxEXPAND, 5);

  // OK and Cancel buttons; wxStdDialogButtonSizer looked ugly
  wxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(new wxButton(this, wxID_CLOSE));
  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(new wxButton(this, wxID_HELP));
  buttonSizer->AddStretchSpacer();
  GetSizer()->Add(buttonSizer, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL);

  Fit();

  restoreSettings();

  return true;
}


void CleanParameters::EndModal(int retCode)
{
  saveSettings();
  return ewxDialog::EndModal(retCode);
}


int CleanParameters::getMaxIterations() const
{
  return p_maxIterations ? p_maxIterations->GetValue() : 100;
}


double CleanParameters::getConvergence() const
{
  return p_convergence ? p_convergence->getValueAsDouble() : 0.0001;
}


double CleanParameters::getForceCutoff() const
{
  return p_forceCutoff ? p_forceCutoff->getValueAsDouble() : 5.0;
}


void CleanParameters::saveSettings()
{
  Preferences prefs(GetName().c_str());
  saveWindowSettings(GetName().c_str(), prefs, false);
  
  prefs.setInt("MAX_ITERATIONS", getMaxIterations());
  prefs.setFloat("CONVERGENCE", getConvergence());
  prefs.setFloat("FORCE_CUTOFF", getForceCutoff());

  prefs.saveFile();
}


void CleanParameters::restoreSettings()
{
  Preferences prefs(GetName().c_str());
  restoreWindowSettings(GetName().c_str(), prefs, true);

  int intValue;
  float floatValue;

  if (prefs.getInt("MAX_ITERATIONS", intValue)) {
    p_maxIterations->SetValue(intValue);
  } else {
    p_maxIterations->SetValue(100);
  }

  if (prefs.getFloat("CONVERGENCE", floatValue)) {
    p_convergence->setValueAsDouble(floatValue);
  } else {
    p_convergence->setValueAsDouble(0.0001);
  }

  if (prefs.getFloat("FORCE_CUTOFF", floatValue)) {
    p_forceCutoff->setValueAsDouble(floatValue);
  } else {
    p_forceCutoff->setValueAsDouble(5.0);
  }

  TransferDataFromWindow();
}


wxSizer * CleanParameters::createUnitLabel(const wxString& unit,
                                           const wxString& range)
{
  wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(new wxStaticText(this, wxID_ANY, unit));
  sizer->Add(new ewxNonBoldLabel(this, wxID_ANY, range));
  return sizer;
}

