#ifndef WXUNITSPROMPT_H_
#define WXUNITSPROMPT_H_

#include "wxgui/WxUnitsPromptGUI.H"

class WxUnitsPrompt : public WxUnitsPromptGUI
{
  public:
    WxUnitsPrompt(wxWindow *parent, const bool& bondsPrompt=false);

    string getUnits() const;
    bool   getGenBonds() const;
};

#endif // WXUNITSPROMPT_H_
