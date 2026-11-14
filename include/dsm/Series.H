#ifndef SERIES_HH
#define SERIES_HH

#include <string>
#include <vector>

class EcceURL;

/**
 * A series (#621): one calculation's setup applied to a list of structures,
 * one new calculation each.  Used by ecce-series and the Organizer's
 * "New series from this...".  No wx.
 */
namespace Series {

  /// "file.xyz" or a calculation's URL; charge and multiplicity from the
  /// xyz comment line ("charge=1 mult=3") or the defaults, or from the
  /// calculation.
  struct Outcome {
    std::string name;
    std::string url;      // the new calculation, "" when not made
    bool ok;
    std::string note;     // why not, or a warning
    Outcome() : ok(false) {}
  };

  /// defaultCharge/defaultMult apply to an xyz without its own; a
  /// multiplicity of 0 takes the lowest one the electron count allows.
  /// Each new calculation is the template copied under folder with the
  /// structure's name (code, theory, run type, details, basis set and launch
  /// settings are kept), its results dropped, the basis set rebuilt from the
  /// template's basis name when the elements differ, and its deck generated.
  std::vector<Outcome> make(const EcceURL& templ, const EcceURL& folder,
                            const std::vector<std::string>& sources,
                            int defaultCharge = 0, int defaultMult = 0,
                            const std::string& nameSuffix = "");

  /// "New from This > In Another Code": the first calculation, of the code
  /// with resource type typeName, from one source's final structure, charge
  /// and multiplicity, named source + suffix.  Not Ready: the user sets it up.
  Outcome first(const std::string& typeName, const EcceURL& folder,
                const std::string& source, const std::string& nameSuffix);

  /// Generate the deck of a calculation and set it Ready (true when it is).
  bool makeReady(const EcceURL& calcUrl, std::string& why);
}

#endif
