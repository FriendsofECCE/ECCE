#ifndef  _CUSTOMBBOXNODE
#define  _CUSTOMBBOXNODE

#include "inv/nodes/SoNode.hpp"
#include "inv/nodes/SoSubNode.hpp"
#include "inv/actions/SoGetBoundingBoxAction.hpp"

#include "inv/SbBox.hpp"


class SoNode;

/**
 */
class  CustomBBoxNode : public SoNode
{

   SO_NODE_HEADER(CustomBBoxNode);

   public:  

   SbBox3f thebox;

   CustomBBoxNode(); 
   void enable();
   void disable();

   SoEXTENDER public:
      virtual void getBoundingBox(SoGetBoundingBoxAction *action);



   SoINTERNAL public:
      static void		initClass();

   protected:
      bool p_disabled;
};

#endif 
