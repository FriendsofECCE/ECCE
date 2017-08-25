/**
 * @file 
 *
 *
 */
#ifndef WINDOWEVENT_HH
#define WINDOWEVENT_HH

#include <string>
using std::string;

#include "util/Event.H"
  using namespace ecce;


class wxWindow;

namespace ecce {

/**
 * Provides notification that something changed for a window.
 * The event name indicates what changed so a listener can take 
 * appropriate action.
 *
 * The use case is that you want to hear about events related to  another 
 * window (not related to yours in the hierarchy).  I found that using Connect
 * worked but caused infinite calling of wxEvtHandler destruction when you
 * try to disconnect.
 *
 */
class WindowEvent : public Event
{

   public:
      WindowEvent(const string& name, const string& value, wxWindow *win) 
          : Event(name, value) { p_window = win;}
      virtual ~WindowEvent() { p_window = 0; }

      wxWindow *getWindow() const { return p_window; }

   private:

      wxWindow *p_window;


};

}

#endif
