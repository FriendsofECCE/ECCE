/**
 * @file
 *
 * @author Jeff Daily
 *
 */
#include <wx/sizer.h>

#include "util/NullPointerException.H"

#include "wxgui/ewxTextCtrl.H"

#include "CalcMgr.H"
#include "PropertyContextPanel.H"
#include "WxProperties.H"



/**
 * Constructor.
 * Shouldn't be used.
 */
WxProperties::WxProperties( )
{

}



/**
 * Constructor.
 * All the default parameters are defined in WxPropertiesGUI class.
 * This class is not intended to be used outside of an application.  If this
 * precondition is maintained, the parent will have an icon associated with it.
 * This icon is then used by this class.
 */
WxProperties::WxProperties( wxWindow* parent, wxWindowID id,
                            const string& caption, const wxPoint& pos,
                            const wxSize& size, long style ):
  p_itemData(NULL),
  p_propPanel(NULL)
{
  Create( parent, id, caption, pos, size, style );

  ewxFrame *parentFrame = dynamic_cast<ewxFrame*>(parent);
  NULLPOINTEREXCEPTION(parentFrame, "Could not cast from wxWindow to ewxFrame");

  wxIcon icon = parentFrame->GetIcon();
  SetIcon(icon);
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_URL));
  if (text) {
     text->SetValue(caption);
  }

}



/**
 *
 */
WxProperties::~WxProperties()
{
  p_itemData = 0;
}



/**
 *
 */
void WxProperties::setContext(WxResourceTreeItemData *itemData)
{
  // cache itemData
  p_itemData = itemData;

  if (p_propPanel != 0) {
    p_propSizer->Detach(p_propPanel);
    p_propPanel->Destroy();
  }

  CalcMgr* calcmgr = dynamic_cast<CalcMgr*>(GetParent());
  NULLPOINTEREXCEPTION(calcmgr, "Could not cast from wxWindow to CalcMgr.");

  p_propPanel = new PropertyContextPanel(calcmgr, itemData, this);
  NULLPOINTEREXCEPTION(p_propPanel, "Property panel could not be generated.");

  p_propSizer->Add(p_propPanel, 1, wxGROW|wxALL, 0);

}



/**
 * Callback when window is closed using the window decorations.
 */
void WxProperties::OnCloseWindow( wxCloseEvent& event )
{
  Show(false);
}



/**
 * Callback when button to close window is clicked.
 */
void WxProperties::OnButtonClosePropertiesClick( wxCommandEvent& event)
{
  Show(false);
}
