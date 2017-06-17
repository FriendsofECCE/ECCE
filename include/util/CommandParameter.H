/**
 * @file 
 *
 *
 */
#ifndef  COMMANDPARAMETER_HH
#define  COMMANDPARAMETER_HH

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "util/Any.H"


/**
 * Encapsulates the idea of a single parameter to a command.
 * Each parameter has both a name and a value.  The values are represented
 * as Any objects.  For convenience, forwarding functions are supplied 
 * for common Any operations.
 *
 * Parameter names are case sensitive.
 *
 * Two command parameters are equal if
 *
 * <ol>
 * <li> they have the same name
 * <li> they have the same type and value
 * </ol>
 *
 */
class CommandParameter
{
   public:

      CommandParameter(const string &name, const string& defaultValue);
      CommandParameter(const string &name, const char * defaultValue);
      CommandParameter(const string &name, int defaultValue);
      CommandParameter(const string &name, bool defaultValue);
      CommandParameter(const string &name, double defaultValue);
      CommandParameter(const string &name, const vector<int>& defaultValue);
      CommandParameter(const string &name, const vector<string>& defaultValue);

      virtual ~CommandParameter();

      bool  operator==(const CommandParameter& rhs);

      string getName() const;
      string toString() const;
      const Any&   getAny() const;

      int          getInteger() const throw(MismatchedTypeException);
      bool         getBoolean() const throw(MismatchedTypeException);
      double       getDouble() const throw(MismatchedTypeException);
      string       getString() const throw(MismatchedTypeException);
      vector<int> *getIntegerList() const throw(MismatchedTypeException);
      vector<string> *getStringList() const throw(MismatchedTypeException);

      void setInteger(int val);
      void setBoolean(bool val);
      void setDouble(double val);
      void setString(const string& val);
      void setIntegerList(const vector<int>& val);
      void setStringList(const vector<string>& val);
      void set(const string& value) throw(EcceException,MismatchedTypeException);


      int          getDefaultInteger() const throw(MismatchedTypeException);
      bool         getDefaultBoolean() const throw(MismatchedTypeException);
      double       getDefaultDouble() const throw(MismatchedTypeException);
      string       getDefaultString() const throw(MismatchedTypeException);
      vector<int> *getDefaultIntegerList() const throw(MismatchedTypeException);
      vector<string> *getDefaultStringList() const throw(MismatchedTypeException);

      void resetToDefault();

   protected:

   private:

      /** The name of this parameter. */
      string p_paramName;

      /** The current value. */
      Any    p_value;

      /** The default value. */
      Any    p_default;

};


#endif
