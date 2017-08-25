/**
 * @file
 *
 * Periodic table element button class.
 *
 *
 */

#include "tdat/TPerTab.H"

#include "util/StringConverter.H"

#include "wxgui/ewxMenu.H"
#include "wxgui/ewxPanel.H"

#include "wxgui/PerTabPanel.H"


BEGIN_EVENT_TABLE( ElementButton, ewxPanel )

  EVT_LEFT_DOWN    (ElementButton::OnMouseLeftDown)
  EVT_LEFT_UP      (ElementButton::OnMouseLeftUp)
  EVT_CONTEXT_MENU (ElementButton::OnContextMenu)
  EVT_ENTER_WINDOW (ElementButton::OnMouseEnterWindow)
  EVT_LEAVE_WINDOW (ElementButton::OnMouseLeaveWindow)
  EVT_PAINT        (ElementButton::OnPaint)

END_EVENT_TABLE()


/**
 * Destructor
 */
ElementButton::~ElementButton() {}


/**
 * Constructor
 *
 * Initialize the panel, save the parameters.
 */
ElementButton::ElementButton(int atomicNum, PerTabPanel * parent,
                             wxWindowID id, const wxPoint& pos,
                             const wxSize& size, long style)
{
  ewxPanel::Create( parent, id, pos, size, style );

  p_atomicNum = atomicNum;
  p_parent = parent;
  p_isSunken = false;

  p_atomicNumStr = StringConverter::toString(atomicNum);
  p_symbol = parent->getTPerTab()->atomicSymbol(atomicNum);
  clearSizeInfo();
}


/**
 * Set the status of the button
 */
void ElementButton::setStatus(bool isSunken)
{
  p_isSunken = isSunken;
  Refresh();
}


/**
 * Get the status of the button
 */
bool ElementButton::getStatus() const
{
  return p_isSunken;
}


/**
 * Event handling function for mouse left down event
 */
void ElementButton::OnMouseLeftDown( wxMouseEvent& event )
{
  p_isSunken = !p_isSunken;
  Refresh();
  if (p_isSunken) {
    p_parent->setCurrentElement(p_atomicNum);
  }
}


/**
 * Event handling function for mouse left up event
 */
void ElementButton::OnMouseLeftUp( wxMouseEvent& event )
{
  p_parent->selectElement(p_atomicNum, p_isSunken);
  p_parent->fireSelectionChanged(p_atomicNum);
}


/**
 * Event handling function for mouse enter window event
 */
void ElementButton::OnMouseEnterWindow( wxMouseEvent& event )
{
  if (event.LeftIsDown()) {
    p_isSunken = !p_isSunken;
    Refresh();
    if (p_isSunken)
      p_parent->setCurrentElement(p_atomicNum);
  }
}


/**
 * Event handling function for mouse leave window event
 */
void ElementButton::OnMouseLeaveWindow( wxMouseEvent& event )
{
  if (event.LeftIsDown()) {
    p_isSunken = !p_isSunken;
    Refresh();
  }
}


/**
 * Event handling function for mouse right down event
 */
void ElementButton::OnContextMenu( wxContextMenuEvent& event )
{
  wxWindow *gparent;
  string gpname = "";
  if ((gparent = p_parent->GetGrandParent()) != NULL) {
    gpname = gparent->GetName();
  }

  if (gpname != "popup") {
    PopupMenu(p_parent->getDataMenu());
    p_parent->setCurrentElement(p_atomicNum);
  }
}


/**
 * Event handling function for paint
 *
 * This function will do the following task:
 *   Paint the background using a proper color.
 *   Draw the atomic number if enabled.
 *   Draw the atomic symbol.
 *   Draw the button border based on the button status.
 *   Draw the triangle mark if enabled.
 */
void ElementButton::OnPaint( wxPaintEvent& event )
{
  wxPaintDC dc(this);
  PrepareDC(dc);
  
  int borderWidth = p_parent->isMini() ? 1 : 2;

  // Paint the background using a proper color.
  dc.SetPen(wxPen(p_parent->getElementColor(p_atomicNum, p_isSunken)));
  dc.SetBrush(wxBrush(p_parent->getElementColor(p_atomicNum, p_isSunken)));
  dc.DrawRectangle(borderWidth, borderWidth,
                   GetSize().GetX()-borderWidth*2,
                   GetSize().GetY()-borderWidth*2);

  // Initialize the width, run only once
  if (p_numHeight == 0) {
    if (p_parent->isMini()) {
      p_numWidth = 1;
      p_numHeight = 1;
      dc.SetFont(getSmallLabelFont());
    }
    else {
      dc.SetFont(getAtomicNumFont());
      dc.GetTextExtent(p_atomicNumStr, &p_numWidth, &p_numHeight);
      dc.SetFont(getBoldFont());
    }
    dc.GetTextExtent(p_symbol, &p_symbolWidth, &p_symbolHeight);
  }

  int width = p_symbolWidth + borderWidth * 2;
  int height = p_symbolHeight + borderWidth * 2;

  if (p_parent->isShowNumber()) {
    width = (p_symbolWidth>p_numWidth ? p_symbolWidth:p_numWidth) +
      borderWidth * 2;
    height += p_numHeight;
  }
  SetMinSize(wxSize(width, height));

  // Draw the atomic number if enabled.
  dc.SetPen(wxPen(getTextColor()));
  if (p_parent->isShowNumber()) {
    dc.SetFont(getAtomicNumFont());
    dc.DrawText(p_atomicNumStr, borderWidth, borderWidth);
  }

  // Draw the atomic symbol.
  if (p_parent->isMini())
    dc.SetFont(getSmallLabelFont());
  else
    dc.SetFont(getBoldFont());

  wxCoord left = (GetSize().GetX() - p_symbolWidth)/2;
  wxCoord top = (GetSize().GetY() - p_symbolHeight)/2;
  if (p_parent->isShowNumber())
    top += p_numHeight/2;
  dc.DrawText(p_symbol, left, top);

  drawButtonBorder(dc, p_isSunken, borderWidth);

  // Draw the triangle mark if enabled.
  if (p_parent->isShowMark(p_atomicNum)) {
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(p_parent->getContextColor(p_isSunken)));
    wxPoint triangle[3] = 
      {wxPoint(borderWidth, GetSize().GetY() - borderWidth),
       wxPoint(GetSize().GetX()/2, GetSize().GetY() - borderWidth),
       wxPoint(borderWidth, GetSize().GetY() * 2 / 3)};
    dc.DrawPolygon(3, triangle);
  }
}


void ElementButton::clearSizeInfo()
{
  p_numWidth = 0;
  p_numHeight = 0;
  p_symbolWidth = 0;
  p_symbolHeight = 0;
}
