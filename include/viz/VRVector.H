#ifndef  __VR_VECTOR_H__
#define  __VR_VECTOR_H__

#include "inv/nodes/SoGroup.H"

class SoScale;
class SoTransform;
class SoMaterial;
class SoCone;
class SoCylinder;
class SoTranslation;

class  VRVector : public SoGroup {
  
    SO_NODE_HEADER(VRVector);

  public:

    VRVector(); 
    void position(double,double,double);
    void direction(double,double,double);
    void setColor(float,float,float);
    void getColor(float*);
    void scaleFactor(float,float,float);
    // Constant shaft/head size whatever the length; the length is |direction|.
    void fixedThickness(bool);
    // Start the arrow this far from the position, along its direction.
    void startRadius(double);
    const double* getPosition(); 
    const double* getDirection(); 
    SoTransform* getTransform();
    SoScale* getScale();

SoINTERNAL public:
    static void		initClass();

  protected:
   virtual  ~VRVector();
   void  constructor();

   SoScale * scal; 
   SoTransform * transf;
   SoMaterial * mat;
   SoCylinder * shaft;
   SoCone * head;
   SoTranslation * shaftTrans;
   SoTranslation * headTrans;
   bool p_fixed;
   double p_startRadius;
   double p_center[3];
   void  place();
   double p_position[3];
   double p_direction[3];

};




#endif /* !__VR_VECTOR_H__*/

