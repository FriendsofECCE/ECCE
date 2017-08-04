/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;
#include <stdio.h>  // sprintf

#include "util/Any.H"
#include "util/NotImplementedException.H"
#include "util/InvalidException.H"


/**
 * Constructor.
 * Create default object with no type and no value.
 */
Any::Any()
{
   p_type = aUnknown;
   p_value = 0;
}



/**
 * Constructor.
 * Create an integer value objet.
 * @param value the value
 */
Any::Any(int value)
{
   p_type = aInt;
   setInteger(value);
}



/**
 * Constructor.
 * Create an bool value objet.
 * @param value the value
 */
Any::Any(bool value)
{
   p_type = aBool;
   setBoolean(value);
}




/**
 * Constructor.
 * Create an double value objet.
 * @param value the value
 */
Any::Any(double value)
{
   p_type = aDouble;
   setDouble(value);
}



/**
 * Constructor.
 * Create a string value objet.
 * This constructor takes a char* instead of a const string& because
 * the g++ compiler was unable to correctly resolve between
 *  Any(true) and Any("a string");
 * @param value the value
 */
Any::Any(const char * value)
{
   p_type = aString;
   setString(string(value));
}




/**
 * Constructor.
 * Create a string value objet.
 *  Any(true) and Any("a string");
 * @param value the value
 */
Any::Any(const string& value)
{
   p_type = aString;
   setString(value);
}



/**
 * Constructor.
 * Create a integer list value objet.
 * @param value the value
 */
Any::Any(const vector<int>& value)
{
   p_type = aIntList;
   setIntegerList(value);
}



/**
 * Constructor.
 * Create a string list value objet.
 * @param value the value
 */
Any::Any(const vector<string>& value)
{
   p_type = aStringList;
   setStringList(value);
}



/**
 * Copy Constructor.
 * Create a string list value objet.
 * @param rhs Any object to be copied
 */
Any::Any(const Any& rhs)
{
   p_type = aUnknown;
   p_value = 0;

   try {
      switch (rhs.p_type) {
         case aInt: 
            setInteger(rhs.getInteger());
            break;
         case aDouble: 
            setDouble(rhs.getDouble());
            break;
         case aBool: 
            setBoolean(rhs.getBoolean());
            break;
         case aString: 
            setString(rhs.getString());
            break;
         case aIntList: 
         {
            vector<int>* tmp = rhs.getIntegerList();
            setIntegerList(*tmp);
            delete tmp;
            break;
         }
         case aStringList: 
         {
            vector<string>* tmp = rhs.getStringList();
            setStringList(*tmp);
            break;
         }
         case aUnknown:
            // make compiler happy (no warnings)
            break;
      }
   } catch (MismatchedTypeException& ex) {
      // Shouldn't happen; will return false if it does
   }
}



/**
 * Destructor.
 * Create default object with no type and no value.
 */
Any::~Any()
{
   cleanValue(p_value);

   p_type = aUnknown;
   p_value = 0;
}



/**
 * Test equality based on type and value.
 */
bool Any::operator==(const Any& rhs)
{
   bool ret = false;
   if (p_type == rhs.p_type) {
      try {
         switch (p_type) {
            case aInt: 
              ret = getInteger() == rhs.getInteger();
              break;
            case aDouble: 
              ret = getDouble() == rhs.getDouble();
              break;
            case aBool: 
              ret = getBoolean() == rhs.getBoolean();
              break;
            case aString: 
              ret = getString() == rhs.getString();
              break;
            case aIntList: 
              ret = getIntegerList() == rhs.getIntegerList();
              break;
            case aStringList: 
              ret = getStringList() == rhs.getStringList();
              break;
            case aUnknown:
              // make compiler happy (no warnings)
              break;
         }
      } catch (MismatchedTypeException& ex) {
         // Shouldn't happen; will return false if it does
      }
   }
   return ret;
}



/**
 * Gets the type of this instance.
 */
Any::TypeCode Any::getTypeCode() const
{
   return p_type;
}



/**
 * Get the type as a string. Values are:
 * <li>unknown</li>
 * <li>string</li>
 * <li>integer</li>
 * <li>boolean</li>
 * <li>double</li>
 * <li>integer-list</li>
 * <li>string-list</li>
 * Primarily useful for serialization.
 */
string Any::getTypeCodeAsString() const
{
   string ret = "unknown";
   switch (p_type) {

      case aUnknown : break; 

      case aInt:           ret="integer"; break; 
      case aBool:          ret="boolean"; break; 
      case aDouble:        ret="double"; break; 
      case aString:        ret="string"; break; 
      case aIntList:   ret="integer-list"; break; 
      case aStringList:    ret="string-list"; break; 
      default:
        throw InvalidException("Unknown type in Any::getTypeCodeAsString()",
                               WHERE);
   }
   return ret;
}




/**
 * Determines type from a string representation.
 * See getTypeCodeAsString() for value and relevence.
 */
Any::TypeCode Any::getTypeCodeFromString(const string& stype)
{
   Any::TypeCode ret = aInt;
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
   } else if (stype == "string-list") {
      ret = aStringList;
   } else if (stype == "unknown") {
      ret = aUnknown;
   } else {
      ret = aUnknown;
      throw InvalidException("Unknown type in Any::getTypeCodeFromString()",
                             WHERE);
   }
   return ret;
}




/**
 * Get the value as an integer.
 * @throw MismatchedTypeException if the value is not an integer.
 * @todo maybe do type conversion for double/bool?
 */
int Any::getInteger() const throw(MismatchedTypeException)
{
   int value = 0;
   if (p_type == aInt) {
      value = *(int*)p_value;
   } else {
      throw MismatchedTypeException("Trying to access " + getTypeCodeAsString()
            + " as integer", WHERE);
   }
   return value;
}



/**
 * Get the value as an double.
 * @throw MismatchedTypeException if the value is not a double.
 * @todo maybe do type conversion for double?
 */
double Any::getDouble() const throw(MismatchedTypeException)
{
   double value = 0.;
   if (p_type == aDouble) {
      value = *(double*)p_value;
   } else {
      throw MismatchedTypeException("Trying to access " + getTypeCodeAsString()
            + " as double", WHERE);
   }
   return value;
}



/**
 * Get the value as a bool.
 * @throw MismatchedTypeException if the value is not a bool.
 */
bool Any::getBoolean() const throw(MismatchedTypeException)
{
   bool value = false;
   if (p_type == aBool) {
      value = *(bool*)p_value;
   } else {
      throw MismatchedTypeException("Trying to access " + getTypeCodeAsString()
            + " as boolean", WHERE);
   }
   return value;
}



/**
 * Get the value as a string.
 * @throw MismatchedTypeException if the value is not a string.
 */
string Any::getString() const throw(MismatchedTypeException)
{
   string value;
   if (p_type == aString) {
      value = *(string*)p_value;
   } else {
      throw MismatchedTypeException("Trying to access " + getTypeCodeAsString()
            + " as string", WHERE);
   }
   return value;
}



/**
 * Get the value as a integer list.
 * @return a copy of the integer vector.  Caller is responsible for memory
 * @throw MismatchedTypeException if the value is not an integer list.
 */
vector<int>* Any::getIntegerList() const throw(MismatchedTypeException)
{
   vector<int>* ret = 0;
   if (p_type == aIntList) {
      ret = new vector<int>;
      *ret = *(vector<int>*)p_value;
   } else {
      throw MismatchedTypeException("Trying to access " + getTypeCodeAsString()
            + " as integer-list", WHERE);
   }
   return ret;
}



/**
 * Get the value as a string list.
 * @return a copy of the string vector.  Caller is responsible for memory
 * @throw MismatchedTypeException if the value is not an string list.
 */
vector<string>* Any::getStringList() const throw(MismatchedTypeException)
{
   vector<string>* ret = 0;
   if (p_type == aStringList) {
      ret = new vector<string>;
      *ret = *(vector<string>*)p_value;
   } else {
      throw MismatchedTypeException("Trying to access " + getTypeCodeAsString()
            + " as string-list", WHERE);
   }
   return ret;
}



/**
 *  Force conversion of value to a string.  This is useful for
 *  serializing commands and their values.
 *  @todo int and string list currently do not serialize. 
 */
string Any::toString() const 
{
  if (p_type == aString) {
    return *(string*)p_value;
  } else {
    char buf[240];
    string ret;
    if (p_type == aInt) {
      sprintf(buf,"%d",*(int*)p_value);
      ret = buf;
    } else if (p_type == aDouble) {
      sprintf(buf,"%lf",*(double*)p_value);
      ret = buf;
    } else if (p_type == aBool) {
      sprintf(buf,"%d",*(bool*)p_value);
      ret = buf;
    } else if (p_type == aIntList) {
      vector<int> *list = (vector<int>*)p_value;
      int nsize = list->size();
      if (nsize > 0) {
        sprintf(buf,"%d",(*list)[0]);
        ret.append(buf);
        for (int i=1; i<nsize; i++) {
          sprintf(buf,",%d",(*list)[i]);
          ret.append(buf);
        }
      }
    } else if (p_type == aStringList) {
      vector<string> *list = (vector<string>*)p_value;
      int nsize = list->size();
      if (nsize > 0) {
        sprintf(buf,"%s",(*list)[0].c_str());
        ret.append(buf);
        for (int i=1; i<nsize; i++) {
          sprintf(buf,",%s",(*list)[i].c_str());
          ret.append(buf);
        }
      }
    }
    return ret;
  }
}



void Any::setString(const string& value)
{
  set(&p_value,value);
}



void Any::setDouble(double value)
{
  set(&p_value,value);
}



void Any::setInteger(int value)
{
  set(&p_value,value);
}


void Any::setIntegerList(const vector<int>& value)
{
  set(&p_value,value);
}



void Any::setStringList(const vector<string>& value)
{
  set(&p_value,value);
}



void Any::setBoolean(bool value)
{
  set(&p_value,value);
}



void Any::set(void **what, const string& value)
{
  cleanValue(*what);

  p_type = aString;
  string *tmp = new string(value);
  *what = tmp;
}




void Any::set(void **what, double value)
{
  cleanValue(*what);

  p_type = aDouble;
  double *tmp = new double(value);
  *what = tmp;
}



void Any::set(void **what,int value)
{
  cleanValue(*what);

  p_type = aInt;
  int *tmp = new int(value);
  *what = tmp;
}

void Any::set(void **what,const vector<int>& value)
{
  cleanValue(*what);

  p_type = aIntList;
  vector<int> *tmp = new vector<int>();
  vector<int>::const_iterator it;
  for (it=value.begin(); it != value.end(); it++) {
    tmp->push_back(*it);
  }
  *what = tmp;
}




void Any::set(void **what,const vector<string>& value)
{
  cleanValue(*what);

  p_type = aStringList;
  vector<string> *tmp = new vector<string>();
  vector<string>::const_iterator it;
  for (it=value.begin(); it != value.end(); it++) {
    tmp->push_back(*it);
  }
  *what = tmp;
}



void Any::set(void **what, bool value)
{
  cleanValue(*what);


  p_type = aBool;
  bool *tmp = new bool(value);
  *what = tmp;
}



/**
 * Clears memory associated with the value.
 */
void Any::cleanValue(void *val)
{
  if (val != (void*)0) {
    switch (p_type) {
      case aString:
        if (val) delete (string*)val;
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
      case aStringList:
        if (val) delete (vector<string>*)val;
        break;
      case aBool:
        if (val) delete (bool*)val;
        break;
      default:
        // Not really a problem - type might be -1 (never set)
        // If other type, hopefully exceptions occur earlier.
        EcceException::log("Unknown type in Any::cleanValue()");
        break;
    }
    val = (void*)0;
  }
}
