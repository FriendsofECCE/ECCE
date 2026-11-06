#include "tdat/ShellLabel.H"

#include <map>

using std::string;
using std::vector;
using std::pair;

namespace {

const char *const symbols[] = {
  "H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S",
  "Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga",
  "Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd",
  "Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd","Pm",
  "Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os",
  "Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn","Fr","Ra","Ac","Th","Pa",
  "U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr","Rf","Db","Sg",
  "Bh","Hs","Mt","Ds","Rg","Cn","Nh","Fl","Mc","Lv","Ts","Og" };

int atomicNumber(const string& element)
{
  for (int z = 0; z < 118; z++)
    if (element == symbols[z]) return z + 1;
  return 0;
}

const int noble[] = { 0, 2, 10, 18, 36, 54, 86, 118 };

//  Madelung order, which is the filling order of the noble gases.
const int madelung[][2] = {
  {1,0},{2,0},{2,1},{3,0},{3,1},{4,0},{3,2},{4,1},{5,0},{4,2},{5,1},
  {6,0},{4,3},{5,2},{6,1},{7,0},{5,3},{6,2},{7,1} };
}


int ShellLabel::period(const string& element)
{
  const int z = atomicNumber(element);
  if (z == 0) return 0;
  for (int p = 1; p <= 7; p++) if (z <= noble[p]) return p;
  return 0;
}


int ShellLabel::valenceN(const string& element, int l)
{
  const int p = period(element);
  if (p == 0) return l + 1;
  const int n = p - (l >= 2 ? l - 1 : 0);
  return n > l + 1 ? n : l + 1;
}


vector< pair<int,int> > ShellLabel::coreShells(const string& element)
{
  vector< pair<int,int> > shells;
  const int p = period(element);
  if (p < 2) return shells;
  int left = noble[p - 1];
  for (size_t k = 0; k < sizeof madelung/sizeof madelung[0] && left > 0; k++) {
    shells.push_back(pair<int,int>(madelung[k][0], madelung[k][1]));
    left -= 2*(2*madelung[k][1] + 1);
  }
  return shells;
}


string ShellLabel::shellName(const string& element, int n, int l)
{
  static const char letters[] = "spdfghik";
  string s = element + " " + std::to_string(n);
  s += (l >= 0 && l < 8) ? letters[l] : '?';
  return s;
}


vector<string> ShellLabel::atomNames(const vector<string>& elements)
{
  std::map<string,int> count;
  for (size_t i = 0; i < elements.size(); i++) count[elements[i]]++;
  std::map<string,int> seen;
  vector<string> names;
  for (size_t i = 0; i < elements.size(); i++) {
    const int k = ++seen[elements[i]];
    names.push_back(count[elements[i]] > 1
                    ? elements[i] + "(" + std::to_string(k) + ")"
                    : elements[i]);
  }
  return names;
}


int ShellLabel::coreCapacity(const vector<string>& elements)
{
  int total = 0;
  for (size_t i = 0; i < elements.size(); i++) {
    const vector< pair<int,int> > shells = coreShells(elements[i]);
    for (size_t k = 0; k < shells.size(); k++) total += 2*shells[k].second + 1;
  }
  return total;
}


vector<ShellLabel::Core> ShellLabel::coreOrbitals(
    const vector<string>& elements, const vector< vector<Part> >& orbitals,
    double threshold)
{
  vector<Core> out(orbitals.size());
  std::map<string,int> atoms;
  for (size_t i = 0; i < elements.size(); i++) atoms[elements[i]]++;

  std::map< pair<string,int>, int > seen;   // orbitals taken so far per (element, l)
  for (size_t o = 0; o < orbitals.size(); o++) {
    //  Share of each (element, l).
    std::map< pair<string,int>, double > sum;
    for (size_t k = 0; k < orbitals[o].size(); k++) {
      const Part& p = orbitals[o][k];
      if (p.atom < 0 || p.atom >= (int)elements.size()) continue;
      sum[pair<string,int>(elements[p.atom], p.l)] += p.share;
    }
    for (std::map< pair<string,int>, double >::iterator it = sum.begin();
         it != sum.end(); ++it) {
      if (it->second < threshold) continue;
      const string& el = it->first.first;
      const int l = it->first.second;
      const int rank = seen[it->first]++;
      vector<int> ns;   // the core shells of this l, in order
      const vector< pair<int,int> > shells = coreShells(el);
      for (size_t k = 0; k < shells.size(); k++)
        if (shells[k].second == l) ns.push_back(shells[k].first);
      const int shell = rank/((2*l + 1)*atoms[el]);
      if (shell < (int)ns.size()) {
        out[o].core = true; out[o].element = el; out[o].l = l;
        out[o].n = ns[shell];
      }
      break;
    }
  }
  return out;
}


string ShellLabel::partName(const vector<string>& elements,
                            const vector<string>& names,
                            const Part& part, const Core& core)
{
  if (part.atom < 0 || part.atom >= (int)elements.size()) return "?";
  const string& el = elements[part.atom];
  const int n = (core.core && core.element == el && core.l == part.l)
                ? core.n : valenceN(el, part.l);
  string s = (part.atom < (int)names.size() ? names[part.atom] : el);
  static const char letters[] = "spdfghik";
  s += " " + std::to_string(n);
  s += (part.l >= 0 && part.l < 8) ? letters[part.l] : '?';
  return s;
}
