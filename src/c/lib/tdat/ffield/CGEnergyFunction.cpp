////////////////////////////////////////////////////////////////////////////////
// CGEnergyFunction Class
//
//
// DESIGN:  
//     This wraps the energy and force functions in the ForceField Class so that
//     they can be used by the CGMinimizer.
//
////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"

#include "tdat/FFCoords.H"
#include "tdat/ObjectiveFunction.H"
#include "tdat/CGEnergyFunction.H"
#include "tdat/ForceField.H"

CGEnergyFunction::CGEnergyFunction()
{
  p_ffield = (ForceField*)0;
}

CGEnergyFunction::~CGEnergyFunction()
{
}

void CGEnergyFunction::setForceField(ForceField *ffield)
{
  p_ffield = ffield;
}

/**
 * @param canceled set to true if the function terminated early (user 
 *        cancel).  With the fancy function pointers, I found that throwing
 *        and catching exceptions did not work at this level in the code.
 *        I left the InvalidException in place anyway because its a program
 *        debug mechanism.
 */
double CGEnergyFunction::getValue(bool& canceled, FFCoordinates *coords) const
       throw (InvalidException)
{
  double ret;
  try  {
    ret = p_ffield->energy(canceled, coords);
  }
  catch( InvalidException&  ie)  {
    EE_ASSERT((p_ffield != (ForceField*)0), EE_FATAL,
               "CGEnergyFunction: Null pointer for ForceField");
  }
  return ret;
}

/**
 * @param canceled set to true if the function terminated early (user 
 *        cancel).  With the fancy function pointers, I found that throwing
 *        and catching exceptions did not work at this level in the code.
 *        I left the InvalidException in place anyway because its a program
 *        debug mechanism.
 */
FFCoordinates* CGEnergyFunction::getGradient( bool& canceled,
                                             const FFCoordinates *coords,
                                             FFCoordinates *gradient) const
       throw (InvalidException)
{
  FFCoordinates * ret;
  try  {
    ret = p_ffield->force(canceled, coords, gradient);
  }
  catch( InvalidException&  ie)  {
    EE_ASSERT((p_ffield != (ForceField*)0), EE_FATAL,
               "CGEnergyFunction: Null pointer for ForceField");
  }
  return ret;
}
