/**
 * @file
 *
 *  See SchedulerQuery.H.
 */

#include "util/TempStorage.H"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <sstream>

#include "comm/RCommand.H"
#include "SchedulerQuery.H"

using std::map;
using std::string;
using std::vector;

namespace SchedulerQuery
{

static string lower(string s)
{
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

static string trim(const string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos)
        return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

static vector<string> lines(const string& text)
{
    vector<string> out;
    std::istringstream in(text);
    string l;
    while (std::getline(in, l))
        out.push_back(l);
    return out;
}

static vector<string> split(const string& s, char sep)
{
    vector<string> out;
    string cur;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == sep) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

//  The leading digits of s, 0 when there are none.
static unsigned leadingNumber(const string& s)
{
    unsigned long v = 0;
    size_t i = 0;
    while (i < s.size() && isspace((unsigned char)s[i]))
        i++;
    for (; i < s.size() && isdigit((unsigned char)s[i]); i++)
        v = v * 10 + (s[i] - '0');
    return (unsigned)v;
}

static bool isNumber(const string& s)
{
    return !s.empty() && s.find_first_not_of("0123456789") == string::npos;
}

//  "256gb", "4G", "8192kb", "1024" (bytes, as PBS says) -> MB.  ECCE's GB is
//  1000 MB (MemoryUnits.H), so 256gb is 256000 MB and shows as 256.
//  defaultUnit is the unit of a bare number: 'b' bytes, 'k' kilobytes.
static unsigned toMB(const string& text, char defaultUnit)
{
    string t = lower(trim(text));
    if (t.empty() || !isdigit((unsigned char)t[0]))
        return 0;
    double v = atof(t.c_str());
    size_t i = 0;
    while (i < t.size() && (isdigit((unsigned char)t[i]) || t[i] == '.'))
        i++;
    string u = trim(t.substr(i));
    char c = u.empty() ? defaultUnit : u[0];
    double mb = v;
    switch (c)
    {
        case 'b': mb = v / 1e6; break;
        case 'k': mb = v / 1e3; break;
        case 'm': mb = v; break;
        case 'g': mb = v * 1e3; break;
        case 't': mb = v * 1e6; break;
        default: break;
    }
    if (mb > 0 && mb < 1)
        return 1;
    return (unsigned)(mb + 0.5);
}

//  "HH:MM:SS", "MM:SS" is not used by PBS/SGE; plain digits are seconds.
static unsigned hmsMinutes(const string& text)
{
    string t = trim(text);
    if (t.empty() || !isdigit((unsigned char)t[0]))
        return 0;
    vector<string> p = split(t, ':');
    unsigned long sec = 0;
    if (p.size() == 1)
        sec = strtoul(p[0].c_str(), 0, 10);
    else if (p.size() == 3)
        sec = strtoul(p[0].c_str(), 0, 10) * 3600 +
              strtoul(p[1].c_str(), 0, 10) * 60 + strtoul(p[2].c_str(), 0, 10);
    else if (p.size() == 2)
        sec = strtoul(p[0].c_str(), 0, 10) * 3600 +
              strtoul(p[1].c_str(), 0, 10) * 60;
    return (unsigned)((sec + 59) / 60);
}

string quote(const string& word)
{
    string out = "'";
    for (size_t i = 0; i < word.size(); i++)
    {
        if (word[i] == '\'')
            out += "'\\''";
        else
            out += word[i];
    }
    return out + "'";
}

string commandLine(const vector<string>& argv)
{
    string out;
    for (size_t i = 0; i < argv.size(); i++)
        out += (i ? " " : "") + quote(argv[i]);
    return out;
}

//  What the user is shown: the words, quoted only where they need it.
static string shown(const vector<string>& argv)
{
    string out;
    for (size_t i = 0; i < argv.size(); i++)
    {
        const string& w = argv[i];
        bool plain = !w.empty() && w.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
            "_-./:=+%@,") == string::npos;
        out += (i ? " " : "") + (plain ? w : quote(w));
    }
    return out;
}

//  ---- finding a code's program ---------------------------------------------

ProgramHelp programHelp(const string& code)
{
    ProgramHelp h;
    if (code == "NWChem" || code == "NWChemMD")
    {
        h.names.push_back("nwchem");
        h.examples.push_back("/usr/bin/nwchem");
    }
    else if (code == "ORCA")
    {
        h.names.push_back("orca");
        h.companion = "orca_2mkl";
        h.examples.push_back("/opt/orca/<version>/orca");
        h.note = "ORCA needs the full path for parallel runs.";
    }
    else if (code.compare(0, 9, "Gaussian-") == 0 && code.size() == 11)
    {
        string g = "g" + code.substr(9);
        h.names.push_back(g);
        h.examples.push_back("/opt/" + g + "/" + g);
    }
    else if (code == "QuantumESPRESSO")
    {
        h.names.push_back("pw.x");
        h.examples.push_back("/usr/bin/pw.x");
    }
    else if (code == "MOPAC")
    {
        h.names.push_back("MOPAC2016.exe");
        h.names.push_back("mopac");
        h.examples.push_back("/opt/mopac/MOPAC2016.exe");
        h.examples.push_back("/usr/bin/mopac");
    }
    else if (code == "GROMACS")
    {
        h.names.push_back("gmx");
        h.names.push_back("gmx_mpi");
        h.examples.push_back("/usr/bin/gmx");
    }
    else if (code == "GAMESS-US")
    {
        h.names.push_back("gamess.00.x");
        h.examples.push_back("/opt/gamess/gamess.00.x");
    }
    else if (code == "Polyrate")
    {
        h.names.push_back("polyrate");
        h.examples.push_back("/opt/polyrate/polyrate");
    }
    else
    {
        string l = code;
        for (size_t i = 0; i < l.size(); i++)
            l[i] = (char)tolower((unsigned char)l[i]);
        h.names.push_back(l);
        h.examples.push_back("/opt/" + l + "/bin/" + l);
    }
    return h;
}

string exampleText(const ProgramHelp& h)
{
    string s;
    for (size_t i = 0; i < h.examples.size(); i++)
        s += (i ? "  or  " : "") + h.examples[i];
    return s;
}

string findProgramScript(const ProgramHelp& h)
{
    string s;
    for (size_t i = 0; i < h.names.size(); i++)
    {
        const string& n = h.names[i];
        if (h.companion.empty())
            s += "p=$(command -v " + n + " 2>/dev/null); case $p in /*) "
                 "echo \"ECCE-FOUND:$p\";; esac\n";
        else
            //  A program of the same name earlier on the PATH (ORCA's is
            //  also the GNOME screen reader) must not hide the real one, so
            //  every PATH entry is looked at, and the companion file must
            //  sit next to the program, or next to what a link points to.
            s += "oIFS=$IFS; IFS=:; for d in $PATH; do IFS=$oIFS; "
                 "case $d in /*) p=$d/" + n + "; if [ -x \"$p\" ]; then "
                 "r=$(readlink -f \"$p\" 2>/dev/null); [ -n \"$r\" ] || r=$p; "
                 "if [ -x \"${r%/*}/" + h.companion + "\" ]; then "
                 "echo \"ECCE-FOUND:$r\"; fi; fi;; esac; done; IFS=$oIFS\n";
    }
    return s;
}

vector<string> parseFound(const string& output)
{
    vector<string> out;
    vector<string> ls = lines(output);
    for (size_t i = 0; i < ls.size(); i++)
    {
        string l = trim(ls[i]);
        if (l.compare(0, 12, "ECCE-FOUND:/") != 0)
            continue;
        l = l.substr(11);
        if (std::find(out.begin(), out.end(), l) == out.end())
            out.push_back(l);
    }
    return out;
}

//  ---- connection ---------------------------------------------------------

Remote::Remote(const Connection& c) : p_c(c), p_rc(NULL)
{
}

Remote::~Remote()
{
    delete p_rc;
}

bool Remote::open(string& err)
{
    if (p_rc == NULL)
        p_rc = new RCommand(p_c.machine, p_c.remShell, p_c.locShell,
                            p_c.userName, p_c.password, p_c.frontendMachine,
                            p_c.frontendBypass, p_c.shellPath, p_c.libPath,
                            p_c.sourceFile);
    if (!p_rc->isOpen())
    {
        p_err = err = p_rc->commError();
        return false;
    }
    return true;
}

bool Remote::run(const vector<string>& argv, string& output, int timeoutSec)
{
    string e;
    if (!open(e))
    {
        output = e;
        return false;
    }
    bool ok = p_rc->execout(commandLine(argv), output, "", timeoutSec);
    if (!ok)
        p_err = p_rc->commError();
    return ok;
}

bool Remote::runWithInput(const vector<string>& argv, const string& file,
                          string& output, int timeoutSec)
{
    string e;
    if (!open(e))
    {
        output = e;
        return false;
    }
    bool ok = p_rc->execout(commandLine(argv) + " < " + quote(file), output, "",
                            timeoutSec);
    if (!ok)
        p_err = p_rc->commError();
    return ok;
}

bool Remote::put(const string& local, const string& remoteDir, string& err)
{
    if (!open(err))
        return false;
    const char* files[2] = { local.c_str(), NULL };
    if (!p_rc->shellput(files, remoteDir))
    {
        p_err = err = p_rc->commError();
        return false;
    }
    return true;
}

string Remote::home()
{
    string out;
    vector<string> argv;
    argv.push_back("sh"); argv.push_back("-c");
    argv.push_back("printf %s \"$HOME\"");
    if (!run(argv, out, 30))
        return "";
    return trim(out);
}

string Remote::error() const
{
    return p_err;
}

//  ---- discovery ----------------------------------------------------------

unsigned slurmMinutes(const string& text)
{
    string t = trim(text);
    if (t.empty() || !isdigit((unsigned char)t[0]))
        return 0;                       // infinite, UNLIMITED, n/a
    unsigned long days = 0;
    size_t dash = t.find('-');
    if (dash != string::npos)
    {
        days = strtoul(t.c_str(), 0, 10);
        t = t.substr(dash + 1);
    }
    vector<string> p = split(t, ':');
    unsigned long sec = 0;
    if (dash != string::npos)
    {
        //  D-H, D-H:M, D-H:M:S
        unsigned long h = p.size() > 0 ? strtoul(p[0].c_str(), 0, 10) : 0;
        unsigned long m = p.size() > 1 ? strtoul(p[1].c_str(), 0, 10) : 0;
        unsigned long s = p.size() > 2 ? strtoul(p[2].c_str(), 0, 10) : 0;
        sec = days * 86400 + h * 3600 + m * 60 + s;
    }
    else if (p.size() == 1)
        sec = strtoul(p[0].c_str(), 0, 10) * 60;           // minutes
    else if (p.size() == 2)
        sec = strtoul(p[0].c_str(), 0, 10) * 60 + strtoul(p[1].c_str(), 0, 10);
    else
        sec = strtoul(p[0].c_str(), 0, 10) * 3600 +
              strtoul(p[1].c_str(), 0, 10) * 60 + strtoul(p[2].c_str(), 0, 10);
    return (unsigned)((sec + 59) / 60);
}

//  The limits as the dialog lists them, in the units of the form: processors,
//  hours, GB.  `noun` names the processor count ("processors", "slots").
static string describe(const Queue& q, const char* noun, const char* memNote)
{
    std::ostringstream d;
    d << q.maxProcs << " " << noun << ", ";
    if (q.maxWallMin)
        d << (q.maxWallMin / 60.0) << " h";
    else
        d << "no time limit";
    if (q.maxMemMB)
        d << ", " << (q.maxMemMB / 1000.0) << " GB" << memNote;
    else
        d << ", no memory limit";
    return d.str();
}

//  sinfo -h -o "%R|%l|%D|%c|%m": partition, time limit, nodes, CPUs per node,
//  memory per node in MB.  A partition with several node types has a row for
//  each: its processors add up, its memory is the largest node's.
vector<Queue> parseSinfo(const string& text)
{
    vector<Queue> out;
    map<string, size_t> at;
    vector<string> ls = lines(text);
    for (size_t i = 0; i < ls.size(); i++)
    {
        vector<string> f = split(trim(ls[i]), '|');
        if (f.size() < 5 || trim(f[0]).empty())
            continue;
        string name = trim(f[0]);
        if (name[name.size() - 1] == '*')
            name.erase(name.size() - 1);
        unsigned nodes = leadingNumber(f[2]), cpus = leadingNumber(f[3]),
                 mem = leadingNumber(f[4]);
        unsigned wall = slurmMinutes(f[1]);
        if (at.find(name) == at.end())
        {
            Queue q;
            q.name = name;
            q.maxWallMin = wall;
            at[name] = out.size();
            out.push_back(q);
        }
        Queue& q = out[at[name]];
        q.maxProcs += nodes * cpus;
        q.maxMemMB = std::max(q.maxMemMB, mem);
    }
    for (size_t i = 0; i < out.size(); i++)
        out[i].detail = describe(out[i], "processors", " per node");
    return out;
}

//  qstat -Qf: "Queue: name", then "    key = value" lines.
vector<Queue> parsePbsQueues(const string& text)
{
    vector<Queue> out;
    vector<string> ls = lines(text);
    bool execution = true;
    for (size_t i = 0; i < ls.size(); i++)
    {
        string l = trim(ls[i]);
        if (l.compare(0, 6, "Queue:") == 0)
        {
            Queue q;
            q.name = trim(l.substr(6));
            out.push_back(q);
            execution = true;
            continue;
        }
        size_t eq = l.find('=');
        if (out.empty() || eq == string::npos)
            continue;
        string key = lower(trim(l.substr(0, eq))), val = trim(l.substr(eq + 1));
        Queue& q = out.back();
        if (key == "queue_type")
            execution = lower(val) == "execution" || lower(val) == "e";
        else if (key == "resources_max.walltime")
            q.maxWallMin = hmsMinutes(val);
        else if (key == "resources_max.ncpus" || key == "resources_max.procs" ||
                 (key == "resources_max.nodect" && q.maxProcs == 0))
            q.maxProcs = leadingNumber(val);
        else if (key == "resources_max.mem")
            q.maxMemMB = toMB(val, 'b');
        if (!execution)
            q.detail = "route";
    }
    vector<Queue> keep;
    for (size_t i = 0; i < out.size(); i++)
    {
        if (out[i].detail == "route" || out[i].name.empty())
            continue;
        out[i].detail = describe(out[i], "processors", "");
        keep.push_back(out[i]);
    }
    return keep;
}

//  qconf -sq NAME: "slots  4", "h_rt  INFINITY|HH:MM:SS|seconds",
//  "h_vmem  INFINITY|4G".  slots may be "1,[node1=4]": the largest counts.
Queue parseSgeQueue(const string& name, const string& text)
{
    Queue q;
    q.name = name;
    vector<string> ls = lines(text);
    for (size_t i = 0; i < ls.size(); i++)
    {
        string l = trim(ls[i]);
        size_t sp = l.find_first_of(" \t");
        if (sp == string::npos)
            continue;
        string key = l.substr(0, sp), val = trim(l.substr(sp));
        if (key == "slots")
        {
            string digits;
            for (size_t k = 0; k <= val.size(); k++)
            {
                if (k < val.size() && isdigit((unsigned char)val[k]))
                    digits += val[k];
                else if (!digits.empty())
                {
                    q.maxProcs = std::max(q.maxProcs,
                                          (unsigned)strtoul(digits.c_str(), 0, 10));
                    digits.clear();
                }
            }
        }
        else if (key == "h_rt")
            q.maxWallMin = hmsMinutes(val);
        else if (key == "h_vmem")
            q.maxMemMB = toMB(val, 'b');
    }
    q.detail = describe(q, "slots", "");
    return q;
}

//  bqueues -l: blocks starting "QUEUE: name"; RUNLIMIT (minutes), MEMLIMIT
//  (number and unit) and PROCLIMIT (min, default, max) each put their value
//  on the next line.
vector<Queue> parseLsfQueues(const string& text)
{
    vector<Queue> out;
    vector<string> ls = lines(text);
    for (size_t i = 0; i < ls.size(); i++)
    {
        string l = trim(ls[i]);
        if (l.compare(0, 6, "QUEUE:") == 0)
        {
            Queue q;
            q.name = trim(l.substr(6));
            out.push_back(q);
            continue;
        }
        if (out.empty() || i + 1 >= ls.size())
            continue;
        Queue& q = out.back();
        string next = trim(ls[i + 1]);
        if (l == "RUNLIMIT")
        {
            double v = atof(next.c_str());
            if (next.find("hour") != string::npos)
                v *= 60;
            q.maxWallMin = (unsigned)(v + 0.5);
        }
        else if (l == "MEMLIMIT")
        {
            q.maxMemMB = toMB(next, 'k');
        }
        else if (l == "PROCLIMIT")
        {
            vector<string> p;
            std::istringstream in(next);
            string w;
            while (in >> w)
                if (isNumber(w))
                    p.push_back(w);
            if (!p.empty())
                q.maxProcs = (unsigned)strtoul(p.back().c_str(), 0, 10);
        }
    }
    for (size_t i = 0; i < out.size(); i++)
        out[i].detail = describe(out[i], "processors", "");
    return out;
}

//  condor_status -af Cpus Memory: one line per slot.  A pool is not divided
//  into queues, so it is offered as one, "pool".
vector<Queue> parseCondorSlots(const string& text)
{
    vector<Queue> out;
    Queue q;
    q.name = "pool";
    unsigned slots = 0;
    vector<string> ls = lines(text);
    for (size_t i = 0; i < ls.size(); i++)
    {
        std::istringstream in(ls[i]);
        string c, m;
        if (!(in >> c >> m) || !isNumber(c) || !isNumber(m))
            continue;
        slots++;
        q.maxProcs = std::max(q.maxProcs, (unsigned)strtoul(c.c_str(), 0, 10));
        q.maxMemMB = std::max(q.maxMemMB, (unsigned)strtoul(m.c_str(), 0, 10));
    }
    if (slots == 0)
        return out;
    std::ostringstream d;
    d << slots << " slots, up to " << q.maxProcs << " processors and "
      << (q.maxMemMB / 1000.0) << " GB in one";
    q.detail = d.str();
    out.push_back(q);
    return out;
}

bool canDiscover(const string& qmgr)
{
    string q = lower(qmgr);
    return q == "slurm" || q == "pbs" || q == "sge" || q == "lsf" ||
           q == "htcondor";
}

string discoveryCommand(const string& qmgr)
{
    string q = lower(qmgr);
    if (q == "slurm") return "sinfo -h -o %R|%l|%D|%c|%m";
    if (q == "pbs") return "qstat -Qf";
    if (q == "sge") return "qconf -sql, then qconf -sq <queue> for each";
    if (q == "lsf") return "bqueues -l";
    if (q == "htcondor") return "condor_status -af Cpus Memory";
    return "";
}

static bool validName(const string& n)
{
    if (n.empty())
        return false;
    for (size_t i = 0; i < n.size(); i++)
        if (!isalnum((unsigned char)n[i]) && n[i] != '_' && n[i] != '.' &&
            n[i] != '-')
            return false;
    return true;
}

bool discover(Remote& r, const string& qmgr, vector<Queue>& queues,
              string& commands, string& err)
{
    queues.clear();
    commands = "";
    err = "";
    string q = lower(qmgr), out;
    vector<string> argv;

    if (q == "slurm")
    {
        argv.push_back("sinfo"); argv.push_back("-h"); argv.push_back("-o");
        argv.push_back("%R|%l|%D|%c|%m");
        commands = shown(argv);
        if (!r.run(argv, out)) { err = out; return false; }
        queues = parseSinfo(out);
    }
    else if (q == "pbs")
    {
        argv.push_back("qstat"); argv.push_back("-Qf");
        commands = shown(argv);
        if (!r.run(argv, out)) { err = out; return false; }
        queues = parsePbsQueues(out);
    }
    else if (q == "lsf")
    {
        argv.push_back("bqueues"); argv.push_back("-l");
        commands = shown(argv);
        if (!r.run(argv, out)) { err = out; return false; }
        queues = parseLsfQueues(out);
    }
    else if (q == "htcondor")
    {
        argv.push_back("condor_status"); argv.push_back("-af");
        argv.push_back("Cpus"); argv.push_back("Memory");
        commands = shown(argv);
        if (!r.run(argv, out)) { err = out; return false; }
        queues = parseCondorSlots(out);
    }
    else if (q == "sge")
    {
        argv.push_back("qconf"); argv.push_back("-sql");
        commands = shown(argv);
        if (!r.run(argv, out)) { err = out; return false; }
        vector<string> names = lines(out);
        for (size_t i = 0; i < names.size(); i++)
        {
            string n = trim(names[i]);
            if (!validName(n))
                continue;
            vector<string> a;
            a.push_back("qconf"); a.push_back("-sq"); a.push_back(n);
            commands += "\n" + shown(a);
            string text;
            if (!r.run(a, text)) { err = text; return false; }
            queues.push_back(parseSgeQueue(n, text));
        }
    }
    else
    {
        err = "Queue discovery is not available for " + qmgr + ".";
        return false;
    }

    //  Names end up in <machine>.Q, which allows only these characters.
    vector<Queue> keep;
    for (size_t i = 0; i < queues.size(); i++)
        if (validName(queues[i].name))
            keep.push_back(queues[i]);
    queues = keep;
    if (queues.empty())
    {
        err = "The scheduler listed no queues.\n" + out;
        return false;
    }
    return true;
}

//  ---- test submission ------------------------------------------------------

TestPlan testPlan(const string& qmgr)
{
    string q = lower(qmgr);
    TestPlan p;
    p.known = true;
    if (q == "slurm") p.what = "sbatch --test-only";
    else if (q == "sge") p.what = "qsub -verify";
    else if (q == "htcondor") p.what = "condor_submit -dry-run";
    else if (q == "pbs") { p.what = "qsub -h, then qdel"; p.holdAndCancel = true; }
    else if (q == "lsf") { p.what = "bsub -H, then bkill"; p.holdAndCancel = true; }
    else if (q == "moab") { p.what = "msub -h, then mjobctl -c"; p.holdAndCancel = true; }
    else p.known = false;
    return p;
}

//  The digits of the job id in a submit command's answer: "12345.server",
//  "Job <12345> is submitted ...", "\n12345\n", "Moab.12345".
static string jobIdIn(const string& qmgr, const string& text)
{
    string q = lower(qmgr);
    if (q == "lsf")
    {
        size_t a = text.find('<'), b = text.find('>');
        if (a != string::npos && b != string::npos && b > a + 1)
            return text.substr(a + 1, b - a - 1);
        return "";
    }
    vector<string> ls = lines(text);
    for (size_t i = 0; i < ls.size(); i++)
    {
        string l = trim(ls[i]);
        if (l.empty())
            continue;
        size_t d = l.find_first_of("0123456789");
        if (d == string::npos)
            continue;
        if (q == "moab" && l.compare(0, 5, "Moab.") == 0)
            return l.substr(5);
        return l;       // qsub's own text: the whole id ("12345.server")
    }
    return "";
}

bool testSubmission(Remote& r, const string& qmgr, const string& script,
                    TestResult& res, string& err)
{
    TestPlan plan = testPlan(qmgr);
    if (!plan.known)
    {
        err = "There is no test for the queue manager " + qmgr + ".";
        return false;
    }
    if (!r.open(err))
        return false;

    string tmpl = TempStorage::systemTempDir() + "/ecce-testsub-XXXXXX";
    int fd = mkstemp(&tmpl[0]);
    if (fd < 0)
    {
        err = "Cannot make a temporary file.";
        return false;
    }
    string local = tmpl;
    string text = script;
    ssize_t w = write(fd, text.c_str(), text.size());
    close(fd);
    if (w != (ssize_t)text.size())
    {
        unlink(local.c_str());
        err = "Cannot write the test script.";
        return false;
    }

    string home = r.home();
    if (home.empty())
        home = ".";
    //  Our own name on the machine: ecce-testsub-<random> in the home directory.
    string base = local.substr(local.rfind('/') + 1);
    string remote = home + "/" + base;
    string e;
    bool ok = r.put(local, home, e);
    unlink(local.c_str());
    if (!ok)
    {
        err = "Cannot copy the test script to the machine: " + e;
        return false;
    }

    string q = lower(qmgr), out;
    vector<string> argv;
    res.ran = true;
    //  A failed command with nothing to show would leave the dialog blank.
    auto said = [&r](const string& text) {
        if (!trim(text).empty())
            return text;
        string e = r.error();
        return e.empty() ? string("(no output: the command timed out after "
                                  "60 s, or could not be started)") : e;
    };

    if (q == "slurm")
    {
        argv.push_back("sbatch"); argv.push_back("--test-only");
        argv.push_back(remote);
        res.commands = shown(argv);
        res.accepted = r.run(argv, out);
        res.answer = res.accepted ? out : said(out);
        //  "sbatch: Job 12346 to start at ..."
        size_t j = out.find("Job ");
        if (res.accepted && j != string::npos)
        {
            size_t e = out.find_first_not_of("0123456789", j + 4);
            res.jobId = out.substr(j + 4, e == string::npos ? e : e - j - 4);
        }
    }
    else if (q == "sge")
    {
        argv.push_back("qsub"); argv.push_back("-verify"); argv.push_back(remote);
        res.commands = shown(argv);
        res.accepted = r.run(argv, out);
        res.answer = res.accepted ? out : said(out);
    }
    else if (q == "htcondor")
    {
        //  The same extraction of the #CONDOR lines as the submit command in
        //  siteconfig/QueueManagers; the program text is fixed, the script
        //  name is a word of its own.
        argv.push_back("sh"); argv.push_back("-c");
        argv.push_back("sed -n \"s/^[#]CONDOR //p\" \"$1\" | "
                       "sed \"s|@SCRIPT@|$1|g\" > \"$1.sub\" && "
                       "condor_submit -dry-run /dev/null \"$1.sub\"");
        argv.push_back("sh"); argv.push_back(remote);
        res.commands = "condor_submit -dry-run /dev/null " + remote + ".sub";
        res.accepted = r.run(argv, out);
        res.answer = res.accepted ? out : said(out);
    }
    else
    {
        string idText;
        vector<string> cancel;
        if (q == "pbs")
        {
            argv.push_back("qsub"); argv.push_back("-h"); argv.push_back(remote);
            cancel.push_back("qdel");
        }
        else if (q == "moab")
        {
            argv.push_back("msub"); argv.push_back("-h"); argv.push_back(remote);
            cancel.push_back("mjobctl"); cancel.push_back("-c");
        }
        else
        {
            argv.push_back("bsub"); argv.push_back("-H");
            cancel.push_back("bkill");
        }
        res.commands = shown(argv) + (q == "lsf" ? " < " + remote : "");
        bool sub = q == "lsf" ? r.runWithInput(argv, remote, idText)
                              : r.run(argv, idText);
        res.answer = sub ? idText : said(idText);
        res.accepted = sub;
        string id = sub ? jobIdIn(qmgr, idText) : "";
        res.jobId = id;
        if (sub && id.empty())
        {
            //  The scheduler took it and we cannot name the job: say so rather
            //  than leave a held job behind unnoticed.
            res.answer += "\nECCE could not read a job id from this answer, so "
                          "the held job was not cancelled. Cancel it by hand.";
        }
        else if (sub)
        {
            cancel.push_back(id);
            string cout_;
            res.commands += "\n" + shown(cancel);
            res.cancelled = r.run(cancel, cout_);
            res.answer += cout_.empty() ? "" : "\n" + cout_;
            if (!res.cancelled)
                res.answer += "\nThe job was submitted on hold but could not "
                              "be cancelled; cancel it by hand: " + shown(cancel);
        }
    }

    vector<string> rm;
    rm.push_back("rm"); rm.push_back("-f"); rm.push_back(remote);
    if (q == "htcondor")
        rm.push_back(remote + ".sub");
    string ignore;
    r.run(rm, ignore, 30);
    return true;
}

}   // namespace SchedulerQuery
