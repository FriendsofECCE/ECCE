/**
 * @ file
 *
 *
 */

// system includes
#include <math.h>

// application includes
#include "util/Transformations.H"
#include "util/ErrMsg.H"
#include "tdat/LinkCellList.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Construct the NonBond object from a fragment pointer.
////////////////////////////////////////////////////////////////////////////// 
/**
 * Constructor.
 * Creates linked cell list and header list.
 * @param atoms - all atoms in the system.
 * @param cutoff - minimum size of cells - cell size will be
 *                 adjusted to get integer number of cells.
 */
LinkCellList::LinkCellList(vector<TAtm*> *atoms, double cutOff)
{
  
  int nAtoms = atoms->size();
  p_atoms = atoms;
  MPoint p1;

//  find minimum and maximum values of x,y,z coordinates
  double xmin,xmax,ymin,ymax,zmin,zmax;
  TAtm *atm1;

  atm1 = (*atoms)[0];
  p1.xyz(atm1->coordinates());

// Initialize xmin,xmax etc. using coordinates of first
// atom in list
  xmin = p1.x();
  xmax = p1.x();
  ymin = p1.y();
  ymax = p1.y();
  zmin = p1.z();
  zmax = p1.z();

  // find xmin, xmax, etc.
  int i;
  for (i = 1; i < nAtoms; i++) {
    atm1 = (*atoms)[i];

    p1.xyz(atm1->coordinates());

    if (p1.x() < xmin) xmin = p1.x();
    if (p1.x() > xmax) xmax = p1.x();
    if (p1.y() < ymin) ymin = p1.y();
    if (p1.y() > ymax) ymax = p1.y();
    if (p1.z() < zmin) zmin = p1.z();
    if (p1.z() > zmax) zmax = p1.z();
  }

// Find number of linked cells needed in each dimension. If
// number of cells in any dimension exceeds imaxcell, set
// the number of cells in that dimension equal to imaxcell

  int nx,ny,nz;
  int imaxcell=200;

  nx = (int)((xmax-xmin)/cutOff);
  ny = (int)((ymax-ymin)/cutOff);
  nz = (int)((zmax-zmin)/cutOff);

  if (nx == 0) nx = 1;
  if (ny == 0) ny = 1;
  if (nz == 0) nz = 1;

  if (nx > imaxcell) nx = imaxcell;
  if (ny > imaxcell) ny = imaxcell;
  if (nz > imaxcell) nz = imaxcell;

// Evaluate the edge length of each cell

  double xlength = (xmax-xmin)/(double)nx;
  double ylength = (ymax-ymin)/(double)ny;
  double zlength = (zmax-zmin)/(double)nz;

// check against pathological cases such as all atoms in
// a plane

  if (xlength == 0.) xlength = 1.;
  if (ylength == 0.) ylength = 1.;
  if (zlength == 0.) zlength = 1.;

// Create linked list and header file

  int cellmax = nx*ny*nz;

  p_linkList = new int[nAtoms];
  p_header = new int[cellmax];

// Scan through all atoms and sort them into their appropriate
// cells. Start by initializing header to -1;

  for (i = 0; i < cellmax; i++) {
    p_header[i] = -1;
  }
  for (i = 0; i < nAtoms; i++) {
    atm1 = (*atoms)[i];

    p1.xyz(atm1->coordinates());

    int ix = int((p1.x() - xmin)/xlength);
    int iy = int((p1.y() - ymin)/ylength);
    int iz = int((p1.z() - zmin)/zlength);
    if (ix == nx) ix = nx-1;
    if (iy == ny) iy = ny-1;
    if (iz == nz) iz = nz-1;

    int l = ix + iy*nx + iz*nx*ny;
    p_linkList[i] = p_header[l];
    p_header[l] = i;
  }
  p_nx = nx;
  p_ny = ny;
  p_nz = nz;
  p_xmin = xmin;
  p_ymin = ymin;
  p_zmin = zmin;
  p_xlen = xlength;
  p_ylen = ylength;
  p_zlen = zlength;

}

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple destructor.
////////////////////////////////////////////////////////////////////////////// 
LinkCellList::~LinkCellList()
{
  delete [] p_header;
  delete [] p_linkList;
}
// ---------- Accessors  ----------
////////////////////////////////////////////////////////////////////////////////
// Description:
//   Provide pointers to link cell list and header array
////////////////////////////////////////////////////////////////////////////////
const int* LinkCellList::getLinkCellList()
{
  return p_linkList;
}

const int* LinkCellList::getHeader()
{
  return p_header;
}



/**
 * Determines list of cell indices that include cell containing atomIdx
 * and its bordering cells.
 * @param cells - out paramater, vector containing list of cell indices
 * @param atomIdx - atom index, assumed it is a valid index (no exception thrown)
 */
void LinkCellList::getCells(vector<int>& cells, int atomIdx)
{
  TAtm *atom;
  MPoint p1;
  int cellMax = p_nx*p_ny*p_nz;
  vector<bool> cellMask(cellMax, false);
  int ix, iy, iz;
  int k, kx, kxmin, kxmax;
  int ky;
  int kz;

  cells.clear();
  atom = (*p_atoms)[atomIdx];
  p1.xyz(atom->coordinates());
  ix = xIndex(p1.x());
  iy = yIndex(p1.y());
  iz = zIndex(p1.z());
  kxmin = ixMin(ix);
  kxmax = ixMax(ix);

  for (kx = kxmin; kx <= kxmax; kx++) {
    int kymin = iyMin(iy);
    int kymax = iyMax(iy);
    for (ky = kymin; ky <= kymax; ky++) {
      int kzmin = izMin(iz);
      int kzmax = izMax(iz);
      for (kz = kzmin; kz <= kzmax; kz++) {
        k = getCellIndex(kx, ky, kz);
        // If not already found - then add it
        if (!cellMask[k]) {
          cells.push_back(k);
          cellMask[k] = true;
        }
      }
    }
  }
}


/**
 * Determine an atom's cell index.
 * @param atomIndex - An atoms index (integer)
 * @return Cell index (integer)
 */
int LinkCellList::getCellIndex(int atomIndex)
{
  TAtm *atom;
  MPoint p1;
  int ix, iy, iz;
  int ret = 0;

  atom = (*p_atoms)[atomIndex];
  p1.xyz(atom->coordinates());
  ix = xIndex(p1.x());
  iy = yIndex(p1.y());
  iz = zIndex(p1.z());
  ret = getCellIndex(ix, iy, iz);
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    return the absolute cell index given cell indices along the axes
////////////////////////////////////////////////////////////////////////////////
int LinkCellList::getCellIndex(int ix, int iy, int iz)
{
  return ix + iy*p_nx + iz*p_nx*p_ny;
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    return the cell indices along the axes given position
////////////////////////////////////////////////////////////////////////////////
int LinkCellList::xIndex(double x)
{
  int ix = int((x - p_xmin)/p_xlen);
  if (ix == p_nx) ix = p_nx-1;
  return ix;
}

int LinkCellList::yIndex(double y)
{
  int iy = int((y - p_ymin)/p_ylen);
  if (iy == p_ny) iy = p_ny-1;
  return iy;
}

int LinkCellList::zIndex(double z)
{
  int iz = int((z - p_zmin)/p_zlen);
  if (iz == p_nz) iz = p_nz-1;
  return iz;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    return return range limits for searching over neighboring cells
////////////////////////////////////////////////////////////////////////////////
int LinkCellList::ixMin(int ix)
{
  int i = ix - 1;
  if (i == -1) {
    i = 0;
  }
  if (p_nx == 1) {
    i = 0;
  }
  return i;
}

int LinkCellList::ixMax(int ix)
{
  int i = ix + 1;
  if (i == p_nx) {
    i = p_nx-1;
  }
  if (p_nx == 1) {
    i = 0;
  }
  return i;
}

int LinkCellList::iyMin(int iy)
{
  int i = iy - 1;
  if (i == -1) {
    i = 0;
  }
  if (p_ny == 1) {
    i = 0;
  }
  return i;
}

int LinkCellList::iyMax(int iy)
{
  int i = iy + 1;
  if (i == p_ny) {
    i = p_ny-1;
  }
  if (p_ny == 1) {
    i = 0;
  }
  return i;
}

int LinkCellList::izMin(int iz)
{
  int i = iz - 1;
  if (i == -1) {
    i = 0;
  }
  if (p_nz == 1) {
    i = 0;
  }
  return i;
}

int LinkCellList::izMax(int iz)
{
  int i = iz + 1;
  if (i == p_nz) {
    i = p_nz-1;
  }
  if (p_nz == 1) {
    i = 0;
  }
  return i;
}
