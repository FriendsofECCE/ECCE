/**
 * @file
 *
 *
 */
#ifndef GRIDATOMELEMENT_HH
#define GRIDATOMELEMENT_HH

#include "util/InvalidException.H"

#include "wxgui/GridElement.H"

class AtomProperty;
class Residue;
class TAtm;

class GridAtomElement : public GridElement 
{
  public:

    GridAtomElement(TAtm *atom, AtomProperty *property)
            throw(InvalidException);
    GridAtomElement(Residue *res, AtomProperty *property)
            throw(InvalidException);

    virtual ~GridAtomElement();

    virtual bool isValidValue();

    virtual wxGridCellAttr *getAttributes();

    virtual int getDefaultWidth();
    virtual bool hasDefaultWidth();

    string getElement();
    int getIndex();

  protected:

    wxString getAtomValue(TAtm *atom);
    wxString getResidueValue(Residue *res);
    bool isValidAtomName();
    bool isValidBehavior();
    bool isValidAtomType();
    bool isValidLinkType();

    string p_element; /**< Atoms atomic symbol */
    int p_index;  /**< Atoms index */

};

#endif

