/**
 * @file
 *
 * The Organizer's view of a Workflow: the steps as boxes in a tree from the
 * first step, a code bar to add a step to the selected box, and Launch Ready
 * Steps.  The data is the Session's links; see WorkflowSteps.H.
 */

#ifndef WORKFLOWPANEL_HH
#define WORKFLOWPANEL_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"

#endif

#include <string>
#include <vector>

#include "util/EcceURL.H"
#include "wxgui/ewxPanel.H"

#include "WorkflowSteps.H"

class wxSpinCtrl;
class CalcMgr;
class Resource;
class ResourceType;
class Session;
class WorkflowCanvas;
class WxResourceTreeItemData;
class PointChooser;
class RecipePanel;
class FragOrbDetailsDialog;

class WorkflowPanel : public ewxPanel
{
public:
  WorkflowPanel(CalcMgr *calcMgr, WxResourceTreeItemData *node,
                wxWindow *parent);

  void refresh();
  void setVertical(bool vertical);
  void selectResource(const EcceURL& url);
  void loseFocus();
  EcceURL selected() const { return p_selected; }

  /// Ctrl-click on a box adds it to (or takes it from) the boxes a join
  /// takes; a plain click leaves only that box.
  void pick(const EcceURL& step, bool add);

  /// The boxes a code button works on: the picked ones, or the selected one.
  std::vector<Resource*> pickedSteps() const;

  /// A code button: asks for the step's details and makes it.
  void codeClicked(size_t code);
  void launchReadyClicked();
  void say(const std::string& text);

  /// The box's context menu, as a right-click on it makes it: its labels
  /// ("a|b|..."), or "error: ..." when building it threw.
  std::string rightClick(const EcceURL& step, bool popup);
  std::string contextMenu(bool popup);

  /// Apply Recipe: docks the set-up panel left of the canvas (after the
  /// selected box, if any).  Null if it is already open.
  RecipePanel *openRecipe();
  /// The box menu's "Apply Recipe from here...": opens the panel with the
  /// selected box as the 'after' box.
  RecipePanel *recipeFromHere();
  static int recipeHereId();
  RecipePanel *recipePanel() const { return p_recipe; }
  /// Create boxes: makes the steps; "" or why not.  Closes the panel on success.
  std::string recipeCreate(const Recipe& recipe, const RecipeVariant& variant,
                           const WorkflowSteps::Levels& levels, Resource *after,
                           bool autorun = false);
  void closeRecipe();
  /// A box's play button: launches that step if it is ready (#736).
  void playClicked(const EcceURL& step, bool interactive = true);
  /// Each box's play button, "name:1|0:reason|...", for the test hook.
  std::string playReport();
  /// The auto-run box ticked or cleared as a click does (test hook).
  void clickAutoRun(bool on);
  /// The Back to Table button pressed as a click does (test hook).
  void clickClose();
  /// Save Workflow as Recipe: "ok <file>" or "error: ...".
  std::string saveAsRecipe(const std::string& id, const std::string& title);
  /// A step's progress or state changed: redraw its result line (#810).
  void resultsChanged(const EcceURL& url);
  /// The result line of every box, "name: text|...", for the test hook.
  std::string resultLines();
  /// The key-data box as text ("" for a workflow made without a recipe).
  std::string keyDataText();
  /// The Rate box as plain lines (T set first when given); the details CSV for a range.
  std::string rateText(const std::string& T = "");
  std::string rateDetailsCsv(const std::string& file, const std::string& range);
  /// The Fragment orbitals box as plain lines; its Details window, shown
  /// without blocking (null when the box has no result).
  std::string fragOrbText();
  FragOrbDetailsDialog *fragOrbDetails();

  /// Geometry of the drawing, for the screenshot test.
  std::string layoutReport();
  /// The Launch Ready Steps button: its label and whether it is on screen.
  std::string launchButtonReport();

private:
  friend class WorkflowCanvas;
  void createControls();
  void updateTexts();
  void rewrapHelp();
  void joinClicked(const WorkflowSteps::Code& code, ResourceType *rt,
                   const std::vector<Resource*>& sources);

  CalcMgr *p_calcMgr;
  Session *p_session;
  EcceURL p_selected;
  std::vector<EcceURL> p_picked;
  WorkflowCanvas *p_canvas;
  wxTextCtrl *p_help;
  wxString p_helpRaw;
  wxStaticText *p_status;
  wxButton *p_launch;
  wxButton *p_close;
  wxCheckBox *p_autorun;
  wxButton *p_apply;
  wxPanel *p_empty;
  wxBoxSizer *p_left;
  wxBoxSizer *p_top;
  RecipePanel *p_recipe;
  bool p_vertical;
  wxToolBar *p_toolbar;
  int p_orientationId;
};


/// Asks for a new step's name and what it takes from its source.
class WorkflowStepDialog : public wxDialog
{
public:
  WorkflowStepDialog(wxWindow *parent, Session *wf,
                     const WorkflowSteps::Code& code, ResourceType *type,
                     Resource *source);

  std::string name() const;
  WorkflowSteps::Carry carry() const;
  /// Whether the dialog offers the carry-over boxes (a step with a source).
  bool hasCarry() const { return p_geometry != 0; }

private:
  void OnName(wxCommandEvent& event);
  void validate();

  Session *p_session;
  ResourceType *p_type;
  wxTextCtrl *p_name;
  PointChooser *p_point;
  wxCheckBox *p_geometry;
  wxCheckBox *p_charge;
  wxCheckBox *p_hessian;
  wxCheckBox *p_best;
  wxSpinCtrl *p_bestN;
  wxCheckBox *p_patterns;
  wxStaticText *p_problem;
  wxButton *p_create;
};



/// Make Workflow from Selected... / Move Selected into Workflow (#874): the
/// new workflow's name (none when moving into an existing one) and which of
/// the proposed links to make.
class WorkflowImportDialog : public wxDialog
{
public:
  WorkflowImportDialog(wxWindow *parent, size_t calcs,
                       const std::vector<WorkflowSteps::ProposedLink>& links,
                       ResourceType *workflowType, const std::string& newName,
                       const std::string& into);

  std::string name() const;
  /// For each proposed link: still ticked.
  std::vector<bool> chosen() const;
  /// "A → B (geometry)", with what was recorded when it was.
  static std::string label(const WorkflowSteps::ProposedLink& link);
  /// "A → B (geometry, Hessian)": the list entry; label() is its tooltip.
  static std::string shortLabel(const WorkflowSteps::ProposedLink& link);

private:
  void validate();

  ResourceType *p_type;
  wxTextCtrl *p_name;
  wxCheckListBox *p_links;
  std::vector<wxString> p_reasons;
  wxStaticText *p_problem;
  wxButton *p_ok;
};


/// Asks for a join's name and the role of each source (reactant, product,
/// TS guess), and the scan point of each source that is a scan.
class WorkflowJoinDialog : public wxDialog
{
public:
  WorkflowJoinDialog(wxWindow *parent, Session *wf,
                     const WorkflowSteps::Code& code, ResourceType *type,
                     const std::vector<Resource*>& sources);

  std::string name() const;
  std::vector<WorkflowSteps::JoinSource> sources() const;

private:
  void validate();

  Session *p_session;
  ResourceType *p_type;
  WorkflowSteps::Code p_code;
  std::vector<Resource*> p_steps;
  wxTextCtrl *p_name;
  std::vector<wxChoice*> p_role;
  std::vector<PointChooser*> p_points;
  wxStaticText *p_what;
  wxStaticText *p_problem;
  wxButton *p_create;
};

#endif
