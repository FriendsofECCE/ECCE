///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TGaussianBasisSet.H
//
//
// CLASS SYNOPSIS:
//
// DESCRIPTION:
//
//////////////////////////////////////////////////////////////////////////////
#ifndef TGBS_H
#define TGBS_H

// Application includes:
   #include "util/CompareOps.H"

// STL:
   #include <string> 
   #include <vector>
   #include <map>

     using std::string;
     using std::vector;
     using std::map;
     using std::multimap;
     using std::less;

class Contraction_;
class ContractionSet;
class JCode;
class ContractionInfo;

struct revision {
  string date;
  string comment;
};

// Typedefs:
   typedef map<string, ContractionSet*, less<string> > ContractionMap;

class TGaussianBasisSet 
{
  public :
     const static int maxShell;

    // Static conversion arrays:
    // converts AngularMomentum enum value to it's corresponding character:
    static const char* shell_formatter[];
    // converts GBSType enum value to it's corresponding string:
    static const char* gbs_type_formatter[];
    static const char* contractionType_formatter[];
    static const char* gbsCategory_formatter[];
    static const char* coordSys_formatter[];
  
  // enum types:
     enum  ContractionType
     {AnyConType, Uncontracted, Segmented, GC, UnknownContType};

     enum  CoordinateSystem 
           { Cartesian, Spherical, UnknownCoordSys };

     enum  AngularMomentum
           { s_shell=0, p_shell, d_shell, f_shell, g_shell, h_shell, i_shell,
	          k_shell };

     enum  GBSCategory {ORBITAL, AUXILIARY};

     enum  GBSType { 
                   // ORBITAL basis set types
                   pople,                     
                   other_segmented,
                   correlation_consistent,
                   other_generally_contracted,
                   ECPOrbital,
                   DFTOrbital,

		   // Augmenting basis set types
                   polarization,
                   diffuse,
                   rydberg,

                   // Special augmenting basis sets
                   ecp,      // auxilliary to ECPOrbital
                   exchange, // auxilliary to DFTOrbital
                   charge,   // auxilliary to DFTOrbital

		   // unknown type for imported calculations
		   UnknownGBSType,

		   // appended, user-specified types
		   appended_tight,
		   appended_diffuse
                 };         

  // Constructors:
     TGaussianBasisSet(void);
     TGaussianBasisSet(GBSType& t, string& n);
     TGaussianBasisSet(const TGaussianBasisSet& gbs);
 
     // Special copy constructor used for MO normalize
     // (uses the same logic for optimizing as TGBSConfig::dump)
     TGaussianBasisSet(const TGaussianBasisSet& gbs,
		       string element_,
		       bool optimize,
		       const JCode* code,
                       ContractionInfo* contInfo);
  // Destructor:
     virtual ~TGaussianBasisSet();

  // Static Functions:
     static TGaussianBasisSet::AngularMomentum charToShell(char ch);
     static TGaussianBasisSet::GBSType strToType(string type);
     static TGaussianBasisSet::ContractionType strToContType(string ct);
     static TGaussianBasisSet::CoordinateSystem strToCoordSys(string cs);
     static TGaussianBasisSet::GBSCategory strToCategory(string cat);

  // Accessors:
     Contraction_* getContraction (const char* atomID, size_t ics) const;
     size_t num_contracted_sets(const char* elementID) const ;
     vector<double> exponents(const char* elementID, size_t ics) const; 
                                  // ics = index of contracted set
     vector<TGaussianBasisSet::AngularMomentum>
                               func_types(const char* atomID, size_t ics) const;
     vector<double> coefficients(const char* atomID, size_t ics) const;
     bool useSpherical(string elementID) const;
     TGaussianBasisSet::AngularMomentum maxlvalue(string elementID) const;
     void contractionInfo(const string& element, 
                          vector<int>& from, vector<int>& to) const;

     // get unique key to identify this basis set:
     string getUniqueKey(void) const;

  // Meta data (only filled on demand)
     // vector<string> atoms; do we need this?
     string p_imageFile;
     string p_references;
     string p_info;
     vector<revision> p_comments;

  // Map of contractions:
     ContractionMap   p_contractions;

  // Data:
     GBSCategory      p_category;
     string           p_atomicOrbital;
     CoordinateSystem p_coordSys;
     ContractionType  p_contType;
     GBSType          p_type;
     string           p_name;  
};

class Contraction_ {
  public :

  // Constructors:
     Contraction_ (Contraction_& cont);
     Contraction_(void);

  // Destructor:
     virtual ~Contraction_(void);

  // Accessors:
     double coefficient(size_t row, size_t col) const;
     vector<bool> getPrintableColumns() const;
     int getOptimizeColumn(const vector<bool>& printableCols, int row) const;

  // Modifiers:
     void coefficient(size_t row, size_t col, double newValue);

  // Data members:

     // length of shells = num_coefficients
     vector<TGaussianBasisSet::AngularMomentum> shells;

     size_t num_exponents;
     double* exponents;    // size of array is num_exponents

     size_t num_coefficients; // means number of coefficients per exponent
     double* coefficients; // size of array is num_exponents * num_coefficients

     // ECP stuff (only used for TGaussianBasisSet::ecp )
     size_t l;
     int* powers; // size of array is num_exponents
};

class ContractionSet : public vector<Contraction_*> {
  // A collection of contractions all associated with the same element:
 
  public :
 
  // Constructors:
     ContractionSet();
     ContractionSet(ContractionSet& newSet);

  // Destructor:
     virtual ~ContractionSet(); 

  // Cloning methods used to optimize contractions in some way:
     ContractionSet* clone_noop_gcNotSupported(ContractionInfo* contInfo);
     ContractionSet* clone_optimizeGC(ContractionInfo* contInfo);
     ContractionSet* clone_normal(ContractionInfo* contInfo);

     // This one only takes a single shell, because we always uncontract
     // one shell at a time
     void cloneUncontractedContraction_PartialShells(ContractionSet* result, 
                                                     Contraction_* cont, 
                                                     const int& shell);

     // This one takes in a vector of shells because this will be
     // called for basis sets that could have more than one shell per 
     // contraction.  We have to say which ones are deleted or not
     void cloneNormalContraction_PartialShells( ContractionSet* result, 
                                                Contraction_* cont, 
                                                const vector<bool>& shells);

  // ECP Data (only used for TGaussianBasisSet::ecp) :
     int ncore;
     int lmax;

};

// Used to compare two TGaussianBasisSet::GBSType values in STL containers
struct gtgbs 
{
  bool operator()(const TGaussianBasisSet::GBSType gbs1,
                  const TGaussianBasisSet::GBSType gbs2) const
  { return gbs1 > gbs2; }
};


/*
 * ecps are a total hack all around ... their functions, primitives,
 * and effective primitives are not counted and there is special
 * attention that is paid to them because of their general weirdness...
 */
   
/*
   ECPs are TGaussianBasisSets with additional data included in the
   contractions.  The ECP basis set is added as a separate data member 
   to the TGBSGroup. 
*/

#endif


