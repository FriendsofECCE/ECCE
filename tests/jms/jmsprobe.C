// Driver for tests/jms: the real JMSSubscriber, JMSPublisher and AuthCache,
// without a GUI, so the test can watch what an ECCE app would see.
//
//   jmsprobe listen <topic> <seconds>   prints PORT, READY, then one GOT
//                                       line per message that was accepted
//   jmsprobe publish <topic> [key=value ...]
//   jmsprobe auth-publish <url> <user> <pass> <realm>
//   jmsprobe auth-get <url> <user> <realm>   prints FOUND <user> or NONE

#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "tdat/AuthCache.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/JMSSubscriber.H"

using namespace std;

static string g_topic;

static void got(JMSMessage& msg)
{
  cout << "GOT " << g_topic << " url=" << msg.getProperty("url")
       << " user=" << msg.getProperty("user")
       << " auth=" << (msg.getProperty("auth").empty() ? "no" : "yes")
       << " body=" << msg.bodyToString() << endl;
}

int main(int argc, char **argv)
{
  string mode = argc > 1 ? argv[1] : "";

  if (mode == "listen" && argc == 4) {
    g_topic = argv[2];
    int seconds = atoi(argv[3]);
    JMSSubscriber sub("jmsprobe");
    sub.subscribe(g_topic.c_str(), got, false);
    sub.startSubscriber();
    cout << "PORT " << sub.getPort() << endl << "READY" << endl;
    for (int i = 0; i < seconds * 5; i++) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(sub.getSocketID(), &fds);
      struct timeval tv = {0, 200000};
      if (select(sub.getSocketID() + 1, &fds, 0, 0, &tv) > 0)
        sub.processMessage();
    }
    sub.unsubscribe();
    return 0;
  }

  if (mode == "publish" && argc >= 3) {
    JMSPublisher pub("jmsprobe");
    JMSMessage *msg = pub.newMessage();
    for (int i = 3; i < argc; i++) {
      string kv = argv[i];
      size_t eq = kv.find('=');
      if (eq != string::npos)
        msg->addProperty(kv.substr(0, eq), kv.substr(eq + 1));
    }
    bool ok = pub.publish(argv[2], *msg);
    delete msg;
    return ok ? 0 : 1;
  }

  if (mode == "auth-publish" && argc == 6) {
    AuthCache::getCache().addAuthentication(argv[2], argv[3], argv[4],
                                            argv[5], true);
    return 0;
  }

  if (mode == "auth-get" && argc == 5) {
    BasicAuth *a = AuthCache::getCache().getAuthentication(argv[2], argv[3],
                                                           argv[4], 1);
    if (a) cout << "FOUND " << a->m_user << endl;
    else cout << "NONE" << endl;
    return 0;
  }

  cerr << "usage: see the comment at the top of jmsprobe.C" << endl;
  return 2;
}
