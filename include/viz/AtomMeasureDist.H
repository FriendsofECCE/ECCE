#ifndef  __ATOMMEASUREDIST_HH
#define  __ATOMMEASUREDIST_HH

#include <string>
using std::string;

#include "inv/SbLinear.H"
#include "inv/fields/SoSFBitMask.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/fields/SoSFBool.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/fields/SoSFVec3f.H"
#include "inv/nodes/SoSeparator.H"

#include "inv/ChemKit/ChemLabel.H"
#include "inv/ChemKit/ChemData.H"

class SGFragment;
class TAtm;


class AtomMeasureDist : public SoLineSet {

  SO_NODE_HEADER(AtomMeasureDist);

  public:

  // Constructor
  AtomMeasureDist(); 
  AtomMeasureDist
     (SGFragment *frag, TAtm *atm1, TAtm *atm2, SoSeparator *parent);
  AtomMeasureDist(int, int, ChemData* ) ;


  // Accessors
  SoSeparator *getParent();
  SGFragment *getFragment();
  TAtm *atom1();
  TAtm *atom2();
  float getDistance();
  string getDistanceName();

  //ChemLabel * label; 

  void replaceSGFragment(SGFragment* frag);

  void jump();

  // Initializes this class
  static void   initClass();


  protected:

  SoVertexProperty * p_line;
  SoSeparator *p_parent;
  SGFragment  *p_frag;
  int	       p_atFirst;
  int          p_atSecond;

  float        p_distance;  
  string       p_distanceName;

  // This implements the GL rendering action. We will inherit
  // all other action behavior, including rayPick(), which is
  // defined by SoShape to pick against all of the triangles
  // created by generatePrimitives.
  virtual void  GLRender(SoGLRenderAction *action);

  private:
  // Destructor
  virtual ~AtomMeasureDist();


};

#endif /* !__SO_ATOMMEASUREDIST_HH */
