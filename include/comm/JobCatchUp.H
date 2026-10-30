///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: JobCatchUp.H
//
// DESCRIPTION:
//    Catching up calculations whose monitoring stopped while the client was
//    away (#208).  reconnect() is Run Management > Reconnect without its
//    dialogs: it starts a fresh job monitor, which reads the job's files
//    from the start with the same parsers as live monitoring, so a finished
//    job ends Completed (or Failed, as it really ended) with its
//    properties, and a running one is monitored again.  run() does that for
//    every calculation recorded in WaitingJobs, at session start.
//
//    Wx-free; the Organizer and tests/launch/launchjob call it.
///////////////////////////////////////////////////////////////////////////////
#ifndef JOBCATCHUP_HH
#define JOBCATCHUP_HH

#include <functional>
#include <string>
#include <vector>

#include "comm/RCommand.H"

class TaskJob;

namespace JobCatchUp {

// On failure the calculation is left waiting for login, with the reason.
// Opens a login; the Organizer runs it off its GUI thread, so that a host
// that does not answer can be abandoned.  False (with why) when no login.
using LoginFn = std::function<bool(const RCommandLogin&, RCommand*& login,
                                   std::string& why)>;

// What opens the login of the calculation's machine; false if unregistered.
bool loginFor(const TaskJob* calc, RCommandLogin& login, std::string& why);

// Runs the remote work after the login (copying the job monitor, its
// commands); the Organizer runs it off its GUI thread. work owns what it
// uses, so an abandoned one finishes alone. False: cancelled.
using RemoteFn = std::function<bool(const std::string& what,
                                    const std::function<bool()>& work,
                                    bool& result)>;

// forget: drop the remembered cancelled or refused logins first, so that
// the user's Reconnect may ask again; a run over many jobs does it once, so
// that one Cancel is not followed by a dialog per job.
bool reconnect(TaskJob* calc, std::string& message,
               const LoginFn& login = nullptr,
               const RemoteFn& remote = nullptr, bool forget = true);

// Re-Parse Outputs (#651): the code's parsers and the progress record run
// again over the outputs stored with the calculation, with no login to the
// compute machine. Properties are replaced; run state, dates and the status
// reason are kept. The job store does the work in the background.
bool reparse(TaskJob* calc, std::string& message);

// One calculation of Reconnect Selected (#601): what the dialog lists.
struct Plan {
  std::string url, name, machine, state;
  std::string problem;       // empty: can be reconnected
};

Plan plan(TaskJob* calc);

struct Result {
  std::string url;
  std::string message;
  bool ok;
};

// Every waiting calculation not already watched by a running monitor.
// progress(done, total) is called before each one.
// progress returning false (Cancel) leaves the rest waiting.
std::vector<Result> run(
    std::function<bool(size_t, size_t)> progress = nullptr,
    const LoginFn& login = nullptr,
    const RemoteFn& remote = nullptr);

// ecce-fetch: copy the run directory's files matching `patterns` (file-name
// globs) over the calculation's login into its stored Outputs, or only list
// the directory. One line per result: "name\tok\tbytes", "pattern\tnone\t",
// "\tls\ttext" (list) or "calc\tno\treason".
bool fetchFiles(TaskJob* calc, const std::vector<std::string>& patterns,
                bool listOnly, const LoginFn& login, const RemoteFn& remote,
                std::vector<std::string>& lines);

}

#endif
