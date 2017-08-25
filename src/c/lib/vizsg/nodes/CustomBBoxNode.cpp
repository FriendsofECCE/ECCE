

#include "viz/CustomBBoxNode.H"

SO_NODE_SOURCE(CustomBBoxNode);

void CustomBBoxNode::initClass()
{
   SO_NODE_INIT_CLASS(CustomBBoxNode, SoNode, "Node");
}

CustomBBoxNode::CustomBBoxNode()
{
  SO_NODE_CONSTRUCTOR(CustomBBoxNode);
  thebox.setBounds(0.,0.,0., 0., 0., 0.);
  p_disabled = true;
}


/**
 * 
 */
void CustomBBoxNode::getBoundingBox(SoGetBoundingBoxAction *action)
{
   if (!p_disabled) {
      action->extendBy(thebox);
      action->resetCenter();
   }
}


void CustomBBoxNode::enable()
{
   p_disabled = false;
}

void CustomBBoxNode::disable()
{
   p_disabled = true;
}
