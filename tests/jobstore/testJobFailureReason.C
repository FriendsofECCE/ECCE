// Unit test for JobFailureReason::extractReason (#-- Andy, 2026-09-27:
// "the Organizer says 'No ORCA TERMINATED NORMALLY'... it doesn't tell
// us WHY. I want this to show up in the launcher window. i.e. too many
// cores.").
//
// Pure header, no ECCE/X11 deps -- compiled with plain g++ against
// include/ only.  Real fixtures (copied, never modified) from
// /home/andy/jobs/benzene-1-1, /home/andy/jobs/methane_orca-2 and
// /home/andy/jobs/N2-1; the Slurm/NWChem cases have no real fixture on
// hand so they're small synthetic files in fixtures/synthetic/.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

#include "comm/JobFailureReason.H"

static int failures = 0;

static void expectContains(const string& label, const string& reason,
                            const string& mustContain)
{
  if (reason.find(mustContain) == string::npos) {
    cerr << "FAIL " << label << ": expected reason to contain \""
         << mustContain << "\", got: \"" << reason << "\"" << endl;
    failures++;
  } else {
    cout << "PASS " << label << endl;
  }
}

static void expectEmpty(const string& label, const string& reason)
{
  if (!reason.empty()) {
    cerr << "FAIL " << label << ": expected no match, got: \""
         << reason << "\"" << endl;
    failures++;
  } else {
    cout << "PASS " << label << endl;
  }
}

// Andy, 2026-09-27: "organizer bottom window is very verbose with
// failures" -- every reason must be one short, single-line message.
static void expectShortSingleLine(const string& label, const string& reason)
{
  bool ok = !reason.empty() && reason.size() <= 145 &&
            reason.find('\n') == string::npos;
  if (!ok) {
    cerr << "FAIL " << label << ": expected a short single line (<=145 "
         << "chars, no newline), got (" << reason.size() << " chars): \""
         << reason << "\"" << endl;
    failures++;
  } else {
    cout << "PASS " << label << endl;
  }
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    cerr << "usage: " << argv[0] << " <fixtures-dir>" << endl;
    return 1;
  }
  string fx = argv[1];

  // ORCA, real failure: Open MPI refused to bind 8 processes to a
  // 4-core node (benzene-1-1, ran BEFORE gensub's marker-check landed --
  // no "setting status"/"ORCA TERMINATED NORMALLY" text at all, so this
  // exercises the raw-output scan on its own).
  {
    vector<string> files;
    files.push_back(fx + "/benzene-1-1/orca.orcaout");
    string reason = JobFailureReason::extractReason(files);
    expectContains("benzene-1-1 (Open MPI bind)", reason, "8 MPI processes");
    expectShortSingleLine("benzene-1-1 reason is short", reason);
  }

  // ORCA, real failure, SAME underlying cause, but this fixture DOES
  // carry gensub's post-fix marker-check line in the ECCE Log block --
  // must still resolve to the Open MPI reason, not the generic
  // "setting status to failed" line.
  {
    vector<string> files;
    files.push_back(fx + "/methane_orca-2/orca.orcaout");
    string reason = JobFailureReason::extractReason(files);
    expectContains("methane_orca-2 (Open MPI bind)", reason,
                    "8 MPI processes");
    expectShortSingleLine("methane_orca-2 reason is short", reason);
  }

  // Gaussian, real failure: bad route card (Symmetry=(PG=D*H,...))
  // caught by Gaussian's own parser, "QPErr" then Lnk1e termination.
  {
    vector<string> files;
    files.push_back(fx + "/N2-1/g16.g16out");
    string reason = JobFailureReason::extractReason(files);
    expectContains("N2-1 (Gaussian QPErr)", reason, "QPErr");
    expectShortSingleLine("N2-1 reason is short", reason);
  }

  // Synthetic: Slurm time-limit cancellation, read from a stderr file
  // alongside an (absent) primary output.
  {
    vector<string> files;
    files.push_back(fx + "/does-not-exist.out");
    files.push_back(fx + "/synthetic/slurm-timelimit.err");
    string reason = JobFailureReason::extractReason(files);
    expectContains("slurm time limit", reason, "time limit");
    expectShortSingleLine("slurm time limit reason is short", reason);
  }

  // Synthetic: Slurm memory-limit kill.
  {
    vector<string> files;
    files.push_back(fx + "/synthetic/slurm-memlimit.err");
    string reason = JobFailureReason::extractReason(files);
    expectContains("slurm mem limit", reason, "memory");
  }

  // Synthetic: NWChem input error block.
  {
    vector<string> files;
    files.push_back(fx + "/synthetic/nwchem-input-error.nwout");
    string reason = JobFailureReason::extractReason(files);
    expectContains("nwchem input error", reason,
                    "There is an error in the input file");
    expectContains("nwchem input error names directive", reason,
                    "bassis");
    expectShortSingleLine("nwchem input error reason is short", reason);
  }

  // No files, or a file with nothing recognizable: no match, not a
  // guess.
  {
    vector<string> files;
    files.push_back(fx + "/does-not-exist.out");
    string reason = JobFailureReason::extractReason(files);
    expectEmpty("no files present", reason);
  }

  if (failures) {
    cerr << failures << " failure(s)" << endl;
    return 1;
  }
  cout << "All tests passed." << endl;
  return 0;
}
