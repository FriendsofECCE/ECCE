#include <stdarg.h>
#include <stdio.h>  // sprintf
#include <string.h>

#include "util/ErrMsg.H"
#include "util/SFile.H"
#include "util/KeyValueReader.H"
#include "dsm/EDSIMessage.H"


// Class statics
vector<string> *EDSIMessage::p_keys = 0;
vector<string> *EDSIMessage::p_msgs = 0;



EDSIMessage::EDSIMessage()
{
  // Probably in most cases there will only be one message.
  p_stack = new vector<string>();
  p_stack_keys = new vector<string>();

  load();
}



EDSIMessage::~EDSIMessage()
{
  delete p_stack;
}



void EDSIMessage::clear()
{
  p_stack->clear();
  p_stack_keys->clear();
}

bool EDSIMessage::messages()
{
  return !p_stack->empty(); 
}

int EDSIMessage::size() const
{
  return p_stack->size();
}

bool EDSIMessage::findKey(const char* key) const
{
  bool ret = false;
  vector<string>::iterator it = p_stack_keys->begin();
  while (it != p_stack_keys->end()) {
    if (*it == key) {
      ret = true;
      break;
    }
    it++;
  }
  return ret;
}

int EDSIMessage::getKey(const char* key) const
{
  int ret = -1;
  vector<string>::iterator it = p_keys->begin();
  int cnt = 0;
  while (it != p_keys->end()) {
    if (*it == key) {
      ret = cnt;
      break;
    }
    cnt++;
    it++;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////
// Description
//   Get the next format string and also set the new index.
//   This is really stupid.
///////////////////////////////////////////////////////////////////////////
string EDSIMessage::findFormat(const string& str, int& idx) const
{
  string ret;
  int pos = str.find('%',idx);
  if (pos == string::npos) {
    idx = -1;
  } else {
    idx = pos;
    ret.append("%");
    if (str[idx+1] == 'l') {
      ret += str[idx+1];
      ret += str[idx+2];
    }  else {
      ret += str[idx+1];
    }
  }
  return ret;
}

//
// Add contents of some other stack to this stack.
//
void EDSIMessage::add(const EDSIMessage& stack)
{
  int stackSize = stack.p_stack->size();
  for (int idx=0; idx<stackSize; idx++) {
    p_stack->push_back((*stack.p_stack)[idx]);
    p_stack_keys->push_back((*stack.p_stack_keys)[idx]);
  }
}

void EDSIMessage::add(const char *key...)
{
  static char str[1024];

  string msg;
  string msgstackkey(key);
  

  int pos = getKey(key);

  if (pos >= 0) {
    msg = (*p_msgs)[pos];

    //  Rigamaro to get variable arguments into message
    if ( msg.length() > 0 ) {
      va_list ap;
      string type;
      int idx=0;
      va_start(ap, key);
      while ( !(type = findFormat(msg,idx)).empty() ) {

        // Create a string corresponding to the format and the data passed in
        if ( type == "%d" )       
          sprintf(str, "%d", va_arg(ap, int));
        else if ( type == "%ld" ) 
          sprintf(str, "%ld", va_arg(ap, long));
        else if ( type == "%f" )  
          sprintf(str, "%lf", va_arg(ap, double));
        else if ( type == "%lf" )
          sprintf(str, "%lf", va_arg(ap, double));
        else if ( type == "%c" ) {
          sprintf(str, "%c", va_arg(ap, int));
        } else if ( type == "%s" ) {
          // This is recursive and won't work so die the hard way!
          //EE_RT_ASSERT(va_arg(ap,char*),EE_UTIL|EE_WARNING,"Null pointer");
          char *cptr = va_arg(ap,char*);
          //cout << "cptr " << cptr << endl;
          if (cptr == (char*)0) {
            // If we get here, either the caller passed a 0 pointer for
            // a string or there weren't enough arguments on the stack.
            // The fatal assert will cause an abort.
            EE_RT_ASSERT(false, EE_FATAL,
                         "Null pointer passed in var args -- doomed to die!");
          } else {
            sprintf(str, "%s", cptr);
          }
        } else {
          string msg = "Invalid format type: '";
          msg += type;
          msg += "'";
          EE_RT_ASSERT(false, EE_FATAL, msg);
        }

        // Replace the printf()-like substring with the corresponding string
        msg = msg.replace(idx, type.length(), str);
        idx += strlen(str);
      }
      va_end(ap);

      p_stack->push_back(msg);
      p_stack_keys->push_back(msgstackkey);
      }
  } else {
    msg = "Undefined Message ID: ";
    msg += key;
    p_stack->push_back(msg);
    p_stack_keys->push_back("UNKNOWN");
  }

}




string EDSIMessage::getMessage() const
{
  string ret;
  vector<string>::iterator it = p_stack->begin();
  while (it != p_stack->end()) {
    ret += (*it);
    it++;
    if (it != p_stack->end()) ret += "\n";
  }
  return ret;
}



void EDSIMessage::load()
{
  static const char *path = "$ECCE_HOME/data/client/config/EDSI";
  if (p_keys == 0) {
    p_keys = new vector<string>;
    p_msgs = new vector<string>;

    // Use SFile class to interprer $ECCE_HOME
    SFile file(path);
    KeyValueReader reader(file.path(true).c_str());

    string key, text;
    while (reader.getpair(key,text)) {
      p_keys->push_back(key);
      p_msgs->push_back(text);
    }
  }
}

/*
/////////////////////////////////////////////////////////////////////////////
// Description
//   Returns an error message for the specified error code.  For now its
//   a giant switch statement.  Later we can read the messages from a
//   file and do fancy things like allowing printf type substitutions in
//   to the messages.
/////////////////////////////////////////////////////////////////////////////
string EDSIMessage::getMessage(EDSICode code)
{
  string ret;
  switch (code) {
    case RESOURCE_NOT_FOUND:
      ret = "The specified resource was not found on this server.";
      break;

    case NOT_COLLECTION:
      ret = "The specified resource is not a directory.";
      break;

    case NOT_REGULAR_FILE:
      ret = "The specified resource is not a regular file.";
      break;

    case NOT_IMPLEMENTED:
      ret = "The requested operation is not supported by this server.";
      break;

    default:
      ret = code + " - Unknown error code.";
      break;
  }
  return ret;
}
*/
