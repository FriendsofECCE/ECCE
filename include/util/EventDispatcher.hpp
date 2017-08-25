/**
 * @file 
 *
 *
 */
#ifndef EVENTDISPATCHER_HH
#define EVENTDISPATCHER_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/Listener.H"
#include "util/Event.H"

namespace ecce {

/**
 * The EventDispatcher is a singleton responsible for routing messages
 * between components in an applications similar to how JMS works for
 * inter-application communications.
 * The EventDispatcher initially does no "filtering" but this could be
 * added fairly easily.
 */
class EventDispatcher
{

   public:
      virtual ~EventDispatcher();
      static EventDispatcher& getDispatcher();

      void subscribe(Listener *l);
      void unsubscribe(const Listener *l);

      void publish(const Event& event);

   protected:
      EventDispatcher();

   private:
      vector<Listener*> p_listeners;

      static EventDispatcher *p_instance;


};

}

#endif
