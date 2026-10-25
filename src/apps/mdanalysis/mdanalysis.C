// Speed distributions and mean-square displacements of a GROMACS run, from
// its topology and .trr/.xtc.  Writes the ecce-parse-md.txt the job parser
// turns into the Builder's Speed Distribution and Displacement panels.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "util/MdAnalysis.H"

namespace {
void usage()
{
  fprintf(stderr,
    "usage: mdanalysis --top run.top (--mdp run.mdp | --tref K)\n"
    "         [--vel file.trr] [--traj file.trr|file.xtc] [--lags N]\n"
    "         [--fit-from ps --fit-to ps] [--out ecce-parse-md.txt]\n"
    "--vel gives the speed distributions, --traj the displacements;\n"
    "a summary goes to standard output.\n");
}
}

int main(int argc, char **argv)
{
  std::string topPath, mdp, vel, traj, outPath, err;
  double tref = 0, fitFrom = 0, fitTo = 0;
  int lags = 120;
  for (int i = 1; i < argc; i++) {
    const std::string a = argv[i];
    const bool more = i + 1 < argc;
    if (a == "--top" && more) topPath = argv[++i];
    else if (a == "--mdp" && more) mdp = argv[++i];
    else if (a == "--tref" && more) tref = atof(argv[++i]);
    else if (a == "--vel" && more) vel = argv[++i];
    else if (a == "--traj" && more) traj = argv[++i];
    else if (a == "--out" && more) outPath = argv[++i];
    else if (a == "--lags" && more) lags = atoi(argv[++i]);
    else if (a == "--fit-from" && more) fitFrom = atof(argv[++i]);
    else if (a == "--fit-to" && more) fitTo = atof(argv[++i]);
    else { usage(); return 2; }
  }
  if (topPath.empty() || (vel.empty() && traj.empty())) { usage(); return 2; }
  MdTopology top;
  if (!readTopology(topPath, top, err)) { fprintf(stderr, "mdanalysis: %s\n", err.c_str()); return 1; }
  if (tref <= 0 && !mdp.empty()) tref = mdpReferenceTemperature(mdp);

  std::string out;
  if (!vel.empty()) {
    std::vector<SpeedCurve> sp;
    if (tref <= 0) { fprintf(stderr, "mdanalysis: no reference temperature (--mdp or --tref)\n"); return 1; }
    if (!speedDistribution(vel, top, tref, 100, sp, err)) { fprintf(stderr, "mdanalysis: %s\n", err.c_str()); return 1; }
    for (const SpeedCurve& c : sp) {
      printf("speeds %-4s mass %.4f u, T_ref %.1f K: most probable %.1f, mean %.1f (MB %.1f), rms %.1f (MB %.1f) m/s\n",
             c.label.c_str(), c.mass, c.refT, mbMostProbable(c.mass, c.refT), c.mean,
             mbMean(c.mass, c.refT), c.rms, mbRms(c.mass, c.refT));
    }
    for (const std::string& r : encodeSpeed(sp)) out += "ECCE-MDSPEED " + r + "\n";
  }
  if (!traj.empty()) {
    std::vector<MsdCurve> ms;
    if (!displacement(traj, top, lags, ms, err)) { fprintf(stderr, "mdanalysis: %s\n", err.c_str()); return 1; }
    for (const MsdCurve& c : ms) {
      DiffusionFit f = fitDiffusion(c.time, c.msd, fitFrom, fitTo);
      if (f.ok)
        printf("msd %-6s %d molecules: D = %.4g x 1e-5 cm2/s (%.4g nm2/ps), fit %.4g-%.4g ps, %d points\n",
               c.label.c_str(), c.molecules, toE5cm2s(f.D), f.D, f.from, f.to, f.points);
    }
    for (const std::string& r : encodeMsd(ms)) out += "ECCE-MDMSD " + r + "\n";
  }
  if (!outPath.empty()) {
    FILE *f = fopen(outPath.c_str(), "wb");
    if (!f || fwrite(out.data(), 1, out.size(), f) != out.size()) {
      fprintf(stderr, "mdanalysis: cannot write %s\n", outPath.c_str());
      return 1;
    }
    fclose(f);
  }
  return 0;
}
