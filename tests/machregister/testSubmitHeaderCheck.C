//  Driver for include/tdat/SubmitHeaderCheck.H (#829): a one-line Slurm
//  entry in a machine's CONFIG warns and names the dropped lines; a normal
//  CONFIG does not.  Header-only; run via tests/machregister/run_tests.py.
#include <cstdio>
#include <map>
#include <string>
#include "../../include/tdat/SubmitHeaderCheck.H"

static int failures = 0;

static void check(bool ok, const char* what, const std::string& got)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) { printf("      got: %s\n", got.c_str()); failures++; }
}

static bool has(const std::string& s, const char* t)
{ return s.find(t) != std::string::npos; }

int main()
{
    std::map<std::string, std::string> cfg;
    std::string w;

    cfg["slurm"] = "#SBATCH --mem-per-cpu=2500M";
    w = SubmitHeaderCheck::warning(cfg, "Slurm", false);
    check(has(w, "--partition") && has(w, "--time") && has(w, "--account"),
          "one-line Slurm names all three dropped lines", w);

    cfg["slurm"] = "#SBATCH --partition=a\n#SBATCH -t 1:00:00\n#SBATCH -A proj";
    w = SubmitHeaderCheck::warning(cfg, "Slurm", true);
    check(w.empty(), "a full Slurm header, short options, no warning", w);

    cfg["slurm"] = "#SBATCH --partition=a\n#SBATCH --time=1:00:00";
    w = SubmitHeaderCheck::warning(cfg, "slurm", true);
    check(has(w, "--account") && !has(w, "--partition") && !has(w, "--time") &&
          has(w, "allocation accounts"),
          "only the missing line is named", w);

    cfg.clear();
    cfg["shell"] = "bash";
    cfg["nwchem"] = "/opt/nwchem";
    w = SubmitHeaderCheck::warning(cfg, "Slurm", true);
    check(w.empty(), "a normal CONFIG gives no warning", w);

    cfg["pbs"] = "#PBS -l walltime=$wallTime";
    w = SubmitHeaderCheck::warning(cfg, "PBS", true);
    check(has(w, "$account"), "accounts used, PBS block without $account", w);
    w = SubmitHeaderCheck::warning(cfg, "PBS", false);
    check(w.empty(), "no accounts used, PBS block is not checked", w);

    w = SubmitHeaderCheck::warning(cfg, "Shell", true);
    check(w.empty(), "no queue manager, no warning", w);

    printf(failures ? "FAIL\n" : "PASS\n");
    return failures ? 1 : 0;
}
