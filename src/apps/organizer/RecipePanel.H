/**
 * @file
 *
 * Apply Recipe (#710): the set-up panel that docks on the left of the
 * workflow view, with a live preview of the boxes on its right.  Choosing
 * the recipe, the code and the level of theory per role redraws the preview;
 * Create boxes makes the steps.  Every control has a method for the tests.
 */

#ifndef RECIPEPANEL_HH
#define RECIPEPANEL_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <set>
#include <string>
#include <vector>

#include "Recipe.H"
#include "WorkflowSteps.H"

class Resource;
class Session;
class WorkflowPanel;
class wxCheckBox;
class wxListCtrl;
class RecipePreview;
class wxScrolledWindow;
class wxSpinCtrl;
class wxTextCtrl;

class RecipePanel : public wxPanel
{
public:
  /// `after` is the selected box, or null.
  RecipePanel(wxWindow *parent, WorkflowPanel *owner, Session *wf,
              Resource *after);

  void setAutoRun(bool on);
  bool selectRecipe(const std::string& id);
  bool selectVariant(const std::string& id);
  bool setLevel(const std::string& role, const std::string& method,
                const std::string& basis);
  void setApplyAfter(bool after);

  const Recipe *recipe() const;
  const RecipeVariant *variant() const;
  WorkflowSteps::Levels levels() const;
  std::string problem() const;
  Resource *after() const { return p_applyAfter ? p_after : 0; }

  /// What the panel shows, one line: recipe, variant, preview, levels, why.
  std::string report() const;

  void createClicked();
  void cancelClicked();

  /// The fragments of a recipe with "fragment" steps (#201); false when
  /// the variant has none.
  bool setFragments(const std::vector<RecipeFragment>& fragments);
  std::vector<RecipeFragment> fragments() const;
  /// Why the fragments cannot be used, "" when they can.
  std::string fragmentProblem() const;
  /// Scrolls the settings to the Fragments section (for a picture).
  void scrollToFragments();

private:
  void fillFragments(const std::vector<RecipeFragment>& fragments);
  void addFragmentRow(const RecipeFragment& f);
  void fragmentsChanged();
  void fillRoles(bool preserve = false);
  void layoutSettings();
  void fillVariants();
  void changed();
  std::set<std::string> registeredCodes() const;

  WorkflowPanel *p_owner;
  Session *p_session;
  Resource *p_after;
  bool p_applyAfter;
  StructureFacts p_facts;
  std::vector<std::string> p_problems;
  std::vector<Recipe> p_recipes;
  int p_recipe;
  int p_variant;

  wxListCtrl *p_list;
  wxBoxSizer *p_variantBox;
  std::vector<wxRadioButton*> p_variantButtons;
  wxScrolledWindow *p_settings;
  wxPanel *p_where;
  wxPanel *p_rolePanel;
  wxBoxSizer *p_roleSizer;
  struct RoleRow {
    std::string id;
    wxComboBox *method, *basis;
    wxStaticText *why, *label, *applies;
  };
  std::vector<RoleRow> p_roles;
  wxRadioButton *p_new, *p_afterButton;
  wxStaticText *p_description, *p_problem, *p_facts_text;
  wxCheckBox *p_autorun;
  RecipePreview *p_preview;

  // Fragments (#201): shown for a variant with "fragment" steps.
  wxPanel *p_fragPanel;
  wxBoxSizer *p_fragRows;
  wxStaticText *p_fragHeading, *p_whereHeading;
  struct FragRow {
    wxTextCtrl *atoms;
    wxSpinCtrl *charge, *mult;
    wxStaticText *formula;
  };
  std::vector<FragRow> p_frags;
  std::vector<int> p_z;        ///< atomic numbers of the selected box's structure
  int p_charge;
  RecipeVariant p_expanded;    ///< the variant with its fragment steps made
  bool p_fragMessage;          ///< p_problem shows the fragments' check
};

#endif
