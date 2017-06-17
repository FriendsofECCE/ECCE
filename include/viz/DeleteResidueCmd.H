/**
 * @file
 *
 *
 */
#ifndef  DELETERESIDUECMD_H
#define  DELETERESIDUECMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Command wrapper for renumbering residues
 */
class DeleteResidueCmd : public FragCloneCmd
{
   public:
      DeleteResidueCmd(const string& name, Receiver * receiver);
      virtual ~DeleteResidueCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




