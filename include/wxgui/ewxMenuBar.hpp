#ifndef _EWXMENUBAR_H
#define _EWXMENUBAR_H

#include "wx/menu.h"
#include "wxgui/ewxStyledWindow.hpp"

class ewxMenuBar : public wxMenuBar, public ewxStyledWindow
{
  public:
    // constructors
    ewxMenuBar();

    virtual ~ewxMenuBar();

    virtual bool show(bool show=true);
};

#endif
