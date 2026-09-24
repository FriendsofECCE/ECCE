/**
 * @file
 *
 * The layout of a molecular orbital correlation diagram.
 *
 * Deliberately separate from anything that draws: what to put where is
 * decided here, in terms of levels, labels and connections, and the
 * panel only paints the result.  That keeps the part with the judgement
 * in it -- which orbitals share a level, which count as core, what
 * connects to what -- testable without a display.
 */

#ifndef MODIAGRAM_HH
#define MODIAGRAM_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

/** One drawn level: a set of orbitals sharing an energy and a label. */
struct MoLevel
{
   double         energy;          // Hartree
   string         label;           // the irrep, as the code reports it
   double         occupancy;       // electrons in the whole level
   int            degeneracy;      // orbitals drawn as this one level
   vector<int>    orbitals;        // their indices, for clicking through

   MoLevel() : energy(0.0), occupancy(0.0), degeneracy(0) {}
};


/** One column of the diagram. */
struct MoColumn
{
   string           title;
   vector<MoLevel>  levels;

   /**
    * Orbitals folded away below the energy cutoff.
    *
    * Drawn as a single band rather than as levels: a molecule with
    * thirty core orbitals squeezes its valence region into nothing
    * otherwise.  Kept as a count so the diagram can SAY how many are
    * hidden -- an absence the reader cannot see is worse than a
    * cluttered diagram.
    */
   int    hiddenCount;
   double hiddenMaxEnergy;

   MoColumn() : hiddenCount(0), hiddenMaxEnergy(0.0) {}
};


/** A line between a fragment level and a molecular orbital. */
struct MoConnection
{
   int leftLevel;     // index into the left column, or -1
   int centreLevel;   // index into the centre column
   int rightLevel;    // index into the right column, or -1
};


class MoDiagram
{
  public:

    MoDiagram();

    /**
     * Group orbitals into drawn levels.
     *
     * Orbitals are one level when they share an irrep label AND lie
     * within tolerance in energy.  The degeneracy that results is
     * reported, but a caller that knows the irrep's dimension should
     * prefer that: an accidental near-degeneracy is not a degenerate
     * set, and grouping by energy alone cannot tell them apart.
     *
     * @param energies     one per orbital, Hartree, ascending
     * @param occupancies  one per orbital
     * @param labels       one per orbital; may be empty, in which case
     *                     grouping is by energy alone and every level
     *                     is labelled unknown
     * @param tolerance    Hartree
     */
    static bool group(const vector<double>& energies,
                      const vector<double>& occupancies,
                      const vector<string>& labels,
                      double tolerance,
                      vector<MoLevel>& levels);

    /**
     * Fold levels below a cutoff into the column's hidden band.
     *
     * @param cutoff  Hartree; levels below this are hidden
     */
    static void hideBelow(MoColumn& column, double cutoff);

    /**
     * A cutoff taken from the biggest gap in the lower half of the
     * spectrum, so it adapts to the molecule instead of assuming what
     * counts as core.
     *
     * Returns the lowest energy in the column when no useful gap is
     * found, which hides nothing.
     */
    static double suggestCoreCutoff(const vector<MoLevel>& levels);

    /**
     * Connect fragment levels to molecular orbitals by matching irrep.
     *
     * Symmetry is the whole rule: orbitals of different irreps cannot
     * mix, and orbitals of the same one must be allowed to.  A centre
     * level with no fragment partner of its irrep is non-bonding and
     * gets a connection with -1 on that side.
     */
    static void connect(const vector<MoLevel>& left,
                        const vector<MoLevel>& centre,
                        const vector<MoLevel>& right,
                        vector<MoConnection>& connections);
};

#endif
