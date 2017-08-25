/**
 * @file
 *
 *
 */

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif

#include <stdlib.h>
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <map>
  using std::map;

#include <utility>
  using std::pair;
  using std::make_pair;


#include "util/EcceMap.H"
#include "util/StringConverter.H"
#include "util/InvalidException.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/STLUtil.H"

#include "tdat/Residue.H"
#include "tdat/TPerTab.H"

#include "dsm/ITraversable.H"
#include "dsm/ResourceType.H"
#include "dsm/GridProperty.H"
#include "dsm/AtomProperty.H"

#include "wxgui/GridElement.H"
#include "wxgui/ewxStyledWindow.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/GridCellPropImageRenderer.H"
#include "wxgui/GridAtomElement.H"


static const char* vcouplingMap[] = {"Interior", "Tail", "Head", "Monomer"};

GridAtomElement::GridAtomElement(TAtm *atom, AtomProperty *property) 
  throw(InvalidException)
{
  // Check to make sure valid AtomProperty
  if ((atom == 0) || (property == 0)) {
    throw InvalidException("Invalid TAtm and/or AtomProperty"
                           "pointers provided to GridAtomElement "
                           "constructor", WHERE);
  }

  // Save local copy of ResourceProperty
  p_property = new AtomProperty(*property);

  // Get atom value
  setValue(getAtomValue(atom));

  // Save local copy of element
  p_element = atom->atomicSymbol();

  // Save local copy of index
  p_index = atom->index();

}


GridAtomElement::GridAtomElement(Residue *res, AtomProperty *property) 
  throw(InvalidException)
{
  // Check to make sure valid AtomProperty
  if ((res == 0) || (property == 0)) {
    throw InvalidException("Invalid Residue and/or AtomProperty"
                           "pointers provided to GridAtomElement "
                           "constructor", WHERE);
  }

  // Save local copy of ResourceProperty
  p_property = new AtomProperty(*property);

  // Get atom value
  setValue(getResidueValue(res));

  // Save local copy of element
  //p_element = atom->atomicSymbol();

  // Save local copy of index
  p_index = res->index();

}


GridAtomElement::~GridAtomElement()
{
   // Note p_property deleted by superclass
}


wxString GridAtomElement::getAtomValue(TAtm *atom)
{
  wxString ret;

  AtomProperty::Prop prop = 
    AtomProperty::strToProp(p_property->getName());
  Residue *residue = atom->getResidue();
  bool hasResidue = (residue != 0);

  switch (prop) {
    case AtomProperty::AP_ELEMENT:
      ret = atom->atomicSymbol();
      break;
    case AtomProperty::AP_NUM:
      ret = StringConverter::toString(atom->index()+1);
      break;
    case AtomProperty::AP_SUFFIX:
      ret = atom->suffixHack();
      break;
    case AtomProperty::AP_ATOM_NAME:
      ret = atom->atomName();
      break;
    case AtomProperty::AP_RES_NAME:
      ret = hasResidue ? atom->residueName() : "";
      break;
    case AtomProperty::AP_PDB_NUM:
      ret = hasResidue ? StringConverter::toString(atom->residueNumber()) : "";
      break;
    case AtomProperty::AP_ICODE:
      ret = hasResidue ? atom->icode() : ' ';
      break;
    case AtomProperty::AP_CHAIN:
      ret = hasResidue ? atom->chain() : ' ';
      break;
    case AtomProperty::AP_SEGMENT:
      ret = hasResidue ? atom->segment() : "";
      break;
    case AtomProperty::AP_XCOORD:
      ret = StringConverter::toString(atom->coordinates()[0]);
      break;
    case AtomProperty::AP_YCOORD:
      ret = StringConverter::toString(atom->coordinates()[1]);
      break;
    case AtomProperty::AP_ZCOORD:
      ret = StringConverter::toString(atom->coordinates()[2]);
      break;
    case AtomProperty::AP_MASS:
      ret = StringConverter::toString(atom->getMass());
      break;
    case AtomProperty::AP_BEHAVIOR:
      ret = TAtm::behaviorTypeToString(atom->getBehaviorType());
      break;
    case AtomProperty::AP_OCC:
      ret = StringConverter::toString(atom->occupancy());
      break;
    case AtomProperty::AP_BFACTOR:
      ret = StringConverter::toString(atom->bFactor());
      break;
    case AtomProperty::AP_CHARGE:
      ret = StringConverter::toString(atom->partialCharge());
      break;
    case AtomProperty::AP_TYPE:
      ret = atom->atomType();
      break;
    case AtomProperty::AP_POL:
      ret = StringConverter::toString(atom->polarizability());
      break;
    case AtomProperty::AP_CG:
      ret = StringConverter::toString(atom->chargeGroup());
      break;
    case AtomProperty::AP_PG:
      ret = StringConverter::toString(atom->polarizationGroup());
      break;
    case AtomProperty::AP_ENV:
      ret = StringConverter::toString(atom->dihedralIndex());
      break;
    case AtomProperty::AP_LINK_NUM:
      ret = StringConverter::toString(atom->connectionIndex());
      break;
    case AtomProperty::AP_RES_NUM:
      ret = hasResidue ? StringConverter::toString(residue->index()+1) : 0;
      break;
    case AtomProperty::AP_COUPLING:
      ret = hasResidue ? vcouplingMap[residue->couplingInfo()] : "";
      break;
    case AtomProperty::AP_STATUS:
      ret = hasResidue ? residue->residueStatusAsString().c_str() : "";
      break;
    default:
      ret = "OHOH";  // for debugging
      break;
  }
  //cout << "returning " << p_property->getName() << "=" << ret.c_str() << "!" << endl;
  return ret;
}


wxString GridAtomElement::getResidueValue(Residue *residue)
{
  wxString ret;

  AtomProperty::Prop prop = AtomProperty::strToProp(p_property->getName());

  switch (prop) {
    case AtomProperty::AP_RES_NUM:
      ret = StringConverter::toString(residue->index()+1);
      break;
    case AtomProperty::AP_COUPLING:
      ret = vcouplingMap[residue->couplingInfo()];
      break;
    case AtomProperty::AP_STATUS:
      ret = residue->residueStatusAsString().c_str();
      break;
    case AtomProperty::AP_CHAIN:
      ret = residue->chain();
      break;
    case AtomProperty::AP_SEGMENT:
      ret = residue->segID();
      break;
    case AtomProperty::AP_RES_NAME:
      ret = residue->name();
      break;
    case AtomProperty::AP_PDB_NUM:
      ret = StringConverter::toString(residue->number());
      break;
    case AtomProperty::AP_ICODE:
      ret = residue->insertCode();
      break;
    case AtomProperty::AP_RES_EDIT:
      ret = "false"; // hardwiring this seems like a potential problem
      break;
    default:
      ret = "OHOH";  // for debugging
      break;
  }
  return ret;
}


bool GridAtomElement::isValidValue()
{
  bool ret = true;

  AtomProperty::Prop prop = 
    AtomProperty::strToProp(p_property->getName());

  switch (prop) {
    case AtomProperty::AP_ATOM_NAME:
      ret = isValidAtomName();
      break;
    case AtomProperty::AP_MASS:
      {
        double mass;
        StringConverter::toDouble(p_value.c_str(), mass);
        ret = (mass > 0);
      }
      break;
    case AtomProperty::AP_BEHAVIOR:
      ret = isValidBehavior();
      break;
    case AtomProperty::AP_TYPE:
      ret = isValidAtomType();
      break;
    case AtomProperty::AP_CG:
      {
        int chargeGroup;
        StringConverter::toInt(p_value.c_str(), chargeGroup);
        ret = (chargeGroup >= 0);
      }
      break;
    case AtomProperty::AP_PG:
      {
        int polarizationGroup;
        StringConverter::toInt(p_value.c_str(), polarizationGroup);
        ret = (polarizationGroup >= 0);
      }
      break;
    case AtomProperty::AP_ENV:
      {
        int dihedralIndex;
        StringConverter::toInt(p_value.c_str(), dihedralIndex);
        ret = (dihedralIndex >= 0);
      }
      break;
    case AtomProperty::AP_LINK_NUM:
      ret = isValidLinkType();
      break;
    default:
      break;
  }

  return ret;
}


wxGridCellAttr *GridAtomElement::getAttributes()
{
  wxGridCellAttr *cellAttr = new wxGridCellAttr();

  // Set read only
  if (p_property->getReadOnly()) {
    cellAttr->SetReadOnly();
    cellAttr->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());
  }
  else {
    cellAttr->SetBackgroundColour(ewxStyledWindow::getInputColor());
  }

  // Set Editor and Renderer
  switch (p_property->getType()) {
    case GridProperty::PT_CDATA:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetEditor(new wxGridCellTextEditor);
      cellAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
      break;
    case GridProperty::PT_NUMERIC:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetEditor(new wxGridCellNumberEditor);
      cellAttr->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
      break;
    case GridProperty::PT_FLOAT:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetEditor(new wxGridCellFloatEditor);
      cellAttr->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
      break;
    case GridProperty::PT_CHECK:
      cellAttr->SetRenderer(new wxGridCellBoolRenderer);
      cellAttr->SetEditor(new wxGridCellBoolEditor);
      cellAttr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
      break;
    default:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetEditor(new wxGridCellTextEditor);
      cellAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
  }
  return cellAttr;
}


int GridAtomElement::getDefaultWidth()
{

  int ret = 10;

  AtomProperty *prop = dynamic_cast<AtomProperty*>(p_property);

  if (prop != 0) {
    ret = prop->getDefaultWidth();
  }

  return ret;
}


bool GridAtomElement::hasDefaultWidth()
{
  return true;
}


/**
 * Validate and if necessary reformat user atom name.
 *
 * The PDB atom name format field is 4 characters long and requires that
 * the atomic symbol be in the second place for a single character symbol
 * and in the first and second places for a two character symbol. Thus the
 * atomic name for an atom of carbon is of the form " C##" while silicon
 * would look like "Si##". All single character element names therefore
 * need to be padded with a single blank up front. The only exception is
 * hydrogen, which can have an integer as the first character, e.g..
 * "2H##".
 *
 * The current implentation will accept atom names with blank spaces, e.g.
 * names such as "N A". It also does a check to compare the element symbol
 * derived from the proposed name with the element from the element column.
 * This can be used to correctly align atom names in ambiguous cases, such
 * as "CA", which could refer to either calcium or carbon A.
 */
bool GridAtomElement::isValidAtomName()
{
  bool ret = true;

  // Check to make sure that new atom name is a valid PDB format
  // atom name, or that it can be put in PDB format by
  // putting a white space in front of it.
  int i;
  string element = p_element; 
  STLUtil::stripLeadingAndTrailingWhiteSpace(element);

  if (p_value.size() > 0) {

    string val = p_value.c_str();

    bool validname = true;
    TPerTab pertab;
    bool isvalid = false;

    // Find out how many characters the string contains
    // after removing white space at either end of string
    string sval = val;
    STLUtil::stripLeadingAndTrailingWhiteSpace(sval);
    int chcnt = sval.size();

    if (chcnt == 0) {
      validname = false;
    }

    string at;
    if (chcnt <= 4 && validname) {

      // Check to see if first two nonblank characters are a valid
      // element symbol

      if (chcnt > 1) {
        at.clear();
        at.append(1,sval[0]);
        at.append(1,sval[1]);
        isvalid = pertab.isValid(at);
        if (isupper(at[0]) == 0) {
          isvalid = false;
        }
        // If atomic symbol derived from first two characters does not
        // match element then reject name and see if one character symbol
        // will work
        if (pertab.atomicNumber(at) != pertab.atomicNumber(element)) {
          isvalid = false;
        }

        // remove leading white space from name if name is valid

        if (isvalid) {
          sval = val;
          STLUtil::stripLeadingWhiteSpace(sval);
          if (sval.size() < 4) {
            for (i=sval.size();i<4;i++) {
              sval.append(1,' ');
            }
          }
        }
      }

      // First two characters are not a valid element symbol. Check to
      // see if first character is a valid symbol.
      if (!isvalid) {

        // Check to see if name might correspond to hydrogen

        at.clear();
        at.append(1,sval[0]);
        if (isdigit(at[0]) != 0) {
          if (chcnt > 1) {
            if (sval[1] != 'H') {
              validname = false;
            } else {
              at.clear();
              at.append(1,'H');
            }
          } else {
            validname = false;
          }
        } else if (chcnt < 4) {
          // Name does not correspond to hydrogen. Check to see if it
          // corresponds to single character element symbol

          at.clear();
          at.append(1,sval[0]);
          isvalid = pertab.isValid(at);
          if (isupper(at[0]) == 0) isvalid = false;
          if (isvalid) {

            // Name is valid, return character string with leading whitespace
            // To make consistent with TAtm::atomName, pad out spaces to 4.
            // If we don't do this, we can't check if the value is the same
            // in setAtomName and avoid duplicate calls (and duplicate
            // undos).
            sval.insert(0," ");
            if (sval.size() < 4) {
              for (i=sval.size();i<4;i++) {
                sval.append(1,' ');
              }
            }
          } else {
            validname = false;
          }
        } else {
          validname = false;
        }
      }
    } else if (chcnt > 4) {

      // string is too long to be a valid PDB atom name

      validname = false;
    }

    // verify that atomic element derived from atom name is the same as
    // element in the app
    if (validname) {
      if (pertab.atomicNumber(at) != pertab.atomicNumber(element)) {
        validname = false;
      }
    }
    ret = validname;
  }

  return ret;
}


bool GridAtomElement::isValidBehavior()
{

  bool ret = false;

  if (p_value.IsSameAs("", false) ||
      p_value.IsSameAs("Quantum", false) ||
      p_value.IsSameAs("Point", false) ||
      p_value.IsSameAs("Bq", false) ||
      p_value.IsSameAs("Bqx", false)) {

    ret = true;

    // Do this to get standard capitalization
    TAtm::BehaviorType type = TAtm::stringToBehaviorType(p_value.c_str());
    string sval = TAtm::behaviorTypeToString(type);
    if (!p_value.IsSameAs(sval.c_str())) {
       p_value = sval;
    }
  }

  return ret;
}


bool GridAtomElement::isValidAtomType()
{
  string val = p_value.c_str();
  STLUtil::stripLeadingAndTrailingWhiteSpace(val);
  p_value = val;

  return true;
}


bool GridAtomElement::isValidLinkType()
{
  bool ret = false;

  int val;
  StringConverter::toInt(p_value.c_str(), val);
  if ((val >= 0) && (val <= TAtm::CROSSLINK)) {
    ret = true;
  }

  return ret;
}


string GridAtomElement::getElement()
{
  return p_element;
}


int GridAtomElement::getIndex()
{
  return p_index;
}


