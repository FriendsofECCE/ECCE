//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TGaussianBasisSet.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
using std::cout;
using std::endl;

#include "util/ErrMsg.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSGroup.H"
#include "dsm/JCode.H"

const int TGaussianBasisSet::maxShell = (int)TGaussianBasisSet::k_shell;

// Static conversion arrays:

// converts AngularMomentum enum value to it's corresponding character:
const char* TGaussianBasisSet::shell_formatter[] = {
        "S","P","D","F","G","H","I","K"
};

// converts GBSType enum value to it's corresponding string:
const char* TGaussianBasisSet::gbs_type_formatter[] = {
        // Orbital basis set types
        "pople",
        "other_segmented",
        "correlation_consistent",
        "other_generally_contracted",
        "ECPOrbital",            
        "DFTOrbital",
      
        // Augmenting basis set types
        "polarization",
        "diffuse",
        "rydberg",
        "ecp",      // Auxilliary to ECPOrbital
        "Exchange", // Auxilliary to DFTOrbital
        "Charge",   // Auxilliary to DFTOrbital
  
        "UnknownGBSType",
        "appended_tight",
        "appended_diffuse"
};
  
const char* TGaussianBasisSet::contractionType_formatter[] = {
        "AnyConType", "Uncontracted", "Segmented", "GC", "UnknownContType"
};
  
const char* TGaussianBasisSet::gbsCategory_formatter[] = {
        "ORBITAL", "AUXILIARY"
};
  
const char* TGaussianBasisSet::coordSys_formatter[] = {
        "N", "Y", "UnknownCoordSys"
};
 

//// Constructors ////

TGaussianBasisSet::TGaussianBasisSet() 
{
  p_category = TGaussianBasisSet::ORBITAL;  // should this default to AUX?
  p_atomicOrbital = ""; // what is this data member for??
  p_coordSys = TGaussianBasisSet::UnknownCoordSys;
  p_contType = TGaussianBasisSet::UnknownContType;
  p_type = TGaussianBasisSet::UnknownGBSType;
  p_name = "unknownName";
}

TGaussianBasisSet::TGaussianBasisSet(GBSType& t, string& n)
{
  p_type = t; 
  p_name = n;
  p_category = TGaussianBasisSet::ORBITAL;
  p_atomicOrbital = "";
  p_coordSys = TGaussianBasisSet::UnknownCoordSys;
  p_contType = TGaussianBasisSet::UnknownContType;
}

/*******************************************************************
 Method : copy constructor
 Summary:
*******************************************************************/
TGaussianBasisSet::TGaussianBasisSet(const TGaussianBasisSet& gbs)
{
  ContractionMap::const_iterator mapIt;

 // Meta data  (Do we need this stuff?)
    // atoms = gbs.atoms; do we need this?
    p_imageFile = gbs.p_imageFile;
    p_references = gbs.p_references;
    p_info = gbs.p_info;
    p_comments = gbs.p_comments;

 // Data
    p_category = gbs.p_category;
    p_atomicOrbital = gbs.p_atomicOrbital;
    p_coordSys = gbs.p_coordSys;
    p_contType = gbs.p_contType;
    p_type = gbs.p_type;
    p_name = gbs.p_name;
    
 // map of ContractionSets:
    for(mapIt = gbs.p_contractions.begin(); mapIt != gbs.p_contractions.end();
	mapIt++) 
    {
      if ((*mapIt).second != (ContractionSet*) 0)
	p_contractions[(*mapIt).first] = 
	   new ContractionSet(*((*mapIt).second));
      else
	p_contractions[(*mapIt).first] = (ContractionSet*) 0;
    }
}

/*******************************************************************
 Method : special copy constructor
 Summary: This copy constructor only copies the ContractionSet for the
          given element.  In addition, it will optimize the contractions
          depending upon the contraction type of the basis set
          and the code type.  It uses the same logic for optimization
	  as the TGBSConfig::dump method.

          This function is used in ComputeMO for normalizing the
	  basis sets.
*******************************************************************/
TGaussianBasisSet::TGaussianBasisSet(const TGaussianBasisSet& gbs,
				     string element_,
				     bool optimize,
				     const JCode* code,
                                     ContractionInfo* contInfo)
{
  // Note that this constructor doesn't copy the meta data,
  // as it won't be needed:

  // Data
  p_category = gbs.p_category;
  p_atomicOrbital = gbs.p_atomicOrbital;
  p_coordSys = gbs.p_coordSys;
  p_contType = gbs.p_contType;
  p_type = gbs.p_type;
  p_name = gbs.p_name;
  
 // map of ContractionSets (only copy the desired element):
 // USE LOGIC FOR OPTIMIZING CONTRACTIONS:
  ContractionMap::const_iterator mapIt;
  
  mapIt = gbs.p_contractions.find(element_);
  
  if (mapIt != gbs.p_contractions.end())
  {
    ContractionSet* contSet;
    
    // Find out if the code supports general contractions:
    bool gcSupported;
    code->get_bool("GeneralContractions", gcSupported);

    // For segmented and unknown contraction types, don't optimize
    if (p_contType == TGaussianBasisSet::Segmented ||
	p_contType == TGaussianBasisSet::UnknownContType)
      contSet = (*mapIt).second->clone_normal(contInfo);
    
    // If uncontracted, optimize contractions
    else if (p_contType == TGaussianBasisSet::Uncontracted)
      contSet = (*mapIt).second->clone_optimizeGC(contInfo);
    
    else if (p_contType == TGaussianBasisSet::GC)
    {
      if (optimize == false)
      {
	// If gc supported by code, don't optimize
	if (gcSupported)
          contSet = (*mapIt).second->clone_normal(contInfo);

	else // consolidate contractions in a different way than optimize
	  contSet = (*mapIt).second->clone_noop_gcNotSupported(contInfo);
      }
      else // optimize is true, so go ahead and optimize
	contSet = (*mapIt).second->clone_optimizeGC(contInfo);
    }
    // Add the new ContractionSet to the map:
    p_contractions[(*mapIt).first] = contSet;
  }
}

/*******************************************************************
 Method : destructor
 Summary:
*******************************************************************/
  TGaussianBasisSet::~TGaussianBasisSet(void)
{
  ContractionMap::iterator mapIt;

  for(mapIt = p_contractions.begin(); mapIt != p_contractions.end();
      mapIt++) 
    if ((*mapIt).second != (ContractionSet*) 0)
      delete (*mapIt).second;
}


//// STATIC METHODS ////

/*******************************************************************
 Method : strToType
 Summary: 
*******************************************************************/
TGaussianBasisSet::GBSType TGaussianBasisSet::strToType(string type)
{
  TGaussianBasisSet::GBSType gbsType;
  if (type == "pople")
    gbsType = TGaussianBasisSet::pople;
  else if (type == "other_segmented")
    gbsType = TGaussianBasisSet::other_segmented;
  else if (type == "correlation_consistent")
    gbsType = TGaussianBasisSet::correlation_consistent;
  else if (type == "other_generally_contracted")
    gbsType = TGaussianBasisSet::other_generally_contracted;
  else if (type == "polarization")
    gbsType = TGaussianBasisSet::polarization;
  else if (type == "diffuse")
    gbsType = TGaussianBasisSet::diffuse;
  else if (type == "rydberg")
    gbsType = TGaussianBasisSet::rydberg;
  else if (type == "ECPOrbital")
    gbsType = TGaussianBasisSet::ECPOrbital;
  else if (type == "ecp")
    gbsType = TGaussianBasisSet::ecp;
  else if (type == "DFTOrbital")
    gbsType = TGaussianBasisSet::DFTOrbital;
  else if (type == "exchange" || type == "Exchange")
    gbsType = TGaussianBasisSet::exchange;
  else if (type == "charge" || type == "Charge")
    gbsType = TGaussianBasisSet::charge;
  else if (type == "UnknownGBSType")
    gbsType = TGaussianBasisSet::UnknownGBSType;
  else if (type == "appended_tight") 
    gbsType = TGaussianBasisSet::appended_tight;
  else if (type == "appended_diffuse") 
    gbsType = TGaussianBasisSet::appended_diffuse;
  else {
    string msg = "***Unrecognized gbs type: " + type;
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }

  return gbsType;
}

/*******************************************************************
 Method : stringToShell
 Summary: 
*******************************************************************/
TGaussianBasisSet::AngularMomentum TGaussianBasisSet::charToShell(char ch)
{
  TGaussianBasisSet::AngularMomentum shell;
  if (ch == 'S' || ch == 's')
    shell = TGaussianBasisSet::s_shell;
  else if (ch == 'P' || ch == 'p')
    shell = TGaussianBasisSet::p_shell;
  else if (ch == 'D' || ch == 'd')
    shell = TGaussianBasisSet::d_shell;
  else if (ch == 'F' || ch == 'f')
    shell = TGaussianBasisSet::f_shell;
  else if (ch == 'G' || ch == 'g')
    shell = TGaussianBasisSet::g_shell;
  else if (ch == 'H' || ch == 'h')
    shell = TGaussianBasisSet::h_shell;
  else if (ch == 'I' || ch == 'i')
    shell = TGaussianBasisSet::i_shell;
  else if (ch == 'K' || ch == 'k')
    shell = TGaussianBasisSet::k_shell;
  else {
    string msg = "***Unrecognized func type: " + ch;
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }

  return shell;
}

/*******************************************************************
 Method : stringToContType
 Summary: 
*******************************************************************/
TGaussianBasisSet::ContractionType 
                   TGaussianBasisSet::strToContType(string ct)
{
  TGaussianBasisSet::ContractionType contType;
  if (ct == "AnyConType")
    contType = TGaussianBasisSet::AnyConType;
  else if (ct == "Uncontracted")
    contType = TGaussianBasisSet::Uncontracted;
  else if (ct == "Segmented")
    contType = TGaussianBasisSet::Segmented;
  else if (ct == "GC")
    contType = TGaussianBasisSet::GC;
  else if (ct == "UnknownContType")
    contType = TGaussianBasisSet::UnknownContType;
  else
    EE_RT_ASSERT(false, EE_WARNING, "Unrecognized contraction type");
  return contType;
}

/*******************************************************************
 Method : stringToCoordSys
 Summary: 
*******************************************************************/
TGaussianBasisSet::CoordinateSystem 
                   TGaussianBasisSet::strToCoordSys(string cs)
{
  TGaussianBasisSet::CoordinateSystem coordSys;
 
  if (cs == "Y")
    coordSys = TGaussianBasisSet::Spherical;
  else if (cs == "N")
    coordSys = TGaussianBasisSet::Cartesian;
  else if (cs == "UnknownCoordSys")
    coordSys = TGaussianBasisSet::UnknownCoordSys;
  else
    EE_RT_ASSERT(false, EE_WARNING, 
                 "Unrecognized designation for spherical coordinate system");
  return coordSys;
}

/*******************************************************************
 Method : stringToCategory
 Summary: 
*******************************************************************/
TGaussianBasisSet::GBSCategory TGaussianBasisSet::strToCategory(string cat)
{
  TGaussianBasisSet::GBSCategory category;
 
  if (cat == "ORBITAL")
    category = TGaussianBasisSet::ORBITAL;
  else if (cat == "AUXILIARY")
    category = TGaussianBasisSet::AUXILIARY;
  else
    EE_RT_ASSERT(false, EE_WARNING, "Unrecognized category");
  return category;
}

//// ACCESSORS ////

/*******************************************************************
 Method : maxLValue
 Summary: Returns the largest shell represented by basis set
          functions for the given element.
*******************************************************************/
TGaussianBasisSet::AngularMomentum 
  TGaussianBasisSet::maxlvalue(string elementID) const
{
  TGaussianBasisSet::AngularMomentum result = TGaussianBasisSet::s_shell;

  // check if the basis set supports the given element
  ContractionMap::const_iterator it = p_contractions.find(elementID);

  if(it != p_contractions.end())
  {
    ContractionSet* contSet = (*it).second;

    // loop thru all the contractions in the set associated w/ that element
    for (size_t i = 0; i < contSet->size(); i++)
    {
      // find the largest shell in the list of shells for each contraction 
      for(size_t s = 0; s < (*contSet)[i]->shells.size(); s++) 
	      if ((*contSet)[i]->shells[s] > result)
	        result = (*contSet)[i]->shells[s];
    }
  }

  return result;
}

/*******************************************************************
 Method : useSpherical
 Summary: Returns true if the basis set is designed for spherical 
          coordinates AND there are basis functions for the tag that
	  are d functions (or higher).  The coordinate system is not
	  relevent to s and p functions.  If the tag is not supported,
	  false is returned.
*******************************************************************/
bool TGaussianBasisSet::useSpherical(string elementID) const
{
  bool result = false;

  // first check if element is supported
  ContractionMap::const_iterator it = p_contractions.find(elementID);
  if(it != p_contractions.end())
    if (p_coordSys == TGaussianBasisSet::Spherical &&
	maxlvalue(elementID) > TGaussianBasisSet::p_shell)
      result = true;

  return result;
}

/*******************************************************************
 Method : getUniqueKey
 Summary: You can't just look up a basis set based on its name
          because some basis sets (like LANL ECP) have the same
	  name for both the orbital and the ecp basis set.  Therefore,
	  to do a unique search, you have to combine the name and 
	  the basis set type into a string.
*******************************************************************/
string TGaussianBasisSet::getUniqueKey(void) const 
{
  string key = p_name + ":" +gbs_type_formatter[p_type];
  return key;
}

/*******************************************************************
 Method : num_contracted_sets
 Summary: Returns the number of contraction sets for the particular
          atom.  Each atom corresponds to its own vector of 
          contractions.  The vector has one contraction per shell
          supported.  The vector is related to the atom name using
          the p_contractions map.  The size of the vector represents
          the number of contracted sets.  
*******************************************************************/
size_t TGaussianBasisSet::num_contracted_sets(const char* elementID) const
{
  size_t result = 0;
  ContractionMap::const_iterator it = p_contractions.find(elementID);
  if(it != p_contractions.end())
     result = (*it).second->size();

  return result;
}

/*******************************************************************
 Method : exponents
 Summary: Returns a vector of exponents used in the functions for
          the given atom and shell(i.e., ics).  The atom is the key to the map,
          and the ics is the index to the vector returned from the map.
*******************************************************************/
vector<double> TGaussianBasisSet::exponents(const char* elementID, 
        size_t ics /* index of contracted set */) const
{
  vector<double> results;
  ContractionMap::const_iterator it = p_contractions.find(elementID);
  if (it == p_contractions.end()) {
    EE_ASSERT(false, EE_WARNING, "Atom ID not found in GBS map");
  } else if ((*it).second == 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Null pointer for contractions vector");
  } else if (ics >= (*(*it).second).size() || ics < 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Invalid contraction index");
  } else {
    Contraction_* cont = (*(*it).second)[ics];
    for (size_t i = 0; i < cont->num_exponents; i++)
      results.push_back(cont->exponents[i]);
  }

  return results;
}

/*******************************************************************
 Method : coeffiecients
 Summary: Returns a vector of coefficients used in the functions for
          the given atom ans shell (i.e., ics).  The atom is the key 
          to the map, and the ics is the index to the vector returned 
          from the map.
*******************************************************************/
vector<double> TGaussianBasisSet::coefficients(const char* atomID,
        size_t ics /* index of contracted set */) const
{
  vector<double> results;
  ContractionMap::const_iterator it = p_contractions.find(atomID);
  if (it == p_contractions.end()) {
    EE_ASSERT(false, EE_WARNING, "Atom ID not found in GBS map");
  } else if ((*it).second == 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Null pointer for contractions vector");
  } else if (ics >= (*(*it).second).size() || ics < 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Invalid contraction index");
  } else {
    Contraction_* cont = (*(*it).second)[ics];
    for (size_t i = 0; i < cont->num_coefficients * cont->num_exponents; i++)
      results.push_back(cont->coefficients[i]);
  }
  return results;
}

/*******************************************************************
 Method : func_types
 Summary: Returns a vector of enumerated types representing each
          shell included in the contraction.  atomID gets the contraction
          list for the designated atom.  ics indexes the right
          contraction from the contraction list.
*******************************************************************/
vector<TGaussianBasisSet::AngularMomentum>
       TGaussianBasisSet::func_types(const char* atomID, size_t ics) const
{
  vector<TGaussianBasisSet::AngularMomentum> results;
  ContractionMap::const_iterator it = p_contractions.find(atomID);
  if (it == p_contractions.end()) {
    EE_ASSERT(false, EE_WARNING, "Atom ID not found in GBS map");
  } else if ((*it).second == 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Null pointer for contractions vector");
  } else if (ics >= (*(*it).second).size() || ics < 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Invalid contraction index");
  } else {
    Contraction_* cont = (*(*it).second)[ics];

    return cont->shells;
  }

  return results;
}

/*******************************************************************
 Method : getContraction() 
 Summary: Returns the indexed contraction from the given atom's
          contraction vector.
	  Returns null on error.
*******************************************************************/
Contraction_* TGaussianBasisSet::getContraction (const char* atomID, 
					       size_t ics /* index of contracted set */) const
{
  Contraction_* result = 0;
  ContractionMap::const_iterator it = p_contractions.find(atomID);
  if (it == p_contractions.end()) {
    EE_ASSERT(false, EE_WARNING, "Atom ID not found in GBS map");
  } else if ((*it).second == 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Null pointer for contractions vector");
  } else if (ics >= (*(*it).second).size() || ics < 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Invalid contraction index");
  } else  {
    result = (*(*it).second)[ics];
  }

  return result;
}


///// CONTRACTION METHODS /////

/*******************************************************************
 Method : default constructor
 Summary:
*******************************************************************/
Contraction_::Contraction_(void)
{ 
  // shells vector is empty

  num_exponents = 0;
  exponents = (double*) 0;

  num_coefficients = 0;
  coefficients = (double*) 0;

  // ECP stuff:
  l = 0;
  powers = (int*) 0;
}

/*******************************************************************
 Method : copy constructor
 Summary:
*******************************************************************/
Contraction_::Contraction_(Contraction_& cont)
{ 
  for (size_t idx = 0; idx < cont.shells.size(); idx++)
    shells.push_back(cont.shells[idx]);

  // copy exponents
  num_exponents = cont.num_exponents;
  if (cont.exponents != (double*) 0)
  {
    exponents = new double[num_exponents];
    for (size_t i = 0; i < num_exponents; i++)
      exponents[i] = cont.exponents[i];
  }
  else exponents = (double*) 0;
  // copy coefficients
  num_coefficients = cont.num_coefficients;
  if (cont.coefficients != (double*) 0)
  {
    coefficients = new double[num_coefficients * num_exponents];
    for (size_t i = 0; i < num_exponents * num_coefficients; i++)
      coefficients[i] = cont.coefficients[i];
  }
  else
    coefficients = (double*) 0;
  // ECP stuff only:
  l = cont.l;
  if (cont.powers != (int*) 0) 
  {
    powers =  new int[num_exponents];
    for (size_t i = 0; i < num_exponents; i++)
      powers[i] = cont.powers[i];
  }
  else
    powers = (int*) 0;

}

/**
 * Destructor.
 */
Contraction_::~Contraction_()
{
  if (exponents != (double*) 0) delete [] exponents;
  if (coefficients != (double*) 0) delete [] coefficients;
  if (powers != (int*) 0) delete [] powers;
}

/**
 * Returns a single coefficient for the contraction as indexed
 * by the row and column.
 */
double Contraction_::coefficient (size_t row, size_t col) const
{
  double result = 0;
  if (row < 0 || row >= num_exponents || col < 0 || col >= num_coefficients) {
    EE_RT_ASSERT(false, EE_WARNING, "Invalid coefficient index");
  } else {
    int index = row * num_coefficients + col;
    result = coefficients[index];
  }
  return result;
}


/**
 * Create bitvector that has a value of true for columns that are printable
 * (ie will not be optimized out) when optimizing general contraction.
 * Pritable columns are columns that do not consist of all zeros with one
 * non-zero value.
 */
vector<bool> Contraction_::getPrintableColumns() const
{
   int numExp = num_exponents;
   int numCoef = num_coefficients;

   vector<bool> printableCol(numCoef, true);
   for (int c = 0; c < numCoef; c++) {
      int numZeros = 0;
      for (int r = 0; r < numExp; r++) {
         if (coefficients[c + (numCoef*r)] == 0.0) {
            numZeros++;
         }
      }
      if (numExp > 1 && (numZeros == (numExp-1))) printableCol[c] = false;
   }

   return printableCol;
}

/**
 * Determines if the specified row can be optimized into its own contraction
 * For this to be true, the column must contain all 0's except for one
 * non-zero value (a nonprintable column) and for the nonprintable columns,
 * only one row value should be non-zero.
 *
 * This algorithm will break if a column is determined to be nonprintable
 * yet the row contains non-zero values for more than one of these
 * columns.
 * @return -1 if not optimizable or index of coeffient value to use
 */
int Contraction_::getOptimizeColumn(const vector<bool>& printableCols,
                                    const int row ) const
{
   int ret = -1;  // means not optimizable
   int numCoef = num_coefficients;
   for (int c=0; c<numCoef; c++) {
      if (!printableCols[c] &&
            (coefficients[c + (numCoef*row)] != 0.0)) {
         ret = c;
         break;
      }
   }
   return ret;
}

/*******************************************************************
 Method : coefficient() 
 Summary: Changes a single coefficient for the contraction as indexed
          by the row and column.
*******************************************************************/
void Contraction_::coefficient (size_t row, size_t col, double newValue)
{
  if (row < 0 || row >= num_exponents || col < 0 || col >= num_coefficients) {
    EE_RT_ASSERT(false, EE_WARNING, "Invalid coefficient index");
  } else {
    int index = row * num_coefficients + col;
    coefficients[index] = newValue;
  }
}

///// ContractionSet METHODS /////

/*******************************************************************
 Method : default constructor
 Summary:
*******************************************************************/
ContractionSet::ContractionSet()
{
  // vector of contractions is empty

  // ECP stuff only:
     ncore = 0;
     lmax = 0;
}

/*******************************************************************
 Method : copy constructor
 Summary:
*******************************************************************/
ContractionSet::ContractionSet(ContractionSet& newSet)
{
// Copy all contractions in vector:
   for (size_t i = 0; i < newSet.size(); i++)
   {
     if (newSet[i] != (Contraction_*) 0)
       push_back(new Contraction_(*(newSet[i])));      
     else
       EE_RT_ASSERT(false, EE_WARNING,
                    "Trying to insert NULL Contraction into ContractionSet!");
   }
  
// ECP stuff only
   ncore = newSet.ncore;
   lmax = newSet.lmax;
}

/*******************************************************************
 Method : destructor
 Summary:
*******************************************************************/
ContractionSet::~ContractionSet()
{
  for (size_t i = 0; i < size(); i++)
    if ((*this)[i] != (Contraction_*) 0)
      delete (*this)[i];
}

/*******************************************************************
 Method : clone_optimizeGC()
 Summary: Clone has fewer number of exponents and coefficients (but
          more contractions in the set), as they have been optimized.

          Each contraction will get converted into a set of 
          contractions:  the "first" contraction (which includes
          the initial set of functions that are not optimizable),
          and some "optimized" contractions that are one function
          each (one exp and one coefficient).

          An optimizable function (row) is one that has a 1.0
          coefficient in it.  
          9/18/05 Actually the number need not be 1.0 as new basis sets 
          show.  The correct definition is a row with a non-zero value
          in a column that is all 0's except one value.

          Example:  

BEFORE OPTIMIZATION:
                                            non-printable column
H    S                                          
        13.01000000         0.01968500         0.00000000 |
         1.96200000         0.13797700         0.00000000 |<== first contract.
         0.44460000         0.47814800         0.00000000 |
         0.12200000         0.50124000         1.00000000 <== optimizable row
         0.12200000         0.50124000         0.99998370 <== optimizable row
H    P
         0.72700000         1.00000000


AFTER OPTIMIZATION:

H    S
        13.01000000         0.01968500   <== first contraction
         1.96200000         0.13797700   
         0.44460000         0.47814800   
H    S
         0.12200000         1.00000000   <== optimizable row
H    S
         0.12200000         0.99998370   <== optimizable row
H    P
         0.72700000         1.00000000


 Preconditions: Will only be called for orbital, diffuse, rydberg,
                or polarization types.
*******************************************************************/
ContractionSet* ContractionSet::clone_optimizeGC(ContractionInfo* contInfo)
{
  ContractionSet* result = new ContractionSet();
  ContractionSet::const_iterator cont_it;

  // TODO - we are not taking into account the extra contraction
  // data (delete, uncontract, etc.) that may be added for Amica.
  // That is because this data is stored with the TGBSConfig, not with
  // the individual basis sets and their contractions.  To add
  // this, we will have to pass in this data in CmdComputeMO::normalize().

  // loop through each contraction in the contracted set
  Contraction_* cont;
  Contraction_* newCont;
  size_t num_coef,num_exp,j,f,i;

  for(cont_it  = begin();
      cont_it != end(); cont_it++) {
    cont = (*cont_it);
    newCont = 0;
    vector<double> firstExps;
    vector<double> firstCoefs;
    vector<Contraction_*>temp; // used for ordering contractions

    // First determine if the contraction has been deleted:
    // (This assumes the contraction for this type of basis set
    // has only ONE shell)
    if (contInfo != 0 && 
        contInfo->deleteFlags[cont->shells[0]] == true ) {
      continue;
    }

    num_coef = cont->num_coefficients;
    num_exp = cont->num_exponents;
 
    // first determine what columns can be printed:
    // (non-printable columns have only one non zero in them)
    vector<bool> printableCols = cont->getPrintableColumns();

    // Loop through each row in the table of exps and coefs
    for (j = 0; j < num_exp; j++) {

      int optCol = cont->getOptimizeColumn(printableCols, j);

      if (optCol == -1) { 
        // optCol will be -1 if the row is not one that will be optimized
        
        // Print exponent for row	
        firstExps.push_back(cont->exponents[j]);
        
        // Print coefficients for row
        for (f = 0; f < num_coef; f++) {
          if (printableCols[f]) {
            firstCoefs.push_back(cont->coefficients[f + (num_coef*j)]);
          }
        }
        
      } else { 
        // make the expon and coef into a separate contraction

        newCont = new Contraction_;
        newCont->shells.push_back(cont->shells[optCol]);

        // Print exponent
        newCont->num_exponents = 1;
        newCont->exponents = new double[1];
        newCont->exponents[0] = cont->exponents[j];

        // Print the coefficient (not necessarily 1.0)
        newCont->num_coefficients = 1;
        newCont->coefficients = new double[1];
        newCont->coefficients[0] = cont->coefficients[optCol + (num_coef*j)];
  
        // Add this contraction to the set
        temp.push_back(newCont);
      }
    }
    // Save the first contraction one time to the contraction set
    if (firstExps.size() > 0) {
      
      // If the uncontract flag has been set, save each as a separate
      // contraction with one coefficient of 1.0
      if (contInfo != 0 && 
          contInfo->uncontractFlags[cont->shells[0]] == true) {
        
        for (i = 0; i < firstExps.size(); i++) {
          newCont = new Contraction_;
          newCont->num_coefficients = 1;
          newCont->num_exponents = 1;
          
          // Create the shells array
          newCont->shells.push_back(cont->shells[0]);
          
          // Create exponents and coefficients arrays;
          newCont->exponents = new double[1];
          newCont->exponents[0] = firstExps[i];
          newCont->coefficients = new double[1];
          newCont->coefficients[1] = 1.0;
          
          // Add this to contraction set:
          result->push_back(newCont);
        }
        
      } else { 
         // print all of first set as one contraction
        
        newCont = new Contraction_;
        
        newCont->num_coefficients = 
          (int)(firstCoefs.size()/firstExps.size());
        newCont->num_exponents = firstExps.size();
        
        // Create the shells array
        for (i = 0; i < newCont->num_coefficients; i++)
          newCont->shells.push_back(cont->shells[i]);
        
        // copy exponents to double array
        newCont->exponents = new double[newCont->num_exponents];
        for (i = 0; i < newCont->num_exponents; i++)
          newCont->exponents[i] = firstExps[i];
        
        // copy coefficients to double array
        newCont->coefficients = new double[firstCoefs.size()];
        for (i = 0; i < firstCoefs.size(); i++)
          newCont->coefficients[i] = firstCoefs[i];
        
        // Add first contraction to the contraction set
        result->push_back(newCont);
      }
    }
    // Now put all the contractions in temp behind the contractions in 
    // first set    
    for (i = 0; i < temp.size(); i++) {
      result->push_back(temp[i]);
    }
    
  }
  return result;

}


/*******************************************************************
 Method : clone_noop_gcNotSupported
 Summary:

 ORIGINAL CONTRACTIONS
H    S
        13.01000000         0.01968500         0.00000000
         1.96200000         0.13797700         0.00000000
         0.44460000         0.47814800         0.00000000
         0.12200000         0.50124000         1.00000000


  AFTER GC NOT SUPPORTED FIX
H    S   
          13.01000000       0.01968500
           1.96200000       0.13797700
           0.44460000       0.47814800
           0.12200000       0.50124000

H    S   
           0.12200000       1.00000000


 Preconditions: Will only be called for orbital, diffuse, rydberg,
                or polarization types.
*******************************************************************/
ContractionSet* ContractionSet::clone_noop_gcNotSupported(
                                                   ContractionInfo* contInfo)
{
  ContractionSet* result = new ContractionSet();

  ContractionSet::const_iterator cont_it;
  size_t num_coef,num_exp,f,j,i;
  bool uncontractFlag;
  Contraction_* newCont;
    
  // loop through each contraction in the contracted set
  for(cont_it  = begin();
      cont_it != end(); cont_it++)
  {
    Contraction_* cont = (*cont_it);

    // If this contraction has been deleted, go on to the next one:
    // (This assumes the contraction for this type of basis set
    // has only ONE shell)
    if (contInfo != 0 && 
        contInfo->deleteFlags[cont->shells[0]] == true ) {
      continue;
    }    

    num_coef = cont->num_coefficients;
    num_exp = cont->num_exponents;

    // Loop through each column (shell) in the table of exps and coefs
    for (f = 0; f < num_coef; f++)
    {
      uncontractFlag = (contInfo != 0 && 
                        contInfo->uncontractFlags[cont->shells[f]] == true);
    
      vector<double> exponents;
      vector<double> coefficients;

      // Only print the exponents if the coef for that column is not 0:
      for(j=0;j<num_exp;j++) {

        if (cont->coefficients[f + (num_coef*j)] == 0.0)
          continue;

        if (uncontractFlag) {
          newCont = new Contraction_;
          newCont->shells.push_back(cont->shells[f]);
          newCont->num_coefficients = 1;
          newCont->num_exponents = 1;
          newCont->exponents = new double[1];
          newCont->coefficients = new double[1];
          newCont->exponents[0] = cont->exponents[j];
          newCont->coefficients[0] = 1.0;
          result->push_back(newCont);
          
        } else {
          // Print exponent:
          exponents.push_back(cont->exponents[j]);
          
          // Print coefficient:
          coefficients.push_back(cont->coefficients[f + (num_coef*j)]);
        }
      }

      // If not uncontracted, this is all one contraction
      if (!uncontractFlag) {

        newCont = new Contraction_;

        // Each contraction will have only one shell
        newCont->shells.push_back(cont->shells[f]);

        newCont->num_coefficients =
          (int) (coefficients.size()/exponents.size());
        newCont->num_exponents = exponents.size();

        // copy exponents to double array
        newCont->exponents = new double[newCont->num_exponents];
      
        for (i = 0; i < newCont->num_exponents; i++)
          newCont->exponents[i] = exponents[i];
    
        // copy coefficients to double array
        newCont->coefficients = new double[coefficients.size()];
        for (i = 0; i < coefficients.size(); i++)
          newCont->coefficients[i] = coefficients[i];

        result->push_back(newCont);
      }
    }
  }
  return result;
}

/**********************************************************************
 This method makes a copy of itself, however, it takes into account
 the uncontract/delete info provided by the ContractionInfo object.
 ContractionInfo objects are maintained by TGBSConfig.  They get set
 when the user deletes or uncontracts a contraction using the
 Basis Set Editor.

 To clone the contraction without the deleted contraction in it, use 
 this method.
 **********************************************************************/
ContractionSet* ContractionSet::clone_normal(ContractionInfo* contInfo)
{
  ContractionSet* result = new ContractionSet();

  ContractionSet::const_iterator cont_it;
  Contraction_* cont;
  bool uncontracted;
  int lastShell;
  int curShell;

  // Loop through all the contractions in the contracted set
  for(cont_it  = begin();
      cont_it != end(); cont_it++) {
    
    cont = (*cont_it);
    uncontracted = false;
    vector<int> uniqueShells;
    vector<bool> shells(TGaussianBasisSet::maxShell +1, false);
    lastShell = -1;
    
    // find out what shells belong in the contraction, and if
    // one of the shells is uncontracted:
    // (only include shells that are not deleted!)
    // (We do this because some basis sets can have shared-shell
    // contractions)
    for(size_t shell_idx=0; shell_idx<cont->shells.size(); shell_idx++) {
      curShell = (int)cont->shells[shell_idx];
      if (curShell != lastShell) {
        if (contInfo == 0 || contInfo->deleteFlags[curShell] == false) {
          uniqueShells.push_back(curShell);
          shells[curShell] = true;
        }
        lastShell = curShell;
      }
      if (contInfo != 0 && contInfo->uncontractFlags[curShell] == true) {
        uncontracted = true;
      }
    }

    if (contInfo == 0) { // 
      cloneNormalContraction_PartialShells(result, cont, shells);
      
    } else if (uncontracted == false) { // 
      cloneNormalContraction_PartialShells(result, cont, shells);
      
    } else if (uniqueShells.size() == 1) { // only 1 shell represented
      
      cloneUncontractedContraction_PartialShells(result, cont, 
                                                 uniqueShells[0]);
      
    } else { // shared-shell contraction needs to be split up (yuck)
      vector<bool> tmp(TGaussianBasisSet::maxShell + 1, false);

      for (size_t i = 0; i < uniqueShells.size(); i++) {
        
        if (contInfo->uncontractFlags[uniqueShells[i]] == false) {
          tmp[i] = true;
          cloneNormalContraction_PartialShells(result, cont, tmp);
          tmp[i] = false;

        } else {
          cloneUncontractedContraction_PartialShells(result, cont, i);
        }
      }
    }
  }
  return result;
}

/*************************************************************************
This is a little different than just making a copy of the Contraction.
Instead, it only includes the coefficients for the indicated shells.
(Used for splitting combined shell contractions.)
(Also used when deleting contractions for certain cells)
*************************************************************************/
void ContractionSet::cloneNormalContraction_PartialShells
(ContractionSet* result, 
 Contraction_* cont, 
 const vector<bool>& shells) // Only non-deleted shells get passed in
{
  Contraction_* newCont =  new Contraction_;
  vector<double>coefficients;
  size_t j,l;

  newCont->num_exponents = cont->num_exponents;
  newCont->exponents = new double[cont->num_exponents];
  size_t num_coef = cont->num_coefficients;
  bool shellsDone = false;

  // Only include the coefficients for the designated shells:
  for(j=0;j<cont->num_exponents;j++) {
    newCont->exponents[j] = cont->exponents[j];
          
    for(l=0;l< num_coef;l++) {
      if (shells[(int)cont->shells[l]] == true) {
        coefficients.push_back(cont->coefficients[l + (num_coef*j)]);
        
        if(!shellsDone) {
          // do the shells
          newCont->shells.push_back(cont->shells[l]);
        }
      }
    }
    shellsDone = true;
  }
  // In the rare (maybe never) event that all the shells have been
  // deleted for this contraction:
  if (coefficients.size() == 0) {
    delete newCont;
    return;
  }
 
  // Do the coefficients
  newCont->num_coefficients = 
    (int)(coefficients.size()/newCont->num_exponents);

  newCont->coefficients = new double[coefficients.size()];

  for (j = 0; j < coefficients.size(); j++) {
    newCont->coefficients[j] = coefficients[j];
  }

  result->push_back(newCont);
  
}

/*************************************************************************
Just like the above function except it also uncontracts all the
functions.
*************************************************************************/
void ContractionSet::cloneUncontractedContraction_PartialShells
(
 ContractionSet* result, 
 Contraction_* cont, 
 const int& shell)
{
  Contraction_* newCont;

  // assume this will effect only 1 shell for the time being
  // print every gaussian primitive as a separate contraction
   
  for(size_t j = 0; j < cont->num_exponents; j++) {

    newCont = new Contraction_;
    newCont->shells.push_back((TGaussianBasisSet::AngularMomentum)shell);
    newCont->num_exponents = 1;
    newCont->num_coefficients = 1;
    newCont->exponents =  new double[1];
    newCont->coefficients = new double[1];   
    newCont->exponents[0] = cont->exponents[j];
    
    // Print out 1.0 for the coefficient, since the coefficients for
    // each function must normalize to 1.0
    newCont->coefficients[0] = 1.0;

    result->push_back(newCont);
  }
}


/**
 * For each shell, fills in the contracted-from and contracted-to data.
 * This is used to provide information on the contraction in the
 * apparently standard notation of (19s)/[2s] 
 * from is the value in () while to is the value in [].
 * From is the total number of exponents for that shell.  To is the
 * number of contraction blocks for that shell.  This is a little messy
 * because of the Pople-style basis sets that have shared S/P functions.
 */
void TGaussianBasisSet::contractionInfo
(
  const string& element, 
  vector<int>& from, 
  vector<int>& to
) const
{
  // Initialize to 0's
  // have to clear them first because resize() only puts 0 for new elements
  // when the vector size is being increased
  from.clear(); 
  to.clear();

  from.resize(sizeof(shell_formatter)/sizeof(char*),0);
  to.resize(sizeof(shell_formatter)/sizeof(char*),0);

  ContractionMap::const_iterator map_it = p_contractions.find(element);
  if (map_it != p_contractions.end()) 
  {

    ContractionSet::const_iterator cont_it;

    int lastFunction;
    for (cont_it=(*map_it).second->begin();
        cont_it!=(*map_it).second->end();cont_it++)
    {
      lastFunction = -1;
      vector<TGaussianBasisSet::AngularMomentum> funcs
        = (*cont_it)->shells;

      for (size_t f=0; f<(*cont_it)->num_coefficients; f++)
      {
        if (funcs[f] != lastFunction) {
          from[funcs[f]] += (*cont_it)->num_exponents;
        }
        to[funcs[f]]++;
        lastFunction = funcs[f];
        
      }
    }
  }
}

