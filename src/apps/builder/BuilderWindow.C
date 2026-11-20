/**
 * @file
 *
 * A Builder window inside the Organizer's process (#983): what
 * BuilderApp::OnInit does for a start from the Organizer, without the
 * once-per-process part (login, preferences, the app's own listeners).
 */
#include "dsm/ResourceDescriptor.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/SGViewer.H"

#include "inv/actions/SoSearchAction.H"

#include "wxgui/InProcessTools.H"
#include "wxgui/ToolTabs.H"

#include "Builder.H"
#include "BuilderMessages.H"


// The Viewer is the Builder under another name (ECCE_INVOKE_VIEWER in its
// own process): its windows are counted and matched apart from Builders.
static WxJMSMessageDispatch *openTool(const string& app, const wxString& title)
{
  Builder *builder = new Builder(NULL, false);
  BuilderMessages *msgs = new BuilderMessages(app, builder);
  builder->setMessages(msgs);
  builder->setInProcess(true);
  if (app == CALCVIEWER) {
    builder->setViewerApp(true);
    builder->SetTitle(_("ECCE Viewer"));
  }
  // The invoke that follows raises registered shells, then fills this one.
  msgs->registerTopShell(builder);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(app, msgs, builder);
  // Before the frame is first shown, so the viewer is first realized in
  // the tab.
  ToolTabs::open(builder, builder->auiManager(), title, "builder");
  return msgs;
}


static WxJMSMessageDispatch *openBuilder() { return openTool(BUILDER, _("Builder")); }
static WxJMSMessageDispatch *openViewer() { return openTool(CALCVIEWER, _("Viewer")); }


void registerBuilderInProcess()
{
  // "builderinfo": what the active Builder or Viewer tab draws, so a test
  // can tell that it shows the molecule without a screenshot (#1122).
  ToolTabs::addCommand("builderinfo", [](const wxString&) -> wxString {
    Builder *b = dynamic_cast<Builder*>(ToolTabs::activeTool());
    if (!b) return "not a Builder tab";
    SGContainer *sg = b->getSG();
    SGFragment *frag = sg ? sg->getFragment() : NULL;
    //  "inscene": the fragment's scene graph is under the viewer's root,
    //  i.e. drawn, not only loaded (#1209).
    int inScene = -1;
    if (sg) {
      SoNode *root = b->getViewer().getSceneGraph();
      inScene = 0;
      if (root) {
        SoSearchAction find;
        find.setNode(sg);
        find.setInterest(SoSearchAction::FIRST);
        find.apply(root);
        inScene = find.getPath() != NULL;
      }
    }
    return wxString::Format("atoms=%d inscene=%d title=%s",
                            frag ? (int)frag->numAtoms() : -1, inScene,
                            b->GetTitle());
  });
  InProcessTools::add(BUILDER, openBuilder);
  InProcessTools::add(CALCVIEWER, openViewer);
}
