#ifndef WXPASSPROMPTAPP_H
#define WXPASSPROMPTAPP_H

#include "wxgui/ewxApp.H"

class WxPassPromptApp : public ewxApp
{
public:
  WxPassPromptApp();
  virtual bool OnInit();
  virtual string getName() const { return "WxPassPrompt"; }
};

DECLARE_APP(WxPassPromptApp)

#endif
