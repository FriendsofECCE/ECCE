#ifndef PROGRESSMONITOR_H
#define PROGRESSMONITOR_H

#include <string>
  using std::string;

/**
 * Interface for compute intensive routines to use to support
 * interrupt and progress updates.
 * Note that the progress updating part is currently deferred until
 * deemed useful.
 */
class ProgressMonitor 
{
   public:

      /* Use this if you want to show indeterminate progress */
      virtual bool isInterrupted() { return false; }

      /* Use this if you want to show indeterminate progress */
      virtual bool isInterrupted(const string& msg, int pct) { return false; }
};

#endif

