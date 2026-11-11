#include "mlpdriver.hpp"

#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace mlp {

namespace {

const double kB = 8.617330337217213e-05;            // eV/K
const double FS = 0.09822694788464063;              // 1 fs in A*sqrt(amu/eV)
const double TO_CM = 521.4709004010551;             // sqrt(eV/A^2/amu) to cm-1

const char* SYMBOLS =
    "X H He Li Be B C N O F Ne Na Mg Al Si P S Cl Ar K Ca Sc Ti V Cr Mn Fe Co Ni Cu Zn "
    "Ga Ge As Se Br Kr Rb Sr Y Zr Nb Mo Tc Ru Rh Pd Ag Cd In Sn Sb Te I Xe Cs Ba La Ce Pr "
    "Nd Pm Sm Eu Gd Tb Dy Ho Er Tm Yb Lu Hf Ta W Re Os Ir Pt Au Hg Tl Pb Bi Po At Rn Fr "
    "Ra Ac Th Pa U Np Pu Am Cm Bk Cf Es Fm Md No Lr Rf Db Sg Bh Hs Mt Ds Rg Cn Nh Fl Mc "
    "Lv Ts Og";
// ase.data.atomic_masses, so vibrations agree with ASE's.
const double MASSES[] = {
    1, 1.008, 4.002602, 6.94, 9.0121831, 10.81, 12.011, 14.007, 15.999, 18.998403,
    20.1797, 22.989769, 24.305, 26.981538, 28.085, 30.973762, 32.06, 35.45, 39.948,
    39.0983, 40.078, 44.955908, 47.867, 50.9415, 51.9961, 54.938044, 55.845, 58.933194,
    58.6934, 63.546, 65.38, 69.723, 72.63, 74.921595, 78.971, 79.904, 83.798, 85.4678,
    87.62, 88.90584, 91.224, 92.90637, 95.95, 97.90721, 101.07, 102.9055, 106.42,
    107.8682, 112.414, 114.818, 118.71, 121.76, 127.6, 126.90447, 131.293, 132.90545,
    137.327, 138.90547, 140.116, 140.90766, 144.242, 144.91276, 150.36, 151.964, 157.25,
    158.92535, 162.5, 164.93033, 167.259, 168.93422, 173.054, 174.9668, 178.49,
    180.94788, 183.84, 186.207, 190.23, 192.217, 195.084, 196.96657, 200.592, 204.38,
    207.2, 208.9804, 208.98243, 209.98715, 222.01758, 223.01974, 226.02541, 227.02775,
    232.0377, 231.03588, 238.02891, 237.04817, 244.06421, 243.06138, 247.07035,
    247.07031, 251.07959, 252.083, 257.09511, 258.09843, 259.101, 262.11, 267.122,
    268.126, 271.134, 270.133, 269.1338, 278.156, 281.165, 281.166, 285.177, 286.182,
    289.19, 289.194, 293.204, 293.208, 294.214};

Mat3 mul(const Mat3& a, const Mat3& b) {
  Mat3 c{};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k) c[3 * i + j] += a[3 * i + k] * b[3 * k + j];
  return c;
}
Mat3 transpose(const Mat3& a) {
  Mat3 t{};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) t[3 * i + j] = a[3 * j + i];
  return t;
}
double det(const Mat3& a) {
  return a[0] * (a[4] * a[8] - a[5] * a[7]) - a[1] * (a[3] * a[8] - a[5] * a[6]) +
         a[2] * (a[3] * a[7] - a[4] * a[6]);
}
Mat3 inverse(const Mat3& a) {
  double d = det(a);
  if (std::fabs(d) < 1e-14) throw std::runtime_error("singular 3x3 matrix");
  Mat3 r{};
  r[0] = (a[4] * a[8] - a[5] * a[7]) / d;
  r[1] = (a[2] * a[7] - a[1] * a[8]) / d;
  r[2] = (a[1] * a[5] - a[2] * a[4]) / d;
  r[3] = (a[5] * a[6] - a[3] * a[8]) / d;
  r[4] = (a[0] * a[8] - a[2] * a[6]) / d;
  r[5] = (a[2] * a[3] - a[0] * a[5]) / d;
  r[6] = (a[3] * a[7] - a[4] * a[6]) / d;
  r[7] = (a[1] * a[6] - a[0] * a[7]) / d;
  r[8] = (a[0] * a[4] - a[1] * a[3]) / d;
  return r;
}
Mat3 identity() { return Mat3{1, 0, 0, 0, 1, 0, 0, 0, 1}; }

// exp of an n x n matrix: scaling and squaring with a Taylor series; the
// matrices here are small (3x3 and the 6x6 Frechet blocks).
std::vector<double> expm(int n, std::vector<double> a) {
  double norm = 0;
  for (int i = 0; i < n; ++i) {
    double row = 0;
    for (int j = 0; j < n; ++j) row += std::fabs(a[n * i + j]);
    norm = std::max(norm, row);
  }
  int s = norm > 0.5 ? static_cast<int>(std::ceil(std::log2(norm / 0.5))) : 0;
  double scale = std::ldexp(1.0, -s);
  for (double& v : a) v *= scale;
  std::vector<double> result(n * n, 0.0), term(n * n, 0.0), next(n * n);
  for (int i = 0; i < n; ++i) result[n * i + i] = term[n * i + i] = 1.0;
  for (int k = 1; k <= 18; ++k) {
    std::fill(next.begin(), next.end(), 0.0);
    for (int i = 0; i < n; ++i)
      for (int l = 0; l < n; ++l) {
        double t = term[n * i + l];
        if (t == 0) continue;
        for (int j = 0; j < n; ++j) next[n * i + j] += t * a[n * l + j];
      }
    for (int i = 0; i < n * n; ++i) term[i] = next[i] / k, result[i] += term[i];
  }
  for (int k = 0; k < s; ++k) {
    std::fill(next.begin(), next.end(), 0.0);
    for (int i = 0; i < n; ++i)
      for (int l = 0; l < n; ++l)
        for (int j = 0; j < n; ++j) next[n * i + j] += result[n * i + l] * result[n * l + j];
    result.swap(next);
  }
  return result;
}

Mat3 expm3(const Mat3& a) {
  std::vector<double> r = expm(3, std::vector<double>(a.begin(), a.end()));
  Mat3 m{};
  std::copy(r.begin(), r.end(), m.begin());
  return m;
}

// Frechet derivative of exp at a in direction e: the upper right block of
// exp([[a, e], [0, a]]).
Mat3 expm_frechet(const Mat3& a, const Mat3& e) {
  std::vector<double> b(36, 0.0);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      b[6 * i + j] = a[3 * i + j];
      b[6 * (i + 3) + j + 3] = a[3 * i + j];
      b[6 * i + j + 3] = e[3 * i + j];
    }
  std::vector<double> r = expm(6, b);
  Mat3 d{};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) d[3 * i + j] = r[6 * i + j + 3];
  return d;
}

Mat3 voigt_full(const std::array<double, 6>& v) {
  return Mat3{v[0], v[5], v[4], v[5], v[1], v[3], v[4], v[3], v[2]};
}

std::vector<double> parse_numbers(const std::string& line) {
  std::vector<double> out;
  const char* p = line.c_str();
  char* end = nullptr;
  for (;;) {
    double v = std::strtod(p, &end);
    if (end == p) break;
    out.push_back(v);
    p = end;
  }
  return out;
}

}  // namespace

double Structure::volume() const { return std::fabs(det(cell)); }

double mass_of(const std::string& symbol) {
  std::istringstream in(SYMBOLS);
  std::string s;
  for (int z = 0; in >> s; ++z)
    if (s == symbol) return MASSES[z];
  throw std::runtime_error("unknown element '" + symbol + "'");
}

Structure read_extxyz(const std::string& text) {
  std::istringstream in(text);
  std::string line;
  if (!std::getline(in, line)) throw std::runtime_error("the structure is empty");
  int n = std::atoi(line.c_str());
  if (n < 1) throw std::runtime_error("the first structure line must be the number of atoms");
  std::string comment;
  std::getline(in, comment);
  Structure s;
  size_t lat = comment.find("Lattice=\"");
  if (lat != std::string::npos) {
    size_t a = lat + 9, b = comment.find('"', a);
    std::vector<double> v = parse_numbers(comment.substr(a, b - a));
    if (v.size() != 9) throw std::runtime_error("Lattice= needs 9 numbers");
    std::copy(v.begin(), v.end(), s.cell.begin());
    s.periodic = true;
  }
  size_t prop = comment.find("Properties=");
  if (prop != std::string::npos &&
      comment.compare(prop, 32, "Properties=species:S:1:pos:R:3") != 0 &&
      comment.compare(prop, 30, "Properties=species:S:1:pos:R:3") != 0)
    throw std::runtime_error("the structure must have species and pos columns first");
  for (int i = 0; i < n; ++i) {
    if (!std::getline(in, line)) throw std::runtime_error("fewer atom lines than the count");
    std::istringstream row(line);
    std::string sym;
    double x, y, z;
    if (!(row >> sym >> x >> y >> z)) throw std::runtime_error("bad atom line: " + line);
    mass_of(sym);
    s.symbols.push_back(sym);
    s.pos.insert(s.pos.end(), {x, y, z});
  }
  return s;
}

std::string extxyz_frame(const Structure& s, double energy, const std::string& extra) {
  std::ostringstream out;
  char buf[160];
  out << s.natoms() << "\n";
  if (s.periodic) {
    out << "Lattice=\"";
    for (int i = 0; i < 9; ++i) {
      std::snprintf(buf, sizeof buf, "%s%.10f", i ? " " : "", s.cell[i]);
      out << buf;
    }
    out << "\" ";
  }
  std::snprintf(buf, sizeof buf, "Properties=species:S:1:pos:R:3 energy=%.10f", energy);
  out << buf << extra << (s.periodic ? " pbc=\"T T T\"" : " pbc=\"F F F\"") << "\n";
  for (int a = 0; a < s.natoms(); ++a) {
    std::snprintf(buf, sizeof buf, "%-2s %16.10f %16.10f %16.10f\n", s.symbols[a].c_str(),
                  s.pos[3 * a], s.pos[3 * a + 1], s.pos[3 * a + 2]);
    out << buf;
  }
  return out.str();
}

Result lennard_jones(const Structure& s, double sigma, double eps, double rc) {
  const int n = s.natoms();
  Result r;
  r.forces.assign(3 * n, 0.0);
  double e0 = 4 * eps * (std::pow(sigma / rc, 12) - std::pow(sigma / rc, 6));
  Mat3 virial{};  // sum over pairs of phi'(d) d_a d_b / d
  int range[3] = {0, 0, 0};
  if (s.periodic) {
    // Images needed: the cut-off over the spacing of each set of planes.
    Mat3 inv = inverse(s.cell);
    for (int k = 0; k < 3; ++k) {
      double g = std::sqrt(inv[k] * inv[k] + inv[3 + k] * inv[3 + k] + inv[6 + k] * inv[6 + k]);
      range[k] = static_cast<int>(std::ceil(rc * g));
    }
  }
  for (int i = 0; i < n; ++i)
    for (int j = i; j < n; ++j)
      for (int a = -range[0]; a <= range[0]; ++a)
        for (int b = -range[1]; b <= range[1]; ++b)
          for (int c = -range[2]; c <= range[2]; ++c) {
            if (i == j && a == 0 && b == 0 && c == 0) continue;
            double d[3];
            for (int k = 0; k < 3; ++k)
              d[k] = s.pos[3 * j + k] - s.pos[3 * i + k] +
                     a * s.cell[k] + b * s.cell[3 + k] + c * s.cell[6 + k];
            double r2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
            if (r2 >= rc * rc) continue;
            double dist = std::sqrt(r2), x6 = std::pow(sigma * sigma / r2, 3);
            double w = (i == j) ? 0.5 : 1.0;  // an atom and its own image: each pair twice
            r.energy += w * (4 * eps * (x6 * x6 - x6) - e0);
            double dphi = 4 * eps * (-12 * x6 * x6 + 6 * x6) / dist;
            for (int k = 0; k < 3; ++k) {
              double f = w * dphi * d[k] / dist;  // force on j is -f, on i +f
              r.forces[3 * i + k] += f;
              r.forces[3 * j + k] -= f;
              for (int l = 0; l < 3; ++l) virial[3 * k + l] += w * dphi * d[k] * d[l] / dist;
            }
          }
  if (s.periodic) {
    double v = s.volume();
    r.has_stress = true;
    r.stress = {virial[0] / v, virial[4] / v, virial[8] / v,
                virial[5] / v, virial[2] / v, virial[1] / v};
  }
  return r;
}

Worker::Worker(const std::vector<std::string>& argv, const Structure& s,
               const std::string& model, const std::string& dtype, const std::string& disp) {
  std::signal(SIGPIPE, SIG_IGN);
  int down[2], up[2];
  if (pipe(down) != 0 || pipe(up) != 0) throw std::runtime_error("pipe() failed");
  std::fflush(stdout);
  pid_ = fork();
  if (pid_ < 0) throw std::runtime_error("fork() failed");
  if (pid_ == 0) {
    dup2(down[0], 0);
    // The protocol goes over fd 3; the worker's own messages (warnings from
    // torch and mace) stay on stdout and stderr, which are the job's output.
    dup2(up[1], 3);
    close(down[0]), close(down[1]), close(up[0]), close(up[1]);
    std::vector<char*> args;
    for (const std::string& a : argv) args.push_back(const_cast<char*>(a.c_str()));
    args.push_back(nullptr);
    execvp(args[0], args.data());
    std::fprintf(stderr, "ecce-mlp: cannot run %s: %s\n", args[0], std::strerror(errno));
    _exit(127);
  }
  close(down[0]);
  close(up[1]);
  to_ = down[1];
  from_ = fdopen(up[0], "r");
  std::ostringstream init;
  init << "init " << s.natoms() << " " << (s.periodic ? 1 : 0) << " " << model << " "
       << dtype << " " << disp << "\n";
  for (const std::string& sym : s.symbols) init << sym << " ";
  init << "\n";
  std::string msg = init.str();
  if (write(to_, msg.data(), msg.size()) != static_cast<ssize_t>(msg.size()))
    throw std::runtime_error("the Python worker did not start");
  std::string reply = read_line();
  std::vector<std::string> parts;
  std::istringstream in(reply);
  for (std::string p; std::getline(in, p, '\t');) parts.push_back(p);
  if (parts.size() < 5 || parts[0] != "ok")
    throw std::runtime_error(parts.size() > 1 ? parts[1] : "the Python worker failed to load the model");
  name = parts[1], version = parts[2], licence = parts[3], weights = parts[4];
}

Worker::~Worker() {
  if (to_ >= 0) {
    ssize_t ignored = write(to_, "quit\n", 5);
    (void)ignored;
    close(to_);
  }
  if (from_) std::fclose(from_);
  if (pid_ > 0) waitpid(pid_, nullptr, 0);
}

std::string Worker::read_line() {
  std::string line;
  char buf[65536];
  while (std::fgets(buf, sizeof buf, from_)) {
    line += buf;
    if (!line.empty() && line.back() == '\n') return line.substr(0, line.size() - 1);
  }
  throw std::runtime_error("the Python worker stopped (see the messages above)");
}

Result Worker::evaluate(const Structure& s) {
  std::string msg = "eval\n";
  char buf[64];
  if (s.periodic) {
    for (double v : s.cell) std::snprintf(buf, sizeof buf, "%.17g ", v), msg += buf;
  } else {
    msg += "none";
  }
  msg += "\n";
  for (double v : s.pos) std::snprintf(buf, sizeof buf, "%.17g ", v), msg += buf;
  msg += "\n";
  if (write(to_, msg.data(), msg.size()) != static_cast<ssize_t>(msg.size()))
    throw std::runtime_error("the Python worker stopped (see the messages above)");
  std::string head = read_line();
  if (head.compare(0, 3, "ok ") != 0) throw std::runtime_error("worker: " + head);
  Result r;
  r.energy = std::strtod(head.c_str() + 3, nullptr);
  r.forces = parse_numbers(read_line());
  if (static_cast<int>(r.forces.size()) != 3 * s.natoms())
    throw std::runtime_error("worker: wrong number of forces");
  std::vector<double> st = parse_numbers(read_line());
  if (st.size() == 6) {
    r.has_stress = true;
    std::copy(st.begin(), st.end(), r.stress.begin());
  }
  return r;
}

double max_row_norm(const std::vector<double>& v) {
  double m = 0;
  for (size_t i = 0; i + 2 < v.size(); i += 3)
    m = std::max(m, std::sqrt(v[i] * v[i] + v[i + 1] * v[i + 1] + v[i + 2] * v[i + 2]));
  return m;
}

// The optimisers work on generalised coordinates q.  Atoms only: q is the
// positions.  With the cell, as ASE's FrechetCellFilter: the positions
// mapped back through the deformation gradient F, then 3 rows of
// N*log(F), F = exp(Q/N) with cell = cell0 F^T; the matching forces are the
// atomic forces times F and -dH/dQ, H = E + pV.
OptOutcome optimize(Structure s, const OptOptions& opt, const Evaluator& eval,
                    const std::function<void(int, const Structure&, const Result&, double)>& on_step,
                    const std::function<double()>& elapsed) {
  const int n = s.natoms();
  const bool cell = opt.cell && s.periodic;
  const double factor = n;
  const Mat3 cell0 = s.cell;
  std::vector<double> q(s.pos);
  if (cell) q.resize(3 * n + 9, 0.0);

  auto apply = [&](const std::vector<double>& x, Mat3& F) {
    Structure t = s;
    F = identity();
    if (!cell) {
      std::copy(x.begin(), x.begin() + 3 * n, t.pos.begin());
      return t;
    }
    Mat3 Q{};
    for (int i = 0; i < 9; ++i) Q[i] = x[3 * n + i] / factor;
    F = expm3(Q);
    t.cell = mul(cell0, transpose(F));
    for (int a = 0; a < n; ++a)
      for (int k = 0; k < 3; ++k) {
        double v = 0;
        for (int j = 0; j < 3; ++j) v += x[3 * a + j] * F[3 * k + j];
        t.pos[3 * a + k] = v;
      }
    return t;
  };
  auto forces = [&](const std::vector<double>& x, const Structure& t, const Result& r,
                    const Mat3& F) {
    std::vector<double> g(x.size(), 0.0);
    if (!cell) {
      std::copy(r.forces.begin(), r.forces.end(), g.begin());
      return g;
    }
    for (int a = 0; a < n; ++a)
      for (int k = 0; k < 3; ++k) {
        double v = 0;
        for (int j = 0; j < 3; ++j) v += r.forces[3 * a + j] * F[3 * j + k];
        g[3 * a + k] = v;
      }
    if (!r.has_stress) throw std::runtime_error("the potential gave no stress; the cell cannot be relaxed");
    Mat3 sig = voigt_full(r.stress);
    for (int k = 0; k < 3; ++k) sig[4 * k] += opt.pressure;
    Mat3 G = mul(sig, transpose(inverse(F)));
    double V = t.volume();
    Mat3 Q{};
    for (int i = 0; i < 9; ++i) Q[i] = x[3 * n + i] / factor;
    for (int i = 0; i < 9; ++i) {
      Mat3 e{};
      e[i] = 1.0;
      Mat3 D = expm_frechet(Q, e);
      double d = 0;
      for (int j = 0; j < 9; ++j) d += G[j] * D[j];
      g[3 * n + i] = -V * d / factor;
    }
    return g;
  };

  const size_t m = q.size();
  // FIRE (ASE's defaults)
  std::vector<double> v(m, 0.0);
  double dt = 0.1, alpha = 0.1;
  int nsteps = 0;
  bool first = true;
  // BFGS (ASE's: H0 = 70 eV/A^2, rows limited to 0.2 A)
  std::vector<double> H, q0, g0;

  OptOutcome out;
  for (int k = 0;; ++k) {
    Mat3 F;
    Structure t = apply(q, F);
    Result r = eval(t);
    std::vector<double> g = forces(q, t, r, F);
    double fm = max_row_norm(g);
    on_step(k, t, r, fm);
    out.s = t, out.r = r, out.steps = k;
    if (fm < opt.fmax) {
      out.converged = true;
      break;
    }
    if (k >= opt.steps) {
      out.stopped = "step limit of " + std::to_string(opt.steps);
      break;
    }
    if (opt.timelimit > 0 && elapsed() > opt.timelimit) {
      char buf[64];
      std::snprintf(buf, sizeof buf, "time limit of %g minutes", opt.timelimit / 60);
      out.stopped = buf;
      break;
    }
    std::vector<double> dr(m);
    if (opt.method == "FIRE") {
      if (!first) {
        double vf = 0, ff = 0, vv = 0;
        for (size_t i = 0; i < m; ++i) vf += v[i] * g[i], ff += g[i] * g[i], vv += v[i] * v[i];
        if (vf > 0) {
          double c = std::sqrt(vv / ff);
          for (size_t i = 0; i < m; ++i) v[i] = (1 - alpha) * v[i] + alpha * g[i] * c;
          if (nsteps > 5) dt = std::min(dt * 1.1, 1.0), alpha *= 0.99;
          ++nsteps;
        } else {
          std::fill(v.begin(), v.end(), 0.0);
          alpha = 0.1, dt *= 0.5, nsteps = 0;
        }
      }
      first = false;
      double norm = 0;
      for (size_t i = 0; i < m; ++i) v[i] += dt * g[i], dr[i] = dt * v[i], norm += dr[i] * dr[i];
      norm = std::sqrt(norm);
      if (norm > 0.2)
        for (double& d : dr) d *= 0.2 / norm;
    } else {
      if (H.empty()) {
        H.assign(m * m, 0.0);
        for (size_t i = 0; i < m; ++i) H[m * i + i] = 70.0;
      } else {
        std::vector<double> s1(m), y(m), Hs(m, 0.0);
        double big = 0;
        for (size_t i = 0; i < m; ++i)
          s1[i] = q[i] - q0[i], y[i] = g[i] - g0[i], big = std::max(big, std::fabs(s1[i]));
        if (big >= 1e-7) {
          double a = 0, b = 0;
          for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < m; ++j) Hs[i] += H[m * i + j] * s1[j];
            a += s1[i] * y[i];
          }
          for (size_t i = 0; i < m; ++i) b += s1[i] * Hs[i];
          for (size_t i = 0; i < m; ++i)
            for (size_t j = 0; j < m; ++j) H[m * i + j] -= y[i] * y[j] / a + Hs[i] * Hs[j] / b;
        }
      }
      std::vector<double> V(H), w;
      eigh(static_cast<int>(m), V, w);
      std::vector<double> c(m, 0.0);
      for (size_t j = 0; j < m; ++j) {
        for (size_t i = 0; i < m; ++i) c[j] += g[i] * V[m * i + j];
        c[j] /= std::fabs(w[j]);
      }
      for (size_t i = 0; i < m; ++i) {
        dr[i] = 0;
        for (size_t j = 0; j < m; ++j) dr[i] += V[m * i + j] * c[j];
      }
      double longest = max_row_norm(dr);
      if (longest > 0.2)
        for (double& d : dr) d *= 0.2 / longest;
      q0 = q, g0 = g;
    }
    for (size_t i = 0; i < m; ++i) q[i] += dr[i];
  }
  return out;
}

MDOutcome molecular_dynamics(Structure s, const MDOptions& md, const Evaluator& eval,
                             const std::function<void(const MDFrame&, const Structure&, const Result&)>& on_frame,
                             const std::function<double()>& elapsed) {
  const int n = s.natoms();
  const double dt = md.dt_fs * FS;
  const int ndf = n > 1 ? 3 * n - 3 : 3;
  std::vector<double> m(n);
  double mtot = 0;
  for (int a = 0; a < n; ++a) m[a] = mass_of(s.symbols[a]), mtot += m[a];
  std::mt19937_64 rng(md.seed);
  std::normal_distribution<double> gauss(0.0, 1.0);

  auto kinetic = [&](const std::vector<double>& v) {
    double e = 0;
    for (int a = 0; a < n; ++a)
      for (int k = 0; k < 3; ++k) e += 0.5 * m[a] * v[3 * a + k] * v[3 * a + k];
    return e;
  };
  // Maxwell-Boltzmann, no centre-of-mass motion, scaled to the temperature.
  std::vector<double> v(3 * n);
  for (int a = 0; a < n; ++a)
    for (int k = 0; k < 3; ++k) v[3 * a + k] = gauss(rng) * std::sqrt(kB * md.temperature / m[a]);
  if (n > 1)
    for (int k = 0; k < 3; ++k) {
      double p = 0;
      for (int a = 0; a < n; ++a) p += m[a] * v[3 * a + k];
      for (int a = 0; a < n; ++a) v[3 * a + k] -= p / mtot;
    }
  double ek = kinetic(v);
  if (ek > 0) {
    double c = std::sqrt(0.5 * ndf * kB * md.temperature / ek);
    for (double& x : v) x *= c;
  }

  Result r = eval(s);
  MDOutcome out;
  double tsum = 0, st = 0, se = 0, stt = 0, ste = 0;
  int count = 0;
  auto frame = [&](int step) {
    MDFrame f;
    f.step = step;
    f.time_ps = step * md.dt_fs * 1e-3;
    f.epot = r.energy;
    f.ekin = kinetic(v);
    f.temperature = 2 * f.ekin / (ndf * kB);
    return f;
  };
  on_frame(frame(0), s, r);
  int step = 0;
  for (step = 1; step <= md.steps; ++step) {
    for (int i = 0; i < 3 * n; ++i) v[i] += 0.5 * dt * r.forces[i] / m[i / 3];
    for (int i = 0; i < 3 * n; ++i) s.pos[i] += dt * v[i];
    r = eval(s);
    for (int i = 0; i < 3 * n; ++i) v[i] += 0.5 * dt * r.forces[i] / m[i / 3];
    if (md.ensemble == "NVT") {
      // Bussi, Donadio and Parrinello, J. Chem. Phys. 126, 014101 (2007):
      // the kinetic energy drawn from its canonical distribution.
      double K = kinetic(v), Kt = 0.5 * ndf * kB * md.temperature;
      double c1 = std::exp(-md.dt_fs / md.tau_fs);
      double R1 = gauss(rng);
      double sum = ndf > 1 ? 2.0 * std::gamma_distribution<double>(0.5 * (ndf - 1), 1.0)(rng) : 0;
      double ratio = Kt / (ndf * K);
      double a2 = c1 + (1 - c1) * ratio * (R1 * R1 + sum) + 2 * R1 * std::sqrt(c1 * (1 - c1) * ratio);
      double alpha = std::sqrt(std::max(a2, 0.0));
      if (R1 + std::sqrt(c1 / ((1 - c1) * ratio)) < 0) alpha = -alpha;
      for (double& x : v) x *= alpha;
    }
    MDFrame f = frame(step);
    double etot = f.epot + f.ekin;
    tsum += f.temperature, ++count;
    st += f.time_ps, se += etot, stt += f.time_ps * f.time_ps, ste += f.time_ps * etot;
    bool last = step == md.steps;
    bool timeout = md.timelimit > 0 && elapsed() > md.timelimit;
    if (step % md.every == 0 || last || timeout) on_frame(f, s, r);
    if (timeout) {
      char buf[64];
      std::snprintf(buf, sizeof buf, "time limit of %g minutes", md.timelimit / 60);
      out.stopped = buf;
      break;
    }
  }
  out.steps = std::min(step, md.steps);
  out.s = s, out.r = r;
  out.mean_temperature = count ? tsum / count : 0;
  if (count > 1) {
    double den = count * stt - st * st;
    out.drift_ev_atom_ps = den > 0 ? (count * ste - st * se) / den / n : 0;
  }
  return out;
}

// Householder tridiagonalisation and the QL algorithm, after the public
// domain JAMA (tred2, tql2).
void eigh(int n, std::vector<double>& a, std::vector<double>& w) {
  std::vector<std::vector<double>> V(n, std::vector<double>(n));
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) V[i][j] = a[n * i + j];
  std::vector<double> d(n), e(n);
  for (int j = 0; j < n; ++j) d[j] = V[n - 1][j];
  for (int i = n - 1; i > 0; --i) {
    double scale = 0, h = 0;
    for (int k = 0; k < i; ++k) scale += std::fabs(d[k]);
    if (scale == 0) {
      e[i] = d[i - 1];
      for (int j = 0; j < i; ++j) d[j] = V[i - 1][j], V[i][j] = 0, V[j][i] = 0;
    } else {
      for (int k = 0; k < i; ++k) d[k] /= scale, h += d[k] * d[k];
      double f = d[i - 1], g = std::sqrt(h);
      if (f > 0) g = -g;
      e[i] = scale * g;
      h -= f * g;
      d[i - 1] = f - g;
      for (int j = 0; j < i; ++j) e[j] = 0;
      for (int j = 0; j < i; ++j) {
        f = d[j];
        V[j][i] = f;
        g = e[j] + V[j][j] * f;
        for (int k = j + 1; k <= i - 1; ++k) g += V[k][j] * d[k], e[k] += V[k][j] * f;
        e[j] = g;
      }
      f = 0;
      for (int j = 0; j < i; ++j) e[j] /= h, f += e[j] * d[j];
      double hh = f / (h + h);
      for (int j = 0; j < i; ++j) e[j] -= hh * d[j];
      for (int j = 0; j < i; ++j) {
        f = d[j], g = e[j];
        for (int k = j; k <= i - 1; ++k) V[k][j] -= (f * e[k] + g * d[k]);
        d[j] = V[i - 1][j];
        V[i][j] = 0;
      }
    }
    d[i] = h;
  }
  for (int i = 0; i < n - 1; ++i) {
    V[n - 1][i] = V[i][i];
    V[i][i] = 1;
    double h = d[i + 1];
    if (h != 0) {
      for (int k = 0; k <= i; ++k) d[k] = V[k][i + 1] / h;
      for (int j = 0; j <= i; ++j) {
        double g = 0;
        for (int k = 0; k <= i; ++k) g += V[k][i + 1] * V[k][j];
        for (int k = 0; k <= i; ++k) V[k][j] -= g * d[k];
      }
    }
    for (int k = 0; k <= i; ++k) V[k][i + 1] = 0;
  }
  for (int j = 0; j < n; ++j) d[j] = V[n - 1][j], V[n - 1][j] = 0;
  V[n - 1][n - 1] = 1;
  e[0] = 0;

  for (int i = 1; i < n; ++i) e[i - 1] = e[i];
  e[n - 1] = 0;
  double f = 0, tst1 = 0, eps = std::ldexp(1.0, -52);
  for (int l = 0; l < n; ++l) {
    tst1 = std::max(tst1, std::fabs(d[l]) + std::fabs(e[l]));
    int m = l;
    while (m < n && std::fabs(e[m]) > eps * tst1) ++m;
    if (m == n) m = n - 1;
    if (m > l) {
      do {
        double g = d[l];
        double p = (d[l + 1] - g) / (2.0 * e[l]);
        double r = std::hypot(p, 1.0);
        if (p < 0) r = -r;
        d[l] = e[l] / (p + r);
        d[l + 1] = e[l] * (p + r);
        double dl1 = d[l + 1], h = g - d[l];
        for (int i = l + 2; i < n; ++i) d[i] -= h;
        f += h;
        p = d[m];
        double c = 1, c2 = c, c3 = c, el1 = e[l + 1], s = 0, s2 = 0;
        for (int i = m - 1; i >= l; --i) {
          c3 = c2, c2 = c, s2 = s;
          g = c * e[i], h = c * p;
          r = std::hypot(p, e[i]);
          e[i + 1] = s * r;
          s = e[i] / r, c = p / r;
          p = c * d[i] - s * g;
          d[i + 1] = h + s * (c * g + s * d[i]);
          for (int k = 0; k < n; ++k) {
            h = V[k][i + 1];
            V[k][i + 1] = s * V[k][i] + c * h;
            V[k][i] = c * V[k][i] - s * h;
          }
        }
        p = -s * s2 * c3 * el1 * e[l] / dl1;
        e[l] = s * p, d[l] = c * p;
      } while (std::fabs(e[l]) > eps * tst1);
    }
    d[l] += f, e[l] = 0;
  }
  std::vector<int> order(n);
  for (int i = 0; i < n; ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](int x, int y) { return d[x] < d[y]; });
  w.resize(n);
  a.assign(static_cast<size_t>(n) * n, 0.0);
  for (int j = 0; j < n; ++j) {
    w[j] = d[order[j]];
    for (int i = 0; i < n; ++i) a[n * i + j] = V[i][order[j]];
  }
}

Vibrations vibrations(const Structure& s, double delta, const Evaluator& eval,
                      const std::function<void(int, int)>& progress) {
  const int n = s.natoms(), N = 3 * n;
  std::vector<double> hess(static_cast<size_t>(N) * N);
  for (int i = 0; i < N; ++i) {
    Structure t = s;
    t.pos[i] = s.pos[i] + delta;
    std::vector<double> fp = eval(t).forces;
    t.pos[i] = s.pos[i] - delta;
    std::vector<double> fm = eval(t).forces;
    for (int j = 0; j < N; ++j) hess[N * i + j] = -(fp[j] - fm[j]) / (2 * delta);
    progress(i + 1, N);
  }
  std::vector<double> inv(N), sq(n);
  double com[3] = {0, 0, 0}, mtot = 0;
  for (int a = 0; a < n; ++a) {
    double m = mass_of(s.symbols[a]);
    sq[a] = std::sqrt(m);
    for (int k = 0; k < 3; ++k) inv[3 * a + k] = 1 / sq[a], com[k] += m * s.pos[3 * a + k];
    mtot += m;
  }
  for (double& c : com) c /= mtot;
  std::vector<double> hm(static_cast<size_t>(N) * N);
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      hm[N * i + j] = 0.5 * (hess[N * i + j] + hess[N * j + i]) * inv[i] * inv[j];

  // Rigid motions, mass-weighted, orthonormalised; a linear molecule loses
  // one rotation.
  std::vector<std::vector<double>> cand;
  for (int k = 0; k < 3; ++k) {
    std::vector<double> t(N, 0.0);
    for (int a = 0; a < n; ++a) t[3 * a + k] = sq[a];
    cand.push_back(t);
  }
  if (!s.periodic)
    for (int k = 0; k < 3; ++k) {
      std::vector<double> t(N, 0.0);
      for (int a = 0; a < n; ++a) {
        double r[3] = {s.pos[3 * a] - com[0], s.pos[3 * a + 1] - com[1], s.pos[3 * a + 2] - com[2]};
        double ax[3] = {0, 0, 0};
        ax[k] = 1;
        t[3 * a] = (ax[1] * r[2] - ax[2] * r[1]) * sq[a];
        t[3 * a + 1] = (ax[2] * r[0] - ax[0] * r[2]) * sq[a];
        t[3 * a + 2] = (ax[0] * r[1] - ax[1] * r[0]) * sq[a];
      }
      cand.push_back(t);
    }
  double biggest = 0;
  for (auto& c : cand) {
    double nn = 0;
    for (double x : c) nn += x * x;
    biggest = std::max(biggest, std::sqrt(nn));
  }
  std::vector<std::vector<double>> rigid;
  for (auto c : cand) {
    for (auto& b : rigid) {
      double dot = 0;
      for (int i = 0; i < N; ++i) dot += b[i] * c[i];
      for (int i = 0; i < N; ++i) c[i] -= dot * b[i];
    }
    double nn = 0;
    for (double x : c) nn += x * x;
    nn = std::sqrt(nn);
    if (nn > 1e-6 * biggest) {
      for (double& x : c) x /= nn;
      rigid.push_back(c);
    }
  }
  // P hm P with P = 1 - R R^T
  std::vector<double> P(static_cast<size_t>(N) * N, 0.0), tmp(static_cast<size_t>(N) * N, 0.0);
  for (int i = 0; i < N; ++i) {
    P[N * i + i] = 1;
    for (auto& b : rigid)
      for (int j = 0; j < N; ++j) P[N * i + j] -= b[i] * b[j];
  }
  for (int i = 0; i < N; ++i)
    for (int k = 0; k < N; ++k) {
      double p = P[N * i + k];
      if (p == 0) continue;
      for (int j = 0; j < N; ++j) tmp[N * i + j] += p * hm[N * k + j];
    }
  std::fill(hm.begin(), hm.end(), 0.0);
  for (int i = 0; i < N; ++i)
    for (int k = 0; k < N; ++k) {
      double t = tmp[N * i + k];
      if (t == 0) continue;
      for (int j = 0; j < N; ++j) hm[N * i + j] += t * P[N * k + j];
    }
  std::vector<double> w;
  eigh(N, hm, w);
  std::vector<double> overlap(N, 0.0);
  for (int j = 0; j < N; ++j)
    for (auto& b : rigid) {
      double dot = 0;
      for (int i = 0; i < N; ++i) dot += b[i] * hm[N * i + j];
      overlap[j] += dot * dot;
    }
  std::vector<int> order;
  for (int j = 0; j < N && static_cast<int>(order.size()) < static_cast<int>(rigid.size()); ++j)
    if (overlap[j] > 0.5) order.push_back(j);
  for (int j = 0; j < N; ++j)
    if (std::find(order.begin(), order.end(), j) == order.end()) order.push_back(j);

  Vibrations out;
  out.rigid = static_cast<int>(rigid.size());
  out.freq.assign(N, 0.0);
  out.modes.assign(static_cast<size_t>(N) * N, 0.0);
  for (int k = out.rigid; k < N; ++k) {
    int j = order[k];
    out.freq[k] = TO_CM * (w[j] < 0 ? -1 : 1) * std::sqrt(std::fabs(w[j]));
    double nn = 0;
    for (int i = 0; i < N; ++i) out.modes[N * k + i] = hm[N * i + j] * inv[i], nn += std::pow(out.modes[N * k + i], 2);
    nn = std::sqrt(nn);
    for (int i = 0; i < N; ++i) out.modes[N * k + i] /= nn;
  }
  return out;
}

}  // namespace mlp
