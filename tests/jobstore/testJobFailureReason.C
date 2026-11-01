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

// The HFX_MEM_INFO block of a real CP2K 2025.1 PBE0 water run (MAX_MEMORY
// 200), with the two program-size lines and the ERI RAM line parameterized
// so the two-block case below can differ from the verbatim first block.
static string hfxBlock(const string& before, const string& after,
                       const string& ram)
{
  return string(" SCF WAVEFUNCTION OPTIMIZATION\n")
       + "\n"
       + "  Step     Update method      Time    Convergence         Total energy    Change\n"
       + "  ------------------------------------------------------------------------------\n"
       + "\n"
       + "  HFX_MEM_INFO| Est. max. program size before HFX [MiB]:                     " + before + "\n"
       + "  HFX_MEM_INFO| Number of cart. primitive ERI's calculated:            275133392\n"
       + "  HFX_MEM_INFO| Number of sph. ERI's calculated:                           89381\n"
       + "  HFX_MEM_INFO| Number of sph. ERI's stored in-core:                       89381\n"
       + "  HFX_MEM_INFO| Number of sph. ERI's stored on disk:                           0\n"
       + "  HFX_MEM_INFO| Number of sph. ERI's calculated on the fly:                    0\n"
       + "  HFX_MEM_INFO| Total memory consumption ERI's RAM [MiB]:                      " + ram + "\n"
       + "  HFX_MEM_INFO| Whereof max-vals [MiB]:                                        1\n"
       + "  HFX_MEM_INFO| Total compression factor ERI's RAM:                         3.23\n"
       + "  HFX_MEM_INFO| Total memory consumption ERI's disk [MiB]:                     0\n"
       + "  HFX_MEM_INFO| Total compression factor ERI's disk:                        0.00\n"
       + "  HFX_MEM_INFO| Size of density/Fock matrix [MiB]:                             0\n"
       + "  HFX_MEM_INFO| Size of buffers [MiB]:                                         0\n"
       + "  HFX_MEM_INFO| Number of periodic image cells considered:                    81\n"
       + "  HFX_MEM_INFO| Est. max. program size after HFX  [MiB]:                     " + after + "\n"
       + "\n"
       + "\n"
       + " MO| EIGENVALUES AND OCCUPATION NUMBERS AFTER SCF STEP 0\n";
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    cerr << "usage: " << argv[0] << " <fixtures-dir>" << endl;
    return 1;
  }
  string fx = argv[1];

  // ORCA, optimisation out of cycles before Freq: the boxed "ERROR !!!"
  // is named by the sentence under it.
  {
    vector<string> files;
    files.push_back(fx + "/orca-opt-maxiter/orca.orcaout");
    string reason = JobFailureReason::extractReason(files);
    expectContains("ORCA ERROR !!! box", reason,
                   "\"The optimization did not converge but reached the "
                   "maximum number of optimization cycles.\"");
    expectShortSingleLine("ORCA ERROR !!! reason is short", reason);
  }

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

  // Quantum ESPRESSO: SCF not converged, and a boxed error.
  {
    vector<string> files;
    files.push_back(fx + "/synthetic/qe-noconverge.pwout");
    string reason = JobFailureReason::extractReason(files);
    expectContains("qe no convergence", reason, "did not converge in 3");
    expectShortSingleLine("qe no convergence is short", reason);
    files.clear();
    files.push_back(fx + "/synthetic/qe-routine-error.pwout");
    reason = JobFailureReason::extractReason(files);
    expectContains("qe routine error", reason, "read_namelists");
    expectContains("qe routine error text", reason, "bad line in namelist");
  }

  // CP2K: an SCF that hit MAX_SCF (real 2025.1 output, steps from the line
  // before the box) and a boxed error naming the missing basis set.
  {
    vector<string> files;
    files.push_back(fx + "/synthetic/cp2k-noconverge.cp2kout");
    string reason = JobFailureReason::extractReason(files);
    expectContains("cp2k no convergence", reason, "did not converge in 8 steps");
    expectContains("cp2k no convergence advice", reason, "Max. SCF Iterations");
    expectShortSingleLine("cp2k no convergence is short", reason);
    files.clear();
    files.push_back(fx + "/synthetic/cp2k-no-basis.cp2kout");
    reason = JobFailureReason::extractReason(files);
    expectContains("cp2k box message", reason, "CP2K stopped");
    expectContains("cp2k box message text", reason,
                   "The requested basis set <NOSUCH-BASIS> for element <Si>");
    expectContains("cp2k box message continues", reason, "<BASIS_MOLOPT>");
    expectShortSingleLine("cp2k box reason is short", reason);
  }

  // GROMACS: the Fatal error paragraph of the tool that failed (real gmx
  // 2025.2 output); for grompp the ERROR paragraph above its own box.
  {
    vector<string> files;
    files.push_back(fx + "/synthetic/gromacs-pdb2gmx-missing-atoms.gmxlog");
    string reason = JobFailureReason::extractReason(files);
    expectContains("gromacs pdb2gmx missing atoms", reason, "gmx pdb2gmx: ");
    expectContains("gromacs pdb2gmx missing atoms text", reason,
                   "8 missing atoms in molecule Protein_chain_A");
    expectShortSingleLine("gromacs pdb2gmx reason is short", reason);
    files.clear();
    files.push_back(fx + "/synthetic/gromacs-pdb2gmx-bad-residue.gmxlog");
    reason = JobFailureReason::extractReason(files);
    expectContains("gromacs pdb2gmx residue type", reason, "do not have a consistent type");
    files.clear();
    files.push_back(fx + "/synthetic/gromacs-genion-no-solvent.gmxlog");
    reason = JobFailureReason::extractReason(files);
    expectContains("gromacs genion names the control", reason,
                   "Lower the Minimum ion distance");
    expectShortSingleLine("gromacs genion reason is short", reason);
    files.clear();
    files.push_back(fx + "/synthetic/gromacs-grompp-error.gmxlog");
    reason = JobFailureReason::extractReason(files);
    expectContains("gromacs grompp error names the cause", reason,
                   "cut-off length is longer");
    expectShortSingleLine("gromacs grompp error is short", reason);
    files.clear();
    files.push_back(fx + "/synthetic/gromacs-grompp-mismatch.gmxlog");
    reason = JobFailureReason::extractReason(files);
    expectContains("gromacs grompp mismatch", reason, "does not match topology");
  }

  // No files, or a file with nothing recognizable: no match, not a
  // guess.
  {
    vector<string> files;
    files.push_back(fx + "/does-not-exist.out");
    string reason = JobFailureReason::extractReason(files);
    expectEmpty("no files present", reason);
  }

  // The scheduler's stderr file: named with its last non-empty line, and
  // no note for an empty or missing file.
  {
    string path = "/tmp/ecce-schederr-unit.err";
    FILE* f = fopen(path.c_str(), "w");
    fputs("slurmstepd: error: oom-kill event\n  \n", f);
    fclose(f);
    expectContains("scheduler note names file",
                   JobFailureReason::schedulerErrNote("slurm.err", path),
                   "see slurm.err in Outputs: slurmstepd: error: oom-kill");
    f = fopen(path.c_str(), "w");
    fputs("slurmstepd: error: Detected 10 oom_kill events in StepId=1.batch\n"
          "ORCA finished by error termination in SCF\n", f);
    fclose(f);
    expectContains("scheduler-ended job: the slurmstepd line, not the last",
                   JobFailureReason::schedulerErrNote("slurm.err", path, true),
                   "Outputs: slurmstepd: error: Detected 10 oom_kill");
    expectContains("code failure: the last line",
                   JobFailureReason::schedulerErrNote("slurm.err", path),
                   "Outputs: ORCA finished by error");
    f = fopen(path.c_str(), "w");
    fputs("\n \n", f);
    fclose(f);
    expectEmpty("scheduler note, blank file",
                JobFailureReason::schedulerErrNote("slurm.err", path));
    remove(path.c_str());
    expectEmpty("scheduler note, missing file",
                JobFailureReason::schedulerErrNote("slurm.err", path));
  }

  // CP2K HFX memory estimate for a scheduler OOM kill (#739): the LAST
  // HFX_MEM_INFO block's two program-size lines and its ERI RAM line
  // become one short sentence; "" when there is no such block.
  {
    const string one = hfxBlock("291", "294", "1");
    expectContains("hfx single block",
                   JobFailureReason::hfxMemoryNote(one),
                   "CP2K's HFX estimate: program size 291 MiB before HFX, "
                   "294 MiB after; ERIs in RAM 1 MiB");
    const string two = one + hfxBlock("300", "305", "7");
    const string note = JobFailureReason::hfxMemoryNote(two);
    expectContains("hfx last block wins", note,
                   "program size 300 MiB before HFX, 305 MiB after; "
                   "ERIs in RAM 7 MiB");
    expectContains("hfx advice", note, "set the memory per rank");
    expectShortSingleLine("hfx note is short", note);
    expectEmpty("hfx no lines", JobFailureReason::hfxMemoryNote(
                   "a CP2K output with no HFX_MEM_INFO\n"));
    expectEmpty("hfx empty text", JobFailureReason::hfxMemoryNote(""));
  }

  if (failures) {
    cerr << failures << " failure(s)" << endl;
    return 1;
  }
  cout << "All tests passed." << endl;
  return 0;
}
