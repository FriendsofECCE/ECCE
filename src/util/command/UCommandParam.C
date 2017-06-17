//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: UCommandParam.C
//
//
// Classes:
//	UCommandParam
//
// DESIGN:
//   A CommandParam is a key/value pair where the value is limited to
//   simple types such as strings, ints, and doubles.  This is a
//   design decision to keep it easy to do command line interfaces
//   where everything can be interpreted as a string.
//
//   The underlying data value is cast into and out of void*.  This
//   should perhaps be a union.  A second pass was made over this code
//   to at least remove the void* from the interface thus ensuring
//   a little type safety.  Asserts are triggered if a parameter
//   is accessed as a type other than the one it was originally defined
//   as.
//////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>  // sprintf

#include "util/ErrMsg.H"
#include "util/UCommandParam.H"


UCommandParam::UCommandParam(const string& nameIn,  const char  *valueIn) 
{
  p_value = (void*)0;
  p_default = (void*)0;
  p_paramName = nameIn;
  p_type = UCommandParam::aString;
  set(valueIn);
  set(&p_default,valueIn);
}

UCommandParam::UCommandParam(const string& nameIn,  int  valueIn) 
{
  p_value = (void*)0;
  p_default = (void*)0;
  p_paramName = nameIn;
  p_type = UCommandParam::aInt;
  set(valueIn);
  set(&p_default,valueIn);
}

UCommandParam::UCommandParam(const string& nameIn,  double valueIn) 
{
  p_value = (void*)0;
  p_default = (void*)0;
  p_paramName = nameIn;
  p_type = UCommandParam::aDouble;
  set(valueIn);
  set(&p_default,valueIn);
}

UCommandParam::UCommandParam(const string& nameIn,  const vector<int>& value) 
{
  p_value = (void*)0;
  p_default = (void*)0;
  p_paramName = nameIn;
  p_type = UCommandParam::aIntList;
  set(value);
  set(&p_default,value);
}

/* If you are using a bool parameter, pass in two bools to the
   constructor, so the compiler can distinguish the difference. */
UCommandParam::UCommandParam(const string& nameIn,  bool  valueIn,
			     bool boolFlag) 
{
  p_value = (void*)0;
  p_default = (void*)0;
  p_paramName = nameIn;
  p_type = UCommandParam::aBool;
  setBool(valueIn);
  setBool(&p_default,valueIn);
}

////////////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////////////
UCommandParam::~UCommandParam(void)
{
  cleanValue(p_value);
  cleanValue(p_default);
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Equality is determined by the key name.
////////////////////////////////////////////////////////////////////////////
bool UCommandParam::operator==(const UCommandParam& rhs)
{ return name() == rhs.name(); }

////////////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////////////
string UCommandParam::name(void) const
{ return p_paramName; }

////////////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////////////
UCommandParam::ParamType UCommandParam::type(void) const
{ return p_type; }

string UCommandParam::typeAsString() const
{
   string ret = "integer";
   switch (p_type) {
      case aString: ret="string"; break; 
      case aDouble: ret="double"; break; 
      case aInt:    ret="integer"; break; 
      case aIntList:    ret="integer-list"; break; 
      case aBool:    ret="boolean"; break; 
      default:
        cerr << "Unknown type " << p_type << endl;
   }
   return ret;
}

UCommandParam::ParamType UCommandParam::typeFromString(const string& stype)
{
   UCommandParam::ParamType ret = aInt;
   if (stype == "string") {
      ret = aString;
   } else if (stype == "integer") {
      ret = aInt;
   } else if (stype == "double") {
      ret = aDouble;
   } else if (stype == "boolean") {
      ret = aBool;
   } else if (stype == "integer-list") {
      ret = aIntList;
   } else {
        cerr << "Unknown type " << stype << endl;
   }
   return ret;

}


////////////////////////////////////////////////////////////////////////////
// Description
//   Returns copy of param value.
////////////////////////////////////////////////////////////////////////////
void UCommandParam::get(int& value) const
{
  EE_ASSERT((p_type==aInt),EE_WARNING,name()+" not declared as int")
  value = *(int*)p_value;
}
void UCommandParam::get(double& value) const
{
  EE_ASSERT((p_type==aDouble),EE_WARNING,name()+" not declared as double");
  value = *(double*)p_value;
}
void UCommandParam::get(string& value) const
{
  EE_ASSERT((p_type==aString),EE_WARNING,name()+" not declared as string");
  value = (char*)p_value;
}
void UCommandParam::get(vector<int>& value) const
{
  EE_ASSERT((p_type==aIntList),EE_WARNING,name()+" not declared as int list");
  value = *(vector<int>*)p_value;
}
void UCommandParam::getBool(bool& value) const
{
  EE_ASSERT((p_type==aBool),EE_WARNING,name()+" not declared as bool")
  value = *(bool*)p_value;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Returns copy of the default param value.
////////////////////////////////////////////////////////////////////////////
void UCommandParam::getDefault(int& value) const
{
  EE_ASSERT((p_type==aInt),EE_WARNING,name()+" not declared as int")
  value = *(int*)p_default;
}
void UCommandParam::getDefault(double& value) const
{
  EE_ASSERT((p_type==aDouble),EE_WARNING,name()+" not declared as double");
  value = *(double*)p_default;
}
void UCommandParam::getDefault(string& value) const
{
  EE_ASSERT((p_type==aString),EE_WARNING,name()+" not declared as string");
  value = (char*)p_default;
}
void UCommandParam::getDefault(vector<int>& value) const
{
  EE_ASSERT((p_type==aIntList),EE_WARNING,name()+" not declared as int list");
  value = *(vector<int>*)p_value;
}
void UCommandParam::getBoolDefault(bool& value) const
{
  EE_ASSERT((p_type==aBool),EE_WARNING,name()+" not declared as bool")
  value = *(bool*)p_default;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Resets the command to its initial value which was cached.
////////////////////////////////////////////////////////////////////////////
void UCommandParam::resetToDefault()
{
  if (p_type == aString) {
    set(&p_value,*(char*)p_default);
  } else if (p_type == aInt) {
    set(&p_value,*(int*)p_default);
  } else if (p_type == aDouble) {
    set(&p_value,*(double*)p_default);
  } else if (p_type == aIntList) {
    set(&p_value,*(vector<int>*)p_default);
  } else if (p_type == aBool) {
    set(&p_value,*(bool*)p_default);
  }
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Force conversion of value to a string.  This is useful for
//   serializing commands and their values.
////////////////////////////////////////////////////////////////////////////
string UCommandParam::toString() const 
{
  if (p_type == aString) {
    return (char*)p_value;
  } else {
    char buf[32];
    if (p_type == aInt) {
      sprintf(buf,"%d",*(int*)p_value);
    } else if (p_type == aDouble) {
      sprintf(buf,"%lf",*(double*)p_value);
    } else if (p_type == aBool) {
      sprintf(buf,"%d",*(bool*)p_value);
    } else if (p_type == aIntList) {
      EE_RT_ASSERT(false, EE_WARNING, "Not implemented - int list to string");
    }
    return buf;
  }
}

void UCommandParam::set(const char *value)
{
  set(&p_value,value);
}
void UCommandParam::set(double value)
{
  set(&p_value,value);
}
void UCommandParam::set(int value)
{
  set(&p_value,value);
}
void UCommandParam::set(const vector<int>& value)
{
  set(&p_value,value);
}
void UCommandParam::setBool(bool value)
{
  setBool(&p_value,value);
}

////////////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////////////
void UCommandParam::set(void **what, const char *value)
{
  EE_ASSERT(value,EE_WARNING,"parameter is null pointer")
  cleanValue(*what);
  EE_ASSERT((p_type==aString),EE_WARNING,"Param type is not a string")

  char *str = (char*)value;
  char *tmp = new char[strlen(str)+1];
  strcpy(tmp,str);
  *what = (void*)tmp;
}

void UCommandParam::set(void **what,double value)
{
  EE_ASSERT((p_type==aDouble),EE_WARNING,name()+": param type is not a double")
  cleanValue(*what);

  double *tmp = new double(value);
  *what = tmp;
}
void UCommandParam::set(void **what,int value)
{
  cleanValue(*what);
  EE_ASSERT((p_type==aInt),EE_WARNING,name()+": param type is not an integer")

  int *tmp = new int(value);
  *what = tmp;
}

void UCommandParam::set(void **what,const vector<int>& value)
{
  cleanValue(*what);
  EE_ASSERT((p_type==aIntList),EE_WARNING,
             name()+": param type is not an integer list")

  vector<int> *tmp = new vector<int>();
  vector<int>::const_iterator it;
  for (it=value.begin(); it != value.end(); it++) {
    tmp->push_back(*it);
  }
  *what = tmp;
}

void UCommandParam::setBool(void **what, bool value)
{
  cleanValue(*what);
  EE_ASSERT((p_type==aBool),EE_WARNING,name()+": param type is not a bool")

  bool *tmp = new bool(value);
  *what = tmp;
}

///////////////////////////////////////////////////////////////////////////
// Description
//   Delete memory associated with the void* depending on the type.
///////////////////////////////////////////////////////////////////////////
void UCommandParam::cleanValue(void *val)
{

  if (val != (void*)0) {
    switch (type()) {
      case aString:
        if (val) delete [] (char*)val;
        break;
      case aDouble:
        if (val) delete (double*)val;
        break;
      case aInt:
        if (val) delete (int*)val;
        break;
      case aIntList:
        if (val) delete (vector<int>*)val;
        break;
      case aBool:
        if (val) delete (bool*)val;
        break;
      default:
        EE_RT_ASSERT(false, EE_WARNING, "Invalid type");
        break;
    }
    val = (void*)0;
  }
}
