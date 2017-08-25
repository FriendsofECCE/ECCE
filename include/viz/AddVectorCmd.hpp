#ifndef ADDVECTORCMD__H
#define ADDVECTORCMD__H

#include <vector>
  using std::vector;

#include "inv/nodes/SoSwitch.H"

#include "viz/PropCmd.H"

class SGContainer;

/**
 * This is a generic action that will draw any number of vectors to the
 * scene graph.  The vectors can be plotted from the center of mass, the
 * center of charge, or the center of each atom.
 */
class AddVectorCmd : public PropCmd
{
   public:

      /**
       * Used to color vectors based on sign for each set of displacements.
       */
      enum SIGN { POS, NEG, NEUTRAL };

      AddVectorCmd(const string& name, Receiver *, IPropCalculation *);
      virtual ~AddVectorCmd();

      // VCommand methods (probably won't need to override these)
      virtual void init();
      virtual bool execute() throw(EcceException);

      // Virtual methods with default behavior that can be overridden
      virtual void getCenters(vector<double*>& centers);
      virtual double getScaleFactor();
      virtual string getPropertyName();
      virtual SoSwitch* getRootNode();

      // Pure virtual behavior
      virtual bool getDisplacements(vector<double*>& displacements,
            vector<SIGN>& signs) = 0;

   protected:

      // Helper methods for calculating centers
      void getSingleCenter(vector<double*>& centers);
      void getAtomCenters(vector<double*>& centers);

      // Assigned on each execute (easier than passing variables)
      SGContainer* p_currentSGC;
      double p_preferredScaleFactor; // best first guess for optimal scaling
      bool p_newData; // forces using preferred scale factor

};

#endif // ADDVECTORCMD__H
