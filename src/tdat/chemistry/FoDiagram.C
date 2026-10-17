//  The fragment-orbital correlation diagram: see include/tdat/FoDiagram.H.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>

#include "tdat/FoDiagram.H"

namespace {

const double HARTREE = 27.211386245988;      // eV

string percent(double x)
{
   char b[16];
   snprintf(b, sizeof b, "%.0f%%", 100.0*x);
   return b;
}

//  The text before the first comma: a ligand FO's dominant character.
string firstCharacter(const string& c)
{
   const string::size_type comma = c.find(',');
   return comma == string::npos ? c : c.substr(0, comma);
}

}  // namespace


bool FoDiagram::applicable(const vector<string>& symbols,
                           const vector<double>& coordsAng, int charge,
                           string& why)
{
   vector<FragmentOrbitals::Fragment> frags;
   if (!FragmentOrbitals::autoFragments(symbols, coordsAng, charge, 1,
                                        frags, why)) {
      return false;
   }
   bool ligand = false;
   for (size_t i = 0; i < frags.size(); i++) {
      if (frags[i].kind == FragmentOrbitals::SHELL) ligand = true;
   }
   if (!ligand) {
      why = "Not applicable: no ligand bonded to the metal.";
      return false;
   }
   string missing;
   if (!FragmentOrbitals::canSolve(symbols, missing)) {
      why = "No extended Hueckel parameters for " + missing + ".";
      return false;
   }
   return true;
}


bool FoDiagram::build(const vector<string>& symbols,
                      const vector<double>& coordsAng, int charge,
                      const vector<EspBasisFunction>& parentBasis,
                      const vector<double>& S,
                      const vector< vector<double> >& moRows,
                      const vector<double>& moEnergies,
                      MoColumn& centre, Result& out, string& why)
{
   out = Result();
   why.clear();
   if (moRows.size() != moEnergies.size()) {
      why = "The orbital energies and coefficients do not match.";
      return false;
   }

   vector<FragmentOrbitals::Fragment> frags;
   if (!FragmentOrbitals::autoFragments(symbols, coordsAng, charge, 1,
                                        frags, why)) {
      return false;
   }
   FragmentOrbitals::FoSet set;
   if (!FragmentOrbitals::build(symbols, coordsAng, frags,
                                FragmentOrbitals::LIGAND_SHELL, set, why)) {
      return false;
   }

   //  Only the drawn MOs are projected.
   vector<int> used;
   vector<int> rowOf(moRows.size(), -1);
   for (size_t c = 0; c < centre.levels.size(); c++) {
      const vector<int>& orbs = centre.levels[c].orbitals;
      for (size_t k = 0; k < orbs.size(); k++) {
         const int m = orbs[k];
         if (m < 0 || m >= (int)moRows.size() || rowOf[m] >= 0) continue;
         rowOf[m] = (int)used.size();
         used.push_back(m);
      }
   }
   if (used.empty()) {
      why = "There are no orbitals to describe.";
      return false;
   }
   vector< vector<double> > rows;
   for (size_t i = 0; i < used.size(); i++) rows.push_back(moRows[used[i]]);

   vector<FragmentOrbitals::MoShares> sh;
   if (!FragmentOrbitals::compose(set, parentBasis, S, rows, sh, why)) {
      return false;
   }

   vector< vector<int> > groups;
   FragmentOrbitals::groupFos(set, groups);
   const size_t nk = set.fos.size(), ng = groups.size();

   //  --- one linear map for every fragment level -------------------
   vector<double> W(nk, 0.0), M(nk, 0.0);
   for (size_t i = 0; i < used.size(); i++) {
      for (size_t k = 0; k < nk; k++) {
         W[k] += sh[i].perFo[k];
         M[k] += sh[i].perFo[k]*moEnergies[used[i]];
      }
   }
   double sw = 0, sx = 0, sy = 0;
   size_t points = 0;
   for (size_t k = 0; k < nk; k++) {
      if (W[k] < 0.05) continue;
      M[k] /= W[k];
      sw += W[k]; sx += W[k]*set.fos[k].energy; sy += W[k]*M[k];
      points++;
   }
   if (points < 3) {
      why = "Too few fragment orbitals contribute to the drawn orbitals "
            "to place them on the energy axis.";
      return false;
   }
   const double mx = sx/sw, my = sy/sw;
   double sxx = 0, sxy = 0, syy = 0;
   for (size_t k = 0; k < nk; k++) {
      if (W[k] < 0.05) continue;
      const double dx = set.fos[k].energy - mx, dy = M[k] - my;
      sxx += W[k]*dx*dx; sxy += W[k]*dx*dy; syy += W[k]*dy*dy;
   }
   if (sxx <= 1.0e-12 || sxy <= 0.0) {
      why = "The fragment orbital energies do not follow the orbital "
            "energies, so they cannot be placed on one scale.";
      return false;
   }
   out.slope = sxy/sxx;
   out.intercept = my - out.slope*mx;
   const double r = (syy > 0) ? sxy/sqrt(sxx*syy) : 0.0;

   //  --- shares per drawn level ------------------------------------
   const size_t nc = centre.levels.size();
   vector< vector<double> > share(nc, vector<double>(ng, 0.0));
   out.captureMin = 1.0; out.captureMax = 0.0;
   int uncaptured = 0;
   for (size_t c = 0; c < nc; c++) {
      MoLevel& level = centre.levels[c];
      double capture = 0, metal = 0, ligand = 0;
      int n = 0;
      for (size_t k = 0; k < level.orbitals.size(); k++) {
         const int m = level.orbitals[k];
         if (m < 0 || m >= (int)rowOf.size() || rowOf[m] < 0) continue;
         const FragmentOrbitals::MoShares& s = sh[rowOf[m]];
         for (size_t g = 0; g < ng; g++) share[c][g] += s.perGroup[g];
         capture += s.capture; metal += s.metal; ligand += s.ligand;
         n++;
      }
      if (n == 0) continue;
      for (size_t g = 0; g < ng; g++) share[c][g] /= n;
      capture /= n; metal /= n; ligand /= n;
      level.shareLeft = metal;
      level.shareRight = ligand;
      out.captureMin = std::min(out.captureMin, capture);
      out.captureMax = std::max(out.captureMax, capture);
      if (capture < FragmentOrbitals::CORE_CAPTURE) uncaptured++;

      //  The tooltip text: class, shares, capture, the largest FOs.
      std::ostringstream t;
      t << "Fragment orbitals (extended Hueckel): "
        << FragmentOrbitals::classOf(capture, metal, ligand)
        << ", metal " << percent(metal) << ", ligands " << percent(ligand)
        << ", captured " << percent(capture) << " of the MO";
      if (n > 1) t << " (mean over its orbitals)";
      t << ".";
      vector< std::pair<double,size_t> > rank;
      for (size_t g = 0; g < ng; g++) {
         if (share[c][g] >= FragmentOrbitals::TOP_MIN) {
            rank.push_back(std::make_pair(-share[c][g], g));
         }
      }
      std::sort(rank.begin(), rank.end());
      for (size_t q = 0; q < rank.size() && q < 4; q++) {
         const FragmentOrbitals::LevelShare d =
             FragmentOrbitals::describeGroup(set, groups[rank[q].second]);
         t << (q ? "; " : "\nLargest: ") << d.label << " "
           << percent(-rank[q].first);
         if (!d.character.empty()) t << " [" << d.character << "]";
      }
      level.detail = t.str();
   }

   //  --- the fragment columns: levels with at least one line --------
   vector<bool> linked(ng, false);
   for (size_t c = 0; c < nc; c++)
      for (size_t g = 0; g < ng; g++)
         if (share[c][g] >= LINK_MIN) linked[g] = true;

   vector< std::pair<double,size_t> > order[2];       // 0 metal, 1 ligand
   for (size_t g = 0; g < ng; g++) {
      if (!linked[g]) continue;
      const FragmentOrbitals::FO& f0 = set.fos[groups[g][0]];
      const bool metalSide =
          set.fragments[f0.fragment].kind == FragmentOrbitals::METAL;
      order[metalSide ? 0 : 1].push_back(std::make_pair(f0.energy, g));
   }
   if (order[0].empty() || order[1].empty()) {
      why = "No metal orbital or no ligand orbital makes up as much as " +
            percent(LINK_MIN) + " of any drawn orbital.";
      return false;
   }

   vector<int> index(ng, -1);
   for (int side = 0; side < 2; side++) {
      std::sort(order[side].begin(), order[side].end());
      MoColumn& col = side == 0 ? out.left : out.right;
      for (size_t i = 0; i < order[side].size(); i++) {
         const size_t g = order[side][i].second;
         index[g] = (int)i;
         const FragmentOrbitals::FO& f0 = set.fos[groups[g][0]];
         const FragmentOrbitals::LevelShare d =
             FragmentOrbitals::describeGroup(set, groups[g]);

         MoLevel lv;
         lv.energy = out.slope*f0.energy + out.intercept;
         lv.tabulated = f0.energy*HARTREE;
         lv.degeneracy = (int)groups[g].size();
         lv.energies.assign(lv.degeneracy, lv.energy);
         char eb[48];
         snprintf(eb, sizeof eb, "%.2f eV", f0.energy*HARTREE);
         if (side == 0) {
            lv.label = d.label;
            lv.annotation = string("H_ii ") + eb;
         } else {
            const string::size_type fo = d.label.find("FO");
            const string::size_type sp = d.label.find(" (", fo);
            lv.label = d.label.substr(fo, sp == string::npos ? string::npos
                                                              : sp - fo);
            lv.annotation = firstCharacter(d.character);
            for (size_t q = 0; q < groups[g].size(); q++)
               lv.occupancy += set.fos[groups[g][q]].occupancy;
         }
         col.levels.push_back(lv);
      }
   }
   out.left.title = "Metal AOs (extended Hueckel)";
   out.right.title = "Ligand orbitals (extended Hueckel)";

   for (size_t c = 0; c < nc; c++) {
      for (size_t g = 0; g < ng; g++) {
         if (share[c][g] < LINK_MIN) continue;
         MoConnection link;
         link.leftLevel = link.rightLevel = -1;
         link.centreLevel = (int)c;
         link.weight = share[c][g];
         const FragmentOrbitals::FO& f0 = set.fos[groups[g][0]];
         if (set.fragments[f0.fragment].kind == FragmentOrbitals::METAL)
            link.leftLevel = index[g];
         else
            link.rightLevel = index[g];
         out.links.push_back(link);
      }
   }

   char fit[200];
   snprintf(fit, sizeof fit, "Scale: E = %.2f E(EHT) %+.3f Hartree, fit r = %.2f.",
            out.slope, out.intercept, r);
   std::ostringstream n;
   n << "Fragment levels: extended Hueckel, placed by a fitted scale; "
        "qualitative.  The fragment orbitals describe the drawn molecular "
        "orbitals to "
     << percent(out.captureMin) << " - " << percent(out.captureMax)
     << " (capture).  " << fit
     << "  A line is drawn where a fragment orbital makes up at least "
     << percent(LINK_MIN) << " of an orbital; thicker is more.";
   if (uncaptured > 0)
      n << "  " << uncaptured << " drawn level"
        << (uncaptured > 1 ? "s are" : " is")
        << " not described by the valence fragment orbitals (capture "
           "below " << percent(FragmentOrbitals::CORE_CAPTURE) << ").";
   out.note = n.str();
   return true;
}
