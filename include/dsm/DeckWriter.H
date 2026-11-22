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
class GUIValues;
class Fragment;
class JCode;
class TGBSConfig;
class TTheory;

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
    /// Preview (#983 Set-up tab): the deck goes to Result::deck and the
    /// calculation is never written.  The fields below, when set, replace
    /// the stored settings in the generator's parameter file (an editor's
    /// unsaved choices); structure and basis stay the stored ones.
    bool dryRun = false;
    const GUIValues *gui = 0;
    std::string category, theory, runtype;
    int spin = -1;
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
    /// The deck a dryRun generated.
    std::string deck;
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

  /// The code needs a periodic unit cell (<RequiresPeriodic> in its .edml)
  /// and the structure has none.
  bool missingCell(const JCode *code, const Fragment *frag);

  /// The Calculation Editor's "ready" rule (Launch enabled): a structure
  /// without open bonds, a cell when the code needs one, and a consistent,
  /// complete basis set when the theory needs one.  The editor passes its
  /// unsaved choices, makeReadyIfComplete what a calculation stores.
  bool setupComplete(Fragment *frag, TGBSConfig *basis,
                     const JCode *code, const TTheory *theory);

  /// When the stored settings pass setupComplete, writes the deck and makes
  /// the calculation ready; otherwise leaves it as it is, with why set.
  bool makeReadyIfComplete(ICalculation *calc, std::string& why);
}

#endif
