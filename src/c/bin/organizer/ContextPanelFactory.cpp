/**
 * @file
 * @author Lisong Sun
 *
 * Generate different context panel objects depend on the content type and
 * resource type of the tree node.
 *
 */

#include <iostream>
using namespace std;

#include "util/ResourceUtils.hpp"
#include "util/StringConverter.hpp"

#include "dsm/Resource.hpp"
#include "dsm/ResourceDescriptor.hpp"
#include "dsm/ResourceType.hpp"

#include "wxgui/WxResourceTreeItemData.hpp"

#include "ContextPanelInterface.hpp"
#include "ContextPanelFactory.hpp"

// List of all context panel header files here
#include "PropertyContextPanel.hpp"
#include "ProjectContextPanel.hpp"
#include "DocumentContextPanel.hpp"
#include "CalculationContextPanel.hpp"
#include "SessionContextPanel.hpp"
#include "SessionProjectContextPanel.hpp"
#include "ImageContextPanel.hpp"
#include "NoAccessContextPanel.hpp"


class CalcMgr;  

ContextPanelInterface * ContextPanelFactory::createContextPanel
(CalcMgr * calcMgr, WxResourceTreeItemData * node, wxWindow * parent)
{
  if (!node->getResource()->hasAccess())
    return new NoAccessContextPanel(calcMgr, node, parent);

  ResourceType *res = node->getResource()->getDescriptor();
  string panelName = "";

  ContextPanelInterface * ret = NULL;

  if (res) panelName = res->getContextPanel();
  
  if (panelName == "DocumentContextPanel")
    ret = new DocumentContextPanel(calcMgr, node, parent);
  else if (panelName == "ImageContextPanel")
    ret = new ImageContextPanel(calcMgr, node, parent);
  else if (panelName == "ProjectContextPanel")
    ret = new ProjectContextPanel(calcMgr, node, parent);
  else if (panelName == "CalculationContextPanel")
    ret = new CalculationContextPanel(calcMgr, node, parent);
  else if (panelName == "MdContextPanel")
    ret = new SessionProjectContextPanel(calcMgr, node, parent);
  else if (panelName == "ReactionContextPanel")
    ret = new SessionProjectContextPanel(calcMgr, node, parent);
  else if (panelName == "PropertyContextPanel")
    ret = new PropertyContextPanel(calcMgr, node, parent);
  else
    ret = new PropertyContextPanel(calcMgr, node, parent);

  return ret;
}
