/**
 * @file 
 *
 *
 */
#ifndef EVENT_HH
#define EVENT_HH

#include <string>
using std::string;


namespace ecce {

  /**
   * An Event is a general superclass for any event used to communicate 
   * between tools WITHIN an application.  Note JMS messaging used for 
   * communicating between applications.  Internal eventing provides a
   * generic mechanism for complex applications to have the same message
   * based model to coordinate activities.
   *
   * An event has a name and optional value.
   * A senderid is included for future filtering but is not currently used.
   *
   */
  class Event
  {

  public:
    Event(const string& name, const string& value = "",
          const string& senderid = "", void * obj = NULL);
    virtual ~Event();

    string getName() const;
    string getValue() const;
    string getSenderId() const;

    void * getObject() const;

  private:

    string p_name;
    string p_value;
    string p_senderId;

    void * p_object;
  };

}

#endif
