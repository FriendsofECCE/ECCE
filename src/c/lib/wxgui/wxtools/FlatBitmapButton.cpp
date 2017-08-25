#include <wx/sizer.h>
#include <wx/statbmp.h>

#include "wxgui/ewxBitmap.H"
#include "wxgui/FlatBitmapButton.H"
#include "wxgui/ForwardMouseEventHandler.H"


IMPLEMENT_DYNAMIC_CLASS(FlatBitmapButton, ewxPanel)


BEGIN_EVENT_TABLE( FlatBitmapButton, ewxPanel )
  EVT_LEFT_DOWN     (FlatBitmapButton::OnMouseLeftDown)
  EVT_LEFT_UP       (FlatBitmapButton::OnMouseLeftUp)
  EVT_RIGHT_DOWN    (FlatBitmapButton::OnMouseRightDown)
  EVT_ENTER_WINDOW  (FlatBitmapButton::OnMouseEnterWindow)
  EVT_LEAVE_WINDOW  (FlatBitmapButton::OnMouseLeaveWindow)
  EVT_PAINT         (FlatBitmapButton::OnPaint)
//  EVT_MENU          (ID_ECCETOOL_NEW, FlatBitmapButton::OnMenuClick)
END_EVENT_TABLE()


/**
 * Constructor.
 * Border will be drawn by default.
 */
FlatBitmapButton::FlatBitmapButton() 
  : ewxPanel()
  , p_buttonSizer(NULL)
  , p_bitmap(NULL)
  , p_isArmed(false)
  , p_showBorder(true)
{
}


/**
 * Constructor.
 * Border will be drawn by default.
 */
FlatBitmapButton::FlatBitmapButton(wxWindow* parent, wxWindowID id, 
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : ewxPanel()
  , p_buttonSizer(NULL)
  , p_bitmap(NULL)
  , p_isArmed(false)
  , p_showBorder(true)
{
  Create(parent, id, pos, size, style, name);
  p_isArmed = false;
  p_showBorder = true;
}


FlatBitmapButton::~FlatBitmapButton() 
{
}


bool FlatBitmapButton::Create(wxWindow * parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!ewxPanel::Create( parent, id, pos, size, style, name )) {
    wxFAIL_MSG( wxT("FlatBitmapButton creation failed") );
    return false;
  }

  p_buttonSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(p_buttonSizer);

  // Hardwired default bitmap
  ewxBitmap bitmap("lock.xpm",wxBITMAP_TYPE_XPM);
  p_bitmap = new wxStaticBitmap( this,
          wxID_STATIC, bitmap, wxDefaultPosition, wxDefaultSize, 0 );
  p_bitmap->PushEventHandler(new ForwardMouseEventHandler(this));

  p_buttonSizer->Add(p_bitmap, 0, wxGROW|wxALL, 2);

  return true;
}


void FlatBitmapButton::showBorder(bool flag)
{
  p_showBorder = flag;
  Refresh();
}


/**
 * Show button as armed.
 */
void FlatBitmapButton::OnMouseLeftDown( wxMouseEvent& event )
{
  showArmed(true);
  event.Skip();
}


/**
 * Show button as un-armed.
 */
void FlatBitmapButton::OnMouseLeftUp( wxMouseEvent& event )
{
  showArmed(false);
  wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  evt.SetEventObject(this);
  ProcessEvent(evt);
  event.Skip();
}


/**
 * No action is taken.
 */
void FlatBitmapButton::OnMouseEnterWindow( wxMouseEvent& event )
{
  event.Skip();
}


/**
 * The button is changed to un-armed.  This may not be the best thing to
 * do since it may become unarmed before it should but this does eliminate
 * potential problems with buttons being left armed if the user clicks then
 * drags off without triggering left button up.
 */
void FlatBitmapButton::OnMouseLeaveWindow( wxMouseEvent& event )
{
  showArmed(false);
  event.Skip();
}


/**
 * No action is taken.
 */
void FlatBitmapButton::OnMouseRightDown( wxMouseEvent& event )
{
  event.Skip();
}


/**
 * No action is taken.
 */
void FlatBitmapButton::OnMenuClick( wxCommandEvent& event)
{
  event.Skip();
}


void FlatBitmapButton::SetBitmap(wxBitmap bitmap)
{
  p_buttonSizer->Detach(p_bitmap);
  if (p_bitmap) delete p_bitmap;
  p_bitmap = new wxStaticBitmap( this, wxID_STATIC, bitmap,
                                 wxDefaultPosition, wxDefaultSize, 0 );
  p_bitmap->SetEventHandler(new ForwardMouseEventHandler(this));
  p_buttonSizer->Add(p_bitmap, 0, wxGROW|wxALL, 2);
  Layout();
}


/**
 * Show button as pushed in.
 */
void FlatBitmapButton::showArmed(bool isArmed)
{
  p_isArmed = isArmed;
  Refresh();
}


/**
 * Find out if button is of pushed-in style.
 */
bool FlatBitmapButton::isArmed() const
{
  return p_isArmed;
}


/**
 * Update the UI.
 * If showBorder was called with true, a 3D border will be drawn taking into
 * account whether it should be drawn as armed(pressed) or not.
 */
void FlatBitmapButton::OnPaint( wxPaintEvent& event )
{
  if (p_showBorder) {
    wxPaintDC dc(this);
    PrepareDC(dc);
    drawButtonBorder(dc, p_isArmed);
  }
  event.Skip();
}

