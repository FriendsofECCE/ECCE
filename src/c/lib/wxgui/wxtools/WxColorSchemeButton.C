/**
 * @file
 *
 * Render a color scheme button.
 *
 */
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxColorSchemeButton.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/WxColorSchemeButton.H"
#include "wxgui/WxColorSchemeChooser.H"

IMPLEMENT_DYNAMIC_CLASS( WxColorSchemeButton, ewxPanel )

BEGIN_EVENT_TABLE( WxColorSchemeButton, ewxPanel )

  EVT_PAINT       ( WxColorSchemeButton::OnPaint )
  EVT_LEFT_DOWN   ( WxColorSchemeButton::OnMouseLeftDown )

END_EVENT_TABLE()


/*!
 * WxColorSchemeButton constructors
 */
WxColorSchemeButton::WxColorSchemeButton( )
{
}


WxColorSchemeButton::WxColorSchemeButton( wxWindow* parent,
                                          wxWindowID id,
                                          const wxPoint& pos,
                                          const wxSize& size,
                                          long style )
{
  ewxPanel::Create(parent, id + SCHEME_BUTTON_BASE, pos, size, style);
  setSelected(false);
}


/**
 * Respond to a paint event - draw border and color bands
 */
void WxColorSchemeButton::OnPaint( wxPaintEvent& event )
{
  wxPaintDC dc(this);
  PrepareDC(dc);

  int width = GetSize().GetX();
  int height = GetSize().GetY();

  // Draw color bands
  int numColor = p_colors.size();
  int laststop = 0;
  for (int i=0; i<numColor; i++) {
    int tmp = laststop;
    laststop = (i+1)*(width-4)/numColor;
    if (tmp == laststop)
      continue;
    dc.SetPen(p_colors[i]);
    dc.SetBrush(p_colors[i]);
    dc.DrawRectangle(tmp+2, 2, laststop - tmp, height-4);
  }

  // Draw border
  drawButtonBorder(dc, p_selected);

  event.Skip();
}


void WxColorSchemeButton::OnMouseLeftDown( wxMouseEvent& event )
{
  //  setSelected(!p_selected);
  WxColorSchemeChooser * parent = 
    dynamic_cast<WxColorSchemeChooser *>(GetParent());
  if (parent)
    parent->onSchemeSelected(GetId() - SCHEME_BUTTON_BASE);
  event.Skip();
}


void WxColorSchemeButton::setSelected( bool selected )
{
  p_selected = selected;
  Refresh();
}


void WxColorSchemeButton::addColor( const wxColour & color )
{
  p_colors.push_back(color);
}


void WxColorSchemeButton::addColors( const vector<wxColour> & colors )
{
  vector<wxColour>::const_iterator colItor = colors.begin();
  for (; colItor != colors.end(); colItor++)
    p_colors.push_back(*colItor);
}


void WxColorSchemeButton::addColors( const vector<string> & colors )
{
  vector<string>::const_iterator colItor = colors.begin();
  for (; colItor != colors.end(); colItor++)
    p_colors.push_back(wxColour(*colItor));
}


void WxColorSchemeButton::addColors( const vector<char *> & colors )
{
  vector<char *>::const_iterator colItor = colors.begin();
  for (; colItor != colors.end(); colItor++)
    p_colors.push_back(wxColour(*colItor));
}

void WxColorSchemeButton::setColors( const vector<wxColour> & colors )
{
  p_colors.clear();
  vector<wxColour>::const_iterator colItor = colors.begin();
  for (; colItor != colors.end(); colItor++)
    p_colors.push_back(*colItor);
}

void WxColorSchemeButton::setColors( const vector<string> & colors )
{
  p_colors.clear();
  vector<string>::const_iterator colItor = colors.begin();
  for (; colItor != colors.end(); colItor++)
    p_colors.push_back(wxColour(*colItor));
}

vector<wxColour> WxColorSchemeButton::getColors() const
{
   return p_colors;
}
