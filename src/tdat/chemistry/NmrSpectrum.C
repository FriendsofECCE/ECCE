#include "tdat/NmrSpectrum.H"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

void NmrSpectrum::set(const std::vector<std::string>& elements,
                      const std::vector<double>& shieldings)
{
  if (elements.size() != shieldings.size())
    throw std::invalid_argument("Shieldings do not match the stored structure.");
  for (double v : shieldings)
    if (!std::isfinite(v)) throw std::invalid_argument("Nonfinite shielding.");
  p_elements = elements;
  p_shieldings = shieldings;
}

std::vector<std::string> NmrSpectrum::elements() const
{
  std::vector<std::string> out;
  for (const auto& e : p_elements)
    if (std::find(out.begin(), out.end(), e) == out.end()) out.push_back(e);
  std::stable_sort(out.begin(), out.end(), [](const std::string& a, const std::string& b) {
    auto rank = [](const std::string& s) { return s == "H" ? 0 : s == "C" ? 1 : 2; };
    return rank(a) == rank(b) ? a < b : rank(a) < rank(b);
  });
  return out;
}

std::optional<double> NmrSpectrum::meanShielding(const std::string& element) const
{
  double sum = 0;
  size_t n = 0;
  for (size_t i = 0; i < p_elements.size(); ++i)
    if (p_elements[i] == element) { sum += p_shieldings[i]; ++n; }
  if (!n) return std::nullopt;
  return sum / n;
}

std::vector<NmrGroup> NmrSpectrum::groups(const std::string& element, double reference) const
{
  if (!std::isfinite(reference)) throw std::invalid_argument("Nonfinite reference.");
  std::vector<std::pair<double, int>> shifts;
  for (size_t i = 0; i < p_elements.size(); ++i)
    if (p_elements[i] == element) shifts.push_back({reference - p_shieldings[i], int(i)});
  std::sort(shifts.begin(), shifts.end());
  std::vector<NmrGroup> out;
  // Compare to the first shift, so chains of near neighbours cannot merge
  // atoms whose shifts differ by more than the grouping tolerance.
  double first = 0;
  for (const auto& s : shifts) {
    if (out.empty() || s.first - first > 0.01 + 1e-12) {
      out.push_back({s.first, {s.second}});
      first = s.first;
    } else {
      auto& g = out.back();
      g.shift += (s.first - g.shift) / (g.atoms.size() + 1);
      g.atoms.push_back(s.second);
    }
  }
  for (auto& g : out) std::sort(g.atoms.begin(), g.atoms.end());
  return out;
}

VibSpectrum NmrSpectrum::spectrum(const std::string& element,
                                 std::optional<double> reference) const
{
  VibSpectrum spec;
  spec.setXUnit(X_PPM);
  spec.setLineShape(LINE_LORENTZIAN);
  spec.setFwhm(0);
  std::vector<double> shifts, counts;
  std::vector<std::string> labels;
  if (reference) for (const auto& g : groups(element, *reference)) {
    shifts.push_back(g.shift);
    counts.push_back(g.atoms.size());
    std::string label = element + " atoms";
    for (int a : g.atoms) label += " " + std::to_string(a + 1);
    labels.push_back(label);
  }
  spec.setModes(shifts, labels);
  spec.setIntensities(VIB_IR, counts, "nuclei");
  return spec;
}

std::string NmrSpectrum::csv(const std::string& element, double reference,
                            const std::string& source, double width) const
{
  auto quote = [](const std::string& s) {
    std::string q = "\"";
    for (char c : s) q += c == '"' ? "\"\"" : std::string(1, c);
    return q + "\"";
  };
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(12);
  out << "# chemical shift = reference shielding - isotropic shielding (ppm)\n"
      << "# element: " << element << "\n# reference shielding (ppm): " << reference
      << "\n# reference source: " << quote(source)
      << "\n# Lorentzian FWHM (ppm): " << width << "\n";
  out << "group,element,shift_ppm,count,atoms_1based\n";
  const auto gs = groups(element, reference);
  for (size_t i = 0; i < gs.size(); ++i) {
    out << i + 1 << ',' << element << ',' << gs[i].shift << ',' << gs[i].atoms.size() << ',';
    std::string atoms;
    for (int a : gs[i].atoms) atoms += (atoms.empty() ? "" : " ") + std::to_string(a + 1);
    out << quote(atoms) << '\n';
  }
  out << "\natom,element,shielding_ppm,shift_ppm\n";
  for (size_t i = 0; i < p_elements.size(); ++i)
    if (p_elements[i] == element)
      out << i + 1 << ',' << element << ',' << p_shieldings[i] << ','
          << reference - p_shieldings[i] << '\n';
  if (width > 0 && !gs.empty()) {
    VibSpectrum spec = spectrum(element, reference);
    spec.setFwhm(width);
    double lo, hi;
    spec.defaultRange(&lo, &hi);
    out << "\nshift_ppm,envelope_nuclei_per_ppm\n";
    for (int i = 0; i <= 2000; ++i) {
      double x = hi - (hi - lo) * i / 2000.;
      out << x << ',' << spec.envelope(VIB_IR, x) << '\n';
    }
  }
  return out.str();
}
