/**
 * @file
 * @author Lisong Sun
 *
 * Calculation Manager Main Application Class
 * 
 *
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CalcMgrApp.H"
#endif

#include <thread>

#include "util/TempStorage.H"
#include <unistd.h>
#include <stdio.h>
#include "util/MqttBroker.H"
#include "util/MqttLink.H"
#include "wxgui/InProcessTools.H"
#include "util/JMSPublisher.H"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include <wx/timer.h>
#endif

//#include <iostream>
//using std::cerr;

#include "tdat/RefMachine.H"
#include "tdat/QueueMgr.H"

#include "dsm/MachineConfig.H"
#include "dsm/EDSIFactory.H"

#include "wxgui/ewxSplash.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/WxState.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/LocalDataMove.H"

#include "CalcMgrApp.H"
#include "CalcMgr.H"

#ifdef __WXMSW__
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "util/Ecce.H"

// The login window's "Use this computer instead": this start was set up for
// the server, so ecce.cmd runs again, once, for a local session and without
// the first-start question, as `ecce` does on Linux.  ECCE_SWITCHED_LOCAL
// carries this process's id: ecce.cmd waits until this Organizer has ended,
// so its session ends first.  This process keeps its own environment (its
// session id) for that; the child gets its own block.
static void restartInLocalMode()
{
  const char *root = getenv("ECCE_ROOT");
  string script = (root && *root ? string(root) : string(Ecce::ecceHome())) +
                  "\\ecce.cmd";
  std::vector<string> vars;
  if (char *block = GetEnvironmentStringsA()) {
    for (char *v = block; *v; v += strlen(v) + 1) {
      string s(v), name = s.substr(0, s.find('='));
      if (name != "ECCE_REMOTE_SERVER" && name != "ECCE_REMOTE_DIR" &&
          name != "ECCE_SERVER_SESSION" && name != "ECCE_SESSION_ID" &&
          name != "ECCE_SWITCHED_LOCAL")
        vars.push_back(s);
    }
    FreeEnvironmentStringsA(block);
  }
  vars.push_back("ECCE_SWITCHED_LOCAL=" +
                 std::to_string((unsigned long)GetCurrentProcessId()));
  string env;
  for (const string& v : vars) env += v + '\0';
  env += '\0';
  // Its output to a file: started with no console window and no standard
  // handles, the session scripts' messages fail to write and ecce.cmd
  // takes the broker start for failed.
  const char *home = getenv("ECCE_REALUSERHOME");
  string log = string(home && *home ? home : ".") + "/.ECCE/switch-to-local.log";
  const char *comspec = getenv("COMSPEC");
  string line = "\"" + string(comspec && *comspec ? comspec : "cmd.exe") +
                "\" /d /s /c \"\"" + script + "\" > \"" + log + "\" 2>&1\"";
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  if (CreateProcessA(NULL, &line[0], 0, 0, FALSE, CREATE_NO_WINDOW, &env[0],
                     0, &si, &pi)) {
    fprintf(stderr, "Organizer: starting again in local mode: %s\n",
            line.c_str());
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  } else {
    fprintf(stderr, "Organizer: could not run %s (error %lu)\n",
            line.c_str(), (unsigned long)GetLastError());
  }
}
#endif


// In builder-objs and calced-objs (#983).
void registerBuilderInProcess();
void registerCalcEdInProcess();
void registerMDOptimizeInProcess();
void registerMDEnergyInProcess();
void registerMDDynamicsInProcess();
void registerGmxPrepareInProcess();
void registerBasisToolInProcess();
void registerMDPrepInProcess();

IMPLEMENT_APP(CalcMgrApp)

IMPLEMENT_CLASS(CalcMgrApp, ewxApp)

/**
 * Default constructor
 *
 * Subclassed from WxJMSMessageDispatch
 */
CalcMgrApp::CalcMgrApp()
  : ewxApp(), WxJMSMessageDispatch(ORGANIZER.c_str())
{
  p_calcMgr = NULL;
}

string CalcMgrApp::getName() const
{
   return "Organizer";
}


/**
 * Initialize CalcMgr application
 *
 * Generate a new CalcMgr object and show it.
 * Subscribe to the misc message event.
 */
bool CalcMgrApp::OnInit()
{
  // Only this process runs tools in-process (#983); its children do not.
  MqttLink::takeOneProcessSwitch(true);
  // The session's own broker (#990), before anything connects to it.
  // ECCE_BROKER=mosquitto keeps the old ecce-broker-win (ecce.cmd).  Only
  // ecce.cmd sets ECCE_LOCAL_BROKER: on Linux and macOS ecce-gateway-start
  // runs the same broker as a program (eccebroker --serve, #1007), since the
  // gateway connects before this Organizer exists; with the variable set
  // here anyway, startForSession adopts that broker.
  {
    wxString which;
    if (wxGetEnv("ECCE_LOCAL_BROKER", NULL) &&
        !(wxGetEnv("ECCE_BROKER", &which) && which == "mosquitto")) {
      std::string err = MqttBroker::startForSession();
      if (!err.empty()) {
        std::cerr << "ECCE: message broker: " << err << std::endl;
        wxMessageBox("ECCE could not start its message broker: " + err,
                     "ECCE", wxOK | wxICON_ERROR);
      }
    }
  }
  ewxApp::OnInit();
#ifdef __WXMSW__
  // No gateway here: the Organizer has the session's login window.
  WxDavAuth::setRestartsOnSwitch(true);
  WxDavAuth::setOnSwitch(restartInLocalMode);
  // Before anything opens the data folder, and before the splash, which
  // would lie over the question.
  offerLocalDataMove();
#endif
#if defined(__WXMSW__) || defined(__WXMAC__)
  // ecce.cmd (and on macOS the gateway) marks the start it makes; the
  // tools started from here must not inherit the mark.
#ifdef __WXMAC__
  // The splash belongs to this process: the gateway has no windows, and
  // a window of a background app does not stay in front of the Organizer's.
  const int splashMs = 3000;
#else
  // The Organizer is up a second later, so the splash goes after two
  // seconds rather than lying over it.
  const int splashMs = 2000;
#endif
  if (wxGetEnv("ECCE_SPLASH", NULL)) {
    wxUnsetEnv("ECCE_SPLASH");
    ewxShowSplash(splashMs);
  }
#endif

  if (argc>2 && strcmp(argv[1].ToStdString().c_str(),"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2].ToStdString());
  }

  p_calcMgr = new CalcMgr(NULL);
  p_calcMgr->Show(true);
  SetTopWindow(p_calcMgr);
  registerTopShell(p_calcMgr);

  // Test hook, inert unless set: open the tree down to this URL, so tests
  // need no clicks at pixel positions.  The tree fills in lazily, so
  // retry until the node exists.
  if (const char *openUrl = getenv("ECCE_ORGANIZER_OPEN")) {
    if (*openUrl)
      openUrlWhenReady(openUrl, 20);
  }

  // Test hook, inert unless set: open the tree down to this URL, so tests
  // need no clicks at pixel positions.
  if (const char *openUrl = getenv("ECCE_ORGANIZER_OPEN")) {
    if (*openUrl) {
      string target = openUrl;
      p_calcMgr->CallAfter([this, target]() {
        p_calcMgr->findNode(EcceURL(target), true, true);
      });
    }
  }

  // Calculations left waiting for login when ECCE was last away (#208).
  p_calcMgr->CallAfter([this]() { p_calcMgr->catchUpWaitingJobs(); });

  // Job folders and temp files left by monitors that were stopped or failed
  // pile up in /tmp, which is RAM on Debian 13 (#924); off the start-up path.
  std::thread([]() { TempStorage::sweepStale(3); }).detach();

  // Desk Top Messages
  subscribe("ecce_quit", (wxJmsCBFunc)&CalcMgrApp::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&CalcMgrApp::identifyMCB);
  // ecce-launch (#687)
  subscribe("ecce_launch_request", (wxJmsCBFunc)&CalcMgrApp::launchRequestMCB);
  subscribe("ecce_rerun_request", (wxJmsCBFunc)&CalcMgrApp::rerunRequestMCB);
  // ecce-terminate (#695)
  subscribe("ecce_terminate_request", (wxJmsCBFunc)&CalcMgrApp::terminateRequestMCB);
  // ecce-annotate (#742)
  subscribe("ecce_annotate_request", (wxJmsCBFunc)&CalcMgrApp::annotateRequestMCB);
  // ecce-move / ecce-rename (#824)
  subscribe("ecce_move_request", (wxJmsCBFunc)&CalcMgrApp::moveRequestMCB);
  // ecce-workflow
  subscribe("ecce_workflow_request", (wxJmsCBFunc)&CalcMgrApp::workflowRequestMCB);
  // ecce-reconnect
  subscribe("ecce_reconnect_request", (wxJmsCBFunc)&CalcMgrApp::reconnectRequestMCB);
  // ecce-fetch
  subscribe("ecce_fetch_request", (wxJmsCBFunc)&CalcMgrApp::fetchRequestMCB);
  // ecce-reparse
  subscribe("ecce_reparse_request", (wxJmsCBFunc)&CalcMgrApp::reparseRequestMCB);
  
  // Preference Messages
  // Not filtered: the Organizer's own Preferences dialog publishes it too.
  subscribe("ecce_preferences_misc", (wxJmsCBFunc)&CalcMgrApp::miscPrefChangeMCB,
            false);

  //  startup status Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&CalcMgrApp::invokeMCB);
  subscribe("ecce_invoke_status", (wxJmsCBFunc)&CalcMgrApp::msgStatusMCB,false);

  // New ecce 2.0 msgs
  subscribe("ecce_auth_changed", (wxJmsCBFunc)&CalcMgrApp::msgAuthMCB);
  subscribe("ecce_url_renamed", (wxJmsCBFunc)&CalcMgrApp::msgRenameMCB);
  subscribe("ecce_url_created", (wxJmsCBFunc)&CalcMgrApp::msgURLCreatedMCB);
  subscribe("ecce_url_removed", (wxJmsCBFunc)&CalcMgrApp::msgURLRemovedMCB);
  subscribe("ecce_url_state", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB, false);
  subscribe("ecce_url_property", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_subject", (wxJmsCBFunc)&CalcMgrApp::msgSubjectMCB);
  subscribe("ecce_url_code", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_spin", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_theory", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_runtype", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_details", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_basis", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_launch", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_progress", (wxJmsCBFunc)&CalcMgrApp::msgProgressMCB);
  subscribe("ecce_machconf_changed", (wxJmsCBFunc)&CalcMgrApp::msgMachConfChangedMCB);
  subscribe("ecce_machreg_changed", (wxJmsCBFunc)&CalcMgrApp::msgMachRegChangedMCB);
  subscribe("ecce_data_server", (wxJmsCBFunc)&CalcMgrApp::msgDataServerMCB,false);


  // Now tell the gateway that we are ready to start the subscriber
  // connection for these subscriptions 
  // (must happen AFTER all subscribe calls):
#ifdef _WIN32
  // Windows runs no gateway process: the Organizer starts the other tools.
  registerMyselfAsAppExecer();
#endif
  startSubscriber();
  notifyReady();

  if (MqttLink::localDelivery()) {
    // Builder and Calculation Editor open as windows of this process
    // (#983); data notices go out as the tool whose window is active.
    registerBuilderInProcess();
    registerCalcEdInProcess();
    registerMDOptimizeInProcess();
    registerMDEnergyInProcess();
    registerMDDynamicsInProcess();
    registerGmxPrepareInProcess();
    registerBasisToolInProcess();
    registerMDPrepInProcess();
    string name = getMyName(), id = getMyID();
    JMSPublisher::setActingTool(name, id);
    p_calcMgr->Bind(wxEVT_ACTIVATE, [name, id](wxActivateEvent& ev) {
      if (ev.GetActive()) JMSPublisher::setActingTool(name, id);
      ev.Skip();
    });
  }

  return true;
}


void CalcMgrApp::openUrlWhenReady(const string& url, int tries)
{
  if (p_calcMgr->findNode(EcceURL(url), true, true) != 0 || tries <= 0)
    return;
  wxTimer *timer = new wxTimer();
  timer->Bind(wxEVT_TIMER, [this, url, tries](wxTimerEvent&) {
    openUrlWhenReady(url, tries - 1);
  });
  timer->StartOnce(1000);
}


int CalcMgrApp::OnExit()
{
  // Tools and job monitors still connected keep the broker for 10 s after
  // the last of them.
  MqttBroker::lingerForSession(10);
  return 0;
}


void CalcMgrApp::quitMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  if (msg.getSender().getName() == GATEWAY) {
    // Tool windows in this process first (#983): the main loop ends with
    // the Organizer's frame, and a window left open would outlive it.
    InProcessTools::closeAll(true);
    GetTopWindow()->Destroy();
  }
}


void CalcMgrApp::identifyMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  JMSMessage* reply = newMessage();
  string currentNode = p_calcMgr->getTreeSelectionName();
  reply->addProperty("info", "Organizer: " + currentNode);
  publish("ecce_identify_reply", *reply);
  delete reply;
}


// Test hook (#897): ECCE_TEST_SLOW_REQUEST names a file holding seconds; the
// next request holds the GUI thread that long (once), to show that requests
// arriving meanwhile are still acknowledged and answered.
static void slowDownForTest()
{
  const char *path = getenv("ECCE_TEST_SLOW_REQUEST");
  if (!path) return;
  FILE *h = fopen(path, "r");
  if (!h) return;
  int secs = 0;
  if (fscanf(h, "%d", &secs) != 1) secs = 0;
  fclose(h);
  unlink(path);
  if (secs > 0) sleep(secs);
}


// ecce-launch: an "ack" at once tells the command an Organizer is there; the
// answer follows when the batch has submitted.
void CalcMgrApp::launchRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  const string id = msg.getProperty("reply_id");
  auto reply = [this, &id](const string& kind, const vector<string>& lines) {
    JMSMessage* r = newMessage();
    r->addProperty("reply_id", id);
    r->addProperty("kind", kind);
    r->addIntProperty("n", (int)lines.size());
    for (size_t i = 0; i < lines.size(); i++)
      r->addProperty("line" + std::to_string(i), lines[i]);
    publish("ecce_launch_reply", *r);
    delete r;
  };
  LaunchBatch::Target target;
  target.machine = msg.getProperty("on");
  target.profile = msg.getProperty("profile");
  target.queue = msg.getProperty("queue");
  target.wall = msg.getProperty("wall");
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));

  vector<string> lines;
  const bool moved = msg.getProperty("move") == "yes";
  if (!(moved ? p_calcMgr->moveLaunchForCommand(urls, target, lines)
              : p_calcMgr->launchForCommand(urls, target, lines)))
    reply("busy", lines);
  else
    reply("done", lines);
}


// ecce-rerun: an "ack" at once tells the command an Organizer is there; the
// answer follows when the batch has submitted.
void CalcMgrApp::rerunRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  const string id = msg.getProperty("reply_id");
  auto reply = [this, &id](const string& kind, const vector<string>& lines) {
    JMSMessage* r = newMessage();
    r->addProperty("reply_id", id);
    r->addProperty("kind", kind);
    r->addIntProperty("n", (int)lines.size());
    for (size_t i = 0; i < lines.size(); i++)
      r->addProperty("line" + std::to_string(i), lines[i]);
    publish("ecce_rerun_reply", *r);
    delete r;
  };
  LaunchBatch::Target target;
  target.machine = msg.getProperty("on");
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));

  vector<string> lines;
  p_calcMgr->rerunForCommand(urls, msg.getProperty("launch") == "yes", target, lines,
                             msg.getProperty("last_geometry") == "yes",
                             strtoul(msg.getProperty("cores").c_str(), 0, 10),
                             msg.getProperty("wall"));
  reply("done", lines);
}


// ecce-annotate: as launchRequestMCB, answered on ecce_annotate_reply.
void CalcMgrApp::annotateRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  slowDownForTest();
  const string id = msg.getProperty("reply_id");
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));
  vector<string> lines;
  p_calcMgr->annotateForCommand(urls, msg.getProperty("text"), lines,
                                msg.getProperty("property"));
  JMSMessage* r = newMessage();
  r->addProperty("reply_id", id);
  r->addProperty("kind", "done");
  r->addIntProperty("n", (int)lines.size());
  for (size_t i = 0; i < lines.size(); i++)
    r->addProperty("line" + std::to_string(i), lines[i]);
  publish("ecce_annotate_reply", *r);
  delete r;
}


// ecce-move / ecce-rename: as annotateRequestMCB, answered on ecce_move_reply.
void CalcMgrApp::moveRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  slowDownForTest();
  const string id = msg.getProperty("reply_id");
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));
  vector<string> lines;
  if (msg.getProperty("verb") == "mkdir")
    p_calcMgr->mkdirForCommand(urls, msg.getProperty("second") == "parents", lines);
  else
    p_calcMgr->moveForCommand(msg.getProperty("verb"), urls,
                              msg.getProperty("second"), lines);
  JMSMessage* r = newMessage();
  r->addProperty("reply_id", id);
  r->addProperty("kind", "done");
  r->addIntProperty("n", (int)lines.size());
  for (size_t i = 0; i < lines.size(); i++)
    r->addProperty("line" + std::to_string(i), lines[i]);
  publish("ecce_move_reply", *r);
  delete r;
}


// ecce-workflow (#807, #874): one answer line, "ok <workflow URL>" or "error: ...".
void CalcMgrApp::workflowRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  string answer;
  if (msg.getProperty("verb") == "create") {
    // create (#874): calcN calculation URLs, linkN "A:B[=role]".
    vector<EcceURL> calcs;
    vector<string> links;
    for (int i = 0; !msg.getProperty("calc" + std::to_string(i)).empty(); i++)
      calcs.push_back(EcceURL(msg.getProperty("calc" + std::to_string(i))));
    for (int i = 0; !msg.getProperty("link" + std::to_string(i)).empty(); i++)
      links.push_back(msg.getProperty("link" + std::to_string(i)));
    answer = p_calcMgr->workflowCreateForCommand(
        msg.getProperty("second"), calcs, links,
        msg.getProperty("autolinks") == "1");
  } else {
    answer = p_calcMgr->workflowForCommand(
        msg.getProperty("verb"), EcceURL(msg.getProperty("calc")),
        msg.getProperty("second"));
  }
  JMSMessage* r = newMessage();
  r->addProperty("reply_id", msg.getProperty("reply_id"));
  r->addProperty("answer", answer);
  publish("ecce_workflow_reply", *r);
  delete r;
}


// ecce-terminate: as launchRequestMCB, answered on ecce_terminate_reply.
void CalcMgrApp::terminateRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  const string id = msg.getProperty("reply_id");
  auto reply = [this, &id](const string& kind, const vector<string>& lines) {
    JMSMessage* r = newMessage();
    r->addProperty("reply_id", id);
    r->addProperty("kind", kind);
    r->addIntProperty("n", (int)lines.size());
    for (size_t i = 0; i < lines.size(); i++)
      r->addProperty("line" + std::to_string(i), lines[i]);
    publish("ecce_terminate_reply", *r);
    delete r;
  };
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));

  vector<string> lines;
  if (!p_calcMgr->terminateForCommand(urls, msg.getProperty("reason"), lines))
    reply("busy", lines);
  else
    reply("done", lines);
}


// ecce-reconnect: as terminateRequestMCB, answered on ecce_reconnect_reply.
void CalcMgrApp::reconnectRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  const string id = msg.getProperty("reply_id");
  auto reply = [this, &id](const string& kind, const vector<string>& lines) {
    JMSMessage* r = newMessage();
    r->addProperty("reply_id", id);
    r->addProperty("kind", kind);
    r->addIntProperty("n", (int)lines.size());
    for (size_t i = 0; i < lines.size(); i++)
      r->addProperty("line" + std::to_string(i), lines[i]);
    publish("ecce_reconnect_reply", *r);
    delete r;
  };
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));

  vector<string> lines;
  if (!p_calcMgr->reconnectForCommand(urls, lines, msg.getProperty("jobid")))
    reply("busy", lines);
  else
    reply("done", lines);
}


// ecce-fetch: as reconnectRequestMCB, answered on ecce_fetch_reply.
void CalcMgrApp::fetchRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  const string id = msg.getProperty("reply_id");
  auto reply = [this, &id](const string& kind, const vector<string>& lines) {
    JMSMessage* r = newMessage();
    r->addProperty("reply_id", id);
    r->addProperty("kind", kind);
    r->addIntProperty("n", (int)lines.size());
    for (size_t i = 0; i < lines.size(); i++)
      r->addProperty("line" + std::to_string(i), lines[i]);
    publish("ecce_fetch_reply", *r);
    delete r;
  };
  vector<string> patterns;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    patterns.push_back(msg.getProperty("pattern" + std::to_string(i)));

  vector<string> lines;
  if (!p_calcMgr->fetchForCommand(EcceURL(msg.getProperty("url")), patterns,
                                  msg.getProperty("list") == "1", lines))
    reply("busy", lines);
  else
    reply("done", lines);
}


void CalcMgrApp::answerReparse(const string& id, const vector<string>& lines)
{
  JMSMessage* r = newMessage();
  r->addProperty("reply_id", id);
  r->addProperty("kind", "done");
  r->addIntProperty("n", (int)lines.size());
  for (size_t i = 0; i < lines.size(); i++)
    r->addProperty("line" + std::to_string(i), lines[i]);
  publish("ecce_reparse_reply", *r);
  delete r;
}


// ecce-reparse: refusals are known at once; each started calculation is
// answered when its job store sends the 'reparse' notice, and the reply
// goes out when the last one has.
void CalcMgrApp::reparseRequestMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  vector<EcceURL> urls;
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++)
    urls.push_back(EcceURL(msg.getProperty("url" + std::to_string(i))));

  vector<string> results;
  string outcome;
  p_calcMgr->reparseOutputs(urls, false, &outcome, &results);
  PendingReparse req;
  req.id = msg.getProperty("reply_id");
  for (size_t i = 0; i < results.size(); i++) {
    // name \t url \t started|refused \t reason
    vector<string> f;
    size_t from = 0, at;
    while ((at = results[i].find('\t', from)) != string::npos) {
      f.push_back(results[i].substr(from, at - from));
      from = at + 1;
    }
    f.push_back(results[i].substr(from));
    if (f.size() < 4) continue;
    if (f[2] == "started") req.waiting[f[1]] = f[0];
    else req.lines.push_back(f[0] + "\trefused\t" + f[3]);
  }
  if (req.waiting.empty()) answerReparse(req.id, req.lines);
  else p_pendingReparse.push_back(req);
}


void CalcMgrApp::reparseNotice(const string& url, const string& value)
{
  for (size_t i = 0; i < p_pendingReparse.size(); i++) {
    PendingReparse& req = p_pendingReparse[i];
    auto it = req.waiting.find(url);
    if (it == req.waiting.end()) continue;
    req.lines.push_back(it->second + (value == "done" ? "\tdone\t" :
        "\tfailed\tthe parser failed; see the calculation's run log"));
    req.waiting.erase(it);
    if (req.waiting.empty()) {
      const string id = req.id;
      const vector<string> lines = req.lines;
      p_pendingReparse.erase(p_pendingReparse.begin() + i);
      answerReparse(id, lines);
    }
    return;
  }
}


void CalcMgrApp::miscPrefChangeMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  p_calcMgr->getFeedback()->readPreferences();
  p_calcMgr->setContextPanel();
  // Icon style may have changed: rebuild the image lists, repaint the tree.
  WxState::refreshStyle();
  p_calcMgr->redrawStateIcons();
}


void CalcMgrApp::invokeMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  //  Only a request for the Organizer itself brings it forward.  Opening
  //  another tool from it must not raise it again or flag it for attention:
  //  GNOME then announces "ECCE Organizer is ready" over the new tool.
  const string app = msg.getProperty("appname");
  if (!app.empty() && app != "Organizer") return;
  p_calcMgr->Iconize(false);
  p_calcMgr->Show(true);
  ewxRaiseWindow(p_calcMgr);
  p_calcMgr->RequestUserAttention();
}


void CalcMgrApp::msgStatusMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  static const char *errMsg =
    "Unable to start the application.  "
    "Please report it at https://github.com/FriendsofECCE/ECCE/issues . "
    "Help > Report a Problem... collects the diagnostic files.";

  //  p_cursor->end_activity();
  string status = msg.getProperty("status");
  if (status == "failed") {
    p_calcMgr->getFeedback()->setMessage(errMsg, WxFeedback::SYSTEM);
  }
}


void CalcMgrApp::msgAuthMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  AuthCache::getCache().msgIn(msg, getMyID());
}


void CalcMgrApp::msgRenameMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  EcceURL oldurl(msg.getProperty("oldurl"));
  EcceURL newurl(msg.getProperty("newurl"));
  p_calcMgr->updateName(oldurl, newurl);
}


void CalcMgrApp::msgURLCreatedMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  EcceURL url(msg.getProperty("url"));
  p_calcMgr->queueUpdate(msg.getProperty("select") == "1"
                         ? CalcMgr::UPDATE_ADDED_SELECT : CalcMgr::UPDATE_ADDED, url);
}


void CalcMgrApp::msgURLRemovedMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  EcceURL url(msg.getProperty("url"));
  p_calcMgr->queueUpdate(CalcMgr::UPDATE_REMOVED, url);
}


void CalcMgrApp::msgChangedMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  EcceURL url(msg.getProperty("url"));
  if (msg.getProperty("name") == "reparse") {
    p_calcMgr->reparseFinished(url, msg.getProperty("value"));
    reparseNotice(url.toString(), msg.getProperty("value"));
  }
  p_calcMgr->queueUpdate(CalcMgr::UPDATE_CHANGED, url);
}


void CalcMgrApp::msgSubjectMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  p_calcMgr->structureStored(EcceURL(msg.getProperty("url")));
  msgChangedMCB(msg);
}


void CalcMgrApp::msgProgressMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  // A "contact" notice (#284) carries the last snapshot heard, if any; it
  // must not forget the shown one when it carries none.
  const string contact = msg.getProperty("contact");
  if (!contact.empty())
    p_calcMgr->contactUpdate(EcceURL(msg.getProperty("url")), contact,
                             msg.getProperty("progress"));
  else
    p_calcMgr->progressUpdate(EcceURL(msg.getProperty("url")),
                              msg.getProperty("progress"));
}


void CalcMgrApp::msgMachConfChangedMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  MachineConfig::markUpdateUserPrefs();
}


void CalcMgrApp::msgMachRegChangedMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  RefMachine::markUpdateUserPrefs();
  //  The queue managers too: a Reconnect after a machine's scheduler was
  //  changed wrote the old one into the monitor's configuration (#635).
  QueueManager::finalize();

  // Issue #150: Register Machines just reported a save or delete -- the
  // notice must not wait for the window to be reactivated to notice.
  if (p_calcMgr != NULL)
    p_calcMgr->refreshMachineNotice();
}


void CalcMgrApp::msgDataServerMCB(JMSMessage& msg)
{
  if (!p_calcMgr || p_calcMgr->IsBeingDeleted()) return;
  string name = msg.getProperty("name");
  string urlstr = msg.getProperty("url");
  p_calcMgr->addRoot(name, urlstr);
}
