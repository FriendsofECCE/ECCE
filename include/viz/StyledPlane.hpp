#ifndef  __STYLEDPLANE_H
#define  __STYLEDPLANE_H

#include "inv/nodes/SoSwitch.H"

class SGPlane;
class SoMaterial;

/**
 * A Plane and the material properties and hints to render it properly.
 * To control the plane, use getPlane to first get the plane from the SG.
 * Convenience functions can be used to set material properties rather
 * than getting the material...  The plane is handeled differently because
 * we expect subclasses to be created (e.g. miller planes, selection planes...)
 *
 * Transparency is on/off rather than a continuous property since most values
 * look lame.
 */
class StyledPlane : public SoSwitch 
{

   SO_NODE_HEADER(StyledPlane);

   public:

   StyledPlane(); 

   StyledPlane(SGPlane *plane); 

   void setColor(float r, float g, float b);
   void getColor(float& r, float& g, float& b);

   void setTransparent(bool transparent);
   bool isTransparent() const;

   SoMaterial* getMaterial();
   SGPlane* getPlane();
   void show(bool flag);

   SoINTERNAL public:
      static void		initClass();

   protected:
   virtual  ~StyledPlane();
   void  constructor();

   SGPlane *p_plane; 
   SoMaterial *p_mat;

};

#endif 
