/**
 * @file
 *
 * A Builder window inside the Organizer's process (#983): what
 * BuilderApp::OnInit does for a start from the Organizer, without the
 * once-per-process part (login, preferences, the app's own listeners).
 */
#include "dsm/ResourceDescriptor.H"

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
  InProcessTools::add(BUILDER, openBuilder);
  InProcessTools::add(CALCVIEWER, openViewer);
}
