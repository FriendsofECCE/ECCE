#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SessionProjectContextPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/splitter.h"

#include <iostream>
using namespace std;

#include "util/Preferences.H"

#include "wxgui/WxResourceTreeItemData.H"

#include "ContextPanelInterface.H"
#include "ProjectContextPanel.H"
#include "SessionContextPanel.H"
#include "SessionProjectContextPanel.H"


/*!
 * SessionProjectContextPanel event table definition
 */
BEGIN_EVENT_TABLE( SessionProjectContextPanel, ewxPanel )
  EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, SessionProjectContextPanel::OnSashPosChanged)
END_EVENT_TABLE()


/*!
 * SessionProjectContextPanel constructors
 */
SessionProjectContextPanel::SessionProjectContextPanel( CalcMgr * calcMgr,
                                                        WxResourceTreeItemData *itemData, 
                                                        wxWindow *parent,
                                                        wxWindowID id,
                                                        const wxPoint& pos,
                                                        const wxSize& size,
                                                        long style ):
  ewxPanel(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr)
{
  p_calcMgr = calcMgr;
  p_itemData = itemData;

  CreateControls();
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  // in the workflow, make sure the leaf of the primary path is visible
  p_session->ensureVisiblePrimaryLeaf();
}


SessionProjectContextPanel::~SessionProjectContextPanel( )
{
}


/*!
 * Control creation for SessionProjectContextPanel
 */
void SessionProjectContextPanel::CreateControls()
{    
  wxBoxSizer * topSizer = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(topSizer);

  p_splitterWindow = new wxSplitterWindow( this,
                                           wxID_ANY,
                                           wxDefaultPosition,
                                           wxDefaultSize,
                                           wxSP_3DBORDER|wxSP_3DSASH|wxNO_BORDER );
  
  p_session = new SessionContextPanel(p_calcMgr, p_itemData, p_splitterWindow);
  p_project = new ProjectContextPanel(p_calcMgr, p_itemData, p_splitterWindow);

  p_project->p_projectView->enableMultiSelect(false);


  Preferences prefs("Organizer");
  int splitPos = 300;
  prefs.getInt("Organizer.SessionHeight", splitPos);

  p_splitterWindow->SplitHorizontally(p_session, p_project, splitPos);

  topSizer->Add(p_splitterWindow, 1, wxGROW|wxALL, 0);
}


void SessionProjectContextPanel::refresh()
{
  p_project->refresh();
  p_session->refresh();
  p_project->p_projectView->enableMultiSelect(false);
}


void SessionProjectContextPanel::selectResource(const EcceURL & url)
{
  p_project->selectResource(url);
  p_session->selectResource(url);
}


vector<EcceURL> SessionProjectContextPanel::getSelections()
{
  return p_project->getSelections();
}




void SessionProjectContextPanel::loseFocus()
{
  p_project->loseFocus();
  p_session->loseFocus();
}


void SessionProjectContextPanel::OnSashPosChanged( wxSplitterEvent & event )
{
  Preferences prefs("Organizer");
  prefs.setInt("Organizer.SessionHeight",
               p_splitterWindow->GetSashPosition());
  prefs.saveFile();
  event.Skip();
}
