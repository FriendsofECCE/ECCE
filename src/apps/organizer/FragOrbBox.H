/**
 * @file
 *
 * The Fragment orbitals box of a workflow (#201): the complex's frontier
 * orbitals expanded in the orbitals of its fragments (FOs), for workflows
 * made by the Fragment analysis recipe.  The numbers are `ecce-fragorb
 * --json`'s (FragOrbRun.H); this file picks the steps, groups degenerate
 * levels and words the result.  No wx.
 */

#ifndef FRAGORBBOX_HH
#define FRAGORBBOX_HH

#include <string>
#include <utility>
#include <vector>

#include "util/FragOrbRun.H"

/// A step of a fragment analysis: the complex (fragment 0) or a fragment.
struct FragStep {
  std::string name, url, stamp;
  std::string formula;       ///< of its structure, "Cr", "C6O6"
  std::string atoms;         ///< "2-13" (a fragment's atoms in the complex)
  int fragment = 0;          ///< 0 the complex, 1.. a fragment
  int charge = 0, mult = 1;
  bool complete = false;
  std::string state;         ///< "failed" etc. when it will not complete
};

/// Degenerate FOs of one fragment (within 0.003 eV) taken together.
struct FragLevel {
  int fragment = 0;
  int first = 0, last = 0;   ///< FO positions in the channel (0-based)
  double energy = 0, occupation = 0;
  std::string label;         ///< "Cr HOMO", "C6O6 LUMO+1"
};

struct FragOrbBoxResult {
  bool applicable = false;   ///< the workflow has a complex and fragment steps
  bool ready = false;        ///< ecce-fragorb answered
  std::string level;         ///< "Cr (charge 0) | C6O6 (charge 0); Mulliken"
  std::string note;          ///< what is awaited, or ecce-fragorb's refusal
  std::vector<std::pair<std::string, std::string> > rows;   ///< label, value
  FragOrbResult data;
  std::vector<std::string> names;   ///< display name per fragment
  std::string text() const;         ///< the box as plain lines
};

/// The complex's folder and the fragments' folders, in fragment order;
/// false when the steps have no complex or fewer than two fragments.
/// `waiting` names the steps that are not finished.
bool fragOrbRequest(const std::vector<FragStep>& steps, std::string& complex,
                    std::vector<std::string>& fragments, std::vector<std::string>& names,
                    std::string& waiting);

FragOrbBoxResult fragOrbBox(const std::vector<FragStep>& steps);

/// The FO levels of a channel, in FO order.
std::vector<FragLevel> fragLevels(const FragOrbResult& r, size_t channel,
                                  const std::vector<std::string>& names);

/// Every MO of every channel with its FO levels of 1 % or more.
std::string fragOrbDetailsText(const FragOrbResult& r, const std::vector<std::string>& names);

/// The whole composition matrix as CSV: one row per MO, one column per FO.
std::string fragOrbCsv(const FragOrbResult& r, const std::vector<std::string>& names);

/// A percentage as "71 %".
std::string fragPercent(double share);

#endif
