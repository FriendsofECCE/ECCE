/**
 *  @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "tdat/FFCategoryDesc.H"

/**
 *  Constructs an empty force field category description (FFCategoryDesc)
 *
 */
FFCategoryDesc::FFCategoryDesc()
{
   p_atomLengthLimit = 4;    // fallback default
}

/**
 *  Destructor that clears vectors.
 */
FFCategoryDesc::~FFCategoryDesc()
{
   p_parameters.clear();
   p_paramDisplayOrder.clear();
   p_atomRules.clear();
}

/**
 *  Registers the force field category name, eg, "Atoms", "Bonds". The name 
 *  is used to recognize the category tag lines in the input parameter files,
 *  e.g., amber.par file.
 *
 *  @param categoryName force field category name
 *
 */
void FFCategoryDesc::setName( const string& categoryName )
{
   p_name = categoryName;
}

/**
 *  Registers the force field category label, eg, "Atoms", "Bonds".  The
 *  label is used to label force field categories in the GUI.  The label 
 *  does not necessarily match the name.
 *
 *  @param categoryLabel force field category label for GUI
 *
 */
void FFCategoryDesc::setLabel( const string& categoryLabel)
{
   p_label = categoryLabel;
}

/**
 *  Registers the category's formula.  Variable names in the formula
 *  should match some of the parameter labels.
 *
 *  @param: formula category's formula (equation) used in the GUI
 *
 */
void FFCategoryDesc::setFormula( const string& formula)
{
   p_formula = formula;
}

/**
 *  Registers the category's multiline flag. In the GUI some categories
 *  have multiple lines of parameter values, other do not.
 *
 *  @param: flag value of false indicates single line of parameters
 *  in GUI, true indicates multiple lines of parameters are required.
 */
void FFCategoryDesc::setMultiLine( bool flag)
{
   p_multiLine = flag;
}

/**
 *  Registers the number of atoms in the type name of this category.  For 
 *  example, the bond category has two atoms in a type name such as "CT-H" 
 *  where CT and H are atom types (only one atom per name).
 *
 *  @param numAtoms number of atoms in this category's type name
 */
void FFCategoryDesc::setNumAtoms( int numAtoms)
{
   p_atomCount = numAtoms;
}

/**
 *  Registers the display order of the parameters.  The display order may 
 *  not match the order that the parameters are passed in.  For example, 
 *  if there are three parameters and the second parameter listed in the 
 *  parameter file (amber.par) should be displayed first and the first 
 *  parameter should be displayed second, the display order would be 1, 0, 2.
 *
 *  @param order vector containing the indices of the parameters arranged in the
 *  order they are to be used for display.
 */
void FFCategoryDesc::defineDisplayOrder( const vector<int>& order)
{
   p_paramDisplayOrder = order;
}

/**
 *  Registers a parameter description for this category
 *
 *  @param pDesc A single instance of a force field parameter description  
 *  (FFParameterDesc) that describes the attributes of a category parameter.
 */
void FFCategoryDesc::addParamDesc( FFParameterDesc& pDesc)
{
   p_parameters.push_back( pDesc);
}

/**
 *  Registers an entire vector of force field parameter descriptions
 *  (FFParameterDesc) that each describe the attributes of a category
 *  parameter.
 *
 *  @param: pDescs vector of FFParameterDesc describing the attributes of the 
 *  category parameter
 */
void FFCategoryDesc::setParamDescs( const vector< FFParameterDesc>& pDescs)
{
   p_parameters = pDescs;
}

/**
 *  Set validation rules for this force field category. The rules apply 
 *  across the force field types (FFType) of this category and are 
 *  represented as a bit mask where 1=required, 0=optional.  There is a 
 *  bit mask for every allowable configuration of required/optional. For 
 *  example, if you have an angle class, there may be only two valid 
 *  combinations represented as:  101, 111.  In this case, the first and
 *  third atoms are requred, the second is optional. 
 *
 *  @param rules vector of boolean values for validating force field types
 *  by name.
 */
void FFCategoryDesc::setAtomRules(const vector< vector< bool> >& rules)
{
   p_atomRules = rules;
}

/** 
 *  Set maximum length for the number of characters used to represent a 
 *  force field type name.  This is used to validate type names. This 
 *  could be set per force field type (FFType) in the category but for 
 *  now we just have one maximum for all of them until we can identify 
 *  the need for more complexity.
 *
 *  @param max maximum length of the force field type names.
 */
void FFCategoryDesc::setMaxAtomLength( int max)
{
   p_atomLengthLimit = max;
}


////////////////////////////////////////////////////////////////////////////////
// Description: Prints this category description to standard out 
//
////////////////////////////////////////////////////////////////////////////////
void FFCategoryDesc::print()
{
   cout << "Category name is " << p_name.c_str();
   cout << ", label is " << p_label.c_str();
   cout << ", the formula is " << p_formula.c_str() << endl;

   vector<string> labels;
   getParamLabels( labels);
   int i=0;
   vector<string>::iterator p = labels.begin();
   while ( p != labels.end())  {
      cout << "   label[" << i++ << "] is " << (*p).c_str() << endl;
      p++;
   }
}

/**
 *  Retrieve the force field category name
 *
 *  @return category name
 *  @see setName()
 */
string FFCategoryDesc::getName() const
{
   return p_name;
}

/**
 *  Retrieve the force field category label
 *
 *  @return category label
 *  @see setLabel()
 */
string FFCategoryDesc::getLabel() const
{
   return p_label;
}

/**
 * Retrieves the index of the parameter with the specified label.
 *
 * @param paramLabel label of parameter 
 *
 * @return index of parameter with matching label
 */
int FFCategoryDesc::getParamIDByLabel( string paramLabel) 
                                                 throw (InvalidException) 
{
   int index = -1;

   for (int i=0;i<p_parameters.size();i++)  {
      if ( paramLabel.compare( p_parameters[i].getLabel().c_str()) == 0)  {
         index = i;
	 break;
      }
   }
   if ( index == -1)
      throw InvalidException("Failed to find parameter label matching " +
      			paramLabel, WHERE);
   return index;
}

/**
 *  Retrieve the force field category formula
 *
 *  @return category formula
 *  @see setFormula()
 */
string FFCategoryDesc::getFormula() const
{
   return p_formula;
}

/**
 *  Retrieve the force field category's multiline flag.
 *
 *  @return  multiline flag for this category.  true indicates a single
 *  line of parameters is required in the GUI, false indicates that multiple
 *  lines of parameters may be required in the GUI.
 *  @see setMultiLine()
 */
bool FFCategoryDesc::getMultiLine() const
{
   return p_multiLine;
}

/**
 *  Retrieve the display order of the category parameters
 *
 *  @return vector of parameter indices giving the display order of 
 *  the parameters (in the GUI).
 *  @set setDisplayOrder()
 */
vector< int> FFCategoryDesc::getDisplayOrder() const
{
   return p_paramDisplayOrder;
}

/** 
 *  Retrieve the number of parameters for this force field category
 *
 *  @return count of category parameters
 */
int FFCategoryDesc::getNumParams() const
{
   return p_parameters.size();
}

/**
 *  Retrieve the parameter descriptions.
 *
 *  @return vector of the category's parameter descriptions
 *  @see setParamDescs();
 */
const vector<FFParameterDesc>& FFCategoryDesc::getParamDescs() const
{
   return p_parameters;
}

/**
 *  Retrieve this category's parameter labels.
 *
 *  @return vector of the category's parameter labels
 */
void FFCategoryDesc::getParamLabels( vector<string>& labels)
{
   labels.clear();
   vector< FFParameterDesc>::iterator piter = p_parameters.begin();
   while ( piter != p_parameters.end())  {
      labels.push_back((*piter).getLabel());
      piter++;
  }
}

/**
 *  Retrieve the number of atoms in the force field category type name
 *
 *  @return count of the atoms in a catergory type name (see setNumAtoms())
 *  @see setNumAtoms();
 */
int FFCategoryDesc::getNumAtoms() const
{
   return p_atomCount;
}

/**
 *  Retrieve the number of atom rules
 *
 *  @return count of atom rules
 *  @see setAtomRules()
 */
int FFCategoryDesc::getNumAtomRules() const
{
   return p_atomRules.size();
}

/**
 *  Retrieve atom rules for the force field category type (FFType) names
 *
 *  @return atom rules for names of individual category types
 *  @see setAtomRules()
 */
vector< vector< bool> > FFCategoryDesc::getAtomRules() const
{
   return p_atomRules;
}

/**
 *  Retrieve maximum number of characters in force field type name for
 *  this category. 
 *
 *  @return maximum number of characters in type name.
 *  @see setMaxAtomLength()
 */
int FFCategoryDesc::getMaxAtomLength() const
{
   return p_atomLengthLimit;
}
