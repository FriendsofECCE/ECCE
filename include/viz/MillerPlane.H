#ifndef  MILLERPLANE_H
#define  MILLERPLANE_H

#include <vector>
using std::vector;
#include <string>
using std::string;

//#include "inv/fields/SoSFBitMask.H"
//#include "inv/nodes/SoFaceSet.H"
//#include "inv/nodes/SoVertexProperty.H"
#include "viz/SGPlane.H"

#include "util/Transformations.H"

/**
 * A plane that has miller indices.
 * We sublcass to that our scene graph objects can double as
 * data storage classes.  
 *
 * KLS 10/27/09 Not currently used since we simplified the GUI design
 * such that its not necessary to store off data with its plane.
 * I'm anticipating a comeback in the idea so am keeping the class around...
 */
class  MillerPlane : public SGPlane 
{

   SO_NODE_HEADER(SGPlane);

   public:

      MillerPlane();
      MillerPlane(MPoint c1, MPoint c2, MPoint c3, MPoint c4); 
      void setIndices(int h, int k, int l);
      void getIndices(int& h, int& k, int& l);


   SoINTERNAL public:
      static void		initClass();

   protected:

      virtual  ~MillerPlane();
      void  constructor();

      int p_h, p_k, p_l; 

};



#endif 
