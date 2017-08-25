#include <iostream>

#include "wx/dcmemory.h"
#include "wx/statusbr.h"
#include "wxgui/ewxBitmap.H"
#include "wxgui/WxState.H"
#include "wxgui/ewxStatusBar.H"
#include "wxgui/FeedbackSaveHandler.H"

BEGIN_EVENT_TABLE(ewxStatusBar, wxStatusBar)
  EVT_SIZE(ewxStatusBar::OnSize)
  EVT_BUTTON(wxID_SAVE, ewxStatusBar::OnButton)
END_EVENT_TABLE()

static const int BITMAP_SIZE_X = 20;
static const int BITMAP_SIZE_Y = 20;

ewxStatusBar::ewxStatusBar()
  : wxStatusBar(),
    ewxStyledWindow()
{
  Init();
}


ewxStatusBar::ewxStatusBar(wxWindow *parent, wxWindowID id, long style,
                           const wxString& name)
  : wxStatusBar(),
    ewxStyledWindow()
{
  Init();
  Create(parent, id, style, name);
}


bool ewxStatusBar::Create(wxWindow *parent, wxWindowID id, long style,
                          const wxString& name)
{
  if (!wxStatusBar::Create(parent, id, style, name)) {
    wxFAIL_MSG( wxT("ewxStatusBar creation failed") );
    return false;
  }

  p_save = new wxBitmapButton(this, wxID_SAVE, ewxBitmap("save.xpm"),
                              wxDefaultPosition, wxDefaultSize,
                              wxNO_BORDER|wxBU_EXACTFIT);
  p_runstate = new WxState(this);
  /*
  p_runstate = new wxBitmapButton(this, wxID_RESET, ewxBitmap("lock.xpm"),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxNO_BORDER|wxBU_EXACTFIT);
  */

  static const int widths[FIELD_MAX] = {BITMAP_SIZE_X, -1, BITMAP_SIZE_X, -1};
  static const int styles[FIELD_MAX] = {wxSB_FLAT, wxSB_NORMAL,
                                        wxSB_FLAT, wxSB_FLAT};
  SetFieldsCount(FIELD_MAX);
  SetStatusWidths(FIELD_MAX, widths);
  SetStatusStyles(FIELD_MAX, styles);
  SetMinHeight(BITMAP_SIZE_Y);

  setStyles(this);

  setEditStatus("EDIT");
  p_runstate->Show(false);

  return true;
}


void ewxStatusBar::Init()
{
  p_save = NULL;
  p_runstate = NULL;
  p_saveHandler = NULL;
}


ewxStatusBar::~ewxStatusBar()
{}


wxBitmap ewxStatusBar::CreateBitmapForButton(bool on)
{
  static const int BMP_BUTTON_SIZE_X = 10;
  static const int BMP_BUTTON_SIZE_Y = 9;

  wxBitmap bitmap(BMP_BUTTON_SIZE_X, BMP_BUTTON_SIZE_Y);
  wxMemoryDC dc;
  dc.SelectObject(bitmap);
  dc.SetBrush(on ? *wxGREEN_BRUSH : *wxRED_BRUSH);
  dc.SetBackground(*wxLIGHT_GREY_BRUSH);
  dc.Clear();
  dc.DrawEllipse(0, 0, BMP_BUTTON_SIZE_X, BMP_BUTTON_SIZE_Y);
  dc.SelectObject(wxNullBitmap);

  return bitmap;
}


void ewxStatusBar::OnSize(wxSizeEvent& event)
{
  wxRect rect;
  wxSize size;
  if (p_save) {
    GetFieldRect(FIELD_SAVE, rect);
    size = p_save->GetSize();
    p_save->Move(rect.x + (rect.width - size.x) / 2,
                 rect.y + (rect.height - size.y) / 2);
  }
  
  if (p_runstate) {
    GetFieldRect(FIELD_RUNSTATE, rect);
    size = p_runstate->GetSize();
    p_runstate->Move(rect.x + (rect.width - size.x) / 2,
                     rect.y + (rect.height - size.y) / 2);
  }

  event.Skip();
}


void ewxStatusBar::OnButton(wxCommandEvent& WXUNUSED(event))
{
  if (p_saveHandler) // && p_editStatus=="MODIFIED")
    p_saveHandler->processSave();
}


void ewxStatusBar::SetStatusText(const wxString& text, int i)
{
  switch (i) {
  case FIELD_SAVE:
    if (text == "READONLY" || text == "MODIFIED" || text == "EDIT")
      setEditStatus(text.c_str());
    else
      wxStatusBar::SetStatusText(text, FIELD_LOG);
    break;
    
  case FIELD_RUNSTATE:
    long state;
    if (text.ToLong(&state))
      setRunState(state);
    else
      wxStatusBar::SetStatusText(text, FIELD_LOG);
    break;

  default:
    wxStatusBar::SetStatusText(text, i);
  }
}


void ewxStatusBar::setSaveHandler(FeedbackSaveHandler *handler)
{
  p_saveHandler = handler;
}


void ewxStatusBar::setEditStatus(string editStatus)
{
  p_editStatus = editStatus;
  if (editStatus == "MODIFIED") {
    p_save->SetBitmapLabel(ewxBitmap("save.xpm"));
    p_save->Enable(true);
    p_save->Show(true);
  }
  else if (editStatus == "READONLY") {
    p_save->SetBitmapLabel(ewxBitmap("lock.xpm"));
    p_save->Enable(false);
    p_save->Show(true);
  }
  else if (editStatus == "EDIT") {
    // p_save->SetBitmapLabel(ewxBitmap("none.xpm"));
    // p_save->Enable(false);
    p_save->Show(false);
  }
  else {
    // Should never reach here!
    wxStatusBar::SetStatusText(editStatus, FIELD_LOG);
  }
  
}


void ewxStatusBar::setRunState(long state)
{
  p_runstate->Show(true);
  p_runstate->setRunState(state);
}


void ewxStatusBar::showRunState(bool show)
{
  p_runstate->Show(show);
}


void ewxStatusBar::setContext(string url)
{
  wxStatusBar::SetStatusText(url, FIELD_URL);
}


string ewxStatusBar::getEditStatus() const
{
  return p_editStatus;
}


ResourceDescriptor::RUNSTATE ewxStatusBar::getRunState() const
{
  return p_runstate->getRunState();
}


string ewxStatusBar::getContext() const
{
  return wxStatusBar::GetStatusText(FIELD_URL).c_str();
}
