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

// "X", optionally with a label ("X1", "X_a"); "Xe" is xenon.
static bool is_dummy_symbol(const std::string& s) {
  if (s.empty() || (s[0] != 'X' && s[0] != 'x')) return false;
  return s.size() == 1 || !std::isalpha(static_cast<unsigned char>(s[1]));
}

Input parse_input(std::istream& in) {
  Input r;
  std::string line;
  bool in_geom = false, explicit_ref = false;
  double unit = kAngstromToBohr;
  std::string method = "hf";
  int lineno = 0;
  std::vector<std::string> geom_lines;
  bool in_basis = false, in_cons = false, in_scan = false;
  struct RawCon { std::string kind; std::vector<std::string> args; int line; };
  std::vector<RawCon> raw_cons, raw_scan;
  ShellDef* cur_shell = nullptr;
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
    if (in_cons || in_scan) {
      if (k == "end") { in_cons = in_scan = false; continue; }
      RawCon c;
      c.kind = k;
      c.line = lineno;
      std::string w;
      while (ss >> w) c.args.push_back(w);
      (in_scan ? raw_scan : raw_cons).push_back(c);
      continue;
    }
    if (in_basis) {
      if (k == "end") { in_basis = false; cur_shell = nullptr; continue; }
      if (k == "shell") {
        std::string el, letter;
        if (!(ss >> el >> letter) || letter.size() != 1) throw bad("shell needs an element and one angular momentum letter");
        const std::string ls = "spdfghi";
        auto l = ls.find(static_cast<char>(std::tolower(static_cast<unsigned char>(letter[0]))));
        if (l == std::string::npos) throw bad("unknown shell type '" + letter + "'");
        auto& v = r.explicit_basis[atomic_number(el)];
        v.emplace_back();
        cur_shell = &v.back();
        cur_shell->l = static_cast<int>(l);
        continue;
      }
      double e, c;
      std::stringstream row(line);
      if (!cur_shell || !(row >> e >> c)) throw bad("expected 'shell <El> <L>' or an 'exponent coefficient' row");
      cur_shell->exps.push_back(e);
      cur_shell->coefs.push_back(c);
      continue;
    }
    if (k == "basis_data") { in_basis = true; continue; }
    if (k == "geometry") { in_geom = true; continue; }
    if (k == "constraints") { in_cons = true; continue; }
    if (k == "scan") { in_scan = true; continue; }
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
    else if (k == "grid_prune") { std::string v = lower(rest); r.scf.grid_prune = (v == "true" || v == "yes" || v == "on" || v == "1"); }
    else if (k == "guess_mix") { std::string v = lower(rest); r.scf.guess_mix = (v == "true" || v == "yes" || v == "on" || v == "1"); }
    else if (k == "read_orbitals") r.read_orbitals = rest;
    else if (k == "write_orbitals") r.write_orbitals = rest;
    else if (k == "units") { std::string v = lower(rest); unit = (v == "bohr" || v == "au") ? 1.0 : kAngstromToBohr; }
    else if (k == "maxiter") r.scf.max_iter = std::stoi(rest);
    else if (k == "conv_energy") r.scf.conv_energy = std::stod(rest);
    else if (k == "conv_grad") r.scf.conv_grad = std::stod(rest);
    else if (k == "runtype") {
      std::string v = lower(rest);
      if (v == "energy") r.runtype = RunType::Energy;
      else if (v == "gradient") r.runtype = RunType::Gradient;
      else if (v == "optimize" || v == "opt") r.runtype = RunType::Optimize;
      else if (v == "frequency" || v == "freq") r.runtype = RunType::Frequency;
      else if (v == "optfreq") r.runtype = RunType::OptFreq;
      else if (v == "ts") r.runtype = RunType::TS;
      else if (v == "irc") r.runtype = RunType::IRC;
      else throw bad("runtype must be energy, gradient, optimize, frequency, optfreq, ts or irc");
    }
    else if (k == "hessian_step") r.hessian_step = std::stod(rest);
    else if (k == "temperature") r.temperature = std::stod(rest);
    else if (k == "pressure") r.pressure = std::stod(rest);
    else if (k == "symmetry_number") r.symmetry_number = std::stoi(rest);
    else if (k == "polarizability" || k == "polarisability" || k == "raman") {
      std::string v = lower(rest);
      const bool on = (v == "true" || v == "yes" || v == "on" || v == "1");
      (k == "raman" ? r.raman : r.polarizability) = on;
    }
    else if (k == "parallel_points") {
      const std::string v = lower(rest);
      r.parallel_points = (v == "true" || v == "yes" || v == "on" || v == "1");
    }
    else if (k == "numeric_hessian") {
      const std::string v = lower(rest);
      r.numeric_hessian = (v == "true" || v == "yes" || v == "on" || v == "1");
    }
    else if (k == "field_step") r.field_step = std::stod(rest);
    else if (k == "maxsteps") r.opt.max_steps = std::stoi(rest);
    else if (k == "irc_step") r.irc.step = std::stod(rest);
    else if (k == "irc_max_points") r.irc.max_points = std::stoi(rest);
    else if (k == "irc_hessian_every") r.irc.hessian_every = std::stoi(rest);
    else if (k == "irc_direction") {
      const std::string v = lower(rest);
      if (v == "both") r.irc_direction = 0;
      else if (v == "forward") r.irc_direction = 1;
      else if (v == "backward") r.irc_direction = -1;
      else throw bad("irc_direction must be both, forward or backward");
    }
    else if (k == "ts_hessian_every") {
      r.opt.ts_hessian_every = std::stoi(rest);
      if (r.opt.ts_hessian_every < 0) throw bad("ts_hessian_every must not be negative");
    }
    else if (k == "ts_mode") r.opt.ts_mode = std::stoi(rest);
    else if (k == "ts_follow") {
      std::stringstream cs(lower(rest));
      std::string w;
      cs >> w;
      GeoConstraint& g = r.opt.ts_follow;
      int nidx;
      if (w == "bond") { g.kind = GeoConstraint::Bond; nidx = 2; }
      else if (w == "angle") { g.kind = GeoConstraint::Angle; nidx = 3; }
      else if (w == "torsion" || w == "dihedral") { g.kind = GeoConstraint::Torsion; nidx = 4; }
      else throw bad("ts_follow: '" + w + "' is not bond, angle or torsion");
      for (int i = 0; i < nidx; ++i) {
        int a;
        if (!(cs >> a)) throw bad("ts_follow: " + w + " needs " + std::to_string(nidx) + " atom numbers");
        g.atoms[i] = a - 1;   // checked against the geometry below
      }
      r.opt.has_ts_follow = true;
    }
    else if (k == "opt_conv") r.opt.set_convergence(lower(rest));
    else if (k == "charges") {
      std::stringstream cs(lower(rest));
      std::string w;
      while (cs >> w) {
        if (w == "loewdin" || w == "lowdin") r.charges.loewdin = true;
        else if (w == "chelpg") r.charges.chelpg = true;
        else if (w == "mk" || w == "merz-kollman") r.charges.mk = true;
        else if (w == "none") r.charges = ChargeOptions();
        else throw bad("charges: unknown scheme '" + w + "' (loewdin, chelpg, mk)");
      }
    }
    else if (k == "localise" || k == "localize") {
      const std::string v = lower(rest);
      r.localise = (v == "true" || v == "yes" || v == "on" || v == "1");
    }
    else if (k == "virtuals") {
      const std::string v = lower(rest);
      if (v == "ivo") r.scf.ivo = true;
      else if (v == "canonical") r.scf.ivo = false;
      else throw bad("virtuals must be canonical or ivo");
    }
    else if (k == "molden") r.molden = true;
    else throw bad("unknown keyword '" + key + "'");
  }
  for (const auto& gl : geom_lines) {
    std::stringstream ss(gl);
    std::string sym;
    Atom a;
    if (!(ss >> sym >> a.x[0] >> a.x[1] >> a.x[2])) throw Error("bad geometry line: " + gl);
    a.Z = is_dummy_symbol(sym) ? 0 : atomic_number(sym);
    for (double& v : a.x) v *= unit;
    r.mol.atoms.push_back(a);
  }
  if (r.mol.atoms.empty()) throw Error("no geometry given");
  if (r.mol.without_dummies().atoms.empty()) throw Error("the geometry holds only dummy atoms");

  // constraints: atom numbers are 1-based and count the dummy atoms; bond
  // values are in the geometry units, angles and torsions in degrees.
  const int nat = static_cast<int>(r.mol.atoms.size());
  for (const auto& c : raw_cons) {
    auto bad = [&](const std::string& m) { return Error("input line " + std::to_string(c.line) + ": " + m); };
    GeoConstraint g;
    int nidx;
    if (c.kind == "bond") { g.kind = GeoConstraint::Bond; nidx = 2; }
    else if (c.kind == "angle") { g.kind = GeoConstraint::Angle; nidx = 3; }
    else if (c.kind == "torsion" || c.kind == "dihedral") { g.kind = GeoConstraint::Torsion; nidx = 4; }
    else if (c.kind == "fix") { g.kind = GeoConstraint::Fix; nidx = 1; }
    else throw bad("constraints: unknown entry '" + c.kind + "' (bond, angle, torsion, fix)");
    if (static_cast<int>(c.args.size()) < nidx || static_cast<int>(c.args.size()) > nidx + (g.kind == GeoConstraint::Fix ? 0 : 1))
      throw bad("constraints: '" + c.kind + "' needs " + std::to_string(nidx) + " atom numbers" +
                (g.kind == GeoConstraint::Fix ? "" : " and an optional value"));
    for (int i = 0; i < nidx; ++i) {
      int a;
      try { a = std::stoi(c.args[i]); } catch (...) { throw bad("constraints: '" + c.args[i] + "' is not an atom number"); }
      if (a < 1 || a > nat) throw bad("constraints: atom " + std::to_string(a) + " is not in the geometry");
      g.atoms[i] = a - 1;
      for (int j = 0; j < i; ++j)
        if (g.atoms[j] == g.atoms[i]) throw bad("constraints: an atom is named twice");
    }
    if (static_cast<int>(c.args.size()) > nidx) {
      double v;
      try { v = std::stod(c.args[nidx]); } catch (...) { throw bad("constraints: '" + c.args[nidx] + "' is not a number"); }
      g.has_value = true;
      g.value = g.kind == GeoConstraint::Bond ? v * unit : v * kPi / 180.0;
    }
    r.opt.constraints.push_back(g);
  }
  if (!raw_scan.empty()) {
    const RawCon& c = raw_scan[0];
    auto bad = [&](const std::string& m) { return Error("input line " + std::to_string(c.line) + ": " + m); };
    if (raw_scan.size() > 1) throw bad("scan: one coordinate only");
    GeoConstraint& g = r.scan.constraint;
    int nidx;
    if (c.kind == "bond") { g.kind = GeoConstraint::Bond; nidx = 2; }
    else if (c.kind == "angle") { g.kind = GeoConstraint::Angle; nidx = 3; }
    else if (c.kind == "torsion" || c.kind == "dihedral") { g.kind = GeoConstraint::Torsion; nidx = 4; }
    else throw bad("scan: unknown coordinate '" + c.kind + "' (bond, angle, torsion)");
    if (static_cast<int>(c.args.size()) != nidx + 3)
      throw bad("scan: '" + c.kind + "' needs " + std::to_string(nidx) + " atom numbers, from, to and the number of points");
    for (int i = 0; i < nidx; ++i) {
      int a;
      try { a = std::stoi(c.args[i]); } catch (...) { throw bad("scan: '" + c.args[i] + "' is not an atom number"); }
      if (a < 1 || a > nat) throw bad("scan: atom " + std::to_string(a) + " is not in the geometry");
      g.atoms[i] = a - 1;
      for (int j = 0; j < i; ++j)
        if (g.atoms[j] == g.atoms[i]) throw bad("scan: an atom is named twice");
    }
    double v0, v1;
    try {
      v0 = std::stod(c.args[nidx]);
      v1 = std::stod(c.args[nidx + 1]);
      r.scan.points = std::stoi(c.args[nidx + 2]);
    } catch (...) { throw bad("scan: from, to and the number of points must be numbers"); }
    if (r.scan.points < 1) throw bad("scan: needs at least one point");
    const double f = g.kind == GeoConstraint::Bond ? unit : kPi / 180.0;
    r.scan.start = v0 * f;
    r.scan.end = v1 * f;
    g.has_value = true;
    g.value = r.scan.start;
    r.scan.on = true;
    if (r.runtype != RunType::Optimize) throw bad("scan needs runtype optimize");
  }
  if (r.raman && !does_frequencies(r.runtype)) throw Error("raman needs runtype frequency or optfreq");
  if (r.field_step <= 0) throw Error("field_step must be positive");
  if (r.runtype == RunType::TS && (!r.opt.constraints.empty() || r.mol.without_dummies().atoms.size() != r.mol.atoms.size()))
    throw Error("runtype ts takes no constraints and no dummy atoms");
  if (r.runtype == RunType::IRC) {
    if (!r.opt.constraints.empty() || r.mol.without_dummies().atoms.size() != r.mol.atoms.size())
      throw Error("runtype irc takes no constraints and no dummy atoms");
    if (r.irc.step <= 0 || r.irc.max_points < 1 || r.irc.hessian_every < 0)
      throw Error("irc_step must be positive, irc_max_points at least 1 and irc_hessian_every not negative");
  }
  if (r.opt.ts_mode < 0) throw Error("ts_mode must be 1 or more");
  if (r.opt.has_ts_follow) {
    const int nidx = r.opt.ts_follow.kind == GeoConstraint::Bond ? 2 : r.opt.ts_follow.kind == GeoConstraint::Angle ? 3 : 4;
    for (int i = 0; i < nidx; ++i) {
      const int a = r.opt.ts_follow.atoms[i];
      if (a < 0 || a >= static_cast<int>(r.mol.atoms.size())) throw Error("ts_follow: atom " + std::to_string(a + 1) + " is not in the geometry");
      for (int j = 0; j < i; ++j)
        if (r.opt.ts_follow.atoms[j] == a) throw Error("ts_follow: an atom is named twice");
    }
  }
  if (does_frequencies(r.runtype) && does_optimize(r.runtype) && !r.opt.constraints.empty())
    throw Error("optfreq with constraints: frequencies at a constrained geometry are not at a minimum; "
                "run runtype optimize with the constraints and a separate frequency run without them");

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
