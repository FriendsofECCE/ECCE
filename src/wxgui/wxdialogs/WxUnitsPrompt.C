
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
  // The label is for display (Ångströms); callers compare these names.
  static const char *names[] = { "Angstroms", "Bohr", "Picometers", "Nanometers" };
  int i = p_units->GetSelection();
  return (i >= 0 && i < 4) ? names[i] : names[0];
}

bool WxUnitsPrompt::getGenBonds() const
{
  return p_genBonds->IsChecked();
}
