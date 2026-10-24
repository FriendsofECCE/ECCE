// MovingAverage: a rise-then-plateau series is followed, not dragged below.
#include <cstdio>
#include <cmath>
#include "MovingAverage.H"

static int fails = 0;
static void check(bool ok, const char *what)
{
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) fails++;
}

int main()
{
  check(MovingAverage::windowPoints(100) == 10, "10 % of 100 points is 10");
  check(MovingAverage::windowPoints(5) == 2, "never fewer than 2 points");
  check(MovingAverage::windowPoints(0) == 2, "empty series still 2");

  std::vector<double> y;
  for (int i = 0; i < 20; i++) y.push_back(-82700 + i * 1000);   // rise
  for (int i = 0; i < 180; i++) y.push_back(-65000);             // plateau
  const std::vector<double> m =
      MovingAverage::centred(y, MovingAverage::windowPoints(y.size()));
  check(m.size() == y.size(), "one value per point");
  check(std::fabs(m.back() - -65000) < 1e-9, "settles on the plateau value");
  double cum = 0;
  for (size_t i = 0; i < y.size(); i++) cum += y[i];
  check(m[150] > cum / y.size() + 500, "well above the cumulative mean");

  std::vector<double> c(30, 7.5);
  const std::vector<double> mc = MovingAverage::centred(c, 3);
  bool same = true;
  for (size_t i = 0; i < mc.size(); i++) same = same && std::fabs(mc[i] - 7.5) < 1e-12;
  check(same, "constant stays constant, ends included");

  const double four[] = {1, 2, 3, 4};
  const std::vector<double> m2 =
      MovingAverage::centred(std::vector<double>(four, four + 4), 2);
  check(m2[0] == 1 && m2[1] == 1.5 && m2[2] == 2.5 && m2[3] == 3.5,
        "window of 2: [i-1,i]");
  check(MovingAverage::centred(std::vector<double>(), 2).empty(),
        "empty in, empty out");
  return fails ? 1 : 0;
}
