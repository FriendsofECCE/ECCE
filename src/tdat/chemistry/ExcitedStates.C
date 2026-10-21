#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <locale>
#include <sstream>

#include "tdat/ExcitedStates.H"

using std::string;
using std::vector;

double ExcitedStates::nm(double ev)
{
  return ev > 0 ? VibSpectrum::evNm() / ev : 0.0;
}

vector<ExcitedTransition> ExcitedStates::parseTransitions(const string& text)
{
  vector<ExcitedTransition> out;
  size_t at = 0;
  while (at < text.size()) {
    size_t end = text.find(',', at);
    if (end == string::npos) end = text.size();
    const string item = text.substr(at, end - at);
    at = end + 1;
    const size_t gt = item.find('>'), colon = item.find(':');
    if (gt == string::npos || colon == string::npos || colon < gt) continue;
    ExcitedTransition t;
    t.from = item.substr(0, gt);
    t.to = item.substr(gt + 1, colon - gt - 1);
    t.coefficient = atof(item.c_str() + colon + 1);
    out.push_back(t);
  }
  return out;
}

void ExcitedStates::set(const vector<double>& ev, const vector<double>& f,
                        const vector<string>& labels,
                        const vector<string>& transitions)
{
  p_states.clear();
  for (size_t i = 0; i < ev.size(); i++) {
    ExcitedState s;
    s.ev = ev[i];
    s.f = i < f.size() ? f[i] : 0.0;
    s.label = i < labels.size() && labels[i] != "-" ? labels[i] : string();
    if (i < transitions.size()) s.transitions = parseTransitions(transitions[i]);
    p_states.push_back(s);
  }
}

vector<string> ExcitedStates::transitionLines(int i) const
{
  vector<ExcitedTransition> t = p_states[i].transitions;
  std::stable_sort(t.begin(), t.end(),
                   [](const ExcitedTransition& a, const ExcitedTransition& b) {
                     return std::fabs(a.coefficient) > std::fabs(b.coefficient);
                   });
  vector<string> out;
  for (size_t k = 0; k < t.size(); k++) {
    char b[96];
    snprintf(b, sizeof b, "%s -> %s   %.3f", t[k].from.c_str(),
             t[k].to.c_str(), t[k].coefficient);
    out.push_back(b);
  }
  return out;
}

VibSpectrum ExcitedStates::spectrum(SpectrumXUnit unit) const
{
  VibSpectrum s;
  s.setXUnit(unit);
  vector<double> ev, f;
  vector<string> labels, notes;
  for (size_t i = 0; i < p_states.size(); i++) {
    ev.push_back(p_states[i].ev);
    f.push_back(p_states[i].f);
    labels.push_back(p_states[i].label);
    string n;
    for (size_t k = 0; k < p_states[i].transitions.size(); k++) {
      char b[96];
      snprintf(b, sizeof b, "%s%s>%s:%.10g", k ? ";" : "",
               p_states[i].transitions[k].from.c_str(),
               p_states[i].transitions[k].to.c_str(),
               p_states[i].transitions[k].coefficient);
      n += b;
    }
    notes.push_back(n);
  }
  s.setModes(ev, labels);
  s.setIntensities(VIB_IR, f, "f");
  s.setNotes(notes);
  s.setFwhm(0.3);
  return s;
}

string excitedSpectrumCsv(const ExcitedStates& states, const VibSpectrum& s,
                          const SpectrumCsvInfo& info,
                          double lo, double hi, double step)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(10);
  const bool curve = s.fwhm() > 0 && step > 0 && hi > lo;
  const bool nmAxis = s.xUnit() == X_NM;

  out << "# ECCE UV-vis spectrum (excited states)\n";
  out << "# calculation: " << info.calculation << "\n";
  out << "# code: " << info.code << "\n";
  out << "# broadening: "
      << (curve ? (s.lineShape() == LINE_LORENTZIAN ? "Lorentzian" : "Gaussian")
                : "none")
      << ", FWHM " << s.fwhm() << " eV\n";
  out << "# the curve is oscillator strength per eV, evaluated in energy "
         "and written against "
      << (nmAxis ? "wavelength" : "energy") << "\n";
  out << "# transitions are from>to:coefficient, orbitals numbered from 1\n";
  out << "# states\n";
  out << "state,energy_eV,wavelength_nm,oscillator_strength,label,"
         "transitions\n";
  for (int i = 0; i < states.count(); i++) {
    const ExcitedState& st = states.state(i);
    out << i + 1 << "," << st.ev << "," << ExcitedStates::nm(st.ev) << ","
        << st.f << "," << st.label << "," << s.note(i) << "\n";
  }

  if (curve) {
    out << "\n# broadened curve, " << step << (nmAxis ? " nm" : " eV")
        << " grid from " << lo << " to " << hi << "\n";
    out << (nmAxis ? "wavelength_nm" : "energy_eV") << ",f_per_eV\n";
    const long n = (long)std::floor((hi - lo) / step + 1e-9);
    for (long i = 0; i <= n; i++) {
      const double x = lo + i * step;
      out << x << "," << s.envelope(VIB_IR, x) << "\n";
    }
  }
  return out.str();
}
