/**
 * @file
 *
 * @author Lisong Sun
 *
 *
 */

#ifndef NOACCESSCONTEXTPANEL_H
#define NOACCESSCONTEXTPANEL_H


#include "ContextPanelInterface.H"
#include "NoAccessContextPanelGUI.H"


class CalcMgr;
class WxResourceTreeItemData;
class ContextPanelFactory;


/**
 *
 */
class NoAccessContextPanel : public NoAccessContextPanelGUI,
                             public ContextPanelInterface
{
public:

  friend class ContextPanelFactory;

  ~NoAccessContextPanel();

  void refresh();

protected:

  NoAccessContextPanel( CalcMgr * calcMgr,
                        WxResourceTreeItemData *itemData,
                        wxWindow *parent,
                        wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxSUNKEN_BORDER| wxTAB_TRAVERSAL);

private:

  /** Pointer to a tree item containing the property data. */
  WxResourceTreeItemData *p_itemData;
};

#endif
