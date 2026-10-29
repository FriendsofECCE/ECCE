#include "comm/CodePrograms.H"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <sys/stat.h>

using std::string;
using std::vector;

vector<string> CodePrograms::names(const string& code)
{
    vector<string> n;
    if (code == "NWChem" || code == "NWChemMD")
        n.push_back("nwchem");
    else if (code == "ORCA")
        n.push_back("orca");
    else if (code.compare(0, 9, "Gaussian-") == 0 && code.size() == 11)
        n.push_back("g" + code.substr(9));
    else if (code == "QuantumESPRESSO")
        n.push_back("pw.x");
    else if (code == "CP2K")
    {
        // psmp is the MPI+OpenMP build, popt plain MPI, ssmp threads only.
        n.push_back("cp2k.psmp");
        n.push_back("cp2k.popt");
        n.push_back("cp2k.ssmp");
        n.push_back("cp2k");
    }
    else if (code == "xTB")
        n.push_back("xtb");
    else if (code == "MOPAC")
    {
        n.push_back("MOPAC2016.exe");
        n.push_back("mopac");
    }
    else if (code == "GROMACS")
    {
        n.push_back("gmx");
        n.push_back("gmx_mpi");
    }
    else if (code == "GAMESS-US")
        n.push_back("gamess.00.x");
    else if (code == "Polyrate")
        n.push_back("polyrate");
    else
    {
        string l = code;
        for (size_t i = 0; i < l.size(); i++)
            l[i] = (char)tolower((unsigned char)l[i]);
        n.push_back(l);
    }
    return n;
}

string CodePrograms::companion(const string& code)
{
    return code == "ORCA" ? "orca_2mkl" : "";
}

static bool isExecutable(const string& p)
{
    struct stat st;
    return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
           access(p.c_str(), X_OK) == 0;
}

// The PATH to search.  A macOS app started from the Dock or Finder gets
// only /usr/bin:/bin:/usr/sbin:/sbin, not the user's Terminal PATH, so the
// login shell is asked once and the usual Homebrew/MacPorts folders added.
static string searchPath()
{
    const char* env = getenv("PATH");
    string path = env ? env : "";
#ifdef __APPLE__
    static string extra;
    static bool asked = false;
    if (!asked) {
        asked = true;
        const char* sh = getenv("SHELL");
        // Interactive too: PATH set in .zshrc (conda, GMXRC) is not read by
        // a login shell alone.  Markers keep out whatever the rc files print.
        string cmd = string(sh && *sh ? sh : "/bin/zsh") +
                     " -l -i -c 'printf \"@ECCEPATH@%s@ECCEPATH@\" \"$PATH\"'"
                     " </dev/null 2>/dev/null";
        string out;
        if (FILE* f = popen(cmd.c_str(), "r")) {
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof buf, f)) > 0)
                out.append(buf, n);
            pclose(f);
        }
        const string mark = "@ECCEPATH@";
        size_t a = out.find(mark);
        size_t b = a == string::npos ? a : out.find(mark, a + mark.size());
        if (b != string::npos)
            extra = out.substr(a + mark.size(), b - a - mark.size());
        extra += ":/opt/homebrew/bin:/usr/local/bin:/opt/local/bin";
    }
    path += ":" + extra;
#endif
    return path;
}

string CodePrograms::findOnPath(const string& code)
{
    string path = searchPath();
    if (path.empty())
        return "";
    vector<string> dirs;
    size_t a = 0;
    while (a <= path.size())
    {
        size_t b = path.find(':', a);
        if (b == string::npos)
            b = path.size();
        // Relative entries (and the empty one) depend on the working directory.
        if (b > a && path[a] == '/')
            dirs.push_back(path.substr(a, b - a));
        a = b + 1;
    }
    string comp = companion(code);
    vector<string> ns = names(code);
    for (size_t i = 0; i < ns.size(); i++)
        for (size_t j = 0; j < dirs.size(); j++)
        {
            string p = dirs[j] + "/" + ns[i];
            if (!isExecutable(p))
                continue;
            if (comp.empty())
                return p;
            // A program of the same name earlier on the PATH (ORCA's is also
            // the GNOME screen reader) must not hide the real one.
            char real[PATH_MAX];
            string r = realpath(p.c_str(), real) ? string(real) : p;
            string dir = r.substr(0, r.rfind('/'));
            if (isExecutable(dir + "/" + comp))
                return r;
        }
    return "";
}
