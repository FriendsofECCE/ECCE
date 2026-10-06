/**
 * @file
 *
 *  See JobPreview.H.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>

#include "util/Ecce.H"
#include "JobPreview.H"

using std::string;
using std::vector;

namespace JobPreview
{

static string numberText(double v)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%g", v);
    return buf;
}

//  Hours as gensub's H:MM:SS and H:MM.
static void wallText(double hours, string& hms, string& hm)
{
    long minutes = (long)(hours * 60.0 + 0.5);
    char a[64], b[64];
    snprintf(a, sizeof a, "%ld:%02ld:00", minutes / 60, minutes % 60);
    snprintf(b, sizeof b, "%ld:%02ld", minutes / 60, minutes % 60);
    hms = a;
    hm = b;
}

//  Runs argv with `env` added; stdout and stderr land in `output`.
static int runProgram(const vector<string>& argv,
                      const vector<std::pair<string, string> >& env,
                      string& output)
{
    int fd[2];
    if (pipe(fd) != 0)
        return -1;
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0)
    {
        close(fd[0]);
        dup2(fd[1], 1);
        dup2(fd[1], 2);
        close(fd[1]);
        for (size_t i = 0; i < env.size(); i++)
            setenv(env[i].first.c_str(), env[i].second.c_str(), 1);
        vector<char*> a;
        for (size_t i = 0; i < argv.size(); i++)
            a.push_back(const_cast<char*>(argv[i].c_str()));
        a.push_back(NULL);
        execvp(a[0], &a[0]);
        _exit(127);
    }
    close(fd[1]);
    char buf[4096];
    ssize_t n;
    while ((n = read(fd[0], buf, sizeof buf)) > 0)
        output.append(buf, (size_t)n);
    close(fd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

bool generate(const Request& r, vector<Line>& lines, string& err)
{
    lines.clear();
    char tmpl[] = "/tmp/ecce-preview-XXXXXX";
    if (mkdtemp(tmpl) == NULL)
    {
        err = "Cannot make a temporary directory.";
        return false;
    }
    string dir = tmpl;
    string params = dir + "/params", cfg = dir + "/draft.cfg",
           submit = dir + "/submit__preview";

    {
        std::ofstream f(cfg.c_str());
        f << r.configText;
    }
    string qm = r.qmgr;
    if (qm.empty() || qm == "None")
        qm = "Shell";
    string hms, hm;
    wallText(r.wallHours, hms, hm);
    {
        std::ofstream f(params.c_str());
        f << " -H " << r.host << "\n"
          << " -d " << (r.fullName.empty() ? r.host : r.fullName) << "\n"
          << " -Q " << qm << "\n"
          << " -c " << r.code << "\n";
        if (!r.queue.empty())
            f << " -q " << r.queue << "\n";
        if (!r.account.empty())
            f << " -a " << r.account << "\n";
        f << " -N " << r.nodes << "\n -n " << r.procs << "\n";
        if (r.wallHours > 0)
            f << " -T " << hms << "\n -w " << hm << "\n";
        if (r.memMB > 0)
            f << " -m " << r.memMB << "\n";
        f << " -r " << (r.runDir.empty() ? string("/path/to/run") : r.runDir)
          << "\n -i input\n -o output\n"
          << " -f " << submit << "\n";
    }

    vector<string> argv;
    argv.push_back("perl");
    argv.push_back(string(Ecce::ecceHome()) + "/scripts/gensub");
    argv.push_back("-p");
    argv.push_back(params);
    vector<std::pair<string, string> > env;
    env.push_back(std::make_pair(string("ECCE_HOME"), string(Ecce::ecceHome())));
    env.push_back(std::make_pair(string("GENSUB_ANNOTATE"), string("1")));
    //  The draft takes the place of the file being edited, and nothing of
    //  the user's own is read under -admin.
    env.push_back(std::make_pair(string(r.admin ? "GENSUB_SITE_CONFIG"
                                                : "GENSUB_USER_CONFIG"), cfg));
    if (r.admin)
        env.push_back(std::make_pair(string("ECCE_REALUSERHOME"), dir));

    string out;
    int rc = runProgram(argv, env, out);
    bool ok = rc == 0;
    if (!ok)
    {
        err = out.empty() ? "gensub exited with status " + numberText(rc) : out;
        while (!err.empty() && err[err.size() - 1] == '\n')
            err.erase(err.size() - 1);
    }
    else
    {
        std::ifstream in(submit.c_str());
        string l, part = "ecce", layer;
        while (std::getline(in, l))
        {
            if (l.compare(0, 14, "#@ecce-section") == 0)
            {
                std::istringstream w(l.substr(14));
                part = "ecce";
                layer = "";
                w >> part >> layer;
                continue;
            }
            Line x;
            x.text = l;
            x.part = part;
            x.layer = layer;
            lines.push_back(x);
        }
        if (lines.empty())
        {
            ok = false;
            err = "gensub wrote no script.";
        }
    }

    unlink(params.c_str());
    unlink(cfg.c_str());
    unlink(submit.c_str());
    rmdir(dir.c_str());
    return ok;
}

string partTitle(const string& part)
{
    if (part == "request") return "Job script > Request lines";
    if (part == "before") return "Job script > Commands run before the calculation";
    if (part == "environment") return "Codes > Environment variables";
    if (part == "command") return "Codes > Command line";
    if (part == "after") return "Job script > Commands run after the calculation";
    return "Written by ECCE";
}

string plainText(const vector<Line>& lines)
{
    string out;
    for (size_t i = 0; i < lines.size(); i++)
        out += lines[i].text + "\n";
    return out;
}

}   // namespace JobPreview
