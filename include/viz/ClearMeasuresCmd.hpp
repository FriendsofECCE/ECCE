/**
 * @file
 *
 *
 */
#ifndef  CLEARMEASURESCMD_H
#define  CLEARMEASURESCMD_H

#include "util/Command.H"

/**
 * Command wrapper for removing measures.
 */
class ClearMeasuresCmd : public Command
{
   public:
      ClearMeasuresCmd(const string& name, Receiver * receiver);
      virtual ~ClearMeasuresCmd();

      virtual bool execute() throw(EcceException);
      virtual bool isUndoable() const;
      virtual void undo();

   protected:
      virtual void init();

};

#endif




