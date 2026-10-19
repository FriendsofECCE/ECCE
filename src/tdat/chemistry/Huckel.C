//  Extended Huckel: molecular orbitals from a structure alone.
//
//  See include/tdat/Huckel.H for what this is for and, more
//  importantly, what it is not for.  The engine is FragmentOrbitals'
//  (EHTParameters, Slater functions fitted by SlaterExpansion); this
//  file keeps the H-Ne interface the diagram's own EHT path uses.

#include <cmath>
#include <string>
#include <vector>
using namespace std;

#include "tdat/FragmentOrbitals.H"
#include "tdat/Huckel.H"
#include "tdat/MoFragments.H"

namespace {

//  Hydrogen to neon, which is all this path has ever drawn.
const char* const FIRST_TWO_ROWS[] = {
   "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne"
};

bool firstTwoRows(const string& element)
{
   for (size_t i = 0; i < 10; i++) if (element == FIRST_TWO_ROWS[i]) return true;
   return false;
}

}  // namespace


bool Huckel::canSolve(const vector<string>& elements, string& missing)
{
   missing.clear();
   for (size_t i = 0; i < elements.size(); i++) {
      string unused;
      if (!FragmentOrbitals::canSolve(vector<string>(1, elements[i]), unused)) {
         if (!missing.empty()) missing += ", ";
         missing += elements[i];
      } else if (!firstTwoRows(elements[i])) {
         if (!missing.empty()) missing += ", ";
         missing += elements[i] + " (past the second row)";
      }
   }
   return missing.empty();
}


bool Huckel::solve(const vector<double>& coords,
                   const vector<string>& elements,
                   int charge,
                   vector<double>& energies,
                   vector<double>& occupancies,
                   vector< vector<double> >& coefficients,
                   vector<int>& perAtom,
                   vector<int>& shellOf,
                   string& why,
                   vector<double>* overlap)
{
   if (overlap != 0) overlap->clear();
   why.clear();
   energies.clear();
   occupancies.clear();
   coefficients.clear();
   perAtom.clear();
   shellOf.clear();

   string missing;
   if (!canSolve(elements, missing)) {
      why = "No extended Huckel parameters for " + missing + ".";
      return false;
   }
   if (coords.size() != 3*elements.size() || elements.empty()) {
      why = "The coordinates and the elements do not describe one molecule.";
      return false;
   }

   //  The whole molecule as one fragment: its orbitals are the
   //  molecule's.
   FragmentOrbitals::Fragment all;
   all.name = "molecule";
   all.kind = FragmentOrbitals::SHELL;
   all.charge = charge;
   for (size_t a = 0; a < elements.size(); a++) all.atoms.push_back((int)a);

   FragmentOrbitals::FoSet set;
   if (!FragmentOrbitals::build(elements, coords,
                                vector<FragmentOrbitals::Fragment>(1, all),
                                FragmentOrbitals::LIGAND_SHELL, set, why)) {
      return false;
   }
   const size_t n = set.funcs.size();
   if (n == 0 || set.fos.size() != n) { why = "No basis functions."; return false; }

   perAtom.assign(elements.size(), 0);
   for (size_t i = 0; i < n; i++) {
      perAtom[set.funcs[i].atom]++;
      shellOf.push_back(set.funcs[i].l);
   }

   for (size_t k = 0; k < n; k++) {
      energies.push_back(set.fos[k].energy);

      //  Unit Euclidean norm, which is the convention every reader of
      //  these rows was written against (the overlap is handed back
      //  for anything that needs the true inner product).
      vector<double> row = set.fos[k].coef;
      double norm = 0.0;
      for (size_t i = 0; i < n; i++) norm += row[i]*row[i];
      if (norm > 0.0) {
         norm = sqrt(norm);
         for (size_t i = 0; i < n; i++) row[i] /= norm;
      }
      coefficients.push_back(row);
   }

   if (overlap != 0) *overlap = set.overlap;

   int electrons = 0;
   for (size_t a = 0; a < elements.size(); a++) {
      electrons += MoFragments::valenceElectrons(elements[a]);
   }
   electrons -= charge;

   for (size_t k = 0; k < n; k++) {
      const double take = (electrons >= 2) ? 2.0
                                           : ((electrons == 1) ? 1.0 : 0.0);
      occupancies.push_back(take);
      electrons -= (int)take;
   }

   return true;
}
