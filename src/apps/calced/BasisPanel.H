/**
 * @file
 *
 * The basis panel of the Set-up tab's detail area (#983): the basis-set
 * library as a filterable list, what the chosen set covers in this
 * chemical system, and the current assignment.  Only the view: the
 * library and the assignment are the Basis Set Tool's classes
 * (EDSIGaussianBasisSetLibrary, GBSAssign).  It does what the Basis Set
 * Tool does: per-element and per-atom assignment, the matching ECP (offered,
 * not added unasked), fitting sets (only where the theory uses them),
 * augmenting sets and tight/diffuse functions.
 */
#ifndef BASISPANEL_HH
#define BASISPANEL_HH

#include <memory>
#include <string>
#include <vector>

#include "wx/panel.h"

#include "dsm/TGaussianBasisSet.H"

class CalcEd;
class EDSIGaussianBasisSetLibrary;
class wxListCtrl;
class wxStaticText;
class wxTextCtrl;
class wxButton;
class wxCheckListBox;
class wxChoice;
class wxSpinCtrl;
class WxContractionEditor;
class WxBasisSetCoefficients;

class BasisPanel : public wxPanel
{
public:
  BasisPanel(wxWindow *parent, CalcEd *editor);
  ~BasisPanel();

  void setFilter(const std::string& text);
  bool pick(const std::string& name);
  // Use for all atoms with the chosen set; false (and a message) if not
  // possible.
  bool useForAll();
  // What a library entry is assigned as.
  enum Kind { ORBITAL, ECP, FIT, AUGMENT };
  // Assign the chosen set to the checked elements; false (and a message)
  // if not possible.
  bool assignChecked();
  // Test hook: choose NAME of that kind, check ELEMENTS (none: all), assign.
  // False if there is no such set.
  bool assignNamed(const std::string& name, Kind kind,
                   const std::vector<std::string>& elements);
  // Several sets of one kind, each on its own targets (elements or atom
  // names; none: all), applied in one step so an uncompleted basis is never
  // seen.  False if a set is unknown or the editor refuses.
  bool assignMany(Kind kind,
      const std::vector<std::pair<std::string, std::vector<std::string> > >& s);
  // Tight or diffuse functions on shell SHELL of ELEMENT's orbital set.
  bool appendFunctions(bool tight, const std::string& element, int shell,
                       int count);
  // Atom names the next assignment goes to instead of the checked elements.
  void setAtoms(const std::string& names);
  // Add the ECP offered after an ECP orbital set; false if none offered.
  bool acceptEcp();
  // Contractions of ELEMENT as "s=9/3 p=4/2" (primitives/contracted per
  // shell).  ROW >= 0 first sets that contraction row to ACTION
  // (uncontract, delete or keep); false with the reason in OUT if refused.
  bool setContraction(const std::string& element, int row,
                      const std::string& action, std::string& out);
  // Exponents and coefficients as the editor's code lists them.
  std::string coefficients() const;
  // The Edit contractions and exponents windows.
  void editContractions();
  void showCoefficients();
  // The editor's basis changed: redraw the assignment.
  void refresh();
  std::string describe() const;

private:
  struct Entry { std::string name; TGaussianBasisSet::GBSType type; };
  CalcEd *p_ed;
  std::unique_ptr<EDSIGaussianBasisSetLibrary> p_lib;
  std::vector<Entry> p_all, p_shown;
  int p_picked = -1;
  std::vector<std::string> p_covered, p_uncovered;
  wxTextCtrl *p_filter;
  wxListCtrl *p_list;
  wxStaticText *p_details, *p_cover, *p_message, *p_current;
  wxButton *p_use, *p_assign, *p_offerButton;
  wxCheckListBox *p_elements;
  wxStaticText *p_offer, *p_fitNote;
  wxTextCtrl *p_atoms;
  wxChoice *p_appendKind;
  wxSpinCtrl *p_appendShell, *p_appendCount;
  WxContractionEditor *p_cntrcEdit = nullptr;
  WxBasisSetCoefficients *p_coeffs = nullptr;
  bool p_inEdit = false;  // the contraction editor is the one changing the basis
  bool p_fitUsed = true;
  std::vector<std::string> p_elementNames;
  std::string p_messageText, p_ecpOffer;
  std::vector<std::string> p_ecpOfferFor;

  void fill();
  void chosen();
  void fillElements();
  std::vector<std::string> checked() const;
  // The atom field's names if any, else the checked elements.
  std::vector<std::string> targets() const;
  std::vector<std::string> targetElements() const;
  void offerEcp(const std::string& basis, const std::vector<std::string>& els);
  void say(const std::string& text);
  Kind kindOf(TGaussianBasisSet::GBSType type) const;
};

#endif
