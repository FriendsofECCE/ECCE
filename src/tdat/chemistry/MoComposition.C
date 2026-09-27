#include <map>
#include <utility>

#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"

using std::map;
using std::pair;


vector<MoComposition::Share>
MoComposition::compute(const vector<double>& coefficients,
                       const vector<int>& functionsPerAtom,
                       const vector<int>& shellOf,
                       const vector<string>& elements,
                       const vector<double>& overlap)
{
   vector<Share> out;

   const size_t nbas = coefficients.size();
   if (nbas == 0 || functionsPerAtom.size() != elements.size()) return out;

   //  Same check share() makes: the mapping has to account for every
   //  function, or a mismatch produces a plausible-looking answer
   //  instead of an error.
   int total = 0;
   for (size_t i = 0; i < functionsPerAtom.size(); i++) total += functionsPerAtom[i];
   if (total != (int)nbas) return out;

   const bool haveShell = (shellOf.size() == nbas);

   //  Which atom each basis function belongs to, and (when given)
   //  which shell -- so the functions can be grouped by (atom, shell)
   //  regardless of where in the row they fall.
   vector<int> atomOf(nbas, -1);
   {
      int at = 0;
      for (size_t a = 0; a < functionsPerAtom.size(); a++) {
         for (int f = 0; f < functionsPerAtom[a]; f++) atomOf[at + f] = (int)a;
         at += functionsPerAtom[a];
      }
   }

   //  Grouped in (atom, shell) order of first appearance -- a plain
   //  map<pair<int,int>, ...> would reorder by shell number first,
   //  which reads oddly for a per-atom breakdown (2p before 2s).
   vector< pair<int,int> > keys;             // (atom, shell) as encountered
   map< pair<int,int>, vector<int> > groups;
   for (size_t mu = 0; mu < nbas; mu++) {
      const int a = atomOf[mu];
      if (a < 0) continue;
      const int shell = haveShell ? shellOf[mu] : -1;
      const pair<int,int> key(a, shell);
      if (groups.find(key) == groups.end()) keys.push_back(key);
      groups[key].push_back((int)mu);
   }

   out.reserve(keys.size());
   for (size_t i = 0; i < keys.size(); i++) {
      const int a = keys[i].first;
      const int shell = keys[i].second;
      const double s = MoFragments::shareOfIndices(coefficients,
                                                    groups[keys[i]], overlap);
      if (s < 0.0) continue;

      Share entry;
      entry.atom    = a;
      entry.element = (a >= 0 && a < (int)elements.size()) ? elements[a] : string();
      entry.shell   = shell;
      entry.share   = s;
      out.push_back(entry);
   }

   return out;
}
