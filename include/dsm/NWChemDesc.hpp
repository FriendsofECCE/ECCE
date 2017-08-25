///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: NWChemDesc.H
//
//
// CLASS SYNOPSIS:
//
//   Represents a class for managing NWChem-specific code
//
// EXPORTED TYPES:
//    Class NWChemDesc
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef NWCHEMDESC_HH
#define NWCHEMDESC_HH

#include <strstream>
using std::ostrstream;

#include <fstream>
using std::ifstream;

#include <vector>
using std::vector;

// forward references
class NWChemDesc;
class FFItem;
class FFModel;

// application includes
#include "util/Serializer.H"
#include "util/Serializable.H"
#include "util/IOException.H"

#include "tdat/FFRules.H"
#include "tdat/FFItem.H"
#include "tdat/FFDesc.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

class NWChemDesc: public Serializer, public FFRules
{

public:
  // Constructor
       NWChemDesc(void);

  // Virtual Destructor
       virtual ~NWChemDesc(void);

  // Accessors 

  // General Methods
       void serialize(const Serializable &model, string &output, EcceMap &map);
       void deserialize(const string &input, Serializable &model, EcceMap &map)
         throw (ParseException);
       string mimetype() const;
       void exportFFAtoms(const FFModel &model, vector<int> sources,
	                  ostrstream &outfile) const;
       void exportFFCross(const FFModel &model, vector<int> sources,
	                  ostrstream &outfile) const;
       void exportFFBonds(const FFModel &model, vector<int> sources,
	                  ostrstream &outfile) const;
       void exportFFAngles(const FFModel &model, vector<int> sources,
	                  ostrstream &outfile) const;
       void exportFFTorsions(const FFModel &model, vector<int> sources,
	                  ostrstream &outfile) const;
       void exportFFImproper(const FFModel &model, vector<int> sources,
	                  ostrstream &outfile) const;
       void loadCategories(FFModel *model, vector<string>& sources)
	                  throw(IOException);
       FFItem* parseNWChemAtomData( ifstream&, FFModel*, char*, int, 
	                          const int, int, int*);
       FFItem* parseNWChemCrossData( ifstream&, FFModel*, char*, const int,
	                           int, int, int*);
       FFItem* parseNWChemBondData( char*, FFModel*, const unsigned long,
                             int, int, int*);
       FFItem* parseNWChemAngleData( char*, FFModel*, const unsigned long,
                             int, int, int*);
       FFItem* parseNWChemTorsionData (ifstream&, FFModel*, char*, int,const int,
	                              int, int*);
       bool parseTorsionLine(const char*, FFModel*, FFItem**, int, int*);
  // Force field rules
       bool checkInversion(TAtm *atm, vector<TBond*> list) const;
       vector<bool> findInversions(TAtm *atm, vector<TBond*> list,
                                   vector<int> *ringAtoms) const;

private:
       FFDesc *p_ffDesc;

};

#endif /* NWCHEMDESC_HH */
