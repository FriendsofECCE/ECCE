#ifndef MQTTBROKER_HH
#define MQTTBROKER_HH

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>

// A small MQTT 5 broker for a local session (#990, #1007): one thread (poll),
// clean sessions, QoS 0 and 1, no retained messages or wills (MqttLink uses
// neither).  On TCP (127.0.0.1, Windows) it checks the in-memory accounts and
// applies the rules of packaging/gateway/ecce-mosquitto.acl: a user reads and
// writes ecce/<user>/#, and everybody reads ecce/+/ecce_machreg_changed.  On a
// Unix socket (Linux, macOS) it takes anyone and applies no rules, as the
// per-user mosquitto's socket did: the 0700 directory keeps other accounts out.
class MqttBroker {
public:
  MqttBroker();
  ~MqttBroker();

  void addUser(const std::string& user, const std::string& password);

  // Listens on 127.0.0.1; port 0 takes a free one.  False if it cannot.
  bool start(int port = 0);
  // Listens on a Unix socket at path (not on Windows), mode 0600.
  bool start(const std::string& path);
  void stop();
  int port() const { return p_port; }
  const std::string& socketPath() const { return p_path; }
  // Clients that have completed CONNECT and are still connected.
  int clients() const;

  // The ACL as functions of a verified account, for tests.
  static bool mayWrite(const std::string& user, const std::string& topic);
  static bool mayRead(const std::string& user, const std::string& topic);
  static bool topicMatches(const std::string& filter, const std::string& topic);

  // The broker of a local session and the session's broker file and pointer.
  // Windows: a login made up for it on a free loopback port.  Linux, macOS:
  // a Unix socket in ~/.ECCE ("socket="), and nothing is started when the
  // file already names a socket that answers (hostsSession() then says no).
  // Needs ECCE_SESSION_ID and ECCE_REALUSERHOME.  Returns an error text,
  // empty on success.
  static std::string startForSession();
  static bool hostsSession();
  static void stopForSession();   // stops it and removes the broker file
  // The Organizer's end of the session: disconnects this process's own link,
  // then keeps serving until no client (Builder, job stores, ...) has been
  // connected for graceSeconds, and stops as stopForSession().  Returns at
  // once if nobody else is connected.
  static void lingerForSession(int graceSeconds);
  // A broker program's life (eccebroker --serve, Linux and macOS): serves
  // until no client has been connected for graceSeconds (startSeconds before
  // the first one), the broker file stops naming its socket (ecce-gateway-
  // stop), or requestStop() (signal-safe); then as stopForSession().
  static void serveForSession(int graceSeconds, int startSeconds);
  static void requestStop();

private:
  struct Impl;
  std::unique_ptr<Impl> p_impl;
  int p_port = 0;
  std::string p_path;
};

#endif
