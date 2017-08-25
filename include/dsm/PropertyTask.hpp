#ifndef PROPERTYTASK_H
#define PROPERTYTASK_H

#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <map>
  using std::map;
  using std::less;

#include "dsm/ChemistryTask.H"
#include "dsm/IPropCalculation.H"
#include "dsm/ResourceDescriptor.H"

#include "util/EcceURL.H"

class PropValue;
class PropString;
class PropVector;
class PropVecString;
class PropTable;
class PropVecTable;
class PropVecGrid;
class PropTSTable;
class PropTSVecTable;
class PropTSVecString;
class PropTSVector;
class PropTSVecVector;
class PropTSVecTable;
class TProperty;
class PropertyDoc;

class PropertyTask : public ChemistryTask,
                     public IPropCalculation
{
  public:
      virtual ~PropertyTask();

      // IPropCalculation virtuals
      virtual vector<string> propertyNames();
      virtual TProperty* getProperty(const string& name);
      virtual TProperty* updateProperty(const string& name, const string& value);
      virtual bool putProperty(TProperty* prop);
      virtual bool deleteProperties();
      virtual void flushPropertyCache();

      virtual bool fragment(Fragment* frag);
      virtual Fragment* fragment() const;
      virtual bool getFragment(Fragment &frag);
      virtual Fragment* getFragmentStep(int step=-1);
      virtual Fragment* getFragmentStep(int step, PropertyDoc& doc);
      virtual bool getFragmentStep(Fragment *frag,int step=-1);
      virtual long getFragmentModifiedDate() const;

      virtual EcceURL getURL() const;

      virtual bool isReadOnly() const;

      virtual bool promptBeforeSave(Fragment *frag, string& msg, string& title);

      // TaskJob virtuals
      virtual bool resetForRerun(const ResourceDescriptor::RUNSTATE&
                                 tostate = ResourceDescriptor::STATE_ILLEGAL);

      // IPropCalculation framework
      static bool canOpen(const EcceURL& url);
      static IPropCalculation *open(const EcceURL& url);
      static string inputMasks();

  protected:
      PropertyTask();

      vector<string> p_names;

  private:
      bool loadStep(Fragment *frag, int step);
      bool loadStep(Fragment *frag, int step, PropertyDoc& doc);
      bool getPropVector(PropVector* property, PropertyDoc& doc);
      bool getPropVecString(PropVecString* property , PropertyDoc&  doc);
      bool getPropTable(PropTable* property, PropertyDoc& doc);
      bool getPropVecTable(PropVecTable* property, PropertyDoc&  doc);
      bool getPropVecGrid(PropVecGrid* property, string name);
      bool getPropTSVecTable(PropTSVecTable* property, PropertyDoc& doc);
      bool getPropTSTable(PropTSTable* property, PropertyDoc& doc);
      bool getPropTSVecString(PropTSVecString* property, PropertyDoc& doc);
      bool getPropValue(PropValue* property, PropertyDoc& doc);
      bool getPropString(PropString* property, PropertyDoc& doc);
      bool getPropTSVector(PropTSVector* property, PropertyDoc& doc);
      bool getPropTSVecVector(PropTSVecVector* property, PropertyDoc& doc);

      bool updatePropVector(PropVector* property, PropertyDoc& doc);
      bool updatePropVecString(PropVecString* property , PropertyDoc&  doc);
      bool updatePropTable(PropTable* property, PropertyDoc& doc);
      bool updatePropVecTable(PropVecTable* property, PropertyDoc&  doc);
      bool updatePropVecGrid(PropVecGrid* property, EcceURL fileUrl);
      bool updatePropTSVecTable(PropTSVecTable* property, PropertyDoc& doc);
      bool updatePropTSTable(PropTSTable* property, PropertyDoc& doc);
      bool updatePropTSVecString(PropTSVecString* property, PropertyDoc& doc);
      bool updatePropValue(PropValue* property, PropertyDoc& doc);
      bool updatePropString(PropString* property, PropertyDoc& doc);
      bool updatePropTSVector(PropTSVector* property, PropertyDoc& doc);
      bool updatePropTSVecVector(PropTSVecVector* property, PropertyDoc& doc);


      void parseValues(char* values, vector<double>& valVec);
      void parseValues(char* values, vector<string>& valVec);
      char* getPropertyDoc(const string& name) ;
      bool initializeXmlUtils();
      TProperty* getTProperty (const string& name);
      void putTProperty (const string& name, TProperty* property);
      bool isOutputFileProp(const string& propName);

      map<string, TProperty*, less<string> > p_properties;
};

#endif /* PROPERTY_TASK */
