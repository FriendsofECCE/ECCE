#ifndef STEPPLOTAXES_HH
#define STEPPLOTAXES_HH

// What the per-step plots (Geometry Trace, Geometry Step Plots) share: the
// name drawn as a title, the unit text, and the time axis of an MD run.

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <wx/string.h>

#include "dsm/IPropCalculation.H"
#include "tdat/PropVector.H"
#include "util/UnitDisplay.H"
#include "MovingAverage.H"
#include "XYPlotCanvas.H"

namespace StepPlotAxes {

// "Pressure Vector" -> "Pressure": the registry names say what is stored,
// the plot says what is drawn.
inline wxString plotName(const std::string& longName)
{
  std::string s = longName;
  // Registry names that are codes or shorthand, not what the plot shows.
  if (s == "Delta E") return "Energy change";
  if (s == "RMSDP") return "RMS density change";
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

// The x axis name a user reads for a stored row label.
inline wxString xAxisName(const std::string& rowLabel)
{
  if (rowLabel == "Wave Step") return "SCF cycle";
  if (rowLabel == "Geometry Step") return "Optimisation step";
  return wxString::FromUTF8(rowLabel.c_str());
}

// What an untimed per-step series is drawn as.
struct Shaped {
  std::vector<double> y;
  wxString yLabel;                // axis title with its unit
  wxString yName, yUnit;          // hover readout
  wxString titleNote;             // appended to the plot name, may be empty
  std::vector<wxString> hover;    // per point, may be empty
  bool log = false;
};

static const double kJPerMolPerHartree = 2625.4996394799;

// Convergence quantities (the energy change, the density change, the
// orbital gradient) span decades, so they are drawn as log10 of their size,
// as the Progress box does; a total energy is drawn relative to its first
// step in kJ/mol, since the absolute value is the same to six digits all
// along an optimisation.  Anything else keeps its values and unit.
inline Shaped shape(const std::string& key, const std::string& units,
                    const std::vector<double>& v)
{
  Shaped r;
  r.y = v;
  r.yUnit = unitText(units);
  r.yLabel = r.yUnit;
  if (key == "DELTAE" || key == "RMSDP" || key == "DEWVEC") {
    r.log = true;
    for (size_t i = 0; i < v.size(); i++)
      r.y[i] = v[i] != 0.0 ? std::log10(std::fabs(v[i]))
                           : std::numeric_limits<double>::quiet_NaN();
    const wxString what = key == "DELTAE"
        ? wxString::FromUTF8("log10 |\xce\x94""E|")
        : key == "RMSDP" ? wxString("log10 RMS density change")
                         : wxString("log10 orbital gradient");
    r.yName = what;
    r.yUnit = "";
    r.yLabel = what + (units == "Hartree" && key == "DELTAE" ? " (Hartree)" : "");
    r.hover.resize(v.size());
    for (size_t i = 0; i < v.size(); i++)
      r.hover[i] = wxString::Format("%.3e", v[i]);
  } else if ((key == "TEVEC" || key == "ESCFVEC" || key == "TECPVEC") &&
             units == "Hartree" && v.size() > 1) {
    for (size_t i = 0; i < v.size(); i++)
      r.y[i] = (v[i] - v[0]) * kJPerMolPerHartree;
    r.yName = "Energy relative to step 1";
    r.yUnit = "kJ/mol";
    r.yLabel = "Energy relative to step 1 (kJ/mol)";
    r.titleNote = wxString::Format("final %.8f Hartree", v.back());
    r.hover.resize(v.size());
    for (size_t i = 0; i < v.size(); i++)
      r.hover[i] = wxString::Format("%.8f Hartree", v[i]);
  }
  return r;
}

// The thin second line of an MD plot: a moving average over 10 % of the
// points, with the window named in the legend (in time when x is time).
inline void addMovingAverage(XYPlotCanvas *plot, const std::vector<double>& x,
                             const std::vector<double>& y,
                             const wxString& timeUnit)
{
  const size_t w = MovingAverage::windowPoints(y.size());
  wxString name;
  if (!timeUnit.empty() && x.size() > 1)
    name = wxString::Format("moving average (%.3g %s window)",
                            w * (x.back() - x.front()) / (x.size() - 1),
                            timeUnit);
  else
    name = wxString::Format("moving average (%d points)", (int)w);
  const int c = plot->addCurve(x, MovingAverage::centred(y, w), name, -1,
                               XYPlotCanvas::LINE);
  plot->setCurveThin(c, true);
  plot->setShowKey(true);
}

}

#endif
