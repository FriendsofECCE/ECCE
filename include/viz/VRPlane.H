#ifndef  __VR_ECCE_PLANE_H__
#define  __VR_ECCE_PLANE_H__

class SoGLRenderAction;
class RTDraggerNode;

#include "inv/nodes/SoGroup.H"

class  VRPlane : public SoGroup {

  SO_NODE_HEADER(VRPlane);


  public:

  VRPlane(); 


  SoINTERNAL public:
    static void		initClass();

  protected:
  virtual  ~VRPlane();
  void  constructor();

  // This implements the GL rendering action. We will inherit
  // all other action behavior, including rayPick(), which is
  // defined by SoShape to pick against all of the triangles
  // created by generatePrimitives.
  virtual void  GLRender(SoGLRenderAction *action);


  private:

  RTDraggerNode * mickeyDragg;



};




#endif /* !__VR_ECCE_PLANE_H__*/

