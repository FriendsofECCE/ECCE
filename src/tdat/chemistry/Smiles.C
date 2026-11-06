#include "tdat/Smiles.H"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>

using namespace std;

namespace Smiles {

namespace {

const char *ELEMENTS[] = {
   "H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S",
   "Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga",
   "Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd",
   "Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd","Pm",
   "Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os",
   "Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn", 0};

bool isElement(const string& s)
{
   for (const char **e = ELEMENTS; *e; e++) if (s == *e) return true;
   return false;
}

// Valence of an aromatic or bracket atom of this charge, for deciding which
// aromatic atoms still need a double bond.
int chargedValence(const string& el, int q)
{
   if (el == "C" || el == "Si") return 4 - abs(q);
   if (el == "N" || el == "P" || el == "As") return 3 + q;
   if (el == "O" || el == "S" || el == "Se") return 2 + q;
   if (el == "B") return 3 - q;
   return 0;
}

int defaultValence(const string& el, int atLeast)
{
   static const map<string, vector<int> > v = {
      {"B", {3}}, {"C", {4}}, {"N", {3, 5}}, {"O", {2}}, {"P", {3, 5}},
      {"S", {2, 4, 6}}, {"F", {1}}, {"Cl", {1}}, {"Br", {1}}, {"I", {1}}};
   auto it = v.find(el);
   if (it == v.end()) return atLeast;
   for (int x : it->second) if (x >= atLeast) return x;
   return atLeast;
}

bool matchAromatic(const vector<vector<pair<int,int> > >& adj, vector<char>& need,
                   vector<int>& mate, long& budget)
{
   int pick = -1, bestN = 1 << 30;
   for (size_t i = 0; i < need.size(); i++) {
      if (!need[i] || mate[i] >= 0) continue;
      int n = 0;
      for (auto& pr : adj[i]) if (need[pr.first] && mate[pr.first] < 0) n++;
      if (n < bestN) { bestN = n; pick = (int)i; }
   }
   if (pick < 0) return true;
   if (bestN == 0 || --budget < 0) return false;
   for (auto& pr : adj[pick]) {
      int j = pr.first;
      if (!need[j] || mate[j] >= 0) continue;
      mate[pick] = j; mate[j] = pick;
      if (matchAromatic(adj, need, mate, budget)) return true;
      mate[pick] = mate[j] = -1;
   }
   return false;
}

}

bool parse(const string& s, Graph& g, string& err)
{
   g = Graph();
   auto fail = [&](size_t col, const string& m) {
      err = "SMILES column " + to_string(col + 1) + ": " + m;
      return false;
   };
   int prev = -1;
   vector<int> stack;
   int pendOrd = 0;           // 0 unspecified, 1,2,3, 5 aromatic
   char pendDir = 0;
   bool pending = false;
   size_t pendCol = 0;
   struct Ring { int atom; size_t slot; int ord; char dir; };
   map<int, Ring> rings;

   auto link = [&](int p, int q, int ord, char dir, int from, bool closure) {
      Bond b; b.a = p; b.b = q; b.ord = ord; b.dir = dir;
      b.dirFrom = dir ? from : -1; b.closure = closure;
      g.bonds.push_back(b);
   };
   auto addAtom = [&](const Atom& a) {
      g.atoms.push_back(a);
      g.nbr.emplace_back();
      g.prior.push_back(prev);
      int idx = (int)g.atoms.size() - 1;
      if (prev >= 0) {
         link(prev, idx, pendOrd, pendDir, prev, false);
         g.nbr[prev].push_back(idx);
         g.nbr[idx].push_back(prev);
      }
      pending = false; pendOrd = 0; pendDir = 0;
      prev = idx;
   };

   const size_t n = s.size();
   for (size_t i = 0; i < n;) {
      char c = s[i];
      if (isspace((unsigned char)c)) { i++; continue; }
      if (c == '(') {
         if (prev < 0) return fail(i, "a branch needs an atom before it");
         stack.push_back(prev); i++; continue;
      }
      if (c == ')') {
         if (stack.empty()) return fail(i, "')' without a matching '('");
         if (pending) return fail(pendCol, "bond symbol with no atom after it");
         prev = stack.back(); stack.pop_back(); i++; continue;
      }
      if (c == '.') {
         if (pending) return fail(pendCol, "bond symbol with no atom after it");
         if (!stack.empty()) return fail(i, "'.' inside a branch");
         prev = -1; i++; continue;
      }
      if (c == '-' || c == '=' || c == '#' || c == '$' || c == ':' || c == '/' || c == '\\') {
         if (pending) return fail(i, "two bond symbols in a row");
         pending = true; pendCol = i;
         if (c == '/' || c == '\\') pendDir = c;
         else if (c == '$') return fail(i, "quadruple bonds are not supported");
         else pendOrd = c == '-' ? 1 : c == '=' ? 2 : c == '#' ? 3 : 5;
         i++; continue;
      }
      if (isdigit((unsigned char)c) || c == '%') {
         size_t col = i;
         int r;
         if (c == '%') {
            if (i + 2 >= n || !isdigit((unsigned char)s[i+1]) || !isdigit((unsigned char)s[i+2]))
               return fail(i, "%nn ring closure needs two digits");
            r = (s[i+1] - '0') * 10 + (s[i+2] - '0'); i += 3;
         } else { r = c - '0'; i++; }
         if (prev < 0) return fail(col, "ring closure with no atom before it");
         auto it = rings.find(r);
         if (it == rings.end()) {
            rings[r] = {prev, g.nbr[prev].size(), pendOrd, pendDir};
            g.nbr[prev].push_back(-1);
         } else {
            Ring ro = it->second;
            if (ro.atom == prev) return fail(col, "a ring closure cannot join an atom to itself");
            for (int q : g.nbr[prev])
               if (q == ro.atom) return fail(col, "ring closure duplicates an existing bond");
            if (ro.ord && pendOrd && ro.ord != pendOrd)
               return fail(col, "the two ends of a ring closure give different bond orders");
            char dir = ro.dir ? ro.dir : pendDir;
            int from = ro.dir ? ro.atom : prev;
            link(ro.atom, prev, ro.ord ? ro.ord : pendOrd, dir, from, true);
            g.nbr[ro.atom][ro.slot] = prev;
            g.nbr[prev].push_back(ro.atom);
            rings.erase(it);
         }
         pending = false; pendOrd = 0; pendDir = 0;
         continue;
      }
      Atom a;
      if (c == '[') {
         size_t col = i++;
         string iso;
         while (i < n && isdigit((unsigned char)s[i])) iso += s[i++];
         if (!iso.empty()) a.isotope = atoi(iso.c_str());
         a.bracket = true;
         if (i >= n) return fail(col, "unclosed '['");
         if (s[i] == '*') { a.el = "*"; i++; }
         else if (islower((unsigned char)s[i])) {
            string two = s.substr(i, 2);
            if (two == "se" || two == "as") { a.el = two; i += 2; }
            else if (string("bcnops").find(s[i]) != string::npos) { a.el = string(1, s[i]); i++; }
            else return fail(i, string("unknown aromatic atom '") + s[i] + "'");
            a.aromatic = true;
            a.el[0] = toupper(a.el[0]);
         } else if (isupper((unsigned char)s[i])) {
            string two = s.substr(i, 2);
            if (two.size() == 2 && islower((unsigned char)two[1]) && isElement(two)) { a.el = two; i += 2; }
            else if (isElement(s.substr(i, 1))) { a.el = s.substr(i, 1); i++; }
            else return fail(i, "unknown element");
         } else return fail(i, "an element symbol is expected after '['");
         if (i < n && s[i] == '@') {
            a.chir = 1; i++;
            if (i < n && s[i] == '@') { a.chir = 2; i++; }
            if (i < n && isupper((unsigned char)s[i]) && s[i] != 'H')
               return fail(i, "only @ and @@ chirality are supported");
         }
         if (i < n && s[i] == 'H') {
            i++; a.hcount = 1;
            if (i < n && isdigit((unsigned char)s[i])) a.hcount = s[i++] - '0';
         }
         if (i < n && (s[i] == '+' || s[i] == '-')) {
            char sg = s[i++]; int q = 1;
            if (i < n && isdigit((unsigned char)s[i])) {
               q = 0;
               while (i < n && isdigit((unsigned char)s[i])) q = q * 10 + (s[i++] - '0');
            } else while (i < n && s[i] == sg) { q++; i++; }
            a.charge = sg == '+' ? q : -q;
         }
         if (i < n && s[i] == ':') {
            i++;
            if (i >= n || !isdigit((unsigned char)s[i])) return fail(i, "atom class needs digits");
            while (i < n && isdigit((unsigned char)s[i])) i++;
         }
         if (i >= n || s[i] != ']') return fail(i < n ? i : col, "expected ']'");
         i++;
      } else if (c == '*') {
         a.el = "*"; i++;
      } else {
         string two = s.substr(i, 2);
         if (two == "Cl" || two == "Br") { a.el = two; i += 2; }
         else if (string("BCNOPSFI").find(c) != string::npos) { a.el = string(1, c); i++; }
         else if (string("bcnops").find(c) != string::npos) {
            a.el = string(1, toupper(c)); a.aromatic = true; i++;
         } else return fail(i, string("unexpected character '") + c + "'");
      }
      addAtom(a);
   }
   if (pending) return fail(pendCol, "bond symbol with no atom after it");
   if (!stack.empty()) return fail(n, "unclosed '('");
   if (!rings.empty()) return fail(n, "ring closure " + to_string(rings.begin()->first) + " is never closed");
   if (g.atoms.empty()) { err = "No SMILES given."; return false; }

   const int na = (int)g.atoms.size();
   vector<vector<int> > bondsOf(na);
   for (size_t k = 0; k < g.bonds.size(); k++) {
      Bond& b = g.bonds[k];
      bondsOf[b.a].push_back((int)k);
      bondsOf[b.b].push_back((int)k);
      if (b.ord == 0) b.ord = (g.atoms[b.a].aromatic && g.atoms[b.b].aromatic) ? 5 : 1;
      if (b.ord == 5) { b.arom = true; b.ord = 1; }
   }
   // Hydrogens of aromatic and bracket atoms, and which aromatic atoms
   // still need a double bond.
   vector<char> need(na, 0);
   for (int i = 0; i < na; i++) {
      Atom& a = g.atoms[i];
      int fixedSum = 0, nArom = 0;
      for (int k : bondsOf[i]) {
         if (g.bonds[k].arom) nArom++; else fixedSum += g.bonds[k].ord;
      }
      if (!a.aromatic) continue;
      if (!a.bracket) {
         int sigma = a.el == "C" ? 3 : (a.el == "N" || a.el == "P" || a.el == "B") ? 2 : 0;
         a.hcount = max(0, sigma - (fixedSum + nArom));
      }
      if (nArom == 0) return fail(0, "aromatic atom " + a.el + " is in no aromatic ring");
      int free = chargedValence(a.el, a.bracket ? a.charge : 0) -
                 (fixedSum + nArom + a.hcount);
      if (!a.bracket && a.el != "C") free = (a.el == "N" || a.el == "P") ? 3 - (fixedSum + nArom + a.hcount) : free;
      need[i] = free >= 1;
   }
   // Kekulise: a perfect matching of the atoms that need a double bond.
   vector<vector<pair<int,int> > > adj(na);
   bool anyArom = false;
   for (size_t k = 0; k < g.bonds.size(); k++) {
      if (!g.bonds[k].arom) continue;
      anyArom = true;
      adj[g.bonds[k].a].push_back({g.bonds[k].b, (int)k});
      adj[g.bonds[k].b].push_back({g.bonds[k].a, (int)k});
   }
   if (anyArom) {
      vector<int> mate(na, -1);
      long budget = 2000000;
      if (!matchAromatic(adj, need, mate, budget))
         return fail(0, "the aromatic atoms cannot be given alternating double bonds (not a valid aromatic system)");
      for (Bond& b : g.bonds)
         if (b.arom && mate[b.a] == b.b) b.ord = 2;
   }
   for (int i = 0; i < na; i++) {
      Atom& a = g.atoms[i];
      if (a.bracket || a.aromatic || a.el == "*") continue;
      int sum = 0;
      for (int k : bondsOf[i]) sum += g.bonds[k].ord;
      a.hcount = max(0, defaultValence(a.el, sum) - sum);
   }
   return true;
}

}
