///////////////////////////////////////////////////////////////////////////////
//
// The class TResItem provides information about residues. At present,
// the only information associated with a residue is its classification
// as and amino acid, RNA, or DNA residue.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TRESITEM_HH
#define TRESITEM_HH

#include <string>

  using std::string;

class TResItem
{

public:
  enum Classification {AminoAcid, RNA, DNA, RNAorDNA, Other};

// Constructors

  TResItem();
  TResItem(const char* name, const Classification& biostatus);

// Destructor

  ~TResItem();

// Accessors

  string name();
  TResItem::Classification classification();
  int charge();
  int hydrophobicity();

// Modifiers

  void name(const char* name);
  void classification(const Classification& biostatus);
  void charge(int charge);
  void hydrophobicity(int hydrophobicity);

private:

  string p_name;
  Classification p_classification;
  int p_charge;
  int p_hydrophobicity;
};
#endif
