//  The "Launch Info" rows of a calculation summary (#648), shared by the
//  Organizer's summary and the Viewer's Calculation Summary. Header-only and
//  free of wx and ECCE types so the order and the hiding rules are tested
//  alone (tests/machregister/testLaunchInfoRows.C).
//
//  Left column: Machine, Processors, Queue, Job ID. Right column: Start Date,
//  Comp. Date, Cost (CPU-h), Allocation, Left. A row whose value is not known is not made at
//  all, so there is nothing to hide.
#ifndef LAUNCHINFOROWS_HH
#define LAUNCHINFOROWS_HH

#include <cstdio>
#include <string>
#include <vector>

namespace LaunchInfoRows
{
    struct Row
    {
        std::string label;
        std::string value;
    };

    struct Columns
    {
        std::vector<Row> left;
        std::vector<Row> right;
    };

    struct Input
    {
        std::string machine;
        long processors;          // <= 0: unknown
        std::string queue;
        std::string jobId;
        std::string startDate;    // as shown
        std::string completionDate;
        std::string coreHours;    // coreHoursText(), or ""
        std::string allocation;
        std::string allocationLeft;
        Input() : processors(0) {}
    };

    //  The id is the scheduler's only when the job went through a queue; a
    //  workstation job stores a process id there, which says nothing to the user.
    inline bool showJobId(const std::string& jobId, const std::string& queue)
    {
        return !jobId.empty() && !queue.empty();
    }

    //  What the job costs: cores x wall time, start to end (or to now for a
    //  running job, marked "so far"). "" when the start or the cores are unknown.
    inline std::string coreHoursText(long processors, long startSec,
                                     long endSec, bool soFar)
    {
        if (processors <= 0 || startSec <= 0 || endSec < startSec)
            return "";
        const double h = processors * (endSec - startSec) / 3600.0;
        char buf[64];
        snprintf(buf, sizeof buf, h < 100 ? "%.1f" : "%.0f", h);
        //  Thousands grouped and CPU-h, as the allocation line has them.
        std::string n = buf, out;
        const size_t dot = n.find('.') == std::string::npos ? n.size() : n.find('.');
        for (size_t i = 0; i < n.size(); i++) {
            if (i > 0 && i < dot && (dot - i) % 3 == 0) out += ' ';
            out += n[i];
        }
        return out + " CPU-h" + (soFar ? " so far" : "");
    }

    inline void add(std::vector<Row>& col, const char *label,
                    const std::string& value)
    {
        if (!value.empty()) {
            Row r;
            r.label = label;
            r.value = value;
            col.push_back(r);
        }
    }

    inline Columns build(const Input& in)
    {
        Columns c;
        add(c.left, "Machine:", in.machine);
        add(c.left, "Processors:",
            in.processors > 0 ? std::to_string(in.processors) : std::string());
        add(c.left, "Queue:", in.queue);
        add(c.left, "Job ID:",
            showJobId(in.jobId, in.queue) ? in.jobId : std::string());
        add(c.right, "Start Date:", in.startDate);
        add(c.right, "Comp. Date:", in.completionDate);
        add(c.right, "Cost:", in.coreHours);
        add(c.right, "Allocation:", in.allocation);
        add(c.right, "Left:", in.allocationLeft);
        return c;
    }
}

#endif
