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

    //  #871: the account a generated script charges, from its directives.
    using SubmitHeaderCheck::submitAccount;
    w = submitAccount("#!/bin/sh\n#SBATCH --partition=normal\n#SBATCH --account=proj7\n");
    check(w == "proj7", "Slurm --account=", w);
    w = submitAccount("#SBATCH -J x --account proj8 -t 1:00\n");
    check(w == "proj8", "Slurm --account VALUE among other options", w);
    w = submitAccount("#SBATCH -A naiss2026-1\n");
    check(w == "naiss2026-1", "Slurm -A", w);
    w = submitAccount("#PBS -A pbsproj\n");
    check(w == "pbsproj", "PBS -A", w);
    w = submitAccount("#BSUB -P lsfproj\n");
    check(w == "lsfproj", "LSF -P", w);
    w = submitAccount("#MSUB -A moabproj\n");
    check(w == "moabproj", "Moab -A", w);
    w = submitAccount("#SBATCH --partition=normal\n#SBATCH --time=1:00:00\n");
    check(w.empty(), "no account directive gives none", w);
    w = submitAccount("#SBATCH --account=##account_no##\n");
    check(w.empty(), "an unsubstituted token is not an account", w);
    w = submitAccount("echo --account=notadirective\n");
    check(w.empty(), "only directive lines count", w);

    printf(failures ? "FAIL\n" : "PASS\n");
    return failures ? 1 : 0;
}
