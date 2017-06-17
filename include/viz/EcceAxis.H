/////////////////////////////////////////////////////////////////////////////
// Synopsis:
//   Node that displays xyz axis.
//
// Description
//   
/////////////////////////////////////////////////////////////////////////////
#ifndef  __ECCE_AXIS_H__
#define  __ECCE_AXIS_H__

#include "inv/SbLinear.H"
#include "inv/fields/SoSFBitMask.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/fields/SoSFBool.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/fields/SoSFVec3f.H"


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
