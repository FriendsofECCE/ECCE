#ifndef TRAJECTORYCALCULATION_H_
#define TRAJECTORYCALCULATION_H_

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

/**
 * Allows trajectory files to be opened and understood as property providers.
 */
class TrajectoryCalculation : public AbstractPropCalculation
{
  public:
    TrajectoryCalculation(const EcceURL& url);
    virtual ~TrajectoryCalculation();

    virtual vector<string> propertyNames(void);

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

#endif // TRAJECTORYCALCULATION_H_
