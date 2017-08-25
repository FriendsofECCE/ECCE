/**
 * @file
 *
 *
 */
#ifndef LATTICEDEF_HH
#define LATTICEDEF_HH

#include <iostream>
  using std::ostream;
#include <vector>
  using std::vector;

#include "util/Transformations.H"

/**
 * Class that contains periodic system boundary definitions
 * The lattice can be defined in several ways:
 *    a: length of vector a
 *    b: length of vector b
 *    c: length of vector c
 *    alpha: angle between b,c
 *    beta: angle between a,c
 *    gamma: angle between a,b
 * This can be converted to a 3,3 matrix that represents the vectors a1,a2,a3
 */
class LatticeDef 
{
   public:
      LatticeDef(double a, double b, double c, 
            double alpha, double beta, double gamma);
      LatticeDef(const LatticeDef& rhs);
      ~LatticeDef();

      LatticeDef& assign(double a, double b, double c,
                  double alpha, double beta, double gamma);
      LatticeDef& assign(MPoint a1, MPoint a2, MPoint a3);
      vector<MPoint> *toVectors() const;
      vector<MPoint> *toLines();// const;
      vector<MPoint> *toReciprocalVectors() const;
      MPoint getLatticeCorner();
      MPoint getPlaneNormal(int h, int k, int l);

      void getReplicationFactors(int& x, int& y, int& z) const;
      void setReplicationFactors(int x, int y, int z);

      void setCentering(bool center);
      bool getCentering() const;
      void setFragCenter(double x, double y, double z);
      void getLattice(double& a, double& b, double& c,
                      double& alpha, double& beta, double& gamma) const;
      /* Find intersections of a plane described by displ and normal with
         bounding box */
      void planeCorners(MPoint displ, MPoint normal, int &n,
	                vector<MPoint> &vertices);

      /* Find range of displacement parameter for a given normal and
	 bounding box */
      double displacementRange(MPoint normal);

      /* Find two lattice vectors that are perpendicular to the reciprocal
	 lattice vector defined by h,k,l */
      bool getLatticePlane(int h,  int k,  int l,
	                   int &h1, int &k1, int &l1,
			   int &h2, int &k2, int &l2);

      /* Transform old unit cell into a new unit cell defined by the triplets
	 hkl, h1k1l1, h2k2l2 */
      void transformCell(int h,  int k,  int l,
	                 int h1, int k1, int l1,
			 int h2, int k2, int l2);

      /* Method to chose a corner of the cell so that normal vector is pointing
	 to the interior */
      MPoint getPlaneOrigin(MPoint normal);

   protected:
      void toMatrix(double a, double b, double c,
                    double alpha, double beta, double gamma,
                    double lattice[][3]) const;
      double deter3(double m[][3]) const;

      vector<MPoint> p_vectors;

      vector<MPoint> p_reciprocalVectors;

      MPoint p_fragcenter;

      /* number of times to replicate along first vector */
      int p_repa1;

      /* number of times to replicate along first vector */
      int p_repa2;

      /* number of times to replicate along first vector */
      int p_repa3;

      /* should lattice center be centered over fragment center */
      bool p_center;

      /* method finds the intersection of a plane, defined by displ and normal,
	 with a line, defined by orig and drctn. The intersect point is
	 defined by intrsct and the parameter tau is between 0 and 1 if
	 the intersection is inside the lattice bounding box. The method
	 returns false if no intersection is found. */
      bool getIntersection(MPoint displ, MPoint normal, MPoint orig,
	                   MPoint drctn, MPoint &intrsct, double &tau);

      friend ostream& operator<< (ostream& os, const LatticeDef& lattice);
}; 


#endif
