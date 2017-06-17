///////////////////////////////////////////////////////////////////////////////
//
// Design:
//   The TResItem objects are used to create the TResTab object. Their only
//   use is to store information about whether a residue is biologically
//   derived or not and if so, what kind of residue it is (amino acid, DNA,
//   or RNA).
//
///////////////////////////////////////////////////////////////////////////////

#include <string>
using std::string;

#include <string.h>

#include "tdat/TResItem.H"


///////////////////////////////////////////////////////////////////////////////
//   Constructors
///////////////////////////////////////////////////////////////////////////////
TResItem::TResItem()
{
}
TResItem::TResItem(const char* name, const Classification& biostatus)
{

// make sure name is right-justified, with one extra space at end if name
// is less than four characters.

  int len = strlen(name);
  p_name = "    ";
  int ioff = 3-len;
  int i;
  if (len < 3) {
    for (i=0; i < len; i++) {
      p_name[ioff+i] = name[i];
    }
  } else {
    for (i=0; i < len; i++) {
      p_name[i] = name[i];
    }
  }
  p_classification = biostatus;
  p_charge = 0;
  p_hydrophobicity = 0;
}
///////////////////////////////////////////////////////////////////////////////
//   Destructor
///////////////////////////////////////////////////////////////////////////////
TResItem::~TResItem()
{
}
///////////////////////////////////////////////////////////////////////////////
//   Accessors
///////////////////////////////////////////////////////////////////////////////
string TResItem::name()
{
  return p_name;
}
TResItem::Classification TResItem::classification()
{
  return p_classification;
}
int TResItem::charge()
{
  return p_charge;
}
int TResItem::hydrophobicity()
{
  return p_hydrophobicity;
}
///////////////////////////////////////////////////////////////////////////////
//   Modifiers
///////////////////////////////////////////////////////////////////////////////
void TResItem::name(const char* name)
{

// make sure name is right-justified, with one extra space at end if name
// is less than four characters.

  int len = strlen(name);
  p_name = "    ";
  int ioff = 3-len;
  int i;
  if (len < 3) {
    for (i=0; i < len; i++) {
      p_name[ioff+i] = name[i];
    }
  } else {
    for (i=0; i < len; i++) {
      p_name[i] = name[i];
    }
  }
}
void TResItem::classification(const Classification& biostatus)
{
  p_classification = biostatus;
}
void TResItem::charge(int charge)
{
  p_charge = charge;
}
void TResItem::hydrophobicity(int hydrophobicity)
{
  p_hydrophobicity = hydrophobicity;
}
