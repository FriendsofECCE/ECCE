///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MqttLink.C
//
// DESIGN:
//    See include/util/MqttLink.H.  The library thread (mosquitto_loop_start)
//    does all socket I/O, reconnects by itself and calls the callbacks here;
//    clean sessions mean every connect re-subscribes everything.
///////////////////////////////////////////////////////////////////////////////
#include <mosquitto.h>
#include <mqtt_protocol.h>

#include <fcntl.h>
#include "util/PosixCompat.H"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/MqttLink.H"

using std::string;

// ------------------------------------------------------------------ topics

bool MqttTopicMap::load(const string& path)
{
  std::ifstream in(path);
  if (!in) return false;

  string line;
  Filter cur = SESSION;
  bool haveFilter = false;
  while (std::getline(in, line)) {
    size_t a = line.find_first_not_of(" \t\r");
    if (a == string::npos) continue;
    line = line.substr(a, line.find_last_not_of(" \t\r") - a + 1);
    if (line[0] == '#' || line.compare(0, 2, "//") == 0) continue;
    if (line.compare(0, 10, "FILTER BY:") == 0) {
      string f = line.substr(10);
      f.erase(0, f.find_first_not_of(" \t"));
      cur = f == "NONE" ? NONE : f == "USER" ? USER : SESSION;
      haveFilter = true;
    } else if (haveFilter) {
      p_filters[line] = cur;
    }
  }
  return !p_filters.empty();
}

MqttTopicMap::Filter MqttTopicMap::filter(const string& t) const
{
  auto it = p_filters.find(t);
  return it == p_filters.end() ? SESSION : it->second;
}

string MqttTopicMap::publishTopic(const string& t, const string& user,
                                  const string& key) const
{
  if (filter(t) == SESSION) return "ecce/" + user + "/session/" + key + "/" + t;
  return "ecce/" + user + "/" + t;
}

string MqttTopicMap::subscribeFilter(const string& t, const string& user,
                                     const string& key) const
{
  if (filter(t) == SESSION) return publishTopic(t, user, key);
  // The one notification every user of a shared broker needs from others.
  if (t == "ecce_machreg_changed") return "ecce/+/" + t;
  return "ecce/" + user + "/" + t;
}

bool MqttTopicMap::parse(const string& m, string& user, string& t)
{
  if (m.compare(0, 5, "ecce/") != 0) return false;
  size_t u = m.find('/', 5);
  if (u == string::npos) return false;
  user = m.substr(5, u - 5);
  string rest = m.substr(u + 1);
  if (rest.compare(0, 8, "session/") == 0) {
    size_t k = rest.find('/', 8);
    if (k == string::npos) return false;
    rest = rest.substr(k + 1);
  }
  t = rest;
  return !user.empty() && !t.empty();
}

// ------------------------------------------------------------------ config

string MqttConfig::sanitizeLevel(const string& s)
{
  string r = s;
  for (size_t i = 0; i < r.size(); i++)
    if (r[i] == '/' || r[i] == '+' || r[i] == '#') r[i] = '_';
  return r;
}

bool MqttConfig::parseFile(const string& path, MqttConfig& cfg)
{
  std::ifstream in(path);
  if (!in) return false;
  string line;
  while (std::getline(in, line)) {
    size_t a = line.find_first_not_of(" \t\r");
    if (a == string::npos || line[a] == '#') continue;
    size_t eq = line.find('=', a);
    if (eq == string::npos) continue;
    string key = line.substr(a, eq - a);
    string val = line.substr(eq + 1);
    key.erase(key.find_last_not_of(" \t") + 1);
    val.erase(0, val.find_first_not_of(" \t"));
    val.erase(val.find_last_not_of(" \t\r") + 1);
    if (key == "socket") cfg.socket = val;
    else if (key == "host") cfg.host = val;
    else if (key == "port") cfg.port = atoi(val.c_str());
    else if (key == "user") cfg.user = val;
    else if (key == "password") cfg.password = val;
    else if (key == "tls") cfg.tls = (val == "1");
    else if (key == "cafile") cfg.cafile = val;
    else if (key == "capath") cfg.capath = val;
    else if (key == "pinned") cfg.pinned = (val == "1");
  }
  return true;
}

// ---------------------------------------------------------------- endpoint

MqttEndpoint::MqttEndpoint(const string& toolName) : p_name(toolName)
{
  p_pipe[0] = p_pipe[1] = -1;
}

MqttEndpoint::~MqttEndpoint()
{
  if (p_pipe[0] >= 0) close(p_pipe[0]);
  if (p_pipe[1] >= 0) close(p_pipe[1]);
}

void MqttEndpoint::answerAs(const string& alias, const string& topic)
{
  std::lock_guard<std::mutex> g(p_lock);
  p_aliases[topic] = alias;
}

void MqttEndpoint::add(const string& topic, bool filterSelf)
{
  std::lock_guard<std::mutex> g(p_lock);
  p_topics[topic] = filterSelf;
}

bool MqttEndpoint::has(const string& topic) const
{
  std::lock_guard<std::mutex> g(p_lock);
  return p_topics.count(topic) != 0;
}

bool MqttEndpoint::filterSelf(const string& topic) const
{
  std::lock_guard<std::mutex> g(p_lock);
  auto it = p_topics.find(topic);
  return it == p_topics.end() ? true : it->second;
}

std::vector<string> MqttEndpoint::topics() const
{
  std::lock_guard<std::mutex> g(p_lock);
  std::vector<string> r;
  for (auto& t : p_topics) r.push_back(t.first);
  return r;
}

void MqttEndpoint::clear()
{
  std::lock_guard<std::mutex> g(p_lock);
  p_topics.clear();
  p_queue.clear();
}

bool MqttEndpoint::enablePipe()
{
  if (p_pipe[0] >= 0) return true;
#ifdef _WIN32
  // No pollable descriptor: Windows pipes cannot be made non-blocking here,
  // and the wx apps take their messages through `poster` instead.
  return false;
#endif
  if (pipe(p_pipe) != 0) {
    p_pipe[0] = p_pipe[1] = -1;
    return false;
  }
  for (int i = 0; i < 2; i++) {
    fcntl(p_pipe[i], F_SETFL, O_NONBLOCK);
    fcntl(p_pipe[i], F_SETFD, FD_CLOEXEC);
  }
  return true;
}

void MqttEndpoint::receive(const MqttInbound& in)
{
  if (!p_alive || p_held) return;

  // The receiver applies JMS's "targetid is NULL or me, targetname is NULL
  // or me" selector.
  auto get = [&in](const char* k) {
    auto it = in.props.find(k);
    return it == in.props.end() ? string() : it->second;
  };
  string tid = get("targetid"), tname = get("targetname");
  if (!tid.empty() && tid != std::to_string(getpid())) return;
  if (!tname.empty() && tname != p_name) {
    std::lock_guard<std::mutex> g(p_lock);
    auto a = p_aliases.find(in.topic);
    if (a == p_aliases.end() || a->second != tname) return;
  }

  // No Local drops this connection's own messages at the broker, but only
  // when every subscriber of the topic asked for it.
  if (filterSelf(in.topic) && get("senderid") == std::to_string(getpid()))
    return;

  if (poster) {
    poster(shared_from_this(), in);
    return;
  }
  {
    std::lock_guard<std::mutex> g(p_lock);
    p_queue.push_back(in);
  }
  if (p_pipe[1] >= 0) {
    char c = 1;
    ssize_t n = write(p_pipe[1], &c, 1);
    (void)n;   // a full pipe already has a wake-up pending
  }
}

void MqttEndpoint::drain()
{
  if (p_pipe[0] >= 0) {
    char buf[256];
    while (read(p_pipe[0], buf, sizeof(buf)) > 0) {}
  }
  for (;;) {
    MqttInbound in;
    {
      std::lock_guard<std::mutex> g(p_lock);
      if (p_queue.empty()) return;
      in = p_queue.front();
      p_queue.pop_front();
    }
    process(in);
  }
}

// What JMSMessage::loadBody accepts. Another account can publish to
// ecce_machreg_changed, and loadBody ends the process on a malformed body.
static bool wellFormedBody(string body)
{
  while (!body.empty()) {
    const char* items[] = {"NAME", "VALUE"};
    for (int i = 0; i < 2; i++) {
      string st = string(items[i]) + "START", en = string(items[i]) + "END";
      size_t a = body.find(st), b = body.find(en);
      if (a == string::npos || b == string::npos || b < a) return false;
      body.erase(a, b + en.size() - a);
    }
  }
  return true;
}

void MqttEndpoint::process(const MqttInbound& in)
{
  if (!p_alive || !handler) return;
  if (!wellFormedBody(in.payload)) return;
  auto get = [&in](const char* k) {
    auto it = in.props.find(k);
    return it == in.props.end() ? string() : it->second;
  };
  JMSMessage msg;
  msg.loadBody(in.payload);
  msg.setSender(Sender(get("sendername"), get("senderid")));
  msg.setTarget(Target(get("targetname"), get("targetid")));
  string reply = get("replytopic");
  if (!reply.empty()) msg.setReplyTopic(reply);
  handler(in.topic, msg);
}

// -------------------------------------------------------------------- link

MqttLink& MqttLink::instance()
{
  static MqttLink link;
  return link;
}

MqttLink::~MqttLink() { shutdown(); }

static MqttCredentialProvider& credentialProvider()
{
  static MqttCredentialProvider p;
  return p;
}

void MqttLink::setCredentialProvider(MqttCredentialProvider provider)
{
  credentialProvider() = provider;
}

static MqttRefusalHandler& refusalHandler()
{
  static MqttRefusalHandler h;
  return h;
}

void MqttLink::setRefusalHandler(MqttRefusalHandler handler)
{
  refusalHandler() = handler;
}

bool MqttLink::ensureConnected()
{
  std::unique_lock<std::mutex> g(p_lock);
  if (p_started) return p_mosq != nullptr;

  // The session this process belongs to (#233). One started outside any
  // session (a job store launched over ssh) joins the newest session of
  // this account on this host while its broker file is there, and passes
  // the id on to what it starts. Otherwise there is no session to
  // message: the process runs as with ECCE_NO_MESSAGING.
  string key = Ecce::sessionKey();
  if (key.empty()) {
    std::ifstream pointer(Ecce::sessionPointerFile());
    string id;
    if (pointer && std::getline(pointer, id) && Ecce::validSessionId(id)) {
      string k = Ecce::sessionKeyFor(id);
      struct stat st;
      if (stat((string(Ecce::realUserPrefPath()) + "broker_" + k).c_str(),
               &st) == 0) {
        setenv("ECCE_SESSION_ID", id.c_str(), 1);
        key = k;
        std::cerr << "MQTT: no ECCE_SESSION_ID; joined this account's newest "
                  << "session " << id << std::endl;
      }
    }
  }
  if (key.empty()) {
    std::cerr << "MQTT: no ECCE session (ECCE_SESSION_ID is not set), so "
              << "no messaging in this process" << std::endl;
    p_started = true;
    p_sessionless = true;
    return false;
  }
  p_cfg.sessionKey = MqttConfig::sanitizeLevel(key);

  // One broker file per session, so a local and a -remote session of one
  // account do not overwrite each other. Written by ecce-gateway-start
  // under the same name; this process may be started a moment before it
  // is complete, so retry briefly.
  string file = string(Ecce::realUserPrefPath()) + "broker_" + key;
  bool have = false;
  for (int i = 0; i < 50 && !(have = MqttConfig::parseFile(file, p_cfg)); i++)
    usleep(100000);
  EE_RT_ASSERT(have, EE_FATAL, "Could not open " + file);
  EE_RT_ASSERT(!p_cfg.socket.empty() || !p_cfg.host.empty(), EE_FATAL,
               "No socket= or host= in " + file);

  // A TCP broker authenticates: the account is the data server login of
  // this session, and it is also the topic user the broker's ACL allows.
  // The Unix-socket broker is private to the account and takes anyone.
  string account, password;
  bool tcp = p_cfg.socket.empty();
  if (tcp && !p_cfg.password.empty() && !p_cfg.user.empty()) {
    // A broker started for this session alone (Windows local mode): the
    // broker file carries its generated login.
    account = p_cfg.user;
    password = p_cfg.password;
  } else if (tcp) {
    if (!credentialProvider() ||
        !credentialProvider()(key, account, password) ||
        account.empty()) {
      if (!p_warnedNoLogin) {
        p_warnedNoLogin = true;
        std::cerr << "MQTT: no data server login for this session yet, so "
                  << "not connecting to the message broker " << p_cfg.host
                  << ":" << p_cfg.port << std::endl;
      }
      return false;
    }
    p_cfg.user = account;
  } else if (p_cfg.user.empty()) {
    const char* u = getenv("USER");
    struct passwd* pw = getpwuid(getuid());
    p_cfg.user = u ? u : (pw ? pw->pw_name : "");
  }
  p_cfg.user = MqttConfig::sanitizeLevel(p_cfg.user);
  p_started = true;

  string messages = string(Ecce::ecceHome()) + "/data/client/config/ecce_messages";
  EE_RT_ASSERT(p_topics.load(messages), EE_FATAL, "Could not read " + messages);

  mosquitto_lib_init();
  p_mosq = mosquitto_new(nullptr, true, this);
  EE_RT_ASSERT(p_mosq, EE_FATAL, "mosquitto_new failed");
  mosquitto_int_option(p_mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
  if (tcp) mosquitto_username_pw_set(p_mosq, account.c_str(), password.c_str());
  mosquitto_connect_v5_callback_set(p_mosq, onConnect);
  mosquitto_disconnect_v5_callback_set(p_mosq, onDisconnect);
  mosquitto_message_v5_callback_set(p_mosq, onMessage);
  mosquitto_publish_v5_callback_set(p_mosq, onPublish);
  mosquitto_reconnect_delay_set(p_mosq, 1, 5, false);

  if (tcp && p_cfg.tls) {
    // No fallback to plain TCP: a TLS broker file means TLS or nothing.
    const char* ca = p_cfg.cafile.empty() ? nullptr : p_cfg.cafile.c_str();
    const char* cp = p_cfg.capath.empty() ? nullptr : p_cfg.capath.c_str();
    int trc = (ca || cp)
      ? mosquitto_tls_set(p_mosq, ca, cp, nullptr, nullptr, nullptr)
      : MOSQ_ERR_INVAL;
    if (trc == MOSQ_ERR_SUCCESS)
      trc = mosquitto_tls_opts_set(p_mosq, 1, "tlsv1.2", nullptr);
    // A pinned certificate is the trust anchor itself, so it is the
    // certificate that is checked; the name in it need not match the
    // address the server is reached by (IP address, alias).
    if (trc == MOSQ_ERR_SUCCESS && p_cfg.pinned)
      trc = mosquitto_tls_insecure_set(p_mosq, true);
    if (trc != MOSQ_ERR_SUCCESS) {
      std::cerr << "MQTT: TLS setup: " << mosquitto_strerror(trc)
                << " (" << (ca ? ca : (cp ? cp : "no certificate source"))
                << ")" << std::endl;
      mosquitto_destroy(p_mosq);
      p_mosq = nullptr;
      return false;
    }
    mosquitto_log_callback_set(p_mosq, onLog);
  }

  int rc = p_cfg.socket.empty()
    ? mosquitto_connect_async(p_mosq, p_cfg.host.c_str(), p_cfg.port, 30)
    : mosquitto_connect_async(p_mosq, p_cfg.socket.c_str(), 0, 30);
  if (rc != MOSQ_ERR_SUCCESS) {
    std::cerr << "MQTT: connect: " << mosquitto_strerror(rc) << std::endl;
    if (rc == MOSQ_ERR_ERRNO) perror("MQTT: connect");
  }
  mosquitto_loop_start(p_mosq);

  // The callbacks take the lock.
  g.unlock();
  for (int i = 0; i < 150 && !p_connected; i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  bool refused;
  { std::lock_guard<std::mutex> g2(p_lock); refused = !p_lastRefusal.empty(); }
  if (!p_connected && !refused)
    std::cerr << "MQTT: no connection to the broker yet; retrying in the "
              << "background" << std::endl;
  return true;
}

void MqttLink::onConnect(mosquitto*, void* obj, int rc, int,
                         const mqtt5__property*)
{
  MqttLink* self = static_cast<MqttLink*>(obj);
  if (rc != 0) {
    // libmosquitto retries; say it once per reason.
    std::string why = mosquitto_reason_string(rc);
    bool fresh;
    {
      std::lock_guard<std::mutex> g(self->p_lock);
      fresh = (why != self->p_lastRefusal);
      self->p_lastRefusal = why;
    }
    if (fresh) {
      std::cerr << "MQTT: the message broker refused the connection: " << why;
      if (rc == MQTT_RC_BAD_USERNAME_OR_PASSWORD || rc == MQTT_RC_NOT_AUTHORIZED)
        std::cerr << ". It did not accept the data server login '"
                  << self->p_cfg.user << "' (" << self->p_cfg.host << ":"
                  << self->p_cfg.port << "): the account is not known "
                  << "to the broker or has another password there; ask the "
                  << "administrator (a shared broker's accounts are added with "
                  << "ecce-broker-setup --user)";
      std::cerr << std::endl;
      if ((rc == MQTT_RC_BAD_USERNAME_OR_PASSWORD || rc == MQTT_RC_NOT_AUTHORIZED) &&
          refusalHandler())
        refusalHandler()(self->p_cfg.user, self->p_cfg.host, self->p_cfg.port, why);
    }
    return;
  }
  std::lock_guard<std::mutex> g(self->p_lock);
  self->p_lastRefusal.clear();
  self->p_subscribed.clear();   // clean session: nothing survives
  self->p_connected = true;
  self->syncSubscriptions();
}

// libmosquitto reports a failed handshake only here (no CONNACK arrives).
void MqttLink::onLog(mosquitto*, void* obj, int level, const char* str)
{
  if (level != MOSQ_LOG_ERR || !str) return;
  std::string m = str;
  if (m.find("OpenSSL") == std::string::npos &&
      m.find("TLS") == std::string::npos) return;
  MqttLink* self = static_cast<MqttLink*>(obj);
  std::string why = "TLS: " + m;
  bool fresh;
  {
    std::lock_guard<std::mutex> g(self->p_lock);
    fresh = (why != self->p_lastRefusal);
    self->p_lastRefusal = why;
  }
  if (!fresh) return;
  std::cerr << "MQTT: could not set up TLS with the message broker "
            << self->p_cfg.host << ":" << self->p_cfg.port << ": " << m
            << ". The server's certificate is not the one this installation "
            << "trusts; ask the administrator." << std::endl;
  if (refusalHandler())
    refusalHandler()(self->p_cfg.user, self->p_cfg.host, self->p_cfg.port, why);
}

void MqttLink::onDisconnect(mosquitto*, void* obj, int, const mqtt5__property*)
{
  MqttLink* self = static_cast<MqttLink*>(obj);
  self->p_connected = false;
  self->p_inflight = 0;
}

void MqttLink::syncSubscriptions()
{
  // One broker subscription per filter. No Local only if every subscriber
  // of it filters itself out; the endpoints check the rest.
  std::map<string, int> want;
  for (auto& ep : p_endpoints) {
    for (auto& t : ep->topics()) {
      string f = p_topics.subscribeFilter(t, p_cfg.user, p_cfg.sessionKey);
      int opt = ep->filterSelf(t) ? MQTT_SUB_OPT_NO_LOCAL : 0;
      auto it = want.find(f);
      if (it == want.end()) want[f] = opt;
      else it->second &= opt;
    }
  }
  if (!p_connected) return;

  for (auto& w : want) {
    auto it = p_subscribed.find(w.first);
    if (it != p_subscribed.end() && it->second == w.second) continue;
    // Mosquitto does not apply changed options to an existing subscription.
    if (it != p_subscribed.end())
      mosquitto_unsubscribe_v5(p_mosq, nullptr, w.first.c_str(), nullptr);
    if (mosquitto_subscribe_v5(p_mosq, nullptr, w.first.c_str(), 1, w.second,
                               nullptr) == MOSQ_ERR_SUCCESS)
      p_subscribed[w.first] = w.second;
  }
  for (auto it = p_subscribed.begin(); it != p_subscribed.end();) {
    if (!want.count(it->first)) {
      mosquitto_unsubscribe_v5(p_mosq, nullptr, it->first.c_str(), nullptr);
      it = p_subscribed.erase(it);
    } else {
      ++it;
    }
  }
}

void MqttLink::activate(const std::shared_ptr<MqttEndpoint>& ep)
{
  {
    std::lock_guard<std::mutex> g(p_lock);
    bool known = false;
    for (auto& e : p_endpoints) known |= (e == ep);
    if (!known) p_endpoints.push_back(ep);
  }
  // Without a login yet (TCP broker) this is false; the subscriptions are
  // made when a later call connects.
  if (!ensureConnected()) return;
  std::lock_guard<std::mutex> g(p_lock);
  syncSubscriptions();
}

void MqttLink::deactivate(const std::shared_ptr<MqttEndpoint>& ep)
{
  std::lock_guard<std::mutex> g(p_lock);
  for (size_t i = 0; i < p_endpoints.size(); i++) {
    if (p_endpoints[i] == ep) {
      p_endpoints.erase(p_endpoints.begin() + i);
      break;
    }
  }
  if (p_mosq) syncSubscriptions();
}

static void addProp(mosquitto_property** p, const char* k, const string& v)
{
  if (!v.empty())
    mosquitto_property_add_string_pair(p, MQTT_PROP_USER_PROPERTY, k, v.c_str());
}

bool MqttLink::awaitingLogin()
{
  std::lock_guard<std::mutex> g(p_lock);
  return !p_started && p_warnedNoLogin;
}

bool MqttLink::publish(const string& topic, const JMSMessage& msg)
{
  if (!ensureConnected()) return false;
  if (!p_mosq || !p_connected) return false;

  mosquitto_property* props = nullptr;
  Sender s = msg.getSender();
  Target t = msg.getTarget();
  addProp(&props, "sendername", s.getName());
  addProp(&props, "senderid", s.getID());
  addProp(&props, "targetname", t.getName());
  addProp(&props, "targetid", t.getID());
  addProp(&props, "replytopic", msg.getReplyTopic());

  string body = msg.bodyToString();
  string mq = p_topics.publishTopic(topic, p_cfg.user, p_cfg.sessionKey);
  p_inflight++;
  int rc = mosquitto_publish_v5(p_mosq, nullptr, mq.c_str(), (int)body.size(),
                                body.data(), 1, false, props);
  mosquitto_property_free_all(&props);
  if (rc != MOSQ_ERR_SUCCESS) {
    p_inflight--;
    std::cerr << "MQTT: publish: " << mosquitto_strerror(rc) << std::endl;
    return false;
  }
  return true;
}

void MqttLink::onPublish(mosquitto*, void* obj, int, int,
                         const mqtt5__property*)
{
  MqttLink* self = static_cast<MqttLink*>(obj);
  if (self->p_inflight > 0) self->p_inflight--;
}

void MqttLink::onMessage(mosquitto*, void* obj, const mosquitto_message* m,
                         const mqtt5__property* props)
{
  MqttLink* self = static_cast<MqttLink*>(obj);

  MqttInbound in;
  if (!MqttTopicMap::parse(m->topic, in.user, in.topic)) return;
  in.payload.assign(static_cast<const char*>(m->payload), m->payloadlen);

  for (const mosquitto_property* p = props; p; p = mosquitto_property_next(p)) {
    char* k = nullptr;
    char* v = nullptr;
    if (mosquitto_property_identifier(p) == MQTT_PROP_USER_PROPERTY &&
        mosquitto_property_read_string_pair(p, MQTT_PROP_USER_PROPERTY,
                                            &k, &v, false))
      in.props[k] = v;
    free(k);
    free(v);
  }

  std::lock_guard<std::mutex> g(self->p_lock);
  for (auto& ep : self->p_endpoints)
    if (ep->has(in.topic)) ep->receive(in);
}

void MqttLink::shutdown()
{
  mosquitto* m;
  {
    std::lock_guard<std::mutex> g(p_lock);
    m = p_mosq;
    p_mosq = nullptr;
  }
  if (!m) return;

  // Publishes are asynchronous; a short-lived tool must not exit before
  // the broker has them.
  for (int i = 0; i < 200 && p_inflight > 0 && p_connected; i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  mosquitto_disconnect(m);
  mosquitto_loop_stop(m, false);
  mosquitto_destroy(m);
  mosquitto_lib_cleanup();
}
