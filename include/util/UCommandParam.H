#ifndef  UCOMMANDPARAM_HH
#define  UCOMMANDPARAM_HH

#include <string>
#include <vector>
  using std::string;
  using std::vector;



class UCommandParam
{
public:

  enum ParamType {aString, aDouble, aInt, aIntList, aBool };

  UCommandParam(const string &name, const char *value);
  UCommandParam(const string &name, int value);
  UCommandParam(const string &name, double value);
  UCommandParam(const string &name, const vector<int>& value);

  /* Bool and int are the same on some compilers, but not on SGI, so I had
     to find some way to distinguish between the two constructors.
     This is pretty hokey - it would be better to subclass, but
     I don't feel like changing everything around. */
  UCommandParam(const string &name, bool value, bool boolFlag);
  ~UCommandParam(void);

  bool  operator==(const UCommandParam& rhs);

  string name(void) const;
  ParamType type(void) const;

  // For serialization
  string    typeAsString() const;
  static ParamType typeFromString(const string& stype);

  void      get(int& value) const;
  void      get(double& value) const;
  void      get(string& value) const;
  void      get(vector<int>& value) const;
  void      getBool(bool& value) const;
  void      getDefault(int& value) const;
  void      getDefault(vector<int>& value) const;
  void      getDefault(double& value) const;
  void      getDefault(string& value) const;
  void      getBoolDefault(bool& value) const;
  string toString() const;

  void set(const char *val);
  void set(int val);
  void set(const vector<int>& val);
  void set(double val);
  void setBool(bool val);
  void resetToDefault();

protected:
  void cleanValue(void *val);
  void set(void **what, const char *val);
  void set(void **what, int val);
  void set(void **what, double val);
  void set(void **what, const vector<int>& val);
  void setBool(void **what, bool val);

private:

  string p_paramName;
  ParamType p_type;
  void* p_value;
  void* p_default;

};


#endif // UCOMMANDPARAM_HH
