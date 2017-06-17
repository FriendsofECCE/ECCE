/**
 * @file
 *
 */

#ifndef _WXPOVRAYOPTIONSDIALOG_H_
#define _WXPOVRAYOPTIONSDIALOG_H_

#include "wxgui/ewxDialog.H"

class WxPovrayOptions;

class WxPovrayOptionsDialog : public ewxDialog
{
  public:
    WxPovrayOptionsDialog( wxWindow* parent, wxWindowID id = wxID_ANY,
                           const wxString& title = "POV-Ray Options" );

    string getFinishStyle() const;
    string getBondStyle() const;
    string getIsoStyle() const;
    bool   isDisplayChecked() const;
    int    getWidth() const;
    int    getHeight() const;

  private:
    WxPovrayOptions * p_options;

};

#endif // _WXPOVRAYOPTIONSDIALOG_H_

