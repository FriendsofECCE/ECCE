// A step past the end of a trace gets the last frame, not zero coordinates.
#include <cstdio>
#include <vector>
using namespace std;
#include "tdat/PropTSVecTable.H"

class PropFactory {
public:
  static PropTSVecTable* make() { return new PropTSVecTable("GEOMTRACE", "Angstrom"); }
};

int main()
{
  PropTSVecTable *p = PropFactory::make();
  vector<vector<double> > *data = new vector<vector<double> >;
  for (int t = 0; t < 3; t++) data->push_back(vector<double>(6, 10.0 * (t + 1)));
  p->setValues(data, 2, 3);

  int fails = 0;
  const int in[]   = { 0, 2, 3, 500, -4 };
  const int want[] = { 0, 2, 2,   2,  0 };
  for (int i = 0; i < 5; i++) {
    int t = p->nearestTable(in[i]);
    double v = t < 0 ? 0 : p->value(t, 1, 2);
    printf("step %d -> table %d value %g\n", in[i], t, v);
    if (t != want[i] || v != 10.0 * (want[i] + 1)) fails++;
  }
  delete p;
  return fails ? 1 : 0;
}
