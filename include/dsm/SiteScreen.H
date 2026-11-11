#ifndef SITESCREEN_HH
#define SITESCREEN_HH

#include <string>
#include <vector>

#include "dsm/Series.H"

class EcceURL;

/**
 * MLP site screening (#728): ecce-sites finds the symmetry-distinct
 * interstitial sites or vacancy pairs of a periodic host, Series makes one
 * calculation per site beside the host, and results() collects the finished
 * ones.  Shared by ecce-site-screen and the Organizer.  No wx.
 */
namespace SiteScreen {

  struct Options {
    bool vacancy;
    std::string element;
    double minHost, minSep, maxDist;   // Angstrom; maxDist 0 = no limit
    std::string folderName;            // "" = "<calculation> sites"
    Options() : vacancy(false), minHost(1.6), minSep(1.0), maxDist(0) {}
  };

  struct MadeSites {
    bool ok;
    std::string message;               // why not, or the report
    std::string folderUrl;
    std::vector<Series::Outcome> outcomes;
    MadeSites() : ok(false) {}
  };

  /// "" if the calculation is an MLP one with a periodic structure, else
  /// the sentence that says what is missing (the template of a screen).
  std::string templateProblem(const EcceURL& calc);

  /// True for a calculation whose structure has a cell, whatever its code.
  bool isPeriodic(const EcceURL& calc);

  MadeSites make(const EcceURL& templ, const Options& opt);

  struct Row {
    std::string name, url, state;
    bool host, finished;
    double te;                         // Hartree
    double de;                         // eV against the host
    std::string description;           // the stored "screen=... site=..." text
    std::string env;
    std::string cell;                  // final cell, nine numbers
    Row() : host(false), finished(false), te(0), de(0) {}
  };

  struct Report {
    bool ok;
    std::string message;
    std::string folderUrl, resultsUrl;
    std::vector<Row> rows;             // host first, then sites by dE
    Report() : ok(false) {}
  };

  /// Reads every finished calculation of the folder that holds `calc` (or
  /// is `calc`).  With writeEnsemble, makes or refreshes "<folder> results":
  /// a calculation whose GEOMTRACE holds the final site structures by dE and
  /// whose SITEDE (eV) and SITEENV properties go with them, which the
  /// Builder's ensemble view shows.
  Report results(const EcceURL& calc, bool writeEnsemble);

  /// The value of key in a stored description (`site=3 env="Cl 2.4 x4"`).
  std::string field(const std::string& description, const std::string& key);
}

#endif
