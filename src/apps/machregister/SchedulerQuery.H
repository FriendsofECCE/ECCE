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
            /// Same, with a file as the command's standard input.
            bool runWithInput(const vector<string>& argv, const string& file,
                              string& output, int timeoutSec = 60);
            /// Copies a local file to `remoteDir` (absolute) under its own name.
            bool put(const string& local, const string& remoteDir, string& err);
            string home();
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

    /// The queue manager's own check of a script.  `holdAndCancel` is true when
    /// there is none and the script is submitted on hold and cancelled.
    struct TestPlan
    {
        bool known;
        bool holdAndCancel;
        string what;            // "sbatch --test-only"
        TestPlan() : known(false), holdAndCancel(false) {}
    };
    TestPlan testPlan(const string& qmgr);

    struct TestResult
    {
        bool ran, accepted, cancelled;
        string commands;        // what ran, one command per line
        string answer;          // the scheduler's text, verbatim
        TestResult() : ran(false), accepted(false), cancelled(false) {}
    };
    /// `script` is the text to send.  Never runs a calculation: the script is
    /// only checked, or held and cancelled.
    bool testSubmission(Remote& r, const string& qmgr, const string& script,
                        TestResult& result, string& err);
}

#endif
