/**
 * @file
 *
 *
 */
#ifndef  CSRADIICMD_H
#define  CSRADIICMD_H

#include "util/Command.H"

class Fragment;

class CSRadiiCmd : public Command
{
   public:
      CSRadiiCmd(const string& name, Receiver * receiver);
      virtual ~CSRadiiCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      virtual void init();


};

#endif




