#include <wx/dc.h>
#include <wx/aui/aui.h>

#include "util/Color.H"

#include "wxgui/ewxImage.H"

#include "BuilderDockArt.H"

// NOTE: see BuilderDockArt.H - the original ewxAUI-specific extra caption
// button bitmaps (open/close, take-focus, add-focus/pin, options) have no
// stock wx3.2 wxAuiDefaultDockArt equivalent and are dropped here; only the
// color customization (which does map directly onto stock wxAUI_DOCKART_*
// constants) is preserved.

BuilderDockArt::BuilderDockArt()
  : wxAuiDefaultDockArt()
{
  SetColor(wxAUI_DOCKART_BACKGROUND_COLOUR,
           wxColour(Color::WINDOW));
//  SetColor(wxAUI_DOCKART_SASH_COLOUR,
//           wxColour(Color::WINDOW));
  SetColor(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR,
           wxColour(Color::CAPTION));
  SetColor(wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR,
           wxColour(Color::CAPTION_GRADIENT));
  SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,
           wxColour(Color::CAPTION));
  SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,
           wxColour(Color::CAPTION_GRADIENT));
  SetColor(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,
           wxColour(Color::TEXT));
  SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,
           wxColour(Color::TEXT));
//  SetColor(wxAUI_DOCKART_BORDER_COLOUR,
//           wxColour(Color::WINDOW));
//  SetColor(wxAUI_DOCKART_GRIPPER_COLOUR,
//           wxColour(Color::WINDOW));
}


BuilderDockArt::~BuilderDockArt()
{
}


// The pin slot is the fold toggle: a triangle pointing down when the
// pane is open and right when it is folded.
void BuilderDockArt::DrawPaneButton(wxDC& dc, wxWindow* window, int button,
                                    int buttonState, const wxRect& rect,
                                    wxAuiPaneInfo& pane)
{
  if (button != wxAUI_BUTTON_PIN) {
    wxAuiDefaultDockArt::DrawPaneButton(dc, window, button, buttonState,
                                        rect, pane);
    return;
  }
  if (buttonState == wxAUI_BUTTON_STATE_HIDDEN) {
    return;
  }

  const bool folded = p_isFolded && p_isFolded(pane.window);
  const wxColour text = GetColour(pane.state & wxAuiPaneInfo::optionActive
                                  ? wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR
                                  : wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR);

  if (buttonState & (wxAUI_BUTTON_STATE_HOVER | wxAUI_BUTTON_STATE_PRESSED)) {
    const bool pressed = (buttonState & wxAUI_BUTTON_STATE_PRESSED) != 0;
    dc.SetPen(wxPen(text));
    dc.SetBrush(wxBrush(text.ChangeLightness(pressed ? 180 : 160)));
    dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
  }

  const int s = wxMin(rect.width, rect.height) / 3;  // half-extent
  const int cx = rect.x + rect.width / 2;
  const int cy = rect.y + rect.height / 2;
  wxPoint tri[3];
  if (folded) {
    tri[0] = wxPoint(cx - s / 2, cy - s);
    tri[1] = wxPoint(cx - s / 2, cy + s);
    tri[2] = wxPoint(cx + s, cy);
  } else {
    tri[0] = wxPoint(cx - s, cy - s / 2);
    tri[1] = wxPoint(cx + s, cy - s / 2);
    tri[2] = wxPoint(cx, cy + s);
  }
  dc.SetPen(wxPen(text));
  dc.SetBrush(wxBrush(text));
  dc.DrawPolygon(3, tri);
}
