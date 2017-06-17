/*-------------------------------------------------------------
 *  This is an example from the Inventor Toolmaker,
 *  chapter 8, an example not printed in the book.
 *
 *  Header file for "TranslateRadialManip"
 *
 *  An example of how create an SoTransformManip
 *  that uses a new kind of dragger.
 *  This is an SoTransformManip that employs a 
 *  TranslateRadialDragger.  The dragger provides a user 
 *  interface for editting the fields of the manip (which
 *  are inherited from SoTransform)
 *
 *----------------------------------------------------------*/

#ifndef  _TRANSLATE_RADIAL_MANIP
#define  _TRANSLATE_RADIAL_MANIP_

#include "inv/manips/SoTransformManip.H"


class TranslateRadialManip : public SoTransformManip
{
   SO_NODE_HEADER(TranslateRadialManip);

  public:
  
   // Constructor
   TranslateRadialManip();

   // Initialize the class. This should be called once
   // after SoInteraction::init() and after 
   // TranslateRadialDragger::init().
   static void initClass();

  private:

   // Destructor.
   ~TranslateRadialManip();
};    

#endif  /* _TRANSLATE_RADIAL_MANIP_ */
