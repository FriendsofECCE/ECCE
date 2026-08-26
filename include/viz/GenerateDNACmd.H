/**
 * @file
 *
 *
 */
#ifndef  GENERATEDNACMD_H
#define  GENERATEDNACMD_H

#include "viz/FragCloneCmd.H"

class GenerateDNACmd : public FragCloneCmd
{
   public:
      GenerateDNACmd(const string& name, Receiver * receiver);
      virtual ~GenerateDNACmd();

      virtual bool execute();

   protected:
      virtual void init();


};

#endif




