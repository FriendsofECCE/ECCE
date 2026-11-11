// ecce-mlp-driver <deck> [python ecce-mlp.py]
//
// Runs an ecce-mlp deck (see scripts/ecce-mlp) with ECCE's own optimisers
// and dynamics; scripts/ecce-mlp starts it with its own interpreter and path
// so that the worker it spawns is the same script.  ECCE_MLP_MOCK=lj uses
// the built-in Lennard-Jones and needs no Python at all.  The output is
// what scripts/parsers/mlp.desc reads.
#include <chrono>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "mlpdriver.hpp"

namespace {

const double EV_TO_HARTREE = 1.0 / 27.211386245988;
const double EV_A3_TO_KBAR = 1602.1766208;
const double GPA_TO_EV_A3 = 1.0 / 160.21766208;

[[noreturn]] void fail(const std::string& text) {
  std::printf("ecce-mlp: ERROR: %s\n", text.c_str());
  std::fflush(stdout);
  std::exit(2);
}

void say(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void say(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::vprintf(fmt, ap);
  va_end(ap);
  std::printf("\n");
  std::fflush(stdout);
}

std::map<std::string, std::string> opts;

std::string opt(const std::string& key, const std::string& def) {
  auto it = opts.find(key);
  return it == opts.end() ? def : it->second;
}
double num(const std::string& key, double def) {
  auto it = opts.find(key);
  if (it == opts.end()) return def;
  char* end = nullptr;
  double v = std::strtod(it->second.c_str(), &end);
  if (end == it->second.c_str() || *end) fail(key + "=" + it->second + " is not a number");
  return v;
}

void frame_block(int index, const mlp::Structure& s, double energy, double time_ps = -1) {
  say("ECCE-MLP-FRAME %d %d", index, s.natoms());
  say("energy: %.10f", energy);
  if (time_ps >= 0) say("time: %.6f", time_ps);
  if (s.periodic) {
    std::string c = "cell:";
    char buf[32];
    for (double v : s.cell) std::snprintf(buf, sizeof buf, " %.10f", v), c += buf;
    say("%s", c.c_str());
  }
  for (int a = 0; a < s.natoms(); ++a)
    say("%-2s %16.10f %16.10f %16.10f", s.symbols[a].c_str(), s.pos[3 * a], s.pos[3 * a + 1],
        s.pos[3 * a + 2]);
  say("ECCE-MLP-ENDFRAME");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 && argc != 4) fail("usage: ecce-mlp-driver <deck> [python ecce-mlp.py]");
  std::ifstream in(argv[1]);
  if (!in) fail(std::string("cannot read ") + argv[1]);
  std::string first, body, line;
  std::getline(in, first);
  while (std::getline(in, line)) body += line + "\n";
  {
    std::istringstream words(first);
    std::string w;
    words >> w;
    if (w != "mlp") fail("line 1 of the deck must start with \"mlp\" followed by key=value options");
    while (words >> w) {
      size_t eq = w.find('=');
      opts[w.substr(0, eq)] = eq == std::string::npos ? "" : w.substr(eq + 1);
    }
  }
  const std::string runtype = opt("runtype", "Energy");
  if (runtype != "Energy" && runtype != "Geometry" && runtype != "Vibration" && runtype != "MD")
    fail("runtype must be Energy, Geometry, Vibration or MD");
  const double delta = num("delta", 0.01);
  if (!(delta > 0 && delta <= 0.1)) fail("delta must be between 0 and 0.1 A");
  mlp::OptOptions oo;
  oo.fmax = num("fmax", 0.05);
  oo.steps = static_cast<int>(num("steps", 200));
  oo.timelimit = num("timelimit", 0) * 60;
  oo.method = opt("optimizer", "FIRE");
  if (oo.method != "FIRE" && oo.method != "BFGS") fail("optimizer must be FIRE or BFGS");
  oo.cell = opt("relax", "atoms") == "cell";
  const double pressure_gpa = num("pressure", 0);
  oo.pressure = pressure_gpa * GPA_TO_EV_A3;
  mlp::MDOptions mo;
  mo.ensemble = opt("ensemble", "NVE");
  if (mo.ensemble != "NVE" && mo.ensemble != "NVT") fail("ensemble must be NVE or NVT");
  mo.dt_fs = num("timestep", 1.0);
  mo.temperature = num("temperature", 300);
  mo.tau_fs = num("tau", 100);
  mo.steps = static_cast<int>(num("mdsteps", 1000));
  mo.every = std::max(1, static_cast<int>(num("every", 10)));
  mo.seed = static_cast<unsigned>(num("seed", 1));
  mo.timelimit = oo.timelimit;
  if (runtype == "MD" && !(mo.dt_fs > 0 && mo.dt_fs <= 10)) fail("timestep must be between 0 and 10 fs");
  if (runtype == "MD" && mo.temperature < 0) fail("temperature must not be negative");
  if (runtype == "MD" && mo.steps < 1) fail("mdsteps must be at least 1");

  mlp::Structure s;
  try {
    s = mlp::read_extxyz(body);
  } catch (const std::exception& e) {
    fail(std::string("cannot read the structure in the deck: ") + e.what());
  }
  if (oo.cell && runtype == "Geometry" && !s.periodic)
    fail("relax=cell needs a periodic structure; this one has no cell");

  const auto start = std::chrono::steady_clock::now();
  auto elapsed = [&] {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  };

  const char* mock = std::getenv("ECCE_MLP_MOCK");
  std::unique_ptr<mlp::Worker> worker;
  mlp::Evaluator eval;
  std::string name, version, licence, weights = "-";
  if (mock && std::string(mock) == "lj") {
    eval = [](const mlp::Structure& t) { return mlp::lennard_jones(t); };
    name = "mock (Lennard-Jones, built in)";
    version = "ecce-mlp-driver";
    licence = "none (test calculator)";
  } else {
    if (argc != 4) fail("the potential needs the Python worker: ecce-mlp-driver <deck> <python> <ecce-mlp.py>");
    try {
      worker.reset(new mlp::Worker({argv[2], argv[3], "--worker"}, s, opt("model", "medium-mpa-0"),
                                   opt("dtype", "float32"), opt("dispersion", "off")));
    } catch (const std::exception& e) {
      fail(e.what());
    }
    eval = [&](const mlp::Structure& t) { return worker->evaluate(t); };
    name = worker->name, version = worker->version, licence = worker->licence,
    weights = worker->weights;
  }

  say("ecce-mlp 1.0");
  say("Model:        %s", name.c_str());
  say("Version:      %s", version.c_str());
  say("Licence:      %s", licence.c_str());
  if (licence == "ASL")
    say("Notice:       This model is under the Academic Software License (no commercial use).");
  say("Precision:    %s", opt("dtype", "float32").c_str());
  say("Dispersion:   %s", opt("dispersion", "off").c_str());
  say("Engine:       ECCE (ecce-mlp-driver)");
  say("Run type:     %s", runtype.c_str());
  say("Atoms:        %d   periodic: %s", s.natoms(), s.periodic ? "yes" : "no");
  if (runtype == "Geometry") {
    say("Optimiser:    %s%s", oo.method.c_str(),
        oo.cell ? ", atoms and cell (exponential cell parametrisation)" : ", atoms only");
    if (oo.cell && pressure_gpa != 0) say("Pressure:     %g GPa", pressure_gpa);
    say("convergence: fmax %g eV/A", oo.fmax);
    if (oo.timelimit > 0)
      say("Limits:       %d steps, %g minutes", oo.steps, oo.timelimit / 60);
    else
      say("Limits:       %d steps", oo.steps);
  }
  if (runtype == "MD") {
    say("Dynamics:     %s, velocity Verlet, time step %g fs, %d steps, a frame every %d steps",
        mo.ensemble.c_str(), mo.dt_fs, mo.steps, mo.every);
    if (mo.ensemble == "NVT")
      say("Temperature:  %g K (Bussi thermostat, time constant %g fs)", mo.temperature, mo.tau_fs);
    else
      say("Temperature:  %g K (initial velocities only)", mo.temperature);
    if (mo.timelimit > 0) say("Limits:       %g minutes", mo.timelimit / 60);
  }
  if (runtype == "Vibration")
    say("Hessian:      central differences of the forces, step %g A, %d force calls", delta,
        6 * s.natoms());
  if (weights != "-") say("Weights:      cached in %s", weights.c_str());
  say("Note:         energies are for screening, not for reporting.");
  say("%s", "");

  std::remove("ecce-mlp.extxyz");
  std::ofstream traj("ecce-mlp.extxyz");
  int frames = 0;
  auto record = [&](const mlp::Structure& t, const mlp::Result& r, double fm) {
    ++frames;
    traj << mlp::extxyz_frame(t, r.energy) << std::flush;
    say("Step %4d", frames - 1);
    say("  Energy %.8f eV", r.energy);
    say("  Fmax   %.5f eV/A", fm);
    say("  Time   %.1f s", elapsed());
    frame_block(frames, t, r.energy);
  };

  mlp::Structure last = s;
  mlp::Result result;
  std::string stopped;
  bool converged = true;
  int opt_steps = 0;
  try {
    if (runtype == "Energy" || runtype == "Vibration") {
      result = eval(s);
      double fm = mlp::max_row_norm(result.forces);
      record(s, result, fm);
      if (runtype == "Vibration") {
        if (fm > 0.05)
          say("Warning:      the forces are not small (Fmax %.3f eV/A); the frequencies are only "
              "meaningful at a minimum.", fm);
        mlp::Vibrations v = mlp::vibrations(s, delta, eval, [&](int i, int n) {
          say("Displacement %d of %d   Time %.1f s", i, n, elapsed());
        });
        const int N = 3 * s.natoms();
        say("%s", "");
        say("ECCE-MLP-VIB %d %d", N, s.natoms());
        for (int k = 0; k < N; ++k) {
          say("mode %d %.4f", k + 1, v.freq[k]);
          for (int a = 0; a < s.natoms(); ++a)
            say("%16.10f %16.10f %16.10f", v.modes[N * k + 3 * a], v.modes[N * k + 3 * a + 1],
                v.modes[N * k + 3 * a + 2]);
        }
        say("ECCE-MLP-ENDVIB");
        say("%s", "");
        say("Frequencies (cm-1; negative = imaginary), %d rigid motions removed:", v.rigid);
        int nimag = 0;
        for (int k = v.rigid; k < N; k += 6) {
          std::string row = " ";
          char buf[32];
          for (int j = k; j < std::min(N, k + 6); ++j) {
            std::snprintf(buf, sizeof buf, " %10.2f", v.freq[j]), row += buf;
          }
          say("%s", row.c_str());
        }
        for (int k = v.rigid; k < N; ++k) nimag += v.freq[k] < -1e-6;
        if (nimag) say("Imaginary frequencies: %d", nimag);
      }
    } else if (runtype == "Geometry") {
      mlp::OptOutcome o = mlp::optimize(
          s, oo, eval,
          [&](int, const mlp::Structure& t, const mlp::Result& r, double fm) { record(t, r, fm); },
          elapsed);
      last = o.s, result = o.r, converged = o.converged, stopped = o.stopped, opt_steps = o.steps;
    } else {
      mlp::MDOutcome o = mlp::molecular_dynamics(
          s, mo, eval,
          [&](const mlp::MDFrame& f, const mlp::Structure& t, const mlp::Result& r) {
            ++frames;
            char extra[64];
            std::snprintf(extra, sizeof extra, " time=%.6f", f.time_ps);
            traj << mlp::extxyz_frame(t, r.energy, extra) << std::flush;
            say("MD step %6d  time %.4f ps  Epot %.8f eV  Ekin %.8f eV  T %.2f K  Etot %.8f eV  "
                "Time %.1f s", f.step, f.time_ps, f.epot, f.ekin, f.temperature, f.epot + f.ekin,
                elapsed());
            frame_block(frames, t, r.energy, f.time_ps);
          },
          elapsed);
      last = o.s, result = o.r, stopped = o.stopped;
      say("%s", "");
      say("MD finished: %d steps, mean temperature %.2f K, total energy drift %.3e eV/atom/ps",
          o.steps, o.mean_temperature, o.drift_ev_atom_ps);
    }
  } catch (const std::exception& e) {
    fail(e.what());
  }
  if (runtype == "Energy" || runtype == "Vibration") last = s;

  say("%s", "");
  say("ECCE-MLP-FINAL");
  say("energy: %.10f", result.energy);
  say("forces:");
  for (int a = 0; a < last.natoms(); ++a)
    say("%-2s %16.10f %16.10f %16.10f", last.symbols[a].c_str(), result.forces[3 * a],
        result.forces[3 * a + 1], result.forces[3 * a + 2]);
  if (last.periodic && result.has_stress) {
    say("stress (kbar):");
    const auto& v = result.stress;
    double full[9] = {v[0], v[5], v[4], v[5], v[1], v[3], v[4], v[3], v[2]};
    for (int i = 0; i < 3; ++i)
      say("%16.8f %16.8f %16.8f", -full[3 * i] * EV_A3_TO_KBAR, -full[3 * i + 1] * EV_A3_TO_KBAR,
          -full[3 * i + 2] * EV_A3_TO_KBAR);
  }
  say("ECCE-MLP-ENDFINAL");
  say("%s", "");
  if (runtype == "Geometry") {
    if (converged)
      say("Optimisation converged after %d steps.", opt_steps);
    else
      say("ecce-mlp: NOT CONVERGED: stopped at the %s; the last structure is kept.", stopped.c_str());
  }
  if (runtype == "MD" && !stopped.empty())
    say("ecce-mlp: STOPPED: the %s was reached; the last structure is kept.", stopped.c_str());
  say("Total energy: %.8f eV (%.8f Hartree)", result.energy, result.energy * EV_TO_HARTREE);
  worker.reset();
  say("ecce-mlp terminated normally");
  return 0;
}
