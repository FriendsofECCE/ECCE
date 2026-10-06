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

class TaskJob;

namespace JobCatchUp {

// On failure the calculation is left waiting for login, with the reason.
bool reconnect(TaskJob* calc, std::string& message);

struct Result {
  std::string url;
  std::string message;
  bool ok;
};

// Every waiting calculation not already watched by a running monitor.
// progress(done, total) is called before each one.
std::vector<Result> run(
    std::function<void(size_t, size_t)> progress = nullptr);

}

#endif
