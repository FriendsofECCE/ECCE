/**
 * @file
 *
 * The rules behind the MOs panel's spin density tables, apart from wx: how
 * a SOMO's composition is cut to a line that fits a table cell, and how the
 * atomic spin populations are ordered and given their share of the total.
 */

#ifndef SPINTABLES_HH
#define SPINTABLES_HH

#include <string>
#include <utility>
#include <vector>

class SpinTables
{
  public:
    /** One atom's row in the spin population table. */
    struct Population
    {
      std::string atom;
      double spin;      // electrons
      double share;     // |spin| as % of the sum of |spin| over all atoms
    };

    /** The composition text of one orbital: label and fraction. */
    typedef std::pair<std::string, double> Part;

    /**
     * "Nb1 40%, O5 20%, O7 10%, other 30%": the leading parts of at least
     * minShare (at most maxShown, largest first) and what the rest adds up
     * to.  The line stays short however many atoms the orbital touches.
     */
    static std::string compositionText(const std::vector<Part>& parts,
                                       size_t maxShown = 3,
                                       double minShare = 0.10);

    /** All parts of at least 1%, largest first, for a tooltip. */
    static std::string fullCompositionText(const std::vector<Part>& parts);

    /**
     * The atoms whose population is at least threshold in magnitude, largest
     * |spin| first (equal ones keep their order), each with its share.
     */
    static std::vector<Population> populations(
        const std::vector<std::string>& names,
        const std::vector<double>& spin, double threshold = 0.0005);
};

#endif
