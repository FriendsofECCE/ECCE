///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: WaitingJobs.H
//
// DESCRIPTION:
//    The calculations whose monitoring stopped while the job may still be
//    running or finished (#208): the client went away, the login lapsed,
//    the monitor's restart budget ran out. Each is a file in
//    $HOME/.ECCE/waiting/ holding the calculation URL, written by the job
//    store when it parks a job and read at the next session start, which
//    catches it up (JobCatchUp). It lives with the client because the
//    client is what logs in to the compute machine; the data server keeps
//    only the run state.
//
//    A running eccejobmaster holds an flock on <hash>.lock for its
//    calculation, so catch-up never starts a second monitor beside it.
///////////////////////////////////////////////////////////////////////////////
#ifndef WAITINGJOBS_HH
#define WAITINGJOBS_HH

#include <string>
#include <vector>

namespace WaitingJobs {

std::string dir();

// Record / forget one calculation. Both are idempotent.
bool add(const std::string& url);
void remove(const std::string& url);

// The recorded calculation URLs, oldest first.
std::vector<std::string> list();

// Held by the monitor of url for as long as it runs; returns the
// descriptor (close it, or exit, to release) or -1.
int watch(const std::string& url);
void unwatch(const std::string& url, int fd);

// Whether a monitor holds url's lock now.
bool watched(const std::string& url);

// True the first time it is called for a session key, so that session
// start catches up once however often the Organizer is opened.
bool firstInSession(const std::string& key);

}

#endif
