/**
 * @file
 *
 * @author Jeff Daily
 *
 *
 */

#ifndef _WXACCESSCONTROL_H_
#define _WXACCESSCONTROL_H_

#include <string>
using std::string;

#include "wxgui/WxFeedback.H"
#include "wxgui/WxAccessControlGUI.H"

/**
 * Implementation of WxAccessControlGUI.
 * This class displays input boxes for setting which users can access the
 * current prject folder.
 */
class WxAccessControl : public WxAccessControlGUI
{
  public:

    WxAccessControl( );
    WxAccessControl( wxWindow* parent, 
                     wxWindowID id = SYMBOL_WXACCESSCONTROLGUI_IDNAME, 
                     const string& caption = SYMBOL_WXACCESSCONTROLGUI_TITLE,
                     const wxPoint& pos = SYMBOL_WXACCESSCONTROLGUI_POSITION,
                     const wxSize& size = SYMBOL_WXACCESSCONTROLGUI_SIZE, 
                     long style = SYMBOL_WXACCESSCONTROLGUI_STYLE ); 
    ~WxAccessControl();

    /* These functions do the work of actually changing access. */
    void setContext(const string &url, const string& path);
    void updateAccess(const string &url, const string &path);
    void changePermissions(const string &url, const string &readOnlyStr,
                           const string &readWriteStr);

    /* Call backs */
    void OnCloseWindow( wxCloseEvent& event );
    void OnButtonChangeClick( wxCommandEvent& event );
    void OnButtonCloseClick( wxCommandEvent& event );
    void OnButtonHelpClick( wxCommandEvent& event );

  protected:

  private:

    string p_projectUrl;

    WxFeedback *p_feedback;
};

#endif // _WXACCESSCONTROL_H_

