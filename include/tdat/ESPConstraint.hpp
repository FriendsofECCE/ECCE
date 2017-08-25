///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ESPConstraint.H
//
//
// CLASS SYNOPSIS:
//    Class ESPConstraint contains data associated with an individual
//    constraint for an NWChem ESP calculation.
//
// DESCRIPTION:
//    ESPConstraint contains and enumerated constant for the constraint type,
//    two lists of atom pointers, and a double for the charge. This data can
//    be used to create an entry in the constraint input field for an esp
//    charge fitting calculation.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ESPCONSTRAINT_HH
#define ESPCONSTRAINT_HH

#include <vector>
#include <string>
using std::vector;
using std::string;

class TAtm;

class ESPConstraint
{

public:

   /**
    * These typs map to conceptual user types.  Some mapping is required to 
    * get to the base NWChem types.
    */
   enum Type {FIXED, EQUAL, NET, GROUP, METHYL, SPECIAL};

   /**
    * Constructor
    */
    ESPConstraint();
   /**
    * Destructor
    */
    virtual ~ESPConstraint();
   /**
    * Operators
    */
    bool    operator==(const ESPConstraint& constraint) const;

   /**
    * Accessors
    */
    ESPConstraint::Type type() const;
    vector<TAtm*> group1() const;
    vector<TAtm*> group2() const;
    double charge() const;
    bool chargeApplies() const;
    bool group2Applies() const;
    string annotation() const;

   /**
    * Modifiers
    */
    void type(ESPConstraint::Type type);
    void group1(vector<TAtm*> list);
    void group2(vector<TAtm*> list);
    void charge(double charge);
    void annotation(string comment);

   /**
    * Methods
    */
    string isValid() const;
    bool hasAtoms(const vector<int>& atoms) const;
    bool hasAtoms() const;
    void makeValid();

   /**
    * Methods associated with types - perhaps should be its own class
    */
    static string getDescription(ESPConstraint::Type type);
    static string typeToLabel(ESPConstraint::Type type);
    static ESPConstraint::Type labelToType(const string& type);


private:
    static void initializeLabels();

    ESPConstraint::Type p_type;
    double p_charge;
    vector<TAtm*> p_group1;
    vector<TAtm*> p_group2;
    string p_annotation;

    static vector<string> *p_typeLabels;

}; // ESPCONSTRAINT


#endif // ESPCONSTRAINT_HH
