/**
 * @file
 *
 *
 */
#ifndef  SHOWHYDROGENSCMD_H
#define  SHOWHYDROGENSCMD_H

#include "util/Command.H"


class ShowHydrogensCmd : public Command
{
   public:
      ShowHydrogensCmd(const string& name, Receiver * receiver);
      virtual ~ShowHydrogensCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      virtual void init();

      bool p_switch;

};

#endif
