///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: SingleGrid.H
//
//
// CLASS SYNOPSIS:
//    SingleGrid contains the description and data for a single grid field.
//      
// DESCRIPTION:
//   For viz.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef SINGLEGRID_HH
#define SINGLEGRID_HH

#include <string>
#include <vector>

using std::string;
using std::vector;

class SingleGrid
{
  public:

// Constructors
    SingleGrid();
    SingleGrid(int size);

// Destructor
    ~SingleGrid();

// Accessors
    string      type();
    string      name();
    int*        dimensions();
    double*     origin();
    double*     corner();
    float*      fieldData();
    float       fieldMax();
    float       fieldMin();
    float       absFieldMax();
    float       absFieldMin();
    int         gridSize();

// Modifiers
    void        type(string fieldType);
    void        name(string fieldName);
    void        dimensions(int x, int y, int z);
    void        origin(double x, double y, double z);
    void        corner(double x, double y, double z);
    void        fieldMax(float max);
    void        fieldMin(float min);
    void        absFieldMax(float absMax);
    void        absFieldMin(float absMin);
    void        setFieldData(float *field);

// General methods
    void        findMinMax();
    SingleGrid *clone();
    void        clearField();
    bool        addFields(float alpha, SingleGrid *grid_a,
                          float beta, SingleGrid *grid_b);
    bool        compare(SingleGrid *grid);

  private:
    string        p_fieldName; //Individual field name
    string        p_fieldType; //Type of grid property
    int           p_dims[3];   //number of elements in x,y,z directions
    double        p_origin[3]; //origin of data
    double        p_corner[3]; //corner of data farthest from origin
    float        *p_field;     //actual field data
    float         p_fieldMin;
    float         p_fieldMax;
    float         p_fieldAbsMax;
    float         p_fieldAbsMin;
};
#endif // SINGLEGRID_HH
