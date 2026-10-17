#include <algorithm>
#include <map>
#include <sstream>

#include "tdat/MoPairs.H"

using std::map;
using std::ostringstream;


vector<MoPairs::PairCount>
MoPairs::countPairs(const vector<PairLevel>& levels)
{
   map<string, PairCount> by;
   vector<string> order;
   for (size_t i = 0; i < levels.size(); i++) {
      const PairLevel& l = levels[i];
      if (!by.count(l.irrep)) { by[l.irrep].irrep = l.irrep; order.push_back(l.irrep); }
      PairCount& c = by[l.irrep];
      const bool inPair = l.character == MoLevel::BONDING ||
                          l.character == MoLevel::ANTIBONDING;
      if (l.shown > 0) {
         if (l.character == MoLevel::BONDING) c.bonding += l.degeneracy;
         else if (l.character == MoLevel::ANTIBONDING) c.antibonding += l.degeneracy;
         else c.nonbonding += l.degeneracy;
      } else if (inPair) {
         (l.shown == 0 ? c.hiddenByCutoff : c.hiddenByFilter) += l.degeneracy;
      }
   }
   vector<PairCount> out;
   for (size_t i = 0; i < order.size(); i++) {
      PairCount c = by[order[i]];
      c.ok = c.bonding == c.antibonding;
      if (!c.ok) {
         ostringstream m;
         m << (c.irrep.empty() ? string("MOs") : c.irrep) << ": " << c.bonding
           << " bonding, " << c.antibonding << " antibonding: pairs incomplete";
         if (c.hiddenByCutoff > 0)
            m << "; " << c.hiddenByCutoff << " partner orbital"
              << (c.hiddenByCutoff > 1 ? "s are" : " is")
              << " beyond the levels cut-off";
         if (c.hiddenByFilter > 0)
            m << "; " << c.hiddenByFilter << " partner orbital"
              << (c.hiddenByFilter > 1 ? "s are" : " is")
              << " hidden (tick Show all levels)";
         c.message = m.str();
      }
      out.push_back(c);
   }
   return out;
}


vector<MoPairs::Pair>
MoPairs::pairByParents(const vector<Parent>& parents, const vector<Level>& L,
                       bool haveSym, double pairFloor, double linkMin)
{
   struct Cand { double score; size_t b, a; int sm, sl; };
   vector<Cand> cands;
   const size_t nL = L.size(), ns = parents.size();
   for (size_t b = 0; b < nL; b++) {
      const Level& vb = L[b];
      if (!vb.valence || (vb.irrep.empty() && haveSym)) continue;
      for (size_t a = 0; a < nL; a++) {
         const Level& va = L[a];
         if (a == b || !va.valence || va.irrep != vb.irrep ||
             va.degeneracy != vb.degeneracy) continue;
         if (va.energy <= vb.energy) continue;
         for (size_t s = 0; s < ns; s++) {
            if (!parents[s].centre || (haveSym && parents[s].irrep != vb.irrep)) continue;
            const double eM = parents[s].energy;
            if (!(vb.energy < eM && va.energy > eM)) continue;
            const double mm = std::min(vb.share[s], va.share[s]);
            if (mm < pairFloor) continue;
            for (size_t l = 0; l < ns; l++) {
               if (parents[l].centre || (haveSym && parents[l].irrep != vb.irrep)) continue;
               const double ml = std::min(vb.share[l], va.share[l]);
               if (ml < pairFloor) continue;
               //  the interaction has to be a real one on at least one side
               if (std::max(vb.share[s], va.share[s]) < linkMin ||
                   std::max(vb.share[l], va.share[l]) < linkMin) continue;
               Cand c; c.score = mm + ml; c.b = b; c.a = a; c.sm = (int)s; c.sl = (int)l;
               cands.push_back(c);
            }
         }
      }
   }
   std::sort(cands.begin(), cands.end(),
             [](const Cand& x, const Cand& y) { return x.score > y.score; });
   vector<Pair> out;
   vector<bool> used(nL, false);
   for (size_t i = 0; i < cands.size(); i++) {
      const Cand& c = cands[i];
      if (used[c.b] || used[c.a]) continue;
      used[c.b] = used[c.a] = true;
      Pair p; p.bonding = c.b; p.antibonding = c.a;
      p.centreParent = c.sm; p.ligandParent = c.sl;
      out.push_back(p);
   }
   return out;
}


static string stripMark(const string& label)
{
   string s = label;
   if (!s.empty() && s[s.size() - 1] == '*') s.erase(s.size() - 1);
   if (s == "nb") return string();
   if (s.size() > 3 && s.compare(s.size() - 3, 3, " nb") == 0) s.erase(s.size() - 3);
   return s;
}


void MoPairs::settle(vector<MoLevel>& L, vector<string>* messages)
{
   for (size_t i = 0; i < L.size(); i++)
      if (L[i].character == MoLevel::MIXED) { L[i].character = MoLevel::NONBONDING; L[i].pairing = -1; }

   vector<string> order;
   vector<size_t> lonely;
   for (size_t i = 0; i < L.size(); i++) {
      const MoLevel& a = L[i];
      if (a.character != MoLevel::BONDING && a.character != MoLevel::ANTIBONDING) continue;
      bool ok = false;
      if (a.pairing >= 0) {
         for (size_t j = 0; j < L.size() && !ok; j++) {
            const MoLevel& b = L[j];
            if (j == i || b.pairing != a.pairing || b.degeneracy != a.degeneracy) continue;
            if (a.character == MoLevel::BONDING && b.character == MoLevel::ANTIBONDING &&
                a.energy < b.energy) ok = true;
            if (a.character == MoLevel::ANTIBONDING && b.character == MoLevel::BONDING &&
                b.energy < a.energy) ok = true;
         }
      }
      if (ok) continue;
      //  Decided after the loop: a partner demoted here must not make
      //  its own partner look settled.
      lonely.push_back(i);
   }
   map<string, vector<string> > byIrrep;
   for (size_t k = 0; k < lonely.size(); k++) {
      MoLevel& a = L[lonely[k]];
      if (!byIrrep.count(a.irrep)) order.push_back(a.irrep);
      byIrrep[a.irrep].push_back(stripMark(a.label).empty() ? a.irrep : stripMark(a.label));
      a.character = MoLevel::NONBONDING;
      a.pairing = -1;
   }
   //  Demoting one end can orphan the other end of a pair that passed
   //  only because of it; the partners were all tested above against the
   //  original characters, so a second pass is needed for those.
   for (size_t i = 0; i < L.size(); i++) {
      MoLevel& a = L[i];
      if (a.character != MoLevel::BONDING && a.character != MoLevel::ANTIBONDING) continue;
      bool ok = false;
      for (size_t j = 0; j < L.size() && !ok; j++) {
         const MoLevel& b = L[j];
         if (j == i || b.pairing != a.pairing || a.pairing < 0) continue;
         if ((a.character == MoLevel::BONDING && b.character == MoLevel::ANTIBONDING) ||
             (a.character == MoLevel::ANTIBONDING && b.character == MoLevel::BONDING)) ok = true;
      }
      if (!ok) { a.character = MoLevel::NONBONDING; a.pairing = -1; }
   }

   for (size_t i = 0; i < L.size(); i++) {
      MoLevel& a = L[i];
      if (a.character == MoLevel::UNKNOWN) continue;
      string base = stripMark(a.label);
      if (a.character == MoLevel::ANTIBONDING) base += "*";
      else if (a.character == MoLevel::NONBONDING) base += base.empty() ? "nb" : " nb";
      a.label = base;
   }

   if (messages) {
      for (size_t k = 0; k < order.size(); k++) {
         const vector<string>& names = byIrrep[order[k]];
         ostringstream m;
         m << (order[k].empty() ? string("MOs") : order[k]) << ": ";
         for (size_t q = 0; q < names.size(); q++) m << (q ? ", " : "") << names[q];
         m << (names.size() == 1 ? " has" : " have")
           << " no partner among the levels drawn and "
           << (names.size() == 1 ? "is" : "are") << " shown as non-bonding";
         messages->push_back(m.str());
      }
   }
}
