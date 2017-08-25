///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TFormula.C
//
//
// DESIGN:
//    The Formula class provides a number of static functions
//    to convert chemical formulas to strings and back.
//    It is currently designed to be used only as the converter
//    but it should soon be included as a data member of
//    other classes (such as ChemicalSystem).
//
//    Right now, the common usage is to construct a Formula
//    from a string.  This causes the string to be parsed
//    (signalling any parse errors).  Once parsed, the formula
//    object can produce a number of convenient representations
//    (some as conversion operators).  Right now this is a
//    ValSortedVector of TagCountPair objects (TCPair.H).
//
//    This class stores a local copy of the string used to construct
//    the formula object.  This will allow a user entered formula
//    to stay exactly as it was entered ("CH2OH").  This is necessary
//    because the parser can't figure the proper order for the formula
//    after parsing and conversion to RWTValSortedVector<TagCountPair>.
//    That is, a parsed formula gets turned into "CH3O".  We keep the
//    original string as the actual formula but we mostly use the
//    parsed version for comparisons and other operations.
//
///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdio.h>

#include <iostream>
  using std::ostream;
  using std::endl;
  using std::ends;
#include <strstream>
  using std::ostrstream;

#include <algorithm>
  using std::find;

#include "util/ErrMsg.H"
#include "util/EcceSortedVector.H"

#include "tdat/TFormula.H"
#include "tdat/TPerTab.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Constructs a Formula from a String
//
// Implementation:
//   The formula gets verified in test_formula.
//   Eventually, test_formula will return a correctly
//   parsed substring of the argument "formula" and will
//   be the formula maintained in the private data member.
//
///////////////////////////////////////////////////////////////////////////////
TFormula::TFormula(const string& formula)
{
  test_formula(formula);
  // p_formula = test_formula(formula);
  p_formula = formula;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Copy Constructor for TFormula
//
///////////////////////////////////////////////////////////////////////////////
TFormula::TFormula(const TFormula& formula)
{
  (*this) = formula;
}

 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Destroy the TFormula
//
///////////////////////////////////////////////////////////////////////////////
TFormula::~TFormula(void)
{}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Assign this TFormula from Another
//
// Implementation:
//   We assume (correctly) that the other TFormula has been validated.
//
///////////////////////////////////////////////////////////////////////////////
TFormula& TFormula::operator=(const TFormula& formula)
{
  p_formula = formula.p_formula;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Compare the TFormula
//
// Implementation:
//   We break the argument's encapsulation until we add an operator string.
//
///////////////////////////////////////////////////////////////////////////////
bool TFormula::operator==(const TFormula& formula) const
{ return (p_formula == formula.p_formula); }

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Compare the TFormula
//
// Implementation:
//   We break the argument's encapsulation until we add an operator string.
//
///////////////////////////////////////////////////////////////////////////////
bool TFormula::operator!=(const TFormula& formula) const
{ return (p_formula != formula.p_formula); }

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Convert the TFormula to a Vector of TagCountPairs.
//   There is only a single TagCountPair per unique tag in the formula.
//   Caller must free the returned memory.
//
// Implementation:
//   Rely on the static operator to parse and allocate the structure
//   to return.  From the SortedVector we convert the structure to
//   an ordered vector (because some classes still require this
//   format).
//
///////////////////////////////////////////////////////////////////////////////
TFormula::operator vector<TTagCountPair>*(void) const
{
  vector<TTagCountPair> *return_val = new vector<TTagCountPair>;
  EcceSortedVector<TTagCountPair, less<TTagCountPair> > *parsed = (*this);

  for (int index = 0; index < parsed->size(); index++) {
    return_val->push_back((*parsed)[index]);
  }
  delete parsed;
  return return_val;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Convert the TFormula to a Sorted Vector of TagCountPair Objects
//   There is only a single TagCountPair per unique tag in the formula.
//   Caller must free the returned memory.
//
// Implementation:
//   Rely on the static operator to parse and allocate the structure
//   to return.
//
///////////////////////////////////////////////////////////////////////////////
TFormula::operator EcceSortedVector<TTagCountPair,
                                    less<TTagCountPair> >*(void) const
{ return parse_formula(p_formula); }


// ------------ Modifiers ------------

// ------------ Accessors ------------

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
///////////////////////////////////////////////////////////////////////////////
string TFormula::formula(int charge) const
{
  string ret;

  EcceSortedVector<TTagCountPair, less<TTagCountPair> >
                   *pairs = parse_formula(p_formula);
  ret = formula(*pairs,charge);
  delete pairs;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//    Returns the empirical formula which follows the convention
//    established by the Quantum Chemistry Literature Database.
//       . Carbon atoms first
//       . H next
//       . the rest in alphabetical order
//       . >1 char elements followed by . (He.)
//       . count follows symbol (H4)
//       . charged species represented by +/-
//
///////////////////////////////////////////////////////////////////////////////
string TFormula::empiricalFormula
(
  const vector<TTagCountPair>& pairs,
  int charge
)
{
  string ret;

  if (pairs.size() > 0) {
    // First handle Carbon
    int idx;
    for (idx=0; idx<pairs.size(); idx++) {
      if (pairs[idx].tag == "C") {
        ret += formatElement("C",pairs[idx].count);
        break;
      }
    }
    // Now Hydrogen
    for (idx=0; idx<pairs.size(); idx++) {
      if (pairs[idx].tag == "H") {
        ret += formatElement("H",pairs[idx].count);
        break;
      }
    }
    EcceSortedVector<TTagCountPair, less<TTagCountPair> > sorted;
    for (idx=0; idx<pairs.size(); idx++) {
      if (pairs[idx].tag != "C" && pairs[idx].tag != "H") {
         sorted.push_back(pairs[idx]);
      }
    }

    for (idx=0; idx<sorted.size(); idx++) {
      ret += formatElement(sorted[idx].tag,sorted[idx].count);
    }
    
    // Now append the charge
    ostrstream os;
    os << chargeString(charge);
    os << ends;

    ret += os.str();
  }
  return ret;
}

string TFormula::formatElement(const string& tag, int count)
{
  ostrstream os;
  os << tag;
  if (tag.length() > 1 && count > 1) {
    os << ".";
  }
  if (count > 1) {
    os << count;
  }
  os << ends;
  string ret = os.str();

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//    Returns a properly formated formula string given the tag/count
//    pairs.
//
// Implementation
//    The rules were described by Hugh - see after();
//    This function is overloaded - both need to be maintained simultaneously.
///////////////////////////////////////////////////////////////////////////////
string TFormula::formula
(
  const EcceSortedVector<TTagCountPair, less<TTagCountPair> >& pairs,
  int charge
)
{
  string ret;

  if (pairs.size() > 0) {
    vector<string> ordered;
    int idx;
    for (idx=0; idx<pairs.size(); idx++) {
      formulaInsert(ordered,pairs[idx].tag);
    }

    // Now append the charge
    ostrstream os;
    for (idx=0; idx < ordered.size(); idx++) {
      os << ordered[idx];
      // Add the number unless its just one
      for (int jdx=0; jdx<pairs.size(); jdx++) {
        if (pairs[jdx].tag == ordered[idx]) {
          if (pairs[jdx].count > 1) os << pairs[jdx].count;
          break;
        }
      }
    }
    os << chargeString(charge);
    os << ends;

    ret = os.str();
  }
  return ret;
}
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//    Returns a properly formated formula string given the tag/count
//    pairs.
//
// Implementation
//    The rules were described by Hugh - see after();
//    This function is overloaded - both need to be maintained simultaneously.
///////////////////////////////////////////////////////////////////////////////
string TFormula::formula
(
  const vector<TTagCountPair>& pairs,
  int charge
)
{
  string ret;

  if (pairs.size() > 0) {
    vector<string> ordered;
    int idx;
    for (idx=0; idx<pairs.size(); idx++) {
      formulaInsert(ordered,pairs[idx].tag);
    }

    // Now append the charge
    ostrstream os;
    for (idx=0; idx < ordered.size(); idx++) {
      os << ordered[idx];
      // Add the number unless its just one
      for (int jdx=0; jdx<pairs.size(); jdx++) {
        if (pairs[jdx].tag == ordered[idx]) {
          if (pairs[jdx].count > 1) os << pairs[jdx].count;
          break;
        }
      }
    }
    os << chargeString(charge);

    os << ends;
    ret = os.str();
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Formats a charge string.  For charge 0, the string is empty; for
//   charges of +1 or -1, only the sign is used;  for all others both
//   the sign and number are used.
///////////////////////////////////////////////////////////////////////////////
string TFormula::chargeString(int charge)
{
  string ret;
  if (charge != 0) {
    if (charge == 1)
        ret = "+";
    else if (charge == -1)
        ret = "-";
    else {
      char buf[12];
      sprintf(buf,"%+d",charge);
      ret = buf;
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Inserts the tag into the proper location in the ordered vector
//   with respect to generating a proper formula.
///////////////////////////////////////////////////////////////////////////////
void TFormula::formulaInsert
(
  vector<string>& ordered,
  const string& tag
)
{
  if (ordered.size() == 0) {
    ordered.push_back(tag);
  } else {
    int cnt = ordered.size();
    for (int idx=cnt-1; idx>=0; idx--) {
      if (after(ordered[idx],tag)) {
        if (idx == (cnt-1)) {
          ordered.push_back(tag);
        } else  {
          vector<string>::iterator it = ordered.begin();
          it += (idx+1);
          ordered.insert(it, tag);
        }
        break;
      }
    }
    if (cnt == ordered.size()) ordered.insert(ordered.begin(),tag);
  }
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Returns TRUE if tag2 comes after tag1 when following proper 
//   periodic table ordering rules.
//
// Implementation
//   Order is determined left to right then bottom to top if there is 
//   a left-to-right tie.
///////////////////////////////////////////////////////////////////////////////
bool TFormula::after(const string& tag1, const string& tag2)
{
  TPerTab tpt;
  int anum1 = tpt.atomicNumber(tag1);
  int anum2 = tpt.atomicNumber(tag2);
  int c1 = tpt.column(anum1);
  int r1 = tpt.row(anum1);
  int c2 = tpt.column(anum2);
  int r2 = tpt.row(anum2);
  return ((c2 > c1) || (c2 == c1 && r2 < r1));
}

// ---------- General Methods  ----------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Convert a TFormula to a Vector of TagCountPair Objects
//   There is only a single TagCountPair per unique tag in the formula.
//   Caller must free the returned memory.
//
// Implementation:
//   This static function is the workhorse of this class.  It handles
//   the parsing of a string into structures that have pairs of
//   tags and their counts.  For example, CH3CH2OH gets transformed
//   into a structure that looks like <C,2><H,5><O,1>.
//   The parse itself works as follows: (PT = Periodic Table of the Elements)
//     Find the largest string (up to three characters - maximum in the PT)
//     Attempt to find successively smaller substrings in the PT
//       (this will always find the longest Atomic Symbol)
//     Once found we attempt to find a trailing count
//     With or without the count we have a tag (Atomic Symbol) and count
//       (1 if none exists)
//     If a TagCountPair is already in the vector add the count otherwise
//       insert a new TagCountObject into the vector.
//     Continue parsing from the last character taken off the string
//
//   An error occurs whenever an invalid atomic symbol is found (i.e.
//   any three successive characters can't produce a valid atomic
//   symbol).  This mechanism has the potential of failing to
//   parse some formulas correctly.  One, of many examples, "ose"
//   would be parsed as "OsE" which would fail on "E" when the
//   formula was intended to be "OSe".  If anyone has a more intelligent
//   parser (that works case-insensitively) we would love to know.
//
///////////////////////////////////////////////////////////////////////////////
EcceSortedVector<TTagCountPair, less<TTagCountPair> >*
TFormula::parse_formula(const string& formula)
{
  EcceSortedVector<TTagCountPair, less<TTagCountPair> > *return_val =
                  new EcceSortedVector<TTagCountPair, less<TTagCountPair> >;
  TTagCountPair tcpair;
  TPerTab tpt;

  char symbol[8];
  char count[64];                        // This magic number will need to grow
  int count_index;
  int symbol_index;
  int num_atoms;
  const char *cursor = formula.c_str();
  const char *start = cursor;

  while ((*cursor) != (char)0) {
    // Get the longest symbol possible
    start = cursor;
    for (symbol_index  = 0; symbol_index < 3; symbol_index++) {
      // Symbol is terminated by a numeral or endline (e.g., H2O)
      // or another capital letter (e.g. HO)
      // Note that CON would become C O N, not Co N
      if (!(isalpha(*cursor))) break;
      if (symbol_index > 0 && isupper(*cursor)) break;
      symbol[symbol_index] = (symbol_index==0) ? toupper(*cursor) : (*cursor);
      cursor++;
    }
    symbol[symbol_index] = '\0';
    int size;
    for (size = symbol_index; size > 0; size--) {
      if (tpt.isValid(symbol)) {
        break;
      }
      symbol[(size-1)] = '\0';
    }
    if (size == 0) {
      ErrMsg().message("EE_PARSE_ERROR", EE_WARNING, WHERE,symbol);
      return return_val;
    }
    cursor = start + size;

    // Get the count, if any
    count_index = 0;
    num_atoms = 1;
    while ((isdigit((*cursor))) && ((*cursor) != (char)0))
      count[count_index++] = *cursor++;
    if (count_index != 0) {
      count[count_index] = '\0';
      num_atoms = atoi(count);
      if (num_atoms <= 0)
      return false;
    }

    if (num_atoms != 0) {
      tcpair.tag = symbol;
      tcpair.count = num_atoms;
      EcceSortedVector<TTagCountPair, less<TTagCountPair> >::iterator it;
      it = find(return_val->begin(), return_val->end(), tcpair);
      if (it == return_val->end()) {
        return_val->push_back(tcpair);
      } else {
        (*it).count += num_atoms;
      }
    }
  }
  return return_val;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Go Through the TFormula and Signal an Error if it can't be Parsed
//
// Implementation:
//   The implementation is exactly the same as parse_formula() except that
//   the return vector isn't constructed.
//
///////////////////////////////////////////////////////////////////////////////
bool TFormula::test_formula(const string& formula)
{
  TPerTab tpt;

  char symbol[8];
  int symbol_index;
  const char *cursor = formula.c_str();

  ErrMsg errs;
  while ((*cursor) != (char)0) {
    // Get the longest symbol possible
    for (symbol_index  = 0; symbol_index < 3; symbol_index++) {
      if (!(isalpha(*cursor))) break;
      symbol[symbol_index] = (symbol_index==0) ? toupper(*cursor) : (*cursor);
      cursor++;
    }
    symbol[symbol_index] = '\0';
    int size;
    for (size = symbol_index; size > 0; size--) {
      if (tpt.isValid(symbol)) {
        break;
      } else {
        errs.flush(errs.count()-1);
      }
      symbol[(size-1)] = '\0';
    }
    if (size == 0) {
      errs.message("EE_PARSE_ERROR", EE_WARNING, WHERE,symbol);
      // Return the Correct SubString Here
      return false;
    }
  }
  // Return the Entire String Here
  return true;
}


// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------



// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Print the TFormula to a Stream
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const TFormula& formula)
{
  os << "[TFormula<" << formula.p_formula << ">]" << endl;
  return os;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//   Read a TFormula from a Stream
//
///////////////////////////////////////////////////////////////////////////////
istream& operator>>(istream& is, TFormula& formula)
{
  char buf[1024];
  is.getline(buf, 1023);
  formula.p_formula = buf;
  return is;
}

// ---------- Encapsulated Behavior ----------
