#ifndef STEPPLOTAXES_HH
#define STEPPLOTAXES_HH

// What the per-step plots (Geometry Trace, Geometry Step Plots) share: the
// name drawn as a title, the unit text, and the time axis of an MD run.

#include <string>
#include <vector>

#include <wx/string.h>

#include "dsm/IPropCalculation.H"
#include "tdat/PropVector.H"
#include "util/UnitDisplay.H"
#include "XYPlotCanvas.H"

namespace StepPlotAxes {

// "Pressure Vector" -> "Pressure": the registry names say what is stored,
// the plot says what is drawn.
inline wxString plotName(const std::string& longName)
{
  std::string s = longName;
  const std::string suffix = " Vector";
  if (s.size() > suffix.size() &&
      s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0)
    s.erase(s.size() - suffix.size());
  return wxString::FromUTF8(s.c_str());
}

inline wxString unitText(const std::string& units)
{
  return wxString::FromUTF8(unitLabel(units).c_str());
}

// The simulation time of each of `n` steps, in ps (ns when the run is
// 1000 ps or longer).  False when the run has no time series, it does not
// match the plotted series, or it never advances (a minimisation prints
// time 0 throughout): the plot then keeps the step index.
inline bool mdTime(IPropCalculation *calc, size_t n,
                   std::vector<double>& t, wxString& unit)
{
  PropVector *prop = dynamic_cast<PropVector*>(calc->getProperty("TIMEVEC"));
  if (!prop || prop->units() != "ps" || n < 2) return false;
  const std::vector<double>& v = prop->values();
  if (prop->rows() != n || v.size() < n) return false;
  t.assign(v.begin(), v.begin() + n);
  if (!(t.back() > t.front())) return false;
  unit = "ps";
  if (t.back() >= 1000.0) {
    for (size_t i = 0; i < n; i++) t[i] /= 1000.0;
    unit = "ns";
  }
  return true;
}

// The mean of y[0..i] at each i: it levels off once the run is long enough
// for the average to have converged.
inline std::vector<double> runningAverage(const std::vector<double>& y)
{
  std::vector<double> r(y.size());
  double sum = 0;
  for (size_t i = 0; i < y.size(); i++) {
    sum += y[i];
    r[i] = sum / (double)(i + 1);
  }
  return r;
}

// The thin second line of an MD plot.  The caller names curve 0.
inline void addRunningAverage(XYPlotCanvas *plot, const std::vector<double>& x,
                              const std::vector<double>& y)
{
  const int c = plot->addCurve(x, runningAverage(y), "running average", -1,
                               XYPlotCanvas::LINE);
  plot->setCurveThin(c, true);
  plot->setShowKey(true);
}

}

#endif
