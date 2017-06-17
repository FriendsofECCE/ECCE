#ifndef PBCEQUIVRECTCMD_H
#define PBCEQUIVRECTCMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to fold outlying atoms into the unit cell.
 */
class PBCEquivRectCmd : public PBCCmd
{
   public:
      PBCEquivRectCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCEquivRectCmd();

      virtual bool execute() throw(EcceException);

   protected:
      void init();

};
#endif
