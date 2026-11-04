// Choosing and attaching the fragment calculations of a density difference
// (#726), shared by the Density Map and Density Profile panels.
#ifndef DENSITYFRAGMENTS__H
#define DENSITYFRAGMENTS__H

#include <string>
#include <vector>

#include "tdat/DensityPlane.H"

class IPropCalculation;
class wxWindow;

namespace DensityFragments {

/** Other calculations with MOs in this one's folder (and, inside a project
 *  of projects, its sibling folders); `labels` gets a name for each. */
std::vector<IPropCalculation*> candidates(IPropCalculation *calc,
                                          std::vector<std::string> *labels);

/** URLs of the candidates named by label (as `candidates` gives them), for
 *  scripts; false if a name is not found. */
bool resolve(IPropCalculation *calc, const std::vector<std::string>& names,
             std::vector<std::string>& urls, std::string& complaint);

/** Builds an Evaluator for each URL and makes `whole` the difference
 *  from them.  On any refusal `whole` is left as a plain density and
 *  `complaint` says why. */
bool attach(DensityPlane::Evaluator& whole, const std::vector<std::string>& urls,
            std::string& complaint);

/** The URLs of the calculations the user ticks; false on Cancel.  A fragment
 *  that cannot be matched is reported when it is attached, not listed here. */
bool choose(wxWindow *parent, IPropCalculation *calc,
            const std::vector<std::string>& current, std::vector<std::string>& chosen);

}  // namespace DensityFragments

#endif
