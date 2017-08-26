#ifndef  __ATOMMEASUREDIST_HH
#define  __ATOMMEASUREDIST_HH

#include <string>
using std::string;

#include "inv/SbLinear.hpp"
#include "inv/fields/SoSFBitMask.hpp"
#include "inv/fields/SoSFFloat.hpp"
#include "inv/fields/SoSFBool.hpp"
#include "inv/nodes/SoLineSet.hpp"
#include "inv/fields/SoSFVec3f.hpp"
#include "inv/nodes/SoSeparator.hpp"

#include "inv/ChemKit/ChemLabel.hpp"
#include "inv/ChemKit/ChemData.hpp"

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
