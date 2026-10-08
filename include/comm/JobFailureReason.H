///////////////////////////////////////////////////////////////////////////
// JobFailureReason.H
//
// Pure, dependency-free extraction of a specific "why did this job fail"
// reason from a code's primary output file (and, opportunistically, any
// stderr file already fetched alongside it -- e.g. Slurm's "slurm.err").
//
// This exists so JobStore.C's cleanup() can show something more useful
// than gensub's own "setting status to failed" line (which names the
// symptom gensub detected -- missing marker, missing file, unexpected
// core -- not the underlying cause).  Kept header-only, with no ECCE/X11
// dependencies, so it can be compiled and unit-tested with plain g++
// (see tests/jobstore).
///////////////////////////////////////////////////////////////////////////
#ifndef JOBFAILUREREASON_H
#define JOBFAILUREREASON_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace JobFailureReason {

// ------------------------------------------------------------------------
// Read a whole file into a vector of lines.  Returns an empty vector if
// the file can't be opened -- callers just get no match from it, same as
// any other file with no recognizable content.
// ------------------------------------------------------------------------
inline std::vector<std::string> readLines(const std::string& path)
{
  std::vector<std::string> lines;
  std::ifstream ifs(path.c_str());
  if (!ifs)
    return lines;
  std::string line;
  while (std::getline(ifs, line))
    lines.push_back(line);
  return lines;
}

// Trim trailing '\r' (files fetched off a remote box are sometimes
// CRLF) and surrounding whitespace, for quoting a line cleanly.
inline std::string trimmed(const std::string& s)
{
  size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return "";
  size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

// Join lines [from, from+count) with spaces, for regexing across a
// short window when a real message wraps onto more than one line.
inline std::string window(const std::vector<std::string>& lines,
                           size_t from, size_t count)
{
  std::ostringstream oss;
  for (size_t i = from; i < lines.size() && i < from + count; i++) {
    if (i > from) oss << ' ';
    oss << lines[i];
  }
  return oss.str();
}

// Andy, 2026-09-27 (live feedback): "Now organizer bottom window is
// very verbose with failures" -- every reason returned by this header
// must fit on one line of the feedback bar, so callers never see a
// multi-line block.  Keep this short; truncate a raw quoted line
// rather than let it run on.
const size_t kMaxReasonLen = 140;

inline std::string truncate(const std::string& s, size_t maxLen = kMaxReasonLen)
{
  if (s.size() <= maxLen)
    return s;
  return s.substr(0, maxLen > 3 ? maxLen - 3 : maxLen) + "...";
}

// ------------------------------------------------------------------------
// Open MPI: "A request was made to bind that would require binding
// processes to more cpus than are available in your allocation" --
// gensub's own new ORCA "normal termination" marker check turns this
// into exit code 231 ("no ORCA TERMINATED NORMALLY"), which is truthful
// but tells the user nothing about *why* -- this is why (#-- Andy,
// 2026-09-27: "it doesn't tell us WHY... too many cores").
// ------------------------------------------------------------------------
inline bool matchOpenMpiBindOversubscribe(const std::vector<std::string>& lines,
                                           std::string& reason)
{
  static const std::regex trigger(
      "A request was made to bind that would require binding");
  static const std::regex procs("#processes:\\s*([0-9]+)");

  for (size_t i = 0; i < lines.size(); i++) {
    if (!std::regex_search(lines[i], trigger))
      continue;

    std::string blob = window(lines, i, 12);
    std::smatch m;
    std::ostringstream oss;
    if (std::regex_search(blob, m, procs)) {
      oss << m[1].str() << " MPI processes requested but not enough CPU "
          << "cores available -- lower the processor count.";
    } else {
      oss << "More MPI processes requested than CPU cores available -- "
          << "lower the processor count.";
    }
    reason = oss.str();
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------
// Open MPI's other common startup refusal: not enough slots configured
// for the requested process count (as opposed to the binding-vs-cpus
// check above).
// ------------------------------------------------------------------------
inline bool matchOpenMpiNotEnoughSlots(const std::vector<std::string>& lines,
                                        std::string& reason)
{
  static const std::regex trigger("There are not enough slots available");
  static const std::regex slots("not enough slots available[^0-9]*([0-9]+)");

  for (size_t i = 0; i < lines.size(); i++) {
    if (!std::regex_search(lines[i], trigger))
      continue;

    std::string blob = window(lines, i, 6);
    std::smatch m;
    std::ostringstream oss;
    oss << "Not enough job slots";
    if (std::regex_search(blob, m, slots))
      oss << " for the " << m[1].str() << " process(es) requested";
    oss << " -- lower the processor count.";
    reason = oss.str();
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------
// Slurm cancellations -- these come from slurm.err/slurm.out, not the
// code's own output file, when the queue itself killed the job.
// ------------------------------------------------------------------------
inline bool matchSlurmCancellation(const std::vector<std::string>& lines,
                                    std::string& reason)
{
  static const std::regex timeLimit("CANCELLED.*DUE TO TIME LIMIT");
  static const std::regex memLimit("[Ee]xceeded job memory limit");

  for (size_t i = 0; i < lines.size(); i++) {
    if (std::regex_search(lines[i], timeLimit)) {
      reason = "Job cancelled by the queue for exceeding its time limit "
                "-- request more walltime.";
      return true;
    }
    if (std::regex_search(lines[i], memLimit)) {
      reason = "Job killed by the queue for exceeding its memory limit "
                "-- request more memory.";
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------------------
// Gaussian: "Error termination via Lnk1e in .../lNNN.exe", plus whatever
// specific error line precedes it (e.g. "QPErr --- A syntax error...").
// ------------------------------------------------------------------------
inline bool matchGaussianError(const std::vector<std::string>& lines,
                                std::string& reason)
{
  static const std::regex lnk1e("Error termination via Lnk1e in (\\S+)");

  for (size_t i = 0; i < lines.size(); i++) {
    std::smatch m;
    if (!std::regex_search(lines[i], m, lnk1e))
      continue;

    // Gaussian's actual diagnostic (QPErr, a route-card parse error,
    // etc) is somewhere in the handful of non-blank lines right before
    // the Lnk1e termination line -- there can be a line or two of
    // parser-internal context (a quoted fragment of the bad input, a
    // cursor position) in between.  Collect a short backward window and
    // prefer whichever line names an actual error over the nearest one.
    std::vector<std::string> backLines;
    for (size_t j = i; j-- > 0 && backLines.size() < 5; ) {
      std::string t = trimmed(lines[j]);
      if (!t.empty())
        backLines.push_back(t);
    }
    std::string preceding;
    static const std::regex errWord("[Ee]rr(or)?\\b");
    for (size_t k = 0; k < backLines.size(); k++) {
      if (std::regex_search(backLines[k], errWord)) {
        preceding = backLines[k];
        break;
      }
    }
    if (preceding.empty() && !backLines.empty())
      preceding = backLines[0];

    // No general translation table for Gaussian's own error text (it's
    // one of many parser/runtime diagnostics) -- quote whichever single
    // line actually names the problem, truncated so it can never turn
    // into a multi-line block in the feedback bar.
    reason = "\"" + truncate(!preceding.empty() ? preceding
                                                 : trimmed(lines[i])) + "\"";
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------
// NWChem: "There is an error in the input file" and the block naming
// what's wrong that follows it.
// ------------------------------------------------------------------------
inline bool matchNWChemInputError(const std::vector<std::string>& lines,
                                   std::string& reason)
{
  static const std::regex trigger("There is an error in the input file");

  for (size_t i = 0; i < lines.size(); i++) {
    if (!std::regex_search(lines[i], trigger))
      continue;

    // NWChem prints the offending directive/module name on the first
    // non-blank, non-separator line right after this banner (the
    // banner itself is usually followed by a row of dashes) -- that
    // single line is the useful part, not the banner or the dashes.
    static const std::regex separator("^[-=]+$");
    std::string detail;
    for (size_t j = i + 1; j < lines.size(); j++) {
      std::string t = trimmed(lines[j]);
      if (t.empty() || std::regex_match(t, separator)) continue;
      detail = t;
      break;
    }
    reason = "\"There is an error in the input file: " +
             truncate(detail, kMaxReasonLen - 12) + "\"";
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------
// ORCA's own generic error markers, used only once the more specific
// Open MPI checks above have had a chance to fire (ORCA's startup abort
// on an MPI oversubscribe is exit-0 and prints no "ERROR" of its own --
// see the Open MPI matchers).
// ------------------------------------------------------------------------
inline bool matchOrcaError(const std::vector<std::string>& lines,
                            std::string& reason)
{
  static const std::regex fileError("\\[file [^\\]]*\\]:\\s*$");
  static const std::regex errorWord("\\bERROR\\b");

  for (size_t i = 0; i < lines.size(); i++) {
    if (std::regex_search(lines[i], fileError)) {
      // ORCA's "[file orca_tools/qcmsg.cpp, line 394]:" is a source
      // pointer with no message of its own -- the actual text ("....
      // aborting the run" or similar) is the next non-blank line, and
      // is the only part worth showing.
      std::string detail;
      for (size_t j = i + 1; j < lines.size(); j++) {
        std::string t = trimmed(lines[j]);
        if (t.empty()) continue;
        detail = t;
        break;
      }
      reason = "\"" + truncate(!detail.empty() ? detail
                                                 : trimmed(lines[i])) + "\"";
      return true;
    }
    if (std::regex_search(lines[i], errorWord)) {
      reason = "\"" + truncate(trimmed(lines[i])) + "\"";
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------------------
// Quantum ESPRESSO (pw.x).  Without convergence it prints "convergence NOT
// achieved after N iterations: stopping" and exits with status 2, which
// would otherwise be shown as "job exited with status 2".  Its other
// failures are boxed: "Error in routine <name> (<code>):" and the message.
// ------------------------------------------------------------------------
inline bool matchQuantumEspressoError(const std::vector<std::string>& lines,
                                       std::string& reason)
{
  static const std::regex noConv(
      "convergence NOT achieved after\\s+([0-9]+) iterations");
  static const std::regex routine("Error in routine\\s+(\\S+)");

  for (size_t i = 0; i < lines.size(); i++) {
    std::smatch m;
    if (std::regex_search(lines[i], m, noConv)) {
      reason = "The self-consistent field did not converge in " + m[1].str() +
               " iterations -- check the structure, lower the mixing factor "
               "or raise the iteration limit in Theory Details.";
      return true;
    }
    if (std::regex_search(lines[i], m, routine)) {
      std::string detail;
      for (size_t j = i + 1; j < lines.size(); j++) {
        std::string t = trimmed(lines[j]);
        if (t.empty()) continue;
        detail = t;
        break;
      }
      reason = "pw.x stopped in " + m[1].str() + ": \"" +
               truncate(detail, 90) + "\"";
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------------------
// Try every known pattern, in priority order (most specific/actionable
// first), across the primary output file and then any auxiliary files
// (stderr, etc) already available locally.  Returns "" if nothing
// recognizable was found -- callers should fall back to gensub's own
// line, and ultimately the generic numeric-status decode.
// ------------------------------------------------------------------------
inline std::string extractReason(const std::vector<std::string>& filePaths)
{
  typedef bool (*Matcher)(const std::vector<std::string>&, std::string&);
  static const Matcher matchers[] = {
    matchOpenMpiBindOversubscribe,
    matchOpenMpiNotEnoughSlots,
    matchSlurmCancellation,
    matchGaussianError,
    matchNWChemInputError,
    matchQuantumEspressoError,
    matchOrcaError,
  };

  for (size_t f = 0; f < filePaths.size(); f++) {
    std::vector<std::string> lines = readLines(filePaths[f]);
    if (lines.empty())
      continue;
    for (size_t m = 0; m < sizeof(matchers)/sizeof(matchers[0]); m++) {
      std::string reason;
      if (matchers[m](lines, reason))
        return truncate(reason);
    }
  }
  return "";
}

} // namespace JobFailureReason

#endif // JOBFAILUREREASON_H
