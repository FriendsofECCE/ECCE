#include "basislib.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "common.hpp"
#include "molecule.hpp"

#ifndef ECCE_QM_DEFAULT_BASIS_DIR
#define ECCE_QM_DEFAULT_BASIS_DIR ""
#endif

namespace qm {

static bool is_dir(const std::string& p) {
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

BasisLibrary::BasisLibrary(const std::string& dir) : dir_(dir) {
  if (!dir_.empty()) return;
  std::vector<std::string> cand;
  if (const char* e = std::getenv("ECCE_BASIS_DIR")) cand.push_back(e);
  if (const char* h = std::getenv("ECCE_HOME")) cand.push_back(std::string(h) + "/data/admin/basissets");
  cand.push_back(ECCE_QM_DEFAULT_BASIS_DIR);
  for (const auto& c : cand)
    if (!c.empty() && is_dir(c)) { dir_ = c; return; }
  // Not an error yet: an input that carries its own basis (ECCE writes one)
  // needs no library.  find_file() complains when one is asked for.
  dir_.clear();
}

std::vector<std::string> BasisLibrary::expand_name(const std::string& name) {
  std::vector<std::string> out;
  std::stringstream ss(name);
  std::string part;
  while (std::getline(ss, part, '+')) {
    if (part.empty()) continue;
    // A trailing "+" belongs to Pople names such as 6-31+G*; only split when
    // the pieces are separated by "+" on both sides of a letter-led token.
    out.push_back(part);
  }
  // Pople-style "+" (diffuse) names are not combined files: keep them whole.
  if (name.find("+G") != std::string::npos || name.find("++") != std::string::npos) out = {name};
  std::vector<std::string> files;
  for (auto& p : out) {
    size_t n = 0;
    while (!p.empty() && p.back() == '*') { p.pop_back(); ++n; }
    files.push_back(p);
    if (n) files.push_back(p + std::string(n, 'S'));
  }
  return files;
}

static std::string norm_key(const std::string& s) {
  std::string k;
  for (char c : s) k += (c == '-' ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  return k;
}

std::string BasisLibrary::find_file(const std::string& base) const {
  if (dir_.empty()) throw Error("basis library directory not found; set ECCE_BASIS_DIR or pass --basis-dir");
  std::string exact = dir_ + "/" + base + ".BAS";
  struct stat st;
  if (stat(exact.c_str(), &st) == 0) return exact;
  DIR* d = opendir(dir_.c_str());
  if (!d) throw Error("cannot read basis directory " + dir_);
  std::string want = norm_key(base) + ".bas", best;
  while (dirent* e = readdir(d)) {
    std::string f = e->d_name;
    if (norm_key(f) == want && (best.empty() || f < best)) best = f;
  }
  closedir(d);
  if (best.empty()) throw Error("basis set file for '" + base + "' not found in " + dir_);
  return dir_ + "/" + best;
}

static int l_of(char c) {
  static const std::string L = "SPDFGHIKLM";
  size_t i = L.find(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
  if (i == std::string::npos) throw Error(std::string("bad shell letter ") + c);
  // Letters run S P D F G H I then K L M (J is skipped by convention).
  return static_cast<int>(i);
}

static double read_double(const std::string& tok) {
  std::string t = tok;
  for (auto& c : t) if (c == 'D' || c == 'd') c = 'E';
  return std::strtod(t.c_str(), nullptr);
}

const BasisLibrary::Table& BasisLibrary::load_file(const std::string& base) {
  auto it = cache_.find(base);
  if (it != cache_.end()) return it->second;
  std::string path = find_file(base);
  std::ifstream in(path);
  if (!in) throw Error("cannot open " + path);
  Table t;
  std::string line;
  int Z = 0;
  while (std::getline(in, line)) {
    if (line.compare(0, 5, "atom=") == 0) {
      std::string s = line.substr(5);
      s.erase(s.find_last_not_of(" \t\r\n") + 1);
      Z = atomic_number(s);
    } else if (line.compare(0, 12, "contraction ") == 0) {
      size_t p = line.find("shell=");
      std::string letters = line.substr(p + 6, line.find(' ', p) - p - 6);
      int np = 0, nc = 0;
      std::sscanf(line.c_str() + line.find("num_primitives="), "num_primitives=%d num_coefficients=%d", &np, &nc);
      if (static_cast<int>(letters.size()) != nc) throw Error("shell/coefficient count mismatch in " + path);
      std::vector<ShellDef> cols(nc);
      for (int c = 0; c < nc; ++c) cols[c].l = l_of(letters[c]);
      for (int k = 0; k < np; ++k) {
        if (!std::getline(in, line)) throw Error("truncated basis file " + path);
        std::stringstream ss(line);
        std::string tok;
        ss >> tok;
        double e = read_double(tok);
        for (int c = 0; c < nc; ++c) {
          if (!(ss >> tok)) throw Error("short contraction row in " + path);
          cols[c].exps.push_back(e);
          cols[c].coefs.push_back(read_double(tok));
        }
      }
      for (auto& c : cols) t[Z].push_back(std::move(c));
    }
  }
  return cache_.emplace(base, std::move(t)).first->second;
}

std::vector<ShellDef> BasisLibrary::shells(const std::string& name, int Z) {
  std::vector<ShellDef> out;
  bool any = false;
  if (!explicit_.empty()) {
    auto e = explicit_.find(Z);
    if (e == explicit_.end())
      throw Error("basis '" + name + "' has no functions for element " + element_symbol(Z));
    return e->second;
  }
  auto files = expand_name(name);
  for (size_t i = 0; i < files.size(); ++i) {
    // Polarisation files omit light elements; the first file must cover Z.
    const Table& t = load_file(files[i]);
    // A .POT file next to the .BAS lists the elements that come with an
    // effective core potential (def2 beyond Kr); ecce-qm has none.
    {
      std::string pot = find_file(files[i]);
      pot.replace(pot.size() - 3, 3, "POT");
      std::ifstream pf(pot);
      std::string pl, want = std::string("atom=") + element_symbol(Z);
      while (pf && std::getline(pf, pl)) {
        if (pl.compare(0, want.size(), want) == 0 && (pl.size() == want.size() || pl[want.size()] == ' '))
          throw Error("basis '" + name + "' needs an effective core potential for " +
                      element_symbol(Z) + ", which ecce-qm does not support");
      }
    }
    auto it = t.find(Z);
    if (it == t.end()) continue;
    any = true;
    out.insert(out.end(), it->second.begin(), it->second.end());
  }
  if (!any) throw Error("basis '" + name + "' has no functions for element " + element_symbol(Z));
  return out;
}

}  // namespace qm
