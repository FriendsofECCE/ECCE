/**
 * @file 
 *
 *
 */
#ifndef  ANY_HH
#define  ANY_HH

#include <string>
#include <vector>

using std::string;
using std::vector;


#include "util/MismatchedTypeException.H"

/**
 * This class represents a single single value that can be one of many types.
 * It is based on the CORBA definition of Any.
 * An Any object consists of two parts:
 *
 *   <li> a data value
 *   <li> a TypeCode describing the type of the data value contained
 *        in the Any object. 
 *
 * A large part of the Any class consists of pairs of methods for 
 * inserting values into and extracting values from an Any object. 
 *
 * Two Any objects are equal if both the type and value are equal.
 *
 * The public interface uses explicitly named getters and setters
 * since that is clear and straightforward to python and because it
 * seems clearer to the reader in this case too.
 */
class Any
{
   public:

      enum TypeCode { aUnknown=-1,
                      aInt, 
                      aDouble, 
                      aBool, 
                      aString, 
                      aIntList, 
                      aStringList };

      Any();
      Any(bool value);
      Any(int value);
      Any(double value);
      Any(const char *value);
      Any(const string& value);
      Any(const vector<int>& defaultValue);
      Any(const vector<string>& defaultValue);
      Any(const Any& rhs);

      virtual ~Any();

      bool  operator==(const Any& rhs);

      TypeCode getTypeCode() const;

      // For serialization
      string    getTypeCodeAsString() const;
      static    TypeCode getTypeCodeFromString(const string& stype);
      string    toString() const;

      int             getInteger() const throw(MismatchedTypeException);
      bool            getBoolean() const throw(MismatchedTypeException);
      double          getDouble() const throw(MismatchedTypeException);
      string          getString() const throw(MismatchedTypeException);
      vector<int>    *getIntegerList() const throw(MismatchedTypeException);
      vector<string> *getStringList() const throw(MismatchedTypeException);

      void setInteger(int val);
      void setBoolean(bool val);
      void setDouble(double val);
      void setString(const string& val);
      void setIntegerList(const vector<int>& val);
      void setStringList(const vector<string>& val);

   protected:

      void cleanValue(void *val);
      void set(void **what, const string& val);
      void set(void **what, int val);
      void set(void **what, double val);
      void set(void **what, const vector<int>& val);
      void set(void **what, const vector<string>& val);
      void set(void **what, bool val);

   private:

      /** The type of value. */
      TypeCode p_type;

      /** The value. */
      void    *p_value;

};


#endif
