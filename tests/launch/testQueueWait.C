//  Driver for include/comm/QueueWait.H (#641): reasons in plain words,
//  unknown ones verbatim, the estimated start in local time.
//    g++ -std=c++17 -o testQueueWait tests/launch/testQueueWait.C && ./testQueueWait
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../include/comm/QueueWait.H"

static int failures = 0;

static void check(bool ok, const char *what)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

static bool has(const std::string& s, const char *part)
{
    return s.find(part) != std::string::npos;
}

int main()
{
    setenv("TZ", "UTC", 1);
    tzset();
    check(QueueWait::text("Resources|2026-10-30T14:20:00Z") ==
          "waiting: no free resources yet; estimated start Fri 30 Oct 14:20",
          "Resources with a start time");
    check(QueueWait::text("Resources|N/A") == "waiting: no free resources yet",
          "N/A start: no start text");
    check(QueueWait::text("Priority") == "waiting: other jobs have priority",
          "no start field at all");
    check(has(QueueWait::text("BadConstraints|N/A"), "requested type is free right now"),
          "BadConstraints");
    check(has(QueueWait::text("AssocGrpCPUMinutesLimit|N/A"), "allocation used up"),
          "AssocGrpCPUMinutesLimit");
    check(has(QueueWait::text("AssocGrpBillingMinutes|N/A"), "allocation used up"),
          "AssocGrpBillingMinutes");
    check(has(QueueWait::text("QOSMaxJobsPerUserLimit|N/A"), "per user"),
          "QOSMaxJobsPerUserLimit");
    check(has(QueueWait::text("JobHeldAdmin|N/A"), "administrator"), "JobHeldAdmin");
    check(has(QueueWait::text("JobHeldUser|N/A"), "held by the user"), "JobHeldUser");
    check(has(QueueWait::text("Dependency|N/A"), "another job"), "Dependency");
    check(has(QueueWait::text("ReqNodeNotAvail|N/A"), "not available"),
          "ReqNodeNotAvail");
    check(QueueWait::text("SomethingNew|N/A") == "waiting: SomethingNew",
          "unknown reason verbatim");
    check(QueueWait::text("None|N/A").empty() && QueueWait::text("").empty(),
          "no reason: no text");
    setenv("TZ", "Europe/Stockholm", 1);
    tzset();
    check(QueueWait::startText("2026-10-30T12:20:00Z") == "Fri 30 Oct 13:20",
          "start shown in local time");
    printf(failures ? "FAILED\n" : "PASSED\n");
    return failures ? 1 : 0;
}
