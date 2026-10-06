///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: SessionLease.H
//
// DESCRIPTION:
//    Which ECCE programs of a session are alive, without /proc (#233), so
//    session end and the reaper work where there is no /proc (macOS,
//    Windows). Every ECCE program with a session id holds an exclusive
//    lock on $ECCE_REALUSERHOME/.ECCE/leases/<session key>/<name>.<pid>
//    for as long as it runs; the kernel drops the lock however it ends. A
//    lease whose lock can be taken is stale and is removed by whoever
//    finds it. ecce-session-lib.sh reads the same files with flock(1) (ecce-flock where there is none).
//
//    ECCE_SESSION_LIVENESS picks the evidence: "lease", "proc", or unset
//    for both where /proc exists (Linux) and the lease elsewhere.
///////////////////////////////////////////////////////////////////////////////
#ifndef SESSIONLEASE_HH
#define SESSIONLEASE_HH

#include <string>
#include <vector>

namespace SessionLease {

struct Holder {
  std::string key;    // session key, <host>_<id>
  std::string name;   // program name, e.g. "builder"
  long pid;
};

// Takes this process's lease, once. Does nothing without a session id or
// ECCE_REALUSERHOME, or when this program is not one of $ECCE_HOME/bin's
// (the rule the /proc scan applies).
void acquire();

// The live leases below statedir/leases, of one session key or ("") of
// every key of this host. Stale leases found on the way are removed.
std::vector<Holder> live(const std::string& statedir,
                         const std::string& key = "");

bool useLease();
bool useProc();

// The running ECCE programs that still use this account's data server
// and broker, apart from this process's session: every program of another
// session of this state directory (on any display, or with no id), and a
// job store or job master of any session, since a job reports through
// both. Holder::key is "" for a program without a session id. The same
// rule as ecce_others_using_services in ecce-session-lib.sh.
std::vector<Holder> othersUsingServices();

}  // namespace SessionLease

#endif
