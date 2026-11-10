/**
 * @file
 * results.h5 for a calculation ECCE has parsed (#690): the Model is made
 * from the stored Props and the stored basis set, checked, and stored as a
 * data file of the calculation.  The readers of orbitals (MoAoBasis::
 * orbitalSet) take the file back through stored().
 *
 * The orbitals are converted from ECCE's own flattened basis (the one
 * BasisFlatten makes, in the code's column order) to the canonical AO order
 * and unit normalisation of results.h5.  Shells are read back from the
 * flattened functions, so every code whose orbitals ECCE reads is handled
 * by the same path.
 */
#ifndef RESULTSFROMCALC_HH
#define RESULTSFROMCALC_HH

#include <memory>
#include <string>
#include <vector>
using std::string;
using std::vector;

#include "tdat/ResultsH5.H"

class IPropCalculation;
class TProperty;
struct EspBasisFunction;

namespace ResultsFromCalc
{
  /** Name of the data file stored beside the calculation. */
  const char* const FILE_NAME = "results.h5";

  /** False when ECCE_RESULTS_H5=0 (both writing and reading are off). */
  bool enabled();

  /** bohr per Angstrom as BasisFlatten uses it for the basis centres. */
  double bohrPerAngstrom();

  /** What ECCE parsed, in ECCE's own flattened order. */
  struct Parsed
  {
    vector<string> symbols;
    vector<int>    atomicNumber;
    vector<double> coordsAng;                 // atom-major, stored frame
    vector<EspBasisFunction> basis;           // one per coefficient column
    vector<vector<double> > rows[2];          // alpha, beta (beta may be empty)
    vector<double> energy[2], occupation[2];
    string code, codeVersion, method, basisName;
  };

  /** Flat basis and rows -> canonical shells and coefficients. */
  bool convert(const Parsed& in, resultsh5::Model& out, string& why);

  /** The calculation's geometry, MOs and flattened basis. */
  bool gather(IPropCalculation *calc, Parsed& out, string& why);

  /**
   * The inverse of convert(): the file's orbitals (rows[0] alpha or
   * restricted, rows[1] beta or empty) over the calculation's own basis
   * columns, in MoAoBasis::coefficients order and normalisation, which for
   * Gaussian is the order Punch=MO writes and Guess=Cards reads.
   */
  bool codeColumns(IPropCalculation *calc, const resultsh5::Model& file,
                   vector<vector<double> > rows[2], string& why);

  /** Energy, charges, dipole, gradient, vibrations from the Props. */
  void addProperties(IPropCalculation *calc, resultsh5::Model& m);

  /**
   * tr(PS) = N per spin (1e-4 relative, absolute when N = 0) and C'SC = 1 over the occupied
   * orbitals (1e-5).  Sets m.checks ("ok: ..." / "failed: ...") and returns
   * the one-line summary for the job store's run log.
   */
  string check(resultsh5::Model& m);

  /** gather + convert + addProperties + check. */
  bool build(IPropCalculation *calc, resultsh5::Model& m, string& line,
             string& why);

  /**
   * Build and store results.h5 as a data file of the calculation.  `line`
   * is the run-log text; false (with the reason in `line`) when the
   * calculation has no orbitals ECCE can express, in which case an older
   * file is removed.  A failed check still stores the file, marked.
   */
  bool store(IPropCalculation *calc, string& line);

  /**
   * The calculation's stored results.h5, or null when there is none, it is
   * marked as failed, it is off (enabled()), or the calculation's orbital
   * point is not the default.  The one place that decides whether a view
   * reads the file or the Props.
   */
  std::shared_ptr<const resultsh5::Model> stored(IPropCalculation *calc);

  /**
   * Like stored(), for vibrations and traces: also a file without
   * orbitals (MD, MOPAC), and at any orbital point.
   */
  std::shared_ptr<const resultsh5::Model> storedAny(IPropCalculation *calc);

  /**
   * The views' read of the vibration and trace properties (VIBFREQ, VIBIR,
   * VIBRAM, VIB, SCANGEOM/ENERGY/COORD, IRCGEOM/ENERGY/COORD, GEOMTRACE,
   * TEVEC): a copy of the parsed property with its values from results.h5,
   * in the property's own units, when the file has them in the same shape;
   * else calc->getProperty(key).  Labels and units stay the property's, so
   * a calculation without the property gets null either way.  The copy
   * lives as long as the process; never update it.
   */
  TProperty *property(IPropCalculation *calc, const string& key);

  /**
   * A calculation shipped with results.h5 instead of its raw output and
   * parsed trace/vibration properties: fill `empty` (made by PropFactory for
   * `key`, one of the keys property() serves) from the file, with the type,
   * units, labels and layout the parsers write.  False when the file has no
   * such data; the caller then has no property, as before.
   */
  bool fromFile(IPropCalculation *calc, const string& key, TProperty *empty);

  /** The keys fromFile() can fill for this calculation. */
  vector<string> keysInFile(IPropCalculation *calc);

  /** Drop the cached copy (the calculation's properties were reloaded). */
  void forget(IPropCalculation *calc);
}

#endif
