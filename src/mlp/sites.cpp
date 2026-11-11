#include "sites.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace mlp {

namespace {

bool invert(const Mat3& m, Mat3& inv) {
  double det = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) +
               m[2] * (m[3] * m[7] - m[4] * m[6]);
  if (std::fabs(det) < 1e-12) return false;
  inv = {(m[4] * m[8] - m[5] * m[7]) / det, (m[2] * m[7] - m[1] * m[8]) / det,
         (m[1] * m[5] - m[2] * m[4]) / det, (m[5] * m[6] - m[3] * m[8]) / det,
         (m[0] * m[8] - m[2] * m[6]) / det, (m[2] * m[3] - m[0] * m[5]) / det,
         (m[3] * m[7] - m[4] * m[6]) / det, (m[1] * m[6] - m[0] * m[7]) / det,
         (m[0] * m[4] - m[1] * m[3]) / det};
  return true;
}

double wrap(double x) {
  x -= std::floor(x);
  return (x > 1 - 1e-7) ? 0.0 : x;
}

// The first shell: neighbours within 10 % of the nearest one.
const double kShell = 1.1;

// Minimum-image length of a fractional difference.  The wrapped vector is
// exact for an orthogonal cell; the 27 neighbours cover a sheared one.
double frac_length(const Structure& s, double d[3]) {
  for (int k = 0; k < 3; ++k) d[k] -= std::round(d[k]);
  double best = 1e300;
  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      for (int l = -1; l <= 1; ++l) {
        double f[3] = {d[0] + i, d[1] + j, d[2] + l}, c[3];
        to_cart(s, f, c);
        best = std::min(best, c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
      }
  return std::sqrt(best);
}

void apply(const SymOp& op, const double f[3], double out[3]) {
  for (int r = 0; r < 3; ++r)
    out[r] = op.W[3 * r] * f[0] + op.W[3 * r + 1] * f[1] + op.W[3 * r + 2] * f[2] + op.t[r];
}

std::vector<double> fractional(const Structure& s) {
  std::vector<double> f(s.pos.size());
  for (int a = 0; a < s.natoms(); ++a) {
    to_frac(s, &s.pos[3 * a], &f[3 * a]);
    for (int k = 0; k < 3; ++k) f[3 * a + k] = wrap(f[3 * a + k]);
  }
  return f;
}

double frac_distance(const Structure& s, const double a[3], const double b[3]) {
  double d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
  return frac_length(s, d);
}

}  // namespace

void to_frac(const Structure& s, const double c[3], double f[3]) {
  Mat3 inv;
  if (!invert(s.cell, inv)) throw std::runtime_error("the cell is singular");
  // r = A^T f, so f = A^-T r.
  for (int k = 0; k < 3; ++k) f[k] = inv[k] * c[0] + inv[3 + k] * c[1] + inv[6 + k] * c[2];
}

void to_cart(const Structure& s, const double f[3], double c[3]) {
  for (int k = 0; k < 3; ++k)
    c[k] = f[0] * s.cell[k] + f[1] * s.cell[3 + k] + f[2] * s.cell[6 + k];
}

double min_image_distance(const Structure& s, const double a[3], const double b[3]) {
  double fa[3], fb[3];
  to_frac(s, a, fa);
  to_frac(s, b, fb);
  return frac_distance(s, fa, fb);
}

std::vector<SymOp> find_symmetry(const Structure& host, double symprec) {
  if (!host.periodic) throw std::runtime_error("the host has no cell");
  const int n = host.natoms();
  const std::vector<double> f = fractional(host);
  double G[9];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      G[3 * i + j] = host.cell[3 * i] * host.cell[3 * j] + host.cell[3 * i + 1] * host.cell[3 * j + 1] +
                     host.cell[3 * i + 2] * host.cell[3 * j + 2];
  double scale = std::max({G[0], G[4], G[8]});

  // Translations are tried from the rarest species, which keeps them few.
  std::map<std::string, std::vector<int>> by;
  for (int a = 0; a < n; ++a) by[host.symbols[a]].push_back(a);
  const std::vector<int>* rare = nullptr;
  for (const auto& kv : by)
    if (!rare || kv.second.size() < rare->size()) rare = &kv.second;
  const int ref = rare->front();

  std::vector<SymOp> ops;
  int W[9];
  for (int code = 0; code < 19683; ++code) {
    int c = code;
    for (int k = 0; k < 9; ++k, c /= 3) W[k] = c % 3 - 1;
    int det = W[0] * (W[4] * W[8] - W[5] * W[7]) - W[1] * (W[3] * W[8] - W[5] * W[6]) +
              W[2] * (W[3] * W[7] - W[4] * W[6]);
    if (det != 1 && det != -1) continue;
    // Keeps lengths: W^T G W = G.
    bool metric = true;
    for (int i = 0; i < 3 && metric; ++i)
      for (int j = 0; j < 3 && metric; ++j) {
        double v = 0;
        for (int k = 0; k < 3; ++k)
          for (int l = 0; l < 3; ++l) v += W[3 * k + i] * G[3 * k + l] * W[3 * l + j];
        metric = std::fabs(v - G[3 * i + j]) < 1e-3 * scale;
      }
    if (!metric) continue;
    SymOp op;
    std::copy(W, W + 9, op.W);
    op.t[0] = op.t[1] = op.t[2] = 0;
    double wref[3];
    apply(op, &f[3 * ref], wref);
    for (int target : *rare) {
      for (int k = 0; k < 3; ++k) op.t[k] = f[3 * target + k] - wref[k];
      op.perm.assign(n, -1);
      bool ok = true;
      std::vector<char> used(n, 0);
      for (int a = 0; a < n && ok; ++a) {
        double img[3];
        apply(op, &f[3 * a], img);
        ok = false;
        for (int b : by[host.symbols[a]])
          if (!used[b] && frac_distance(host, img, &f[3 * b]) < symprec) {
            op.perm[a] = b;
            used[b] = 1;
            ok = true;
            break;
          }
      }
      if (ok) {
        for (int k = 0; k < 3; ++k) op.t[k] = wrap(op.t[k] + 1e-9) - 1e-9;
        ops.push_back(op);
      }
    }
  }
  return ops;
}

std::string Environment::summary() const {
  std::map<std::string, std::vector<double>> shell;
  if (neighbours.empty()) return "";
  double first = neighbours.front().distance;
  for (const Neighbour& nb : neighbours)
    if (nb.distance <= kShell * first) shell[nb.symbol].push_back(nb.distance);
  std::ostringstream out;
  bool sep = false;
  for (const auto& kv : shell) {
    double mean = 0;
    for (double d : kv.second) mean += d;
    mean /= kv.second.size();
    char buf[64];
    std::snprintf(buf, sizeof buf, "%s%s %.2f x%zu", sep ? ", " : "", kv.first.c_str(), mean,
                  kv.second.size());
    out << buf;
    sep = true;
  }
  return out.str();
}

Environment environment(const Structure& host, const double cart[3], double cutoff, int skip_a,
                        int skip_b) {
  // Every periodic image within the cutoff, not the minimum image only: in a
  // small cell the two neighbours at +-a/2 are one atom seen twice.
  int range[3];
  for (int k = 0; k < 3; ++k) {
    double len = std::sqrt(host.cell[3 * k] * host.cell[3 * k] + host.cell[3 * k + 1] * host.cell[3 * k + 1] +
                           host.cell[3 * k + 2] * host.cell[3 * k + 2]);
    range[k] = static_cast<int>(std::ceil(cutoff / len)) + 1;
  }
  Environment env;
  for (int a = 0; a < host.natoms(); ++a) {
    if (a == skip_a || a == skip_b) continue;
    for (int i = -range[0]; i <= range[0]; ++i)
      for (int j = -range[1]; j <= range[1]; ++j)
        for (int l = -range[2]; l <= range[2]; ++l) {
          double f[3] = {double(i), double(j), double(l)}, c[3];
          to_cart(host, f, c);
          double d2 = 0;
          for (int k = 0; k < 3; ++k) {
            double x = host.pos[3 * a + k] + c[k] - cart[k];
            d2 += x * x;
          }
          double d = std::sqrt(d2);
          if (d > 1e-6 && d <= cutoff) env.neighbours.push_back({host.symbols[a], d});
        }
  }
  std::sort(env.neighbours.begin(), env.neighbours.end(),
            [](const Neighbour& x, const Neighbour& y) { return x.distance < y.distance; });
  if (!env.neighbours.empty()) {
    double first = env.neighbours.front().distance;
    for (const Neighbour& nb : env.neighbours) env.coordination += nb.distance <= kShell * first;
  }
  return env;
}

std::vector<Site> find_voids(const Structure& host, const std::vector<SymOp>& ops,
                             const VoidOptions& o) {
  const std::vector<double> f = fractional(host);
  const int n = host.natoms();
  int g[3];
  for (int k = 0; k < 3; ++k) {
    double len = std::sqrt(host.cell[3 * k] * host.cell[3 * k] + host.cell[3 * k + 1] * host.cell[3 * k + 1] +
                           host.cell[3 * k + 2] * host.cell[3 * k + 2]);
    g[k] = std::max(4, static_cast<int>(std::ceil(len / o.grid)));
  }
  auto clearance = [&](const double p[3]) {
    double best = 1e300;
    for (int a = 0; a < n; ++a) best = std::min(best, frac_distance(host, p, &f[3 * a]));
    return best;
  };
  std::vector<double> field(static_cast<size_t>(g[0]) * g[1] * g[2]);
  auto idx = [&](int i, int j, int k) {
    i = (i % g[0] + g[0]) % g[0];
    j = (j % g[1] + g[1]) % g[1];
    k = (k % g[2] + g[2]) % g[2];
    return (static_cast<size_t>(i) * g[1] + j) * g[2] + k;
  };
  for (int i = 0; i < g[0]; ++i)
    for (int j = 0; j < g[1]; ++j)
      for (int k = 0; k < g[2]; ++k) {
        double p[3] = {double(i) / g[0], double(j) / g[1], double(k) / g[2]};
        field[idx(i, j, k)] = clearance(p);
      }

  struct Cand { double f[3]; double d; };
  std::vector<Cand> cands;
  for (int i = 0; i < g[0]; ++i)
    for (int j = 0; j < g[1]; ++j)
      for (int k = 0; k < g[2]; ++k) {
        double v = field[idx(i, j, k)];
        if (v < o.min_host) continue;
        bool peak = true;
        for (int di = -1; di <= 1 && peak; ++di)
          for (int dj = -1; dj <= 1 && peak; ++dj)
            for (int dk = -1; dk <= 1 && peak; ++dk)
              if ((di || dj || dk) && field[idx(i + di, j + dj, k + dk)] > v + 1e-12) peak = false;
        if (peak) cands.push_back({{double(i) / g[0], double(j) / g[1], double(k) / g[2]}, v});
      }

  // A grid maximum lies within a cell diagonal of the true one; averaging
  // the images that fall back onto it (its site-symmetry group) puts a
  // special position exactly on its special point.
  const double snap = std::max(0.3, 2.0 * o.grid);
  for (Cand& c : cands) {
    for (int pass = 0; pass < 3; ++pass) {
      double sum[3] = {0, 0, 0};
      int cnt = 0;
      for (const SymOp& op : ops) {
        double img[3];
        apply(op, c.f, img);
        double d[3] = {img[0] - c.f[0], img[1] - c.f[1], img[2] - c.f[2]};
        for (int k = 0; k < 3; ++k) d[k] -= std::round(d[k]);
        if (frac_length(host, d) < snap) {
          for (int k = 0; k < 3; ++k) sum[k] += d[k];
          ++cnt;
        }
      }
      if (cnt)
        for (int k = 0; k < 3; ++k) c.f[k] = wrap(c.f[k] + sum[k] / cnt);
    }
    c.d = clearance(c.f);
  }
  std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.d > b.d; });

  std::vector<Site> sites;
  std::vector<std::vector<std::array<double, 3>>> orbits;
  for (const Cand& c : cands) {
    if (c.d < o.min_host) continue;
    bool near = false;
    for (const auto& orb : orbits)
      for (const auto& p : orb)
        if (frac_distance(host, c.f, p.data()) < o.min_sep) { near = true; break; }
    if (near) continue;
    std::vector<std::array<double, 3>> orb;
    for (const SymOp& op : ops) {
      std::array<double, 3> img;
      apply(op, c.f, img.data());
      for (double& x : img) x = wrap(x);
      bool seen = false;
      for (const auto& p : orb)
        if (frac_distance(host, img.data(), p.data()) < 0.1) { seen = true; break; }
      if (!seen) orb.push_back(img);
    }
    if (orb.empty()) orb.push_back({c.f[0], c.f[1], c.f[2]});
    Site s;
    std::copy(c.f, c.f + 3, s.frac);
    to_cart(host, s.frac, s.cart);
    s.clearance = c.d;
    s.multiplicity = static_cast<int>(orb.size());
    s.env = environment(host, s.cart);
    sites.push_back(s);
    orbits.push_back(orb);
  }
  return sites;
}

std::vector<VacancyPair> vacancy_pairs(const Structure& host, const std::vector<SymOp>& ops,
                                       const std::string& element, double max_dist) {
  std::vector<int> idx;
  for (int a = 0; a < host.natoms(); ++a)
    if (host.symbols[a] == element) idx.push_back(a);
  if (idx.size() < 2) throw std::runtime_error("the host has fewer than two " + element + " atoms");
  std::map<std::pair<int, int>, size_t> classOf;
  std::vector<VacancyPair> out;
  for (size_t x = 0; x < idx.size(); ++x)
    for (size_t y = x + 1; y < idx.size(); ++y) {
      int a = idx[x], b = idx[y];
      double d = min_image_distance(host, &host.pos[3 * a], &host.pos[3 * b]);
      if (max_dist > 0 && d > max_dist + 1e-6) continue;
      if (classOf.count({a, b})) continue;
      VacancyPair vp{a, b, d, 0};
      std::set<std::pair<int, int>> orbit;
      for (const SymOp& op : ops) {
        int p = op.perm[a], q = op.perm[b];
        orbit.insert({std::min(p, q), std::max(p, q)});
      }
      if (orbit.empty()) orbit.insert({a, b});
      for (const auto& pq : orbit) classOf[pq] = out.size();
      vp.multiplicity = static_cast<int>(orbit.size());
      out.push_back(vp);
    }
  std::stable_sort(out.begin(), out.end(),
                   [](const VacancyPair& p, const VacancyPair& q) { return p.distance < q.distance - 1e-6; });
  return out;
}

std::string write_extxyz(const Structure& s, const std::string& comment) {
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
  out << "Properties=species:S:1:pos:R:3 pbc=\"" << (s.periodic ? "T T T" : "F F F") << "\"";
  if (!comment.empty()) out << " " << comment;
  out << "\n";
  for (int a = 0; a < s.natoms(); ++a) {
    std::snprintf(buf, sizeof buf, "%-2s %16.10f %16.10f %16.10f\n", s.symbols[a].c_str(),
                  s.pos[3 * a], s.pos[3 * a + 1], s.pos[3 * a + 2]);
    out << buf;
  }
  return out.str();
}

}  // namespace mlp
