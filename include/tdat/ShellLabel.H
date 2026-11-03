/**
 * @file
 *
 * Names for the atomic shells an orbital is made of, with the principal
 * quantum number: "F 1s", "F 2s", "H(1) 1s" (#827).  No wx.
 *
 * The AO basis only says "the nth shell of this l on this atom", which is
 * not a quantum number: HF's 1s-dominated MO and its 2s-dominated MO are
 * both "the s shell of F".  n comes from the element instead.  An orbital
 * that is (nearly) all of one element's AOs of one l and sits among the
 * deepest of them is a core orbital of that element, and takes the core
 * shells in energy order (1s, 2s, 2p, 3s, ...); everything else takes the
 * valence n of its l.
 */

#ifndef SHELLLABEL_HH
#define SHELLLABEL_HH

#include <string>
#include <utility>
#include <vector>

class ShellLabel
{
  public:

    /** Period (row) of an element symbol, 0 if unknown. */
    static int period(const std::string& element);

    /**
     * Principal quantum number of a valence shell of angular momentum l:
     * the period for s and p, period-1 for d, period-2 for f, but never
     * below l+1 (a hydrogen p or a carbon d is polarisation: 2p, 3d).
     */
    static int valenceN(const std::string& element, int l);

    /** The (n, l) shells of the preceding noble gas, lowest first. */
    static std::vector< std::pair<int,int> > coreShells(const std::string& element);

    /** "F 2p" (n, l letter); l above 7 is written as '?'. */
    static std::string shellName(const std::string& element, int n, int l);

    /**
     * Atom names for a list of element symbols: the symbol alone, or
     * "H(1)", "H(2)" (numbered within the element) when the element
     * occurs more than once.
     */
    static std::vector<std::string> atomNames(const std::vector<std::string>& elements);

    /** One piece of an orbital: its share on the AOs of one l of one atom. */
    struct Part
    {
      int atom;
      int l;
      double share;
      Part() : atom(-1), l(0), share(0.0) {}
      Part(int a, int ll, double s) : atom(a), l(ll), share(s) {}
    };

    /** What an orbital is the core shell of, if anything. */
    struct Core
    {
      bool core;
      std::string element;
      int l;
      int n;
      Core() : core(false), l(0), n(0) {}
    };

    /**
     * Which of the orbitals, given in energy order, are core orbitals
     * and of which shell.  An orbital whose AOs of one l on atoms of one
     * element add up to `threshold` or more is the next one of that
     * (element, l); the first (2l+1) x (atoms of the element) x
     * (core shells of that l) of them are core, in shell order, the rest
     * valence.  Scanning stops once every possible core orbital has been
     * seen, so only the deepest orbitals need to be passed.
     */
    static std::vector<Core> coreOrbitals(
        const std::vector<std::string>& elements,
        const std::vector< std::vector<Part> >& orbitals,
        double threshold = 0.9);

    /**
     * The name of one part of an orbital: "F 1s" if the orbital is a core
     * orbital of that part's element and l, otherwise the valence shell.
     */
    static std::string partName(const std::vector<std::string>& elements,
                                const std::vector<std::string>& names,
                                const Part& part, const Core& core);

    /** The number of orbitals the noble-gas cores of these atoms can hold. */
    static int coreCapacity(const std::vector<std::string>& elements);
};

#endif
