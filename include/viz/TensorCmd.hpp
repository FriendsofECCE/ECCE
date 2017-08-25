#ifndef TENSORCMD__H
#define TENSORCMD__H

#include <vector>
  using std::vector;


#include "viz/AddVectorCmd.H"

class SGContainer;

/**
 * This command provides vector data for properties such as
 *    SHIELDTENSOR, DIPOLETENSOR, EFIELDGRAD
 */
class TensorCmd : public AddVectorCmd
{
   public:

      TensorCmd(const string& name, Receiver *, IPropCalculation *);
      virtual ~TensorCmd();


      /* Override to handle the case of displaying no vectors. */
      virtual bool execute() throw(EcceException);

      // CmdAddVector methods
      virtual bool getDisplacements(vector<double*>& displacements,
            vector<SIGN>& signs);
      virtual void getCenters(vector<double*>& centers);
      virtual double getScaleFactor(); // takes into account log scale

   protected:
      double addTensor(double xx, double xy, double xz,
            double yy, double yz, double zz,
            vector<double*>& displacements,
            vector<SIGN>& signs);


};

#endif // TENSORCMD__H
