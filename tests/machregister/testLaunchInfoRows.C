//  Driver for include/dsm/LaunchInfoRows.H (#648): two columns in the agreed
//  order, empty values make no row, Job ID only for a queued job.
#include <cstdio>
#include <string>
#include "../../include/dsm/LaunchInfoRows.H"

using namespace LaunchInfoRows;

static int failures = 0;

static void check(bool ok, const char *what)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

static std::string labels(const std::vector<Row>& v)
{
    std::string s;
    for (size_t i = 0; i < v.size(); i++)
        s += v[i].label + (i + 1 < v.size() ? " " : "");
    return s;
}

int main()
{
    {
        Input w;
        w.queue = "normal";
        w.queueWait = "waiting: other jobs have priority";
        Columns cw = build(w);
        check(labels(cw.left) == "Queue:" && labels(cw.full) == "Queue state:",
              "queue wait is a full-width row under both columns (#657)");
        w.queueWait = "";
        check(labels(build(w).left) == "Queue:" && build(w).full.empty(),
              "no queue wait: no row");
    }
    Input in;
    in.machine = "kebnekaise";
    in.processors = 28;
    in.queue = "batch";
    in.jobId = "12345";
    in.startDate = "2026-10-28";
    in.completionDate = "2026-10-29";
    in.allocation = "hpc2n2026-001";
    Columns c = build(in);
    check(labels(c.left) == "Machine: Processors: Queue: Job ID:", "left column order");
    check(labels(c.right) == "Start Date: Comp. Date: Allocation:", "right column order, no Left row");
    check(c.left[3].value == "12345", "job id value");

    in.allocationRemaining = "95.1 % of 5 000 CPU-h/month (past 30 days)";
    c = build(in);
    check(labels(c.right) == "Start Date: Comp. Date: Allocation: Remaining on submission:",
          "remaining row follows Allocation");
    in.allocation = "";
    c = build(in);
    check(labels(c.right) == "Start Date: Comp. Date:", "no allocation: no remaining row either");
    in.allocation = "hpc2n2026-001";
    in.allocationRemaining = "";
    c = build(in);
    check(labels(c.right) == "Start Date: Comp. Date: Allocation:", "no snapshot: no remaining row");

    in.completionDate = "";
    in.allocation = "";
    c = build(in);
    check(labels(c.right) == "Start Date:", "empty Comp. Date and Allocation make no rows");

    in.queue = "";
    c = build(in);
    check(labels(c.left) == "Machine: Processors:", "no queue: no Queue, no Job ID (pid)");
    check(!showJobId("", "batch"), "queued job without an id shows none");

    in.processors = 0;
    in.machine = "";
    c = build(in);
    check(c.left.empty(), "unknown processors and machine make no rows");

    // Cost: cores x hours, thousands grouped, "so far" while running.
    check(coreHoursText(128, 0 + 1000, 1000 + 9 * 3600, false) == "1 152 CPU-h",
          "128 cores for 9 h is 1 152 CPU-h");
    check(coreHoursText(4, 1000, 1000 + 1800, true) == "2.0 CPU-h so far",
          "a running job's cost says so far");
    check(coreHoursText(0, 1000, 2000, false).empty() &&
          coreHoursText(4, 0, 2000, false).empty() &&
          coreHoursText(4, 3000, 2000, false).empty(),
          "no cores, no start, or an end before the start: no cost");
    in.coreHours = "5.0 CPU-h";
    in.machine = "m";
    c = build(in);
    check(labels(c.right).find("Comp. Date: Cost:") != std::string::npos ||
          labels(c.right).find("Cost:") != std::string::npos,
          "Cost is in the right column");

    {
        check(maxWallSeconds("0 4:00") == 4 * 3600 &&
              maxWallSeconds("1 2:30") == 26 * 3600 + 1800 &&
              maxWallSeconds("") == 0, "wall time parsed from D H:M");
        const long L = 4 * 3600;
        check(wallTimeText(L, WAITING, 0, 0) == "4 h requested", "queued: requested");
        check(wallTimeText(L, RUNNING, 1000, 1000 + 2 * 3600 + 40 * 60) ==
              "1 h 20 min left of 4 h", "running: left of the limit");
        check(wallTimeText(L, RUNNING, 1000, 1000 + L - 600) ==
              "10 min left of 4 h (nearly out)", "under 10 % left: nearly out");
        check(wallTimeText(L, RUNNING, 1000, 1000 + L + 5) ==
              "limit of 4 h reached", "past the limit");
        check(wallTimeText(L, ENDED, 1000, 1000 + 3 * 3600 + 12 * 60) ==
              "used 3 h 12 min of 4 h", "ended: used of the limit");
        check(wallTimeText(0, RUNNING, 1000, 2000).empty(), "no limit: no row");
        Input w;
        w.queue = "batch";
        w.queueWait = "waiting: no node free";
        w.coreHours = "317 CPU-h so far";
        w.wallTime = "1 h 20 min left of 4 h";
        w.allocation = "hpc2n2026-351";
        Columns cw = build(w);
        check(labels(cw.right) == "Cost: Wall time: Allocation:" &&
              labels(cw.full) == "Queue state:",
              "wall time after Cost; queue state stays a row of its own");
    }
    printf(failures ? "FAILED\n" : "all passed\n");
    return failures ? 1 : 0;
}
