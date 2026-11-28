#ifndef STRUCTFILECALCULATION_H_
#define STRUCTFILECALCULATION_H_

#include <string>
  using std::string;

#include "AbstractPropCalculation.H"
#include "CalculationFactory.H"

/**
 * A structure opened from an SDF/MOL, MOL2 or PDBQT file, or from any other
 * file Open Babel reads.  The four plugins differ only in which extensions
 * they claim; the reading is DockFragmentIO, shared with Add Structure.
 * Saving always asks for a new name, because ECCE cannot write these back.
 */
class StructFileCalculation : public AbstractPropCalculation
{
  public:
    StructFileCalculation(const EcceURL& url);

    virtual bool fragment(Fragment *frag);
    virtual bool getFragment(Fragment &frag);
    virtual EcceURL getURL() const;
    virtual bool isReadOnly() const;
    virtual bool canSaveInPlace() const;

    static string extension(const EcceURL& url);

  protected:
    EcceURL p_url;
};

class SdfCalculation : public StructFileCalculation
{
  public:
    SdfCalculation(const EcceURL& url) : StructFileCalculation(url) {}
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();
    DECLARE_CALCULATION()
};

class Mol2Calculation : public StructFileCalculation
{
  public:
    Mol2Calculation(const EcceURL& url) : StructFileCalculation(url) {}
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();
    DECLARE_CALCULATION()
};

class PdbqtCalculation : public StructFileCalculation
{
  public:
    PdbqtCalculation(const EcceURL& url) : StructFileCalculation(url) {}
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();
    DECLARE_CALCULATION()
};

/** Every other format Open Babel reads; present only with obabel. */
class ObabelCalculation : public StructFileCalculation
{
  public:
    ObabelCalculation(const EcceURL& url) : StructFileCalculation(url) {}
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();
    DECLARE_CALCULATION()
};

#endif // STRUCTFILECALCULATION_H_
