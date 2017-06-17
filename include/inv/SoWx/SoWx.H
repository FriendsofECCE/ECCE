#ifndef __SOWX_H
#define __SOWX_H

#include <wx/wx.h>

#include "inv/SoWx/SoWxEventHandler.H"

//#define SOWXDEBUG 1

class SoWx {
public:
  static void init(wxWindow * topWin);
  
  static wxTopLevelWindow * getTopLevelWidget(wxWindow * child);

private:
  SoWx();
  ~SoWx();

  static wxWindow * p_topWindow;
  static SoWxEventHandler * p_eventHandler;

};

#endif
