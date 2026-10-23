#include <algorithm>
#include <cctype>
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

int ExcitedStates::orbitalNumber(const string& s, char* spin)
{
  if (spin) *spin = 0;
  const int n = atoi(s.c_str());
  if (spin && !s.empty() && isalpha((unsigned char)s[s.size() - 1]))
    *spin = (char)tolower((unsigned char)s[s.size() - 1]);
  return n;
}

string ExcitedStates::orbitalLabel(int mo, int homo)
{
  char b[32];
  if (mo <= 0 || homo <= 0) {
    snprintf(b, sizeof b, "MO %d", mo);
  } else if (mo <= homo) {
    if (mo == homo) return "HOMO";
    snprintf(b, sizeof b, "HOMO-%d", homo - mo);
  } else {
    if (mo == homo + 1) return "LUMO";
    snprintf(b, sizeof b, "LUMO+%d", mo - homo - 1);
  }
  return b;
}

int ExcitedStates::homo(char spin) const
{
  if (spin == 'b' && p_homoB > 0) return p_homoB;
  if (p_homoA > 0) return p_homoA;
  int h = 0;
  for (size_t i = 0; i < p_states.size(); i++)
    for (size_t k = 0; k < p_states[i].transitions.size(); k++)
      h = std::max(h, orbitalNumber(p_states[i].transitions[k].from));
  return h;
}

double ExcitedStates::weightPercent(const ExcitedTransition& t,
                                    const string& stateLabel)
{
  char spin = 0;
  orbitalNumber(t.from, &spin);
  const bool restricted = spin == 0 &&
      (stateLabel.compare(0, 7, "Singlet") == 0 ||
       stateLabel.compare(0, 7, "Triplet") == 0);
  return 100.0 * (restricted ? 2.0 : 1.0) * t.coefficient * t.coefficient;
}

vector<string> ExcitedStates::transitionLabels(int i) const
{
  vector<ExcitedTransition> t = p_states[i].transitions;
  std::stable_sort(t.begin(), t.end(),
                   [](const ExcitedTransition& a, const ExcitedTransition& b) {
                     return std::fabs(a.coefficient) > std::fabs(b.coefficient);
                   });
  vector<string> out;
  for (size_t k = 0; k < t.size(); k++) {
    char sa = 0, sb = 0;
    const int from = orbitalNumber(t[k].from, &sa);
    const int to = orbitalNumber(t[k].to, &sb);
    char b[160];
    snprintf(b, sizeof b, "%s -> %s   %.1f %%   [MO %d -> %d]",
             orbitalLabel(from, homo(sa)).c_str(),
             orbitalLabel(to, homo(sb)).c_str(),
             weightPercent(t[k], p_states[i].label), from, to);
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
