/**
 * @file
 *
 * The character table of a point group.
 *
 * PointGroup already knows the irrep NAMES of 46 groups, which is all
 * that is needed to label an orbital the code has already classified.
 * Deciding which irreps a set of orbitals spans -- which is what turns
 * a list of atomic orbitals into the symmetry orbitals of a molecular
 * orbital diagram -- needs the characters themselves.
 *
 * Kept separate from PointGroup rather than bolted onto it: PointGroup
 * is loaded by everything that displays a symmetry label, and only the
 * symmetry analysis needs this.
 *
 * The parsing is deliberately reachable without Ecce::ecceDataPrefPath()
 * or any of the runtime around it, so the data can be checked by a test
 * that compiles with nothing but this header.
 */

#ifndef CHARACTERTABLE_HH
#define CHARACTERTABLE_HH

#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;

class CharacterTable
{
  public:

    CharacterTable();

    /**
     * The table for a group, or null if there is none.
     *
     * Case insensitive, since the group name reaches this from several
     * directions -- the detector, the .edml files, and the codes' own
     * output -- which do not agree on capitalisation.
     */
    static const CharacterTable* lookup(const string& group);

    /**
     * Load a character table file, replacing whatever was loaded.
     *
     * Public so a test can point at the file directly.  Returns the
     * number of groups read; zero means the file was missing or
     * unreadable, which is a condition the caller should report rather
     * than proceed through.
     */
    static int loadFile(const string& path);

    /**
     * Every group that has been loaded, by name.
     *
     * For finding a SUBGROUP: the operations that preserve a fragment
     * form a group, and identifying which one means trying the
     * candidates of the right order against its class structure.
     * There is no way to do that without being able to ask what the
     * candidates are.
     */
    static vector<string> names(void);

    const string& name(void) const            { return p_name; }
    int order(void) const                     { return p_order; }
    const vector<string>& classes(void) const { return p_classes; }
    const vector<int>& counts(void) const     { return p_counts; }
    const vector<string>& irreps(void) const  { return p_irreps; }

    /**
     * Characters of one irrep across the classes, or null.
     *
     * Doubles, not integers.  Most tables are integral, but the
     * five-fold and eight-fold groups are not: D5's E1 characters are
     * 2cos72 and 2cos144, D4d's involve sqrt(2), and the icosahedral
     * ones are built from the golden ratio.  Rounding those would break
     * orthogonality outright.
     */
    const vector<double>* characters(const string& irrep) const;

    /** An irrep's dimension: its character under the identity. */
    int dimension(const string& irrep) const;

    /**
     * Reduce a representation into its irreducible components.
     *
     *     n_i = (1/h) sum_c  n_c  chi_i(c)  chi(c)
     *
     * @param chi           one character per class, in class order
     * @param multiplicity  filled with one count per irrep, in irrep
     *                      order
     *
     * @return false if chi is the wrong length, or if any multiplicity
     *         comes out negative or non-integral -- which means the
     *         input was not a representation of this group, and is a
     *         reason to stop rather than to round.
     */
    bool reduce(const vector<double>& chi, vector<int>& multiplicity) const;

    /** The reduction written out, e.g. "A1 + E" or "2A' + A''". */
    string reduceToString(const vector<double>& chi) const;

  private:

    string         p_name;
    int            p_order;
    vector<string> p_classes;
    vector<int>    p_counts;
    vector<string> p_irreps;

    map<string, vector<double> > p_characters;

    static map<string, CharacterTable> p_tables;
    static bool p_loaded;

    static void initialize(void);
};

#endif
