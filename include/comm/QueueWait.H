//  Why a Submitted job waits (#641): Slurm's pending reason and estimated
//  start, as the monitor sends them ("Reason|2026-10-30T12:20:00Z", the time
//  in UTC), turned into the line the Organizer shows. Header-only and free
//  of wx and ECCE types so it is tested alone (tests/launch/testQueueWait.C).
#ifndef QUEUEWAIT_HH
#define QUEUEWAIT_HH

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

namespace QueueWait
{
    //  Plain words for a reason; an unknown one is shown as Slurm gives it.
    inline std::string reasonText(const std::string& r)
    {
        if (r == "Resources")
            return "no free resources yet";
        if (r == "Priority")
            return "other jobs have priority";
        if (r == "BadConstraints")
            return "no node of the requested type is free right now (BadConstraints)";
        if (r == "QOSMaxJobsPerUserLimit")
            return "the limit on running jobs per user is reached "
                   "(QOSMaxJobsPerUserLimit)";
        if (r.compare(0, 8, "AssocGrp") == 0 &&
            r.find("Minutes") != std::string::npos)
            return "allocation used up (" + r + ")";
        if (r == "ReqNodeNotAvail")
            return "a requested node is not available (ReqNodeNotAvail)";
        if (r == "Dependency")
            return "waiting for another job to finish";
        if (r == "JobHeldUser")
            return "the job is held by the user (JobHeldUser)";
        if (r == "JobHeldAdmin")
            return "the job is held by an administrator (JobHeldAdmin)";
        return r;
    }

    //  "Fri 30 Oct 14:20" in local time from a UTC ISO time; "" for N/A or
    //  anything unreadable.
    inline std::string startText(const std::string& iso)
    {
        int y, mo, d, h, mi, s;
        char z = 0;
        if (sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d%c", &y, &mo, &d, &h, &mi,
                   &s, &z) < 6)
            return "";
        struct tm t = {};
        t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d;
        t.tm_hour = h; t.tm_min = mi; t.tm_sec = s;
#ifdef _WIN32
        const time_t when = _mkgmtime(&t);
#else
        const time_t when = timegm(&t);
#endif
        if (when == (time_t)-1)
            return "";
        struct tm loc;
        localtime_r(&when, &loc);
        char buf[64];
        strftime(buf, sizeof buf, "%a %d %b %H:%M", &loc);
        return buf;
    }

    //  The monitor's value as one line; "" when it carries no reason.
    inline std::string text(const std::string& value)
    {
        const size_t bar = value.find('|');
        const std::string reason = value.substr(0, bar);
        if (reason.empty() || reason == "None")
            return "";
        std::string out = "waiting: " + reasonText(reason);
        const std::string start =
            bar == std::string::npos ? "" : startText(value.substr(bar + 1));
        if (!start.empty())
            out += "; estimated start " + start;
        return out;
    }
}

#endif
