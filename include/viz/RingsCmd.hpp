/**
 * @file
 *
 *
 */
#ifndef  RINGSCMD_H
#define  RINGSCMD_H

#include "util/Command.H"

class Fragment;

class RingsCmd : public Command
{
   public:
      RingsCmd(const string& name, Receiver * receiver);
      virtual ~RingsCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      virtual void init();

      bool p_lastState;


};

#endif




