#ifndef  SGLATTICE__H
#define  SGLATTICE__H

#include "inv/fields/SoSFBitMask.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/nodes/SoVertexProperty.H"

#include "tdat/LatticeDef.H"

/**
 * A to draw lines that represent the unit cell of a periodic system.
 * This class also draws replicated boxes.
 * See also LatticeDef
 */
class  SGLattice : public SoLineSet 
{

   SO_NODE_HEADER(SGLattice);

   public:

      SGLattice(); 

      void setLattice(LatticeDef *lattice);

   SoINTERNAL public:
      static void		initClass();

   protected:

      virtual  ~SGLattice();
      void  constructor();
      void  GLRender(SoGLRenderAction *action);
      void updateColors();


      SoVertexProperty *p_line;
      MPoint p_a1str, p_a2str, p_a3str;
      uint32_t p_color, p_xcolor, p_ycolor, p_zcolor;

};



#endif 
