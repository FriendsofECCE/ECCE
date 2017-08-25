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

#include "util/Transformations.H"

#include "viz/RotateAboutLineCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

RotateAboutLineCmd::RotateAboutLineCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
RotateAboutLineCmd::~RotateAboutLineCmd()
{

}



void RotateAboutLineCmd::init()
{
   addParameter(new CommandParameter("x1", 0.0));
   addParameter(new CommandParameter("y1", 0.0));
   addParameter(new CommandParameter("z1", 0.0));
   addParameter(new CommandParameter("x2", 1.0));
   addParameter(new CommandParameter("y2", 0.0));
   addParameter(new CommandParameter("z2", 0.0));
   addParameter(new CommandParameter("angle", 0.0));
   addParameter(new CommandParameter("apply", "all"));
   setHelpMessage("Rotate existing structure about line.");
}



bool RotateAboutLineCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() > 0 ) {
      ret = true;

       
      double x1 = getParameter("x1")->getDouble();
      double y1 = getParameter("y1")->getDouble();
      double z1 = getParameter("z1")->getDouble();
      double x2 = getParameter("x2")->getDouble();
      double y2 = getParameter("y2")->getDouble();
      double z2 = getParameter("z2")->getDouble();
      double angle = getParameter("angle")->getDouble();
      string apply = getParameter("apply")->getString();
      if (apply == "all") {
        p_all = true;
      } else {
        p_all = false;
      }

      p_x1 = x1;
      p_y1 = y1;
      p_z1 = z1;
      p_x2 = x2;
      p_y2 = y2;
      p_z2 = z2;
      p_angle = -angle;
      p_highlight.clear();
      p_highlight = frag->m_atomHighLight;

      // Create transformation matrix;
      MPoint p1(x1,y1,z1);
      MPoint p2(x2,y2,z2);
      MMatrix matrix;
      matrix.rotateLineMatrix(p1,p2,angle);

      int i, nsize = frag->m_atomHighLight.size();
      TAtm* atm;
      if (nsize > 0 && !p_all) {
        for (i=0; i<nsize; i++) {
          atm = frag->atomRef(frag->m_atomHighLight[i]);
          MPoint p3;
          p3.xyz(atm->coordinates());
          MPoint p4 = matrix.transformPoint(p3);
          atm->coordinates(p4.x(),p4.y(),p4.z());
        }
      } else {
        nsize = frag->numAtoms();
        for (i=0; i<nsize; i++) {
          atm = frag->atomRef(i);
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

   return ret;
}



void RotateAboutLineCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   MPoint p1(p_x1,p_y1,p_z1);
   MPoint p2(p_x2,p_y2,p_z2);
   MMatrix matrix;
   matrix.rotateLineMatrix(p1,p2,p_angle);

   if (frag->numAtoms() > 0) {
     frag->m_atomHighLight.clear();
     int i, nsize=p_highlight.size();
     TAtm *atm;
     if (nsize > 0 && !p_all) {
       for (i=0; i<nsize; i++) {
         atm = frag->atomRef(p_highlight[i]);
         MPoint p3;
         p3.xyz(atm->coordinates());
         MPoint p4 = matrix.transformPoint(p3);
         atm->coordinates(p4.x(),p4.y(),p4.z());
       }
     } else {
       nsize = frag->numAtoms();
       for (i=0; i<nsize; i++) {
         atm = frag->atomRef(i);
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
}
