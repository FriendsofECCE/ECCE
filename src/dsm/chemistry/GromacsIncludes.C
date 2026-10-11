#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>

#include "dsm/GromacsIncludes.H"

namespace fs = std::filesystem;

static const char *SEP = "__SLASH__";

string GromacsIncludes::encodeName(const string& relativePath)
{
  string out;
  for (size_t i = 0; i < relativePath.size(); i++) {
    if (relativePath[i] == '/') out += SEP;
    else out += relativePath[i];
  }
  return out;
}

string GromacsIncludes::decodeName(const string& storedName)
{
  string out = storedName;
  string sep(SEP);
  size_t pos;
  while ((pos = out.find(sep)) != string::npos) out.replace(pos, sep.size(), "/");
  return out;
}

// The target of an #include line, or "" for any other line.
static string includeTarget(const string& line)
{
  size_t i = line.find_first_not_of(" \t");
  if (i == string::npos || line[i] != '#') return "";
  i = line.find_first_not_of(" \t", i + 1);
  if (i == string::npos || line.compare(i, 7, "include") != 0) return "";
  i = line.find_first_not_of(" \t", i + 7);
  if (i == string::npos || (line[i] != '"' && line[i] != '<')) return "";
  char close = line[i] == '"' ? '"' : '>';
  size_t end = line.find(close, i + 1);
  if (end == string::npos) return "";
  return line.substr(i + 1, end - i - 1);
}

// GROMACS's own library directories, where grompp looks after the current one.
static vector<fs::path> libraryDirs()
{
  vector<fs::path> dirs;
  const char *lib = getenv("GMXLIB");
  if (lib && *lib) dirs.push_back(lib);
  const char *data = getenv("GMXDATA");
  if (data && *data) dirs.push_back(fs::path(data) / "top");
  dirs.push_back("/usr/share/gromacs/top");
  dirs.push_back("/usr/local/gromacs/share/gromacs/top");
  return dirs;
}

static void follow(const fs::path& file, const fs::path& topDir,
                   const vector<fs::path>& lib, std::set<string>& visited,
                   vector<GromacsIncludes::File>& found,
                   vector<string>& system, vector<string>& missing)
{
  std::ifstream in(file);
  string line;
  while (std::getline(in, line)) {
    string target = includeTarget(line);
    if (target.empty()) continue;

    // Next to the including file first, then next to the topology.
    fs::path hit;
    fs::path cands[2] = { file.parent_path() / target, topDir / target };
    for (int c = 0; c < 2 && hit.empty(); c++) {
      std::error_code ec;
      if (fs::is_regular_file(cands[c], ec)) hit = cands[c];
    }

    if (hit.empty()) {
      bool inLib = false;
      for (size_t d = 0; d < lib.size() && !inLib; d++) {
        std::error_code ec;
        inLib = fs::is_regular_file(lib[d] / target, ec);
      }
      vector<string>& list = inLib ? system : missing;
      if (std::find(list.begin(), list.end(), target) == list.end())
        list.push_back(target);
      continue;
    }

    string rel = hit.lexically_normal().lexically_relative(
                   topDir.lexically_normal()).generic_string();
    if (rel.empty() || rel.compare(0, 3, "../") == 0) {
      // Outside the topology's folder: it cannot be recreated beside it.
      if (std::find(missing.begin(), missing.end(), target) == missing.end())
        missing.push_back(target);
      continue;
    }
    if (!visited.insert(rel).second) continue;
    GromacsIncludes::File f;
    f.name = rel;
    f.path = hit.string();
    found.push_back(f);
    follow(hit, topDir, lib, visited, found, system, missing);
  }
}

void GromacsIncludes::collect(const string& topPath, vector<File>& found,
                              vector<string>& system, vector<string>& missing)
{
  fs::path top(topPath);
  fs::path topDir = top.parent_path();
  if (topDir.empty()) topDir = ".";
  std::set<string> visited;
  follow(top, topDir, libraryDirs(), visited, found, system, missing);
}
