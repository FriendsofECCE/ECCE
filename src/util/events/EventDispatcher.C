/**
 * @file
 *
 *
 */
#include <algorithm>
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
   // erase(it) invalidates it, so it must be reseated from erase's return
   // value rather than reused as-is on the next loop check.
   vector<Listener*>::iterator it = p_listeners.begin();
   while (it != p_listeners.end()) {
      if ((*it) == l) {
         it = p_listeners.erase(it);
      } else {
         it++;
      }
   }
}

void EventDispatcher::publish(const Event& event)
{
  // A listener may subscribe others (the vector reallocates) or unsubscribe
  // and destroy some while handling the event: walk a copy and skip any
  // that left in the meantime.
  const vector<Listener*> snapshot(p_listeners);
  for (Listener *l : snapshot) {
    if (std::find(p_listeners.begin(), p_listeners.end(), l) !=
        p_listeners.end())
      l->eventMCB(event);
  }
}


}  // namespace
