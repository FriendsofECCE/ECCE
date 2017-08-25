/**
 * @file
 *
 *
 */

#ifndef RESTOREMASSCMD_H
#define RESTOREMASSCMD_H


#include "viz/FragCloneCmd.H"

class wxWindow;

class RestoreMassCmd : public FragCloneCmd
{
   public:
      RestoreMassCmd(const string& name, Receiver * receiver);
      ~RestoreMassCmd();

      virtual bool execute() throw(EcceException);

   protected:

      void init();


};
#endif
