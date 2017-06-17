/**
 * @file
 *
 *
 */

#ifndef PROPVECGRID_HH
#define PROPVECGRID_HH

#include "tdat/PropGrids.H"
#include "tdat/TProperty.H"


/**
 * PropVecGrid contains an STL vector of PropGrids objects. PropVecGrids is 
 * write once - meaning that each PropGrid object is stored/written once
 * to the STL vector. 
 */
class PropVecGrid : public TProperty
{

public:
  virtual ~PropVecGrid();

  int getNumPropGrids();
  PropGrids * getPropGrids(int index); 
  void addGrid(PropGrids *grid);
  bool readGrid(istream& grid);
  void clearGrids();
  void clearGrids(int index);
  void removeGrids();
  void removeGrids(int index);
  
private:

  vector<PropGrids *> p_grids;  /**< Vector of PropGrids pointers.  Currently,
                                     PropGrids is used to support Gaussian
                                     CUBE property files. */

  PropVecGrid(const string& type, const string& units); 
    
  friend class PropFactory;

};

#endif
