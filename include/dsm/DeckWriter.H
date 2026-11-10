#ifndef DECKWRITER_HH
#define DECKWRITER_HH

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

  Result generate(ICalculation *calc, const Options& options = Options());

  /// Gives the calculation the library basis set called name for the
  /// elements of its own structure, as the Calculation Editor's basis choice
  /// does (library lookup, then the code's automatic optimisation).
  /// False with why set when the name or an element is not in the library.
  bool rebuildBasis(ICalculation *calc, const std::string& name,
                    std::string& why);
}

#endif
