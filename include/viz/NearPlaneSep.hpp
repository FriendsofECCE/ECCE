#ifndef  _NEARPLANESEP_
#define  _NEARPLANESEP_

#include "inv/nodes/SoSeparator.H"


class SoSeparator;

/**
 * NearPlaneSep is a special separator that prevents its children
 * from being including in the bounding box calculation.
 * This allows us to do things like draw an axis on the near clip plane
 * without affecting things like centering.
 */
class  NearPlaneSep : public SoSeparator
{

   SO_NODE_HEADER(NearPlaneSep);

   public:  

   NearPlaneSep(); 

   SoEXTENDER public:
      virtual void getBoundingBox(SoGetBoundingBoxAction *action);



   SoINTERNAL public:
      static void		initClass();

};

#endif 
