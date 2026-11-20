#ifndef __SOWX_H
#define __SOWX_H

#include <wx/wx.h>

#include "inv/SoWx/SoWxEventHandler.H"

//#define SOWXDEBUG 1

class SoWx {
public:
  static void init(wxWindow * topWin);
  //  Removes and frees the handlers init pushed on topWin; call it from the
  //  window's destructor, before its base classes pop their own handlers.
  static void release(wxWindow * topWin);
  
  static wxTopLevelWindow * getTopLevelWidget(wxWindow * child);

private:
  SoWx();
  ~SoWx();

  static wxWindow * p_topWindow;
  static SoWxEventHandler * p_eventHandler;

};

#endif
