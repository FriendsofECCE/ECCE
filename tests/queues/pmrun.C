// ProcessMachine::run, the path Save and Delete in Machine Registration
// take, against the real processmachine in a temporary ECCE_HOME:
//
//    pmrun <path to scripts/processmachine>
//
// Hostile names and values must reach the files unchanged and run nothing;
// a missing script and one that exits without reading stdin must give an
// error status, not a crash or a SIGPIPE death.
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "util/ProcessMachine.H"

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  std::cout << (ok ? "ok    " : "FAIL  ") << what << std::endl;
  if (!ok)
    failures++;
}

static std::string slurp(const std::string& path)
{
  std::ifstream in(path);
  if (!in)
    return "<missing>";
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}

static void spit(const std::string& path, const std::string& text)
{
  std::ofstream(path) << text;
}

static bool exists(const std::string& path)
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0;
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "usage: pmrun <scripts/processmachine>" << std::endl;
    return 2;
  }
  char real[PATH_MAX];
  if (!realpath(argv[1], real)) {
    std::cerr << "no such script: " << argv[1] << std::endl;
    return 2;
  }

  const char* tmpdir = getenv("TMPDIR");
  std::string tmpl = std::string(tmpdir ? tmpdir : "/tmp") + "/ecce-pmrun-XXXXXX";
  if (!mkdtemp(&tmpl[0]))
    return 2;
  std::string tmp = tmpl;
  std::string home = tmp + "/home", user = tmp + "/user";
  std::string ue = user + "/.ECCE", script = home + "/scripts/processmachine";
  mkdir(home.c_str(), 0755);
  mkdir((home + "/scripts").c_str(), 0755);
  mkdir((home + "/siteconfig").c_str(), 0755);
  mkdir(user.c_str(), 0755);
  mkdir(ue.c_str(), 0755);
  symlink(real, script.c_str());
  setenv("ECCE_HOME", home.c_str(), 1);
  setenv("ECCE_REALUSERHOME", user.c_str(), 1);
  // a relative "touch executed" in a name lands here
  if (chdir(tmp.c_str()) != 0)
    return 2;

  typedef ProcessMachine PM;
  const std::string marker = tmp + "/executed";
  const std::string odd = "odd\"$(touch executed)\"`touch executed`";
  const std::string path = "/opt/nw chem/+ & = % \" $ \\ ' `x` $(id)/nwchem";
  const std::string qmgr = "/opt/q+m&g=r%41/bin";
  const std::string vendor = "A&B=C+D%20";

  // 1. Save, as collectSettings() builds it
  std::string form = "type=accept";
  form += PM::field("siteconfig", "false");
  form += PM::field("machine", "odd.example.org");
  form += PM::field("name", odd);
  form += PM::field("vendor", vendor);
  form += PM::field("model", "Unspecified");
  form += PM::field("processor", "Unspecified");
  form += PM::field("procs", "1") + PM::field("nodes", "1");
  form += PM::field("ssh", "true");
  form += PM::field("registeredcodes", "NWChem");
  form += PM::field("NWChem", path);
  form += PM::field("perlPath", "") + PM::field("qmgrPath", qmgr);
  form += PM::field("AA", "false") + PM::field("qmgr", "Slurm");
  form += PM::field("numQueues", "1");
  form += PM::field("q0", "name|q.a-1_b,minNodes|1,maxNodes|4,maxCPU|0,"
                          "maxMemory|0,minScratch|0,");
  check(PM::run(form) == 0, "save returns 0");
  std::string cfg = slurp(ue + "/CONFIG." + odd);
  check(cfg.find("NWChem: " + path + "\n") != std::string::npos,
        "the code path reaches CONFIG unchanged");
  check(cfg.find("qmgrPath: " + qmgr + "\n") != std::string::npos,
        "qmgrPath reaches CONFIG unchanged");
  std::string mm = slurp(ue + "/MyMachines");
  check(mm.find(odd + "\todd.example.org\t" + vendor + "\t") == 0,
        "name and vendor reach MyMachines unchanged");
  check(slurp(ue + "/" + odd + ".Q").find("q.a-1_b|maxProcessors:") !=
        std::string::npos, "the .Q file is written");

  // 2. Delete, as removeMachine() posts it; a second, hand-written
  //    entry has a '/' in its name
  const std::string slashed = "q\"$(touch " + marker + ")";
  spit(ue + "/MyMachines", mm + slashed + "\tx\tU\tU\tU\t1\tssh\t:\tMN\n");
  check(PM::run("type=delete" + PM::field("siteconfig", "false") +
                PM::field("name", odd)) == 0, "delete returns 0");
  check(PM::run("type=delete" + PM::field("siteconfig", "false") +
                PM::field("name", slashed)) == 0,
        "delete of a name with '/' returns 0");
  check(!exists(marker), "no command in a name or value ran");
  check(!exists(ue + "/CONFIG." + odd) && !exists(ue + "/" + odd + ".Q"),
        "CONFIG and .Q of the deleted machine are removed");
  check(slurp(ue + "/MyMachines").find_first_not_of("\n") == std::string::npos,
        "both lines leave MyMachines");

  // 3. a script that exits without reading a form larger than a pipe
  unlink(script.c_str());
  spit(script, "#!/bin/sh\nexit 3\n");
  chmod(script.c_str(), 0755);
  std::string big = "type=accept" + PM::field("pad", std::string(1 << 20, '&'));
  check(PM::run(big) == 3, "an early exit gives its status, no SIGPIPE death");
  struct sigaction now;
  sigaction(SIGPIPE, nullptr, &now);
  check(now.sa_handler == SIG_DFL, "SIGPIPE handling is restored");

  // 4. no script at all
  unlink(script.c_str());
  check(PM::run(form) == -1, "a missing script gives -1");

  if (failures == 0)
    std::filesystem::remove_all(tmp);
  else
    std::cout << "kept " << tmp << std::endl;
  std::cout << (failures ? "FAILED" : "PASSED") << std::endl;
  return failures ? 1 : 0;
}
