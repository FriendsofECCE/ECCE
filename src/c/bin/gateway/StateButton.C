/**
 * @file
 * @author Lisong Sun
 *
 * State Button Class
 *
 * This is a homemade widget that mimic a button but can be turned off
 * as flat if needed. It also got customized drawing on it using WxState.
 * It is used in GatewayPref dialog to set or reset state colors.
 *
 */
#include "wx/wx.h"


#include "dsm/ResourceDescriptor.H"

#include "StateButton.H"
#include "GatewayPrefs.H"

#include "wxgui/ewxColorDialog.H"
#include "wxgui/WxState.H"


BEGIN_EVENT_TABLE( StateButton, wxPanel )

  EVT_LEFT_DOWN         (StateButton::OnMouseLeftDown)
  EVT_LEFT_UP           (StateButton::OnMouseLeftUp)
  EVT_ENTER_WINDOW      (StateButton::OnMouseEnterWindow)
  EVT_LEAVE_WINDOW      (StateButton::OnMouseLeaveWindow)
  EVT_PAINT             (StateButton::OnPaint)

END_EVENT_TABLE()


/**
 * Empty constructor, should never be used.
 */
StateButton::StateButton() {}


/**
 * Destructor.
 *
 * Nothing need to be done here.
 */
StateButton::~StateButton() {}


/**
 * Default constructor.
 *
 * Call the Create function which does the real job.
 * @param state The button's run state.
 * @param isDefault Whether the button is the user customization one or
 *                  the reset to default one.
 * @param parent The parent dialog which is the GatewayPrefs.
 */
StateButton::StateButton(ResourceDescriptor::RUNSTATE state, bool isDefault,
                         GatewayPrefs * parent, wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style)
{
  Create(state, isDefault, parent, id, pos, size, style);
}


/**
 * Create the button by putting a WxState in the center and set the status.
 */
void StateButton::Create(ResourceDescriptor::RUNSTATE state, bool isDefault,
                         GatewayPrefs * parent, wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style)
{
  ewxPanel::Create( parent, id, pos, size, style );

  p_state = state;
  p_isDefault = isDefault;
  p_sibling = NULL;
  p_parent = parent;

  wxBoxSizer * buttonSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(buttonSizer);

  p_stateIcon = new WxState(this);
  p_stateIcon->setRunState(state, isDefault);
  p_stateIcon->SetEventHandler(this);

  p_butStatus = RAISENBUTTON;

  if (isDefault && (!CanBeReset()))
    p_butStatus = FLATBUTTON;

  buttonSizer->Add(p_stateIcon, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 4);
  Refresh();
}


/**
 * Enable/Disable the tool.
 *
 * This override the wxWindow::Enable function. It sets the render style
 * of the widget so that the draw function will know whether it needs to
 * draw the border.
 */
bool StateButton::Enable(bool enable)
{
  bool ret = (p_butStatus != FLATBUTTON);
  if (enable) SetStatus(RAISENBUTTON);
  else SetStatus(FLATBUTTON);
  return ret;
}


/**
 * Utility to set FLAT, RAISE, and SUNKEN status.
 */
void StateButton::SetStatus(BUTTONSTATUS status)
{
  p_butStatus = status;
  Refresh();
}


/**
 * Return true if the state color is not the default one.
 */
bool StateButton::CanBeReset()
{
  return p_stateIcon->canBeReset();
}


/**
 * Event handling function for mouse left button down event.
 *
 * Set the status of the button to be SUNKEN.
 */
void StateButton::OnMouseLeftDown( wxMouseEvent& event )
{
  if (p_butStatus != FLATBUTTON)
    SetStatus(SUNKENBUTTON);
}


/**
 * Event handling function for mouse left button up event
 *
 * Based on the previous button status, call the ToolActive function
 * or just reset the status.
 */
void StateButton::OnMouseLeftUp( wxMouseEvent& event )
{
  if (p_butStatus != FLATBUTTON) {
    if (p_butStatus == SUNKENBUTTON) {
      ToolActive();
    }
    if (p_isDefault) SetStatus(FLATBUTTON);
    else SetStatus(RAISENBUTTON);
    p_parent->saveSettings();
  }
}


/**
 * Event handling function for mouse entering window event
 */
void StateButton::OnMouseEnterWindow( wxMouseEvent& event )
{
}


/**
 * Event handling function for mouse leaving window event
 */
void StateButton::OnMouseLeaveWindow( wxMouseEvent& event )
{
  if (p_butStatus != FLATBUTTON)
    SetStatus(RAISENBUTTON);
}


/**
 * Paint the border based on the StateButton's status.
 */
void StateButton::OnPaint( wxPaintEvent& event )
{
  wxPaintDC dc(this);
  PrepareDC(dc);

  if (p_butStatus == FLATBUTTON)
    SetToolTip(NULL);
  else {
    if (p_isDefault)
      SetToolTip(_("Click to reset the custom color to default"));
    else
      SetToolTip(_("Click to change the custom color"));

    drawButtonBorder(dc, p_butStatus == SUNKENBUTTON);
  }

  p_stateIcon->refresh();
  event.Skip();
}


/**
 * Event handling function when the button is clicked.
 */
void StateButton::ToolActive()
{
  // If this button is a custom one
  if (!p_isDefault) {
    ewxColorDialog colorDialog(ewxColor(p_stateIcon->getColorString().c_str()), p_parent);
    if (colorDialog.ShowModal() == wxID_OK) {
      ((GatewayPrefs *)p_parent)->changeStateColor(p_stateIcon->getPrefString(), ewxColor(colorDialog.GetColor()).ToString());
      p_stateIcon->resetToUserDefault();
      p_sibling->SiblingClicked();
    }
  }
  // If this button is a default one
  else {
    ((GatewayPrefs *)p_parent)->resetStateColor(p_stateIcon->getPrefString());
    p_sibling->SiblingClicked();
  }
}


/**
 * Set the sibling.
 *
 * A StateButton's sibling is the one next to it in GatewayPref, for example,
 * the default one is the sibling of the custom one.
 */
void StateButton::SetSibling(StateButton * sibling)
{
  p_sibling = sibling;
}


/**
 * Event handling function when the sibling is clicked.
 */
void StateButton::SiblingClicked()
{
  // If this button is a default one
  if (p_isDefault) {
    if (CanBeReset())
      SetStatus(RAISENBUTTON);
    else
      SetStatus(FLATBUTTON);
  }
  // If this button is a custom one
  else {
    p_stateIcon->resetToUserDefault();
    Refresh();
  }
}
