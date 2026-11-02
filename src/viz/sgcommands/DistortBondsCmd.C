#include <cstdlib>
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "util/StringTokenizer.H"
using namespace ecce;

#include "tdat/BondDistort.H"
#include "tdat/TAtm.H"
#include "viz/DistortBondsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

DistortBondsCmd::DistortBondsCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}

DistortBondsCmd::~DistortBondsCmd()
{
}

void DistortBondsCmd::init()
{
   addParameter(new CommandParameter("centre", 0));
   addParameter(new CommandParameter("dr", ""));
   addParameter(new CommandParameter("list", ""));
   addParameter(new CommandParameter("jt", ""));
   addParameter(new CommandParameter("axis", "longest"));
}

string DistortBondsCmd::getUndoLabel()
{
   return "Distort Bonds";
}

bool DistortBondsCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   int centre = getParameter("centre")->getInteger() - 1;
   if (centre < 0 && frag->m_atomHighLight.size() == 1)
      centre = frag->m_atomHighLight[0];
   if (centre < 0 || centre >= (int)frag->numAtoms()) {
      setErrorMessage("Select one atom, the centre of the bonds to distort.");
      return false;
   }

   vector<int> nb;
   vector<double> dr;
   string all = getParameter("dr")->getString();
   string list = getParameter("list")->getString();
   string jt = getParameter("jt")->getString();
   if (list != "") {
      StringTokenizer tok(list, ",");
      vector<string> items = tok.tokenize(",");
      for (size_t i = 0; i < items.size(); i++) {
         size_t colon = items[i].find(':');
         if (colon == string::npos) {
            setErrorMessage("list wants atom:change pairs, e.g. 3:0.1,5:-0.05");
            return false;
         }
         nb.push_back(atoi(items[i].c_str()) - 1);
         dr.push_back(atof(items[i].c_str() + colon + 1));
      }
   } else {
      nb = BondDistort::neighbours(*frag, centre);
      if (jt != "") {
         if (!BondDistort::jahnTeller(*frag, centre, nb, atof(jt.c_str()),
                                      getParameter("axis")->getString(), dr)) {
            setErrorMessage("The Jahn-Teller preset needs an atom with six "
                            "bonds and an axis x, y, z or longest.");
            return false;
         }
      } else if (all != "") {
         dr = BondDistort::uniform(nb.size(), atof(all.c_str()));
      }
   }
   if (nb.empty() || nb.size() != dr.size()) {
      setErrorMessage("That atom has no bonds to distort.");
      return false;
   }

   cloneFragment(frag);
   if (!BondDistort::apply(*frag, centre, nb, dr)) {
      clearUndoFragment();
      setErrorMessage("A bond could not be distorted (bad atom or zero length).");
      return false;
   }

   frag->touchNumbers();
   sg->touchChemDisplay();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   return true;
}
