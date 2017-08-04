#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Preferences.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxConfig.H"

/**
 * Default ctor.
 */
ewxDialog::ewxDialog() : wxDialog(), ewxStyledWindow()
{
  Init();
}




/**
 * Ctor.
 */
ewxDialog::ewxDialog(wxWindow* parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : wxDialog(),
    ewxStyledWindow()
{
  Init();
  Create(parent, id, title, pos, size, style, name);
}


/**
 * Two-step ctor.
 */
bool ewxDialog::Create(wxWindow* parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!wxDialog::Create(parent, id, title, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ewxDialog creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}


/**
 * Destructor.
 */
ewxDialog::~ewxDialog()
{
   //  callers responsibliity to delete
   //p_contents = 0;
}



/**
 * Common part of all ctors.
 */
void ewxDialog::Init()
{
   //p_contents = 0;
}


/**
 * This method allows allows you to create a dialog and set its contents to
 * some arbitrary panel.  The way to use it is to create the dialog, create
 * your panel, then call this method.  This should only be done one time
 * and if you try to use it any other way, you won't be happy with it.
 * Its basically a convenience method for wrapping any panel as a popup dialog.
 *
 * Note that in theory, you could create the dialog without a close button
 * but I found that this doesn't actually work on linux.
 *
 * THe caller is responsible for deleting their panel.
 *
 * @param allowClose if true, the window manager close will destroy the window
 *    and the dialog will have ok/cancel/help buttons
void ewxDialog::setContents(wxWindow *panel, bool allowClose)
{
   int MARGIN = 0;
   wxFlexGridSizer* topSizer = new wxFlexGridSizer(2, 1, 0, 0);
   topSizer->AddGrowableRow(0);
   topSizer->AddGrowableCol(0);
   SetSizer(topSizer);

   //SetAutoLayout(TRUE);

   // THe user panel will be set to expand - no margin
   topSizer->Add(panel, 0, wxEXPAND|wxGROW|wxALL, 0);
   p_contents = panel;
   int iw, ih, tw, th;

   if (allowClose == false) {
      // In this case we need to disable the close button.  Couldn't get it to
      // not show up int title bar on linux at least.
      Connect(wxID_ANY,
            wxEVT_CLOSE_WINDOW,
            (wxObjectEventFunction) (wxEventFunction)
            (wxCloseEventFunction)
            &ewxDialog::OnCloseWindow, 0, this);
     panel->GetSize(&tw,&th);
     SetSize(tw, th);

   } else {
      // Add a section that has ok/cancel/help buttons.  This section should
      // not resize because of how the flexible sizer is set up.
      wxBoxSizer* lineSizer = new wxBoxSizer(wxHORIZONTAL);
      topSizer->Add(lineSizer, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);
      ewxStaticLine* item4 = new ewxStaticLine( this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
      lineSizer->Add(item4, 1, wxGROW|wxALL, 0);

      wxBoxSizer* butSizer = new wxBoxSizer(wxHORIZONTAL);
      topSizer->Add(butSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, MARGIN);
      ewxButton* item6 = new ewxButton( this, ID_BUTTON_EWXDIALOG_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
      butSizer->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, MARGIN);
      ewxButton* item7 = new ewxButton( this, ID_BUTTON_EWXDIALOG_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
      butSizer->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxALL, MARGIN);
      ewxButton* item8 = new ewxButton( this, -1, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
      butSizer->Add(item8, 0, wxALIGN_CENTER_VERTICAL|wxALL, MARGIN);

      panel->GetSize(&tw,&th);
      item7->GetSize(&iw,&ih);
      th += ih;
      th += (MARGIN * 3 * 2);  // 2=top+bottom, 3=sizer+buttons+overallsizer
      //Figure out if we need to increase size to accomodate the buttons.
      // 3 is the number of buttons, 2=left+right margins
      if (iw*3 + MARGIN*3*2 > tw) {
         tw = iw*3 + MARGIN*3*2;
      }
      SetSize(tw, th);

      Connect(ID_BUTTON_EWXDIALOG_CANCEL,
            wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) (wxEventFunction)
            (wxCloseEventFunction)
            &ewxDialog::OnCloseButton, 0, this);
      Connect(ID_BUTTON_EWXDIALOG_OK,
            wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) (wxEventFunction)
            (wxCloseEventFunction)
            &ewxDialog::OnOkButton, 0, this);
   }
}
 */



/**
 * Gets pointer to window supplied in call to setContents method.
 * @return 0 if setContents was not used.
wxWindow *ewxDialog::getContents() const
{
   return p_contents;
}
 */


/**
 * THis handler captures the close event and then does nothing.
 * It supports the concept of a dialog that should not be closed by the user
 * and is used when setContents is called with a flag of false.
void ewxDialog::OnCloseWindow( wxCloseEvent& event )
{
}

void ewxDialog::OnCloseButton( wxCloseEvent& event )
{
   Close(true);
}


void ewxDialog::OnOkButton( wxCloseEvent& event )
{
}
 */


void ewxDialog::saveWindowSettings(ewxConfig * config, bool saveSize) 
{
  ewxWindowUtils::saveWindowSettings(this, config, saveSize);
}


void ewxDialog::restoreWindowSettings(ewxConfig * config, bool restoreSize) 
{
  ewxWindowUtils::restoreWindowSettings(this, config, restoreSize);
}


/**
 * @deprecated Should use the ewxConfig version from now on
 */
void ewxDialog::saveWindowSettings(const string& prefix,
                                   Preferences & prefs,
                                   bool saveSize)
{
  ewxWindowUtils::saveWindowSettings(this, prefix, prefs, saveSize);
}


/**
 * @deprecated Should use the ewxConfig version from now on
 */
bool ewxDialog::restoreWindowSettings(const string& prefix, 
                                      Preferences & prefs, 
                                      bool restoreSize)
{
   return 
     ewxWindowUtils::restoreWindowSettings(this, prefix, prefs, restoreSize);
}

void ewxDialog::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this, family);
}
