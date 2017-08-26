/**
* @file
*
*
*/
#ifndef _ICALCULATION_HH
#define _ICALCULATION_HH

#include "util/RetryException.hpp"
#include "util/CancelException.hpp"

#include "tdat/SpinMult.hpp"
#include "tdat/TRunType.hpp"
#include "tdat/GUIValues.hpp"
#include "tdat/TTheory.hpp"
#include "tdat/Fragment.hpp"
#include "tdat/ESPConstraintModel.hpp"
#include "tdat/GeomConstraintModel.hpp"

#include "dsm/TGBSConfig.hpp"
#include "dsm/PropertyTask.hpp"


/**
 * Abstract class providing an access architecture for applications
 * requiring persistent Ecce calculation data structure access or to
 * be used by transient calculations.  
 */
class ICalculation : virtual public PropertyTask 
{

  public:
    virtual ~ICalculation() {;}

    virtual bool theory(TTheory* theory, const string& theoryType="") = 0;
    virtual TTheory* theory(const string& theoryType="") const = 0;

    virtual bool runtype(const TRunType& name) = 0;
    virtual TRunType runtype() const = 0;

    virtual bool guiparams(GUIValues *params, const string& theoryType="") = 0;
    virtual GUIValues *guiparams(const string& theoryType="") const = 0;

    virtual bool gbsConfig(TGBSConfig* config, const string& theoryType="") = 0;
    virtual TGBSConfig* gbsConfig(const string& theoryType="") const = 0;

    virtual bool gbsUseExpCoeff(bool useExpCoeff) = 0;
    virtual bool gbsUseExpCoeff() const = 0;

    virtual bool openShells(unsigned long openshell) = 0;
    virtual unsigned long openShells() const = 0;

    virtual bool spinMultiplicity(SpinMult::SpinMultEnum sm) = 0;
    virtual SpinMult::SpinMultEnum spinMultiplicity() const = 0;

  protected:
    ICalculation() {;}
 
};


#endif


