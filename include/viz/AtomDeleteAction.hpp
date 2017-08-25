#ifndef ATOMDELETEACTION_HH
#define ATOMDELETEACTION_HH

#include "inv/actions/SoSubAction.H"

class AtomDeleteAction : public SoAction {

   SO_ACTION_HEADER(AtomDeleteAction);

 public:
   // Initializes this action class for use with scene graphs
   static void    initClass();

   // Constructor and destructor
   AtomDeleteAction();
   virtual ~AtomDeleteAction();


 protected:
   // Initiates action on graph
   virtual void   beginTraversal(SoNode *node);

 private:

   // These are the methods that are used to apply the action
   // to various node classes. The third method is registered
   // for all relevant non-shape nodes. The calling sequence for
   // these methods is that used for all methods in the global
   // action table.
   static void    verify_rot(SoAction *, SoNode *);
   static void    verify_manip(SoAction *, SoNode *);
   static void    verify_dist(SoAction *, SoNode *);
   static void    verify_theta(SoAction *, SoNode *);
   static void    verify_phy(SoAction *, SoNode *);
   static void    verify_line(SoAction *, SoNode *);
   static void    callDoAction(SoAction *, SoNode *);
};

#endif /* ATOMDELETEACTION_HH */
