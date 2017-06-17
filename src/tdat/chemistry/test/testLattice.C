#include <iostream.h>
#include <strstream.h>
#include <unistd.h>

#include "tdat/LatticeDef.H"

int main(int argc, char** argv)
{

  double a,b,c,alpha,beta,gamma;
  a = 9.4007;
  b= 9.5022;
  c = 4.2196;
  alpha = 90;
  beta = 90;
  gamma = 120;

  // Numbers from the 8_0.car file
  LatticeDef lattice(a,b,c,alpha,beta,gamma);
  cout << "starting: " << lattice << endl;

  // Generate the lattice vectors
  vector<MPoint> *vecs = lattice.toVectors();
  cout << "a1 " << (*vecs)[0].x() << " " << (*vecs)[0].y() << " " << (*vecs)[0].z() << endl;
  cout << "a2 " << (*vecs)[1].x() << " " << (*vecs)[1].y() << " " << (*vecs)[1].z() << endl;
  cout << "a3 " << (*vecs)[2].x() << " " << (*vecs)[2].y() << " " << (*vecs)[2].z() << endl;

  // Modify  the a2 x value
  (*vecs)[1].x(2.);

  // assign to lattice which causes new a,b,c,alpha,beta,gamma to be generated
  lattice.assign((*vecs)[0], (*vecs)[1], (*vecs)[2]);
  delete vecs;

  cout << "ending: " << lattice << endl;

  // Now turn those back into the vectors and see that they don't match what 
  // we sent into the assign call
  vecs = lattice.toVectors();
  cout << "a1 " << (*vecs)[0].x() << " " << (*vecs)[0].y() << " " << (*vecs)[0].z() << endl;
  cout << "a2 " << (*vecs)[1].x() << " " << (*vecs)[1].y() << " " << (*vecs)[1].z() << endl;
  cout << "a3 " << (*vecs)[2].x() << " " << (*vecs)[2].y() << " " << (*vecs)[2].z() << endl;
  delete vecs;



  return 0;
}
