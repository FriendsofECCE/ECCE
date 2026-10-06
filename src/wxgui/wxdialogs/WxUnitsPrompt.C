
#include "wx/wx.h"
#include <cstdlib>

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

int WxUnitsPrompt::ShowModal()
{
  // Test hook: ECCE_TEST_XYZ_UNITS=<angstrom|bohr|picometer|nanometer> answers
  // the prompt without showing it, for headless runs of the XYZ readers.
  const char *units = getenv("ECCE_TEST_XYZ_UNITS");
  if (units) {
    wxString name = wxString(units).Lower();
    int sel = name.StartsWith("bohr") ? 1 : name.StartsWith("pico") ? 2
            : name.StartsWith("nano") ? 3 : 0;
    p_units->SetSelection(sel);
    return wxID_OK;
  }
  return WxUnitsPromptGUI::ShowModal();
}
