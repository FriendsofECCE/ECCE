#ifndef  __VR_VECTOR_H__
#define  __VR_VECTOR_H__

#include "inv/nodes/SoGroup.H"

class SoScale;
class SoTransform;
class SoMaterial;

class  VRVector : public SoGroup {
  
    SO_NODE_HEADER(VRVector);

  public:

    VRVector(); 
    void position(double,double,double);
    void direction(double,double,double);
    void setColor(float,float,float);
    void getColor(float*);
    void scaleFactor(float,float,float);
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
   double p_position[3];
   double p_direction[3];

};




#endif /* !__VR_VECTOR_H__*/

