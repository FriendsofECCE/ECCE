//  Writes the JCAMP-DX and CSV exports of a model spectrum (#986) for
//  tests/spectrum/jcamp_tests.py, which reads them back.
//      jcamp_export_test <outdir>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "tdat/VibSpectrum.H"

int main(int argc, char **argv)
{
  if (argc < 2) return 2;
  const std::string dir = argv[1];
  VibSpectrum s;
  std::vector<double> f = {2169.8577, 4141.5426, 4392.6459};
  std::vector<double> ir = {7.2460, 44.2829, 29.9543};
  std::vector<std::string> sym = {"A'", "A'", "A'"};
  s.setModes(f, sym);
  s.setIntensities(VIB_IR, ir, "KM/Mole");
  s.setFwhm(15);
  s.setScale(0.96);
  SpectrumCsvInfo info;
  info.calculation = "water";
  info.code = "Gaussian 16";
  auto put = [&](const char *name, const std::string& t) {
    std::ofstream(dir + "/" + name) << t;
  };
  put("curve.jdx", vibSpectrumJcamp(s, VIB_IR, info, 1500, 4500, 1.0));
  put("curve.csv", vibSpectrumCsv(s, VIB_IR, info, 1500, 4500, 1.0));
  s.setFwhm(0);
  put("sticks.jdx", vibSpectrumJcamp(s, VIB_IR, info, 1500, 4500, 1.0));
  return 0;
}
