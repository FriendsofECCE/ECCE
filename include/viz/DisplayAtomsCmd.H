/**
 * @file
 *
 *
 */
#ifndef  DISPLAYATOMSCMD_H
#define  DISPLAYATOMSCMD_H

#include "util/Command.H"

class Fragment;

class DisplayAtomsCmd : public Command
{
   public:
      DisplayAtomsCmd(const string& name, Receiver * receiver);
      virtual ~DisplayAtomsCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

      virtual bool isUndoable() const;

   protected:
      virtual void init();

};

#endif // DISPLAYATOMSCMD_H
