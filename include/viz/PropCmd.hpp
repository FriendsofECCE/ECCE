/**
 * @file
 *
 *
 */
#ifndef  PROPCMD_H
#define  PROPCMD_H

#include "util/Command.H"

class IPropCalculation;

/**
 * A superclass for property commands which 
 * a) are typically undoable
 * b) need access to the calc.
 */
class PropCmd : public Command
{
   public:
      PropCmd(const string& name, Receiver * receiver, IPropCalculation *calc)
         : Command(name, receiver) { p_calc = calc; }
      virtual ~PropCmd() { p_calc = 0;}


      virtual bool execute() throw(EcceException) {return false;}
      virtual void undo() {;}
      virtual bool isUndoable() { return false;}
      IPropCalculation *getCalculation() { return p_calc;}

   protected:
      virtual void init() {;}

      IPropCalculation *p_calc;


};

#endif
