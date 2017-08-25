#ifndef CLEANCMD_HH
#define CLEANCMD_HH


#include "util/ProgressMonitor.H"
#include "viz/FragCloneCmd.H"


/**
 * Command wrapper for universal forcefield clean.
 */
class CleanCmd : public FragCloneCmd
{
   public:
      CleanCmd(const string& name, Receiver * receiver, ProgressMonitor *mon=0);
      ~CleanCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

      ProgressMonitor *p_monitor;
      /** Since clean is slow, we want to save the redo frag and use it
          if available in execute */
      SGFragment *p_redo;


};
#endif
