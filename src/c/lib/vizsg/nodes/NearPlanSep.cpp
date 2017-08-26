#include "inv/actions/SoGetBoundingBoxAction.hpp"

#include "inv/nodes/SoSeparator.hpp"

#include "viz/NearPlaneSep.hpp"

SO_NODE_SOURCE(NearPlaneSep);

void NearPlaneSep::initClass()
{
   SO_NODE_INIT_CLASS(NearPlaneSep, SoSeparator, "Separator");
}

NearPlaneSep::NearPlaneSep()
{
  SO_NODE_CONSTRUCTOR(NearPlaneSep);
}


/**
 * Return without having child nodes contribute to the bounding box.
 */
void NearPlaneSep::getBoundingBox(SoGetBoundingBoxAction *action)
{
   return;
}
