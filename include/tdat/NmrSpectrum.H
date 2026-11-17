#ifndef ECCE_NMR_SPECTRUM_H
#define ECCE_NMR_SPECTRUM_H

#include <optional>
#include "tdat/VibSpectrum.H"

struct NmrGroup {
  double shift;
  std::vector<int> atoms; // zero-based indices in the stored structure
};

/** Isotropic shieldings and references in ppm; no isotope or coupling model. */
class NmrSpectrum {
public:
  void set(const std::vector<std::string>& elements,
           const std::vector<double>& shieldings);
  std::vector<std::string> elements() const;
  std::optional<double> meanShielding(const std::string& element) const;
  std::vector<NmrGroup> groups(const std::string& element, double reference) const;
  VibSpectrum spectrum(const std::string& element,
                       std::optional<double> reference) const;
  std::string csv(const std::string& element, double reference,
                  const std::string& source, double width) const;
private:
  std::vector<std::string> p_elements;
  std::vector<double> p_shieldings;
};
#endif
