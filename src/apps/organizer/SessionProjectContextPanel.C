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

#include "wxgui/ewxBitmap.H"
#include "wxgui/WxResourceTreeItemData.H"

#include "ContextPanelInterface.H"
#include "ProjectContextPanel.H"
#include "SessionContextPanel.H"
#include "SessionProjectContextPanel.H"
#include "WorkflowPanel.H"
#include "dsm/ResourceDescriptor.H"


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
  p_session = 0;
  p_workflow = 0;

  CreateControls();
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  // in the workflow, make sure the leaf of the primary path is visible
  if (p_session) p_session->ensureVisiblePrimaryLeaf();
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
  
  wxWindow *top = 0;
  if (p_itemData->getResource()->getApplicationType() ==
      ResourceDescriptor::AT_WORKFLOW) {
    p_workflow = new WorkflowPanel(p_calcMgr, p_itemData, p_splitterWindow);
    top = p_workflow;
  } else {
    p_session = new SessionContextPanel(p_calcMgr, p_itemData, p_splitterWindow);
    top = p_session;
  }
  p_project = new ProjectContextPanel(p_calcMgr, p_itemData, p_splitterWindow);

  p_project->p_projectView->enableMultiSelect(false);


  Preferences prefs("Organizer");
  //  The workflow drawing and its code bar need more height than an MD
  //  study's.
  int splitPos = p_workflow ? 420 : 300;
  prefs.getInt(p_workflow ? "Organizer.WorkflowHeight"
                          : "Organizer.SessionHeight", splitPos);
  splitPos = ewxBitmap::scaled(splitPos);

  p_splitterWindow->SplitHorizontally(top, p_project, splitPos);

  topSizer->Add(p_splitterWindow, 1, wxGROW|wxALL, 0);
}


void SessionProjectContextPanel::refresh()
{
  p_project->refresh();
  if (p_session) p_session->refresh();
  if (p_workflow) p_workflow->refresh();
  p_project->p_projectView->enableMultiSelect(false);
}


void SessionProjectContextPanel::selectResource(const EcceURL & url)
{
  p_project->selectResource(url);
  if (p_session) p_session->selectResource(url);
  if (p_workflow) p_workflow->selectResource(url);
}


void SessionProjectContextPanel::selectResources(const vector<EcceURL>& urls)
{
  p_project->selectResources(urls);
}


vector<EcceURL> SessionProjectContextPanel::getSelections()
{
  return p_project->getSelections();
}




void SessionProjectContextPanel::loseFocus()
{
  p_project->loseFocus();
  if (p_session) p_session->loseFocus();
  if (p_workflow) p_workflow->loseFocus();
}


void SessionProjectContextPanel::OnSashPosChanged( wxSplitterEvent & event )
{
  Preferences prefs("Organizer");
  prefs.setInt(p_workflow ? "Organizer.WorkflowHeight"
                          : "Organizer.SessionHeight",
               (int)(p_splitterWindow->GetSashPosition() /
                     ewxBitmap::dpiScale() + 0.5));
  prefs.saveFile();
  event.Skip();
}
