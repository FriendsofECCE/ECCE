//  The allocation of each cluster account and how much of it is used (#613),
//  for Tools > Allocations in the Organizer. wx-free; the caller turns onDone
//  into a CallAfter.
#ifndef ALLOCATIONQUERY_HH
#define ALLOCATIONQUERY_HH

#include <functional>
#include <string>
#include <vector>

#include "dsm/TaskJob.H"

namespace AllocationQuery
{
    //  The stored account, else the one in the machine's profile (Default when
    //  none was stored), for a calculation launched before #541. "" if none.
    std::string account(const Launchdata& l);

    struct Target
    {
        std::string machine, account;
    };

    //  One per registered machine and account found in its profiles, for the
    //  machines that have an allocation command and a remote connection.
    std::vector<Target> targets();

    struct Result
    {
        enum State { Value, Checking, NotLoggedIn } state;
        std::string host;       // the cluster the login is to
        std::string used;       // "" when the output was not recognised
        std::string raw;        // the output line naming the account, for a tooltip
        long when;              // time of the answer shown, 0 if none
        Result() : state(NotLoggedIn), when(0) {}
    };

    //  What is known now. Answers are kept 15 minutes per cluster and account;
    //  a refresh runs on a background thread and only over an ssh connection
    //  that is already logged in, so it never asks for a password. force
    //  asks again at once. onDone is called from that thread when a refresh
    //  has finished.
    Result check(const Target& t, bool force, std::function<void()> onDone);
}

#endif
