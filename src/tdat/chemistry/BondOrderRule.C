#include "tdat/BondOrderRule.H"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <sstream>

using namespace std;

namespace {

int valenceOf(const string& e)
{
  if (e == "C") return 4;
  if (e == "N") return 3;
  if (e == "O" || e == "S") return 2;
  return -1;                    // takes no part in multiple bonds
}

struct Solver {
  int n;
  vector<int> rem;                              // free valence left
  vector<vector<pair<int,int> > > adj;          // (neighbour, bond index)
  vector<int> inc;                              // increments per bond
  vector<int> bestInc;
  int bestRem;
  long nodes;

  void search()
  {
    if (++nodes > 200000) return;
    // Most constrained atom with free valence and at least one partner.
    int pick = -1, pickCand = 1 << 30, total = 0;
    for (int a = 0; a < n; a++) {
      if (rem[a] <= 0) continue;
      total += rem[a];
      int c = 0;
      for (auto& nb : adj[a])
        if (rem[nb.first] > 0 && inc[nb.second] < 2) c++;
      if (c > 0 && c < pickCand) { pick = a; pickCand = c; }
    }
    if (total < bestRem) { bestRem = total; bestInc = inc; }
    if (pick < 0 || total == 0) return;
    vector<pair<int,int> > cand;
    for (auto& nb : adj[pick])
      if (rem[nb.first] > 0 && inc[nb.second] < 2) cand.push_back(nb);
    // Terminal partners first: they have no other use for their valence.
    stable_sort(cand.begin(), cand.end(),
      [&](const pair<int,int>& x, const pair<int,int>& y) {
        return adj[x.first].size() < adj[y.first].size(); });
    for (auto& nb : cand) {
      inc[nb.second]++; rem[pick]--; rem[nb.first]--;
      search();
      inc[nb.second]--; rem[pick]++; rem[nb.first]++;
      if (bestRem == 0 || nodes > 200000) return;
    }
  }
};

string label(const vector<string>& el, int i)
{
  ostringstream os;
  os << el[i] << (i + 1);
  return os.str();
}

const char* shape(int neighbours)
{
  return neighbours == 1 ? "terminal" : neighbours == 2 ? "linear"
       : neighbours == 3 ? "trigonal" : "";
}

}

namespace BondOrderRule {

Result perceive(const vector<string>& el, const vector<pair<int,int> >& bonds)
{
  Result res;
  const int n = (int)el.size(), nb = (int)bonds.size();
  res.orders.assign(nb, 1.0);

  Solver s;
  s.n = n;
  s.adj.assign(n, vector<pair<int,int> >());
  vector<int> deg(n, 0);
  for (int b = 0; b < nb; b++) {
    deg[bonds[b].first]++;
    deg[bonds[b].second]++;
  }
  s.rem.assign(n, 0);
  for (int a = 0; a < n; a++) {
    int v = valenceOf(el[a]);
    s.rem[a] = v < 0 ? 0 : max(0, v - deg[a]);
    if (el[a] == "S" && deg[a] > 2) s.rem[a] = 0;
  }
  for (int b = 0; b < nb; b++) {
    int x = bonds[b].first, y = bonds[b].second;
    if (s.rem[x] > 0 && s.rem[y] > 0) {
      s.adj[x].push_back(make_pair(y, b));
      s.adj[y].push_back(make_pair(x, b));
    }
  }
  s.inc.assign(nb, 0);
  s.bestInc = s.inc;
  s.bestRem = 1 << 30;
  s.nodes = 0;
  vector<int> start = s.rem;
  s.search();
  s.rem = start;
  for (int b = 0; b < nb; b++) {
    s.rem[bonds[b].first] -= s.bestInc[b];
    s.rem[bonds[b].second] -= s.bestInc[b];
    res.orders[b] = 1.0 + s.bestInc[b];
  }

  for (int a = 0; a < n; a++)
    if (s.rem[a] > 0 || ((el[a] == "S" || el[a] == "P") && deg[a] > 2))
      res.unresolved.push_back(a);

  // Six-membered rings of alternating double/single bonds with every atom's
  // double bond inside the ring are aromatic: all six bonds become 1.5.
  map<pair<int,int>, int> bondOf;
  for (int b = 0; b < nb; b++) {
    bondOf[make_pair(bonds[b].first, bonds[b].second)] = b;
    bondOf[make_pair(bonds[b].second, bonds[b].first)] = b;
  }
  vector<vector<int> > nbr(n);
  for (int b = 0; b < nb; b++) {
    nbr[bonds[b].first].push_back(bonds[b].second);
    nbr[bonds[b].second].push_back(bonds[b].first);
  }
  vector<int> ringBonds;
  int rings = 0;
  vector<int> path;
  function<void(int)> walk = [&](int start0) {
    int cur = path.back();
    if (path.size() == 6) {
      if (!bondOf.count(make_pair(cur, start0))) return;
      if (path[1] > path[5]) return;           // one direction only
      vector<int> rb;
      int dbl = 0;
      for (int k = 0; k < 6; k++)
        rb.push_back(bondOf[make_pair(path[k], path[(k + 1) % 6])]);
      for (int k = 0; k < 6; k++) {
        if (el[path[k]] != "C" && el[path[k]] != "N") return;
        double o1 = res.orders[rb[k]], o2 = res.orders[rb[(k + 5) % 6]];
        if (!((o1 == 2.0 && o2 == 1.0) || (o1 == 1.0 && o2 == 2.0)))
          return;
        if (o1 == 2.0) dbl++;
      }
      if (dbl == 3) {
        ringBonds.insert(ringBonds.end(), rb.begin(), rb.end());
        rings++;
      }
      return;
    }
    for (int nxt : nbr[cur]) {
      if (nxt < start0) continue;
      if (find(path.begin(), path.end(), nxt) != path.end()) continue;
      path.push_back(nxt);
      walk(start0);
      path.pop_back();
    }
  };
  for (int a = 0; a < n; a++)
    if (el[a] == "C" || el[a] == "N") {
      path.assign(1, a);
      walk(a);
    }
  for (int b : ringBonds) res.orders[b] = 1.5;

  for (int b = 0; b < nb; b++) {
    int x = bonds[b].first, y = bonds[b].second;
    double o = res.orders[b];
    if (o == 1.0 || o == 1.5) continue;
    ostringstream os;
    os << label(el, x) << (o == 2.0 ? "=" : "#") << label(el, y)
       << (o == 2.0 ? " double (" : " triple (");
    if (deg[x] == deg[y]) os << "both " << shape(deg[x]);
    else os << label(el, x) << " " << shape(deg[x]) << ", "
            << label(el, y) << " " << shape(deg[y]);
    os << ")";
    res.notes.push_back(os.str());
  }
  if (rings) {
    ostringstream os;
    os << rings << " six-membered ring" << (rings > 1 ? "s" : "")
       << " treated as aromatic (bond order 1.5)";
    res.notes.push_back(os.str());
  }
  return res;
}

double typicalLength(const string& e1, const string& e2, double order)
{
  // Covalent radii (Cordero 2008) give the single bond; higher orders shorter.
  static const map<string,double> r = {
    {"H",0.31},{"B",0.84},{"C",0.76},{"N",0.71},{"O",0.66},{"F",0.57},
    {"Si",1.11},{"P",1.07},{"S",1.05},{"Cl",1.02},{"Br",1.20},{"I",1.39}};
  auto a = r.find(e1), b = r.find(e2);
  if (a == r.end() || b == r.end()) return 0.0;
  double d = a->second + b->second;
  if (e1 == "H" || e2 == "H") return d;
  if (order >= 3.0) return d * 0.78;
  if (order >= 2.0) return d * 0.87;
  if (order >= 1.5) return d * 0.92;
  return d;
}

}
