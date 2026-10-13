/**
 * @file
 *
 * Reads calculations into the plain matrices FragmentAnalysis works on (#201),
 * through the same MoAoBasis the MO panel and ecce-mocomp use, so the basis
 * order, Cartesian/spherical choice, overlap matrix and per-code MO column
 * order are decided in one place.  Restricted calculations only.
 */

#ifndef FRAGMENTLOADER_HH
#define FRAGMENTLOADER_HH

#include <string>
#include <vector>

#include "tdat/FragmentAnalysis.H"
#include "tdat/MoAoBasis.H"

struct LoadedCalc
{
  string name;
  vector<string> symbols;
  vector<double> coordsAng;               // atom-major x,y,z, stored frame
  MoAoBasis basis;
  vector< vector<double> > rows;          // MO coefficients, basis order
  vector<double> energies, occupations;
};

/** Open a calculation folder (path or URL) and read its last geometry,
 *  basis, overlap and MOs.  `problem` says why on false. */
bool loadCalc(const string& path, LoadedCalc& out, string& problem);

FragmentParent parentOf(const LoadedCalc& calc);

/**
 * `frag` as a same-basis fragment of `parent`: each fragment atom is matched
 * to a parent atom of the same element within posTolAng, and its basis
 * functions (l, shell, component) must be the parent atom's.  Fails, naming
 * the atom, when no parent atom matches or the basis differs.
 */
bool sameBasisFragment(const LoadedCalc& parent, const LoadedCalc& frag,
                       double posTolAng, FragmentOrbitalSet& out,
                       string& problem);

#endif
