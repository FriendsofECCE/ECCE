#include "input.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace qm {

static std::string lower(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

Input parse_input(std::istream& in) {
  Input r;
  std::string line;
  bool in_geom = false, explicit_ref = false;
  double unit = kAngstromToBohr;
  std::string method = "hf";
  int lineno = 0;
  std::vector<std::string> geom_lines;
  while (std::getline(in, line)) {
    ++lineno;
    auto h = line.find('#');
    if (h != std::string::npos) line.erase(h);
    std::stringstream ss(line);
    std::string key;
    if (!(ss >> key)) continue;
    std::string k = lower(key);
    auto bad = [&](const std::string& m) { return Error("input line " + std::to_string(lineno) + ": " + m); };
    if (in_geom) {
      if (k == "end") { in_geom = false; continue; }
      geom_lines.push_back(line);
      continue;
    }
    if (k == "geometry") { in_geom = true; continue; }
    std::string rest;
    std::getline(ss, rest);
    rest.erase(0, rest.find_first_not_of(" \t"));
    rest.erase(rest.find_last_not_of(" \t\r\n") + 1);
    if (k == "title") r.title = rest;
    else if (k == "charge") r.mol.charge = std::stoi(rest);
    else if (k == "multiplicity") r.mol.multiplicity = std::stoi(rest);
    else if (k == "method") method = lower(rest);
    else if (k == "basis") r.basis = rest;
    else if (k == "spherical") { std::string v = lower(rest); r.spherical = (v == "true" || v == "yes" || v == "1"); }
    else if (k == "grid") { std::stringstream g(rest); if (!(g >> r.scf.grid_radial >> r.scf.grid_angular)) throw bad("grid needs two integers"); }
    else if (k == "units") { std::string v = lower(rest); unit = (v == "bohr" || v == "au") ? 1.0 : kAngstromToBohr; }
    else if (k == "maxiter") r.scf.max_iter = std::stoi(rest);
    else if (k == "conv_energy") r.scf.conv_energy = std::stod(rest);
    else if (k == "conv_grad") r.scf.conv_grad = std::stod(rest);
    else if (k == "molden") r.molden = true;
    else throw bad("unknown keyword '" + key + "'");
  }
  for (const auto& gl : geom_lines) {
    std::stringstream ss(gl);
    std::string sym;
    Atom a;
    if (!(ss >> sym >> a.x[0] >> a.x[1] >> a.x[2])) throw Error("bad geometry line: " + gl);
    a.Z = atomic_number(sym);
    for (double& v : a.x) v *= unit;
    r.mol.atoms.push_back(a);
  }
  if (r.mol.atoms.empty()) throw Error("no geometry given");

  // method prefix
  std::string m = method;
  Reference ref = r.mol.multiplicity == 1 ? Reference::Restricted : Reference::RestrictedOpen;
  if (m.compare(0, 2, "ro") == 0 && m != "ro") { ref = Reference::RestrictedOpen; m = m.substr(2); explicit_ref = true; }
  else if (m[0] == 'u') { ref = Reference::Unrestricted; m = m.substr(1); explicit_ref = true; }
  else if (m[0] == 'r' && m != "r") { ref = Reference::Restricted; m = m.substr(1); explicit_ref = true; }
  (void)explicit_ref;
  r.scf.method = m;
  r.scf.reference = ref;
  functional_by_name(m);  // validate
  return r;
}

Input parse_input_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw Error("cannot open input file " + path);
  return parse_input(f);
}

}  // namespace qm
