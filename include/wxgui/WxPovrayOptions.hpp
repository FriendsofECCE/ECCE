/**
 * @file
 *
 */

#ifndef _WXPOVRAYOPTIONS_H_
#define _WXPOVRAYOPTIONS_H_

#include <string>
  using std::string;

#include "wxgui/WxPovrayOptionsGUI.H"

class WxPovrayOptions : public WxPovrayOptionsGUI
{
  public:
    WxPovrayOptions();
    WxPovrayOptions( wxWindow* parent,
                     wxWindowID id = SYMBOL_WXPOVRAYOPTIONSGUI_IDNAME,
                     const wxPoint& pos = SYMBOL_WXPOVRAYOPTIONSGUI_POSITION,
                     const wxSize& size = SYMBOL_WXPOVRAYOPTIONSGUI_SIZE,
                     long style = SYMBOL_WXPOVRAYOPTIONSGUI_STYLE );

    string getFinishStyle() const;
    string getBondStyle() const;
    string getIsosurfaceStyle() const;
    bool   isDisplayChecked() const;
    int    getWidth() const;
    int    getHeight() const;

  private:

};

#endif // _WXPOVRAYOPTIONS_H_
