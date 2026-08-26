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
