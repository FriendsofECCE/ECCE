///////////////////////////////////////////////////////////////////////////////
// Standalone reproduction for GitHub issue #27:
//   "Gaussian 09 -- Assertion (false) failed trying to access out-of-bounds
//    index in PropTSVecTable  ASSERTION PropTSVecTable.C:126"
//
// Not wired into CMake (like the other hand-run test sources under
// src/dsm/dav/test).  Build and run from the repo root with:
//
//   g++ -std=c++17 -fsanitize=address -I include \
//       -o /tmp/prop27 src/tdat/test/PropTSVecTableIssue27Test.C \
//       build-cmake/libeccetdat.a build-cmake/libecceutil.a
//   ASAN_OPTIONS=detect_leaks=0 /tmp/prop27
//
// It builds a PropTSVecTable with exactly the shape the Gaussian GEOMTRACE
// pipeline produced for a Z-matrix-with-dummy-atom optimization (verified
// against real g16 output):
//
//   * gaussian-*.desc routes "Input orientation", "Z-Matrix orientation" and
//     "Standard orientation" blocks at Frequency=all into three parse types,
//     all of which run gaussian-*.geomtrace, which hardcodes
//     "key: GEOMTRACE" -- so all three land in one PropTSVecTable.
//   * "Z-Matrix orientation" lists dummy centres (atomic number -1),
//     "Standard orientation" does not, so the steps have different row counts
//     (5 vs 4 for NH3 with one dummy X).
//   * DavPropCache::putTSProp() writes the rows/columns attributes only when
//     it creates the property document, i.e. from the FIRST step; later steps
//     append a bare <step> element.  PropertyTask::getPropTSVecTable() then
//     calls setValues() with that single rows/columns pair.
//
// Before the fix: value(1,4,2) passes the row<p_numRows / col<p_numColumns
// check (4 < 5, 2 < 3), computes index = 4*3+2 = 14, and reads element 14 of
// a 12-element vector -- ASan reports a heap-buffer-overflow READ of size 8,
// 16 bytes past a 96-byte allocation.
// After the fix: the short step is dropped by setValues() with a warning, and
// the accessor bounds-checks against the real per-table vector size.
///////////////////////////////////////////////////////////////////////////////
#include <cstdio>
#include <vector>
#include <string>
using namespace std;
#include "tdat/PropTSVecTable.H"

// PropTSVecTable's constructor is private with "friend class PropFactory";
// declaring that name here is enough to construct one without dragging in
// all of dsm (and the properties config file PropFactory needs).
class PropFactory {
public:
  static PropTSVecTable* make() {
    return new PropTSVecTable("GEOMTRACE", "Angstrom");
  }
};

int main()
{
  PropTSVecTable *p = PropFactory::make();

  vector<vector<double> > *data = new vector<vector<double> >;
  vector<double> zmatOrientation;   // 5 centres x 3 coords (incl. dummy X)
  for (int i = 0; i < 15; i++) zmatOrientation.push_back(100.0 + i);
  vector<double> stdOrientation;    // 4 atoms x 3 coords (dummy dropped)
  for (int i = 0; i < 12; i++) stdOrientation.push_back(200.0 + i);
  data->push_back(zmatOrientation);
  data->push_back(stdOrientation);

  p->setValues(data, 5, 3);         // rows/columns from the FIRST step only

  printf("tables=%d rows=%d columns=%d\n", p->tables(), p->rows(),
         p->columns());

  // GTStepCmd::execute() loops j < sgfrag->numAtoms() and calls
  // trace->value(step, j, 0..2) -- the row index comes from the fragment,
  // not from the table.
  for (int t = 0; t < 2; t++) {
    printf("  value(%d,4,2) = %g\n", t, p->value(t, 4, 2));
  }

  delete p;
  return 0;
}
