/**
* @file
*
*
*/
#ifndef _ICALCULATION_HH
#define _ICALCULATION_HH

#include "util/RetryException.H"
#include "util/CancelException.H"

#include "tdat/SpinMult.H"
#include "tdat/TRunType.H"
#include "tdat/GUIValues.H"
#include "tdat/TTheory.H"
#include "tdat/Fragment.H"
#include "tdat/ESPConstraintModel.H"
#include "tdat/GeomConstraintModel.H"

#include "dsm/TGBSConfig.H"
#include "dsm/PropertyTask.H"


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


