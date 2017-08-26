#ifndef  __ATOMMEASUREANGLE_HH
#define  __ATOMMEASUREANGLE_HH

#include <string>
using std::string;
  
#include "inv/SbLinear.hpp"
#include "inv/fields/SoSFBitMask.hpp"
#include "inv/fields/SoSFFloat.hpp"
#include "inv/nodes/SoShape.hpp"
#include "inv/fields/SoSFVec3f.hpp"
#include "inv/nodes/SoLineSet.hpp"

#include "inv/ChemKit/ChemData.hpp"

class SGFragment;
class TAtm;

class AtomMeasureAngle : public SoLineSet {

   SO_NODE_HEADER(AtomMeasureAngle);

public:

  // Initializes this class
  static void   initClass();

  AtomMeasureAngle();
  AtomMeasureAngle(SGFragment *,TAtm *,TAtm *, TAtm *,SoSeparator *);
  AtomMeasureAngle(int, int, int, ChemData* ) ;

  // Accessors
  SoSeparator *getParent();
  SGFragment  *getFragment();
  TAtm *atom1();
  TAtm *atom2();
  TAtm *atom3();
  float getAngle();
  string getAngleName();

  void replaceSGFragment(SGFragment* frag) ;
  
  void jump();

 protected:


  SoVertexProperty * p_line;

  int p_atFirst;     // First Point
  int p_atSecond;    // Second Point
  int p_atThird;     // Third Point
  string p_angleName;

  // For moving lines when atoms are deleted
  SGFragment *p_frag;
  SoSeparator *p_parent;

  // Current value
  float p_value; 

   // This implements the GL rendering action. We will inherit
   // all other action behavior, including rayPick(), which is
   // defined by SoShape to pick against all of the triangles
   // created by generatePrimitives.
   virtual void  GLRender(SoGLRenderAction *action);


 private:

   // Destructor
   virtual ~AtomMeasureAngle();

 
};

#endif /* !__ATOMMEASUREANGLE_HH */
