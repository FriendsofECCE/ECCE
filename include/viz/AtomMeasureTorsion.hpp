#ifndef  __ATOMMEASURETORSION_HH
#define  __ATOMMEASURETORSION_HH

#include <string>
using std::string;

#include "inv/SbLinear.H"
#include "inv/fields/SoSFBitMask.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/nodes/SoShape.H"
#include "inv/fields/SoSFVec3f.H"
#include "inv/nodes/SoFaceSet.H"

#include "inv/ChemKit/ChemData.H"

class SGFragment;
class TAtm;

class AtomMeasureTorsion : public SoFaceSet {

  SO_NODE_HEADER(AtomMeasureTorsion);

public:

  // Initializes this class
  static void   initClass();

  // Constructor
  AtomMeasureTorsion();
  AtomMeasureTorsion(SGFragment *, TAtm *,  TAtm *,  TAtm *, TAtm *,
      SoSeparator *parent);
  AtomMeasureTorsion(int , int, int , int, ChemData* ) ;

  // Accessors
  SoSeparator *getParent();
  SGFragment  *getFragment();
  TAtm *atom1();
  TAtm *atom2();
  TAtm *atom3();
  TAtm *atom4();
  float getTorsion();
  string getTorsionName();

  void replaceSGFragment(SGFragment* frag) ;
  void jump();

protected:


  SoVertexProperty * p_planes; 

  float p_value;       // current torsion.

  // For moving lines when atoms are deleted
  SGFragment  *p_frag;
  SoSeparator *p_parent;

  int p_atFirst;     // First Point
  int p_atSecond;    //  Second Point
  int p_atThird;     //  Third Point
  int p_atFourth;     //  Third Point
  string p_torsionName;

  // This implements the GL rendering action. We will inherit
  // all other action behavior, including rayPick(), which is
  // defined by SoShape to pick against all of the triangles
  // created by generatePrimitives.
  virtual void  GLRender(SoGLRenderAction *action);


private:

  // Destructor
  virtual ~AtomMeasureTorsion();


};

#endif /* !__ATOMMEASURETORSION_HH */
