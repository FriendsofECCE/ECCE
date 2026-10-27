//  The allocation a calculation was launched under and how much of it is
//  left (#613), for the Organizer's and the Viewer's calculation summaries.
//  wx-free; the caller turns onDone into a CallAfter.
#ifndef ALLOCATIONQUERY_HH
#define ALLOCATIONQUERY_HH

#include <functional>
#include <string>

#include "dsm/TaskJob.H"

namespace AllocationQuery
{
    //  The stored account, else the one in the machine's profile (Default when
    //  none was stored), for a calculation launched before #541. "" if none.
    std::string account(const Launchdata& l);

    //  "" when nothing is to be shown. Answers are kept 15 minutes per machine
    //  and account; a refresh runs on a background thread and only over an ssh
    //  connection that is already logged in, so it never asks for a password.
    //  onDone is called from that thread when a refresh has finished.
    std::string left(const Launchdata& l, std::function<void()> onDone);
}

#endif
