///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Transformations.C
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


#include <iostream>
using std::cout;
using std::endl;

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "util/Transformations.H"

///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//  Point Class - 3 doubles to represent xyz in space
//
//
//
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//   Constructors/ Destructors
//
///////////////////////////////////////////////////////////////////////////////
MPoint::MPoint()
{
    //basic constructor
    //set all double to zero
    px=py=pz=0.0;
}

MPoint::MPoint(double x, double y, double z)
{
    //assignment constructor
    px=x;
    py=y;
    pz=z;
}

MPoint::~MPoint()
{
}

///////////////////////////////////////////////////////////////////////////////
//
//   Simple accessors
//
///////////////////////////////////////////////////////////////////////////////
double MPoint::x() {return px;}
double MPoint::y() {return py;}
double MPoint::z() {return pz;}
void MPoint::x(double x){px=x;}
void MPoint::y(double y){py=y;}
void MPoint::z(double z){pz=z;}

void MPoint::xyz(const double *coords)
{
  px = coords[0];
  py = coords[1];
  pz = coords[2];
}

void MPoint::xyz(double x, double y, double z)
{
  px=x;
  py=y;
  pz=z;
}

void MPoint::xyz(MPoint p2)
{
  px= p2.x();
  py= p2.y();
  pz= p2.z();
}

void MPoint::print(const char *name)
{
    cout << "Point ... "<<name<< " : ";
    cout << px << ", " << py << ", " << pz << endl;
}

///////////////////////////////////////////////////////////////////////////////
//
//   Are two numbers equal, within bounds
//
///////////////////////////////////////////////////////////////////////////////

int MPoint::isEqual(double a, double b)
{
    double c;
    c= fabs(a-b);
    if (c <= 0.00001)return(true);
    return(false);
}


///////////////////////////////////////////////////////////////////////////////
//
//   Null vector ?
//
///////////////////////////////////////////////////////////////////////////////

int MPoint::isNull()
{
    if (isEqual(px,0.0)==false)return(false);
    if (isEqual(py,0.0)==false)return(false);
    if (isEqual(pz,0.0)==false)return(false);
    return(true);
}


///////////////////////////////////////////////////////////////////////////////
//
//   Cross product of two vectors
//
//      Useful for returning a vector perpendicular to the two given
//
///////////////////////////////////////////////////////////////////////////////

void MPoint::crossProduct( MPoint v2)
{
    MPoint temp;
    temp.x( py * v2.z() - pz * v2.y() );
    temp.y( pz * v2.x() - px * v2.z() );
    temp.z( px * v2.y() - py * v2.x() );
    xyz(temp);
}

MPoint MPoint::crossProduct1( MPoint v2)
{
    MPoint temp;
    temp.x( py * v2.z() - pz * v2.y() );
    temp.y( pz * v2.x() - px * v2.z() );
    temp.z( px * v2.y() - py * v2.x() );
    //xyz(temp);

    return temp ;
}

///////////////////////////////////////////////////////////////////////////////
//
//   Dot product of two vectors
//
//      Useful for returning the angle between the two given vectors
//
///////////////////////////////////////////////////////////////////////////////

double MPoint::dotProduct( MPoint v2)
{
    double result;
    result = (px * v2.x()) + (py * v2.y()) + (pz * v2.z());
    return (result);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Length
//
///////////////////////////////////////////////////////////////////////////////
double MPoint::length()
{
    double result= 0.0;
    if (px==0.0 && py==0.0 && pz==0.0)return(result);
    result= sqrt((px * px) + (py * py) + (pz * pz));
    return (result);
}

double MPoint::lengthSqr()
{
    double result= 0.0;
    if (px==0.0 && py==0.0 && pz==0.0)return(result);
    result= (px * px) + (py * py) + (pz * pz);
    return (result);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Normalize Unit Vector
//
///////////////////////////////////////////////////////////////////////////////
void MPoint::normalize()
{
    double len;
    len = length();
    if (isEqual(len, 0.0) == true)
    {
        xyz(0,0,0);
    }
    else
    {
        px = px/len;
        py = py/len;
        pz = pz/len;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//   Scale Vector
//
///////////////////////////////////////////////////////////////////////////////
void MPoint::scale(double scale)
{
    px= px * scale;
    py= py * scale;
    pz= pz *scale;
}

///////////////////////////////////////////////////////////////////////////////
//
//   Subtract Vector
//
///////////////////////////////////////////////////////////////////////////////
void MPoint::subtract(MPoint v2)
{
    px =px - v2.x();
    py =py - v2.y();
    pz =pz - v2.z();
}


MPoint MPoint::subtract1(MPoint v2)
{
   MPoint temp ;
    temp.px = px - v2.x();
    temp.py = py - v2.y();
    temp.pz = pz - v2.z();

    return temp ;
}


///////////////////////////////////////////////////////////////////////////////
//
//   Add Vector
//
///////////////////////////////////////////////////////////////////////////////
void MPoint::add(MPoint v2)
{
    px =px + v2.x();
    py =py + v2.y();
    pz =pz + v2.z();
}

MPoint MPoint::add1(MPoint v2)
{   
   MPoint temp ;
   temp.px =px + v2.x();
   temp.py =py + v2.y();
   temp.pz =pz + v2.z();

   return temp ;
}




///////////////////////////////////////////////////////////////////////////////
//
//
//
//
//  Matrix Class - 4x4 doubles matrix
//
//
//
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//   Constructors/ Destructors
//
///////////////////////////////////////////////////////////////////////////////
MMatrix::MMatrix()
{
    //initialize to unit matrix
    unitMatrix();
}
MMatrix::~MMatrix()
{
}

double MMatrix::get(int i, int j)
{
    return(matrix[i][j]);
}

void MMatrix::set(int i, int j, double v)
{
    matrix[i][j]=v;
}

///////////////////////////////////////////////////////////////////////////////
//
//   Copy Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::copyMatrix(MMatrix *m1, MMatrix *m2)
{
    int i,j;
    for (i = 0; i <= 3; i++)
        for (j = 0; j <= 3; j++)
            m1->set(i,j, m2->get(i,j));
}


///////////////////////////////////////////////////////////////////////////////
//
//   Print Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::print(char *name)
{
    int i,j;
    cout << "Matrix ... " << name << endl;
    for (i = 0; i <= 3; i++)
    {
        for (j = 0; j <= 3; j++)
            cout << matrix[i][j] << " ,  ";
        cout << endl;
    }
    cout << endl;
}


///////////////////////////////////////////////////////////////////////////////
//
//   Unit Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::unitMatrix()
{
    int i,j;
    for (i = 0; i <= 3; i++)
    {
        for (j = 0; j <= 3; j++)
            matrix[i][j] = 0.0;
        matrix[i][i] = 1.0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//   Transpose Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::transposeMatrix()
{
    int i, j;
    MMatrix temp;

    for (i = 0; i <= 3; i++)
    {
        for (j = 0; j <= 3; j++)
            temp.set(i,j, matrix[j][i]);
    }
    copyMatrix(this, &temp);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Muliply Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::multiplyMatrix(MMatrix m2)
{
    int i ,j;
    double temp;
    MMatrix m3;
    double d[4], e[4];

    for (i = 0; i <= 3; i++)
    {
        d[i] = (matrix[i][1] * matrix[i][0]) +  (matrix[i][3] * matrix[i][2]);
        e[i] = (m2.get(0,i) * m2.get(1,i)) + (m2.get(2,i) * m2.get(3,i));
    }

    for (i = 0; i <= 3; i++)
    {
        for (j = 0; j <= 3; j++)
        {
            temp = (matrix[i][1] + m2.get(0,j)) * (matrix[i][0] + m2.get(1,j));
            temp += (matrix[i][3] + m2.get(2,j)) * (matrix[i][2] + m2.get(3,j)) - d[i] - e[j];
            m3.set(i,j, temp);
        }
    }
    copyMatrix(this, &m3);
 }



///////////////////////////////////////////////////////////////////////////////
//
//   Generate Shift Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::shiftMatrix(MPoint shift)
{
    unitMatrix();
    matrix[0][3]= shift.x();
    matrix[1][3]= shift.y();
    matrix[2][3]= shift.z();
}


///////////////////////////////////////////////////////////////////////////////
//
//   Generate Scale Matrix
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::scaleMatrix(MPoint scale )
{
    unitMatrix();
    matrix[0][0]= scale.x();
    matrix[1][1]= scale.y();
    matrix[2][2]= scale.z();
}


///////////////////////////////////////////////////////////////////////////////
//
//   Generate Rotation Matrix given axis rotation value
//
///////////////////////////////////////////////////////////////////////////////
void MMatrix::rotateMatrix(double angle, MAxisType axis )
{
    double cosTheta, sinTheta, degToRad;
    degToRad = 3.14159263 / 180.0 ;

    cosTheta  = cos(angle * degToRad);
    sinTheta =  sin(angle * degToRad);
    unitMatrix();
    switch (axis)
    {
        case XAXIS:
            matrix[2][2] = cosTheta;
            matrix[1][1] = cosTheta;
            matrix[1][2] = -sinTheta;
            matrix[2][1] = sinTheta;
            break;

        case YAXIS:
            matrix[0][0] = cosTheta;
            matrix[2][2] = cosTheta;
            matrix[0][2] = sinTheta;
            matrix[2][0] = -sinTheta;
            break;

        case ZAXIS:
            matrix[0][0] = cosTheta;
            matrix[1][1] = cosTheta;
            matrix[0][1] = -sinTheta;
            matrix[1][0] = sinTheta;
            break;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//   Generate Rotation Matrix to rotate from vector1 to vector2
//  Cf ROGERS and ADAMS page 55-59
///////////////////////////////////////////////////////////////////////////////

int MMatrix::alRotateMatrix(MPoint p1, MPoint p2)
{
    MPoint  perp;
    double t1, cosTheta, sinTheta, Theta;
//    double pi = 4.0 * atan(1.0) ;

    p1.normalize();
    p2.normalize();

    /* Get sine theta (cross product) and cos theta (dot product) */
    cosTheta = p1.x() * p2.x() + p1.y() * p2.y() + p1.z() * p2.z();

    // evaluate sin(Theta), making sure to keep track of additional
    // factor of PI
    perp.x(p1.x() - cosTheta*p2.x());
    perp.y(p1.y() - cosTheta*p2.y());
    perp.z(p1.z() - cosTheta*p2.z());
    sinTheta = p2.x()*(p1.x()-perp.x())
             + p2.y()*(p1.y()-perp.y())
             + p2.z()*(p1.z()-perp.z());

    Theta = acos( cosTheta ) ;

//    if ( Theta > pi/2 )
//       Theta = Theta - pi ;

    cosTheta = cos( Theta ) ;
    
    p1.crossProduct(p2);

    sinTheta = sin(Theta) ;

  //    if ( sinTheta== 0 ) sinTheta=0.0001; //fudge to stop

    p1.normalize();
    unitMatrix();   /* get a unit matrix */
    t1 = 1.0 - cosTheta;
    matrix[0][0] = (p1.x() * p1.x()) + (1.0 - (p1.x() * p1.x())) * cosTheta;
    matrix[1][1] = (p1.y() * p1.y()) + (1.0 - (p1.y() * p1.y())) * cosTheta;
    matrix[2][2] = (p1.z() * p1.z()) + (1.0 - (p1.z() * p1.z())) * cosTheta;
    matrix[1][0] = (p1.x() * p1.y() * t1) + (p1.z() * sinTheta);
    matrix[2][0] = (p1.x() * p1.z() * t1) - (p1.y() * sinTheta);
    matrix[0][1] = (p1.x() * p1.y() * t1) - (p1.z() * sinTheta);
    matrix[2][1] = (p1.y() * p1.z() * t1) + (p1.x() * sinTheta);
    matrix[0][2] = (p1.x() * p1.z() * t1) + (p1.y() * sinTheta);
    matrix[1][2] = (p1.y() * p1.z() * t1) - (p1.x() * sinTheta);
    return (0);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Generate Rotation Matrix to rotate from vector1 to vector2
//  Cf ROGERS and ADAMS page 55-59
///////////////////////////////////////////////////////////////////////////////

int MMatrix::vvRotateMatrix(MPoint p1, MPoint p2)
{
    MPoint  nvec;
    double t1, cosTheta, sinTheta;

    p1.normalize();
    p2.normalize();


    /* Get sine theta (cross product) and cos theta (dot product) */
    cosTheta = p1.x() * p2.x() + p1.y() * p2.y() + p1.z() * p2.z();
    p1.crossProduct(p2);


    sinTheta = p1.length();
  //    if ( sinTheta== 0 )sinTheta=0.0001; //fudge to stop

    p1.normalize();
    unitMatrix();   /* get a unit matrix */
    t1 = 1.0 - cosTheta;
    matrix[0][0] = (p1.x() * p1.x()) + (1.0 - (p1.x() * p1.x())) * cosTheta;
    matrix[1][1] = (p1.y() * p1.y()) + (1.0 - (p1.y() * p1.y())) * cosTheta;
    matrix[2][2] = (p1.z() * p1.z()) + (1.0 - (p1.z() * p1.z())) * cosTheta;
    matrix[1][0] = (p1.x() * p1.y() * t1) + (p1.z() * sinTheta);
    matrix[2][0] = (p1.x() * p1.z() * t1) - (p1.y() * sinTheta);
    matrix[0][1] = (p1.x() * p1.y() * t1) - (p1.z() * sinTheta);
    matrix[2][1] = (p1.y() * p1.z() * t1) + (p1.x() * sinTheta);
    matrix[0][2] = (p1.x() * p1.z() * t1) + (p1.y() * sinTheta);
    matrix[1][2] = (p1.y() * p1.z() * t1) - (p1.x() * sinTheta);
    return (0);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Generate Rotation Matrix to rotate about a line given by two points
//
///////////////////////////////////////////////////////////////////////////////

void MMatrix::rotateLineMatrix(MPoint p1, MPoint p2,  double angle)
{
    MMatrix shift, rotate;
    MPoint toOrigin, fromOrigin, nvec;
    double t1, cosTheta, sinTheta, degToRad;
    degToRad = 3.14159263 / 180.0 ;

    nvec.xyz(p2);
    nvec.subtract(p1);
    if (nvec.isNull())return;
    nvec.normalize();

    cosTheta = cos(angle * degToRad);
    sinTheta = sin(angle * degToRad);
    t1 = 1.0 - cosTheta;

    toOrigin.xyz(p1);
    toOrigin.scale(-1.0);

    fromOrigin.xyz(p1);

    //make the rotation matrix
    unitMatrix(); /* get a unit matrix */
    matrix[0][0] = (nvec.x() * nvec.x()) + ((1.0-(nvec.x()*nvec.x()))*cosTheta);
    matrix[1][1] = (nvec.y() * nvec.y()) + ((1.0-(nvec.y()*nvec.y()))*cosTheta);
    matrix[2][2] = (nvec.z() * nvec.z()) + ((1.0-(nvec.z()*nvec.z()))*cosTheta);
    matrix[1][0] = (nvec.x() * nvec.y() * t1) + (nvec.z() * sinTheta);
    matrix[2][0] = (nvec.x() * nvec.z() * t1) - (nvec.y() * sinTheta);
    matrix[0][1] = (nvec.x() * nvec.y() * t1) - (nvec.z() * sinTheta);
    matrix[2][1] = (nvec.y() * nvec.z() * t1) + (nvec.x() * sinTheta);
    matrix[0][2] = (nvec.x() * nvec.z() * t1) + (nvec.y() * sinTheta);
    matrix[1][2] = (nvec.y() * nvec.z() * t1) - (nvec.x() * sinTheta);
    copyMatrix(&rotate, this);

    //   multiply together in reverse order ...
    //              fromOrigin .x. rotate .x. toOrigin

    shiftMatrix(fromOrigin);
    multiplyMatrix(rotate);

    shift.shiftMatrix(toOrigin);
    multiplyMatrix(shift);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Transform point by matrix
//
///////////////////////////////////////////////////////////////////////////////

MPoint MMatrix::transformPoint( MPoint point)
{
    MPoint temp;
    double w;

    temp.x( (matrix[0][0] * point.x()) + (matrix[0][1] * point.y()) +
                            (matrix[0][2] * point.z()) + matrix[0][3]);

    temp.y((matrix[1][0] * point.x()) + (matrix[1][1] * point.y()) +
                            (matrix[1][2] * point.z()) + matrix[1][3]);

    temp.z((matrix[2][0] * point.x()) + ( matrix[2][1] * point.y()) +
                            (matrix[2][2] * point.z()) + matrix[2][3]);

    w = matrix[3][0] * point.x() + matrix[3][1] * point.y() +
        matrix[3][2] * point.z() + matrix[3][3];

    if (fabs(w)<0.00001)w=0.00001;
    w = 1.0 / w;
    temp.x( temp.x()*w );
    temp.y( temp.y()*w );
    temp.z( temp.z()*w );
    return temp;
}


///////////////////////////////////////////////////////////////////////////////
//
//   Invert matrix
//
///////////////////////////////////////////////////////////////////////////////

int MMatrix::invertMatrix()
{
    double b[4], c[4], w, y;
    int z[4], i, j, k, l, p;
    MMatrix invmat;

    copyMatrix(&invmat, this);

    for (j = 1; j <= 4; j++)
        z[j - 1] = j;
    for (i = 1; i <= 4; i++)
    {
        k = i;
        y = invmat.get(i-1, i-1);
        l = i - 1;
        p = i + 1;
        for (j = p; j <= 4; j++)
        {
            w = invmat.get(i-1,j-1);
            if (fabs(w) > fabs(y))
            {
                k = j;
                y = w;
            }
        }
        if (fabs(y)<=0.0001) return (-1);  /* matrix has no inverse */
        y = 1.0 / y;
        for (j = 0; j <= 3; j++)
        {
            c[j] = invmat.get(j,k-1);
            invmat.set(j,k-1, invmat.get(j,i-1));
            invmat.set(j, i-1,  -c[j] * y);
            invmat.set(i-1,j, invmat.get(i-1,j) *y);
            b[j] = invmat.get(i-1,j);
        }
        invmat.set(i-1,i-1,y);
        j = z[i - 1];
        z[i - 1] = z[k - 1];
        z[k - 1] = j;
        for (k = 0; k < l; k++)
        {
            for (j = 0; j < l; j++)
                invmat.set(k,j, invmat.get(k,j)- ( b[j] * c[k]));
            for (j = p - 1; j <= 3; j++)
                invmat.set(k,j, invmat.get(k,j)- ( b[j] * c[k]));
        }
        for (k = p - 1; k <= 3; k++)
        {
            for (j = 0; j < l; j++)
                invmat.set(k,j, invmat.get(k,j)- ( b[j] * c[k]));
            for (j = p - 1; j <= 3; j++)
                invmat.set(k,j, invmat.get(k,j)- ( b[j] * c[k]));
        }
    }
    for (i = 0; i <= 3; i++)
    {
        do
        {
            k = z[i];
            if (k != i + 1)
            {
                for (j = 0; j <= 3; j++)
                {
                    w = invmat.get(i,j);
                    invmat.set(i,j, invmat.get(k-1,j));
                    invmat.set(k-1,j,w);
                }
                p = z[i];
                z[i] = z[k - 1];
                z[k - 1] = p;
            }
        }
        while (k != i + 1);
    }
    copyMatrix(this, &invmat);

    return 0;
}
