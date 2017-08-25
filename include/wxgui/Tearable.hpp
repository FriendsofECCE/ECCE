#ifndef TEARABLE_H_
#define TEARABLE_H_

/**
 * Classes implementing this interface represent any window that can
 * be torn from its containing window and floated.
 */
class Tearable
{    

   public:
      Tearable() {;}
      virtual ~Tearable( ) {;}

      /** Clone the UI and its current state. */
      virtual Tearable *clone(wxWindow *parent, bool constrainSize=false) = 0;

      /** Provide access to top level window (usually a panel). */
      virtual wxWindow *getTopWindow() = 0;

};

#endif // TEARABLE_H_

