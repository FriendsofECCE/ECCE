#ifndef MOVEACTION_HH
#define MOVEACTION_HH

#include "inv/actions/SoSubAction.H"

class MoveAction : public SoAction {

   SO_ACTION_HEADER(MoveAction);

 public:
   // Initializes this action class for use with scene graphs
   static void    initClass();

   // Constructor and destructor
   MoveAction();
   virtual ~MoveAction();


 protected:
   // Initiates action on graph
   virtual void   beginTraversal(SoNode *node);

 private:
  

   // These are the methods that are used to apply the action
   // to various node classes. The third method is registered
   // for all relevant non-shape nodes. The calling sequence for
   // these methods is that used for all methods in the global
   // action table.
   static void    jump_rot(SoAction *, SoNode *);
   static void    jump_manip(SoAction *, SoNode *);
   static void    jump_dist(SoAction *, SoNode *);
   static void    jump_theta(SoAction *, SoNode *);
   static void    jump_phy(SoAction *, SoNode *);
   static void    jump_line(SoAction *, SoNode *);
   static void    callDoAction(SoAction *, SoNode *);
};

#endif /* MOVEACTION_HH */
