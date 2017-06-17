#include "wx/listctrl.h"
#include "wxgui/ewxListViewComboPopup.H"
#include "wxgui/ewxStyledWindow.H"

BEGIN_EVENT_TABLE(ewxListViewComboPopup, wxListView)
  EVT_MOTION(ewxListViewComboPopup::OnMouseMove)
  // NOTE: Left down event is used instead of left up right now
  //       since MSW wxListCtrl doesn't seem to emit left ups
  //       consistently.
  EVT_LEFT_DOWN(ewxListViewComboPopup::OnMouseClick)
END_EVENT_TABLE()


ewxListViewComboPopup::ewxListViewComboPopup(int id)
{
  m_comboId = id;
}


void ewxListViewComboPopup::Init()
{
  m_value = -1;
  m_itemHere = -1; // hot item in list
}


bool ewxListViewComboPopup::Create( wxWindow* parent )
{
  bool ret = wxListView::Create(parent,1,
                                wxPoint(0,0),wxDefaultSize,
                                wxLC_LIST|wxLC_SINGLE_SEL|
                                wxLC_SORT_ASCENDING|wxSIMPLE_BORDER);
  wxListView::SetBackgroundColour(ewxStyledWindow::getInputColor());
  return ret;
}


wxWindow * ewxListViewComboPopup::GetControl()
{
  return this;
}


void ewxListViewComboPopup::SetStringValue( const wxString& s )
{
  int n = wxListView::FindItem(-1,s);
  if ( n >= 0 && n < GetItemCount() )
    wxListView::Select(n);
}


wxString ewxListViewComboPopup::GetStringValue() const
{
  if ( m_value >= 0 )
    return wxListView::GetItemText(m_value);
  return wxEmptyString;
}


//
// Popup event handlers
//

// Mouse hot-tracking
void ewxListViewComboPopup::OnMouseMove(wxMouseEvent& event)
{
  // Move selection to cursor if it is inside the popup
  int resFlags;
  int itemHere = HitTest(event.GetPosition(),resFlags);
  if ( itemHere >= 0 )
  {
    wxListView::Select(itemHere,true);
    m_itemHere = itemHere;
  }
  event.Skip();
}


// On mouse left, set the value and close the popup
void ewxListViewComboPopup::OnMouseClick(wxMouseEvent& WXUNUSED(event))
{
  m_value = m_itemHere;

  //Not sure what is the right event to send
  wxListEvent evt(wxEVT_COMMAND_TEXT_ENTER, m_comboId);
  //wxListEvent evt(wxEVT_COMMAND_LIST_ITEM_SELECTED, GetId());
  evt.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(evt);
  //ProcessEvent(evt);

  Dismiss();
}


void ewxListViewComboPopup::AddSelection( const wxString& selstr )
{
  wxListView::InsertItem(GetItemCount(),selstr);
}
