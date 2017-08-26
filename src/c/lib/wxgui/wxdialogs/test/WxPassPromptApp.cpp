#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/WxPassPrompt.hpp"

#include "WxPassPromptApp.hpp"

IMPLEMENT_APP(WxPassPromptApp)

WxPassPromptApp::WxPassPromptApp() {}

bool WxPassPromptApp::OnInit()
{
  WxPassPrompt::authenticate();
  return true;
}
