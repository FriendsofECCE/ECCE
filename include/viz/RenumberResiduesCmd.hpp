/**
 * @file
 *
 *
 */
#ifndef  RENUMBERRESIDUESCMD_H
#define  RENUMBERRESIDUESCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Command wrapper for renumbering residues
 */
class RenumberResiduesCmd : public FragCloneCmd
{
   public:
      RenumberResiduesCmd(const string& name, Receiver * receiver);
      virtual ~RenumberResiduesCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




