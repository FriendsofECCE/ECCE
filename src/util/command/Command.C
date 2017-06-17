/**
 * @file
 *
 *
 */
#include <iostream>
 using std::cout;
 using std::endl;

#include "util/STLUtil.H"
#include "util/Command.H"
#include "util/Receiver.H"
#include "util/CommandParameter.H"

#include "util/StringTokenizer.H"

/**
 * Constructor.
 * @param name name of this command - ususally the same as the class name.
 */
Command::Command(const string& name, Receiver * const receiver)
{
  p_receiver = receiver;
  p_name =  name;
}


/**
 * Destructor.
 */
Command::~Command()
{
}


/**
 * Provide psuedo constructor for pythong subclasses.
 * Since this class is abstract, pythong does not create a constructor
 * that I can initialize with.  Probably considered poor design to 
 * have an abstract class that has a real constructor.
 * Additionally it must be public for swig unless I miss something
 * @todo fix my design
 */
void Command::psuedoConstructor(const string& name, Receiver * const receiver)
{
  p_receiver = receiver;
  p_name =  name;
}


/** 
 * Reports if this command is designed to support undo or not.
 * Subclasses that don't support undo (this is discouraged) should override
 * this and return false.
 * @return true
 */
bool Command::isUndoable() const
{
   return true;
}


/**
 * Equality check based on command name only.
 */
bool Command::operator==(const Command& rhs)
{
  return (p_name == rhs.p_name);
}



string Command::getName() const
{ 
  return p_name;
}



Receiver *Command::getReceiver() const
{ 
  return p_receiver;
}


string Command::getUndoLabel() const
{
   return getName();
}



/**
 * Add a parameter for this command.
 * Currently no error checking is done to see if the parameter might
 * already be defined.
 */
void Command::addParameter(CommandParameter * param)
{
   p_params.push_back(param);
}



CommandParameter * Command::getParameter(const string& name)
{
  CommandParameter * ret = 0;

  vector<CommandParameter * >::const_iterator it;

  string lcname = name;
  STLUtil::toLower(lcname);

  string pname;
  for (it=p_params.begin(); it != p_params.end(); it++) {
    pname = (*it)->getName();
    STLUtil::toLower(pname);
    if (lcname == pname) {
       ret = (*it);
      break;
    }
  }
  return ret;
}



void Command::resetToDefaults()
{
  vector<CommandParameter * >::const_iterator it;

  for (it=p_params.begin(); it != p_params.end(); it++) {
     (*it)->resetToDefault();
  }
}



vector<string> *Command::getParameterNames() const
{
  vector<string> *ret = new vector<string>();

  vector<CommandParameter * >::const_iterator it;

  for (it=p_params.begin(); it != p_params.end(); it++) {
     ret->push_back((*it)->getName());
  }
  return ret;
}



/**
 * Dump command for debug print. 
 */
ostream& operator<<(ostream& ostr, const Command& cmd)
{
  ostr << "<command name=\"" << cmd.getName() << "\">" << endl;;

  for (size_t idx=0; idx<cmd.p_params.size(); idx++) {
     CommandParameter *p = cmd.p_params[idx];
     ostr << "<param name=" << p->getName() << ">" << p->toString() << "</param>\n";

  }
  
  ostr << "</command>" << endl;
  //ostr << "Help: " << command.getMessage() << ";";
  //ostr << "Error: " << command.getErrorMessage() << ";";
  //ostr << "]" << endl;
  return ostr;
}




void Command::setHelpMessage(const string& msg)
{
  p_helpMsg = msg;
}




string Command::getHelpMessage() const
{
  return p_helpMsg;
}




void Command::setErrorMessage(const string& msg)
{
   p_lastError = msg;
}




string Command::getErrorMessage() const
{
  return p_lastError;
}


void Command::parse(const string& data) throw(EcceException)
{
   if (data != "") {
      StringTokenizer tokenizer(data," ");
      vector<string> tokens = tokenizer.tokenize(" ");
      for (size_t idx=0; idx<tokens.size(); idx++) {
         // Get a keyword
         string token = tokens[idx];
         CommandParameter *p = getParameter(token);
         if (p == 0) {
            string msg = "Parameter not defined: ";
            msg += token;
            msg += ". Supported parameters are: ";
            vector<string> *names = getParameterNames();
            for (size_t idx=0; idx<names->size(); idx++) {
               msg += (*names)[idx] + ",";
            }
            if (msg[msg.size()-1] == ',') msg.erase(msg.size()-1);
            delete names;
            throw EcceException(msg, WHERE);
         } else if (p->getAny().getTypeCode() == Any::aBool) {
            // If its a boolean, the parameter is considered optional
            idx++;  // increment to value
            if (idx < tokens.size()) {
               string value = tokens[idx];
               STLUtil::toLower(value);
               if (value == "true" || value == "false" ||
                   value == "1" || value == "0") {
                  p->set(value);
               } else {
                  idx--;  // assume it wasn't a value and process as token
                  p->set("true"); // assume true
               }
            } else {
               // No parameter - again assume true
               p->set("true");
            }

         } else {
            idx++;  // skip the value
            if (idx >= tokens.size()) {
              throw EcceException(string("No value defined for parameter: ") + token, WHERE);
            }
            string value = tokens[idx];
            p->set(value);
         }
      }
   }
}
