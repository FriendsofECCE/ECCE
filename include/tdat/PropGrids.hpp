////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropGrids.H
//
//
// CLASS SYNOPSIS:
//    PropGrids contains a fragment and multiple grids of data associated with
//    the fragment.
//      
// DESCRIPTION:
//   This class is used to read in grid data for constructing isosurfaces etc.,
//   from a file and to create a set of SingleGrid objects that can be used
//   by the viz library for doing visualization of isosurfaces, contour slices,
//   etc.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef PROPGRIDS_HH
#define PROPGRIDS_HH

#include <iostream>
using std::istream;

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "util/IOException.H"

class Fragment;
class SingleGrid;

class PropGrids
{
  public:
    PropGrids();
    virtual ~PropGrids();

// Accessors
    int                    numGrids() const;
    Fragment&              getFrag() const;
    SingleGrid*            returnGrid(int index) const;

// Modifiers
   void removeGrid(int index);
   void addGrid(SingleGrid *);

// General methods
    bool        readCubeFile(istream &infile) throw (IOException);
    bool        compare(PropGrids *grids);
    bool        absorb(PropGrids *grids);

  private:

    void        clearGridPtrs();

    Fragment           *p_chemsys;
    vector<SingleGrid*> p_grids;
};
#endif // PROPGRIDS_HH
