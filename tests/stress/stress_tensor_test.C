#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "util/StressTensor.H"

static void check(bool ok) {
  if (!ok) throw std::runtime_error("stress tensor check failed");
}
static bool near(double a, double b) { return std::fabs(a-b)<1e-8; }

int main() {
  // Display [[2,1,0],[1,2,0],[0,0,-2]] GPa has eigenvalues 3,1,-2,
  // along (1,1,0), (1,-1,0), (0,0,1), respectively.
  double stored[]={-20,-10,0,-10,-20,0,0,0,20};
  StressTensor::Result r;
  check(StressTensor::analyse(stored,r));
  check(near(r.principal[0],3) && near(r.principal[1],1) && near(r.principal[2],-2));
  const double expected[3][3]={{1/std::sqrt(2.),1/std::sqrt(2.),0},
      {1/std::sqrt(2.),-1/std::sqrt(2.),0},{0,0,1}};
  for (int i=0;i<3;++i) {
    double dot=0;
    for (int j=0;j<3;++j) {
      dot+=r.axis[i][j]*expected[i][j];
      double product=0;
      for (int k=0;k<3;++k) product+=r.sigma[j][k]*r.axis[i][k];
      check(near(product,r.principal[i]*r.axis[i][j]));
    }
    check(near(std::fabs(dot),1));
  }
  check(near(r.pressure,-2./3.) && near(r.maxShear,2.5));
  check(near(r.vonMises,std::sqrt(19.)));
  double cell[3][3]={{2,2,0},{0,0,5},{3,-3,0}};
  check(near(StressTensor::normal(r,cell[0]),3));
  check(StressTensor::cellWords(r,cell)==
        "The cell wants to shrink along a and c, expand along b.");
  double hydro[]={100,0,0,0,100,0,0,0,100};
  check(StressTensor::analyse(hydro,r));
  check(near(r.pressure,10) && near(r.vonMises,0) && near(r.maxShear,0));
  for (int i=0;i<3;++i) check(near(r.principal[i],-10));
  double zero[9]={};
  check(StressTensor::analyse(zero,r) && near(r.vonMises,0));
  stored[1]=-12; stored[3]=-8;
  check(StressTensor::analyse(stored,r) && near(r.principal[0],3));
  stored[0]=std::numeric_limits<double>::quiet_NaN();
  check(!StressTensor::analyse(stored,r));
  std::cout << "PASS hand-computed principal values/directions, pressure, shear, cell frame, hydrostatic, zero, asymmetric, invalid\n";
}
