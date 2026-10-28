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

#include "viz/SGFragment.H"
#include "viz/cvSGContainer.H"
#include "viz/VRVector.H"
#include "viz/AddDipoleMomentCmd.H"

namespace {

// Below this |mu| (Debye) the label would read 0.00 anyway.
const double ZERO_MU = 0.005;

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
         havePoles = true;
         // The pole separation measured along mu: the head then lies on mu's
         // line at the positive pole's height.
         double dist = 0.0;
         for (int k = 0; k < 3; k++) {
            start[k] = neg[k];
            len += (pos[k]-neg[k])*m[k]/mag;
            dist += (pos[k]-neg[k])*(pos[k]-neg[k]);
         }
         if (len < 0.0) {
            oppose = true;
            len = -len;
            for (int k = 0; k < 3; k++) start[k] = pos[k];
         } else if (len == 0.0) {
            len = sqrt(dist);
         }
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
      //  The poles are drawn at the arrow's ends: point charges miss the
      //  atoms' own polarisation, so their centres need not lie on mu's line.
      if (havePoles) {
         double head[3];
         for (int k = 0; k < 3; k++) head[k] = start[k] + len*m[k]/mag;
         root->addChild(poleSphere(start));
         root->addChild(poleSphere(head));
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
