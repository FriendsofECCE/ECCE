#ifndef  SGPLANE__H_H
#define  SGPLANE__H_H

#include <vector>
using std::vector;
#include <string>
using std::string;

#include "inv/fields/SoSFBitMask.H"
#include "inv/nodes/SoFaceSet.H"
#include "inv/nodes/SoVertexProperty.H"

#include "util/Transformations.H"

/**
 * Class for drawing a plane defined by n points.
 * The reason for supporting n points is that the intended use is for
 * a miller plane intersecting the lattice box which can result in between 
 * 3 and 8 points.
 */
class  SGPlane : public SoFaceSet 
{

   SO_NODE_HEADER(SoFaceSet);

   public:

      SGPlane();
      SGPlane(MPoint c1, MPoint c2, MPoint c3, MPoint c4); 
      void setCorners(MPoint c1, MPoint c2, MPoint c3, MPoint c4);
      void setCorners(const vector<MPoint>& corners);
      void setLabel(const string& label);
      void setColor(float r, float g, float b);
      string getLabel() const;


   SoINTERNAL public:
      static void		initClass();

   protected:

      virtual  ~SGPlane();
      void  constructor();
      void  GLRender(SoGLRenderAction *action);
      void  regenerate();


      vector<MPoint> p_cnrs;
      string p_label;
      float p_r, p_g, p_b; // color components for label

};



#endif 
