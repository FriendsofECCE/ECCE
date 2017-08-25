/**
 * Panel that implements translations of the system
 */

#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"

#include "viz/TranslateCmd.H"

#include "TranslatePanel.H"

/**
 * TranslatePanel constructors
 */
TranslatePanel::TranslatePanel( )
{
  initializeGUI();
}

TranslatePanel::TranslatePanel( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style )
{
  Create(parent, id, pos, size, style);
  initializeGUI();
}

/**
 * TranslatePanel destructor
 */
TranslatePanel::~TranslatePanel()
{
}

void TranslatePanel::initializeGUI()
{
  ewxTextCtrl *text = 0;
  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TRANSL_X));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TRANSL_Y));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TRANSL_Z));
  validator.setHardRange("(,)");
  validator.setValue(0.0);
  text->SetValidator(validator);
}

/**
 * Event handler to implement translation
 */

void TranslatePanel::OnButtonTranslGoClick( wxCommandEvent& event )
{
  ewxTextCtrl *text = 0;
  WxVizToolFW &fw = getFW();

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TRANSL_X));
  double x = text->getBaseValueAsDouble();
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TRANSL_Y));
  double y = text->getBaseValueAsDouble();
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TRANSL_Z));
  double z = text->getBaseValueAsDouble();

  Command *cmd = new TranslateCmd("Translate", &fw.getSceneGraph());
  cmd->getParameter("x")->setDouble(x);
  cmd->getParameter("y")->setDouble(y);
  cmd->getParameter("z")->setDouble(z);
  fw.execute(cmd);
  event.Skip();
}
