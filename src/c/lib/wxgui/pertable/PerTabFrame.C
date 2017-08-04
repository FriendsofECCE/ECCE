/**
 * @file
 *
 * Periodic table frame class.
 *
 *
 */

#include <iostream>
using std::cerr;

#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "util/BrowserHelp.H"
#include "util/Preferences.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxMenu.H"
#include "wxgui/ewxMenuBar.H"

#include "wxgui/PerTabFrame.H"

IMPLEMENT_DYNAMIC_CLASS( PerTabFrame, ewxFrame )

BEGIN_EVENT_TABLE( PerTabFrame, ewxFrame )
  
  EVT_MENU( wxID_EXIT, PerTabFrame::OnMenuClick )
  EVT_MENU( wxID_HELP, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_FEEDBACK, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_PHYS, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_METAL, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_DEFAULT, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_USER, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_SHOWNUM, PerTabFrame::OnMenuClick )
  EVT_MENU( ID_ITEM_SHOWDATA, PerTabFrame::OnMenuClick )

  EVT_CLOSE( PerTabFrame::OnCloseWindow )

END_EVENT_TABLE()


/**
 * The default constructor of PerTabFrame class
 *
 * This class contains a PerTabPanel object and a menubar.
 * Note that the "View" menu is handled differently because it is shared
 * with the p_topPanel as a right click menu. In order to avoid two copies
 * of the same menu and the chaos of syncronizing both with the same
 * selection, only one menu object is generated in p_topPanel and shared
 * here by calling p_topPanel->GetLongViewMenu().
 */
PerTabFrame::PerTabFrame(wxWindow* parent, wxWindowID id, const wxString& caption,
			 const wxPoint& pos, const wxSize& size, long style)
{
  ewxFrame::Create( parent, id, caption, pos, size, style , PERTAB);

  wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(topSizer);

  p_topPanel = new PerTabPanel(this, false, ID_ITEM_DEFAULT, true, true, false,
                               ID_PANEL_PERTAB, PERTABPANEL_POSITION,
                               PERTABPANEL_SIZE, PERTABPANEL_STYLE);
  topSizer->Add(p_topPanel, 1, wxEXPAND|wxALL, 0);

  ewxMenuBar * menuBar = new ewxMenuBar;

  // Mac doesn't need this menu
#ifndef __WXMAC__
  ewxMenu * fileMenu = new ewxMenu;
  fileMenu->Append(wxID_EXIT, "Quit", "", wxITEM_NORMAL);
  menuBar->Append(fileMenu, "File");
#endif

  menuBar->Append(p_topPanel->getLongViewMenu(), "View");

  ewxMenu * helpMenu = new ewxMenu;
  helpMenu->Append(wxID_HELP, "on Periodic Table...", "", wxITEM_NORMAL);
  helpMenu->Append(ID_ITEM_FEEDBACK, "Support...", "", wxITEM_NORMAL);
  menuBar->Append(helpMenu, "Help");

  SetMenuBar(menuBar);

  Preferences prefs("PerTable");
  
  restoreWindowSettings("PerTab", prefs, true);

  // Get Registry
  ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();


  // Set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(PERTAB)->getIcon()),
                                       wxBITMAP_TYPE_XPM));

  SetMinSize(GetBestSize());
}


/**
 * MenuClick call back function
 *
 * This menu click call back function handles all the menu click events.
 * Note that "Quit", "About" and "Feedback" are handled in the function,
 * all the other events triggered from the view menu are handled in p_topPanel.
 */
void PerTabFrame::OnMenuClick(wxCommandEvent& event)
{
  BrowserHelp help;
  switch (event.GetId()) {
  case wxID_EXIT:
    Close(TRUE);
    break;
  case wxID_HELP:
    help.showPage(help.URL(PERTAB));
    break;
  case ID_ITEM_FEEDBACK:
    help.showFeedbackPage();
    break;
  default:
    p_topPanel->OnMenuClick(event);
  }
}


/**
 * Close window event call back function
 *
 * This function will be called when close button is clicked.
 * It will save off the preference settings and close off the frame.
 */
void PerTabFrame::OnCloseWindow( wxCloseEvent& event )
{
  Preferences prefs("PerTable");
  saveWindowSettings("PerTab", prefs, true);
  prefs.saveFile();
  Destroy();
}


/**
 * Reset using user preferences
 */
void PerTabFrame::resetUserPreferences()
{
  p_topPanel->resetUserPreferences();
}


vector<string> PerTabFrame::selectedAtomSymbols()
{
  // Not finished yet.
  // Need to support multiple selection.
  vector<string> a;
  return a;
}
