/**
 * @file
 *
 *
 */
#ifndef  ADDSEGMENTHCMD_H
#define  ADDSEGMENTHCMD_H

#include "viz/FragCloneCmd.H"

class AddSegmentHCmd : public FragCloneCmd
{
   public:
      AddSegmentHCmd(const string& name, Receiver * receiver);
      virtual ~AddSegmentHCmd();

      virtual bool execute();

   protected:
      virtual void init();

};

#endif




