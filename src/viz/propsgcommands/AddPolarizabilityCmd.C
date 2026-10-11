#include <algorithm>
#include <cmath>

#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoShapeHints.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoTransform.H"

#include "util/Color.H"
#include "util/Diag3.H"
#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/cvSGContainer.H"
#include "viz/AddPolarizabilityCmd.H"

const double AddPolarizabilityCmd::LONGEST_SEMI_AXIS = 1.5;

AddPolarizabilityCmd::AddPolarizabilityCmd(const string& name,
                         Receiver *receiver, IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
  p_preferredScaleFactor = 0.3;
}

AddPolarizabilityCmd::~AddPolarizabilityCmd()
{
}

bool AddPolarizabilityCmd::getDisplacements(vector<double*>&, vector<SIGN>&)
{
  return false;
}


bool AddPolarizabilityCmd::principalAxes(const PropVector *vec, double ew[3],
                                         double ev[3][3])
{
  if (!vec || (vec->rows() != 6 && vec->rows() != 9)) return false;

  double xx, xy, xz, yy, yz, zz;
  if (vec->rows() == 6) {
    xx = vec->value(0); xy = vec->value(1); xz = vec->value(2);
    yy = vec->value(3); yz = vec->value(4); zz = vec->value(5);
  } else {
    // A code that prints the full tensor may not print it symmetric.
    xx = vec->value(0); yy = vec->value(4); zz = vec->value(8);
    xy = 0.5*(vec->value(1) + vec->value(3));
    xz = 0.5*(vec->value(2) + vec->value(6));
    yz = 0.5*(vec->value(5) + vec->value(7));
  }

  double w[3], v[3][3];
  Diag3(xx, xy, xz, yy, yz, zz).calculate(w, v[0], v[1], v[2]);

  int order[3] = {0, 1, 2};
  std::sort(order, order + 3, [&](int a, int b) { return w[a] > w[b]; });
  for (int i = 0; i < 3; i++) {
    ew[i] = w[order[i]];
    for (int k = 0; k < 3; k++) ev[i][k] = v[order[i]][k];
  }

  // A left-handed set would make SbRotation a reflection.
  double det = ev[0][0]*(ev[1][1]*ev[2][2] - ev[1][2]*ev[2][1])
             - ev[0][1]*(ev[1][0]*ev[2][2] - ev[1][2]*ev[2][0])
             + ev[0][2]*(ev[1][0]*ev[2][1] - ev[1][1]*ev[2][0]);
  if (det < 0.0) for (int k = 0; k < 3; k++) ev[2][k] = -ev[2][k];
  return true;
}


bool AddPolarizabilityCmd::execute()
{
  p_currentSGC = dynamic_cast<SGContainer*>(getReceiver());
  p_newData = getParameter("NewData")->getBoolean();

  SoSwitch *root = getRootNode();
  root->removeAllChildren();

  IPropCalculation *propcalc = getCalculation();
  INTERNALEXCEPTION(propcalc, "No calculation object!");

  double ew[3], ev[3][3];
  if (!principalAxes((PropVector*) propcalc->getProperty(getPropertyName()),
                     ew, ev))
    return false;

  // The largest eigenvalue sets the default; a tensor with no positive
  // eigenvalue (not a polarizability) draws nothing.
  const double emax = ew[0];
  if (emax <= 0.0) return false;
  p_preferredScaleFactor = LONGEST_SEMI_AXIS / emax;
  const double scale = getScaleFactor();

  vector<double*> centers;
  getCenters(centers);

  SoTransform *transf = new SoTransform;
  transf->translation.setValue((float)centers[0][0], (float)centers[0][1],
                               (float)centers[0][2]);
  transf->rotation = SbRotation(SbMatrix(
        (float)ev[0][0], (float)ev[0][1], (float)ev[0][2], 0.f,
        (float)ev[1][0], (float)ev[1][1], (float)ev[1][2], 0.f,
        (float)ev[2][0], (float)ev[2][1], (float)ev[2][2], 0.f,
        0.f, 0.f, 0.f, 1.f));
  // A small positive eigenvalue keeps the scale matrix invertible.
  float s[3];
  for (int i = 0; i < 3; i++) s[i] = (float)(std::max(ew[i], 1e-3*emax)*scale);
  transf->scaleFactor.setValue(s[0], s[1], s[2]);
  for (size_t i = 0; i < centers.size(); i++) delete[] centers[i];

  Color color(getParameter("neutral")->getString());
  SoMaterial *mat = new SoMaterial;
  mat->diffuseColor.setValue(color.red(), color.green(), color.blue());
  mat->transparency = 0.55f;

  // Both faces of a blended closed surface interleave into stripes; draw
  // the near one only, on a finer mesh.
  SoShapeHints *hints = new SoShapeHints;
  hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  hints->shapeType = SoShapeHints::SOLID;
  SoComplexity *cplx = new SoComplexity;
  cplx->value = 0.9f;

  SoSeparator *sep = new SoSeparator;
  sep->addChild(transf);
  sep->addChild(mat);
  sep->addChild(hints);
  sep->addChild(cplx);
  sep->addChild(new SoSphere);
  root->addChild(sep);
  return true;
}
