/////////////////////////////////////////////////////////////////////////////
// Synopsis:
//   Node that displays xyz axis.
//
// Description
//   
/////////////////////////////////////////////////////////////////////////////
#ifndef  __ECCE_AXIS_H__
#define  __ECCE_AXIS_H__

#include "inv/SbLinear.hpp"
#include "inv/fields/SoSFBitMask.hpp"
#include "inv/fields/SoSFFloat.hpp"
#include "inv/fields/SoSFBool.hpp"
#include "inv/nodes/SoLineSet.hpp"
#include "inv/fields/SoSFVec3f.hpp"


class EcceAxis : public SoLineSet {

  SO_NODE_HEADER(EcceAxis);

  public:

  // Constructor
  EcceAxis(); 

  // Inventor - Initializes this class
  static void   initClass();



  protected:
  SoVertexProperty * line;
  // This implements the GL rendering action. We will inherit
  // all other action behavior, including rayPick(), which is
  // defined by SoShape to pick against all of the triangles
  // created by generatePrimitives.
  virtual void  GLRender(SoGLRenderAction *action);
  virtual void  GLRenderCenter(SoGLRenderAction *action);
  virtual void  GLRenderCorner(SoGLRenderAction *action);



  private:
  // Destructor
  virtual ~EcceAxis();


};

#endif /* !__ECCE_AXIS_H__ */
