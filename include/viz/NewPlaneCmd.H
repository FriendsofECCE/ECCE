#ifndef NEWPLANECMD_H
#define NEWPLANECMD_H


#include "viz/FragCloneCmd.H"

#include <vector>
  using std::vector;

class NewPlaneCmd : public FragCloneCmd
{
   public:
      NewPlaneCmd(const string& name, Receiver * receiver);
      ~NewPlaneCmd();

      virtual bool execute();

   protected:
      void init();

};
#endif
