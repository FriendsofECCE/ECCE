/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#include "NoAccessContextPanel.H"


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
}

NoAccessContextPanel::~NoAccessContextPanel( )
{
  p_itemData = 0;
}


void NoAccessContextPanel::refresh()
{}

