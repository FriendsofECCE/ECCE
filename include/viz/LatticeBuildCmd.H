#ifndef LATTICEBUILDCMD_HH
#define LATTICEBUILDCMD_HH


#include "viz/FragCloneCmd.H"

#include <vector>
  using std::vector;

class LatticeBuildCmd : public FragCloneCmd
{
   public:
      LatticeBuildCmd(const string& name, Receiver * receiver);
      ~LatticeBuildCmd();

      virtual bool execute() throw(EcceException);

   protected:
      void init();

};
#endif
