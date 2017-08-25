#ifndef PDBCALCULATION_H_
#define PDBCALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

class PdbCalculation : public AbstractPropCalculation
{
  public:
    PdbCalculation();
    PdbCalculation(const EcceURL& url);

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
    bool    p_internalFormat;

    DECLARE_CALCULATION()
};

#endif // PDBCALCULATION_H_
