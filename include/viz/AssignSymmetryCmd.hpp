/**
 * @file
 *
 *
 */
#ifndef  ASSIGNSYMMETRYCMD_H
#define  ASSIGNSYMMETRYCMD_H

#include "util/Command.H"

class Fragment;

/**
 * Executes the fortran code that does find symmetry.
 *
 */
class AssignSymmetryCmd : public Command
{
   public:
      AssignSymmetryCmd(const string& name, Receiver * receiver);
      virtual ~AssignSymmetryCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      virtual void init();
      string p_undoGroup;

};

#endif




