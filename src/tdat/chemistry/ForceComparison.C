#include "tdat/ForceComparison.H"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>

namespace {
bool factor(const std::string& units, double& f)
{
  if (units == "Hartree/Bohr") f = -51.42208619;
  else if (units == "eV/Angstrom") f = -1;
  else return false;
  return true;
}

std::string quoted(const std::string& s)
{
  std::ostringstream out;
  out << '"';
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') out << '\\' << c;
    else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec;
    else out << c;
  }
  out << '"';
  return out.str();
}
}

ForceComparison compareForces(const ForceInput& a, const ForceInput& b)
{
  ForceComparison r;
  if (!a.problem.empty() || !b.problem.empty()) {
    r.problem = !a.problem.empty() ? "A: " + a.problem : "B: " + b.problem;
    return r;
  }
  const size_t n = a.elements.size();
  if (!n || a.elements != b.elements) {
    r.problem = "Same atoms required: atom counts and element sequences must match.";
    return r;
  }
  if (a.gradient.size() != 3*n || b.gradient.size() != 3*n ||
      a.coordinates.size() != 3*n || b.coordinates.size() != 3*n) {
    r.problem = "The gradient or final geometry does not contain three coordinates per atom.";
    return r;
  }
  double fa, fb;
  if (!factor(a.units, fa) || !factor(b.units, fb)) {
    r.problem = "Unsupported gradient units: A=" + a.units + ", B=" + b.units + ".";
    return r;
  }
  std::map<std::string, ForceRow> elements;
  ForceRow all;
  all.element = "all atoms";
  double geometrySum = 0;
  for (size_t i = 0; i < n; ++i) {
    double d2 = 0;
    for (size_t k = 3*i; k < 3*i+3; ++k) {
      if (!std::isfinite(a.gradient[k]) || !std::isfinite(b.gradient[k]) ||
          !std::isfinite(a.coordinates[k]) || !std::isfinite(b.coordinates[k])) {
        r.problem = "The gradient or geometry contains a non-finite value.";
        return r;
      }
      const double d = fb*b.gradient[k] - fa*a.gradient[k];
      d2 += d*d;
      const double x = b.coordinates[k] - a.coordinates[k];
      geometrySum += x*x;
    }
    ForceRow& el = elements[a.elements[i]];
    el.element = a.elements[i];
    for (ForceRow *row : {&el, &all}) {
      ++row->count;
      row->maximum = std::max(row->maximum, std::sqrt(d2));
      row->rms += d2;
    }
  }
  for (auto& item : elements) {
    item.second.rms = std::sqrt(item.second.rms / item.second.count);
    r.rows.push_back(item.second);
  }
  all.rms = std::sqrt(all.rms / n);
  r.rows.push_back(all);
  r.geometryRms = std::sqrt(geometrySum / n);
  if (r.geometryRms > 0.01)
    r.notes.push_back("Warning: final geometries differ by more than 0.01 Å RMS; forces of different geometries are not a convergence test.");
  if (a.energy.code != b.energy.code || a.energy.state.method != b.energy.state.method ||
      a.energy.xc != b.energy.xc || a.energy.dispersion != b.energy.dispersion) {
    r.notes.push_back("No energy difference: the two calculations use different methods.");
  } else if (a.energy.code.empty() || a.energy.state.method.empty()) {
    r.notes.push_back("No energy difference: the stored code or theory is missing.");
  } else if (!a.energy.state.problem.empty() || !b.energy.state.problem.empty() ||
             a.energy.heat || b.energy.heat ||
             !std::isfinite(a.energy.state.energy) || !std::isfinite(b.energy.state.energy)) {
    r.notes.push_back("No energy difference: both calculations need a final total energy in convertible units.");
  } else {
    r.hasEnergy = true;
    r.deltaEV = (b.energy.state.energy - a.energy.state.energy) * 27.211386245988;
    r.deltaMeVPerAtom = r.deltaEV * 1000 / n;
  }
  r.ok = true;
  return r;
}

std::string forceComparisonText(const ForceComparison& r)
{
  if (!r.ok) return r.problem + "\n";
  std::ostringstream out;
  out << std::setprecision(12)
      << "Element\tn atoms\tmax |Delta F| (eV/Angstrom)\trms |Delta F| (eV/Angstrom)\n";
  for (const ForceRow& row : r.rows)
    out << row.element << '\t' << row.count << '\t' << row.maximum << '\t' << row.rms << '\n';
  if (r.hasEnergy)
    out << "Delta E = E(B) - E(A)\t" << r.deltaEV << " eV/cell\t"
        << r.deltaMeVPerAtom << " meV/atom\n";
  for (const auto& note : r.notes) out << note << '\n';
  return out.str();
}

std::string forceComparisonJson(const ForceComparison& r)
{
  std::ostringstream out;
  out << std::setprecision(17) << "{\"ok\":" << (r.ok ? "true" : "false")
      << ",\"problem\":" << quoted(r.problem) << ",\"force_units\":\"eV/Angstrom\",\"rows\":[";
  for (size_t i = 0; i < r.rows.size(); ++i) {
    const ForceRow& row = r.rows[i];
    if (i) out << ',';
    out << "{\"element\":" << quoted(row.element) << ",\"n\":" << row.count
        << ",\"max\":" << row.maximum << ",\"rms\":" << row.rms << '}';
  }
  out << "],\"geometry_rms_angstrom\":" << r.geometryRms << ",\"delta_e_ev\":";
  if (r.hasEnergy) out << r.deltaEV; else out << "null";
  out << ",\"delta_e_mev_per_atom\":";
  if (r.hasEnergy) out << r.deltaMeVPerAtom; else out << "null";
  out << ",\"notes\":[";
  for (size_t i = 0; i < r.notes.size(); ++i) {
    if (i) out << ',';
    out << quoted(r.notes[i]);
  }
  out << "]}\n";
  return out.str();
}
