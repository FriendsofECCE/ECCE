/**
 * @file
 *
 * @author Jack Chatterton
 *
 *
 */

#ifndef WXCONFIGURENWFSARCHIVE_H
#define WXCONFIGURENWFSARCHIVE_H

#include <wx/wx.h>

#include <string>
using std::string;

#include "WxConfigureNwfsArchiveGUI.H"

/**
 * The implementation of WxConfigureNwfsArchiveGUI.
 */
class WxConfigureNwfsArchive : public WxConfigureNwfsArchiveGUI {
  public:

    WxConfigureNwfsArchive( );
    WxConfigureNwfsArchive( wxWindow* parent, 
            wxWindowID id = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_IDNAME,
            const wxString& caption = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_TITLE,
            const wxPoint& pos = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_POSITION,
            const wxSize& size = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_SIZE,
            long style = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_STYLE );
    virtual ~WxConfigureNwfsArchive( );
    virtual bool Show(const bool show);


  protected:
    virtual void OnOkClick( wxCommandEvent& event );
    virtual void OnCancelClick( wxCommandEvent& event );

  private:
    void setTextCtrlValues();

/*
    void setFrom(const string& value);


  protected:

    virtual void OnFindButtonGoClick( wxCommandEvent& event );
    virtual void OnFindButtonFindClick( wxCommandEvent& event );
    virtual void OnFindButtonCloseClick( wxCommandEvent& event );
    virtual void OnFindButtonHelpClick( wxCommandEvent& event );
    virtual void OnFindResultsSelected( wxCommandEvent& event );
*/
};

#endif

