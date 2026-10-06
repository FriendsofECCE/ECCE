/**
 * @file
 *
 *  Asking a machine's batch scheduler about itself, for Register Machines
 *  (#212): which queues it has (discovery) and whether it would accept a job
 *  script (a test submission that never runs a calculation).  No wx here, so
 *  the parsers can be exercised without a window.
 *
 *  Every command is an argv.  Each word is single-quoted before it reaches
 *  the remote shell (RCommand takes a command string), so nothing the user
 *  typed is ever interpreted as shell syntax.
 */
#ifndef SCHEDULERQUERY_HH
#define SCHEDULERQUERY_HH

#include <string>
#include <vector>

class RCommand;

namespace SchedulerQuery
{
    using std::string;
    using std::vector;

    /// What RCommand needs to reach the machine, from the Register Machines form.
    struct Connection
    {
        string machine, remShell, locShell, userName, password,
               frontendMachine, frontendBypass, shellPath, libPath, sourceFile;
    };

    /// One queue as the scheduler reports it, in the units of the .Q file:
    /// wall time in minutes, memory in MB; 0 means the scheduler gave no limit.
    struct Queue
    {
        string name;
        unsigned maxProcs, maxWallMin, maxMemMB;
        string detail;          // the limits as the scheduler wrote them
        Queue() : maxProcs(0), maxWallMin(0), maxMemMB(0) {}
    };

    string quote(const string& word);
    string commandLine(const vector<string>& argv);

    /// A connection to the machine for a few commands.
    class Remote
    {
        public:
            explicit Remote(const Connection& c);
            ~Remote();
            bool open(string& err);
            /// Runs argv (words quoted); `output` holds stdout and stderr as the
            /// command wrote them.  True when it exited with status 0.
            bool run(const vector<string>& argv, string& output, int timeoutSec = 60);
            string error() const;

        private:
            Connection p_c;
            RCommand *p_rc;
            string p_err;
    };

    /// True when `qmgr` (as named on the Queues tab) can be asked for queues.
    bool canDiscover(const string& qmgr);
    /// The commands discovery runs, for the dialog ("sinfo -h -o ...").
    string discoveryCommand(const string& qmgr);
    bool discover(Remote& r, const string& qmgr, vector<Queue>& queues,
                  string& commands, string& err);

    //  Parsers of what the commands print; also called by discover().
    vector<Queue> parseSinfo(const string& text);
    vector<Queue> parsePbsQueues(const string& text);
    Queue parseSgeQueue(const string& name, const string& text);
    vector<Queue> parseLsfQueues(const string& text);
    vector<Queue> parseCondorSlots(const string& text);
    /// "30:00", "1-12:00:00", "infinite" -> minutes (0 for no limit).
    unsigned slurmMinutes(const string& text);

}

#endif
