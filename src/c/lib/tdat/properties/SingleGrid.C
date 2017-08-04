////////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: SingleGrid.C
//
//
// CLASS SYNOPSIS:
//    SingleGrid contains the data associated with a single grid property.
//      
// DESCRIPTION:
//   For viz.
//
////////////////////////////////////////////////////////////////////////////////
//system includes
#include <math.h>

#include "tdat/SingleGrid.H"

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Constructor for SingleGrid
////////////////////////////////////////////////////////////////////////////////
SingleGrid::SingleGrid()
{
  p_field = (float*)0;
}

SingleGrid::SingleGrid(int size)
{
  p_field = new float[size];
}
////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Destructor for SingleGrid
////////////////////////////////////////////////////////////////////////////////
SingleGrid::~SingleGrid()
{
  delete[] p_field;
}
//
//-----------Accessors------------
//
string SingleGrid::type()
{
  return p_fieldType;
}

string SingleGrid::name()
{
  return p_fieldName;
}

int* SingleGrid::dimensions()
{
  return p_dims;
}

double* SingleGrid::origin()
{
  return p_origin;
}

double* SingleGrid::corner()
{
  return p_corner;
}

float* SingleGrid::fieldData()
{
  return p_field;
}

float SingleGrid::fieldMax()
{
  return p_fieldMax;
}

float SingleGrid::fieldMin()
{
  return p_fieldMin;
}

float SingleGrid::absFieldMax()
{
  return p_fieldAbsMax;
}

float SingleGrid::absFieldMin()
{
  return p_fieldAbsMin;
}

int SingleGrid::gridSize()
{
  return p_dims[0]*p_dims[1]*p_dims[2];
}
//
//-----------Modifiers------------
//
void SingleGrid::type(string fieldType)
{
  p_fieldType = fieldType;
}

void SingleGrid::name(string fieldName)
{
  p_fieldName = fieldName;
}

void SingleGrid::dimensions(int x, int y, int z)
{
  p_dims[0] = x;
  p_dims[1] = y;
  p_dims[2] = z;
}

void SingleGrid::origin(double x, double y, double z)
{
  p_origin[0] = x;
  p_origin[1] = y;
  p_origin[2] = z;
}

void SingleGrid::corner(double x, double y, double z)
{
  p_corner[0] = x;
  p_corner[1] = y;
  p_corner[2] = z;
}

void SingleGrid::fieldMax(float max)
{
  p_fieldMax = max;
}

void SingleGrid::fieldMin(float min)
{
  p_fieldMin = min;
}

void SingleGrid::absFieldMax(float absMax)
{
  p_fieldAbsMax = absMax;
}

void SingleGrid::absFieldMin(float absMin)
{
  p_fieldAbsMin = absMin;
}

void SingleGrid::setFieldData(float *field)
{
  p_field = field;
}
//-------------------General Methods---------------
void SingleGrid::findMinMax()
{
  int i;
  int gridsize = p_dims[0]*p_dims[1]*p_dims[2];
  p_fieldMin = p_field[0];
  p_fieldMax = p_field[0];
  p_fieldAbsMin = fabs(p_field[0]);
  for (i=1; i<gridsize; i++) {
    if (p_field[i] < p_fieldMin) p_fieldMin = p_field[i];
    if (p_field[i] > p_fieldMax) p_fieldMax = p_field[i];
    if (fabs(p_field[i]) < p_fieldAbsMin)
      p_fieldAbsMin=fabs(p_field[i]);
  }
  if (fabs(p_fieldMin)>fabs(p_fieldMax)) p_fieldAbsMax =fabs(p_fieldMin);
  else p_fieldAbsMax =fabs(p_fieldMax);
}

SingleGrid *SingleGrid::clone()
{
  int gridsize = p_dims[0]*p_dims[1]*p_dims[2];
  SingleGrid *ret = new SingleGrid(gridsize);
  int i;
  for (i=0; i<3; i++) {
    ret->type(p_fieldType);
    ret->dimensions(p_dims[0],p_dims[1],p_dims[2]);
    ret->origin(p_origin[0],p_origin[1],p_origin[2]);
    ret->corner(p_corner[0],p_corner[1],p_corner[2]);
  }
  float *field = ret->fieldData();
  for (i=0; i<gridsize; i++) {
    field[i] = p_field[i];
  }
  return ret;
}

void SingleGrid::clearField()
{
  if (p_field != (float*)0)
    delete[] p_field;
  p_field = (float*)0;
}
////////////////////////////////////////////////////////////////////////////////
// Description:
//   Compare two Single grid objects to see if they have the same overall
//   dimensions and number of grid elements. Return true if they do, false
//   otherwise.
////////////////////////////////////////////////////////////////////////////////
bool SingleGrid::compare(SingleGrid *grid)
{
  int i;
  for (i=0; i<3; i++) {
    // Compare number of elements in each coordinate direction
    if (p_dims[i] != grid->dimensions()[i])
      return false;
    // Compare origins
    if (p_origin[i] != grid->origin()[i])
      return false;
    // Compare far corners
    if (p_corner[i] != grid->corner()[i])
      return false;
  }
  return true;
}
////////////////////////////////////////////////////////////////////////////////
// Description:
//   Add two grids of same dimensions and number of points in the linear
//   combination
//      alpha*grid_a + beta*grid_b
//   Return false if grids are incompatible
////////////////////////////////////////////////////////////////////////////////
bool SingleGrid::addFields(float alpha, SingleGrid *grid_a,
                           float beta, SingleGrid *grid_b)
{
  if (!grid_a->compare(grid_b))
    return false;
  clearField();
  int nsize = grid_a->gridSize();
  p_field = new float[nsize];
  float *a = grid_a->fieldData();
  float *b = grid_b->fieldData();
  int i;
  for (i=0; i<nsize; i++)
    p_field[i] = alpha*a[i]+beta*b[i];
  for (i=0; i<3; i++) {
    p_dims[i] = grid_a->dimensions()[i];
    p_origin[i] = grid_a->origin()[i];
    p_corner[i] = grid_a->corner()[i];
  }
  findMinMax();
  return true;
}
