// The MO Diagram's one-atom (aufbau) view: which arrows each orbital gets.
#include <cstdio>
#include "MoAufbauModel.H"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static const AufbauShell *find(const vector<AufbauShell>& s, const string& label)
{
  for (const AufbauShell& x : s) if (x.label == label) return &x;
  return 0;
}

int main()
{
  const vector<double> none;
  const vector<string> noneS;

  // O, restricted open shell: 2p at -0.46 (x2, one electron each) and -0.36 (two).
  {
    vector<double> e = { -19.26, -0.94, -0.46, -0.46, -0.36, 0.07, 0.07, 0.10, 0.49 };
    vector<double> o = { 2, 2, 1, 1, 2, 0, 0, 0, 0 };
    vector<string> l = { "1s", "2s", "2p", "2p", "2p", "2p (virtual)", "2p (virtual)",
                         "s (virtual)", "p (virtual)" };
    l[5] = l[6] = "p (virtual)";
    vector<AufbauShell> s = aufbau::shells(e, o, l, none, none, noneS);
    CHECK(aufbau::configuration("O", s, true) == "O: 1s2 2s2 2p4");
    const AufbauShell *p = find(s, "2p");
    CHECK(p && p->orbs.size() == 3);
    int up = 0, down = 0;
    if (p) for (const AufbauOrbital& x : p->orbs) { up += x.up; down += x.down; }
    CHECK(up == 3 && down == 1);
    CHECK(find(s, "1s") && find(s, "1s")->core);
    CHECK(find(s, "2s") && !find(s, "2s")->core);
    CHECK(p && !p->core);
    // The lowest empty subshells only: two of the three empty ones.
    int empty = 0;
    for (const AufbauShell& x : s) if (x.electrons == 0) empty++;
    CHECK(empty == 2);
    // In order of energy.
    for (size_t i = 1; i < s.size(); i++) CHECK(s[i-1].mean <= s[i].mean);
  }

  // C, unrestricted: alpha 1s 2s 2p 2p; beta 1s 2s only.
  {
    vector<double> e = { -11.3, -0.7, -0.4, -0.4, -0.4 };
    vector<double> o = { 1, 1, 1, 1, 0 };
    vector<string> l = { "1s", "2s", "2p", "2p", "2p" };
    vector<double> eB = { -11.3, -0.6, 0.0, 0.0, 0.0 };
    vector<double> oB = { 1, 1, 0, 0, 0 };
    vector<AufbauShell> s = aufbau::shells(e, o, l, eB, oB, l);
    CHECK(aufbau::configuration("C", s, true) == "C: 1s2 2s2 2p2");
    const AufbauShell *p = find(s, "2p");
    int up = 0, down = 0;
    if (p) for (const AufbauOrbital& x : p->orbs) { up += x.up; down += x.down; }
    CHECK(up == 2 && down == 0);
    const AufbauShell *s1 = find(s, "1s");
    CHECK(s1 && s1->orbs[0].up == 1 && s1->orbs[0].down == 1);
  }

  // Fractional (spherically averaged) occupation fills by Hund's rule.
  {
    vector<double> e = { -0.5, -0.3, -0.3, -0.3 };
    vector<double> o = { 2, 4.0 / 3, 4.0 / 3, 4.0 / 3 };
    vector<string> l = { "2s", "2p", "2p", "2p" };
    vector<AufbauShell> s = aufbau::shells(e, o, l, none, none, noneS);
    const AufbauShell *p = find(s, "2p");
    int up = 0, down = 0;
    if (p) for (const AufbauOrbital& x : p->orbs) { up += x.up; down += x.down; }
    CHECK(up == 3 && down == 1);
  }
  if (!failures) printf("aufbau model ok\n");
  return failures ? 1 : 0;
}
