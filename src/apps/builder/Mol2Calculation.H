#ifndef MOL2CALCULATION_H_
#define MOL2CALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

// Tripos mol2; a file with several molecules gives the first.
class Mol2Calculation : public AbstractPropCalculation
{
  public:
    Mol2Calculation();
    Mol2Calculation(const EcceURL& url);

    // IPropCalculation virtuals
    virtual bool fragment(Fragment *frag);
    virtual bool getFragment(Fragment &frag);
    virtual EcceURL getURL() const;
    virtual bool isReadOnly() const;

    // IPropCalculation framework
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();

  private:
    EcceURL p_url;

    DECLARE_CALCULATION()
};

#endif // MOL2CALCULATION_H_
