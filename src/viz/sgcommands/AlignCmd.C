/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "viz/AlignCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

AlignCmd::AlignCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
AlignCmd::~AlignCmd()
{

}



void AlignCmd::init()
{
   addParameter(new CommandParameter("vect_x", 0.0));
   addParameter(new CommandParameter("vect_y", 0.0));
   addParameter(new CommandParameter("vect_z", 0.0));
   addParameter(new CommandParameter("apply", "all"));
   addParameter(new CommandParameter("toPlane", false));
   setHelpMessage("Align selected atom.");
}



bool AlignCmd::execute() throw(EcceException)
{
   bool ret = false;
   setErrorMessage("");

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag==(SGFragment*)0 || frag->numAtoms()==0) {
     return ret;
   }

   p_highlight.clear();
   p_highlight = frag->m_atomHighLight;

   if (p_highlight.size() == 1) {

      double x1, y1, z1, x2, y2, z2, x3, y3, z3;
      double pi = 4.0*atan(1.0);

      TAtm* atm = frag->atomRef(frag->m_atomHighLight[0]);

      // return if selected atom is a ghost
      if (atm->atomicSymbol() == "X") {
        setErrorMessage("Cannot perform alignment for a ghost atom.");
        return ret;
      }

      bool toPlane = getParameter("toPlane")->getBoolean();
      x2 = getParameter("vect_x")->getDouble();
      y2 = getParameter("vect_y")->getDouble();
      z2 = getParameter("vect_z")->getDouble();

      const double* coords = atm->coordinates();
      x1 = coords[0];
      y1 = coords[1];
      z1 = coords[2];

      // return if action not well defined
      if ((x1*x1+y1*y1+z1*z1) == 0.0) {
        setErrorMessage("Cannot perform alignment for atom at origin.");
        return ret;
      }

      ret = true;

      x3 = y1*z2 - y2*z1;
      y3 = z1*x2 - z2*x1;
      z3 = x1*y2 - x2*y1;
      string apply = getParameter("apply")->getString();
      if (apply == "all") {
        p_all = true;
      } else {
        p_all = false;
      }

      double cos12 = x1*x2+y1*y2+z1*z2;
      cos12 /= sqrt(x1*x1+y1*y1+z1*z1);
      cos12 /= sqrt(x2*x2+y2*y2+z2*z2);

      // handle degenerate case that points 1 and 2 are
      // colinear
      if (fabs(fabs(cos12)-1.0) < 1.0e-10) {
        if (x1 != 0.0 || y1 != 0.0) {
          x3 = 0.0;
          y3 = 0.0;
          z3 = 1.0;
        }  else {
          x3 = 1.0;
          y3 = 0.0;
          z3 = 0.0;
        }
        double cos13 = x1*x3+y1*y3+z1*z3;
        cos13 /= sqrt(x1*x1+y1*y1+z1*z1);
        x3 = x3 - x1*cos13;
        y3 = y3 - y1*cos13;
        z3 = z3 - z1*cos13;
      }

      double angle = acos(cos12);
      if (!toPlane) {
        if (angle > pi/2.0) {
          angle = -(pi - angle);
        }
      } else {
        if (angle < pi/2.0) {
          angle =-(pi/2.0-angle);
        } else {
          angle = angle - pi/2.0;
        }
      }
      angle = angle*180.0/pi;

      p_x3 = x3;
      p_y3 = y3;
      p_z3 = z3;
      p_angle = angle;

      x1 = 0.0;
      y1 = 0.0;
      z1 = 0.0;

      // Create transformation matrix;
      MPoint p1(x1,y1,z1);
      MPoint p2(x3,y3,z3);
      MMatrix matrix;
      matrix.rotateLineMatrix(p1,p2,angle);

      int i, nsize = frag->numAtoms();
      if (p_all) {
        for (i=0; i<nsize; i++) {
          atm = frag->atomRef(i);
          MPoint p3;
          p3.xyz(atm->coordinates());
          MPoint p4 = matrix.transformPoint(p3);
          atm->coordinates(p4.x(),p4.y(),p4.z());
        }
      } else {
        nsize = p_highlight.size();
        for (i=0; i<nsize; i++) {
          atm = frag->atomRef(p_highlight[i]);
          MPoint p3;
          p3.xyz(atm->coordinates());
          MPoint p4 = matrix.transformPoint(p3);
          atm->coordinates(p4.x(),p4.y(),p4.z());
        }
      }

      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   } else {
     setErrorMessage("Must select a single atom to align.");
     return ret;
   }

   return ret;
}



void AlignCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() > 0 ) {

      double x1 = 0.0;
      double y1 = 0.0;
      double z1 = 0.0;
      double angle = -p_angle;

      // Create transformation matrix;
      MPoint p1(x1,y1,z1);
      MPoint p2(p_x3,p_y3,p_z3);
      MMatrix matrix;
      matrix.rotateLineMatrix(p1,p2,angle);

      int i, nsize = frag->numAtoms();
      TAtm *atm;
      if (p_highlight.size() == 0 || p_all) {
        for (i=0; i<nsize; i++) {
          atm = frag->atomRef(i);
          MPoint p3;
          p3.xyz(atm->coordinates());
          MPoint p4 = matrix.transformPoint(p3);
          atm->coordinates(p4.x(),p4.y(),p4.z());
        }
      } else {
        nsize = p_highlight.size();
        for (i=0; i<nsize; i++) {
          atm = frag->atomRef(p_highlight[i]);
          MPoint p3;
          p3.xyz(atm->coordinates());
          MPoint p4 = matrix.transformPoint(p3);
          atm->coordinates(p4.x(),p4.y(),p4.z());
        }
      }

      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   }

   return;
}
