///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: MqttLink.H
//
// DESCRIPTION:
//    The one MQTT 5 connection of a process (libmosquitto) behind
//    JMSPublisher, JMSSubscriber and WxJMSSubscriber.  libmosquitto runs
//    its own thread; each subscriber is an MqttEndpoint that receives the
//    messages on that thread and hands them to its owner's thread (a
//    self-pipe for the Xt job store, wxApp::CallAfter for the GUI).
//
//    Topics: ecce/<user>/<topic> for FILTER BY NONE and USER,
//    ecce/<user>/session/<host>_<display>/<topic> for USER:HOSTNAME:DISPLAY.
//    Sender and target travel as MQTT 5 user properties.
///////////////////////////////////////////////////////////////////////////////
#ifndef MQTTLINK_HH
#define MQTTLINK_HH

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "util/JMSMessage.H"

struct mosquitto;
struct mosquitto_message;
struct mqtt5__property;

// The three filter classes of data/client/config/ecce_messages.
class MqttTopicMap {
public:
  enum Filter { SESSION, NONE, USER };

  bool load(const std::string& eccemessagesPath);
  Filter filter(const std::string& topic) const;

  std::string publishTopic(const std::string& topic, const std::string& user,
                           const std::string& sessionKey) const;
  // What a subscriber asks the broker for.
  std::string subscribeFilter(const std::string& topic,
                              const std::string& user,
                              const std::string& sessionKey) const;
  // Splits ecce/<user>/[session/<key>/]<topic>; false if not ECCE's.
  static bool parse(const std::string& mqttTopic, std::string& user,
                    std::string& eccetopic);
private:
  std::map<std::string, Filter> p_filters;
};

// What the link reads from <Ecce::realUserPrefPath()>broker.
struct MqttConfig {
  std::string socket;      // Unix-socket broker, or
  std::string host;        // host + port
  int port = 1883;
  std::string user;        // topic user
  std::string sessionKey;  // <host>_<display>

  // Parses key=value lines; unknown keys are ignored.
  static bool parseFile(const std::string& path, MqttConfig& cfg);
  static std::string sanitizeLevel(const std::string& s);
};

// Where a TCP connection (central or shared broker) gets its account: the
// data server login of this session. Defined in tdat (AuthCache), which
// util cannot call; host and display are the session's, as MqttLink
// derives them. Returns false while there is no login yet.
typedef std::function<bool(const std::string& host, const std::string& display,
                           std::string& user, std::string& password)>
  MqttCredentialProvider;

// Told once per refusal reason when the broker refuses the login, so a GUI
// process can show it (util has no widgets). Called on libmosquitto's
// thread: hand it to the main thread.
typedef std::function<void(const std::string& account, const std::string& host,
                           int port, const std::string& reason)>
  MqttRefusalHandler;

struct MqttInbound {
  std::string topic;    // the ECCE topic, e.g. ecce_url_state
  std::string user;     // publisher's account, from the topic
  std::string payload;
  std::map<std::string, std::string> props;
};

// One subscriber's view of the connection.
class MqttEndpoint : public std::enable_shared_from_this<MqttEndpoint> {
public:
  explicit MqttEndpoint(const std::string& toolName);
  ~MqttEndpoint();

  // Called on the owner's thread for each accepted message.
  std::function<void(const std::string& topic, JMSMessage& msg)> handler;
  // If set, replaces the self-pipe; called on libmosquitto's thread.
  std::function<void(std::shared_ptr<MqttEndpoint>, MqttInbound)> poster;

  void add(const std::string& topic, bool filterSelf);
  bool has(const std::string& topic) const;
  bool filterSelf(const std::string& topic) const;
  std::vector<std::string> topics() const;
  void clear();

  void hold(bool h) { p_held = h; }
  const std::string& name() const { return p_name; }

  // Self-pipe for event loops that watch a file descriptor.
  bool enablePipe();
  int pipeFd() const { return p_pipe[0]; }
  // Drains the pipe and runs the handler for everything queued.
  void drain();
  // Runs the handler for one message (owner's thread).
  void process(const MqttInbound& in);

  // After this, queued or posted messages are dropped.
  void kill() { p_alive = false; }

  // libmosquitto's thread: hold, target and self filters, then hand over.
  void receive(const MqttInbound& in);
private:
  std::string p_name;
  mutable std::mutex p_lock;
  std::map<std::string, bool> p_topics;   // topic -> filterSelf
  std::atomic<bool> p_held{false};
  std::atomic<bool> p_alive{true};
  int p_pipe[2];
  std::deque<MqttInbound> p_queue;
};

class MqttLink {
public:
  static MqttLink& instance();

  // Set once, before the first connection.
  static void setCredentialProvider(MqttCredentialProvider provider);
  static void setRefusalHandler(MqttRefusalHandler handler);

  // Reads the configuration and connects (once). A missing configuration
  // is fatal after a short retry; an unreachable broker is retried by
  // libmosquitto in the background. A TCP broker needs a credential: with
  // none yet this returns false and the next call tries again.
  bool ensureConnected();
  bool connected() const { return p_connected; }

  const MqttConfig& config() const { return p_cfg; }
  const MqttTopicMap& topics() const { return p_topics; }

  bool publish(const std::string& topic, const JMSMessage& msg);

  // Subscribes to everything the endpoint has added; also after reconnects.
  void activate(const std::shared_ptr<MqttEndpoint>& ep);
  void deactivate(const std::shared_ptr<MqttEndpoint>& ep);

  // Waits for outstanding publishes, then disconnects (process exit).
  void shutdown();
private:
  MqttLink() = default;
  ~MqttLink();
  static void onConnect(mosquitto*, void*, int rc, int flags,
                        const mqtt5__property*);
  static void onDisconnect(mosquitto*, void*, int rc, const mqtt5__property*);
  static void onMessage(mosquitto*, void*, const mosquitto_message*,
                        const mqtt5__property*);
  static void onPublish(mosquitto*, void*, int mid, int rc,
                        const mqtt5__property*);
  void syncSubscriptions();   // caller holds p_lock

  mosquitto* p_mosq = nullptr;
  MqttConfig p_cfg;
  MqttTopicMap p_topics;
  std::mutex p_lock;
  std::vector<std::shared_ptr<MqttEndpoint>> p_endpoints;
  std::map<std::string, int> p_subscribed;   // broker filter -> options
  std::atomic<bool> p_connected{false};
  std::string p_lastRefusal;   // so a retry loop reports a refusal once
  bool p_warnedNoLogin = false;
  std::atomic<int> p_inflight{0};
  bool p_started = false;
};

#endif
