// Simple event and listener interface for interruptable progress events.
#ifndef PROGRESSEVENT_H
#define PROGRESSEVENT_H

#include <string>
  using std::string;

class  ProgressEvent {
  public:
  string m_msg;       // message regarding progress or action
  int    m_tick;      // how many times has this event been sent
  int    m_interval;  // seconds
  bool   m_stop;      // should it stop
};

class ProgressEventListener
{
  public:
  virtual void progressNotice(ProgressEvent& event) = 0;
};
#endif
