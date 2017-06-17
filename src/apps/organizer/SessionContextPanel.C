/**
 * @file
 *
 * @author
 *
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
using std::cerr;

#include "wx/colordlg.h"
#include "wx/tooltip.h"

#include "util/EcceURL.H"

#include "dsm/Session.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/WxState.H"

#include "CalcMgr.H"
#include "MDTaskPanel.H"
#include "SessionContextPanel.H"


/**
 * Constructor with parameters.
 */
SessionContextPanel::SessionContextPanel( CalcMgr *calcmgr,
                                          WxResourceTreeItemData * node,
                                          wxWindow *parent,
                                          wxWindowID id,
                                          const wxPoint& pos,
                                          const wxSize& size,
                                          long style):
  ewxPanel(parent, id, pos, size, style),
  ContextPanelInterface(calcmgr)
{
  p_session = dynamic_cast<Session *>(node->getResource()),
  p_lastHighlighted = EcceURL();

  Preferences *prefs = new Preferences("Organizer");
  if (!prefs->getBool("Organizer.Session.Workflow.Vertical", p_isVertical)) {
    p_isVertical = true;
  }
  delete prefs;

  // create all items within the panel
  createControls();
}


/**
 * Destructor.
 * The cached session is really owned by the Organizer's tree, so we do not
 * delete it here.
 */
SessionContextPanel::~SessionContextPanel()
{
  p_session = 0;
}


/**
 * Create all items within the panel.
 */
void SessionContextPanel::createControls()
{
  // create the main sizer for the entire panel
  wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(topSizer);

  // create sizer to contain workflow graph and legend
  wxBoxSizer *workflowAndLegendSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(workflowAndLegendSizer, 1, wxGROW|wxALL, 0);

  // create the workflow graph scrolled window and its own internal sizer
  p_workflow = new SessionWorkflow(this);
  wxBoxSizer *workflowSizer = new wxBoxSizer(p_isVertical ? wxVERTICAL :
                                                            wxHORIZONTAL);
  p_workflow->SetSizer(workflowSizer);
  workflowAndLegendSizer->Add(p_workflow, 1, wxGROW|wxALL, 0);

  // get all of the tasks which are roots
  // for each root task, build their respective trees in the workflow's sizer
  vector<Resource*> roots;
  roots = p_session->getRootMembers(); 
  workflowSizer->AddStretchSpacer(1);

  // special hardwired STTR logic for displaying reaction studies
  if (p_session->getApplicationType()==ResourceDescriptor::AT_REACTION_STUDY &&
      roots.size()>1) {
    // re-order the roots vector so transition state is in the middle which
    // makes the workflow aesthetically pleasing
    vector<Resource*> rxnRoots;
    vector<Resource*>::iterator root;

    // since there are only a few root nodes possible in a reaction study
    // (5 max), don't worry about inefficiency of iterating over the same
    // nodes multiple times in order to find what we want and re-order
    for (root = roots.begin(); root != roots.end(); root++) {
      if ((*root)->getName().find("Reactant") == 0)
        rxnRoots.push_back(*root);
    }
    for (root = roots.begin(); root != roots.end(); root++) {
      if ((*root)->getName().find("Transition-State") == 0)
        rxnRoots.push_back(*root);
        // definitely only one of these so save a nanosecond and break
        break;
    }

    for (root = roots.begin(); root != roots.end(); root++) {
      if ((*root)->getName().find("Product") == 0)
        rxnRoots.push_back(*root);
    }

    showTargets(workflowSizer, rxnRoots);
  } else {
    showTargets(workflowSizer, roots);
  }

  // Add a legend for the graph
  ewxPanel *legend = new ewxPanel(this, wxID_ANY, wxDefaultPosition, 
                                  wxDefaultSize, wxNO_BORDER);
  wxBoxSizer *legendSizer = new wxBoxSizer(wxHORIZONTAL);
  legend->SetSizer(legendSizer);
  ewxButton *button;
  wxSize buttonSize(20,10);

  legendSizer->Add(new ewxStaticText(legend, wxID_STATIC, "Workflow Paths:"),
                   0, wxALL, 2);

  // primary path button, label, and event handling
  button = new ewxButton(legend, ID_COLOR_PRIMARY_PATH, "",
                         wxDefaultPosition, buttonSize);
  button->SetBackgroundColour(getPathColor(Session::PRIMARY_PATH));
  button->SetToolTip("Change path color");
  legendSizer->Add(new ewxNonBoldLabel(legend, wxID_STATIC, "Current"),
                   0, wxALL, 2);
  legendSizer->Add(button, 0, wxALL, 5);
  Connect(ID_COLOR_PRIMARY_PATH, wxEVT_COMMAND_BUTTON_CLICKED, 
          wxCommandEventHandler(SessionContextPanel::pathColorClick));

  // input provider button, label, and event handling
  button = new ewxButton(legend, ID_COLOR_INPUT_PROVIDER, "",
                         wxDefaultPosition, buttonSize);
  button->SetBackgroundColour(getPathColor(Session::INPUT_PROVIDER));
  button->SetToolTip("Change path color");
  legendSizer->Add(new ewxNonBoldLabel(legend, wxID_STATIC, "Branches"),
                   0, wxALL, 2);
  legendSizer->Add(button, 0, wxALL, 5);
  Connect(ID_COLOR_INPUT_PROVIDER, wxEVT_COMMAND_BUTTON_CLICKED, 
          wxCommandEventHandler(SessionContextPanel::pathColorClick));

  workflowAndLegendSizer->Add(new ewxStaticLine(this, -1), 0, wxGROW, 0);

  workflowAndLegendSizer->Add(legend, 0, wxGROW|wxALL, 0);

  // Add a toolbar in the main sizer
  p_toolBar = new wxToolBar(this,
                            wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxTB_FLAT|wxTB_VERTICAL|wxNO_BORDER|wxWANTS_CHARS);
  p_toolBar->SetToolPacking(1);
  
  ResourceType * resType = p_session->getDescriptor();
  
  if (resType != 0) {
    vector<ResourceType *> resVec = resType->getContains();
    vector<ResourceType *>::iterator resItor = resVec.begin();
    for (;resItor != resVec.end(); resItor++) {
      p_toolBar->AddTool((*resItor)->getId(),
                         (*resItor)->getMenuItem(),
                         ewxBitmap((*resItor)->getIcon()),
                         (*resItor)->getMenuItem(), wxITEM_NORMAL);
      if ((*resItor)->getMenuItem() == "New NWChem MD Prepare..." &&
          p_session->numMembers() == 0)
        resItor += 3;
      else if ((*resItor)->getMenuItem() ==
               "New Direct Dynamics VTST Calculation..." &&
               p_session->numMembers() == 0)
        resItor += 5;
    }
  }

  p_toolBar->AddSeparator();

  // create toggle for workflow orientation
  ewxBitmap orientation(p_isVertical? "vtree16.xpm":"htree16.xpm");
  p_toolBar->AddTool(ID_ORIENTATION, "Orientation", orientation, wxNullBitmap,
                   wxITEM_NORMAL, "Change tree orientation");

  Connect(ID_ORIENTATION, wxEVT_COMMAND_TOOL_CLICKED, 
          wxCommandEventHandler(SessionContextPanel::orientationClick));

  topSizer->Add(p_toolBar, 0, wxGROW|wxALL, 0);

  // now that showTargets is finished recursively, adjust the sizers
  Layout();

  // This was removed because it was causing the tool bar to be pushed off the
  // screen when any actions occurred in the context panel.
  // adjust panel to fit everything
  //Fit();
}



/**
 * Updates the panel.
 */
void SessionContextPanel::refresh()
{
  // get rid of all the mdtaskpanels and the sizers
  DestroyChildren();      // destroys MDTaskPanels

  // Repeated step, also in createControls, but more appropriate there
  //SetSizer(new wxBoxSizer(wxHORIZONTAL)); // deletes the old sizer

  // build the hierarchy of mdtaskpanels
  createControls();

  // attempt to reselect original selection
  selectResource(p_lastHighlighted);

  Refresh();
}



/**
 * Recursive function to build session tree.
 * Each task is represented by a button which will select the task in the
 * Organizer's tree when clicked.  Horizontal sizers are created when the tree
 * branches (when a Task has more than one target.)  Recursion ends when a leaf 
 * of the tree is reached (the Task vector targets is empty.)
 *
 * @param parentSizer   the sizer to insert the new task/sizer into
 * @param targets       the targets with the given linkType of the source Task
 * @param linkType      the kind of links from which the tree is built
 */
void SessionContextPanel::showTargets( wxBoxSizer *parentSizer, 
                                       vector<Resource*> targets,
                                       Session::LinkType linkType )
{
  vector<Resource*>::iterator target;

  for (target = targets.begin(); target != targets.end(); target++) {
    // create a vertical sizer to house the task panel
    wxBoxSizer *vSizer = new wxBoxSizer(p_isVertical ? wxHORIZONTAL :
                                                       wxVERTICAL);

    // add the new vertical sizer to the parent sizer
    parentSizer->Add(vSizer,
        0,          // make vertically stretchable
        wxEXPAND);  // make horizontally stretchable

    parentSizer->AddStretchSpacer(1);

    // create the task panel
    MDTaskPanel *taskPanel = new MDTaskPanel(
        (*target),
        this,
        p_workflow,
        (*target)->getSessionIndex());

    // place the task panel into it's sizer
    vSizer->Add(taskPanel,
        0,                          // make vertically unstretchable
        wxALL |                     //    and make border all around
        (p_isVertical ? wxALIGN_CENTER_VERTICAL : wxALIGN_CENTER_HORIZONTAL),
        10 );                        // set border width to 10

    // create a horizontal sizer for the task panel's children
    wxBoxSizer *hSizer = new wxBoxSizer(p_isVertical ? wxVERTICAL :
                                                       wxHORIZONTAL);

    hSizer->AddStretchSpacer(1);

    // add the children sizer to it's parent
    vSizer->Add(hSizer,
        1,          // make vertically stretchable
        wxEXPAND);  // make horizontally stretchable

    // fill the task panel's children sizer with its children
    showTargets(hSizer, p_session->getTargets((*target), linkType), linkType);
  }
}



/**
 * Uses a simple line drawing to link tasks related by their linkTypes.
 * Currently supports a 3 pixel wide black line for INPUT_PROVIDER links,
 * a 1 pixel wide green line for PRIMARY_PATH links, and defaults to a 1 pixel
 * wide black line if the linkType is unrecognized.  The order in which the
 * links are drawn is important as in a painting, the most resently drawn 
 * obscures the previous one.
 *
 * @param linkType  the LinkType mentioned above
 */
void SessionContextPanel::drawConnections(Session::LinkType linkType)
{
  // set drawing canvas and pen
  // note wxWidgets documentation says not to cache DC objects - we don't here
  wxPaintDC dc(p_workflow);
  wxPen pen;

  switch (linkType) {
    case Session::INPUT_PROVIDER:
      pen.SetWidth(4); break;
    case Session::PRIMARY_PATH:
      pen.SetWidth(2); break;
    default:
      pen.SetWidth(4); break;
  }
  pen.SetColour(getPathColor(linkType));
  dc.SetPen(pen);

  // special reaction rate study logic to calculate the maxiumum width of
  // all root nodes because this will be needed instead of just using the
  // current node width to avoid lines drawing over nodes
  // only the vertical orientation needs this calculation because that's
  // where the variable node width can cause problems
  int maxRootWidth = 0;
  if (p_session->getApplicationType()==ResourceDescriptor::AT_REACTION_STUDY &&
      p_isVertical) {
    vector<Resource*> roots = p_session->getRootMembers(); 
    if (roots.size() > 1) {
      vector<Resource*>::iterator root;
      for (root = roots.begin(); root != roots.end(); root++) {
        wxWindow *rootWin = FindWindow(_T((*root)->getURL().toString()));
        if (rootWin!=0 && rootWin->GetRect().GetRight()>maxRootWidth)
          maxRootWidth = rootWin->GetRect().GetRight();
      }
    }
  }

  // draw input provider connections
  // get all of the tasks and draw link from each to its target, if one exists
  // for the given type
  // if the correct type of link exists between source and target, get the 
  // rectangle boundaries of the buttons using FindWindow and the unique URLs
  // of the Tasks as keys.
  vector<Resource*> tasks = p_session->getMembers();
  vector<Resource*>::iterator task;

  for (task = tasks.begin(); task != tasks.end(); task++) {
    vector<Resource*> targets = p_session->getTargets((*task), linkType);
    vector<Resource*>::iterator target;
    wxWindow *parent = FindWindow(_T((*task)->getURL().toString()));
    if (parent != 0) {
      for (target = targets.begin(); target != targets.end(); target++) {
        wxWindow *child = FindWindow(_T((*target)->getURL().toString()));
        if (child != 0) {
          int px, py, cx, cy, between;
          if (p_isVertical) {
            px = parent->GetRect().GetRight();
            py = (parent->GetRect().GetTop() + parent->GetRect().GetBottom())/2;
            cx = child->GetRect().GetLeft();
            cy = (child->GetRect().GetTop() + child->GetRect().GetBottom())/2;

            // figure out whether the reaction rate study calculation for
            // the maximum root node width is needed or not
            if (maxRootWidth > px)
              between = maxRootWidth + 10;
            else
              between = px + 10;

            dc.DrawLine(px, py, between, py);
            dc.DrawLine(between, py, between, cy);
            dc.DrawLine(between, cy, cx, cy);
          } else {
            px = (parent->GetRect().GetRight() + parent->GetRect().GetLeft())/2;
            py = parent->GetRect().GetBottom();
            cx = (child->GetRect().GetRight() + child->GetRect().GetLeft())/2;
            cy = child->GetRect().GetTop();
            between = py + 10;
            dc.DrawLine(px, py, px, between);
            dc.DrawLine(px, between, cx, between);
            dc.DrawLine(cx, between, cx, cy);
          }
        }
      }
    }
  }

  // pens for DC objects must be 'unset' when finished
  dc.SetPen(wxNullPen);
}



/**
 * Returns single selection of mdtask if one is selected.
 *
 * @return  always a length 1 vector of task pointer (as a resource pointer)
 */
vector<EcceURL> SessionContextPanel::getSelections()
{
  vector<EcceURL> ret;

  if (p_lastHighlighted != EcceURL()) {
    ret.push_back(p_lastHighlighted);
  }

  return ret;
}



/**
 * Color the currently selected MDTaskPanel the unselected color.
 */
void SessionContextPanel::loseFocus()
{
  if (p_lastHighlighted != EcceURL()) {
    MDTaskPanel * currentSelection = findTaskPanel(p_lastHighlighted);
    if (currentSelection) {
      currentSelection->setBackground(MDTaskPanel::LOSTFOCUS);
    }
  }
}



/**
 * Returns the color for the given LinkType from the preferences file.
 */
wxColour SessionContextPanel::getPathColor(Session::LinkType linkType)
{
  wxColour ret;

  // set up defaults and preference key
  string prefKey;
  switch (linkType) {
    case Session::INPUT_PROVIDER:
      prefKey = "Organizer.Session.Workflow.Color.InputProvider";
      ret = (*wxBLACK);
      break;
    case Session::PRIMARY_PATH:
      prefKey = "Organizer.Session.Workflow.Color.PrimaryPath";
      ret = (*wxGREEN);
      break;
    default:
      ret = (*wxBLACK);
      break;
  }

  // override defaults with preference if it exists
  vector<int> rgb;
  Preferences *prefs = new Preferences("Organizer");
  if (prefs->getIntList(prefKey, rgb)) {
    ret.Set(rgb[0], rgb[1], rgb[2]);
  }
  delete prefs;

  return ret;
}


/**
 * Useful such as during a rename, since this panel is not smart enough to know
 * which url points to the old selected resource.
 *
 * @param url   The url which looks up the window to select
 */
void SessionContextPanel::selectResource(const EcceURL & url)
{
  if (url != p_lastHighlighted) {
    MDTaskPanel * lastSelection = findTaskPanel(p_lastHighlighted);
    if (lastSelection)
      lastSelection->setBackground(MDTaskPanel::UNSELECTED);
  }

  MDTaskPanel * currentSelection = findTaskPanel(url);
  if (currentSelection) {
    currentSelection->setBackground(MDTaskPanel::SELECTED);
    p_lastHighlighted = url;
    // scroll the panel to make sure selection is visible
    ensureVisible(url);
  } else {
    ensureVisiblePrimaryLeaf();
  }

  Refresh();
}



/**
 * Scrolls the window so that the given mdtask panel is visible.
 *
 * @param url   The url which looks up the window to make visible
 */
void SessionContextPanel::ensureVisible(const EcceURL & url)
{
  MDTaskPanel * panel = findTaskPanel(url);
  if (!panel)
    return;

  int clientSizeX, clientSizeY;
  wxRect rect;
  int viewStartX, viewStartY;
  int pixelsPerUnitX, pixelsPerUnitY;
  p_workflow->GetClientSize(&clientSizeX, &clientSizeY);
  rect = panel->GetRect();
  p_workflow->GetViewStart(&viewStartX, &viewStartY);
  p_workflow->GetScrollPixelsPerUnit(&pixelsPerUnitX, &pixelsPerUnitY);

  int xFix = 0, yFix = 0;
  if (rect.GetLeft() < 0) { // left of panel is offscreen left
    xFix = (int) floor((double)rect.GetLeft() / pixelsPerUnitX);
  }
  if (rect.GetTop() < 0) { // top of panel is offscreen top
    yFix = (int) floor((double)rect.GetTop() / pixelsPerUnitY);
  }
  if (rect.GetRight() > clientSizeX) { // right of panel is offscreen right
    xFix = (int) ceil((double)(rect.GetRight() - clientSizeX) / pixelsPerUnitX);
  }
  if (rect.GetBottom() > clientSizeY) { // bottom of panel is offscreen bottom
    yFix = (int) ceil((double)(rect.GetBottom() - clientSizeY) /pixelsPerUnitY);
  }

  p_workflow->Scroll(viewStartX + xFix, viewStartY + yFix);
}



/**
 * Scrolls the window so that the leaf of the primary path is visible.
 */
void SessionContextPanel::ensureVisiblePrimaryLeaf()
{
  vector<Resource *> resources;
  resources = p_session->getLeafMembers(Session::PRIMARY_PATH);
  if (!resources.empty()) {
    ensureVisible(resources[0]->getURL());
  }
}



/**
 * Whenever the window needs to be redrawn, this callback gets called.
 * wxWidgets documentation says a wxPaintDC object must be created regardless
 * of actually performing any drawing during this function.  This requirement
 * is maintained in the drawConnections function calls.
 */
void SessionContextPanel::paint( wxPaintEvent& event )
{
  // the order matters here, as input provider is a solid black line of width 3
  // while the primary path draws over with width of 1 in green
  drawConnections(Session::UNSPECIFIED_LINKTYPE);
  drawConnections(Session::INPUT_PROVIDER);
  drawConnections(Session::PRIMARY_PATH);
  // don't skip the event, the buck stops here
}


void SessionContextPanel::size( wxSizeEvent& event )
{
  // invalidate the whole session context window to force a redraw
  Refresh();

  event.Skip();
}



/**
 * Whenever a recognized key is pressed, execute an action by the organizer.
 */
void SessionContextPanel::keyDown( wxKeyEvent& event )
{
  bool skipEvent = true;
  int keyCode = event.GetKeyCode();

  if (p_lastHighlighted != EcceURL()) {

    if (event.ControlDown()) {

      switch (keyCode) {
        case 'x':
        case 'X':
          p_calcMgr->copySelectionToClipboard(true);
          skipEvent = false;
          break;
        case 'c':
        case 'C':
          p_calcMgr->copySelectionToClipboard(false);
          skipEvent = false;
          break;
        case 'v':
        case 'V':
          p_calcMgr->paste();
          skipEvent = false;
          break;
      }
    }
    else {

      switch (keyCode) {
        case WXK_DELETE:
        case WXK_BACK:
          p_calcMgr->deleteSelection();
          skipEvent = false;
          break;
        case WXK_RETURN:
          p_calcMgr->findNode(p_lastHighlighted, true);
          skipEvent = false;
          break;
      }
    }
  }

  // Skip to default handling if key down was
  // not handled here
  if (skipEvent) {
    event.Skip();
  }
}


/**
 * Swap the workflow graph orientation when tool button is clicked.
 */
void SessionContextPanel::orientationClick( wxCommandEvent& event )
{
  /*
  ewxBitmap orientation(p_isVertical? "vtree.xpm":"htree.xpm");
  p_toolBar->AddTool(ID_ORIENTATION, "Orientation", orientation, wxNullBitmap,
                     wxITEM_NORMAL, "Change tree orientation");
  */

  p_isVertical = !p_isVertical;

  p_toolBar->DeleteTool(ID_ORIENTATION);
  ewxBitmap orientation(p_isVertical? "vtree16.xpm":"htree16.xpm");
  p_toolBar->AddTool(ID_ORIENTATION, "Orientation", orientation, wxNullBitmap,
                     wxITEM_NORMAL, "Change tree orientation");

  Preferences *prefs = new Preferences("Organizer");
  prefs->setBool("Organizer.Session.Workflow.Vertical", p_isVertical);
  prefs->saveFile();
  delete prefs;

  refresh();
}


/**
 * Use the color chooser wx dialog to pick a path color.
 */
void SessionContextPanel::pathColorClick( wxCommandEvent& event )
{
  wxColour oldcolor = (event.GetId()==ID_COLOR_PRIMARY_PATH)?
    getPathColor(Session::PRIMARY_PATH):getPathColor(Session::INPUT_PROVIDER);    
  ewxColorDialog * colorDialog =
    new ewxColorDialog(oldcolor, this);
  if (colorDialog->ShowModal() == wxID_OK) {
    ewxColor col = colorDialog->GetColor();

    string key;
    if (event.GetId() == ID_COLOR_PRIMARY_PATH) {
      key = "Organizer.Session.Workflow.Color.PrimaryPath";
    } else if (event.GetId() == ID_COLOR_INPUT_PROVIDER) {
      key = "Organizer.Session.Workflow.Color.InputProvider";
    }

    Preferences *prefs = new Preferences("Organizer");
    vector<int> rgb;
    rgb.push_back(col.Red());
    rgb.push_back(col.Green());
    rgb.push_back(col.Blue());
    prefs->setIntList(key, rgb);
    prefs->saveFile();
    delete prefs;

    refresh();
  }
}


MDTaskPanel * SessionContextPanel::findTaskPanel(const EcceURL & url)
{
  MDTaskPanel * ret = 0;
  wxWindow *window = FindWindow(url.toString());
  if (window)
    ret = dynamic_cast<MDTaskPanel*>(window);
  return ret;
}


CalcMgr * SessionContextPanel::getCalcMgr()
{
  return p_calcMgr;
}


SessionWorkflow::SessionWorkflow(SessionContextPanel * parent, wxWindowID id):
  ewxScrolledWindow(parent, id)
{
  p_parent = parent;
  SetScrollbars(10, 10, 0, 0);
  Connect(wxEVT_PAINT, 
          wxPaintEventHandler(SessionWorkflow::OnPaint));
  Connect(wxEVT_SIZE, 
          wxSizeEventHandler(SessionWorkflow::OnSize));
  Connect(wxEVT_KEY_DOWN,
          wxKeyEventHandler(SessionWorkflow::OnKeyDown));
}


void SessionWorkflow::OnPaint( wxPaintEvent& event )
{
  p_parent->paint(event);
}


void SessionWorkflow::OnSize( wxSizeEvent& event )
{
  p_parent->size(event);
}


void SessionWorkflow::OnKeyDown( wxKeyEvent& event )
{
  p_parent->keyDown(event);
}
