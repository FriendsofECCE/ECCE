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

#include <string>
#include <vector>

#include "util/EcceURL.H"
#include "wxgui/ewxPanel.H"

#include "WorkflowSteps.H"

class CalcMgr;
class Resource;
class ResourceType;
class Session;
class WorkflowCanvas;
class WxResourceTreeItemData;
class PointChooser;

class WorkflowPanel : public ewxPanel
{
public:
  WorkflowPanel(CalcMgr *calcMgr, WxResourceTreeItemData *node,
                wxWindow *parent);

  void refresh();
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

  /// Geometry of the drawing, for the screenshot test.
  std::string layoutReport();

private:
  friend class WorkflowCanvas;
  void createControls();
  void updateTexts();
  void joinClicked(const WorkflowSteps::Code& code, ResourceType *rt,
                   const std::vector<Resource*>& sources);

  CalcMgr *p_calcMgr;
  Session *p_session;
  EcceURL p_selected;
  std::vector<EcceURL> p_picked;
  WorkflowCanvas *p_canvas;
  wxStaticText *p_help;
  wxStaticText *p_status;
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

private:
  void OnName(wxCommandEvent& event);
  void validate();

  Session *p_session;
  ResourceType *p_type;
  wxTextCtrl *p_name;
  PointChooser *p_point;
  wxCheckBox *p_geometry;
  wxCheckBox *p_charge;
  wxStaticText *p_problem;
  wxButton *p_create;
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
