/*
 * ecce-site-screen make <template> interstitial|vacancy --element X
 *                  [--min-host A] [--min-sep A] [--max-dist A] [--folder NAME]
 * ecce-site-screen results <folder or calculation in it> [--json] [--no-ensemble]  (#728)
 *
 * make: the template is an MLP calculation of a periodic host.  One new
 * calculation per symmetry-distinct site (and the host) in the folder
 * "<template> sites", with the template's run type, model and cell setting;
 * launch them with Launch Selected.  results: the finished ones as a table of
 * E - E(host) in eV, lowest first, and (unless --no-ensemble) the calculation
 * "<folder> results" that the Builder shows as an ensemble.
 *
 * Exit 0: done; 1: nothing made or collected; 2: no local data folder; 64: usage.
 */
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "dsm/SiteScreen.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/LocalData.H"

using std::string;

namespace {

bool toUrl(const string& arg, string& url, string& why)
{
  if (arg.find("://") != string::npos) { url = arg; return true; }
  string root = LocalData::dir();
  if (root.empty()) { why = "this session uses a data server: give a URL"; return false; }
  char buf[PATH_MAX], rbuf[PATH_MAX];
  if (!realpath(arg.c_str(), buf)) { why = "no such calculation or folder"; return false; }
  string path = buf;
  string base = realpath(root.c_str(), rbuf) ? string(rbuf) : root;
  if (path.compare(0, base.size() + 1, base + "/") != 0) { why = "not inside the data folder " + base; return false; }
  url = "file://" + path;
  return true;
}

string json(const string& s)
{
  string o = "\"";
  for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
  return o + "\"";
}

const char *USAGE =
    "usage: ecce-site-screen make <template calculation> interstitial|vacancy --element X\n"
    "                         [--min-host A] [--min-sep A] [--max-dist A] [--folder NAME]\n"
    "       ecce-site-screen results <folder or calculation> [--json] [--no-ensemble]\n";

}  // namespace

int main(int argc, char **argv)
{
  if (!getenv("ECCE_REALUSERHOME")) {
    const char *h = getenv("HOME");
    if (h) setenv("ECCE_REALUSERHOME", h, 0);
  }
  std::vector<string> a(argv + 1, argv + argc);
  if (a.size() < 2 || (a[0] != "make" && a[0] != "results")) { std::cerr << USAGE; return 64; }
  Ecce::initialize();
  string url, why;
  if (!toUrl(a[1], url, why)) { std::cout << a[1] << ": " << why << std::endl; return 2; }

  if (a[0] == "make") {
    if (a.size() < 3 || (a[2] != "interstitial" && a[2] != "vacancy")) { std::cerr << USAGE; return 64; }
    SiteScreen::Options o;
    o.vacancy = a[2] == "vacancy";
    for (size_t i = 3; i < a.size(); i++) {
      if (i + 1 >= a.size()) { std::cerr << USAGE; return 64; }
      const string k = a[i], v = a[++i];
      if (k == "--element") o.element = v;
      else if (k == "--min-host") o.minHost = atof(v.c_str());
      else if (k == "--min-sep") o.minSep = atof(v.c_str());
      else if (k == "--max-dist") o.maxDist = atof(v.c_str());
      else if (k == "--folder") o.folderName = v;
      else { std::cerr << USAGE; return 64; }
    }
    SiteScreen::MadeSites m = SiteScreen::make(EcceURL(url), o);
    for (const Series::Outcome& r : m.outcomes)
      std::cout << r.name << (r.ok ? ": made " + r.url : ": not made: " + r.note) << std::endl;
    std::cout << m.message << std::endl;
    if (m.ok) std::cout << "folder: " << m.folderUrl << std::endl;
    return m.ok ? 0 : 1;
  }

  bool asJson = false, ensemble = true;
  for (size_t i = 2; i < a.size(); i++) {
    if (a[i] == "--json") asJson = true;
    else if (a[i] == "--no-ensemble") ensemble = false;
    else { std::cerr << USAGE; return 64; }
  }
  SiteScreen::Report r = SiteScreen::results(EcceURL(url), ensemble);
  if (asJson) {
    std::cout << "{\"ok\":" << (r.ok ? "true" : "false") << ",\"message\":" << json(r.message)
              << ",\"results\":" << json(r.resultsUrl) << ",\"rows\":[";
    for (size_t i = 0; i < r.rows.size(); i++) {
      const SiteScreen::Row& w = r.rows[i];
      char b[160];
      snprintf(b, sizeof b, ",\"te_hartree\":%.10f,\"de_ev\":%.8f", w.te, w.de);
      std::cout << (i ? "," : "") << "{\"name\":" << json(w.name) << ",\"url\":" << json(w.url)
                << ",\"host\":" << (w.host ? "true" : "false") << ",\"state\":" << json(w.state)
                << (w.finished ? b : "") << ",\"description\":" << json(w.description)
                << ",\"environment\":" << json(w.env) << ",\"cell\":" << json(w.cell) << "}";
    }
    std::cout << "]}" << std::endl;
  } else {
    std::cout << r.message << std::endl;
    for (const SiteScreen::Row& w : r.rows) {
      char b[200];
      if (w.finished) snprintf(b, sizeof b, "%-10s %16.8f Ha %12.4f eV", w.name.c_str(), w.te, w.host ? 0.0 : w.de);
      else snprintf(b, sizeof b, "%-10s %s", w.name.c_str(), w.state.c_str());
      std::cout << b << "  " << w.env << std::endl;
    }
  }
  return r.ok ? 0 : 1;
}
