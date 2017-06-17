#ifndef CARCALCULATION_H_
#define CARCALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

class CarCalculation : public AbstractPropCalculation
{
  public:
    CarCalculation();
    CarCalculation(const EcceURL& url);

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

#endif // CARCALCULATION_H_
