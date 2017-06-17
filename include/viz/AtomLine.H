/////////////////////////////////////////////////////////////////////////////
// $Source$
//
// Synopsis:
//   Draws lines connecting two atoms in the scenegraph.
//
// Description
//   
/////////////////////////////////////////////////////////////////////////////
#ifndef  __ATOM_LINE_H__
#define  __ATOM_LINE_H__
  
#include "inv/SbLinear.H"
#include "inv/fields/SoSFBitMask.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/fields/SoSFBool.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/fields/SoSFVec3f.H"

#include "tdat/TAtm.H"

class SGFragment;

class AtomLine : public SoLineSet {

   SO_NODE_HEADER(AtomLine);

public:

   // Constructor
   AtomLine(); 
   AtomLine(SGFragment *frag, TAtm *, TAtm *, SoSeparator *parent);

   // Inventor - Initializes this class
   static void   initClass();


   // For moving lines when atoms are moved
   void jump();

   // Accessors
   SoSeparator *getParent();
   SGFragment *getFragment();
   TAtm *atom1();
   TAtm *atom2();

protected:
   SoVertexProperty *p_line;
   SoSeparator *p_parent;
   SGFragment *p_frag;  // Fragment that contains them
   TAtm *p_atFirst;     // First Point
   TAtm *p_atSecond;    //  Second Point

 private:
  // Destructor
   virtual ~AtomLine();

 
};

#endif /* !__ATOM_LINE_H__ */
