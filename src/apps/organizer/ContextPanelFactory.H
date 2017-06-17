/**
 * @file
 * @author Lisong Sun
 *
 * Generate different context panel objects depend on the content type and
 * resource type of the tree node.
 *
 */

#ifndef _CONTEXTPANELFACTORY_HH
#define _CONTEXTPANELFACTORY_HH

class ContextPanelInterface;
class WxResourceTreeItemData;
class CalcMgr;

class ContextPanelFactory
{
public:

  static ContextPanelInterface * 
  createContextPanel(CalcMgr * calcMgr, 
                     WxResourceTreeItemData * itemData,
                     wxWindow * parent);

protected:
  ContextPanelFactory() {};
  ~ContextPanelFactory() {};
};

#endif
