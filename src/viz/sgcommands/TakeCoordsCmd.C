#include <cstdlib>
#include <sstream>
#include <vector>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/CoordCombine.H"
#include "tdat/TAtm.H"
#include "viz/TakeCoordsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

TakeCoordsCmd::TakeCoordsCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver), p_rmsd(0)
{
   init();
}

TakeCoordsCmd::~TakeCoordsCmd()
{
}

void TakeCoordsCmd::init()
{
   addParameter(new CommandParameter("set", ""));
   addParameter(new CommandParameter("elements", ""));
   addParameter(new CommandParameter("coords", ""));
   addParameter(new CommandParameter("align", false));
}

string TakeCoordsCmd::getUndoLabel()
{
   return "Take Coordinates";
}

bool TakeCoordsCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   const int n = (int)frag->numAtoms();

   vector<string> elA, elB;
   vector<double> A, B;
   for (int i = 0; i < n; i++) {
      const double *c = frag->atomRef(i)->coordinates();
      elA.push_back(frag->atomRef(i)->atomicSymbol());
      for (int k = 0; k < 3; k++) A.push_back(c[k]);
   }
   { istringstream in(getParameter("elements")->getString()); string s;
     while (in >> s) elB.push_back(s); }
   { istringstream in(getParameter("coords")->getString()); double d;
     while (in >> d) B.push_back(d); }

   vector<int> S;
   string err, spec = getParameter("set")->getString();
   if (spec.empty()) {
      S = frag->m_atomHighLight;
      if (S.empty()) {
         setErrorMessage("Select the atoms whose coordinates to take, or "
                         "give an atom list.");
         return false;
      }
   } else if (!CoordCombine::parseSet(spec, n, S, err)) {
      setErrorMessage(err);
      return false;
   }

   CoordCombine::Result r;
   if (!CoordCombine::combine(elA, A, elB, B, S,
                              getParameter("align")->getBoolean(), r)) {
      setErrorMessage(r.error);
      return false;
   }
   p_rmsd = r.rmsd;

   cloneFragment(frag);
   for (size_t s = 0; s < S.size(); s++) {
      int i = S[s];
      frag->atomRef(i)->coordinates(r.xyz[3*i], r.xyz[3*i+1], r.xyz[3*i+2]);
   }
   frag->touchNumbers();
   sg->touchChemDisplay();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   return true;
}
