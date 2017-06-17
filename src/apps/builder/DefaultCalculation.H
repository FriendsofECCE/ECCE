#ifndef DEFAULTCALCULATION_H_
#define DEFAULTCALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"


/**
 * Interface for a Calculation/Task that understand output properties.
 */
class DefaultCalculation : public AbstractPropCalculation
{
  public:
    DefaultCalculation();

    virtual EcceURL getURL() const;

    virtual bool isReadOnly() const;

    static string nextURL();

    // IPropCalculation framework
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();

    static string getDefaultPrefix() { return DEFAULT_NAME; }

  private:
    static const string DEFAULT_NAME;
    static int p_count;

    int p_myIndex;

    DECLARE_CALCULATION()
};

#endif // DEFAULTCALCULATION_H_
