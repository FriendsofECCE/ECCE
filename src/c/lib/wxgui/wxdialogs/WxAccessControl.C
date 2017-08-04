/**
 * @file
 *
 * @author Jeff Daily
 *
 * wx version of calculation manager's access control dialog
 *
 */

#include <wx/sizer.h>

#include "dsm/EDSIServerCentral.H"

#include "util/NullPointerException.H"

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxFeedback.H"

#include "wxgui/WxAccessControl.H"


/**
 * Constructor.
 * Shouldn't be used, yet it is not easy to define it away.
 */
WxAccessControl::WxAccessControl( )
{
  // It might be nice to put some assert(false) in here.
}



/**
 * Constructor.
 * All the default parameters are defined in WxAccessControlGUI class.
 * This class gets its icon from its parent, which has a TopLevelWindow.
 */
WxAccessControl::WxAccessControl( wxWindow* parent, wxWindowID id,
                                  const string& caption, const wxPoint& pos,
                                  const wxSize& size, long style )
{
  Create( parent, id, caption, pos, size, style );

  // get icon of parent's TopLevelWindow
  wxTopLevelWindow *topWindow = dynamic_cast<wxTopLevelWindow*>(parent);
  NULLPOINTEREXCEPTION(topWindow, 
      "Unable to cast from wxWindow to wxTopLevelWindow");
  SetIcon(topWindow->GetIcon());

  p_feedback = new WxFeedback(this);
  p_feedback->hideEditState();
  p_feedback->hideDropSite();
  p_feedback->hideTopLine();
  p_feedback->hideStateIcon();
  p_topSizer->Detach(p_buttonSizer);
  p_topSizer->Add(p_feedback, 0, wxGROW, 0);
  p_topSizer->Add(p_buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
  Fit();
  SetMinSize(GetBestSize());
}



/**
 * Destructor.
 */ 
WxAccessControl::~WxAccessControl()
{
  // do nothing, parent window uses Destroy().
}


    
/**
 *
 */
void WxAccessControl::setContext(const string &url, const string& path)
{
  p_projectUrl = url;
  p_projectName->SetLabel(path);
  updateAccess(url, path);
}



/**
 *
 */
void WxAccessControl::updateAccess(const string &url, const string &path)
{
   // retrieve access permissions
  EDSIServerCentral servers;
  string access;
  string errMessage;
  int status = servers.getAccess(url, access, errMessage);

  if (status == 200) {
     string line1 = access;
     string line2 = access;
     string::size_type newline = line1.find('\n');

     // get rid of 2nd line for line1
     line1.erase(newline, line1.length()-newline);
     // get rid of leading #R#
     line1.erase(0, 3);
     // get rid of trailing #R#
     line1.erase(line1.length()-3, 3);

     // get rid of 1st line for line2
     line2.erase(0, newline+1);
     // get rid of leading #RW#
     line2.erase(0, 4);
     // get rid of trailing #RW#\n
     line2.erase(line2.length()-5, 5);


     p_readOnly->SetValue(line1);
     p_readWrite->SetValue(line2);
  } else {
     // report error back to main explorer window
     p_readOnly->SetValue("");
     p_readWrite->SetValue("");
     p_feedback->setMessage("Unable to retrieve access permissions: "+errMessage);
  }
}



/**
 *
 */
void WxAccessControl::changePermissions(const string &url, 
                                        const string &readOnlyStr,
                                        const string &readWriteStr)
{
  EDSIServerCentral servers;
  string errMessage;
  int status = 
    servers.setAccess(p_projectUrl, readOnlyStr, readWriteStr, errMessage);

  if (status == 200) {
    p_feedback->setMessage("Access permissions updated.", WxFeedback::INFO);
  } else {
    p_feedback->setMessage("Unable to set access permissions: " + errMessage);
  }
}



/* All event handling functions. */

/// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
void WxAccessControl::OnCloseWindow( wxCloseEvent& event )
{
  Show(false);
}

/// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CHANGE
void WxAccessControl::OnButtonChangeClick( wxCommandEvent& event )
{
  changePermissions(p_projectUrl, 
                    p_readOnly->GetValue().c_str(), 
                    p_readWrite->GetValue().c_str());
  event.Skip();
}

/// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE
void WxAccessControl::OnButtonCloseClick( wxCommandEvent& event )
{
  Show(false);
}

/// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
void WxAccessControl::OnButtonHelpClick( wxCommandEvent& event )
{
  event.Skip();
}

