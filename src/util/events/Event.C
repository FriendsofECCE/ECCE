/**
 * @file
 *
 *
 */
#include "util/Event.H"

namespace ecce {

  Event::Event(const string& name, const string& value, 
               const string& sender, void * obj)
    : p_name(name), p_value(value), p_senderId(sender), p_object(obj)
  {
  }


  Event::~Event()
  {
  }


  string Event::getName() const
  {
    return p_name;
  }


  string Event::getValue() const
  {
    return p_value;
  }


  string Event::getSenderId() const
  {
    return p_senderId;
  }


  void * Event::getObject() const
  {
    return p_object;
  }

}  // namespace
