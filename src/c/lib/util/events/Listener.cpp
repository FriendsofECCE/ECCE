/**
 * @file
 *
 *
 */
#include "util/Listener.hpp"
#include "util/EventDispatcher.hpp"

using namespace ecce;

namespace ecce {

Listener::Listener()
  : p_id("")
{
}

Listener::Listener(const string& id)
  : p_id(id)
{
}

Listener::~Listener()
{
}

string Listener::getId() const
{
   return p_id;
}

void Listener::setId(const string& id)
{
  p_id = id;
}

/**
 * @param filterSelf - controls whether sender recieves own messages. 
 *       NOT IMPLEMTED
 */
void Listener::subscribe(bool filterSelf)
{
   EventDispatcher::getDispatcher().subscribe(this /*,filterSelf*/);
}


void Listener::unsubscribe()
{
   EventDispatcher::getDispatcher().unsubscribe(this);
}


}  // namespace
