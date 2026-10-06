/**
 * @file
 *
 *  The job script gensub would write, for Register Machines' preview (#212):
 *  built from a draft of the machine's CONFIG file, not from the saved one
 *  (GENSUB_USER_CONFIG, or GENSUB_SITE_CONFIG under -admin), and with each
 *  line tagged by the part of the script it belongs to (GENSUB_ANNOTATE).
 *  Nothing is submitted and nothing is written outside a temporary
 *  directory.  No wx here.
 */
#ifndef JOBPREVIEW_HH
#define JOBPREVIEW_HH

#include <string>
#include <vector>

namespace JobPreview
{
    struct Request
    {
        std::string host;         // the machine's name (gensub -H)
        std::string fullName;     // its host name (-d)
        std::string qmgr;         // as on the Queues tab; "" or None is Shell
        std::string code, queue, account;
        unsigned nodes, procs, memMB;   // memMB 0: not requested
        double wallHours;               // 0: not requested
        std::string runDir;       // "" is an example path
        std::string configText;   // the edited file as the form would write it
        bool admin;               // the edited file is the site file
        Request() : nodes(1), procs(1), memMB(0), wallHours(0), admin(false) {}
    };

    struct Line
    {
        std::string text;
        std::string part;         // ecce, request, before, environment, command, after
        std::string layer;        // user, site, vendor, submit.site, built-in, or ""
    };

    /// False with `err` (gensub's own message) when no script can be made,
    /// e.g. a code without a program path.
    bool generate(const Request& r, std::vector<Line>& lines, std::string& err);

    /// "Job script > Request lines" and so on, for the legend.
    std::string partTitle(const std::string& part);
    /// The text of the script alone, one line per entry.
    std::string plainText(const std::vector<Line>& lines);
}

#endif
