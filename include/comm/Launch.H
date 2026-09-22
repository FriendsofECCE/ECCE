///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Launch.H
//
//
// CLASS SYNOPSIS:
//   Interface to iteratively perform operations associated with
//   launching a job.
//
// EXPORTED TYPES:
//    Class Launch
//
// LOCAL TYPES:
//
// DESCRIPTION:
//   The Lauch interface allows the client program to instance
//   a Launch and then sequentially perform all the steps necessary
//   to complete the launch.  The iterative approach allows the
//   caller to get status messages after each operation.
//
//   After any step fails, no subsequent operations can be performed.
//
// EXAMPLES:
///////////////////////////////////////////////////////////////////////////////
#ifndef LAUNCH_HH
#define LAUNCH_HH

#include <string>
  using std::string;

class Launch;
class RCommand;
class CalcInfo;
class TaskJob;
class EcceMap;


typedef bool (Launch::*launchOperation)(void);

typedef struct _LaunchData {
  const char *description;
  launchOperation operation;
} LaunchData;


class Launch
{

public:

  // Constructors
        Launch(TaskJob * taskjob,
               EcceMap& kvargs,
               const bool& fullLaunchFlag=true,
               const string& importDir="", const string& importName="");

  // Virtual Destructor
        virtual ~Launch(void);

  // Modifiers
        bool nextOperation(void);

  // Accessors
        string description(void) const;
        string message(void) const;
        string info(void) const;

        // Where the generated input deck and submit script were written.
        // Meaningful for a noRemoteAccess launch (issue #94), where that
        // directory is the deliverable: the user takes it from there.
        string stagingDirectory(void) const;
        // True when this launch only prepared files locally.
        bool   isLocalOnly(void) const;
        bool done(void) const;
        int       percentDone(void) const;

  // Public Operations (not called by nextOperation but directly)
        bool validateLocalDir(void);
        bool validateRemoteLogin(void);
        bool generateJobMonitoringFiles(void);
        bool moveJobMonitoringFiles(void);
        bool startJobStore(const string& importDir);
        bool doLaunch(void);

private:
  // State
        // The full pipeline: remote login, local preparation, remote
        // checks, transfer, submit.
        static const LaunchData p_opData[];
        // Local preparation only, for machines flagged noRemoteAccess
        // (issue #94). Same operations, in the same order, minus every
        // step that needs to reach the machine.
        static const LaunchData p_opDataLocal[];
        // Whichever of the two this launch is using.
        const LaunchData       *p_ops;
        TaskJob                *p_taskjob;
        TaskJob                *p_restartProvider;
        TaskJob                *p_topologyProvider;
        TaskJob                *p_fragmentProvider;
        bool                    p_isPrepareTask;
        bool                    p_isMetadynamicsTask;
        EcceMap                *p_options;
        unsigned long           p_numOps;
        // True when this launch only prepares files locally (issue #94).
        bool                    p_localOnly;
        unsigned long           p_curOp;
        RCommand               *p_connection;
        RCommand               *p_localconn;
        bool                    p_valid;
        string                  p_lastMessage;
        string                  p_infoMessage;
        CalcInfo               *p_cache;
        string                  p_inputFile;

  // Encapsulated Behavior
        void cacheCalcInfo(const string& importDir, const string& importName);
        bool postProcessInput(void);
        bool instanceScript(EcceMap& kv);
        void instanceCommand(string& command,
                             EcceMap& kv);
        void substituteRealFileName(string& fileName);

  // The Operations
        bool validateCalculation(void);
        bool validateRemotePerl(void);
        bool generateJobSubmissionFile(void);
        bool validateRemoteDir(void);
        bool validateScratchDir(void);
        bool moveFiles(void);
        bool launchNormal(void);
        bool launchGlobus(void);

        ///////////
        // Operations helper functions
        ///////////
        void changeWD(const string& dir);
        bool checkLocalDir(const string& dir);
        bool checkRemoteDir(const string& dir, const bool& rerunCheck);
        bool checkDiskSpace(const string& dir);
        void updateState(void);
        bool checkLocalCmd(const string& command,const string& msg);
        bool checkRemoteCmd(const string& command,const string& msg);
        bool generateDescriptorFile(const string& source, const string& target);

}; // Launch

#endif /* LAUNCH_HH */
