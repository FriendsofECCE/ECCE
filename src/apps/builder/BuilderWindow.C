/**
 * @file
 *
 * A Builder window inside the Organizer's process (#983): what
 * BuilderApp::OnInit does for a start from the Organizer, without the
 * once-per-process part (login, preferences, the app's own listeners).
 */
#include "dsm/ResourceDescriptor.H"

#include "wxgui/InProcessTools.H"

#include "Builder.H"
#include "BuilderMessages.H"


static WxJMSMessageDispatch *openBuilder()
{
  Builder *builder = new Builder(NULL, false);
  BuilderMessages *msgs = new BuilderMessages(BUILDER, builder);
  builder->setMessages(msgs);
  builder->setInProcess(true);
  // The invoke that follows raises registered shells, then fills this one.
  msgs->registerTopShell(builder);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(BUILDER, msgs, builder);
  return msgs;
}


void registerBuilderInProcess()
{
  InProcessTools::add(BUILDER, openBuilder);
}
