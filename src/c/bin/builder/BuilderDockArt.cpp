#include <wx/ewxaui/framemanager.H>

#include "util/Color.H"

#include "wxgui/ewxImage.H"

#include "BuilderDockArt.H"


BuilderDockArt::BuilderDockArt()
  : ewxAuiDefaultDockArt()
{
  m_inactive_open_in_bitmap = ewxImage::get(ewxImage::ARROW_DOWN);
  m_inactive_open_out_bitmap = ewxImage::get(ewxImage::ARROW_RIGHT);
  m_inactive_take_focus_in_bitmap = ewxImage::get(ewxImage::VIZ_FOCUS_IN);
  m_inactive_take_focus_out_bitmap = ewxImage::get(ewxImage::VIZ_FOCUS_OUT);
  m_inactive_add_focus_in_bitmap = ewxImage::get(ewxImage::VIZ_PIN_IN);
  m_inactive_add_focus_out_bitmap = ewxImage::get(ewxImage::VIZ_PIN_OUT);
  m_inactive_options_bitmap = ewxImage::get(ewxImage::MENU);
  m_active_open_in_bitmap = m_inactive_open_in_bitmap;
  m_active_open_out_bitmap = m_inactive_open_out_bitmap;
  m_active_take_focus_in_bitmap = m_inactive_take_focus_in_bitmap;
  m_active_take_focus_out_bitmap = m_inactive_take_focus_out_bitmap;
  m_active_add_focus_in_bitmap = m_inactive_add_focus_in_bitmap;
  m_active_add_focus_out_bitmap = m_inactive_add_focus_out_bitmap;
  m_active_options_bitmap = m_inactive_options_bitmap;

  SetColor(ewxAUI_DOCKART_BACKGROUND_COLOUR,
           wxColour(Color::WINDOW));
//  SetColor(ewxAUI_DOCKART_SASH_COLOUR,
//           wxColour(Color::WINDOW));
  SetColor(ewxAUI_DOCKART_ACTIVE_CAPTION_COLOUR,
           wxColour(Color::CAPTION));
  SetColor(ewxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR,
           wxColour(Color::CAPTION_GRADIENT));
  SetColor(ewxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,
           wxColour(Color::CAPTION));
  SetColor(ewxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,
           wxColour(Color::CAPTION_GRADIENT));
  SetColor(ewxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,
           wxColour(Color::TEXT));
  SetColor(ewxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,
           wxColour(Color::TEXT));
//  SetColor(ewxAUI_DOCKART_BORDER_COLOUR,
//           wxColour(Color::WINDOW));
//  SetColor(ewxAUI_DOCKART_GRIPPER_COLOUR,
//           wxColour(Color::WINDOW));
}


BuilderDockArt::~BuilderDockArt()
{
}

