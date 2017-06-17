///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Matrix.cc
//
//
// DESIGN:
//    The FFCoordinates class is currently designed to be the minimal structure
//    and function necessary to allow persistent storage of two-dimensional
//    arrays of simple types (double, int, short).  Persistence is
//    completely determined by the constructor caller, not transactions
//    are done in this code.
//
//    Construction returns a 0 by 0 matrix (default) or allows size and/or
//    initial elements to be supplied.  Assignment replaces the left-hand-side
//    matrix with a copy of the right-hand-side matrix.  Comparisons are
//    value-based.  Element access and
//    assignment are accomplished through obtaining a FFCoordinates pointer
//    though the coordinates function.
//    Access to the private members, rows and columns, are
//    provided but these values can't be changed (except through assignment
//    or construction).  
//
//    WARNING!  Currently little error checking is provided.  Use at your
//    own risk.
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#include <iomanip>
  using std::ios;

#include "tdat/FFCoords.H"
#include "util/ErrMsg.H"

// #ifndef DEBUG
// #define DEBUG
// #endif

#define PI  3.14159265358979323846
#define RAD * 180.0 / PI

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif /* MAX */

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Default Constructor with Optional Initial Values
//      If the Values are not Provided, the Values will be Zero
//
//  Implementation
//      The default FFCoordinates constructor creates an empty, 0-by-0 matrix.
//      The number of rows and columns can also be provided to construct
//      different sized matrices with 0-valued elements by default.
//      If not 0, the third argument must point to a row-major (C-style)
//      two-dimensional array of T (the size specified by the
//      first two arguments).  These values will be the initial matrix
//      values.
//
//      If initial values are specified (double* != 0) the values will be
//      copied into the newly constructed matrix (optimized by copying
//      entire rows using memcpy).  We assume a sequential layout of
//      the matrix to copy from.
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates::FFCoordinates(int rows, int columns, 
                             double* initial_values) :
                             p_rows(rows),
                             p_columns(columns),
                             p_matrix((double**)0)
{
  if ((p_rows != 0) && (p_columns != 0)) {

    p_matrix = new double*[p_rows];
    double *column = new double[p_rows * p_columns];
    for(int i=0;i<p_rows;i++)
      p_matrix[i] = &column[i*p_columns];

    if (initial_values != (double*)0)
      memcpy(p_matrix[0], initial_values,
             (int)(p_rows * p_columns * sizeof(double)));
    else
      memset(p_matrix[0], (char)0, 
             (int)(p_rows * p_columns * sizeof(double)));
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      FFCoordinates Copy Constructor
//
//  Implementation
//      The copy constructor creates a new matrix containing the identical
//      values of the copied matrix.  The matrices will, of course, be
//      of the same size, as well.
//
//      The implementation is almost identical to the default constructor.
//      Note we use memcpy to copy the complete matrix over.
//
//      Rows/Columns are set inside the function so we don't break
//      the encapsulation (function calls aren't allowed in the ":"
//      section).
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates::FFCoordinates(const FFCoordinates& matrix) 
                                           : p_rows(0),
                                             p_columns(0),
                                             p_matrix((double**)0)
{
  (*this) = matrix;
}

 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Virtual Matrix Destructor
//
//  Implementation
//      Destruction is implemented within the private "deallocate_memory"
//      member function.
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates::~FFCoordinates(void)
{ 
  if(p_matrix != (double**)0) {
    delete[] p_matrix[0];
  }
  delete[] p_matrix;
  p_matrix = (double**)0; 
}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Assignment Operator
//
//  Implementation
//      The assignment operator replaces the current matrix with
//      another matrix.  Each element and the matrix sizes
//      will be identical.
//
//      If the matrices are not of the same dimensions,
//      the implementation first destroys the old matrix
//      and then allocates the new matrix.
//      Note that we use memcpy to copy the entire matrix at once.
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates& FFCoordinates::operator=(const FFCoordinates& matrix)
{
  if (this != &matrix) {   // check for copy over self
    if (columns() != matrix.columns() || rows() != matrix.rows()) { 
      // Matrix cannot be copied over existing Matrix; allocate new one
      p_rows = matrix.rows();
      p_columns = matrix.columns();
    }
    if ((p_rows != 0) && (p_columns != 0)) {

        /* deallocate */
        if(p_matrix != (double**)0) {
          delete[] p_matrix[0];
          delete[] p_matrix;
          p_matrix = (double**)0;
        }

        p_matrix = (double**) new double*[p_rows];
        double *column = new double[p_rows * p_columns];
        for(int i=0;i<p_rows;i++)
          p_matrix[i] = &column[i*p_columns];
    
        if (matrix.coordinates() != (double**)0)
          memcpy(p_matrix[0], matrix.coordinates()[0],
                 (int)(p_rows * p_columns * sizeof(double)));
        else
          memset(p_matrix[0], (char)0,
                 (int)(p_rows * p_columns * sizeof(double)));
    }
  }
  return *this;  
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Value-Based Matrix Equality
//
//  Implementation
//      The equality operator returns a boolean value indicating whether
//      the values in the two matrices are identical.
//
//      The implementation checks sizes and then uses memcmp to bit
//      compare each matrices private matrix data.
//
///////////////////////////////////////////////////////////////////////////////
bool FFCoordinates::operator==(const FFCoordinates& matrix) const
{
  int rows = matrix.rows();
  int cols = matrix.columns();
  if ((p_rows != rows) || (p_columns != cols)) {
    return false;
  }
  if (memcmp(p_matrix, matrix.p_matrix,
      (int)(p_rows*p_columns*sizeof(double))))
    return false;
  else
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Value-Based Matrix Inequality
//
//  Implementation
//      The inequality operator returns a boolean value indicating whether
//      the values of the two matrices are different.
//
//      The implementation calls operator==.
//
///////////////////////////////////////////////////////////////////////////////
bool FFCoordinates::operator!=(const FFCoordinates& matrix) const
{ return !((*this) == matrix); }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      This member function sums the argument matrix into (this), multiplied
//      by the (optional) scalar x.  This function is  MODIFIER.  
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates* FFCoordinates::add(const FFCoordinates& coords, const double x)
{
  //
  // verify that the matrices are compatible. 
  if (p_columns == coords.columns() && p_rows == coords.rows()) { 
    // Matrices can be added
    double **matrix = coords.coordinates();
    for (int lrow = 0; lrow < rows(); lrow++) {
      for (int rcolumn = 0; rcolumn < columns(); rcolumn++) {
        p_matrix[lrow][rcolumn] += x * matrix[lrow][rcolumn];
      }
    }
  } else {
    EE_RT_ASSERT(0,EE_WARNING,"Matrix dimension mismatch")
  }
  return this;
}

void FFCoordinates::add(int irow, const double *row)
{
  if (irow < p_rows) {
    int rcolumn;
    for (rcolumn = 0; rcolumn < p_columns; rcolumn++) {
      p_matrix[irow][rcolumn] += row[rcolumn];
    }
  } else {
    EE_RT_ASSERT(0,EE_WARNING,"Row not included in matrix");
  }
}
/*
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Add the SparseMatrix argument, multiplied by the (optional) scalar x,
//      to this
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates* FFCoordinates::add(const SparseMatrix& spm, const double x)
{
  int fullrow;
  if ( this->rows() > spm.maxFullIndex() ) { 
    for (int isparse = 0;  isparse< spm.rows(); isparse++) {
      fullrow = spm.fullIndex(isparse);
      for (int rcolumn = 0; rcolumn < 3; rcolumn++) {
	p_matrix[fullrow][rcolumn] += x * spm(isparse, rcolumn);
      }
    }
  } else {
    EE_RT_ASSERT(0,EE_WARNING,"Matrix dimension mismatch")
  }
  return this;
}
*/
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Zero the elements of the matrix.
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates* FFCoordinates::zero(void)
{
  memset(p_matrix[0], (char)0, 
    (int)(p_rows * p_columns * sizeof(double)));
  return this;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Direction cosine (dot product of unit-normalized vectors) of
//      matrices (treated as vectors) a.k.a.
//      SUM(i,j) A(i,j)B(i,j)/(Norm(A)*Norm(B)) = Tr(AB)/(Norm(A)*Norm(B))
//
///////////////////////////////////////////////////////////////////////////////
double FFCoordinates::directionCosine(const FFCoordinates& coords) const
{
  double result = 0.0;
  double **matrix = coords.coordinates();
  if (columns()*rows() == coords.rows()*coords.columns())
    {    // Matrices can be multiplied as vectors
      for (int lrow = 0; lrow < rows(); lrow++)
        for (int rcolumn = 0; rcolumn < coords.columns(); rcolumn++)
          result += p_matrix[lrow][rcolumn] * matrix[lrow][rcolumn];
      double temp = this->norm() * coords.norm();
      if ( fabs(temp) > 1.e-10 ) result /= temp;
    } else {
      EE_RT_ASSERT(0,EE_WARNING, "Matrix dimension mismatch")
    }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Multiply matrix by a scalar double (new name)
//
//  Implementation
//      Multiplies all elements by the scalar argument
//
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates* FFCoordinates::scalarMult(const double scalar)
{
  for (int lrow = 0; lrow < p_rows; lrow++){
    for (int rcolumn = 0; rcolumn < p_columns; rcolumn++) {
      p_matrix[lrow][rcolumn] *= scalar;
    }
  }
  return this;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Normalize the elements of the matrix according to a simple vector
//      norm of all of the elements (i.e., treat the matrix as a vector).
//      By default, it is normalized to one, else to the passed-in quantity.
//
///////////////////////////////////////////////////////////////////////////////
FFCoordinates* FFCoordinates::normalize( double desired_norm )
{
  double norm = 0.0;
  int lrow, rcolumn;
  for ( lrow = 0; lrow < p_rows; lrow++){
    for ( rcolumn = 0; rcolumn < p_columns; rcolumn++) {
      norm += p_matrix[lrow][rcolumn] * p_matrix[lrow][rcolumn];
    }
  }

  double invnorm = desired_norm / sqrt(norm) ;
  for ( lrow = 0; lrow < p_rows; lrow++){
    for ( rcolumn = 0; rcolumn < p_columns; rcolumn++) {
      p_matrix[lrow][rcolumn] *= invnorm;
    }
  }
  return this;
}

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Accessors to Data Members
//
///////////////////////////////////////////////////////////////////////////////
int FFCoordinates::rows(void) const
{ return p_rows; }
int FFCoordinates::columns(void) const
{ return p_columns; }
const double* FFCoordinates::row(int row) const
{ return p_matrix[row]; }

double** FFCoordinates::coordinates( void ) const
{ return p_matrix; }

// --------- Geometry operation accessors --------------
//////////////////////////////////////////////////////////////////////////////
//
//  A number of geometry operations that make sense only for Matrix<double>
//  with three columns are inserted here.  When coordinates are moved to
//  Centers, these will become global functions.
//
////////////////////////////////////////////////////////////////////////////// 


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Returns a vector containing the difference between rows i_row and j_row 
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
double* FFCoordinates::rowdiff(int i_row, int j_row, double* result ) const
{
  if ( result == (double*)0 ) {
    result = new double[3];
  }
  int icol;
  for (icol = 0; icol < 3; icol++) {
    result[icol] = p_matrix[j_row][icol] - p_matrix[i_row][icol];
  }
  return result;

}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//     Returns a vector containing the NORMALIZED difference between 
//     rows i_row and j_row: 
//     space for result vector is passed in from calling routine.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void FFCoordinates::normalized_rowdiff(int i_row, int j_row, 
                                       double* result ) const
{
  double sqnorm = 0;
  int icol;

  for (icol = 0; icol < 3; icol++) {
    result[icol] = p_matrix[j_row][icol]
                 - p_matrix[i_row][icol];
    sqnorm += result[icol] * result[icol];
  }
  if(sqnorm != 0.0) {
    double invnorm = 1.0/sqrt(sqnorm);
    for (icol = 0; icol < 3; icol++) result[icol] *= invnorm;
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Returns a double containing the norm of the difference between 
//      rows i_row and j_row.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
double FFCoordinates::rowdiff_norm( int i_row, int j_row ) const
{
  int icol;
  double norm = 0;
  double temp = 0;

  for (icol = 0; icol < 3; icol++) {
    temp = p_matrix[j_row][icol]
         - p_matrix[i_row][icol];
    norm += temp * temp;
  }

  double result = sqrt(norm);
  return result;
}

// ---------- Printing  ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Print the FFCoorinates to an Output Stream
//      Optional Argument Specifies the Decimal Precision of Elements
//
//  Implementation
//      The implementation sends each element to the ostream.
//
///////////////////////////////////////////////////////////////////////////////
ostream& FFCoordinates::print(ostream& os, unsigned int precision) const
{
  os << "[Matrix<";
  os.width(precision);
  os.setf(ios::showpoint);
  for (int row = 0; row < p_rows; row++) {
    os << "<";
    for (int col = 0; col < p_columns; col++) {
      os << p_matrix[row][col] << " ";
    }
    os << ">" << endl;
  }
  os << ">]" << endl;
  return os;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Print two Matrices to an Output Stream as 6 columns
//      Optional Argument Specifies the Decimal Precision of Elements
//      Intended for debug visualization.
//
///////////////////////////////////////////////////////////////////////////////
ostream& FFCoordinates::print_xyz(ostream& os, const FFCoordinates& matrix, 
			      const char** center_names, 
			      unsigned int precision ) const
{
  os.width(precision);
  os.setf(ios::showpoint);
  os << p_rows << endl;
  os << "output for xyz file " << endl;
  for (int index = 0; index < p_rows; index++) {
    os << *center_names[index] << "  " 
	 << p_matrix[index][0]   << "  "
	 << p_matrix[index][1] << "  "
	 << p_matrix[index][2] <<  "  "
	 << (matrix.coordinates())[index][0] << "  "
	 << (matrix.coordinates())[index][1] << "  "
	 << (matrix.coordinates())[index][2] << endl;
  }
  return os;
}
/*
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Print a FFCoordinates and a SparseMatrix to an Output Stream
//      as 6 columns.
//      Optional Argument Specifies the Decimal Precision of Elements.
//      Intended for debug visualization.
//
///////////////////////////////////////////////////////////////////////////////
ostream& FFCoordinates::print_xyz(ostream& os, 
                              const SparseMatrix& sparsematrix, 
			      const char** center_names, 
			      unsigned int precision ) const
{
  os.width(precision);
  os.setf(ios::showpoint);
  os << this->rows() << endl;
  os << "output for xyz file " << endl;
  for (int index = 0; index < this->rows(); index++) {
    os << *center_names[index] << "  " 
         << p_matrix[index][0]   << "  "
         << p_matrix[index][1] << "  "
         << p_matrix[index][2] <<  "  ";
    for (int isparse=0; isparse<sparsematrix.rows(); isparse++ ) {
      if ( sparsematrix.fullIndex(isparse) == index ) {
         os << (sparsematrix.matrix()->coordinates())[isparse][0] << " "
            << (sparsematrix.matrix()->coordinates())[isparse][1] << " "
            << (sparsematrix.matrix()->coordinates())[isparse][2] << endl;
	 break;
      }
      if ( isparse == sparsematrix.rows()-1 ) {
	os << 0.0 << "  "
	   << 0.0 << "  "
	   << 0.0 <<  endl;
      }
    }
  }
  return os;
}
*/

// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------

// ------------------------
// Private Member Functions
// ------------------------

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:  
//        Cross product of two vectors, ASSUMED TO BE OF LENGTH 3
//
////////////////////////////////////////////////////////////////////////////// 
void FFCoordinates::cross_product( const double* vec1, 
                                   const double* vec2, 
                                         double* t1_x_t2 ) const
{
  t1_x_t2[0] = vec1[1]*vec2[2] - vec1[2]*vec2[1];
  t1_x_t2[1] = vec1[2]*vec2[0] - vec1[0]*vec2[2];
  t1_x_t2[2] = vec1[0]*vec2[1] - vec1[1]*vec2[0];
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Dot product of vectors ASSUMED TO BE THE SAME LENGTH AS A ROW
//      OF THIS MATRIX
//
////////////////////////////////////////////////////////////////////////////// 
double FFCoordinates::dot( double* vec1, double* vec2 ) const
{
  double result = 0.0;
  for (int icol = 0; icol < p_columns; icol++) 
    result += vec1[icol] * vec2[icol];
  return result;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Norm of a row vector ASSUMED TO BE THE SAME LENGTH AS A ROW
//      OF THIS MATRIX
//
////////////////////////////////////////////////////////////////////////////// 
double FFCoordinates::norm( double* vec ) const
{
  double result = 0.0;
  for (int icol = 0; icol < p_columns; icol++) 
    result += vec[icol] * vec[icol];
  result = sqrt(result);
  return result;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Largest norm of any row of this Matrix.
//
//////////////////////////////////////////////////////////////////////////////
double FFCoordinates::maxRowNorm(void) const
{
  double result = 0.0;
  int irow;
  for (irow = 0; irow < p_rows; irow++) {
    double rownorm = 0.0;
    for (int icol = 0; icol < p_columns; icol++) {
      rownorm += pow(p_matrix[irow][icol],2);
    }
    result = MAX(result,rownorm);
  }
  return sqrt(result);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//      Multiply matrices as vectors, a.k.a.
//      double-dot product, SUM(i,j) A(i,j)B(i,j) = Tr(AB)
//      For the case of geometries; this corresponds to a dot product
//      of the full 3N geometry elements considered as a vector.
//
//
///////////////////////////////////////////////////////////////////////////////
double FFCoordinates::dotdot(const FFCoordinates& coords) const
{
  double result = 0.0;
  double **matrix = coords.coordinates();
  if (columns()*rows() == coords.rows()*coords.columns())
    {    // Matrices can be multiplied as vectors
      for (int lrow = 0; lrow < rows(); lrow++)
        for (int rcolumn = 0; rcolumn < coords.columns(); rcolumn++)
          result += (p_matrix[lrow][rcolumn] * matrix[lrow][rcolumn]);
    } else {
      EE_RT_ASSERT(0,EE_WARNING,"Matrix dimension mismatch")
    }
  return result;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Norm of this Matrix (vector norm running over all elements)
//
////////////////////////////////////////////////////////////////////////////// 
double FFCoordinates::norm(void) const
{
  double result = 0.0;
  int irow;
  for (irow = 0; irow < p_rows; irow++) 
    for (int icol = 0; icol < p_columns; icol++) 
      result += p_matrix[irow][icol] * p_matrix[irow][icol];

  result = sqrt(result);
  return result;
}

// ---------- Friends ----------
// ---------- Encapsulated Behavior ----------
