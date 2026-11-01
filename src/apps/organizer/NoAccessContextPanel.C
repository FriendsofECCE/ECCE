/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#include "NoAccessContextPanel.H"
#include "dsm/Resource.H"
#include "wxgui/WxResourceTreeItemData.H"


/**
 * Constructor with default parameters.
 */
NoAccessContextPanel::NoAccessContextPanel( CalcMgr * calcMgr,
                                            WxResourceTreeItemData *itemData,
                                            wxWindow *parent,
                                            wxWindowID id,
                                            const wxPoint& pos,
                                            const wxSize& size,
                                            long style ):
  NoAccessContextPanelGUI(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr)
{
  // cache the tree item pointer
  p_itemData = itemData;

  Resource *res = itemData->getResource();
  wxString text;
  switch (res->accessFailure()) {
  case Resource::ACCESS_MISSING:
    text = _("This item no longer exists. It was moved, renamed or\nremoved outside this window.\nClick the refresh button on the tool bar to update the tree.");
    break;
  case Resource::ACCESS_LOGIN:
    text = _("The data server did not accept the login for this project.\nTo re-enter the password, click on the refresh button\non the tool bar.");
    break;
  default:
    text = _("This item could not be read.");
    if (!res->accessMessage().empty())
      text += wxString("\n") + wxString::FromUTF8(res->accessMessage().c_str());
    break;
  }
  if (p_message) {
    p_message->SetLabel(text);
    Layout();
  }
}

NoAccessContextPanel::~NoAccessContextPanel( )
{
  p_itemData = 0;
}


void NoAccessContextPanel::refresh()
{}

