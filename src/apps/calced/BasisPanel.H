/**
 * @file
 *
 * The basis panel of the Set-up tab's detail area (#983): the basis-set
 * library as a filterable list, what the chosen set covers in this
 * chemical system, and the current assignment.  Only the view: the
 * library and the assignment are the Basis Set Tool's classes
 * (EDSIGaussianBasisSetLibrary, GBSAssign).  It is meant to grow into the
 * Basis Set Tool's replacement: per-element assignment, the matching ECP
 * (offered, not added unasked) and charge/exchange fitting sets.
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
  enum Kind { ORBITAL, ECP, FIT };
  // Assign the chosen set to the checked elements; false (and a message)
  // if not possible.
  bool assignChecked();
  // Test hook: choose NAME of that kind, check ELEMENTS (none: all), assign.
  // False if there is no such set.
  bool assignNamed(const std::string& name, Kind kind,
                   const std::vector<std::string>& elements);
  // Add the ECP offered after an ECP orbital set; false if none offered.
  bool acceptEcp();
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
  wxStaticText *p_offer;
  std::vector<std::string> p_elementNames;
  std::string p_messageText, p_ecpOffer;
  std::vector<std::string> p_ecpOfferFor;

  void fill();
  void chosen();
  void fillElements();
  std::vector<std::string> checked() const;
  void offerEcp(const std::string& basis, const std::vector<std::string>& els);
  void say(const std::string& text);
  Kind kindOf(TGaussianBasisSet::GBSType type) const;
};

#endif
