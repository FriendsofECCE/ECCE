#ifndef  _ATOMROTDRAGGER_HH
#define  _ATOMROTDRAGGER_HH

#include "inv/draggers/SoRotateDiscDragger.H"
#include "inv/nodes/SoTransform.H"


class SGFragment;
class TAtm;

class AtomRotDragger : public SoRotateDiscDragger
{
  SO_NODE_HEADER(AtomRotDragger);

public:
  AtomRotDragger(SGFragment *frag,TAtm *at1,TAtm *at2,SoSeparator *parent);

  //  virtual void  GLRender(SoGLRenderAction *action);
  void jump();

  static void initClass();

  // Accessors
  TAtm *atom1();
  TAtm *atom2();
  SGFragment *getFragment();
  SoSeparator *getParent();
  SoTransform *getTransform();
  float getRotAngle();

  // Modifiers
  void setTransform(SoTransform *trans);
  void setRotAngle(float angle);


protected:
  TAtm * p_at1;
  TAtm * p_at2;
  float p_old_ang_rb;
  SGFragment *p_frag;
  SoTransform * p_tran;
  SoSeparator *p_parent;

  AtomRotDragger();
  ~AtomRotDragger();

private:

};    
#endif  /* _ATOMROTDRAGGER_HH */
