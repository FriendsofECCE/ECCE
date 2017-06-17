#ifndef CUBECALCULATION_H_
#define CUBECALCULATION_H_

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

/**
 * Allows trajectory files to be opened and understood as property providers.
 */
class CubeCalculation : public AbstractPropCalculation
{
  public:
    CubeCalculation(const EcceURL& url);
    virtual ~CubeCalculation();

    virtual vector<string> propertyNames(void);

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

#endif // CUBECALCULATION_H_
