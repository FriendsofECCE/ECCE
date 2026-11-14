/**
 * @file
 *
 * The Builder window's messages: a dispatcher of its own instead of the
 * wxApp, so that several tool windows can share one process (#983), as
 * MDPrep and PartialCharge already do.
 */

#ifndef BUILDERMESSAGES_HH
#define BUILDERMESSAGES_HH

#include "util/JMSMessage.H"
#include "wxgui/WxJMSMessageDispatch.H"

class Builder;

class BuilderMessages : public WxJMSMessageDispatch
{
public:
  BuilderMessages(const string& name, Builder *builder);

  void subscribeMessages();
  // An ecce_invoke has arrived (a Viewer without one gives up).
  bool invokeReceived() const { return p_invokeReceived; }

  void invokeMCB(JMSMessage& msg);
  void quitMCB(JMSMessage& msg);
  void identifyMCB(JMSMessage& msg);
  void invokeStatusMCB(JMSMessage& msg);

  void unitsPrefsMCB(JMSMessage& msg);
  void pertabPrefsMCB(JMSMessage& msg);
  void miscPrefsMCB(JMSMessage& msg);
  void ffConfigEditMCB(JMSMessage& msg);

  void urlRenameMCB(JMSMessage& msg);
  void urlRemoveMCB(JMSMessage& msg);
  void urlStateMCB(JMSMessage& msg);

  void propertyChangeMCB(JMSMessage& msg);
  void detailsMCB(JMSMessage& msg);
  void vinaBoxMCB(JMSMessage& msg);

  void authChangedMCB(JMSMessage& msg);

  void setSelectionMCB(JMSMessage& msg);

  string identify();

private:
  Builder *p_builder;
  bool p_invokeReceived = false;
};

#endif
