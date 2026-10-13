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
    // The same with these sizes in Angstrom (the defaults are the
    // normal-mode arrow's 0.035, 0.09, 0.18).
    void fixedThickness(float shaftRadius, float headRadius, float headLength);
    // Start the arrow this far from the position, along its direction.
    void startRadius(double);
    // Draw the arrow a second time, after everything else's depth test, at
    // 40 % opacity, so the part inside an atom or bond still shows.
    void showThroughMolecule();
    // Radius of the arrowhead as drawn (for tests).
    float headRadius() const;
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
   SoMaterial * throughMat;
   SoCylinder * shaft;
   SoCone * head;
   SoTranslation * shaftTrans;
   SoTranslation * headTrans;
   bool p_fixed;
   float p_shaftRadius, p_headRadius, p_headLength;
   double p_startRadius;
   double p_center[3];
   void  place();
   double p_position[3];
   double p_direction[3];

};




#endif /* !__VR_VECTOR_H__*/

