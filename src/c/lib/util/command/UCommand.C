//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: UCommand.C
//
//
// DESIGN:
//   A utiltity abstract command class whose main feature is that it
//   collects state for a command in command parameters and provides
//   an abstract execute() method that operates on an arbitrary
//   ICommandObject object.
//
//////////////////////////////////////////////////////////////////////////////
#include "util/UCommand.H"
#include "util/ErrMsg.H"

#include <iostream>
using std::ostream;
using std::istream;

//////////////////////////////////////////////////////////////////////////////
// Description
//   Constructor.
//   10 is a guess at a reasonable upper limit of the number of params a
//   command might have.  The vector will resize if necessary but at
//   least we avoid the monstrous default of 64.
//////////////////////////////////////////////////////////////////////////////
UCommand::UCommand(const string& commName)
{
  p_name =  commName;
  p_params = new vector<UCommandParam*>; 
}


//////////////////////////////////////////////////////////////////////////////
// Description
//  TODO -> delete the command params too.
//////////////////////////////////////////////////////////////////////////////
UCommand::~UCommand()
{
  delete p_params;
}

//////////////////////////////////////////////////////////////////////////////
// Description
//////////////////////////////////////////////////////////////////////////////
bool UCommand::operator==(const UCommand& rhs)
{
  return (p_name == rhs.p_name);
}

string UCommand::name() const
{ 
  return p_name;
}

string UCommand::getCommandClassId() const
{
   return "UCommand";
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Add a parameter to the command.
////////////////////////////////////////////////////////////////////////////
//
//  addParam
//
// addParam( some char*, char* )
bool UCommand::addParam(const string& paramName, const char* value)
{
  UCommandParam *line = new UCommandParam(paramName, value);
  p_params->push_back(line);
  return true;
}
bool UCommand::addParam(const char* paramName, const char* value)
{
   return addParam(string(paramName), value);
}
bool UCommand::addParam(const char* paramName, const vector<int>& value)
{
   return addParam(string(paramName), value);
}
bool UCommand::addParam(const string& paramName, const vector<int>& value)
{
  UCommandParam *line = new UCommandParam(paramName, value);
  p_params->push_back(line);
  return true;
}
//addParam(some char*, double)
bool UCommand::addParam(const string& paramName, double value)
{
  UCommandParam *line = new UCommandParam(paramName, value);
  p_params->push_back(line);
  return true;
}
bool UCommand::addParam(const char* paramName, double value)
{
   return addParam(string(paramName), value);
}

// addParam(some char*, int)
bool UCommand::addParam(const string& paramName, int value)
{
  UCommandParam *line = new UCommandParam(paramName, value);
  p_params->push_back(line);
  return true;
}
bool UCommand::addParam(const char* paramName, int value)
{
   return addParam(string(paramName), value);
}

// addParam(some char*, bool)
bool UCommand::addBoolParam(const string& paramName, bool value)
{
  UCommandParam *line = new UCommandParam(paramName, value, true);
  p_params->push_back(line);
  return true;
}
bool UCommand::addBoolParam(const char* paramName, bool value)
{
   return addBoolParam(string(paramName), value);
}

//
//    setParam
//
// setParam(some char*, char*)
bool UCommand::setParam(const string& paramName, const char* value)
{
  bool status = false;

  for (size_t i=0; i<p_params->size(); i++) {
    string tmp = (*p_params)[i]->name();
    if (paramName == (*p_params)[i]->name()) {
      (*p_params)[i]->set(value);
      status = true;
      break;
    }
  }
  EE_ASSERT(status, EE_WARNING, paramName + " :parameter not found - not set!");
  return status;
}
bool UCommand::setParam(const char* paramName, const char* value)
{
   return setParam(string(paramName), value);
}

bool UCommand::setParam(const string& paramName, const vector<int>& value)
{
  bool status = false;

  int length = p_params->size();
  for (int i=0; i<length; i++) {
    if (paramName == (*p_params)[i]->name()) {
      (*p_params)[i]->set(value);
      status = true;
      break;
    }
  }
  EE_ASSERT(status, EE_WARNING, paramName + " :parameter not found - not set!");
  return status;
}
bool UCommand::setParam(const char* paramName, const vector<int>& value)
{
   return setParam(string(paramName), value);
}

// setParam(some char*, int)
bool UCommand::setParam(const string& paramName, int value)
{
  bool status = false;

  int length = p_params->size();
  for (int i=0; i<length; i++) {
    if (paramName == (*p_params)[i]->name()) {
      (*p_params)[i]->set(value);
      status = true;
      break;
    }
  }
  EE_ASSERT(status, EE_WARNING, paramName + " :parameter not found - not set!");
  return status;
}
bool UCommand::setParam(const char* paramName, int value)
{
   return setParam(string(paramName), value);
}

// setParam(some char*, double)
bool UCommand::setParam(const string& paramName, double value)
{
  bool status = false;

  for (size_t i=0; i<p_params->size(); i++) {
    if (paramName == (*p_params)[i]->name()) {
      (*p_params)[i]->set(value);
      status = true;
      break;
    }
  }
  EE_ASSERT(status, EE_WARNING, paramName + " :parameter not found - not set!");
  return status;
}
bool UCommand::setParam(const char* paramName, double value)
{
   return setParam(string(paramName), value);
}

bool UCommand::setBoolParam(const string& paramName, bool value)
{
  bool status = false;

  int length = p_params->size();
  for (int i=0; i<length; i++) {
    if (paramName == (*p_params)[i]->name()) {
      (*p_params)[i]->setBool(value);
      status = true;
      break;
    }
  }
  EE_ASSERT(status, EE_WARNING, paramName + " :parameter not found - not set!");
  return status;
}

bool UCommand::setBoolParam(const char* paramName, bool value)
{
   return setBoolParam(string(paramName), value);
}

//
// getParam
//
// getParam(some char*, int)
void UCommand::getParam(const string& name, int& value)
{
  bool found = false;
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->get(value);
      found = true;
      break;
    }
  }
  EE_RT_ASSERT(found, EE_WARNING, name + ": parameter not found!");
}

void UCommand::getParam(const char* name, int& value)
{
   getParam(string(name), value);
}

// getParam(some char*, double)
void UCommand::getParam(const string& name, double& value)
{
  bool found = false;
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->get(value);
      found = true;
      break;
    }
  }
  EE_RT_ASSERT(found, EE_WARNING, name + ": parameter not found!");
}

void UCommand::getParam(const string& name, vector<int>& value)
{
  bool found = false;
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->get(value);
      found = true;
      break;
    }
  }
  EE_RT_ASSERT(found, EE_WARNING, name + ": parameter not found!");
}
void UCommand::getParam(const char* name, vector<int>& value)
{
   getParam(string(name), value);
}

void UCommand::getParam(const char* name, double& value)
{
   getParam(string(name), value);
}

// getParam(some char*, some char*)
void UCommand::getParam(const string& name, string& value)
{
  bool found = false;
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->get(value);
      found = true;
      break;
    }
  }
  EE_RT_ASSERT(found, EE_WARNING, name + ": parameter not found!");
}
// getParam(some char*, bool)
void UCommand::getBoolParam(const string& name, bool& value)
{
  bool found = false;
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->getBool(value);
      found = true;
      break;
    }
  }
  EE_RT_ASSERT(found, EE_WARNING, name + ": parameter not found!");
}

void UCommand::getBoolParam(const char* name, bool& value)
{
   getBoolParam(string(name), value);
}

void UCommand::getParamDefault(const string& name, int& value)
{
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->getDefault(value);
    }
  }
}
void UCommand::getParamDefault(const string& name, double& value)
{
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->getDefault(value);
    }
  }
}
void UCommand::getParamDefault(const string& name, string& value)
{
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->getDefault(value);
    }
  }
}

void UCommand::getParamDefault(const string& name, vector<int>& value)
{
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->getDefault(value);
    }
  }
}
void UCommand::getBoolParamDefault(const string& name, bool& value)
{
  for (size_t i=0; i<p_params->size(); i++) {
    if (name == (*p_params)[i]->name()) {
      (*p_params)[i]->getBoolDefault(value);
    }
  }
}

void UCommand::resetToDefaults()
{
  int cnt = p_params->size();
  for (int idx=0; idx<cnt; idx++) {
    (*p_params)[idx]->resetToDefault();
  }
}

vector<string> *UCommand::getParameterNames() const
{
  int cnt = p_params->size();
  vector<string> *ret = new vector<string>();
  for (int idx=0; idx<cnt; idx++) {
    ret->push_back((*p_params)[idx]->name());
  }
  return ret;
}



////////////////////////////////////////////////////////////////////////////
// TODO - dump in xml format
////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& ostr, const UCommand& command)
{
  ostr << "<Command name=\"" << command.name() << "\">" << endl;;
  
  for (size_t index = 0; index < command.p_params->size(); index++) {
    ostr << "  <Param name=\"" << (*command.p_params)[index]->name();
    ostr << "\" type=\"" ;
    switch ((*command.p_params)[index]->type()) {
      case UCommandParam::aDouble:
        ostr << "double\"";
        break;
      case UCommandParam::aInt:
        ostr << "int\"";
        break;
      case UCommandParam::aIntList:
        ostr << "int-list\"";
        break;
      case UCommandParam::aString:
        ostr << "string\"";
        break;
      case UCommandParam::aBool:
        ostr << "bool\"";
        break;
      default:
        ostr << "???\"";
        break;
    }
    ostr << ">" << endl;
    ostr << "    " << (*command.p_params)[index]->toString() << endl;

    ostr << "  </Param>" << endl;
  }
  ostr << "</Command>" << endl;
  //ostr << "Help: " << command.getMessage() << ";";
  //ostr << "Error: " << command.getErrorMessage() << ";";
  //ostr << "]" << endl;
  return ostr;
}




void UCommand::setMessage(string msg)
{
  p_helpMsg = msg;
}

string UCommand::getMessage(void) const
{
  return p_helpMsg;
}

void UCommand::setErrorMessage(string msg)
{
   p_lastError = msg;
}

string UCommand::getErrorMessage(void) const
{
  return p_lastError;
}
