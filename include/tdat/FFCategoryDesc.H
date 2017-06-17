/**
 * @file
 *
 *
 */

// CLASS SYNOPSIS: Contains information needed to describe a force field 
//                 category.
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FF_CATEGORY_DESC_HH
#define FF_CATEGORY_DESC_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "tdat/FFParameterDesc.H"
#include "util/InvalidException.H"

/**
 *  The Force Field Category Description (FFCategoryDesc) class contains 
 *  standard (and standardized) information about force field categories 
 *  necessary primarily for the display of the information but also for
 *  internal processing. This information is maintained in an .xml file.
 *  The intent of separting this information from the code was to minimize,
 *  if not eliminate, hardcoded force field category information.
 *
 *  @see FFDescXMLSerializer
 *  @see FFCategory
 *  @see FFModel
 */
class FFCategoryDesc  {

   public:

      FFCategoryDesc( void);
      virtual ~FFCategoryDesc( void);

      //-- Accessors -----------------------------------------------------------
            string                    getName() const ;
            string                    getLabel() const ;
            string                    getFormula() const ;
            bool                      getMultiLine() const ;
            vector<int>               getDisplayOrder() const;
      const vector<FFParameterDesc>&  getParamDescs() const;
            void                      getParamLabels( vector<string>&);
            int                       getNumParams() const;
            int                       getNumAtoms() const;
            int                       getNumAtomRules() const;
            int                       getMaxAtomLength() const;
            vector< vector<bool> >    getAtomRules() const;
            int                       getParamIDByLabel( string) throw (InvalidException);
            void                      print();

      //-- Modifiers -----------------------------------------------------------
            void  setName( const string& categoryName);
            void  setLabel( const string& categoryLabel);
            void  setFormula( const string& formula);
            void  setNumAtoms( int numAtoms);
            void  setMultiLine( bool flag);
            void  defineDisplayOrder( const vector< int>& order);
            void  addParamDesc( FFParameterDesc& pDesc);
            void  setParamDescs( const vector< FFParameterDesc>& pDescs);
            void  setAtomRules( const vector< vector<bool> >& rules);
            void  setMaxAtomLength(int max);

   private:

      string                   p_name;             // category name for i/o file
      string                   p_label;            // category label for GUI
      string                   p_formula;          // formula for GUI
      vector< FFParameterDesc> p_parameters;       // parameter descriptors
      vector< int>             p_paramDisplayOrder;// order to display params
      int                      p_paramCount;       // number of parameters per row
      int                      p_atomCount;        // number of atoms in type name
      vector< vector<bool> >   p_atomRules;        // rules for new type name
      int                      p_atomLengthLimit;  // max chars for atom value
      bool                     p_multiLine;        // multiple lines of parameters

};

#endif // FF_CATEGORY_DESC_HH
