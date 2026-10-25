// The speed distribution and MSD analyses against what they must give for
// synthetic data with a known answer: Maxwell-Boltzmann velocities (He and
// Ar of one run) and random walkers of known D, positions wrapped into the
// box and a dimer split across it.  The file reader is exercised on the
// .trr written here; tests/mdanalysis/gmx_oracle.py reads real .xtc files.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "util/MdAnalysis.H"
#include "util/MdTrajectory.H"

static int failures = 0;
static void check(bool ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}

static void putInt(FILE *f, int32_t v)
{
  const unsigned char b[4] = {(unsigned char)(v >> 24), (unsigned char)(v >> 16),
                              (unsigned char)(v >> 8), (unsigned char)v};
  fwrite(b, 1, 4, f);
}
static void putFloat(FILE *f, float v)
{
  uint32_t u;
  memcpy(&u, &v, 4);
  putInt(f, (int32_t)u);
}

static void writeTrrFrame(FILE *f, int natoms, int step, float t, float box,
                          const std::vector<float>& x, const std::vector<float>& v)
{
  putInt(f, 1993); putInt(f, 13); putInt(f, 12);
  fwrite("GMX_trn_file", 1, 12, f);
  const int n3 = natoms * 3 * 4;
  const int sizes[10] = {0, 0, 36, 0, 0, 0, 0, x.empty() ? 0 : n3, v.empty() ? 0 : n3, 0};
  for (int i = 0; i < 10; i++) putInt(f, sizes[i]);
  putInt(f, natoms); putInt(f, step); putInt(f, 0);
  putFloat(f, t); putFloat(f, 0);
  for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) putFloat(f, i == j ? box : 0);
  for (float c : x) putFloat(f, c);
  for (float c : v) putFloat(f, c);
}

static std::string writeFile(const std::string& name, const std::string& text)
{
  std::ofstream(name.c_str()) << text;
  return name;
}

int main(int argc, char **argv)
{
  const std::string dir = argc > 1 ? argv[1] : ".";
  std::mt19937 rng(7);
  std::normal_distribution<double> gauss(0, 1);

  // ---- speeds: 3000 He and 3000 Ar at 300 K, 20 frames
  const double T = 300, kB = 0.0083144626;  // kJ/mol/K; nm/ps with u
  const int nHe = 3000, nAr = 3000, N = nHe + nAr;
  const std::string top = writeFile(dir + "/mb.top",
      "[ atomtypes ]\nHE 2 4.0026 0 A 0.25 0.08\nAR 18 39.948 0 A 0.34 0.99\n"
      "[ moleculetype ]\nHE 1\n[ atoms ]\n1 HE 1 HE HE 1 0 4.0026\n"
      "[ moleculetype ]\nAR 1\n[ atoms ]\n1 AR 1 AR AR 1 0 39.948\n"
      "[ molecules ]\nHE 3000\nAR 3000\n");
  const std::string trr = dir + "/mb.trr";
  {
    FILE *f = fopen(trr.c_str(), "wb");
    for (int fr = 0; fr < 20; fr++) {
      std::vector<float> x(N * 3, 1.0f), v(N * 3);
      for (int i = 0; i < N; i++) {
        const double m = i < nHe ? 4.0026 : 39.948;
        const double s = std::sqrt(kB * T / m);
        for (int k = 0; k < 3; k++) v[3 * i + k] = (float)(s * gauss(rng));
      }
      writeTrrFrame(f, N, fr * 500, (float)fr, 5.0f, x, v);
    }
    fclose(f);
  }
  MdTopology t;
  std::string err;
  check(readTopology(top, t, err), "topology reads");
  check(t.atomCount() == N && t.types.size() == 2, "topology: two types, 6000 atoms");
  std::vector<SpeedCurve> sp;
  check(speedDistribution(trr, t, T, 100, sp, err) && sp.size() == 2, "speed curves for He and Ar");
  if (sp.size() == 2) {
    check(sp[0].label == "He" && sp[1].label == "Ar", "labelled by element");
    double worst = 0, peak = 0;
    for (const SpeedCurve& c : sp) {
      const double w = c.vmax / c.density.size();
      peak = 0;
      for (size_t b = 0; b < c.density.size(); b++)
        peak = std::max(peak, mbDensity((b + 0.5) * w, c.mass, T));
      for (size_t b = 0; b < c.density.size(); b++)
        worst = std::max(worst, std::fabs(c.density[b] - mbDensity((b + 0.5) * w, c.mass, T)) / peak);
    }
    check(worst < 0.08, "histogram follows the Maxwell-Boltzmann curve (max deviation < 8% of the peak)");
    check(std::fabs(sp[0].mean / mbMean(sp[0].mass, T) - 1) < 0.01 &&
          std::fabs(sp[1].rms / mbRms(sp[1].mass, T) - 1) < 0.01, "mean and rms speed within 1% of theory");
    check(std::fabs(sp[0].mean / sp[1].mean / std::sqrt(39.948 / 4.0026) - 1) < 0.015,
          "He/Ar speed ratio is sqrt(m_Ar/m_He)");
    double integral = 0;
    for (double d : sp[1].density) integral += d * sp[1].vmax / sp[1].density.size();
    check(std::fabs(integral - 1) < 0.002, "density is normalised");
  }

  // ---- MSD: walkers with D = 0.005 nm^2/ps, wrapped into a 3 nm box;
  // type A single atoms, type DIM two atoms 0.2 nm apart wrapped separately
  const double D = 0.005, dt = 1.0, L = 3.0;
  const int nA = 300, nD = 300, F = 1200;
  const std::string top2 = writeFile(dir + "/rw.top",
      "[ moleculetype ]\nA 1\n[ atoms ]\n1 X 1 A X 1 0 10.0\n"
      "[ moleculetype ]\nDIM 1\n[ atoms ]\n1 X 1 DIM X1 1 0 10.0\n2 X 1 DIM X2 1 0 10.0\n"
      "[ molecules ]\nA 300\nDIM 300\n");
  MdTopology t2;
  check(readTopology(top2, t2, err), "random-walk topology reads");
  std::vector<double> cx(nA + nD, 0.0), cy(nA + nD, 0.0), cz(nA + nD, 0.0);
  for (int i = 0; i < nA + nD; i++) { cx[i] = L * (rng() % 1000) / 1000.0; cy[i] = L * (rng() % 1000) / 1000.0; cz[i] = L * (rng() % 1000) / 1000.0; }
  const std::string trr2 = dir + "/rw.trr";
  {
    FILE *f = fopen(trr2.c_str(), "wb");
    const double sd = std::sqrt(2 * D * dt);
    for (int fr = 0; fr < F; fr++) {
      std::vector<float> x;
      for (int i = 0; i < nA + nD; i++) {
        cx[i] += sd * gauss(rng); cy[i] += sd * gauss(rng); cz[i] += sd * gauss(rng);
        const int atoms = i < nA ? 1 : 2;
        for (int a = 0; a < atoms; a++) {
          const double off = 0.2 * a;
          x.push_back((float)(cx[i] + off - L * std::floor((cx[i] + off) / L)));
          x.push_back((float)(cy[i] - L * std::floor(cy[i] / L)));
          x.push_back((float)(cz[i] - L * std::floor(cz[i] / L)));
        }
      }
      writeTrrFrame(f, (int)x.size() / 3, fr, (float)(fr * dt), (float)L, x, std::vector<float>());
      if (fr == 0) check(true, "wrote positions");
    }
    fclose(f);
  }
  std::vector<MsdCurve> ms;
  check(displacement(trr2, t2, 40, ms, err) && ms.size() == 2, "msd for both molecule types");
  for (const MsdCurve& c : ms) {
    DiffusionFit fit = fitDiffusion(c.time, c.msd, 0, 0);
    const std::string what = "D of " + c.label + " within 10% of the walkers' D (" +
                             std::to_string(fit.D) + " vs 0.005 nm2/ps)";
    check(fit.ok && std::fabs(fit.D / D - 1) < 0.10, what.c_str());
  }
  if (ms.size() == 2) {
    // short lags: MSD(t) = 6 D t
    check(std::fabs(ms[0].msd[0] / (6 * D * ms[0].time[0]) - 1) < 0.1, "first lag follows 6 D t");
    std::vector<MsdCurve> back;
    check(decodeMsd(encodeMsd(ms), back) && back.size() == 2 &&
          back[0].time.size() == ms[0].time.size() &&
          std::fabs(back[0].msd.back() / ms[0].msd.back() - 1) < 1e-5, "MSD records round-trip");
  }
  std::vector<SpeedCurve> sb;
  check(decodeSpeed(encodeSpeed(sp), sb) && sb.size() == sp.size() &&
        sb[0].density.size() == sp[0].density.size(), "speed records round-trip");
  DiffusionFit line = fitDiffusion({0, 1, 2, 3, 4}, {0, 6, 12, 18, 24}, 0, 4);
  check(line.ok && std::fabs(line.D - 1) < 1e-12, "fit of an exact line: D = slope/6");
  return failures ? 1 : 0;
}
