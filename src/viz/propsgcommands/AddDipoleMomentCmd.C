#include <cmath>
#include <cstdio>
#include <GL/gl.h>

#include "inv/ChemKit/ChemColor.H"
#include "inv/nodes/SoCallback.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoText2.H"
#include "inv/nodes/SoTranslation.H"

#include "util/Color.H"
#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/SGFragment.H"
#include "viz/cvSGContainer.H"
#include "viz/VRVector.H"
#include "viz/AddDipoleMomentCmd.H"

namespace {

// The label is drawn over atoms and bonds, which would hide it.
void depthTestOff(void*, SoAction*) { glDisable(GL_DEPTH_TEST); }
void depthTestOn(void*, SoAction*) { glEnable(GL_DEPTH_TEST); }

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


string AddDipoleMomentCmd::label(double mu)
{
   if (mu < ZERO_MU) return "mu = 0";
   char buf[64];
   snprintf(buf, sizeof buf, "mu = %.2f D", mu);
   return buf;
}


/**
 * The arrow starts at the negative pole and runs along mu as far as the
 * positive pole lies along mu, so it points the way mu does even where the
 * charges do not reproduce mu exactly.  The shape is constant: the label, not the
 * arrow, carries |mu|.
 */
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
         // The pole separation measured along mu: the head then lies on mu's
         // line at the positive pole's height.
         double dist = 0.0;
         for (int k = 0; k < 3; k++) {
            start[k] = neg[k];
            len += (pos[k]-neg[k])*m[k]/mag;
            dist += (pos[k]-neg[k])*(pos[k]-neg[k]);
         }
         if (len <= 0.0) len = sqrt(dist);
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

   Color color(getParameter("neutral")->getString());
   double tip[3] = {0, 0, 0};
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
      for (int k = 0; k < 3; k++) tip[k] = start[k] + m[k]/mag*len;
   } else if (frag) {
      vector<double*> c;
      getSingleCenter(c);
      for (int k = 0; k < 3; k++) tip[k] = c[0][k];
      delete[] c[0];
   }

   SoSeparator *text = new SoSeparator;
   SoMaterial *mat = new SoMaterial;
   // The viewer's text colour, which contrasts with its background.
   ChemColor *cc = sg->getColorNode();
   if (cc && cc->atomLabelColor.getNum() > 0)
      mat->diffuseColor = cc->atomLabelColor[0];
   else
      mat->diffuseColor.setValue(color.red(), color.green(), color.blue());
   SoCallback *off = new SoCallback;
   off->setCallback(depthTestOff);
   text->addChild(off);
   text->addChild(mat);
   SoFont *font = new SoFont;
   font->size = 20;
   text->addChild(font);
   SoTranslation *at = new SoTranslation;
   at->translation.setValue((float)tip[0], (float)tip[1], (float)tip[2]);
   text->addChild(at);
   SoText2 *t = new SoText2;
   t->string.setValue(label(mag).c_str());
   text->addChild(t);
   SoCallback *on = new SoCallback;
   on->setCallback(depthTestOn);
   text->addChild(on);
   root->addChild(text);
   return true;
}


bool AddDipoleMomentCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   // Not used: execute() draws the arrow itself.
   return true;
}
