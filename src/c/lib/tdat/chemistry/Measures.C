/**
  * @file
  *
  *
  */

#include <math.h>

#include "util/Transformations.H"
#include "tdat/Measures.H"

/**
  *  Return value of the distance between atoms a1 and a2
  */
double Measures::computeLength(TAtm *a1, TAtm *a2)
{
  MPoint p1, p2;
  p1.xyz(a1->coordinates());
  p2.xyz(a2->coordinates());
  MPoint diff = p1.subtract1(p2);
  return diff.length();
}

/**
  *  Return value of the angle (in degrees) between atoms a1,
  *  a2, and a3
  */
double Measures::computeAngle(TAtm *a1, TAtm *a2, TAtm *a3)
{
  double angle, r12 ,r32;
  MPoint p1, p2, p3;

  // Get value of pi
  double pi = 4.0*atan(1.0);

  p1.xyz(a1->coordinates());
  p2.xyz(a2->coordinates());
  p3.xyz(a3->coordinates());

  // Evaluate difference vectors

  MPoint vec12 = p1.subtract1(p2);
  MPoint vec32 = p3.subtract1(p2);

  // Evaluate dot product of difference vectors

  angle = vec12.dotProduct(vec32);

  // find angle (in degrees)

  r12 = vec12.length();
  r32 = vec32.length();
  angle = angle/(r12*r32);
  angle = acos(angle);
  angle = angle*180.0/pi;
  return (angle);
}

/**
  *  Return value of the dihedral angle (in degrees) between atoms
  *  a1, a2, a3, and a4
  */
double Measures::computeDihedral(TAtm *a1, TAtm *a2, TAtm *a3, TAtm *a4)
{
  double dihedralDegrees = 0;
  MPoint v1,v2,v3, p1,p2, p12, tmp;
  double dihedralRadians,  s1, s2, test, cosine;

  v1.xyz(a1->coordinates());
  tmp.xyz(a2->coordinates());
  v1.subtract(tmp);
  v1.normalize();

  v2.xyz(a2->coordinates());
  tmp.xyz(a3->coordinates());
  v2.subtract(tmp);
  v2.normalize();

  v3.xyz(a3->coordinates());
  tmp.xyz(a4->coordinates());
  v3.subtract(tmp);
  v3.normalize();

  p1.xyz(v1);
  p1.crossProduct(v2);
  s1 = p1.length();

  p2.xyz(v2);
  p2.crossProduct(v3);
  s2= p2.length();
  if ((s1*s2)<=1.e-10) {
    return(0.0);       // Dihedral error .. co-linear system
  }
  p12.xyz(p1);
  p12.crossProduct(p2);
  test = p12.dotProduct( v2 );
  // positive or negative
  // torsion

  cosine = p1.dotProduct(p2) / (s1*s2);
  if (cosine>=1.0) cosine=1.0;
  if (cosine<=-1.0) cosine=-1.0;
  dihedralRadians = acos(cosine);

  dihedralDegrees = dihedralRadians * (180/ 3.1415926);
  if (test > 0.0)
    dihedralDegrees = -dihedralDegrees;

  return dihedralDegrees;
}
