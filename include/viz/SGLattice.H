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

      // World position of the a/b/c label (i = 0..2) as drawn; false when
      // the cell has none (a GROMACS compact box).
      bool labelPosition(int i, double xyz[3]);

      // Lowest and highest drawn vertex (all replicas); false if none.
      bool drawnBounds(double lo[3], double hi[3]);

   SoINTERNAL public:
      static void		initClass();

   protected:

      virtual  ~SGLattice();
      void  constructor();
      void  GLRender(SoGLRenderAction *action);
      void updateColors();


      SoVertexProperty *p_line;
      MPoint p_a1str, p_a2str, p_a3str;
      bool p_noLabels;   // a GROMACS compact box has no a/b/c edges to label
      uint32_t p_color, p_xcolor, p_ycolor, p_zcolor;

};



#endif 
