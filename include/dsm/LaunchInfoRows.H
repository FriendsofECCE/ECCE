//  The "Launch Info" rows of a calculation summary (#648), shared by the
//  Organizer's summary and the Viewer's Calculation Summary. Header-only and
//  free of wx and ECCE types so the order and the hiding rules are tested
//  alone (tests/machregister/testLaunchInfoRows.C).
//
//  Left column: Machine, Processors, Queue, Job ID; below both, full width: Queue state. Right column: Start Date,
//  Comp. Date, Cost (CPU-h), Wall time, Allocation, Remaining on submission. A row whose value is not known is not made at
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
        std::vector<Row> full;    // under both columns, full width
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
        std::string wallTime;     // wallTimeText(), or ""
        std::string allocation;
        std::string allocationRemaining;   // snapshot at submission, or ""
        std::string queueWait;    // why a Submitted job waits (#641), or ""
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

    //  The Launcher's wall time, "D H:M"; 0 when none was set.
    inline long maxWallSeconds(const std::string& maxwall)
    {
        int d = 0, h = 0, m = 0;
        if (sscanf(maxwall.c_str(), "%d %d:%d", &d, &h, &m) != 3)
            return 0;
        return ((long)d * 24 + h) * 3600L + m * 60L;
    }

    inline std::string durationText(long sec)
    {
        if (sec < 0) sec = 0;
        const long min = (sec + 59) / 60, d = min / 1440, h = min % 1440 / 60,
                   m = min % 60;
        std::string t;
        if (d) t += std::to_string(d) + " d";
        if (h) t += (t.empty() ? "" : " ") + std::to_string(h) + " h";
        if (m && !d) t += (t.empty() ? "" : " ") + std::to_string(m) + " min";
        return t.empty() ? "0 min" : t;
    }

    enum Phase { WAITING, RUNNING, ENDED };

    //  How much of the requested wall time is left, so a job about to be cut
    //  off by the scheduler shows it. "" without a limit or a start date.
    inline std::string wallTimeText(long limitSec, Phase phase, long startSec,
                                    long nowOrEndSec)
    {
        if (limitSec <= 0)
            return "";
        const std::string limit = durationText(limitSec);
        if (phase == WAITING)
            return limit + " requested";
        if (startSec <= 0 || nowOrEndSec < startSec)
            return "";
        const long used = nowOrEndSec - startSec;
        if (phase == ENDED)
            return "used " + durationText(used) + " of " + limit;
        if (used >= limitSec)
            return "limit of " + limit + " reached";
        std::string t = durationText(limitSec - used) + " left of " + limit;
        if ((limitSec - used) * 10 < limitSec)
            t += " (nearly out)";
        return t;
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
        add(c.full, "Queue state:", in.queueWait);
        add(c.right, "Start Date:", in.startDate);
        add(c.right, "Comp. Date:", in.completionDate);
        add(c.right, "Cost:", in.coreHours);
        add(c.right, "Wall time:", in.wallTime);
        add(c.right, "Allocation:", in.allocation);
        add(c.right, "Remaining on submission:",
            in.allocation.empty() ? std::string() : in.allocationRemaining);
        return c;
    }
}

#endif
