///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Store a list of known residue names and classify them based on whether
//    they are amino acids, RNA or DNA residues
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TRESTAB_HH
#define TRESTAB_HH

#include <string>
#include <vector>

  using std::string;
  using std::vector;

#include "tdat/TResItem.H"

/**
 * TResTab contains reference residue information that is loaded only
 * one time.  You can use finalize to clear the data and force a reload.
 * The hydrophobicity is based on the tables of Averaged Scale of
 * Hydrophobicity in Palecz, JACS, vol. 124, pp. 6003-6008 (2002). All
 * charged residues are assigned a hydrophobicity of -1. The remaining
 * amino acids are assigned a hydrophobicity of -1 if the averaged
 * hydrophobicity is less than -50, a value of 0 if the averaged
 * hydrophobicity is between -49 and 50, and a value of 1 if the
 * averaged hydrophobicity is greater the 50.
 **/
class TResTab
{

public:

// Constructor

  TResTab();

// Destructor

  ~TResTab();

// Methods

  TResItem::Classification verify(const char* resname);
  int charge(const char* resname);
  int hydrophobicity(const char* resname);

  static vector<string> getAminoAcids();

  // Clears internal static data.  Should be called when a program exits.
  static void finalize();

private:
  static void initialize();

  static vector<TResItem*>* p_resItemList;
};
#endif
