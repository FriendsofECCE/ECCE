// D3(BJ) checks (#453).
//   qm_d3_test <fixtures.txt>
// 1. The analytic gradient against central differences of the energy, for
//    every fixture geometry and method (and a distorted copy, so that the
//    coordination-number chain rule is exercised away from symmetry).
// 2. Energy and gradient against the ORCA 6.1.1 values in the fixture file
//    (tests/qm/d3bj_orca.txt, made by gen_d3bj_fixture.py from the outputs
//    in tests/qm/d3bj_orca/).
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "common.hpp"
#include "d3.hpp"
#include "molecule.hpp"

namespace {

struct Case {
  std::string name, method;
  qm::Molecule mol;
  double energy = 0;
  std::vector<double> grad;
};

std::vector<Case> read_fixtures(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw qm::Error("cannot read " + path);
  std::vector<Case> out;
  std::string line;
  while (std::getline(f, line)) {
    std::istringstream ss(line);
    std::string k;
    if (!(ss >> k) || k[0] == '#') continue;
    if (k == "case") {
      out.emplace_back();
      ss >> out.back().name >> out.back().method;
    } else if (k == "energy") {
      ss >> out.back().energy;
    } else if (k == "atom") {
      std::string sym;
      qm::Atom a;
      double g[3];
      ss >> sym >> a.x[0] >> a.x[1] >> a.x[2] >> g[0] >> g[1] >> g[2];
      a.Z = qm::atomic_number(sym);
      for (double& v : a.x) v *= qm::kAngstromToBohr;
      out.back().mol.atoms.push_back(a);
      out.back().grad.insert(out.back().grad.end(), g, g + 3);
    }
  }
  return out;
}

double max_fd_error(const qm::Molecule& mol, const qm::D3Params& p) {
  std::vector<double> g(3 * mol.atoms.size(), 0.0);
  qm::d3bj_energy(mol, p, &g);
  const double h = 1e-4;
  double worst = 0;
  for (size_t i = 0; i < mol.atoms.size(); ++i)
    for (int c = 0; c < 3; ++c) {
      qm::Molecule m = mol;
      m.atoms[i].x[c] += h;
      const double ep = qm::d3bj_energy(m, p);
      m.atoms[i].x[c] -= 2 * h;
      const double em = qm::d3bj_energy(m, p);
      worst = std::max(worst, std::fabs((ep - em) / (2 * h) - g[3 * i + c]));
    }
  return worst;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: qm_d3_test fixtures.txt\n";
    return 2;
  }
  int fails = 0;
  const auto cases = read_fixtures(argv[1]);
  if (cases.empty()) {
    std::cerr << "no fixtures\n";
    return 1;
  }
  std::printf("%-12s %-6s %18s %18s %10s %10s %10s\n", "molecule", "method", "E(ECCE-QM)", "E(ORCA)", "dE", "max|dg|", "fd err");
  for (const Case& c : cases) {
    qm::D3Params p;
    if (!qm::d3bj_params(c.method, p)) {
      std::cerr << "no parameters for " << c.method << "\n";
      return 1;
    }
    std::vector<double> g(3 * c.mol.atoms.size(), 0.0);
    const double e = qm::d3bj_energy(c.mol, p, &g);
    double dg = 0;
    for (size_t k = 0; k < g.size(); ++k) dg = std::max(dg, std::fabs(g[k] - c.grad[k]));
    // A distorted copy: off-equilibrium CNs and no symmetry.
    qm::Molecule d = c.mol;
    for (size_t i = 0; i < d.atoms.size(); ++i)
      for (int k = 0; k < 3; ++k) d.atoms[i].x[k] += 0.07 * std::sin(1.3 * i + 2.1 * k + 0.4);
    const double fd = std::max(max_fd_error(c.mol, p), max_fd_error(d, p));
    const bool ok = std::fabs(e - c.energy) < 1e-8 && dg < 1e-7 && fd < 1e-8;
    std::printf("%-12s %-6s %18.12f %18.12f %10.2e %10.2e %10.2e %s\n", c.name.c_str(), c.method.c_str(), e, c.energy,
                e - c.energy, dg, fd, ok ? "ok" : "FAIL");
    if (!ok) ++fails;
  }
  // Elements without reference data are refused, not silently skipped.
  try {
    qm::Molecule m;
    qm::Atom a;
    a.Z = 104;
    m.atoms.push_back(a);
    qm::D3Params p;
    qm::d3bj_params("pbe", p);
    qm::d3bj_energy(m, p);
    std::printf("Z = 104 was not refused: FAIL\n");
    ++fails;
  } catch (const qm::Error&) {
  }
  std::printf("%s\n", fails ? "FAILED" : "all passed");
  return fails ? 1 : 0;
}
