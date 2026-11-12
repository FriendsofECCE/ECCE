#ifndef DECKWRITER_HH
#define DECKWRITER_HH

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class ICalculation;

/**
 * Makes a calculation's input deck from what it stores: the .frag/.basis/
 * .param/.esp/.con files and the code's ai.<code> generator (its one process),
 * and puts the deck into the calculation.  The Calculation Editor's Save and
 * the series command both call this, so there is one implementation.
 * No wx.
 */
namespace DeckWriter {

  struct Options {
    bool useExpCoeff;
    /// Keep the generator's temporary directory (Result::keptDir).
    bool keepFiles;
    /// Open shells to write; -1 takes the calculation's stored value.
    int openShells;
    /// Told the name of each slow step as it starts (a progress display).
    std::function<void(const std::string&)> stage;
    Options() : useExpCoeff(false), keepFiles(false), openShells(-1) {}
  };

  struct Result {
    bool ok;
    /// Why not, with the generator's own output.
    std::string message;
    /// Combination warnings from the generator: (anchor, text).
    std::vector<std::pair<std::string, std::string> > warnings;
    /// Set when the symmetry-unique atoms could not be expanded.
    std::string fullMoleculeWarning;
    std::string qstStatus, qstDetail, qstTable;
    std::string keptDir;
    Result() : ok(false) {}
  };

  /// ECCE_SAVE_TIMING=1: one stderr line per Save step with its elapsed ms
  /// (where a Save of a large structure spends its time, #964).
  class StepTimer {
  public:
    explicit StepTimer(const char *what)
      : p_on(getenv("ECCE_SAVE_TIMING") != 0), p_what(what),
        p_t0(std::chrono::steady_clock::now()), p_last(p_t0) {}
    void lap(const std::string& step) {
      if (!p_on) return;
      const auto now = std::chrono::steady_clock::now();
      fprintf(stderr, "ECCE_SAVE_TIMING: %s: %-34s %8.1f ms (total %.1f)\n",
              p_what, step.c_str(),
              std::chrono::duration<double, std::milli>(now - p_last).count(),
              std::chrono::duration<double, std::milli>(now - p_t0).count());
      p_last = std::chrono::steady_clock::now();
    }
  private:
    bool p_on;
    const char *p_what;
    std::chrono::steady_clock::time_point p_t0, p_last;
  };

  Result generate(ICalculation *calc, const Options& options = Options());

  /// Gives the calculation the library basis set called name for the
  /// elements of its own structure, as the Calculation Editor's basis choice
  /// does (library lookup, then the code's automatic optimisation).
  /// False with why set when the name or an element is not in the library.
  bool rebuildBasis(ICalculation *calc, const std::string& name,
                    std::string& why);
}

#endif
