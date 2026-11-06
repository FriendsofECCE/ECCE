#include <algorithm>
#include <cmath>
#include <cstdio>

#include <GL/gl.h>
#include "inv/nodes/SoCallback.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoTranslation.H"
#ifdef OIV_COIN
#include <Inventor/nodes/SoTransparencyType.h>
#endif

#include "util/Color.H"
#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "tdat/TBond.H"
#include "viz/SGFragment.H"
#include "viz/cvSGContainer.H"
#include "viz/VRVector.H"
#include "viz/AddDipoleMomentCmd.H"

namespace {

// Below this |mu| (Debye) the label would read 0.00 anyway.
const double ZERO_MU = 0.005;

// 1 e Angstrom in debye.
const double DEBYE_PER_E_ANGSTROM = 4.80320;
// Bond-dipole arrows: purple, apart from the yellow molecular arrow and the
// element colours.
const char *BOND_ARROW_COLOR = "#7b2d9b";
// Nearly uncharged atoms would put the poles far outside the molecule.
const double MAX_POLE_SEPARATION = 10.0;

// Centre of the atoms' charges of one sign: sum q r / sum q.
bool pole(const vector<double>& q, const vector<const double*>& xyz,
          bool positive, double c[3])
{
   double sum = 0.0;
   c[0] = c[1] = c[2] = 0.0;
   for (size_t i = 0; i < q.size(); i++) {
      if ((positive && q[i] > 0.0) || (!positive && q[i] < 0.0)) {
         sum += q[i];
         for (int k = 0; k < 3; k++) c[k] += q[i]*xyz[i][k];
      }
   }
   if (sum == 0.0) return false;
   for (int k = 0; k < 3; k++) c[k] /= sum;
   return true;
}

}


AddDipoleMomentCmd::AddDipoleMomentCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
   addParameter(new CommandParameter("ChargesOppose", false));
   addParameter(new CommandParameter("PoleCharge", 0.0));
   addParameter(new CommandParameter("PoleSeparation", 0.0));
   addParameter(new CommandParameter("BondCharges", ""));
}

AddDipoleMomentCmd::~AddDipoleMomentCmd()
{
}


string AddDipoleMomentCmd::chargeKey(IPropCalculation *calc)
{
   static const char *keys[] = { "ESPFIT", "MKFIT", "MULLIKEN" };
   for (int i = 0; calc && i < 3; i++) {
      if (calc->getProperty(keys[i])) return keys[i];
   }
   return "";
}


string AddDipoleMomentCmd::chargeName(const string& key)
{
   if (key == "ESPFIT") return "CHELPG (ESP-fitted)";
   if (key == "MKFIT") return "Merz-Kollman (ESP-fitted)";
   if (key == "MULLIKEN") return "Mulliken";
   return "";
}


vector<string> AddDipoleMomentCmd::bondChargeKeys(IPropCalculation *calc)
{
   static const char *keys[] = { "MULLIKEN", "LOEWDIN", "HIRSHFELD", "ESPFIT",
                                 "MKFIT" };
   vector<string> out;
   for (int i = 0; calc && i < 5; i++)
      if (calc->getProperty(keys[i])) out.push_back(keys[i]);
   return out;
}


string AddDipoleMomentCmd::bondChargeName(const string& key)
{
   if (key == "LOEWDIN") return "L\u00f6wdin";
   if (key == "HIRSHFELD") return "Hirshfeld";
   return chargeName(key);
}


bool AddDipoleMomentCmd::bondDipoles(IPropCalculation *calc, SGFragment *frag,
                                     const string& key,
                                     vector<std::pair<int,int> >& bonds,
                                     BondDipoles::Result& result)
{
   PropVector *qv = key.empty() || !calc ? 0 :
      (PropVector*) calc->getProperty(key);
   if (!qv || !frag) return false;
   int n = std::min((int)frag->numAtoms(), (int)qv->rows());
   if (n < 2) return false;
   vector<double> q(n), xyz(3*n);
   for (int i = 0; i < n; i++) {
      q[i] = qv->value(i);
      const double *c = frag->atomRef(i)->coordinates();
      for (int k = 0; k < 3; k++) xyz[3*i + k] = c[k];
   }
   vector<std::pair<int,int> > graph;
   vector<TBond*> *tb = frag->bonds();
   for (size_t b = 0; tb && b < tb->size(); b++) {
      int i = (*tb)[b]->atom1()->index(), j = (*tb)[b]->atom2()->index();
      if (i >= 0 && j >= 0 && i < n && j < n && i != j)
         graph.push_back(std::make_pair(i, j));
   }
   delete tb;
   if (graph.empty()) return false;
   double com[3];
   frag->centerOfMass(com[0], com[1], com[2]);
   result = BondDipoles::compute(q, xyz, graph, com);
   bonds.clear();
   for (size_t b = 0; b < graph.size(); b++) {
      // delta > 0: atom i is the positive end.
      if (result.delta[b] >= 0.0)
         bonds.push_back(std::make_pair(graph[b].second, graph[b].first));
      else bonds.push_back(graph[b]);
   }
   return true;
}


string AddDipoleMomentCmd::label(double mu, const string& key, bool oppose)
{
   if (mu < ZERO_MU) return "Dipole moment \u03bc = 0 D";
   char buf[64];
   snprintf(buf, sizeof buf, "Dipole moment \u03bc = %.2f D", mu);
   string r = buf;
   const string against = oppose ? " point the other way" : "";
   if (key == "ESPFIT") r += " (ESP charges" + against + ")";
   else if (key == "MKFIT") r += " (MK charges" + against + ")";
   else if (key == "MULLIKEN") r += " (Mulliken charges" + against + ")";
   return r;
}


/**
 * The arrow starts at the negative pole and runs along mu as far as the
 * positive pole lies along mu, so it points the way mu does even where the
 * charges do not reproduce mu exactly.  Where the charges point against mu
 * (CO: atomic dipoles outweigh the charge transfer) it runs between the same
 * poles from the positive one, so both ends stay on the molecule.  The shape is constant: the corner text, not the
 * arrow, carries |mu|.
 */
namespace {
void depthOff(void*, SoAction*) { glDisable(GL_DEPTH_TEST); }
void depthOn(void*, SoAction*) { glEnable(GL_DEPTH_TEST); }

//  A small maroon sphere at a pole, drawn twice like the arrow: a pole often
//  sits on a nucleus (ammonia's N) and would otherwise be hidden by the atom.
SoSeparator *poleSphere(const double at[3])
{
   SoSeparator *sep = new SoSeparator;
   SoTranslation *t = new SoTranslation;
   t->translation.setValue(at[0], at[1], at[2]);
   SoSphere *ball = new SoSphere;
   ball->radius = 0.08f;
   SoMaterial *mat = new SoMaterial;
   mat->diffuseColor.setValue(0.5f, 0.0f, 0.0f);
   SoSeparator *solid = new SoSeparator;
   solid->addChild(mat);
   solid->addChild(t);
   solid->addChild(ball);
   sep->addChild(solid);

   SoSeparator *pass = new SoSeparator;
   SoCallback *off = new SoCallback;
   off->setCallback(depthOff);
   pass->addChild(off);
#ifdef OIV_COIN
   SoTransparencyType *blend = new SoTransparencyType;
   blend->value = SoTransparencyType::BLEND;
   pass->addChild(blend);
#endif
   SoMaterial *through = new SoMaterial;
   through->diffuseColor.setValue(0.5f, 0.0f, 0.0f);
   through->transparency.setValue(0.6f);
   pass->addChild(through);
   pass->addChild(t);
   pass->addChild(ball);
   SoCallback *on = new SoCallback;
   on->setCallback(depthOn);
   pass->addChild(on);
   sep->addChild(pass);
   return sep;
}
}

bool AddDipoleMomentCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   p_currentSGC = sg;

   SoSwitch *root = getRootNode();
   root->removeAllChildren();

   IPropCalculation *calc = getCalculation();
   INTERNALEXCEPTION(calc, "No calculation object!");
   PropVector *mu = (PropVector*) calc->getProperty(getPropertyName());
   if (!mu || mu->rows() < 3) return true;

   double m[3] = { mu->value(0), mu->value(1), mu->value(2) };
   double mag = sqrt(m[0]*m[0] + m[1]*m[1] + m[2]*m[2]);

   double start[3] = {0, 0, 0}, len = 0.0;
   bool arrow = mag >= ZERO_MU;
   bool oppose = false;
   bool havePoles = false;
   double poleCharge = 0.0;
   SGFragment *frag = sg->getFragment();

   if (arrow && frag) {
      string key = chargeKey(calc);
      PropVector *qv = key.empty() ? 0 : (PropVector*) calc->getProperty(key);
      double neg[3], pos[3];
      vector<double> q;
      vector<const double*> xyz;
      int n = frag->numAtoms();
      for (int i = 0; qv && i < n && i < (int)qv->rows(); i++) {
         q.push_back(qv->value(i));
         xyz.push_back(frag->atomRef(i)->coordinates());
      }
      if (pole(q, xyz, false, neg) && pole(q, xyz, true, pos)) {
         // The charges give how much charge sits at each pole; the poles are
         // put along mu, centred between the charge centres, as far apart as
         // that charge must be to give mu (charges alone can point elsewhere,
         // or the other way, as in CO).
         havePoles = true;
         double qpos = 0.0, along = 0.0;
         for (size_t i = 0; i < q.size(); i++) if (q[i] > 0.0) qpos += q[i];
         for (int k = 0; k < 3; k++) along += (pos[k]-neg[k])*m[k]/mag;
         oppose = along < 0.0;
         len = qpos > 0.0 ? mag/DEBYE_PER_E_ANGSTROM/qpos : 0.0;
         if (len > MAX_POLE_SEPARATION) len = MAX_POLE_SEPARATION;
         poleCharge = qpos;
         for (int k = 0; k < 3; k++)
            start[k] = 0.5*(neg[k]+pos[k]) - 0.5*len*m[k]/mag;
      } else {
         // No charges: a 1 A arrow centred on the centre of nuclear charge.
         vector<double*> c;
         getSingleCenter(c);
         len = 1.0;
         for (int k = 0; k < 3; k++) start[k] = c[0][k] - 0.5*m[k]/mag;
         delete[] c[0];
      }
      if (len <= 0.0) arrow = false;
   }
   getParameter("ChargesOppose")->setBoolean(oppose);
   getParameter("PoleCharge")->setDouble(havePoles ? poleCharge : 0.0);
   getParameter("PoleSeparation")->setDouble(havePoles ? len : 0.0);

   Color color(getParameter("neutral")->getString());
   if (arrow) {
      VRVector *vec = new VRVector;
      vec->setColor(color.red(), color.green(), color.blue());
      vec->position(start[0], start[1], start[2]);
      // VRVector puts its tip at 1.075 |direction|.
      double s = len/mag/1.075;
      vec->direction(m[0]*s, m[1]*s, m[2]*s);
      vec->fixedThickness(0.04f, 0.10f, 0.20f);
      vec->showThroughMolecule();
      root->addChild(vec);
      if (havePoles) {
         double head[3];
         for (int k = 0; k < 3; k++) head[k] = start[k] + len*m[k]/mag;
         root->addChild(poleSphere(start));
         root->addChild(poleSphere(head));
      }
   }

   const string bondKey = getParameter("BondCharges")->getString();
   vector<std::pair<int,int> > bonds;
   BondDipoles::Result bd;
   if (frag && !bondKey.empty() &&
       bondDipoles(calc, frag, bondKey, bonds, bd)) {
      Color bcol(BOND_ARROW_COLOR);
      // One scale for all bonds: the largest bond dipole fills its bond
      // (the bond less 0.25 A of atom at each end), the others follow
      // |mu| and are clamped to their own bond; a floor keeps small ones
      // visible unless the table shows 0.00 D.
      double rad = sg->displayedBondRadius(bonds.empty() ? 0 : bonds[0].first);
      if (rad <= 0.0) rad = 0.1;
      const size_t nb = bonds.size();
      vector<double> mag(nb), room(nb);
      size_t big = 0;
      for (size_t b = 0; b < nb; b++) {
         mag[b] = sqrt(bd.mu[0][b]*bd.mu[0][b] + bd.mu[1][b]*bd.mu[1][b] +
                       bd.mu[2][b]*bd.mu[2][b]) * DEBYE_PER_E_ANGSTROM;
         const double *a = frag->atomRef(bonds[b].first)->coordinates();
         const double *z = frag->atomRef(bonds[b].second)->coordinates();
         double len = sqrt((z[0]-a[0])*(z[0]-a[0]) + (z[1]-a[1])*(z[1]-a[1]) +
                           (z[2]-a[2])*(z[2]-a[2]));
         // The visible gap between the two drawn atom spheres, with a margin.
         room[b] = std::max(0.15, len - sg->displayedSphereRadius(bonds[b].first)
                               - sg->displayedSphereRadius(bonds[b].second) - 0.1);
         if (mag[b] > mag[big]) big = b;
      }
      const double scale = mag.empty() || mag[big] <= 0 ? 0 : room[big]/mag[big];
      for (size_t b = 0; b < nb; b++) {
         if (mag[b] < 0.005) continue;
         const double *a = frag->atomRef(bonds[b].first)->coordinates();
         const double *z = frag->atomRef(bonds[b].second)->coordinates();
         double L = std::min(room[b], std::max(0.3, mag[b]*scale));
         double d[3], len = 0.0;
         for (int k = 0; k < 3; k++) { d[k] = z[k]-a[k]; len += d[k]*d[k]; }
         len = sqrt(len);
         VRVector *vec = new VRVector;
         vec->setColor(bcol.red(), bcol.green(), bcol.blue());
         // Centred in the gap between the atom spheres.
         double r1 = sg->displayedSphereRadius(bonds[b].first);
         double f = (r1 + 0.05 + 0.5*(room[b] - L))/len;
         vec->position(a[0] + f*d[0], a[1] + f*d[1], a[2] + f*d[2]);
         double s2 = L/len/1.075;
         vec->direction(d[0]*s2, d[1]*s2, d[2]*s2);
         // Opaque and a little wider than the bond, so the arrow sheathes it.
         vec->fixedThickness((float)(1.3*rad), (float)std::min(2.0*rad, 0.6*L),
                             (float)std::min(0.20, 0.4*L));
         root->addChild(vec);
      }
   }

   return true;
}


bool AddDipoleMomentCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   // Not used: execute() draws the arrow itself.
   return true;
}
