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
    Input in;
    in.machine = "kebnekaise";
    in.processors = 28;
    in.queue = "batch";
    in.jobId = "12345";
    in.startDate = "2026-10-28";
    in.completionDate = "2026-10-29";
    in.allocation = "hpc2n2026-001";
    in.allocationLeft = "21.2 % used";
    Columns c = build(in);
    check(labels(c.left) == "Machine: Processors: Queue: Job ID:", "left column order");
    check(labels(c.right) == "Start Date: Comp. Date: Allocation: Left:", "right column order");
    check(c.left[3].value == "12345", "job id value");

    in.allocationLeft = "";
    c = build(in);
    check(labels(c.right) == "Start Date: Comp. Date: Allocation:", "empty Left makes no row");

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

    printf(failures ? "FAILED\n" : "all passed\n");
    return failures ? 1 : 0;
}
