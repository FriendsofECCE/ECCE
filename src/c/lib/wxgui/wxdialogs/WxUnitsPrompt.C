
#include "wx/wx.h"

#include "wxgui/ewxChoice.H"
#include "wxgui/ewxCheckBox.H"

#include "wxgui/WxUnitsPrompt.H"

WxUnitsPrompt::WxUnitsPrompt(wxWindow *parent, const bool& bondsPrompt)
  : WxUnitsPromptGUI(parent)
{
  if (bondsPrompt) {
    p_genBonds->Show(true);
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
  }
}

string WxUnitsPrompt::getUnits() const
{
  return p_units->GetStringSelection().c_str();
}

bool WxUnitsPrompt::getGenBonds() const
{
  return p_genBonds->IsChecked();
}
