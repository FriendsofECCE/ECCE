#ifndef _SESSIONPROJECTCONTEXTPANEL_H_
#define _SESSIONPROJECTCONTEXTPANEL_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SessionProjectContextPanel.C"
#endif


#include "wx/splitter.h"

#include "ContextPanelInterface.H"

class CalcMgr;
class WxResourceTreeItemData;
class SessionContextPanel;
class WorkflowPanel;
class ProjectContextPanel;


/*!
 * Compatibility
 */
#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif


/*!
 * SessionProjectContextPanel class declaration
 */
class SessionProjectContextPanel: public ewxPanel,
                                  public ContextPanelInterface
{    
  DECLARE_EVENT_TABLE()

public:
  friend class ContextPanelFactory;

  ~SessionProjectContextPanel();

  virtual void refresh();

  virtual vector<EcceURL> getSelections();

  virtual void loseFocus();

  virtual void selectResource(const EcceURL & url);
  virtual void selectResources(const vector<EcceURL>& urls);

  void OnSashPosChanged( wxSplitterEvent & event );

  WorkflowPanel * getWorkflowPanel() { return p_workflow; }


protected:
  /// Constructors
  SessionProjectContextPanel( CalcMgr * calcMgr,
                              WxResourceTreeItemData *itemData, 
                              wxWindow *parent, wxWindowID id = -1,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize, 
                              long style = wxTAB_TRAVERSAL );
  
  /// Creates the controls and sizers
  void CreateControls();

  wxSplitterWindow * p_splitterWindow;

  SessionContextPanel * p_session;

  /// Set instead of p_session when the session is a Workflow.
  WorkflowPanel * p_workflow;

  ProjectContextPanel * p_project;

  WxResourceTreeItemData * p_itemData;
};

#endif
// _SESSIONPROJECTCONTEXTPANEL_H_
