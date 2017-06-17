#ifndef ABSTRACTPROPCALCULATION_H_
#define ABSTRACTPROPCALCULATION_H_

#include "dsm/IPropCalculation.H"
#include "dsm/PropertyDoc.H"

#include "util/EcceURL.H"

#include <string>
  using std::string;
#include <vector>
  using std::vector;

class Fragment;
class TProperty;

class AbstractPropCalculation : public IPropCalculation
{
  public:
    virtual ~AbstractPropCalculation() { };
    virtual vector<string> propertyNames(void);
    virtual TProperty* getProperty(const string& name);
    virtual TProperty* updateProperty(const string& name, const string& value);
    virtual bool putProperty(TProperty* prop);
    virtual bool deleteProperties(void);
    virtual void flushPropertyCache(void); 

    virtual bool fragment(Fragment *frag);
    virtual Fragment* fragment() const;
    virtual bool getFragment(Fragment &frag);
    // A step of -1 implies the last step
    virtual Fragment* getFragmentStep(int step=-1); 
    virtual Fragment* getFragmentStep(int step, PropertyDoc& doc); 
    virtual bool getFragmentStep(Fragment *frag,int step=-1); 
    virtual long getFragmentModifiedDate() const; 

    virtual EcceURL getURL() const = 0;

    virtual bool isReadOnly() const;

    virtual bool promptBeforeSave(Fragment *frag, string& msg, string& title);

  protected:
    AbstractPropCalculation() { };
};

#endif /* ABSTRACTPROPCALCULATION_H_ */
