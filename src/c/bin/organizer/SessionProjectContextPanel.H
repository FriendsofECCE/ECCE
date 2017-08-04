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

  void OnSashPosChanged( wxSplitterEvent & event );


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

  ProjectContextPanel * p_project;

  WxResourceTreeItemData * p_itemData;
};

#endif
// _SESSIONPROJECTCONTEXTPANEL_H_
