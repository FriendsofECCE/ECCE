#ifndef SPINSPINTENSORCMD__H
#define SPINSPINTENSORCMD__H

#include <vector>
  using std::vector;


#include "viz/TensorCmd.H"

class SGContainer;

/**
 * This command provides vector data for properties such as
 *    SHIELDTENSOR, DIPOLETENSOR, EFIELDGRAD
 */
class SpinSpinTensorCmd : public TensorCmd
{
   public:

      SpinSpinTensorCmd(const string& name, Receiver *, IPropCalculation *);
      virtual ~SpinSpinTensorCmd();


      /* Override to handle the case of displaying no vectors. */
      virtual bool execute() throw(EcceException);

      // CmdAddVector methods
      virtual bool getDisplacements(vector<double*>& displacements,
            vector<SIGN>& signs);
      virtual void getCenters(vector<double*>& centers);



};

#endif // SPINSPINTENSORCMD__H
