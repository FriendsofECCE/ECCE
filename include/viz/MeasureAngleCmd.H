/**
 * @file
 *
 *
 */
#ifndef  MEASUREANGLECMD_H
#define  MEASUREANGLECMD_H

#include "util/Command.H"
#include "viz/MeasureBaseCmd.H"

class Fragment;

/**
 * Command wrapper for adding a angle measure.
 */
class MeasureAngleCmd : public MeasureBaseCmd
{
   public:
      MeasureAngleCmd(const string& name, Receiver * receiver);
      virtual ~MeasureAngleCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif
