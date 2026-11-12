// ecce-sites: symmetry-distinct interstitial sites or vacancy pairs of a
// periodic host, as a table and (with --out) one structure file per site,
// ready for ecce-series (#728).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include "sites.hpp"

namespace {

const char* USAGE =
    "usage: ecce-sites interstitial <host.xyz> --element X [--min-host A] [--min-sep A]\n"
    "                  [--grid A] [--symprec A] [--out DIR]\n"
    "       ecce-sites vacancy <host.xyz> --element E [--max-dist A] [--symprec A] [--out DIR]\n"
    "       ecce-sites symmetry <host.xyz> [--symprec A]\n"
    "The host is an extended xyz file with a Lattice=\"...\" entry.  --out writes\n"
    "host.xyz, site-NN.xyz (one per symmetry-distinct site) and sites.tsv.\n";

std::string read_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot read " + path);
  std::ostringstream s;
  s << in.rdbuf();
  return s.str();
}

void write_file(const std::string& path, const std::string& text) {
  std::ofstream out(path);
  out << text;
  if (!out) throw std::runtime_error("cannot write " + path);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) { std::cerr << USAGE; return 64; }
  std::string mode = argv[1], path = argv[2], element, outdir;
  mlp::VoidOptions vo;
  double max_dist = 0;
  for (int i = 3; i < argc; ++i) {
    std::string a = argv[i];
    auto num = [&]() { if (i + 1 >= argc) throw std::runtime_error(a + " needs a value"); return std::atof(argv[++i]); };
    try {
      if (a == "--element" && i + 1 < argc) element = argv[++i];
      else if (a == "--out" && i + 1 < argc) outdir = argv[++i];
      else if (a == "--min-host") vo.min_host = num();
      else if (a == "--min-sep") vo.min_sep = num();
      else if (a == "--grid") vo.grid = num();
      else if (a == "--symprec") vo.symprec = num();
      else if (a == "--max-dist") max_dist = num();
      else { std::cerr << "unknown option " << a << "\n" << USAGE; return 64; }
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 64; }
  }
  try {
    mlp::Structure host = mlp::read_extxyz(read_file(path));
    if (!host.periodic) throw std::runtime_error("the host needs a Lattice= entry (a periodic cell)");
    std::vector<mlp::SymOp> ops = mlp::find_symmetry(host, vo.symprec);
    if (mode == "symmetry") {
      std::printf("operations\t%zu\n", ops.size());
      return 0;
    }
    if (element.empty()) throw std::runtime_error("--element is required");
    mlp::mass_of(element);
    if (!outdir.empty()) {
      mkdir(outdir.c_str(), 0777);
      write_file(outdir + "/host.xyz", mlp::write_extxyz(host, "screen=host"));
    }
    std::ostringstream table;
    char buf[512];
    if (mode == "interstitial") {
      std::vector<mlp::Site> sites = mlp::find_voids(host, ops, vo);
      table << "site\tfrac_a\tfrac_b\tfrac_c\tclearance_A\tweight\tcn\tenvironment\n";
      for (size_t s = 0; s < sites.size(); ++s) {
        const mlp::Site& x = sites[s];
        std::string env = x.env.summary();
        std::snprintf(buf, sizeof buf, "%zu\t%.4f\t%.4f\t%.4f\t%.3f\t%d\t%d\t%s\n", s + 1,
                      x.frac[0], x.frac[1], x.frac[2], x.clearance, x.multiplicity,
                      x.env.coordination, env.c_str());
        table << buf;
        if (!outdir.empty()) {
          mlp::Structure t = host;
          t.symbols.push_back(element);
          t.pos.insert(t.pos.end(), {x.cart[0], x.cart[1], x.cart[2]});
          std::snprintf(buf, sizeof buf, "screen=interstitial site=%zu weight=%d cn=%d env=\"%s\"",
                        s + 1, x.multiplicity, x.env.coordination, env.c_str());
          char name[64];
          std::snprintf(name, sizeof name, "/site-%02zu.xyz", s + 1);
          write_file(outdir + name, mlp::write_extxyz(t, buf));
        }
      }
      std::fprintf(stderr, "%zu operations, %zu distinct site(s)\n", ops.size(), sites.size());
    } else if (mode == "vacancy") {
      std::vector<mlp::VacancyPair> pairs = mlp::vacancy_pairs(host, ops, element, max_dist);
      table << "site\tatom_a\tatom_b\tdistance_A\tweight\tenvironment_a\n";
      for (size_t s = 0; s < pairs.size(); ++s) {
        const mlp::VacancyPair& p = pairs[s];
        mlp::Environment env = mlp::environment(host, &host.pos[3 * p.a], 4.0, p.a, p.b);
        std::string e = env.summary();
        std::snprintf(buf, sizeof buf, "%zu\t%d\t%d\t%.3f\t%d\t%s\n", s + 1, p.a + 1, p.b + 1,
                      p.distance, p.multiplicity, e.c_str());
        table << buf;
        if (!outdir.empty()) {
          mlp::Structure t;
          t.periodic = true;
          t.cell = host.cell;
          for (int a = 0; a < host.natoms(); ++a)
            if (a != p.a && a != p.b) {
              t.symbols.push_back(host.symbols[a]);
              t.pos.insert(t.pos.end(), {host.pos[3 * a], host.pos[3 * a + 1], host.pos[3 * a + 2]});
            }
          std::snprintf(buf, sizeof buf,
                        "screen=vacancy site=%zu atoms=%d,%d distance=%.3f weight=%d env=\"%s\"",
                        s + 1, p.a + 1, p.b + 1, p.distance, p.multiplicity, e.c_str());
          char name[64];
          std::snprintf(name, sizeof name, "/site-%02zu.xyz", s + 1);
          write_file(outdir + name, mlp::write_extxyz(t, buf));
        }
      }
      std::fprintf(stderr, "%zu operations, %zu distinct pair(s)\n", ops.size(), pairs.size());
    } else {
      std::cerr << USAGE;
      return 64;
    }
    std::cout << table.str();
    if (!outdir.empty()) write_file(outdir + "/sites.tsv", table.str());
  } catch (const std::exception& e) {
    std::cerr << "ecce-sites: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
