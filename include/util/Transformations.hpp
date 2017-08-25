///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Transformations.H
//
//
//
// CLASS SYNOPSIS:
//  Basic matrix and point transformation routines
//
//
// DESCRIPTION:
//  Contains various basic utilites for creating and manipulating
//  a 4x4 matrix and a 3x1 point
//
//
///////////////////////////////////////////////////////////////////////////////


#ifndef TRANSFORMATIONS_HH
#define TRANSFORMATIONS_HH


///////////////////////////////////////////////////////////////////////////////
//
//   MPoint class - simple 3 doubles representing xyz
//
///////////////////////////////////////////////////////////////////////////////

class MPoint
{
    double px,py,pz;

public:
    MPoint();
    MPoint(double x, double y, double z);
    ~MPoint();

    double x();
    double y();
    double z();
    void x(double z);
    void y(double y);
    void z(double z);
    void xyz(double x, double y, double z);
    void xyz(const double *coords);
    void xyz(MPoint p2);
    void print(const char *name);

    int isEqual(double a, double b);
    int isNull();
    void crossProduct(MPoint p2);
    MPoint crossProduct1(MPoint p2);
    double dotProduct(MPoint p2);
    double length();
    double lengthSqr();
    void normalize();
    void scale(double scale);
    void subtract(MPoint p2);
    MPoint subtract1(MPoint p2);
    void add(MPoint p2);
    MPoint add1(MPoint p2);
};



///////////////////////////////////////////////////////////////////////////////
//
//   MMatrix class - 4x4 matrix of doubles
//
///////////////////////////////////////////////////////////////////////////////



enum MAxisType {XAXIS , YAXIS, ZAXIS};


class MMatrix
{
    double matrix[4][4];

public:
    MMatrix();
    ~MMatrix();

    double get(int i, int j);
    void set(int i, int j, double v);
    void copyMatrix(MMatrix *m1, MMatrix *m2);
    void unitMatrix();
    void transposeMatrix();
    void multiplyMatrix(MMatrix m2);
    void shiftMatrix(MPoint shift);
    void scaleMatrix(MPoint scale);
    void rotateMatrix(double angle, MAxisType axis);
    int vvRotateMatrix(MPoint p1, MPoint p2);
    int alRotateMatrix(MPoint p1, MPoint p2);
    void rotateLineMatrix(MPoint p1, MPoint p2, double angle);
    int invertMatrix();
    MPoint transformPoint(MPoint p1);
    void print(char *name);
};


#endif
