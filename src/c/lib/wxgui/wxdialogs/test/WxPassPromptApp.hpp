#ifndef WXPASSPROMPTAPP_H
#define WXPASSPROMPTAPP_H

#include "wxgui/ewxApp.hpp"

class WxPassPromptApp : public ewxApp
{
public:
  WxPassPromptApp();
  virtual bool OnInit();
  virtual string getName() const { return "WxPassPrompt"; }
};

DECLARE_APP(WxPassPromptApp)

#endif
