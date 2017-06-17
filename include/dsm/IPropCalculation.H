#ifndef _IPROPCALCULATION_H_
#define _IPROPCALCULATION_H_

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "dsm/PropertyDoc.H"

#include "util/EcceURL.H"

class Fragment;
class TProperty;


/**
 * Interface for a Calculation/Task that understand output properties.
 */
class IPropCalculation
{
   public:
      virtual ~IPropCalculation() { };
      virtual vector<string> propertyNames(void) = 0;
      virtual TProperty* getProperty(const string& name) = 0;
      virtual TProperty* updateProperty(const string& name, const string& value) = 0;
      virtual bool putProperty(TProperty* prop) = 0;
      virtual bool deleteProperties(void) = 0;
      virtual void flushPropertyCache(void) = 0; 

      virtual bool fragment(Fragment *frag) = 0;
      virtual Fragment* fragment() const = 0;
      virtual bool getFragment(Fragment &frag) = 0;
      // A step of -1 implies the last step
      virtual Fragment* getFragmentStep(int step=-1) = 0; 
      virtual Fragment* getFragmentStep(int step, PropertyDoc& doc) = 0; 
      virtual bool getFragmentStep(Fragment *frag,int step=-1) = 0; 
      virtual long getFragmentModifiedDate() const = 0; 

      virtual EcceURL getURL() const = 0;

      virtual bool isReadOnly() const = 0;

      virtual bool promptBeforeSave(Fragment *frag, string& msg, string& title) = 0;

   protected:
      IPropCalculation() { };
};

#endif /* _IPROPCALCULATION_H_ */
