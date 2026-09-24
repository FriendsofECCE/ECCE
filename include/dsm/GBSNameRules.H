///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: GBSNameRules.H
//
// CLASS SYNOPSIS:
//    Two rules about basis-set NAMES that the Basis Set Tool needs and that
//    have nothing to do with wx, WebDAV or the rest of ECCE:
//
//      * matchesFilter() -- the substring test behind the Basis Set Tool's
//        per-list filter box (#117).  377 sets in a flat alphabetical list
//        box is unusable; element coverage alone still leaves 176 of them
//        for an ordinary organic molecule.
//
//      * pairedECP() -- which entry of the library's `ecp` index goes with
//        a given entry of its `ECPOrbital` index (#122).  Picking
//        def2-svp for a molecule with heavy atoms essentially always means
//        wanting def2-svp-ecp too, and nothing connected the two.
//
//    Kept free of every ECCE dependency (only <string>/<vector>) so the
//    rules can be unit tested against the shipped library with a plain
//    g++ compile -- see tests/basis/testBasisNameRules.C.  ONE
//    implementation, exercised by both the app and the test: the library
//    index files and a reimplementation of these rules in the test would
//    drift apart, which is the trap CLAUDE.md records for the dialog /
//    generator keyword lists.
//
// SEE ALSO:
//    EDSIGaussianBasisSetLibrary.H, src/apps/basistool/WxBasisTool.C
///////////////////////////////////////////////////////////////////////////////

#ifndef GBSNAMERULES_H
#define GBSNAMERULES_H

#include <string>
#include <vector>

class GBSNameRules
{
  public:

    /**
     * Does a basis set name match what the user typed into a filter box?
     *
     * The filter is whitespace separated and every term has to appear,
     * case insensitively, somewhere in the name -- so "cc pp" finds
     * cc-pVTZ-PP and "31 3df" finds 6-31G(3df,3pd).  An empty (or
     * all-whitespace) filter matches everything, which is how "show me
     * all of them" stays reachable.
     */
    static bool matchesFilter(const std::string& name,
                              const std::string& filter);

    /**
     * The ECP that belongs with an orbital basis set, or "" if there is
     * none.
     *
     * @param basisName  name of the ECPOrbital entry, as the index spells it
     * @param basisFiles that entry's files= list (a .BAS plus a .POT)
     * @param ecpNames   name= of every entry in the ecp index
     * @param ecpFiles   the first files= entry of each, same order
     *
     * Three rules, tried in order, all discovered from the shipped data
     * rather than hardcoded per family:
     *
     *   1. the "<basis>-ecp" naming convention.  This is the ONLY thing
     *      that finds the def2 pairs: every def2 orbital record points at
     *      one shared DEF2_ECP.POT, which the ecp index does not list at
     *      all, while the per-basis def2-svp.POT ... files it does list
     *      are reachable by name only.
     *   2. the same name in both indexes (LANL2DZ ECP, CRENBL ECP,
     *      Stuttgart RLC ECP ... are each listed twice, once as the
     *      orbital set and once as the potential).
     *   3. a shared .POT file, which is what pairs the correlation
     *      consistent pseudopotential sets (cc-pVDZ-PP and the rest all
     *      name STUTTGART-MCDHF-RSC.POT) with their ECP.
     *
     * Returns the ecp index's own spelling of the name, so the caller can
     * hand it straight back to the library.
     */
    static std::string pairedECP(const std::string& basisName,
                                 const std::vector<std::string>& basisFiles,
                                 const std::vector<std::string>& ecpNames,
                                 const std::vector<std::string>& ecpFiles);

  private:
    static std::string lower(const std::string& s);
};

#endif
