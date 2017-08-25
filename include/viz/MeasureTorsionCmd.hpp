/**
 * @file
 *
 *
 */
#ifndef  MEASURETORSIONCMD_H
#define  MEASURETORSIONCMD_H

#include "util/Command.H"
#include "viz/MeasureBaseCmd.H"

class Fragment;

/**
 * Command wrapper for adding a torsion measure.
 */
class MeasureTorsionCmd : public MeasureBaseCmd
{
   public:
      MeasureTorsionCmd(const string& name, Receiver * receiver);
      virtual ~MeasureTorsionCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif
