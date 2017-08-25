#include <wx/menu.h>
#include <wx/minifram.h>
#include <wx/sizer.h>
#include <wx/toolbar.h>

#include "wxgui/ewxImage.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/TearableContent.H"


IMPLEMENT_DYNAMIC_CLASS(TearButton, ewxPanel)


BEGIN_EVENT_TABLE(TearButton, ewxPanel)
  EVT_PAINT(TearButton::OnPaint)
  EVT_LEFT_DOWN(TearButton::OnLeftDown)
END_EVENT_TABLE()


TearButton::TearButton()
  : ewxPanel(),
    p_bitmap(wxNullBitmap)
{
}


TearButton::TearButton(wxWindow *parent, wxWindowID id, long orientation)
  : ewxPanel(),
    p_bitmap(wxNullBitmap)
{
  Create(parent, id, orientation);
}


TearButton::~TearButton()
{
}


bool TearButton::Create(wxWindow *parent, wxWindowID id, long orientation)
{
  if (!ewxPanel::Create(parent, id)) {
    wxFAIL_MSG( wxT("TearButton creation failed") );
    return false;
  }

  SetWindowStyle(orientation);

  if (orientation & wxHORIZONTAL) {
    p_bitmap = ewxImage::get(ewxImage::TEAR);
    SetSize(wxSize(-1,p_bitmap.GetHeight()));
  }
  else if (orientation & wxVERTICAL) {
    p_bitmap = ewxImage::get(ewxImage::TEAR).Rotate90();
    SetSize(wxSize(p_bitmap.GetWidth(),-1));
  }

  return true;
}


void TearButton::OnPaint(wxPaintEvent& event)
{
  wxSize size = GetSize();
  wxPaintDC dc(this);
  PrepareDC(dc);
  if (GetWindowStyle() & wxHORIZONTAL) {
    int x = 0;
    while (x < size.GetWidth()) {
      dc.DrawBitmap(p_bitmap,x,0,true);
      x+=p_bitmap.GetWidth();  // needs to be size of pixmap
    }
  } else {
    int y = 0;
    while (y < size.GetHeight()) {
      dc.DrawBitmap(p_bitmap,0,y,true);
      y+=p_bitmap.GetHeight();  // needs to be size of pixmap
    }
  }
}


void TearButton::OnLeftDown(wxMouseEvent& event)
{
  wxCommandEvent newEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  newEvent.SetEventObject(this);
  ProcessEvent(newEvent);
}


IMPLEMENT_DYNAMIC_CLASS(TearFrame, wxMiniFrame)


BEGIN_EVENT_TABLE(TearFrame, wxMiniFrame)
  EVT_CLOSE(TearFrame::OnClose)
END_EVENT_TABLE()


/**
 * DO NOT USE.
 */
TearFrame::TearFrame()
  : wxMiniFrame()
{
}


TearFrame::TearFrame(wxWindow * parent, const wxString& caption)
  : wxMiniFrame()
{
  Create(parent, caption);
}


TearFrame::~TearFrame()
{
}


bool TearFrame::Create(wxWindow * parent, const wxString& caption)
{
  if (!wxMiniFrame::Create(parent, wxID_ANY, caption, wxGetMousePosition(),
            wxDefaultSize, wxCAPTION|wxRESIZE_BORDER|wxCLOSE_BOX)) {
    wxFAIL_MSG( wxT("TearFrame creation failed") );
    return false;
  }

  SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

  return true;
}


void TearFrame::OnClose(wxCloseEvent& event)
{
  TransferDataFromWindow();
  event.Skip();
}


const wxWindowID TearableContent::ID_TEAR = wxNewId();


IMPLEMENT_DYNAMIC_CLASS(TearableContent, wxPopupTransientWindow)


BEGIN_EVENT_TABLE(TearableContent, wxPopupTransientWindow)
  EVT_BUTTON(ID_TEAR, TearableContent::OnTearClick)
  EVT_TOOL(wxID_ANY, TearableContent::OnToolClick)
  EVT_BUTTON(wxID_ANY, TearableContent::OnToolClick)
END_EVENT_TABLE()


/**
 * DO NOT USE.
 */
TearableContent::TearableContent()
  : wxPopupTransientWindow(),
    p_content(NULL)
{
}


/**
 * Reparents content and adds it to this tearable window.
 * Searches for any window with the given content's name in order to 
 * destroy multiple copies of the torn content.
 */
TearableContent::TearableContent(wxWindow *content)
  : wxPopupTransientWindow(content->GetParent()),
    p_content(NULL)
{
  SetContent(content);
}


/**
 * The passed menu is translated into a wxToolBar.
 */
TearableContent::TearableContent(wxWindow *parent, wxMenu *menu)
  : wxPopupTransientWindow(parent),
    p_content(NULL)
{
  SetContent(ToolBarFromMenu(parent,menu));
}


/**
 * Assign dangling pointers to NULL.
 */
TearableContent::~TearableContent()
{
  p_content = NULL;
}


void TearableContent::Popup(wxWindow * focus) 
{
  p_content->TransferDataToWindow();
  wxPopupTransientWindow::Popup(focus);
}


void TearableContent::SetContent(wxWindow *content)
{
  p_content = content;

  wxWindow * existingContent = GetParent()->FindWindow(GetFrameName());
  if (existingContent) {
    existingContent->Hide();
    existingContent->Destroy();
  }

  ewxPanel * panel = new ewxPanel(this);
  wxSizer * sizer = new wxBoxSizer(wxVERTICAL);

  TearButton * tearButton = new TearButton(panel, ID_TEAR);
  sizer->Add(tearButton, 0, wxGROW|wxALL, 0);

  content->Reparent(panel);
  sizer->Add(content, 1, wxGROW|wxALL, 0);

  panel->SetSizerAndFit(sizer);
  SetSize(panel->GetBestSize());
}


/**
 * Does not handle submenus nor does it transfer any state from the given
 * menu (e.g. IsChecked()).
 */
wxToolBar * TearableContent::ToolBarFromMenu(wxWindow *parent, wxMenu *menu)
{
  wxToolBar *tb = new wxToolBar(parent, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          menu->GetTitle().IsEmpty() ? "No Menu Title" : menu->GetTitle());

  wxMenuItemList list = menu->GetMenuItems();
  wxMenuItemList::iterator iter = list.begin();
  if (! menu->GetTitle().IsEmpty()) {
    iter++; // skip title item
    iter++; // skip title separator
  }
  while (iter != list.end()) {
    wxMenuItem *item = *iter;
    tb->AddTool(item->GetId(), item->GetLabel(), item->GetBitmap(),
                item->GetHelp(), item->GetKind());
    ++iter;
  }

  return tb;
}


wxString TearableContent::GetFrameName()
{
  if (p_content) {
    return p_content->GetName() + " Tear-Off Frame";
  }
  return "Tear-Off Frame";
}


void TearableContent::OnTearClick(wxCommandEvent& event)
{
  wxString name;
  if (p_content) {
    name = p_content->GetName();
  } else {
    name = "No Name - Menu TODO";
  }

  TearFrame * frame = new TearFrame(GetParent(), name);
  frame->SetName(GetFrameName());

  if (p_content) {
    p_content->Reparent(frame);
    frame->SetSizer(new wxBoxSizer(wxHORIZONTAL));
    frame->GetSizer()->Add(p_content, 1, wxGROW|wxALL, 0);
    frame->GetSizer()->Fit(frame);
  }

  // Show the new frame
  frame->Show();

  // Dismiss the popup
  Dismiss();
}


/**
 * Convenience function for user of this tearable content to popdown when done.
 * If user doesn't want to popdown on tool click, then don't use event.Skip
 * in event handler further up in the event handling chain.
 */
void TearableContent::OnToolClick(wxCommandEvent& event)
{
  // create a new event to avoid crashes
  wxCommandEvent newEvent(event.GetEventType(), event.GetId());
  newEvent.SetTimestamp(event.GetTimestamp());
  newEvent.SetClientData(event.GetClientData());
  newEvent.SetClientObject(event.GetClientObject());
  newEvent.SetExtraLong(event.GetExtraLong());
  newEvent.SetInt(event.GetInt());
  newEvent.SetString(event.GetString());
  GetParent()->GetEventHandler()->AddPendingEvent(newEvent);
  DismissAndNotify();
}


/**
 * Destroy upon non-programmatic dismissal.
 */
void TearableContent::OnDismiss()
{
  p_content->TransferDataFromWindow();
  wxPopupTransientWindow::OnDismiss();
  Destroy();
}

