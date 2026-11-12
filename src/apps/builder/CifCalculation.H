#ifndef CIFCALCULATION_H_
#define CIFCALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

class CifCalculation : public AbstractPropCalculation
{
  public:
    CifCalculation();
    CifCalculation(const EcceURL& url);

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

#endif // CIFCALCULATION_H_
