#include <iostream>
using namespace std;
#include <float.h>
#include <math.h>           // angle calculations
#include <stdio.h>
#include <algorithm>

#include "tdat/LatticeDef.H"

//#define DEBUG

LatticeDef::LatticeDef(double a, double b, double c,
            double alpha, double beta, double gamma) 
{
   p_repa1 = p_repa2 = p_repa3 = 1;
   p_center = true;

   assign(a, b, c, alpha, beta, gamma);
   p_fragcenter.x(0);
   p_fragcenter.y(0);
   p_fragcenter.z(0);
}

LatticeDef::LatticeDef(const LatticeDef& rhs)
{
   p_vectors = rhs.p_vectors;
   p_reciprocalVectors = rhs.p_reciprocalVectors;
   p_fragcenter = rhs.p_fragcenter;
   p_center = rhs.p_center;
   p_repa1 = rhs.p_repa1;
   p_repa2 = rhs.p_repa2;
   p_repa3 = rhs.p_repa3;
}

LatticeDef::~LatticeDef()
{
}

LatticeDef& LatticeDef::assign(double a, double b, double c,
            double alpha, double beta, double gamma)
{
   double m[3][3];
   toMatrix(a, b, c, alpha, beta, gamma, m);
   // NOTE: vectors in columns
   MPoint p;
   p_vectors.clear();
   for (int i=0; i<3; i++) {
     p.xyz(m[i][0],m[i][1],m[i][2]);
     p_vectors.push_back(p);
   }

   double volume =
     p_vectors[0].dotProduct(p_vectors[1].crossProduct1(p_vectors[2]));
   p_reciprocalVectors.clear();
   p_reciprocalVectors.push_back(p_vectors[1].crossProduct1(p_vectors[2]));
   p_reciprocalVectors.push_back(p_vectors[2].crossProduct1(p_vectors[0]));
   p_reciprocalVectors.push_back(p_vectors[0].crossProduct1(p_vectors[1]));
   p_reciprocalVectors[0].scale(1.0/volume);
   p_reciprocalVectors[1].scale(1.0/volume);
   p_reciprocalVectors[2].scale(1.0/volume);
   return *this;
}


LatticeDef& LatticeDef::assign(MPoint a1, MPoint a2, MPoint a3)
{
   p_vectors.clear();
   p_vectors.push_back(a1);
   p_vectors.push_back(a2);
   p_vectors.push_back(a3);

   double volume =
     p_vectors[0].dotProduct(p_vectors[1].crossProduct1(p_vectors[2]));
   p_reciprocalVectors.clear();
   p_reciprocalVectors.push_back(p_vectors[1].crossProduct1(p_vectors[2]));
   p_reciprocalVectors.push_back(p_vectors[2].crossProduct1(p_vectors[0]));
   p_reciprocalVectors.push_back(p_vectors[0].crossProduct1(p_vectors[1]));
   p_reciprocalVectors[0].scale(1.0/volume);
   p_reciprocalVectors[1].scale(1.0/volume);
   p_reciprocalVectors[2].scale(1.0/volume);
   return *this;
}

void LatticeDef::setReplicationFactors(int na1, int na2, int na3)
{
   p_repa1 = na1;
   p_repa2 = na2;
   p_repa3 = na3;
}

void LatticeDef::getReplicationFactors(int& na1, int& na2, int& na3) const
{
   na1 = p_repa1;
   na2 = p_repa2;
   na3 = p_repa3;
}

void LatticeDef::setCentering(bool center)
{
   p_center = center;
}

bool LatticeDef::getCentering() const
{
   return p_center;
}

/*
void LatticeDef::setA(double newa)
{
   double a,b,c,alpha,beta,gamma;
   getLattice(a,b,c,alpha,beta,gamma);
   assign(newa,b,c,alpha,beta,gamma);
}
*/


/**
 * Return lattice values from internally held lattice vectors.
 */
void LatticeDef::getLattice(double& a, double& b, double& c,
                            double& alpha, double& beta, double& gamma) const
{
   MPoint a1 = p_vectors[0];
   MPoint a2 = p_vectors[1];
   MPoint a3 = p_vectors[2];

   double d2;
   //a = sqrt(a1.x() * a1.x() + a1.y() * a1.y() + a1.z() * a1.z());
   //c = sqrt(a3.x() * a3.x() + a3.y() * a3.y() + a3.z() * a3.z());
   //b = sqrt(a2.x() * a2.x() + a2.y() * a2.y() + a2.z() * a2.z());
   a = a1.length();
   b = a2.length();
   c = a3.length();
#ifdef DEBUG
   cout << "abc " << a << " " << b << " " << c << endl;
#endif

   // column1 = a1
   // column2 = a2
   // column3 = a3
   d2 = pow(a2.x() - a3.x(),2.) +
        pow(a2.y() - a3.y(),2.) +
        pow(a2.z() - a3.z(),2.);
   //d2 = (lattice_unita(1,2)-lattice_unita(1,3))**2
   //           + (lattice_unita(2,2)-lattice_unita(2,3))**2
   //           + (lattice_unita(3,2)-lattice_unita(3,3))**2
   alpha = (b* b + c*c - d2)/(2.0 * b * c);
   alpha = acos(alpha)*180.0 / M_PI;


   //d2 = (lattice_unita(1,3)-lattice_unita(1,1))**2
   //           + (lattice_unita(2,3)-lattice_unita(2,1))**2
   //           + (lattice_unita(3,3)-lattice_unita(3,1))**2
   d2 = pow(a3.x() - a1.x(), 2.) +
        pow(a3.y() - a1.y(), 2.) +
        pow(a3.z() - a1.z(), 2.);
   beta = (c*c + a*a - d2)/(2.0*c*a);
   beta = acos(beta)*180.0/ M_PI;

   d2 = pow(a1.x() - a2.x(), 2.) +
        pow(a1.y() - a2.y(), 2.) +
        pow(a1.z() - a2.z(), 2.);
   //d2 = (lattice_unita(1,1)-lattice_unita(1,2))**2
   //     + (lattice_unita(2,1)-lattice_unita(2,2))**2
   //     + (lattice_unita(3,1)-lattice_unita(3,2))**2
   gamma = (a*a + b*b - d2)/(2.0* a * b);
   gamma = acos(gamma)*180.0/ M_PI;
}



void LatticeDef::setFragCenter(double x, double y, double z)
{
   p_fragcenter.x(x);
   p_fragcenter.y(y);
   p_fragcenter.z(z);
}


/**
 * Generate three vectors a1,a2,a3 from the lattice definition.
 * Caller responsible for memory
 */
vector<MPoint> *LatticeDef::toVectors() const
{
   vector<MPoint> *ret = new vector<MPoint>;
   *ret = p_vectors;

   return ret;
}

/**
 * Generate three reciprocal lattice vectors b1,b2,b3 from the lattice
 * definition. Caller responsible for memory
 */
vector<MPoint> *LatticeDef::toReciprocalVectors() const
{
   vector<MPoint> *ret = new vector<MPoint>;
   *ret = p_reciprocalVectors;

   return ret;
}

/**
 * Generate the normal vector based on the three Miller indices h,k,l
 */
MPoint LatticeDef::getPlaneNormal(int h, int k, int l)
{
  MPoint ret;
  ret.x(((double)h)*p_reciprocalVectors[0].x() + 
        ((double)k)*p_reciprocalVectors[1].x() + 
        ((double)l)*p_reciprocalVectors[2].x());
  ret.y(((double)h)*p_reciprocalVectors[0].y() + 
        ((double)k)*p_reciprocalVectors[1].y() + 
        ((double)l)*p_reciprocalVectors[2].y());
  ret.z(((double)h)*p_reciprocalVectors[0].z() + 
        ((double)k)*p_reciprocalVectors[1].z() + 
        ((double)l)*p_reciprocalVectors[2].z());
  return ret;
}

/**
 * Generates the edges of the unit cell.
 * Each line is represented by two points.  
 * Lines for replicated cells is not currently returned.
 *
 * What to do about start location of box.... setCenter??
 * The first line is
 * 0,0,0 - a1 and is held in the first two points and so on.
 *
 * Caller responsible for memory.
 */
// should be const but don't want to change MPoint
//vector<MPoint> *LatticeDef::toLines() const
vector<MPoint> *LatticeDef::toLines() 
{
   vector<MPoint> *ret = new vector<MPoint>;

   vector<MPoint> *vecs = toVectors();
   MPoint a = (*vecs)[0];
   MPoint b = (*vecs)[1];
   MPoint c = (*vecs)[2];
   MPoint ab = a.add1(b);
   MPoint ac = a.add1(c);
   MPoint bc = b.add1(c);
   MPoint abc = a.add1(b).add1(c);
   MPoint origin(0.,0.,0.);
//   MPoint o = p_fragcenter;
//   o.subtract(MPoint(a.length()/2., b.length()/2., c.length()/2.));
//#ifdef DEBUG
//   o.print("corner ");
//#endif
   delete vecs;

   // Edge  o->a
   ret->push_back(origin);
   ret->push_back(a);

   // Edge o->b
   ret->push_back(origin);
   ret->push_back(b);

   // Edge 0->c
   ret->push_back(origin);
   ret->push_back(c);

   // ---
   ret->push_back(a);
   ret->push_back(ab);

   ret->push_back(a);
   ret->push_back(ac);

   ret->push_back(b);
   ret->push_back(ab);

   ret->push_back(b);
   ret->push_back(bc);


   ret->push_back(c);
   ret->push_back(ac);

   ret->push_back(c);
   ret->push_back(bc);

   // Connections to abc
   ret->push_back(ac);
   ret->push_back(abc);

   ret->push_back(ab);
   ret->push_back(abc);

   ret->push_back(bc);
   ret->push_back(abc);

   if (p_center) {
      // compute box center
      int numpoints = ret->size();
      double cmx = 0.0;
      double cmy = 0.0;
      double cmz = 0.0;
      double mtot = 0.0;
      double weight = 1.0;


      MPoint p;
      for (int idx=0; idx<numpoints; idx++) {
         p = (*ret)[idx];
         mtot += weight;
         cmx += weight*p.x();
         cmy += weight*p.y();
         cmz += weight*p.z();
      }
      cmx = cmx/mtot;
      cmy = cmy/mtot;
      cmz = cmz/mtot;
      // This is the center of the lattice.
      // The lattice corner is currently 0,0,0
      p.xyz(-cmx, -cmy, -cmz);
      //cout << "center of box = " << cmx << " " << cmy << " " << cmz << endl;
      //p_fragcenter.print("fragcenter");

      for (int idx=0; idx<numpoints; idx++) {
         (*ret)[idx].add(p_fragcenter);
         (*ret)[idx].add(p);
      }
   }


   return ret;
}

/**
 * Currently not keeping the lattice corner so compute it.
 */
MPoint LatticeDef::getLatticeCorner()
{
   vector<MPoint> *lines = toLines();
   MPoint ret = (*lines)[0];
   delete lines;
   return ret;
}

/** Find range of displacement parameter for a given normal and
 *  bounding box
 */
double LatticeDef::displacementRange(MPoint normal)
{
  // Get intersections with bounding box corners
  const vector<MPoint> *basis = toVectors();
  double tau = 0.0;
  double xn;
  MPoint corner, tmp;
  MPoint a0 = (*basis)[0];
  MPoint a1 = (*basis)[1];
  MPoint a2 = (*basis)[2];

  // Adjust basis vectors so that normal points into unit cell
  if (a0.dotProduct(normal)<0.0) a0.scale(-1.0);
  if (a1.dotProduct(normal)<0.0) a1.scale(-1.0);
  if (a2.dotProduct(normal)<0.0) a2.scale(-1.0);

  corner.xyz(a0);
  corner.scale((double)p_repa1);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  corner.xyz(a1);
  corner.scale((double)p_repa2);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  corner.xyz(a2);
  corner.scale((double)p_repa3);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  corner.xyz(a0);
  corner.scale((double)p_repa1);
  tmp = a1;
  tmp.scale((double)p_repa2);
  corner.add(tmp);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  corner.xyz(a0);
  corner.scale((double)p_repa1);
  tmp = a2;
  tmp.scale((double)p_repa3);
  corner.add(tmp);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  corner.xyz(a1);
  corner.scale((double)p_repa2);
  tmp = a2;
  tmp.scale((double)p_repa3);
  corner.add(tmp);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  corner.xyz(a0);
  corner.scale((double)p_repa1);
  tmp = a1;
  tmp.scale((double)p_repa2);
  corner.add(tmp);
  tmp = a2;
  tmp.scale((double)p_repa3);
  corner.add(tmp);
  xn = fabs(corner.dotProduct(normal));
  if (xn > tau) tau = xn;
  return tau;
}

/** Find intersection of a plane described by displ and normal with
 *  bounding box
 */
void LatticeDef::planeCorners(MPoint displ, MPoint normal, int &n,
                              vector<MPoint> &vertices)
{
  vertices.clear();
  n = 0;

  // Get lattice origin
  MPoint origin = getLatticeCorner();
  MPoint start = getPlaneOrigin(normal);

  // Get lattice basis
  const vector<MPoint> *basis = toVectors();
  MPoint a = (*basis)[0];
  MPoint b = (*basis)[1];
  MPoint c = (*basis)[2];
  bool ok;
  double tau;
  MPoint intrsct;

  a.scale((double)p_repa1);
  b.scale((double)p_repa2);
  c.scale((double)p_repa3);


  // Check for intersections
  MPoint orig;
  MPoint direction;
  MPoint planeOrig;

  planeOrig.xyz(displ.x(), displ.y(), displ.z());
  planeOrig.add(start);

  orig.xyz(origin.x(), origin.y(), origin.z());
  direction.xyz(a.x(), a.y(), a.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+a.x(), origin.y()+a.y(), origin.z()+a.z());
  direction.xyz(b.x(), b.y(), b.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x(), origin.y(), origin.z());
  direction.xyz(b.x(), b.y(), b.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+b.x(), origin.y()+b.y(), origin.z()+b.z());
  direction.xyz(a.x(), a.y(), a.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x(), origin.y(), origin.z());
  direction.xyz(c.x(), c.y(), c.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+a.x(), origin.y()+a.y(), origin.z()+a.z());
  direction.xyz(c.x(), c.y(), c.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x() + a.x() + b.x(), origin.y() + a.y() + b.y(),
           origin.z() + a.z() + b.z());
  direction.xyz(c.x(), c.y(), c.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+b.x(), origin.y()+b.y(), origin.z()+b.z());
  direction.xyz(c.x(), c.y(), c.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+c.x(), origin.y()+c.y(), origin.z()+c.z());
  direction.xyz(a.x(), a.y(), a.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+a.x()+c.x(), origin.y()+a.y()+c.y(), origin.z()+a.z()+c.z());
  direction.xyz(b.x(), b.y(), b.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+c.x(), origin.y()+c.y(), origin.z()+c.z());
  direction.xyz(b.x(), b.y(), b.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  orig.xyz(origin.x()+b.x()+c.x(), origin.y()+b.y()+c.y(), origin.z()+b.z()+c.z());
  direction.xyz(a.x(), a.y(), a.z());
  ok = getIntersection(planeOrig, normal, orig, direction, intrsct, tau);
  if (ok && tau >= 0.0 and tau <= 1.0) {
    n++;
    vertices.push_back(intrsct);
  }

  // Order points so that they are accessed in a clockwise or counter clockwise
  // manner about the center

  if (n>0) {
    MPoint center;

    center.xyz(0.0, 0.0, 0.0);
    int i, j;

    //Find center of points
    for (i=0; i<n; i++) {
      center.add(vertices[i]);
    }
    center.scale(1.0/((double)n));

    // Create orthonormal vectors in plane of points
    MPoint vec1, vec2;
    double vdot;
    if (normal.x() != 0.0 || normal.y() != 0.0) {
      vec1.xyz(0.0, 0.0, 1.0);
    } else {
      vec1.xyz(1.0, 0.0, 0.0);
    }
    vdot = vec1.dotProduct(normal);
    vec2.xyz(normal.x(), normal.y(), normal.z());
    vec2.scale(vdot);
    vec1.subtract(vec2);
    vec1.normalize();

    vec2.xyz(normal.x(), normal.y(), normal.z());
    vec2.crossProduct(vec1);
    vec2.normalize();

    // Get angle of points
    double length, cos1, cos2, pi, theta;
    pi = 4.0*atan(1.0);
    vector<double> angle;

    MPoint displacement;
    for (i=0; i<n; i++) {
      displacement.xyz(vertices[i]);
      displacement.subtract(center);
      length = displacement.length();
      if (length > 0.0) {
        cos1 = displacement.dotProduct(vec1);
        cos2 = displacement.dotProduct(vec2);
        cos1 /= length;
        cos2 /= length;
        if (cos1 >  1.0) cos1 =  1.0;
        if (cos1 < -1.0) cos1 = -1.0;
        theta = acos(cos1);
        if (cos2 < 0) {
          theta = 2.0*pi - theta;
        }
      } else {
        theta = 0.0;
      }
      angle.push_back(theta);
    }

    // Sort points by increasing angle
    double tmpAngle;
    MPoint tmpPnt;

    for (i=0; i<n; i++) {
      for (j=i+1; j<n; j++) {
        if (angle[i] > angle[j]) {
          tmpAngle = angle[i];
          tmpPnt.xyz(vertices[i].x(), vertices[i].y(), vertices[i].z());
          angle[i] = angle[j];
          vertices[i].xyz(vertices[j].x(), vertices[j].y(), vertices[j].z());
          angle[j] = tmpAngle;
          vertices[j].xyz(tmpPnt.x(), tmpPnt.y(), tmpPnt.z());
        }
      }
    }
#ifdef DEBUG
    for (i=0; i<n; i++) {
      cout<<"Angle["<<i<<"]: "<<angle[i]<<endl;
    }
#endif
  }
}

/** Find two lattice vectors that are perpendicular to the reciprocal
 *  lattice vector defined by h,k,l
 */
bool LatticeDef::getLatticePlane(int h,  int k,  int l,
                                 int &h1, int &k1, int &l1,
                                 int &h2, int &k2, int &l2)
{
  if (h == 0 && k == 0 && l == 0) return false;

  // Find kmax
  int kmax = ((int)sqrt((double)(h*h+k*k+l*l))) + 1;

  // Find vectors that are perpendicular to hkl
  int ii, jj, kk;
  vector<int> hsav, ksav, lsav;
  for (ii = -kmax; ii<=kmax; ii++) {
    for (jj = -kmax; jj<=kmax; jj++) {
      for (kk = -kmax; kk<=kmax; kk++) {
        if (ii*h + jj*k + kk*l == 0 && ii*ii+jj*jj+kk*kk > 0) {
          hsav.push_back(ii);
          ksav.push_back(jj);
          lsav.push_back(kk);
        }
      }
    }
  }
  if (hsav.size() == 0) return false;
#ifdef DEBUG
  cout <<"Found "<<hsav.size()<<" vectors that are perpendicular to hkl"<<endl;
  for (ii=0; ii<hsav.size(); ii++) {
    cout <<"hkl["<<ii<<"]: "<<hsav[ii]<<" "<<ksav[ii]<<" "<<lsav[ii]<<endl;
  }
#endif

  // Find the shortest vector perpendicular to hkl
  int isqr = hsav[0]*hsav[0]+ksav[0]*ksav[0]+lsav[0]*lsav[0];
  int idx = 0;
  for (ii = 0; ii<hsav.size(); ii++) {
    if (hsav[ii]*hsav[ii]+ksav[ii]*ksav[ii]+lsav[ii]*lsav[ii]<isqr) {
      isqr = hsav[ii]*hsav[ii]+ksav[ii]*ksav[ii]+lsav[ii]*lsav[ii];
      idx = ii;
    }
  }
  // Choose this vector as one of the basis vectors in the new unit cell
  h1 = hsav[idx];
  k1 = ksav[idx];
  l1 = lsav[idx];
#ifdef DEBUG
  cout <<"Shortest vector perpendicular to hkl is "<<h1<<" "<<k1<<" "<<l1<<endl;
#endif

  // Find a second vector that is perpendicular to the first
  vector<int> hsav2, ksav2, lsav2;
  for (ii = 0; ii<hsav.size(); ii++) {
#ifdef DEBUG
    cout<<"hh1+kk1+ll1["<<ii<<"]: "<<h1*hsav[ii]+k1*ksav[ii]+l1*lsav[ii]<<endl;
#endif
    if (h1*hsav[ii]+k1*ksav[ii]+l1*lsav[ii]==0) {
      hsav2.push_back(hsav[ii]);
      ksav2.push_back(ksav[ii]);
      lsav2.push_back(lsav[ii]);
    }
  }
  if (hsav2.size() == 0) return false;
#ifdef DEBUG
  cout <<"Found "<<hsav2.size()<<" vectors that are perpendicular to hkl and h1k1l1"<<endl;
  for (ii=0; ii<hsav2.size(); ii++) {
    cout <<"hkl["<<ii<<"]: "<<hsav2[ii]<<" "<<ksav2[ii]<<" "<<lsav2[ii]<<endl;
  }
#endif

  // Find the shortest vector perpendicular to hkl and h1,k1,l1
  isqr = hsav2[0]*hsav2[0]+ksav2[0]*ksav2[0]+lsav2[0]*lsav2[0];
  idx = 0;
  for (ii = 0; ii<hsav2.size(); ii++) {
    if (hsav2[ii]*hsav2[ii]+ksav2[ii]*ksav2[ii]+lsav2[ii]*lsav2[ii]<isqr) {
      isqr = hsav2[ii]*hsav2[ii]+ksav2[ii]*ksav2[ii]+lsav2[ii]*lsav2[ii];
      idx = ii;
    }
  }
  h2 = hsav2[idx];
  k2 = ksav2[idx];
  l2 = lsav2[idx];

#ifdef DEBUG
  cout <<"Shortest vector perpendicular to hkl and h1k1l1 is "<<h2<<" "<<k2<<" "<<l2<<endl;
#endif
  return true;
}

/** Transform old unit cell into a new unit cell defined by the triplets
 *  hkl, h1k1l1, h2k2l2
 */
void LatticeDef::transformCell(int h,  int k,  int l,
                               int h1, int k1, int l1,
                               int h2, int k2, int l2)
{
  // construct 3 new basis vectors for new unit cell
  MPoint c1, c2, c3;
  c1.x(((double)h)*p_vectors[0].x()+((double)k)*p_vectors[1].x()
      +((double)l)*p_vectors[2].x());
  c1.y(((double)h)*p_vectors[0].y()+((double)k)*p_vectors[1].y()
      +((double)l)*p_vectors[2].y());
  c1.z(((double)h)*p_vectors[0].z()+((double)k)*p_vectors[1].z()
      +((double)l)*p_vectors[2].z());

  c2.x(((double)h1)*p_vectors[0].x()+((double)k1)*p_vectors[1].x()
      +((double)l1)*p_vectors[2].x());
  c2.y(((double)h1)*p_vectors[0].y()+((double)k1)*p_vectors[1].y()
      +((double)l1)*p_vectors[2].y());
  c2.z(((double)h1)*p_vectors[0].z()+((double)k1)*p_vectors[1].z()
      +((double)l1)*p_vectors[2].z());

  c3.x(((double)h2)*p_vectors[0].x()+((double)k2)*p_vectors[1].x()
      +((double)l2)*p_vectors[2].x());
  c3.y(((double)h2)*p_vectors[0].y()+((double)k2)*p_vectors[1].y()
      +((double)l2)*p_vectors[2].y());
  c3.z(((double)h2)*p_vectors[0].z()+((double)k2)*p_vectors[1].z()
      +((double)l2)*p_vectors[2].z());

  p_vectors[0] = c2;
  p_vectors[1] = c3;
  p_vectors[2] = c1;
  double volume =
    p_vectors[0].dotProduct(p_vectors[1].crossProduct1(p_vectors[2]));
  p_reciprocalVectors.clear();
  p_reciprocalVectors.push_back(p_vectors[1].crossProduct1(p_vectors[2]));
  p_reciprocalVectors.push_back(p_vectors[2].crossProduct1(p_vectors[0]));
  p_reciprocalVectors.push_back(p_vectors[0].crossProduct1(p_vectors[1]));
  p_reciprocalVectors[0].scale(1.0/volume);
  p_reciprocalVectors[1].scale(1.0/volume);
  p_reciprocalVectors[2].scale(1.0/volume);
}

/** Method to chose a corner of the cell so that normal vector is pointing
 *  to the interior
 */
MPoint LatticeDef::getPlaneOrigin(MPoint normal)
{
  MPoint ret = getLatticeCorner();
  MPoint incr;
  if (normal.dotProduct(p_vectors[0])<0.0) {
    incr = p_vectors[0];
    incr.scale((double)p_repa1);
    ret.add(incr);
  }
  if (normal.dotProduct(p_vectors[1])<0.0) {
    incr = p_vectors[1];
    incr.scale((double)p_repa2);
    ret.add(incr);
  }
  if (normal.dotProduct(p_vectors[2])<0.0) {
    incr = p_vectors[2];
    incr.scale((double)p_repa3);
    ret.add(incr);
  }
  return ret;
}

/**
 * the center of the box is at (0,0,0).  For a cubic box with a side
 * length of L. this means that the lower lefthand corner of the box
 * is located at (-L/2,-L/2,-L/2) and the the upperrightand corner is at
 * (L/2,L/2,L/2).  This is the convention I use in the cell shift routine I sent you.
 */
void LatticeDef::toMatrix(double a, double b, double c, 
                          double alpha, double beta, double gamma, 
                          double lattice_a[][3]) const
{
#if 0
   const double D2R = 0.0174532925;

   //cout << a << " " << b << " " << c << " " << alpha << " " << beta << " " << gamma << endl;
   // local variables ****
   int i,j,iang;
   double c1,c2,c3,s3;
   double cdist[3],cang[3];
   double gmat[3][3];

   cdist[0] = a;
   cdist[1] = b;
   cdist[2] = c;
   cang[0] = alpha * D2R;
   cang[1] = beta * D2R;
   cang[2] = gamma * D2R;

   // build the metrical matrix (atomic units) ****
   for (i=0; i<3; i++) {
      gmat[i][i] = pow(cdist[i],2);
   }
   iang=2;
   for (i=0; i<3; i++) {
      for (j=i+1; j<3; j++) {
         gmat[i][j] = cdist[i] *cdist[j] * cos(cang[iang]);
         gmat[j][i]=gmat[i][j];
         iang=iang-1;
      }
   }

   // get volume 
   double vol=sqrt(deter3(gmat));

   // generate lattice_a 
   c1 = cos(cang[0]);
   c2=cos(cang[1]);
   c3=cos(cang[2]);
   s3=sin(cang[2]);
   lattice_a[0][0] = cdist[0]*s3;
   lattice_a[0][1] = 0.0;
   lattice_a[0][2] = (cdist[2]*(c2-c1*c3)/s3);
   lattice_a[1][0] = cdist[0]*c3;
   lattice_a[1][1] = cdist[1];
   lattice_a[1][2] = cdist[2]*c1;
   lattice_a[2][0] = 0.0;
   lattice_a[2][1] = 0.0;
   lattice_a[2][2] = (vol/(cdist[0]*cdist[1]*s3));

   /*
   for (i=0; i<3; i++) {
      cout << "col ";
      for (j=0; j<3; j++) {
        cout << lattice_a[j][i] << " " ;
      }
      cout << endl;
   }
   */
#else
   const double D2R = 4.0*atan(1.0)/180.0;

   //cout << a << " " << b << " " << c << " " << alpha << " " << beta << " " << gamma << endl;
   // local variables ****
   double ca,cb,cg,sg,sgstar;
   double cdist[3],cang[3];

   cdist[0] = a;
   cdist[1] = b;
   cdist[2] = c;
   cang[0] = alpha * D2R;
   cang[1] = beta * D2R;
   cang[2] = gamma * D2R;

   // generate lattice_a 
   ca = cos(cang[0]);
   cb=cos(cang[1]);
   cg=cos(cang[2]);
   sg=sin(cang[2]);
   sgstar = sqrt(1.0-ca*ca-cb*cb-cg*cg+2.0*ca*cb*cg);
   lattice_a[0][0] = cdist[0];
   lattice_a[0][1] = cdist[1]*cg;
   lattice_a[0][2] = cdist[2]*cb;

   lattice_a[1][0] = 0.0;
   lattice_a[1][1] = cdist[1]*sg;
   lattice_a[1][2] = cdist[2]*(ca-cb*cg)/sg;

   lattice_a[2][0] = 0.0;
   lattice_a[2][1] = 0.0;
   lattice_a[2][2] = cdist[2]*sgstar/sg;
#endif

}



/**
 * return the determinant of a 3x3 matrix
 */
double LatticeDef::deter3(double r[3][3]) const
{
   return  r[0][0]*(r[1][1]*r[2][2]-r[1][2]*r[2][1]) -
           r[0][1]*(r[1][0]*r[2][2]-r[1][2]*r[2][0]) +
           r[0][2]*(r[1][0]*r[2][1]-r[1][1]*r[2][0]);

}

/**
 * method finds the intersection of a plane, defined by displ and normal,
 * with a line, defined by orig and drctn. The intersect point is
 * defined by intrsct and the parameter tau is between 0 and 1 if
 * the intersection is inside the lattice bounding box. The method
 * returns false if no intersection is found.
 */
bool LatticeDef::getIntersection(MPoint displ, MPoint normal, MPoint orig,
                                 MPoint drctn, MPoint &intrsct, double &tau)
{
  if (normal.dotProduct(drctn) == 0.0) {
    return false;
  } else {
    double nor, ndi, ndr;
    nor = normal.dotProduct(orig);
    ndi = normal.dotProduct(displ);
    ndr = normal.dotProduct(drctn);
    tau = (ndi-nor)/ndr;

    intrsct.xyz(drctn.x(), drctn.y(), drctn.z());
    intrsct.scale(tau);
    intrsct.add(orig);
#ifdef DEBUG
    cout<<"orig x: "<<orig.x()<<" y: "<<orig.y()<<" z: "<<orig.z()<<endl;
    cout<<"drctn x: "<<drctn.x()<<" y: "<<drctn.y()<<" z: "<<drctn.z()<<endl;
    cout<<"intrsct x: "<<intrsct.x()<<" y: "<<intrsct.y()<<" z: "<<intrsct.z()<<endl;
    cout<<"tau: "<<tau<<endl;
#endif

    return true;
  }
}

ostream& operator<<(ostream& os, const LatticeDef& lattice) 
{
  // Header
  double a,b,c,alpha,beta,gamma;
  lattice.getLattice(a,b,c,alpha,beta,gamma);
  os << "[Lattice<";

  os << "a: " << a << ",";
  os << "b: " << b << ",";
  os << "c: " << c << ",";
  os << "alpha: " << alpha << ",";
  os << "beta: " << beta << ",";
  os << "gamma: " << gamma << ",";
  os << "replicate: " << lattice.p_repa1 << "," << lattice.p_repa2 << "," << lattice.p_repa3;

  // Trailer
  os << ">]" << endl;
  return os;
}
