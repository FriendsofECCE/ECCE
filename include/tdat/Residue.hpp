///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Residue.H
//
//
// CLASS SYNOPSIS:
//    A data container for residue sequence information.
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef RESIDUE_HH
#define RESIDUE_HH

#include <iostream>
  using std::ostream;

#include <string>
  using std::string;

#include "tdat/TAtm.H"
#include "tdat/DisplayStyle.H"

class Fragment;

class Residue 
{

public:
  enum Coupling {INTERIOR, TAIL, HEAD, MONOMER}; 
  enum Attribute {NONE, HELIX, SHEET}; 
  enum ResidueStatus {MISSING, COMPLETE, INCOMPLETE, UNKNOWN, GENERATABLE};

  // Constructors
    Residue(const char *name, const int& number);
    Residue(const Residue& rhs);
    virtual ~Residue(void);

  // Operators
    Residue& operator=(const Residue& rhs);
    bool operator==(const Residue& rhs) const;
    bool operator!=(const Residue& rhs) const; 

    // For making hash tables and so forth.
    static unsigned long hFun(const Residue& r);

    // For parsing strings to get residue parameters
    static void parseString(string input, string &name, int &pdb_idx,
	                    char &chain, int &idx);
    static Residue::ResidueStatus statusFromString(const string& status);
    static string statusToString(Residue::ResidueStatus);

  // Accessors
    //string                  name(void) const;
    string name() const;
    int                     number(void) const;
    char                    chain(void) const;
    char                    insertCode(void) const;
    Residue::Coupling       couplingInfo(void) const;
    Residue::ResidueStatus  residueStatus(void) const;
    string                  residueStatusAsString() const;
    TAtm*		    firstAtomPtr(void) const;
    string                  segID(void) const;
    Residue::Attribute      getProteinAttribute(void) const;
    DisplayStyle     displayStyle() const;
    int                     index() const;
    bool                    isSolvent() const;

  // Modifiers
    void name(const char* value);
    void segID(const char* value);
    void number(const int& value);
    void chain(const char value);
    void insertCode(const char value);
    void couplingInfo(const Coupling& value);
    void residueStatus(const ResidueStatus& value);
    void firstAtomPtr(TAtm* atm);
    void setProteinAttribute(const Attribute& value);
    void displayStyle( const DisplayStyle& ds );
    void index(int value);

protected:

  // State
    char           p_name[5];
    char           p_segID[5];
    int            p_number;
    char           p_chain;
    char           p_insertCode;
    ResidueStatus  p_status;
    Coupling       p_coupling;
    TAtm*	   p_firstAtomPtr;
    Attribute      p_attribute;
    DisplayStyle p_displayStyle;
    /** variable for fast access to index within fragment ala TAtom, TBond */
    int            p_index;

private:

friend ostream& operator<< (ostream& os, const Residue& rhs);

}; // Residue


#endif // RESIDUE_HH
