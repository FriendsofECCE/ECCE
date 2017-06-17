#ifndef  _CUSTOMBBOXNODE
#define  _CUSTOMBBOXNODE

#include "inv/nodes/SoNode.H"
#include "inv/nodes/SoSubNode.H"
#include "inv/actions/SoGetBoundingBoxAction.H"

#include "inv/SbBox.H"


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
