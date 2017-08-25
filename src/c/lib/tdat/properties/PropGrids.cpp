////////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropGrids.C
//
//
// CLASS SYNOPSIS:
//    PropGrids contains a fragment and multiple grids of data associated with
//    the fragment.
//      
// DESCRIPTION:
//   For viz.
//
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>

#include <string>

using std::string;


#include "util/StringTokenizer.H"
#include "util/LineReader.H"
#include "util/ErrMsg.H"
#include "util/IOException.H"
#include "tdat/PropGrids.H"
#include "tdat/SingleGrid.H"
#include "tdat/Fragment.H"
#include "tdat/TPerTab.H"

////////////////////////////////////////////////////////////////////////////////
// Description:
//   PropGrids constructor
////////////////////////////////////////////////////////////////////////////////
PropGrids::PropGrids()
{
  p_chemsys = (Fragment*)0;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   PropGrids destructor
////////////////////////////////////////////////////////////////////////////////
PropGrids::~PropGrids()
{
  delete p_chemsys;
  for (int i=0; i<p_grids.size(); i++) {
    delete p_grids[i];
  }
  p_grids.clear();
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Return total number of grids store in propgrids
////////////////////////////////////////////////////////////////////////////////
int PropGrids::numGrids() const
{
  return p_grids.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Return pointer to geometry fragment associated with propgrids
////////////////////////////////////////////////////////////////////////////////
Fragment& PropGrids::getFrag() const
{
  return *p_chemsys;
}

/**
 * Removes and deletes grid at specified index.
 * Quietly ignores invalid requests.
 */
void PropGrids::removeGrid(int index)
{
  if (index >= 0 && index < p_grids.size()) {
    vector<SingleGrid*>::iterator it = p_grids.begin() + index;
    delete (*it);
    p_grids.erase(it);
  }
}

/**
 * Adds a new SingleGrid object.
 */
void PropGrids::addGrid(SingleGrid *grid)
{
  if (grid) {
    p_grids.push_back(grid);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Read a Gaussian format cube file
////////////////////////////////////////////////////////////////////////////////
bool PropGrids::readCubeFile(istream &infile) throw (IOException)
{
  bool isMOdata = false;
  string title = "";
  string line;
  string type;
  vector<string> words;
  LineReader linereader(infile);
  linereader.stripTrailingComments(true);
  int i, j, natoms;
  double x_origin,y_origin,z_origin;
  double x_inc, y_inc, z_inc;
  int x_dim,y_dim,z_dim;
  int num_mo;
  vector<int> mo_ids;
  // This is the value NIST is reporting in 2002
  const double bohr_to_ang = 0.5291772083;
  // Read title (currently ignored)
  if (!linereader.getLine(title))
    throw IOException("Title card does not exist", WHERE);
  // Read data type (currently ignored)
  if (!linereader.getLine(type))
    throw IOException("Data type card does not exist", WHERE);

  // Start reading in geometry
  // Get number of atoms and origin of grid
  if (linereader.getLine(line)) {
    StringTokenizer tokens(line);
    words = tokens.tokenize("\t\n ");
    if (words.size() == 4) {
      natoms = atoi(words[0].c_str());
      if (natoms < 0) {
        isMOdata = true;
        natoms = -natoms;
      }
      x_origin = bohr_to_ang*(double)atof(words[1].c_str());
      y_origin = bohr_to_ang*(double)atof(words[2].c_str());
      z_origin = bohr_to_ang*(double)atof(words[3].c_str());
    } else {
      throw IOException("Incorrect number of items"
                            " in line containing number of atoms and grid"
                            " origin", WHERE);
    }
  } else {
    throw IOException("Line containing number of items and grid"
                          " origin is missing", WHERE);
  }
  // Get dimensions of grid and grid increments. We are currently
  // assuming that the grid is orthogonal and that the grid
  // basis vectors are written out in the order x, y, z.
  if (linereader.getLine(line)) {
    StringTokenizer tokens(line);
    words = tokens.tokenize("\t\n ");
    if (words.size() == 4) {
      x_dim = atoi(words[0].c_str());
      x_inc = bohr_to_ang*(double)atof(words[1].c_str());
    } else {
      throw IOException("Incorrect number of items in line containing"
                            " X dimension and x direction vector", WHERE);
    }
  } else {
    throw IOException("Line containing X dimension and x"
                          " direction vector is missing", WHERE);
  }
  if (linereader.getLine(line)) {
    StringTokenizer tokens(line);
    words = tokens.tokenize("\t\n ");
    if (words.size() == 4) {
      y_dim = atoi(words[0].c_str());
      y_inc = bohr_to_ang*(double)atof(words[2].c_str());
    } else {
      throw IOException("Incorrect number of items in line containing"
                            " Y dimension and y direction vector", WHERE);
    }
  } else {
    throw IOException("Line containing Y dimension and y"
                          " direction vector is missing", WHERE);
  }
  if (linereader.getLine(line)) {
    StringTokenizer tokens(line);
    words = tokens.tokenize("\t\n ");
    if (words.size() == 4) {
      z_dim = atoi(words[0].c_str());
      z_inc = bohr_to_ang*(double)atof(words[3].c_str());
    } else {
      throw IOException("Incorrect number of items in line containing"
                            " Z dimension and z direction vector", WHERE);
    }
  } else {
    throw IOException("Line containing Z dimension and z"
                          " direction vector is missing", WHERE);
  }
  // Get geometry (if necessary)
  if (p_chemsys == (Fragment*)0) {
    int iat;
    vector<double> charge;
    double *coords;
    coords = new double[3*natoms];
    TPerTab pertab;
    vector<string> tags;
    for (i=0; i<natoms; i++) {
      if (linereader.getLine(line)) {
        StringTokenizer tokens(line);
        words = tokens.tokenize("\t\n ");
        if (words.size() == 5) {
          iat = atoi(words[0].c_str());
          if (iat < 0) iat = -iat;
          charge.push_back((double)atof(words[1].c_str()));
          coords[3*i] = bohr_to_ang*(double)atof(words[2].c_str());
          coords[3*i+1] = bohr_to_ang*(double)atof(words[3].c_str());
          coords[3*i+2] = bohr_to_ang*(double)atof(words[4].c_str());
          tags.push_back(pertab.atomicSymbol(iat));
        } else {
          throw IOException("Incorrect number of items"
                                " in geometry field", WHERE);
          delete[] coords;
        }
      } else {
        throw IOException("Geometry field not found", WHERE);
      }
    }
    // Create fragment
    p_chemsys = new Fragment(title, tags, coords);
    p_chemsys->addCovalentBonds();
    delete[] coords;
    // Set atomic charges
    for (i=0; i<natoms; i++) {
      p_chemsys->atomRef(i)->partialCharge(charge[i]);
    }
  } else {
    for (i=0; i<natoms; i++) {
      if (!linereader.getLine(line))
        throw IOException("Geometry field not found", WHERE);
    }
  }

  // If we are computing MOs, parse lines containing number of MOs and
  // the MO numbers.
  if (isMOdata) {
    int mo_cnt = 0;
    if (linereader.getLine(line)) {
      StringTokenizer tokens(line);
      words = tokens.tokenize("\t\n ");
      num_mo = atoi(words[0].c_str());
      int nword = words.size() > num_mo+1 ? num_mo+1 : words.size();
      for (i=1; i<nword; i++) {
        mo_cnt++;
        mo_ids.push_back(atoi(words[i].c_str()));
      }
    } else {
      throw IOException("MO specification not found", WHERE);
    }
    // If we haven't accounted for all MOs, keep parsing
    while (mo_cnt < num_mo) {
      if (linereader.getLine(line)) {
        StringTokenizer tokens(line);
        words = tokens.tokenize("\t\n ");
        for (i=0; i<words.size(); i++) {
          mo_cnt++;
          mo_ids.push_back(atoi(words[i].c_str()));
        }
      } else {
        throw IOException("MO specification not found", WHERE);
      }
    }
  }
  // Create the grids
  int ngrids, offset;
  offset = p_grids.size();
  if (isMOdata) ngrids = num_mo;
  else ngrids = 1;
  int gridsize = x_dim * y_dim * z_dim;
  char moNum[80];
  for (i=0; i<ngrids; i++) {
    SingleGrid *grid = new SingleGrid(gridsize);
    grid->dimensions(x_dim, y_dim, z_dim);
    grid->origin(x_origin, y_origin, z_origin);
    grid->corner(x_inc*(double)(x_dim-1)+x_origin,
                 y_inc*(double)(y_dim-1)+y_origin,
                 z_inc*(double)(z_dim-1)+z_origin);
    if (isMOdata) {
      sprintf(moNum,"MO-%d",mo_ids[i]);
      grid->name(moNum);
    } else grid->name(title);
    grid->type(type);
    p_grids.push_back(grid);
  }
  // Read in the grid data
  int totaldata = ngrids * gridsize;
  int datacnt = 0;
  float *field;
  int ix, iy, iz, ifield;
  int xy_dim = x_dim*y_dim;
  while (datacnt < totaldata) {
    if (linereader.getLine(line)) {
      StringTokenizer tokens(line);
      words = tokens.tokenize("\t\n ");

      for (i=0; i<words.size(); i++) {
        j = datacnt;
        ifield = j%ngrids;
        j = (j-ifield)/ngrids;
        iz = j%z_dim;
        j = (j-iz)/z_dim;
        iy = j%y_dim;
        ix = (j-iy)/y_dim;
        j = iz*xy_dim + iy*x_dim + ix;
        field = p_grids[offset + ifield]->fieldData();
        field[j] = atof(words[i].c_str());
        datacnt++;
      }
    } else {
      throw IOException("Insufficient data in cube file", WHERE);
    }
  }
  // Set max and min values of grid
  for (i=0; i<ngrids; i++) {
    SingleGrid *grid = p_grids[offset + i];
    grid->findMinMax();
  }
  // File has been successfully read in
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Return pointer to SingleGrid indexed by i
////////////////////////////////////////////////////////////////////////////////
SingleGrid* PropGrids::returnGrid(int i) const
{
  if (i >= p_grids.size() || i < 0)
    EE_RT_ASSERT(false, EE_WARNING,
            "trying to access out-of-bounds index in PropGrids::returnGrid");
  return p_grids[i];
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Compare two PropGrids to see if they are equivalent. Equivalence is defined
//   to be:
//     a) the geometry fragments for both PropGrids have the same empirical
//        formula
//     b) the grid dimensions, origin, and corner are the same
////////////////////////////////////////////////////////////////////////////////
bool PropGrids::compare(PropGrids *grids)
{
  // Check to see if empirical formulas are the same
  if (p_chemsys->generateEmpiricalFormula() !=
      grids->getFrag().generateEmpiricalFormula()) return false;
  // Check to see if atom types and coordinates are the same
  vector<TAtm*> *frgList = p_chemsys->atoms();
  vector<TAtm*> *grdList = grids->getFrag().atoms();
  if (frgList->size() != grdList->size()) return false;
  int size = frgList->size();
  int i;
  double *fcoords, *gcoords;
  TAtm *fatm, *gatm;
  for (i=0; i<size; i++) {
    fatm = (*frgList)[i];
    gatm = (*grdList)[i];
    if (fatm->atomicNumber() != gatm->atomicNumber()) return false;
    fcoords = (double*)fatm->coordinates(); 
    gcoords = (double*)gatm->coordinates(); 
    if (fcoords[0] != gcoords[0] ||
        fcoords[1] != gcoords[1] ||
        fcoords[2] != gcoords[2]) return false;
  }

  // Check to see if grid dimensions are the same
  SingleGrid *grid1 = p_grids[0];
  SingleGrid *grid2 = returnGrid(0);
  int *dim1, *dim2;
  dim1 = grid1->dimensions();
  dim2 = grid2->dimensions();
  for (i=0; i<3; i++) {
    if (dim1[i] != dim2[i]) return false;
  }
  // Check to see if grid origins are the same
  double *origin1, *origin2;
  origin1 = grid1->origin();
  origin2 = grid2->origin();
  for (i=0; i<3; i++) {
    if (origin1[i] != origin2[i]) return false;
  }
  double *corner1, *corner2;
  corner1 = grid1->corner();
  corner2 = grid2->corner();
  for (i=0; i<3; i++) {
    if (corner1[i] != corner2[i]) return false;
  }
  return true;
}
////////////////////////////////////////////////////////////////////////////////
// Description:
//   Clear SingleGrid pointers without doing a delete on the grids
////////////////////////////////////////////////////////////////////////////////
void PropGrids::clearGridPtrs()
{
  p_grids.clear();
}
////////////////////////////////////////////////////////////////////////////////
// Description:
//   Absorb SingleGrids from one PropGrids into another. Note that delete must
//   be called on the argument PropGrids after calling absorb.
////////////////////////////////////////////////////////////////////////////////
bool PropGrids::absorb(PropGrids *grids)
{
  if (!this->compare(grids)) return false;
  int i, ngrid = grids->numGrids();
  for (i=0; i<ngrid; i++) p_grids.push_back(grids->returnGrid(i));
  grids->clearGridPtrs();
  return true;
}
