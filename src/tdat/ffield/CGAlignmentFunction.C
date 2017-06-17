////////////////////////////////////////////////////////////////////////////////
// CGAlignmentFunction Class
//
//
// DESIGN:  
//     This wraps the energy and force functions in the ForceField Class so that
//     they can be used by the CGMinimizer.
//
////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <iostream>
  using std::cout;
  using std::endl;
#include <math.h>

#include "util/ErrMsg.H"

#include "tdat/FFCoords.H"
#include "tdat/ObjectiveFunction.H"
#include "tdat/CGAlignmentFunction.H"

CGAlignmentFunction::CGAlignmentFunction()
{
  p_target = (FFCoordinates*)0;
  p_fragment = (FFCoordinates*)0;
}

CGAlignmentFunction::~CGAlignmentFunction()
{
  if (p_target) {
    delete p_target;
  }
  if (p_fragment) {
    delete p_fragment;
  }
}

double CGAlignmentFunction::getValue(bool& cancel, FFCoordinates *coords) const
       throw (InvalidException)
{
  double value = 0.0;
  try  {
    int i,j,k;
    double rot[3][3];
    double trans[3];
    double zero = 0.0;
    setRotation(coords, rot);
    setTranslation(coords, trans);
    FFCoordinates diff(p_target->rows(),p_target->columns(), &zero);
    diff.zero();
    for (i=0; i<p_fragment->rows(); i++) {
      for (j=0; j<3; j++) {
        for (k=0; k<3; k++) {
          diff.coordinates()[i][j] += rot[j][k]*p_fragment->row(i)[k];
        }
      }
      for (j=0; j<3; j++) {
        diff.coordinates()[i][j] += trans[j] - p_target->row(i)[j];
      }
    }
    for (i=0; i<p_fragment->rows(); i++) {
      for (j=0; j<3; j++) {
        value += diff.row(i)[j]*diff.row(i)[j];
      }
    }
  }
  catch( InvalidException&  ie)  {
    EE_ASSERT((p_target != (FFCoordinates*)0 &&
               p_fragment != (FFCoordinates*)0), EE_FATAL,
               "CGAlignmentFunction: Coordinates have not been set");
  }
  return value;
}

FFCoordinates* CGAlignmentFunction::getGradient(bool& canceled,
                         const FFCoordinates *coords,
                         FFCoordinates *gradient) const
       throw (InvalidException)
{
  try  {
    int i,j,k,l;
    double rot[3][3];
    double trans[3];
    double drot[3][3][3];
    double dtrans[3][3];
    double zero = 0.0;

    setRotation(coords,rot);
    setTranslation(coords,trans);
    setRotationGrad(coords,drot);
    setTranslationGrad(coords,dtrans);
    FFCoordinates diff(p_target->rows(),p_target->columns(),&zero);
    FFCoordinates dg(p_target->rows(),p_target->columns(),&zero);
    diff.zero();
    dg.zero();
    gradient->zero();
    // Evaluate difference vector (this is used repeatedly)
    for (i=0; i<p_fragment->rows(); i++) {
      for (j=0; j<3; j++) {
        for (k=0; k<3; k++) {
          diff.coordinates()[i][j] += rot[j][k]*p_fragment->row(i)[k];
        }
      }
      for (j=0; j<3; j++) {
        diff.coordinates()[i][j] += trans[j] - p_target->row(i)[j];
      }
    }
    // Evaluate gradients with respect to rotation matrix parameters
    for (l=0; l<3; l++) {
      dg.zero();
      for (i=0; i<p_fragment->rows(); i++) {
        for (j=0; j<3; j++) {
          for (k=0; k<3; k++) {
            dg.coordinates()[i][j] += drot[l][j][k]*p_fragment->row(i)[k];
          }
        }
      }
      for (i=0; i<p_fragment->rows(); i++) {
        for (j=0; j<3; j++) {
          gradient->coordinates()[0][l] -= 2.0*diff.row(i)[j]*dg.row(i)[j];
        }
      }
    }
    // Evaluate gradients with respect to translation parameters
    for (l=0; l<3; l++) {
      dg.zero();
      for (i=0; i<p_fragment->rows(); i++) {
        for (j=0; j<3; j++) {
          dg.coordinates()[i][j] += dtrans[l][j];
        }
      }
      for (i=0; i<p_fragment->rows(); i++) {
        for (j=0; j<3; j++) {
          gradient->coordinates()[1][l] -= 2.0*diff.row(i)[j]*dg.row(i)[j];
        }
      }
    }
  }
  catch( InvalidException&  ie)  {
    EE_ASSERT((p_target != (FFCoordinates*)0 &&
               p_fragment != (FFCoordinates*)0), EE_FATAL,
               "CGAlignmentFunction: Coordinates have not been set");
  }
  return gradient;
}

void CGAlignmentFunction::setTargetCoordinates(FFCoordinates *coords)
{
  if (p_target) {
    delete p_target;
  }
  p_target = new FFCoordinates(*coords);
}

void CGAlignmentFunction::setInitialFragmentCoordinates(FFCoordinates *coords)
{
  if (p_fragment) {
    delete p_fragment;
  }
  p_fragment = new FFCoordinates(*coords);
  // Shift fragment so that its center is at the origin
  double cm[3];
  int i;
  cm[0] = 0.0;
  cm[1] = 0.0;
  cm[2] = 0.0;
  int atot = 0;
  for (i=0; i<p_fragment->rows(); i++) {
    cm[0] += p_fragment->coordinates()[i][0];
    cm[1] += p_fragment->coordinates()[i][1];
    cm[2] += p_fragment->coordinates()[i][2];
    atot++;
  }
  if (atot > 0) {
    cm[0] /= (double)atot;
    cm[1] /= (double)atot;
    cm[2] /= (double)atot;
  }
  for (i=0; i<p_fragment->rows(); i++) {
    p_fragment->coordinates()[i][0] -= cm[0];
    p_fragment->coordinates()[i][1] -= cm[1];
    p_fragment->coordinates()[i][2] -= cm[2];
  }
}

void CGAlignmentFunction::getFragmentCoordinates(FFCoordinates *transf,
                          FFCoordinates *coords)
{
  int i,j,k;
  double rot[3][3];
  double trans[3];
  setRotation(transf, rot);
  setTranslation(transf, trans);
  coords->zero();
  for (i=0; i<p_fragment->rows(); i++) {
    for (j=0; j<3; j++) {
      for (k=0; k<3; k++) {
        coords->coordinates()[i][j] += rot[j][k]*p_fragment->row(i)[k];
      }
    }
    for (j=0; j<3; j++) {
      coords->coordinates()[i][j] += trans[j];
    }
  }
}

void CGAlignmentFunction::setRotation(const FFCoordinates *coords,
                          double rot[3][3]) const
{
  double theta = coords->row(0)[0];
  double psi = coords->row(0)[1];
  double phi = coords->row(0)[2];

  rot[0][0] = cos(psi)*cos(phi)-cos(theta)*sin(phi)*sin(psi);
  rot[0][1] = cos(psi)*sin(phi)+cos(theta)*cos(phi)*sin(psi);
  rot[0][2] = sin(psi)*sin(theta);

  rot[1][0] = -sin(psi)*cos(phi)-cos(theta)*sin(phi)*cos(psi);
  rot[1][1] = -sin(psi)*sin(phi)+cos(theta)*cos(phi)*cos(psi);
  rot[1][2] = cos(psi)*sin(theta);

  rot[2][0] = sin(theta)*sin(phi);
  rot[2][1] = -sin(theta)*cos(phi);
  rot[2][2] = cos(theta);
}

void CGAlignmentFunction::setRotationGrad(const FFCoordinates *coords,
                          double drot[3][3][3]) const
{
  double theta = coords->row(0)[0];
  double psi = coords->row(0)[1];
  double phi = coords->row(0)[2];

  drot[0][0][0] = sin(theta)*sin(phi)*sin(psi);
  drot[0][0][1] = -sin(theta)*cos(phi)*sin(psi);
  drot[0][0][2] = sin(psi)*cos(theta);

  drot[0][1][0] = sin(theta)*sin(phi)*cos(psi);
  drot[0][1][1] = -sin(theta)*cos(phi)*cos(psi);
  drot[0][1][2] = cos(psi)*cos(theta);

  drot[0][2][0] = cos(theta)*sin(phi);
  drot[0][2][1] = -cos(theta)*cos(phi);
  drot[0][2][2] = -sin(theta);

  drot[1][0][0] = -sin(psi)*cos(phi)-cos(theta)*sin(phi)*cos(psi);
  drot[1][0][1] = -sin(psi)*sin(phi)+cos(theta)*cos(phi)*cos(psi);
  drot[1][0][2] = cos(psi)*sin(theta);

  drot[1][1][0] = -cos(psi)*cos(phi)+cos(theta)*sin(phi)*sin(psi);
  drot[1][1][1] = -cos(psi)*sin(phi)-cos(theta)*cos(phi)*sin(psi);
  drot[1][1][2] = -sin(psi)*sin(theta);

  drot[1][2][0] = 0.0;
  drot[1][2][1] = 0.0;
  drot[1][2][2] = 0.0;

  drot[2][0][0] = -cos(psi)*sin(phi)-cos(theta)*cos(phi)*sin(psi);
  drot[2][0][1] = cos(psi)*cos(phi)-cos(theta)*sin(phi)*sin(psi);
  drot[2][0][2] = 0.0;

  drot[2][1][0] = sin(psi)*sin(phi)-cos(theta)*cos(phi)*cos(psi);
  drot[2][1][1] = -sin(psi)*cos(phi)-cos(theta)*sin(phi)*cos(psi);
  drot[2][1][2] = 0.0;

  drot[2][2][0] = sin(theta)*cos(phi);
  drot[2][2][1] = sin(theta)*sin(phi);
  drot[2][2][2] = 0.0;

}

void CGAlignmentFunction::setTranslation(const FFCoordinates *coords,
                          double trans[3]) const
{
  trans[0] = coords->row(1)[0];
  trans[1] = coords->row(1)[1];
  trans[2] = coords->row(1)[2];
}

void CGAlignmentFunction::setTranslationGrad(const FFCoordinates *coords,
                          double dtrans[3][3]) const
{
  dtrans[0][0] = 1.0;
  dtrans[0][1] = 0.0;
  dtrans[0][2] = 0.0;

  dtrans[1][0] = 0.0;
  dtrans[1][1] = 1.0;
  dtrans[1][2] = 0.0;

  dtrans[2][0] = 0.0;
  dtrans[2][1] = 0.0;
  dtrans[2][2] = 1.0;
}
