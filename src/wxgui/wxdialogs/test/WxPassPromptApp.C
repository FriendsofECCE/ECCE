#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/WxPassPrompt.H"

#include "WxPassPromptApp.H"

IMPLEMENT_APP(WxPassPromptApp)

WxPassPromptApp::WxPassPromptApp() {}

bool WxPassPromptApp::OnInit()
{
  WxPassPrompt::authenticate();
  return true;
}
