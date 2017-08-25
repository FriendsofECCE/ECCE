/**
 * @ file
 *
 *
 */
#ifndef LINKCELLLIST_HH
#define LINKCELLLIST_HH


#include <vector>
using std::vector;

class LinkCellList;
class TAtm;


/**
 * Get a spatial partition of bonds based on a link-cell list algorithm.
 * Class for setting up and manipulating link cell lists for evaluating
 * non-bond interactions, finding covalent and hydrogen bonds, finding
 * overlapping atoms, etc.
 * <br>
 * This class contains two main items:
 *   <ul>
 *   <li>A grid of cells (linked cell list which is of size nAtoms)</li>
 *   <li>A header which contains a pointer to the first atom for each cell.</li>
 *   </ul>
 * So you can iterate over the atoms of interest (aka particles).
 * The header list is equal to the number of cells.  You traverse the
 * link cell list starting at the first pointer until a -1 is reached.
 * <br>
 * <br>
 * For Example: 
 * \code
 *    vector<TAtm*> *atoms = frag->atoms();
 *    const int *link_list = cellList.getLinkCellList();
 *    const int *header = cellList.getHeader();
 *    vector<int> cells;
 *    int cellIdx;
 *    int atomIdx;
 *
 *    for (i = 0; i < atoms.size(); i++) {
 *
 *      // Get list of cells that contain "atom" and border it
 *      cellList.getCells(cells, i);
 *
 *      // Iterate over each  cell
 *      for (j = 0; j < cells.size(); j++) {
 *
 *        // Determine cell index
 *        cellIdx = cells[j];
 *
 *        // Use cell index to get first atom from header array
 *        atomIdx = header[cellIdx];
 *
 *        // Iterate over all atoms in cell 
 *        while (atomIdx >= 0) {
 *
 *          // ----------------------
 *          // YOUR CODE HERE
 *          // ----------------------
 *
 *          // Get next atom in cell
 *          atomIdx = link_list[atomIdx];
 *        }
 *      }
 *    }
 * \endcode
 */ 
class LinkCellList
{

public:
  
  // Constructors
  LinkCellList(vector<TAtm*> *atoms, double cutoff);

  // Virtual Destructor
  virtual ~LinkCellList();

  // Accessors
  const int* getLinkCellList();
  const int* getHeader();

  // General Methods
  void getCells(vector<int>& cells, int atomIdx);
  int getCellIndex(int atomIndex);
  int getCellIndex(int ix, int iy, int iz);

  /**
   * @todo Should probably make these methos protected
   *       and convert existing usage to new interface.
   */
  int xIndex(double x);
  int yIndex(double y);
  int zIndex(double z);
  int ixMin(int ix);
  int ixMax(int ix);
  int iyMin(int iy);
  int iyMax(int iy);
  int izMin(int iz);
  int izMax(int iz);

private:

  // State

  /**
   * Vector of chemical system's atoms.
   * It is assumed that this vector will exist for the
   * lifetime of LinkCellList
   */
  const vector<TAtm*> *p_atoms;

  int *p_header;
  int *p_linkList;
  int p_nx, p_ny, p_nz;
  double p_xmin, p_ymin, p_zmin;
  double p_xlen, p_ylen, p_zlen;

};

#endif /* LINKCELLLIST_HH */
