/**
 * @file 
 *
 *
 */

#ifndef RECEIEVER_HH
#define RECEIEVER_HH

#include <string>
using std::string;

/**
 * Abstract class that defines the Receiver role for the <b>Command Pattern</b>.
 * Any object that will be used as the subject of commands must implement
 * this interface.  The interface is primarily used for casting/polymorphic
 * purposes.
 */
class Receiver
{
   public:

      /** dumps summary of object state for debugging. */
      virtual void debugPrint() const { ; }

};

#endif
