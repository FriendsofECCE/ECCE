/**
 * @file
 *
 *
 */
#include <iostream>
 using namespace std;
#include "util/Event.H"
#include "util/Listener.H"
#include "util/EventDispatcher.H"

using namespace ecce;

namespace ecce {

EventDispatcher *EventDispatcher::p_instance = 0;

EventDispatcher::EventDispatcher()
{
}

EventDispatcher::~EventDispatcher()
{
}

EventDispatcher& EventDispatcher::getDispatcher()
{
   if (p_instance == 0) {
      p_instance = new EventDispatcher();
   }

   return *p_instance;
}

/**
 * Adds a subscriber.
 * The listener will only be added if its not already a listener which
 * is determined by checking Listener ids.
 */
void EventDispatcher::subscribe(Listener *l)
{
   bool doit = true;

   // First check to see if we already have this one.
   for (size_t idx=0; idx< p_listeners.size(); idx++) {
      if (p_listeners[idx] == l) {
         doit = false;
         break;
      }
   }
   if (doit) {
      p_listeners.push_back(l);
   }
}


void EventDispatcher::unsubscribe(const Listener *l)
{
   vector<Listener*>::iterator it = p_listeners.begin();
   while (it != p_listeners.end()) {
      if ((*it) == l) {
         p_listeners.erase(it);
      } else {
         it++;
      }
   }
}

void EventDispatcher::publish(const Event& event)
{
  size_t count = p_listeners.size();
  vector<Listener*>::const_iterator it = p_listeners.begin();
  while (it != p_listeners.end()) {
    (*it)->eventMCB(event);
    // A special hack to make sure when an event caused unsubscribe, the loop
    // will quit to avoid possible iterator out of bound error.
    if (p_listeners.size()<count)
      break;
    it++;
  }
}


}  // namespace
