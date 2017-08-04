/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cerr;
  using std::endl;

#include "tdat/PropGrids.H"
#include "tdat/TProperty.H"
#include "tdat/PropVecGrid.H"



/** 
 * Clears the PropGrids vector along with deleting the associated
 * PropGrids objects.  
 */
PropVecGrid::~PropVecGrid()
{
  removeGrids();
}


/** 
 * Size of p_grids vector
 *
 * @return Returns size of p_grids
 */
int PropVecGrid::getNumPropGrids()
{
  return p_grids.size();
}


/**
 * Returns a PropGrids pointer from the p_grids
 * vector for a specific index.  Null pointer
 * will be returned if the index is out of
 * range.
 *
 * @return Pointer to a PropGrids object.  Null pointer
 *         will be returned if the index is out of
 *         range.
 */
PropGrids * PropVecGrid::getPropGrids(int index)
{
  PropGrids *ret = 0;
  if (index >= 0 && index < p_grids.size()) {
    ret = p_grids[index];
  }
  return ret;
}


/**
 * Adds a PropGrids pointer to the p_grids vector.
 */

void PropVecGrid::addGrid(PropGrids *propGrid)
{
  p_grids.push_back(propGrid);
}


/**
 * Reads a CUBE file and adds it to the vector of PropGrids.
 * Currently, only CUBE files are supported by the PropVecGrid 
 * property.  
 * 
 * @param is CUBE input stream
 *
 * @return Indicates if the new CUBE file was added
 *         into the PropVecGrid property.
 */
bool PropVecGrid::readGrid(istream& is)
{
  bool ret = true;
  PropGrids *propGrid = new PropGrids();
  try {
    propGrid->readCubeFile(is);
    addGrid(propGrid);
  } 
  catch (IOException& ex) {
    delete propGrid;
    ret = false;
  }
  return ret;
}



/**
 * Clears the PropGrids pointers from the vector.  Does not delete
 * the PropGrids objects.  Use removeGrids() to delete the PropGrids.
 */
void PropVecGrid::clearGrids() 
{
  p_grids.clear();
}


/**
 * Clears a PropGrids pointer from the vector.  Does not delete
 * the PropGrid object.  Use removeGrids(index) to delete a specific
 * PropGrids object.
 */
void PropVecGrid::clearGrids(int index)
{
  if (index >= 0 && index < p_grids.size()) {
    vector<PropGrids*>::iterator it = p_grids.begin() + index;
    p_grids.erase(it);
  } 
}


/** 
 * Clears the PropGrids vector along with deleting the associated
 * PropGrids objects.
 */
void PropVecGrid::removeGrids() 
{
  int i;

  // Remove PropGrids
  for (i = 0; i < p_grids.size(); i++) {
    if (p_grids[i] != 0) {
      delete p_grids[i];
    }
  }
  p_grids.clear();
}


/** 
 * Clears a PropGrids pointer from the vector along with deleting
 * the associated PropGrids object.
 */
void PropVecGrid::removeGrids(int index)
{
  if (index >= 0 && index < p_grids.size()) {
    vector<PropGrids*>::iterator it = p_grids.begin() + index;
    PropGrids *byby = (*it);
    p_grids.erase(it);
    delete byby;
  } 
  else {
    cerr << "Asking to delete grid at invalid index " << index << endl;
  }
}


/** 
 * Constructor - PropFactory must be used to create a new 
 * PropVecGrid object.  
 */
PropVecGrid::PropVecGrid(const string& type, 
                         const string& units) : TProperty(type, units)
{
  p_classType = TProperty::PROPVECGRID;
}


