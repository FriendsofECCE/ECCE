#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "util/Ecce.H"
#include "util/MdAnalysisRun.H"

using std::string;

namespace {

string quote(const string& s)
{
#ifdef _WIN32
  return Ecce::winArg(s);
#else
  string o = "'";
  for (char c : s) o += c == '\'' ? string("'\\''") : string(1, c);
  return o + "'";
#endif
}

int runArgs(const std::vector<string>& argv)
{
  string cmd, out;
  for (size_t i = 0; i < argv.size(); i++) cmd += (i ? " " : "") + quote(argv[i]);
#ifdef _WIN32
  return Ecce::readCommand(cmd, out, false);
#else
  return Ecce::readCommand(cmd + " 2>&1", out);
#endif
}

string slurp(const string& path)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  std::ostringstream os;
  os << in.rdbuf();
  return os.str();
}

}

int MdAnalysisRun::mdpSteps(const string& mdpText, const string& want)
{
  std::istringstream in(mdpText);
  string line;
  int found = 0;
  while (std::getline(in, line)) {
    line = line.substr(0, line.find(';'));
    const size_t eq = line.find('=');
    if (eq == string::npos) continue;
    std::istringstream k(line.substr(0, eq)), v(line.substr(eq + 1));
    string key;
    k >> key;
    for (char& c : key) if (c == '_') c = '-';
    int n = 0;
    if (key == want && (v >> n)) found = n;
  }
  return found;
}

string MdAnalysisRun::program()
{
  const char* home = getenv("ECCE_HOME");
  return (home && *home ? string(home) + "/bin/" : string()) + "mdanalysis"
#ifdef _WIN32
    ".exe"
#endif
    ;
}

string MdAnalysisRun::run(const string& mda, const string& top, const string& mdp,
                          const string& xtc, const string& trr, const string& work)
{
  const string msdFile = work + "/ecce_md_msd.txt", speedFile = work + "/ecce_md_speed.txt";
  string msd, speed;
  if (runArgs({ mda, "--top", top, "--mdp", mdp, "--traj", xtc, "--out", msdFile }) == 0)
    msd = slurp(msdFile);
  if (!trr.empty() && mdpSteps(slurp(mdp), "nstvout") > 0) {
    if (runArgs({ mda, "--top", top, "--mdp", mdp, "--vel", trr, "--out", speedFile }) == 0)
      speed = slurp(speedFile);
  } else if (!msd.empty()) {
    //  The Speed panel says why it has no curve.
    speed = "ECCE-MDSPEED none\n";
  }
  std::remove(msdFile.c_str());
  std::remove(speedFile.c_str());
  return msd + speed;
}
