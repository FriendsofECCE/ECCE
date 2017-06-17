/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/ResourceUtils.H"
#include "util/NullPointerException.H"

#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"

#include "wxgui/ewxStaticText.H"
#include "wxgui/ForwardMouseEventHandler.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxState.H"

#include "MDTaskPanel.H"
#include "SessionContextPanel.H"
#include "CalcMgr.H"


/**
 * Constructor.  Should not be used.
 */
MDTaskPanel::MDTaskPanel( )
{
}


/**
 * Constructor.
 */
MDTaskPanel::MDTaskPanel( Resource *task,
                          SessionContextPanel * sessionPanel,
                          wxWindow *parent,
                          wxWindowID id,
                          const wxPoint& pos,
                          const wxSize &size,
                          long style ):
  MDTaskPanelGUI(parent, id, pos, size, style)
{
  p_task = task;
  p_parent = sessionPanel;

  SetName(p_task->getURL().toString());
  createControls();
  setBackground(MDTaskPanel::UNSELECTED);
  connectEventHandlers();
}


/**
 * Destructor.
 */
MDTaskPanel::~MDTaskPanel()
{
  p_task = 0;
}


/**
 * Retrieve the task represented by this panel.
 *
 * @return  pointer to task represented by this panel
 */
Resource* MDTaskPanel::getTask()
{
  return p_task;
}


/**
 * Retrieve the state icon for this panel.
 *
 * @return  pointer to state icon represented by this panel
 */
WxState* MDTaskPanel::getState()
{
  return p_state;
}


/**
 * Creates remaining controls to be displayed within the mdtask panel.
 */
void MDTaskPanel::createControls()
{
  // we want the whole panel surface to be clickable, so the panel's children
  // need to respond to clicks in a transparent way (the children would capture
  // these events normally).  We do this by replacing their default event
  // handling.

  // for the next steps, we need the Task to be at least a TaskJob
  TaskJob *taskjob = dynamic_cast<TaskJob*>(p_task);
  NULLPOINTEREXCEPTION(taskjob, "MDTaskPanel used without taskjob");

  // add the state icon
  p_state = new WxState(this);
  p_state->setRunState(taskjob->getState());
  p_nameSizer->Prepend(p_state, 0, wxALL, 2);

  // set the name of this task to display
  p_name->SetLabel(_T(taskjob->getName()));

  // add the code/application bitmap
  p_codeBitmap->SetBitmap(WxResourceImageList::getBitmap(taskjob));

  // set the name of the code/application to display
  string codeName;
  codeName = taskjob->getProp(VDoc::getEcceNamespace() + ":application");
  p_code->SetLabel(_T(codeName));

  // adjust the sizer for this task panel dynamically
  p_topSizer->Layout();
  Fit();
}


/**
 * Sets the event handling functions for all panel children.
 */
void MDTaskPanel::connectEventHandlers()
{
  Connect(wxEVT_PAINT,
          wxPaintEventHandler(MDTaskPanel::OnPaint));
  Connect(wxEVT_LEFT_DOWN,
          wxMouseEventHandler(MDTaskPanel::OnLeftClick));
  Connect(wxEVT_LEFT_DCLICK,
          wxMouseEventHandler(MDTaskPanel::OnLeftDClick));
  Connect(wxEVT_RIGHT_DOWN,
          wxMouseEventHandler(MDTaskPanel::OnRightClick));
  p_state->SetEventHandler(new ForwardMouseEventHandler(this));
  p_name->SetEventHandler(new ForwardMouseEventHandler(this));
  p_codeBitmap->SetEventHandler(new ForwardMouseEventHandler(this));
  p_code->SetEventHandler(new ForwardMouseEventHandler(this));
}


/**
 * Handles painting.
 */
void MDTaskPanel::OnPaint( wxPaintEvent& event )
{
  p_state->refresh();
  event.Skip();
}


/**
 * Handles when single left click of mouse.
 */
void MDTaskPanel::OnLeftClick( wxMouseEvent& event )
{
  wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_PANEL_SELECTION);
  evt.SetString(p_task->getURL().toString());
  evt.SetInt(1);
  AddPendingEvent(evt);
  SetFocus();
}


/**
 * Handles left double clicking of mouse.
 */
void MDTaskPanel::OnLeftDClick( wxMouseEvent& event )
{
  // Change the context panel of the organizer to the selection
  // @todo lisong
  // Consider findtoplevel window to find p_calcmgr or use treectrl event.
  // Better not use friend class this way
  p_parent->getCalcMgr()->findNode(p_task->getURL(), true);
}


/**
 * Handles right click of the mouse.  Raises a context panel when appropriate.
 */
void MDTaskPanel::OnRightClick( wxMouseEvent& event )
{
  wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_PANEL_SELECTION);
  evt.SetString(p_task->getURL().toString());
  evt.SetInt(1);
  ProcessEvent(evt);

  wxMenu menu;
  p_parent->getCalcMgr()->getContextMenu(menu);
  p_parent->PopupMenu(&menu);
}


void MDTaskPanel::setBackground(STATE state)
{
  wxColour bgcolor, fgcolor;
  if (state == UNSELECTED) {
    bgcolor = ewxStyledWindow::getWindowColor();
    fgcolor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
  }
  else if (state == SELECTED) {
    bgcolor = ewxStyledWindow::getFocusedSelectionColor();
    fgcolor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
  }
  else if (state == LOSTFOCUS) {
    bgcolor = ewxStyledWindow::getUnfocusedSelectionColor();
    fgcolor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
  }
  SetBackgroundColour(bgcolor);
  p_name->SetForegroundColour(fgcolor);
  p_code->SetForegroundColour(fgcolor);
  getState()->SetBackgroundColour(bgcolor);
}
