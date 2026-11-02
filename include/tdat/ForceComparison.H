#ifndef FORCECOMPARISON_H
#define FORCECOMPARISON_H

#include <string>
#include <vector>
#include "tdat/EnergySums.H"

struct ForceInput {
  std::vector<std::string> elements;
  std::vector<double> coordinates; // Angstrom, in the stored frame
  std::vector<double> gradient;
  std::string units, problem;
  EnergyTerm energy;
};

struct ForceRow {
  std::string element;
  size_t count = 0;
  double maximum = 0, rms = 0; // eV/Angstrom
};

struct ForceComparison {
  bool ok = false, hasEnergy = false;
  double geometryRms = 0, deltaEV = 0, deltaMeVPerAtom = 0;
  std::string problem;
  std::vector<std::string> notes;
  std::vector<ForceRow> rows;
};

// Gradients are converted explicitly to forces before taking vector differences.
ForceComparison compareForces(const ForceInput& a, const ForceInput& b);
std::string forceComparisonText(const ForceComparison& result);
std::string forceComparisonJson(const ForceComparison& result);

#endif
