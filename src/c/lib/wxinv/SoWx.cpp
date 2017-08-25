#include <wx/wx.h>

#include "inv/SoDB.H"
#include "inv/nodekits/SoNodeKit.H"
#include "inv/SoInteraction.H"

#include "inv/SoWx/SoWxEventHandler.H"
#include "inv/SoWx/SoWx.H"

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
