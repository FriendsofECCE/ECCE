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

bool reconnect(TaskJob* calc, std::string& message,
               const LoginFn& login = nullptr);

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
std::vector<Result> run(
    std::function<void(size_t, size_t)> progress = nullptr,
    const LoginFn& login = nullptr);

}

#endif
