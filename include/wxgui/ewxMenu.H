#ifndef _EWXMENU_H
#define _EWXMENU_H

#include "wx/menu.h"

/**
 * Menu sublcass.
 * Note that a Menu is not a wxWindo.
 * So we probably don't need this class but just in case I'll leave it.
 */
class ewxMenu : public wxMenu
{
  public:
    ewxMenu();
};

#endif
