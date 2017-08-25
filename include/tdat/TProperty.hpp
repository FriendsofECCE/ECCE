/**
 * @file
 *
 *
 */


#ifndef TPROPERTY_HH
#define TPROPERTY_HH

#include <iostream>
  using std::ostream;

#include <vector>
  using std::vector;
#include <string>
  using std::string;



/**
 * TProperty Objects Represent Discrete Results from Calculations
 * This version removed all object store references and removed
 * several data members that were not used.
 *
 * Class TProperty is an abstract class that
 * has a very broad subclass layer (Vector, Table, Value, TSVector,
 * TSVecString, TSVecTable, TSVecVector) where the real properties
 * are stored.
 *
 * TProperty contains the information common to all properties (type,
 * units, classType).
 *
 * TProperty subclasses are transient classes that store property
 * values in memory while an application is running.  Creating a
 * TProperty is done through the PropFactory interface.  Making a
 * TProperty persistent is done through the ICalculation interface.
 * Constructors are protected so no TProperty can be instantiated
 * directly.
 */ 
class TProperty
{

public:
  /**
   *  Indicates which subclass is instantiated.  
   *  Currently 2 types of subclasses exist:<br>
   *  <ol>
   *    <li>Write Once: These types are not updated one unit
   *                    at a time.</li>
   *    <li>Write Many: These can append one element at a time back
   *                    to back (TS stands for time-stepped)</li>
   *  </ol>
   */
  enum ClassType {
     PROPVALUE,            /**< Write Once - single value */
     PROPSTRING,           /**< Write Once - single string value */
     PROPVECTOR,           /**< Write Once - 1-d vector of values */
     PROPVECSTRING,        /**< Write Once - 1-d vector of strings */
     PROPTABLE,            /**< Write Once - 2-d table of values  */
     PROPVECTABLE,         /**< Write Once - 3-d table of values */
     PROPVECGRID,          /**< Write Once - vector of grids */
     PROPTSTABLE,          /**< Write Many - 3-d vector of tables */
     PROPTSVECTABLE,       /**< Write Many - 3-d vector of tables */
     PROPTSVECSTRING,      /**< Write Many - 1-d vector of strings */
     PROPTSVECTOR,         /**< Write Many - 1-d vector of values */
     PROPTSVECVECTOR,      /**< Write Many - 2-d vector of vectors */
     SUPER                 /**< Reserved case, indicating that the parent
                                class should never be instantiated. */
  };

  static string   classTypeToStr (TProperty::ClassType classType);
  static TProperty::ClassType   strToClassType (const string& str);
  virtual ~TProperty(void);
  TProperty& operator=(const TProperty& property);
  bool operator==(const TProperty& property) const;
  bool operator!=(const TProperty& property) const;
  string type(void) const;
  string units(void) const;
  TProperty::ClassType classType(void) const;
  virtual bool canScalarize(void) const;
  virtual double scalarize(void) const;
  void type(const string& type);
  void units(const string& units);


protected:
  TProperty(const string& type, const string& units);
  TProperty(const TProperty& property);
  TProperty::ClassType p_classType;    // which subclass is instantiated

private:
  string    p_type;   /**< The property type.  Used by PropFactory
                       *  to instantiate the correct subclass. A
                       *  property type must be defined in the 
                       *  config/properties file - p_type is referred to as
                       *  a "short name" in the properties file.
                       */

  string    p_units;  /**< Units for the property class */

  friend ostream& operator<< (ostream& os, const TProperty& property);
  friend class PropFactory;  // so it can construct properties

};

#endif
