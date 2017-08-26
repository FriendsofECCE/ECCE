#include <wx/wx.h>

#include "inv/SoDB.hpp"
#include "inv/nodekits/SoNodeKit.hpp"
#include "inv/SoInteraction.hpp"

#include "inv/SoWx/SoWxEventHandler.hpp"
#include "inv/SoWx/SoWx.hpp"

SoWxEventHandler * SoWx::p_eventHandler = 0;
wxWindow * SoWx::p_topWindow = 0;

void SoWx::init(wxWindow * topWin)
{
  // init Inventor
  SoDB::init();
  SoNodeKit::init();
  SoInteraction::init();

  p_topWindow = topWin;

  p_eventHandler = new SoWxEventHandler(topWin);
  //  topWin->PushEventHandler(p_eventHandler);
}


wxTopLevelWindow * SoWx::getTopLevelWidget(wxWindow * child)
{
  if (child == NULL)
    return NULL;
  else if (child->IsTopLevel())
    return dynamic_cast<wxTopLevelWindow *>(child);
  else
    return getTopLevelWidget(child->GetParent());
}
