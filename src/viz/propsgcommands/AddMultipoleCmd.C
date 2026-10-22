#include <algorithm>
#include <cmath>
#include <cctype>

#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoIndexedFaceSet.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoMaterialBinding.H"
#include "inv/nodes/SoNormal.H"
#include "inv/nodes/SoNormalBinding.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoShapeHints.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoTransform.H"

#include "util/Color.H"
#include "util/InternalException.H"
#include "util/Multipole.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/SGFragment.H"
#include "viz/cvSGContainer.H"
#include "viz/AddMultipoleCmd.H"

const char *AddMultipoleCmd::POSITIVE_COLOR = "#0072B2";
const char *AddMultipoleCmd::NEGATIVE_COLOR = "#D55E00";

namespace {
  // Icosphere level 5: 10242 vertices, smooth lobes and sign boundaries.
  const int MESH_LEVEL = 5;
  // Default size of the largest lobe against the molecule's radius.
  const double LOBE_PER_RADIUS = 1.5;
}

AddMultipoleCmd::AddMultipoleCmd(const string& name, Receiver *receiver,
                                 IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
  addParameter(new CommandParameter("Code", ""));
  getParameter("positive")->setString(POSITIVE_COLOR);
  getParameter("negative")->setString(NEGATIVE_COLOR);
  p_preferredScaleFactor = 1.0;
}

AddMultipoleCmd::~AddMultipoleCmd()
{
}

bool AddMultipoleCmd::getDisplacements(vector<double*>&, vector<SIGN>&)
{
  return false;
}

int AddMultipoleCmd::rankOf(const string& p)
{
  return p == "QUADPOLE" ? 2 : p == "OCTAPOLE" ? 3 : p == "HEXPOLE" ? 4 : 0;
}

bool AddMultipoleCmd::tensor(const PropVector *vec, int rank,
                             const string& code, vector<double>& full)
{
  const int want = rank == 2 ? 6 : rank == 3 ? 10 : rank == 4 ? 15 : -1;
  if (!vec || (int)vec->rows() != want) return false;

  vector<string> labels;
  vector<double> values;
  const vector<string> *rl = vec->rowLabels();
  for (int i = 0; i < want; i++) {
    values.push_back(vec->value(i));
    if (rl && (int)rl->size() == want) labels.push_back((*rl)[i]);
  }
  if (labels.empty()) {
    // Only the quadrupole is stored in a fixed order without labels.
    if (rank != 2) return false;
    const char *q[6] = {"XX", "XY", "XZ", "YY", "YZ", "ZZ"};
    for (int i = 0; i < 6; i++) labels.push_back(q[i]);
  }

  string up = code;
  for (size_t i = 0; i < up.size(); i++) up[i] = (char)toupper((unsigned char)up[i]);
  if (up.find("ORCA") != string::npos) {
    // e a0^k -> D Angstrom^(k-1)
    const double f = 2.541746*pow(0.529177, rank - 1);
    for (size_t i = 0; i < values.size(); i++) values[i] *= f;
  }

  vector<double> raw;
  if (!Multipole::fromComponents(rank, labels, values, raw)) return false;
  full = Multipole::harmonicPart(rank, raw);
  return true;
}

bool AddMultipoleCmd::execute()
{
  p_currentSGC = dynamic_cast<SGContainer*>(getReceiver());
  p_newData = getParameter("NewData")->getBoolean();

  SoSwitch *root = getRootNode();
  root->removeAllChildren();

  IPropCalculation *propcalc = getCalculation();
  INTERNALEXCEPTION(propcalc, "No calculation object!");

  const int rank = rankOf(getPropertyName());
  vector<double> full;
  if (!tensor((PropVector*) propcalc->getProperty(getPropertyName()), rank,
              getParameter("Code")->getString(), full))
    return false;
  const double fmax = Multipole::maxAbs(rank, full);
  if (fmax <= 0.0) return false;

  vector<double*> centers;
  getCenters(centers);

  if (p_newData) {
    // The largest lobe reaches LOBE_PER_RADIUS times the farthest atom.
    double radius = 1.0;
    SGFragment *frag = p_currentSGC->getFragment();
    for (int i = 0; frag && i < frag->numAtoms(); i++) {
      const double *c = frag->atomRef(i)->coordinates();
      double d = sqrt(pow(c[0] - centers[0][0], 2) + pow(c[1] - centers[0][1], 2)
                      + pow(c[2] - centers[0][2], 2));
      radius = std::max(radius, d);
    }
    p_preferredScaleFactor = LOBE_PER_RADIUS*radius/fmax;
  }
  const double scale = getScaleFactor();

  Multipole::Mesh mesh = Multipole::surface(rank, full, scale, MESH_LEVEL);

  SoTransform *transf = new SoTransform;
  transf->translation.setValue((float)centers[0][0], (float)centers[0][1],
                               (float)centers[0][2]);
  for (size_t i = 0; i < centers.size(); i++) delete[] centers[i];

  Color pos(getParameter("positive")->getString());
  Color neg(getParameter("negative")->getString());
  const size_t nv = mesh.f.size();
  SoMaterial *mat = new SoMaterial;
  mat->diffuseColor.setNum((int)nv);
  SbColor *col = mat->diffuseColor.startEditing();
  for (size_t i = 0; i < nv; i++) {
    const Color& c = mesh.f[i] >= 0.f ? pos : neg;
    col[i].setValue(c.red(), c.green(), c.blue());
  }
  mat->diffuseColor.finishEditing();
  mat->transparency = 0.4f;

  SoCoordinate3 *coords = new SoCoordinate3;
  coords->point.setNum((int)nv);
  SbVec3f *pt = coords->point.startEditing();
  for (size_t i = 0; i < nv; i++)
    pt[i].setValue(mesh.xyz[3*i], mesh.xyz[3*i + 1], mesh.xyz[3*i + 2]);
  coords->point.finishEditing();

  SoNormal *normals = new SoNormal;
  normals->vector.setNum((int)nv);
  SbVec3f *nm = normals->vector.startEditing();
  for (size_t i = 0; i < nv; i++)
    nm[i].setValue(mesh.normal[3*i], mesh.normal[3*i + 1], mesh.normal[3*i + 2]);
  normals->vector.finishEditing();

  SoIndexedFaceSet *faces = new SoIndexedFaceSet;
  const int nt = (int)mesh.tri.size()/3;
  faces->coordIndex.setNum(4*nt);
  faces->materialIndex.setNum(4*nt);
  faces->normalIndex.setNum(4*nt);
  int32_t *ci = faces->coordIndex.startEditing();
  int32_t *mi = faces->materialIndex.startEditing();
  int32_t *ni = faces->normalIndex.startEditing();
  for (int t = 0; t < nt; t++)
    for (int k = 0; k < 4; k++) {
      int v = k < 3 ? mesh.tri[3*t + k] : -1;
      ci[4*t + k] = mi[4*t + k] = ni[4*t + k] = v;
    }
  faces->coordIndex.finishEditing();
  faces->materialIndex.finishEditing();
  faces->normalIndex.finishEditing();

  SoMaterialBinding *mb = new SoMaterialBinding;
  mb->value = SoMaterialBinding::PER_VERTEX_INDEXED;
  SoNormalBinding *nb = new SoNormalBinding;
  nb->value = SoNormalBinding::PER_VERTEX_INDEXED;

  // Closed surface: draw the near face only, so a blended surface does not
  // interleave its two sides.
  SoShapeHints *hints = new SoShapeHints;
  hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  hints->shapeType = SoShapeHints::SOLID;

  SoSeparator *sep = new SoSeparator;
  sep->addChild(transf);
  sep->addChild(hints);
  sep->addChild(mat);
  sep->addChild(mb);
  sep->addChild(coords);
  sep->addChild(normals);
  sep->addChild(nb);
  sep->addChild(faces);
  root->addChild(sep);
  return true;
}
