#ifndef MQTTBROKER_HH
#define MQTTBROKER_HH

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>

// A small MQTT 5 broker for a Windows local session (#990): loopback only,
// one thread (poll), clean sessions, QoS 0 and 1, no retained messages or
// wills (MqttLink uses neither).  Accounts live in memory.  The access rules
// are those of packaging/gateway/ecce-mosquitto.acl: a user reads and writes
// ecce/<user>/#, and everybody reads ecce/+/ecce_machreg_changed.
class MqttBroker {
public:
  MqttBroker();
  ~MqttBroker();

  void addUser(const std::string& user, const std::string& password);

  // Listens on 127.0.0.1; port 0 takes a free one.  False if it cannot.
  bool start(int port = 0);
  void stop();
  int port() const { return p_port; }
  // Clients that have completed CONNECT and are still connected.
  int clients() const;

  // The ACL as functions of a verified account, for tests.
  static bool mayWrite(const std::string& user, const std::string& topic);
  static bool mayRead(const std::string& user, const std::string& topic);
  static bool topicMatches(const std::string& filter, const std::string& topic);

  // The session broker of a Windows local session: a login made up for it,
  // a free loopback port, and the session's broker file and pointer written
  // as ecce-broker-win does.  Needs ECCE_SESSION_ID and ECCE_REALUSERHOME.
  // Returns an error text, empty on success.
  static std::string startForSession();
  static void stopForSession();   // stops it and removes the broker file
  // The Organizer's end of the session: disconnects this process's own link,
  // then keeps serving until no client (Builder, job stores, ...) has been
  // connected for graceSeconds, and stops as stopForSession().  Returns at
  // once if nobody else is connected.
  static void lingerForSession(int graceSeconds);

private:
  struct Impl;
  std::unique_ptr<Impl> p_impl;
  int p_port = 0;
};

#endif
