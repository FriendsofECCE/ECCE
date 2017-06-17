/**
 * @file 
 *
 *
 */
#ifndef LISTENER_HH
#define LISTENER_HH

#include <string>
  using std::string;

#include "util/Event.H"
  using ecce::Event;

namespace ecce {

/**
 * The Listener defines a lightweight interface for components to 
 * participate in itra-application messaging.
 * Any component that implements this interface can subscribe to messages.
 */
class Listener
{

   public:
      Listener();
      Listener(const string& id);
      virtual ~Listener();

      void subscribe(bool filterSelf=false);
      void unsubscribe();

      string getId() const;
      void setId(const string& id);

      virtual void eventMCB(const ecce::Event& event) = 0;

   protected:


   private:
      /** name or other id for this listener. */
      string p_id;


};

}

#endif
