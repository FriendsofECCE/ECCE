// Reader for ECCE's basis-set library files (data/admin/basissets/*.BAS).
//
// File format: "atom=<Sym>" starts an element block; each
// "contraction shell=<S|P|D|F|G|H|I|SP|SS|...> num_primitives=N
// num_coefficients=M" line is followed by N rows of "exponent c1 ... cM".
// The shell string has one letter per coefficient column (SP = an s and a p
// sharing exponents).  ECCE stores polarisation sets as separate files
// (6-31G* = 6-31G.BAS + 6-31GS.BAS), so a name may combine several files.
#pragma once
#include <map>
#include <string>
#include <vector>

namespace qm {

// One contracted shell: a single angular momentum, one coefficient column.
struct ShellDef {
  int l = 0;
  std::vector<double> exps;
  std::vector<double> coefs;  // as in the file (primitive-normalised, see Basis)
};

class BasisLibrary {
 public:
  // dir: directory holding the .BAS files.  Empty = search ECCE_BASIS_DIR,
  // $ECCE_HOME/data/admin/basissets, then the build-time default.
  explicit BasisLibrary(const std::string& dir = "");

  // name: "def2-SVP", "6-31G*", "6-31G**", "STO-3G", or "fileA+fileB".
  // Shells of all files for element Z, in file order (s first within a file).
  std::vector<ShellDef> shells(const std::string& name, int Z);

  // Explicit shells for element Z, used instead of a library file whenever
  // shells() is asked for that element (ECCE writes the basis it has stored
  // into the input, so a user-edited set is honoured).
  void set_explicit(int Z, std::vector<ShellDef> shells) { explicit_[Z] = std::move(shells); }
  bool has_explicit() const { return !explicit_.empty(); }

  const std::string& dir() const { return dir_; }

  // Files a name expands to ("6-31G*" -> {"6-31G", "6-31GS"}).
  static std::vector<std::string> expand_name(const std::string& name);

 private:
  using Table = std::map<int, std::vector<ShellDef>>;
  const Table& load_file(const std::string& base);
  std::string find_file(const std::string& base) const;

  std::string dir_;
  std::map<int, std::vector<ShellDef>> explicit_;
  std::map<std::string, Table> cache_;
};

}  // namespace qm
