#include <fstream>
#include <sstream>
using std::ifstream;
using std::ofstream;
using std::istringstream;

#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/TempStorage.H"
#include "comm/RCommand.H"

#include "InputVerifier.H"


bool InputVerifier::run(const string& text,
                        const string& codeName,
                        int atoms,
                        vector<VerifyFinding>& findings,
                        string& error)
{
  findings.clear();
  error = "";

  string checker(Ecce::ecceDataControllersPath());
  checker += "/verifyinput";

  RCommand localconn;
  if (!localconn.exists(checker)) {
    //  Say where it looked.  The one recurring packaging fault in this
    //  tree is a script that exists in the repository and is absent
    //  from the installed .deb, and "not found" without a path sends
    //  the reader looking in the wrong place for it.
    error = "The input checker was not found at " + checker + ".";
    return false;
  }

  //  The deck is checked as TEXT, not read from the calculation's
  //  stored input file, so that the Final Edit dialog can check what
  //  the user has in front of them rather than what was last saved.
  SFile* deck = TempStorage::getTempFile("ECCEverify");
  if (deck == 0) {
    error = "A temporary file for the checker could not be created.";
    return false;
  }
  {
    ofstream out(deck->path().c_str());
    if (!out) {
      error = "A temporary file for the checker could not be written.";
      delete deck;
      return false;
    }
    out << text;
  }

  string command = checker + " -f " + deck->path();
  if (!codeName.empty())
    command += " -c \"" + codeName + "\"";
  if (atoms > 0) {
    std::ostringstream n;
    n << atoms;
    command += " -a " + n.str();
  }

  string output;
  //  execout() returns false on a non-zero exit status, and the
  //  checker EXITS NON-ZERO WHENEVER IT FOUND SOMETHING BAD.  That is
  //  its normal, successful behaviour, so the return value cannot be
  //  used to decide whether it ran.  Parse the output instead: findings
  //  mean it ran, and nothing at all means it did not.
  localconn.execout(command, output);

  deck->remove();
  delete deck;

  istringstream lines(output);
  string line;
  while (getline(lines, line)) {
    if (line.empty()) continue;

    const string::size_type a = line.find('|');
    if (a == string::npos) continue;
    const string::size_type b = line.find('|', a + 1);
    if (b == string::npos) continue;
    const string::size_type c = line.find('|', b + 1);
    if (c == string::npos) continue;

    VerifyFinding finding;
    const string level = line.substr(0, a);
    if      (level == "BAD")    finding.level = VerifyFinding::BAD;
    else if (level == "UNSURE") finding.level = VerifyFinding::UNSURE;
    else if (level == "GOOD")   finding.level = VerifyFinding::GOOD;
    else continue;

    finding.line    = atoi(line.substr(a + 1, b - a - 1).c_str());
    finding.check   = line.substr(b + 1, c - b - 1);
    finding.message = line.substr(c + 1);
    findings.push_back(finding);
  }

  if (findings.empty()) {
    error = "The input checker produced nothing.";
    if (!output.empty())
      error += "\n\n" + output;
    return false;
  }

  return true;
}


VerifyFinding::Level
InputVerifier::worst(const vector<VerifyFinding>& findings)
{
  VerifyFinding::Level worst = VerifyFinding::GOOD;
  for (size_t i = 0; i < findings.size(); i++)
    if (findings[i].level > worst)
      worst = findings[i].level;
  return worst;
}
