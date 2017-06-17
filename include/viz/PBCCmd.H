#ifndef PBCCMD_H
#define PBCCMD_H


#include "viz/FragCloneCmd.H"

#include <vector>
  using std::vector;

class SGFragment; 

/**
 * Abstract subclass for PBC related commands.
 * Not sure if it will be needed but may be useful to help manage
 * the special problem between the base fragment and the fragment that is being 
 * rendered
 */
class PBCCmd : public FragCloneCmd
{
   public:
      PBCCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCCmd();

      virtual void cloneFragment(SGFragment *frag);
      virtual void undo();
      virtual bool shouldAutoReplicate() const;
      virtual bool shouldAutoReplicateOnUndo() const;


};
#endif
