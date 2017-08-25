/**
 * @file
 *
 *
 */
#ifndef  PBCGENERATEFRAGCMD_H
#define  PBCGENERATEFRAGCMD_H

#include "viz/PBCCmd.H"

class Fragment;

/**
 * Executes the fortran code that generates a full fragment from 
 * the reduced frag and symmetry group.
 *
 */
class PBCGenerateFragCmd : public PBCCmd
{
   public:
      PBCGenerateFragCmd(const string& name, Receiver * receiver, SGFragment *base);
      virtual ~PBCGenerateFragCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




