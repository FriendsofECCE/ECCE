///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: eccejobstore.C
//
//
// USAGE:
//  eccejobstore [-v] [-jobId jobId -configFile configFile]
//
// PURPOSE:
//  eccejobstore.C implements the client side of job monitoring.
//  It receives message blocks from eccejobmonitor running on the
//  compute server and processes them.  This includes parsing properties
//  into the database and sending messages to update other apps.
//
// DESCRIPTION:
//  (To be filled in later.)
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h> // exit
#include <string.h>

#include <iostream>
  using std::cout;
  using std::endl;
 
#include "tdat/AuthCache.H"

#include "comm/RCommand.H"

int main(int argc, char** argv)
{
  extern void jobstore_main(RCommand* remoteconn, const string& jobId,
              const string& configFileName, const bool& restartFlag);

  string jobId, configFileName;
  bool restartFlag;

  for (int it = 1; it < argc; it++) {
    if (strcmp(argv[it], "-pipe") == 0)
      AuthCache::getCache().pipeIn(argv[++it]);
    else if (strcmp(argv[it], "-jobId") == 0)
      jobId = argv[++it];
    else if (strcmp(argv[it], "-configFile") == 0)
      configFileName = argv[++it];
    else if (strcmp(argv[it], "-restart") == 0)
      restartFlag = true;
    else if (strcmp(argv[it], "-v") == 0) {
      cout << argv[0] << ": $Revision: 22539 $" << endl;
      exit(0);
    }
  }

  // passing remote connection as 0 causes jobstore_main() to create it
  jobstore_main((RCommand*)0, jobId, configFileName, restartFlag);

  // should never get here because jobstore_main() exits
  // and does not return
  exit(0);
}

