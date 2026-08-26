/**
 * @file
 *
 *
 */
#ifndef  GENERATEPEPTIDECMD_H
#define  GENERATEPEPTIDECMD_H

#include "viz/FragCloneCmd.H"

class GeneratePeptideCmd : public FragCloneCmd
{
   public:
      GeneratePeptideCmd(const string& name, Receiver * receiver);
      virtual ~GeneratePeptideCmd();

      virtual bool execute();

   protected:
      virtual void init();


};

#endif




