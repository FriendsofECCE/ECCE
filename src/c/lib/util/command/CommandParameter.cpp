/**
 * @file
 *
 *
 */

#include <stdlib.h>
#include <iostream>
  using namespace std;

#include "util/CommandParameter.H"
#include "util/StringTokenizer.H"


/**
 * Constructor.
 * Create string parameter of the specified name.
 */
CommandParameter::CommandParameter(const string& nameIn,  const string& valueIn)
{
  p_paramName = nameIn;
  p_value.setString(valueIn);
  p_default.setString(valueIn);
}



/**
 * Constructor.
 * Create string parameter of the specified name.
 */
CommandParameter::CommandParameter(const string& nameIn,  const char* valueIn) 
{
  p_paramName = nameIn;
  p_value.setString(valueIn);
  p_default.setString(valueIn);
}




CommandParameter::CommandParameter(const string& nameIn,  int  valueIn) 
{
  p_paramName = nameIn;
  p_value.setInteger(valueIn);
  p_default.setInteger(valueIn);
}




CommandParameter::CommandParameter(const string& nameIn,  double valueIn) 
{
  p_paramName = nameIn;
  p_value.setDouble(valueIn);
  p_value.setDouble(valueIn);
}




CommandParameter::CommandParameter(const string& nameIn,  bool  valueIn)
{
  p_paramName = nameIn;

  p_value.setBoolean(valueIn);
  p_default.setBoolean(valueIn);
}




CommandParameter::CommandParameter(const string& nameIn,  const vector<int>& value) 
{
  p_paramName = nameIn;
  p_value.setIntegerList(value);
  p_default.setIntegerList(value);
}



CommandParameter::CommandParameter(const string& nameIn,  const vector<string>& value) 
{
  p_paramName = nameIn;
  p_value.setStringList(value);
  p_default.setStringList(value);
}



/**
 * Destructor.
 */
CommandParameter::~CommandParameter()
{
}




/**
 * Check equality based on both the parameter name and values.
 */
bool CommandParameter::operator==(const CommandParameter& rhs)
{ 
   return (getName() == rhs.getName() &&
           p_value == rhs.p_value);
}




string CommandParameter::getName() const
{ 
   return p_paramName; 
}



const Any& CommandParameter::getAny() const
{
   return p_value;
}



int CommandParameter::getInteger() const throw(MismatchedTypeException)
{
  return p_value.getInteger();
}



double CommandParameter::getDouble() const throw(MismatchedTypeException)
{
  return p_value.getDouble();
}




string CommandParameter::getString() const throw(MismatchedTypeException)
{
  return p_value.getString();
}



vector<int>* CommandParameter::getIntegerList() const 
        throw(MismatchedTypeException)
{
  return p_value.getIntegerList();
}




vector<string>* CommandParameter::getStringList() const 
        throw(MismatchedTypeException)
{
  return p_value.getStringList();
}




bool CommandParameter::getBoolean() const throw(MismatchedTypeException)
{
  return p_value.getBoolean();
}



int CommandParameter::getDefaultInteger() const throw(MismatchedTypeException)
{
  return p_default.getInteger();
}




double CommandParameter::getDefaultDouble() const throw(MismatchedTypeException)
{
  return p_default.getDouble();
}




string CommandParameter::getDefaultString() const throw(MismatchedTypeException)
{
  return p_default.getString();
}



vector<int>* CommandParameter::getDefaultIntegerList() const 
        throw(MismatchedTypeException)
{
  return p_default.getIntegerList();
}




vector<string>* CommandParameter::getDefaultStringList() const 
        throw(MismatchedTypeException)
{
  return p_default.getStringList();
}




bool CommandParameter::getDefaultBoolean() const throw(MismatchedTypeException)
{
  return p_default.getBoolean();
}



/**
 * Resets value to default.
 */
void CommandParameter::resetToDefault()
{
  p_value = p_default;
}



/**
 * Get parameter value as string.
 */
string CommandParameter::toString() const 
{
  return p_value.toString();
}

/**
 * Parses a value according the type defined for this parameter.
 * The type is set at construction time.
 * Lists must be comma delimited with no spaces.
 */
void CommandParameter::set(const string& value) 
       throw(EcceException,MismatchedTypeException)
{
   Any::TypeCode type = p_value.getTypeCode();
   if (type == Any::aInt) {
      setInteger(atoi(value.c_str()));
   } else if (type == Any::aDouble) {
      setDouble(atof(value.c_str()));
   } else if (type == Any::aBool) {
      if (value == "true") {
         setBoolean(true);
      } else if (value == "false") {
         setBoolean(false);
      } else {
        setBoolean(atoi(value.c_str()));
      }
   } else if (type == Any::aString) {
      setString(value);
   } else if (type == Any::aStringList) {
      vector<string> tokens;
      StringTokenizer t(value,",");
      while (t.hasMoreTokens()) {
         tokens.push_back(t.next());
      }
      setStringList(tokens);
   } else if (type == Any::aIntList) {
      vector<int> tokens;
      StringTokenizer t(value,",");
      while (t.hasMoreTokens()) {
         tokens.push_back(atoi(t.next().c_str()));
      }
      setIntegerList(tokens);
   } else {
      throw EcceException("Unknown type for parameter " + getName(),WHERE);
   }
}


void CommandParameter::setString(const string& value)
{
  p_value.setString(value);
}


void CommandParameter::setDouble(double value)
{
  p_value.setDouble(value);
}

void CommandParameter::setInteger(int value)
{
  p_value.setInteger(value);
}

void CommandParameter::setIntegerList(const vector<int>& value)
{
  p_value.setIntegerList(value);
}


void CommandParameter::setStringList(const vector<string>& value)
{
  p_value.setStringList(value);
}


void CommandParameter::setBoolean(bool value)
{
  p_value.setBoolean(value);
}

