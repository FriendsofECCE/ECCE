/**
 * @file
 *
 * @author Jeff Daily
 *
 *
 */

#ifndef WXFIND_H
#define WXFIND_H

#include <wx/wx.h>

#include <string>
using std::string;

#include "WxFindGUI.H"

/**
 * The implementation of WxFindGUI.
 * User is prompted for a string which is matched to any filename which contains
 * the given string as a substring.
 */
class WxFind : public WxFindGUI
{
  public:

    WxFind( );
    WxFind( wxWindow* parent, 
            wxWindowID id = SYMBOL_WXFINDGUI_IDNAME,
            const wxString& caption = SYMBOL_WXFINDGUI_TITLE,
            const wxPoint& pos = SYMBOL_WXFINDGUI_POSITION,
            const wxSize& size = SYMBOL_WXFINDGUI_SIZE,
            long style = SYMBOL_WXFINDGUI_STYLE );
    virtual ~WxFind( );

    void setFrom(const string& value);

  protected:

    virtual void OnFindButtonGoClick( wxCommandEvent& event );
    virtual void OnFindButtonFindClick( wxCommandEvent& event );
    virtual void OnFindButtonCloseClick( wxCommandEvent& event );
    virtual void OnFindButtonHelpClick( wxCommandEvent& event );
    virtual void OnFindResultsSelected( wxCommandEvent& event );

};

#endif

