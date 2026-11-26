#ifndef SDFCALCULATION_H_
#define SDFCALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

// MDL mol/sdf (V2000); a multi-record .sdf gives its first record.
class SdfCalculation : public AbstractPropCalculation
{
  public:
    SdfCalculation();
    SdfCalculation(const EcceURL& url);

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

#endif // SDFCALCULATION_H_
