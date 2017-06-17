/////////////////////////////////////////////////////////////////////////////
// Design
//   This is a very simple message class intended for use with the EDSI
//   component. It is similar but smaller than the ErrMessage class.  A
//   key difference is that the message stack is not static - this class
//   does not follow the singleton pattern.
/////////////////////////////////////////////////////////////////////////////
#ifndef _EDSIMESSAGE_H
#define _EDSIMESSAGE_H

#include <string>
#include <vector>
  using std::string;
  using std::vector;

class EDSIMessage 
{
   public: 
      EDSIMessage();
      virtual ~EDSIMessage();

      void clear();
      void add(const char * key...);
      void add(const EDSIMessage& stack);  // merge in another stack
      bool messages(); 

      int  size() const;

      // Returns a string of the current stack.  Each entry separated by a
      // linefeed.
      string getMessage() const;
      bool findKey( const char* key) const;

   protected:

      // For other message sets, subclass and re-implement the load method.
      virtual void load();
      int  getKey(const char *key) const;
      string findFormat(const string& str, int& idx) const;

      vector<string> *p_stack;
      vector<string> *p_stack_keys;

      // This should probably be an associative structure but I'm not
      // familiar with STL enough and don't want to take time now.
      static vector<string> *p_keys;
      static vector<string> *p_msgs;
};

#endif
