///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFCoord.H
//
//
// CLASS SYNOPSIS:
//    Class FFCoordinates is a Template Class for Builtin C++ Types
//
// EXPORTED TYPES:
//    Class FFCoordinates
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//    FFCoordinates is a class allowing the creation, storage, and retrieval
//    of two-dimensional arrays (matrices) of doubles
//    This class is extremely simple.  It does not allow dynamic growth
//    or many traditional matrix operations.
//
//    WARNING!  Currently little error checking is provided.  Use at your
//    own risk.
//
// EXAMPLES:
//    testMatrixDouble.C
//
//    double values[10] = { 1.0,  0.0,  1.0,
//			    2.0,  1.0,  3.0,
//			   -1.0,  4.0, -2.0, 0.0 };
//    FFCoorinates matrix(3, 3, values);
//    for (int row = 0; row < matrix.rows(); row++) {
//      for (int col = 0; col < matrix.columns(); col++)
//        cout << matrix(row, col) << " ";
//      cout << endl;
//    }
//    matrix.print(cout);
//    
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FFCOORDS_H
#define FFCOORDS_H

#include <iostream>
  using std::ostream;


class FFCoordinates 
{

public:
  // Constructors
    FFCoordinates(int rows = 0, int columns = 0, double* values = 0);
    FFCoordinates(const FFCoordinates& coords);

  // Virtual Destructor
    virtual ~FFCoordinates(void);

  // Operators
    FFCoordinates& operator=(const FFCoordinates& coords);
    bool  operator==(const FFCoordinates& coords) const;
    bool  operator!=(const FFCoordinates& coords) const;

  // Modifiers
    // See operator() above
//    FFCoordinates* add(const SparseMatrix& spmatrix, const double x=1.0);
    void add(int irow, const double *row);
    FFCoordinates* add(const FFCoordinates& coords, const double x=1.0);
    FFCoordinates* scalarMult(const double scalar);
    FFCoordinates* normalize(double desired_norm=1.0);
    FFCoordinates* zero(void);

    /* called in ForceField::finiteDiffError */
    double directionCosine(const FFCoordinates& matrix) const;

  // Accessors
    int    rows(void) const;
    int    columns(void) const;
    const double*   row(int) const;
    double** coordinates( void ) const;

    double norm( void ) const;

    /* called in CGMinimizer */
    double maxRowNorm( void ) const;
    double dotdot(const FFCoordinates& matrix) const;


  // Output
    ostream& print(ostream& os, unsigned int precision = 0) const;

  // Debug output for examining vector quantities
    ostream& print_xyz(ostream& os, const FFCoordinates& matrix,
                       const char** center_names,
                       unsigned int precision = 0 ) const;

//    ostream& print_xyz(ostream& os, const SparseMatrix& spmatrix,
//                       const char** center_names,
//                       unsigned int precision = 0 ) const;

  // basic bond (row difference) operations
    double* rowdiff( int i_row, int j_row,
                     double* result=(double*)0 ) const;
    void normalized_rowdiff( int i_row, int j_row,
                             double* diffvec ) const;
    double rowdiff_norm(int i_row, int j_row ) const;

  // Simple functions to clean up internal vector operations 
  // for Geometry stuff
    void cross_product( const double* vec1, 
                        const double* vec2, double* t1_x_t2 ) const;

    double dot( double* vec1, double* vec2 ) const;
    double norm( double* vec ) const;

protected:

private:

   // State
    int p_rows;
    int p_columns;
    double   **p_matrix;

};

#endif
