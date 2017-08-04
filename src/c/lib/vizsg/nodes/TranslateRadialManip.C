/*
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the name of Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
 * POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*-------------------------------------------------------------
 *  This is an example from the Inventor Toolmaker,
 *  chapter 8, an example not printed in the book.
 *
 *  Source file for "TranslateRadialManip"
 *
 *----------------------------------------------------------*/

#include "inv/nodes/SoSurroundScale.H"

#include "viz/TranslateRadialDragger.H"
#include "viz/TranslateRadialManip.H"

SO_NODE_SOURCE(TranslateRadialManip);

//  Initializes the type ID for this manipulator node. This
//  should be called once after SoInteraction::init().
//  and after TranslateRadialDragger::initClass()
void
TranslateRadialManip::initClass()
{
    if (TranslateRadialDragger::getClassTypeId().isBad())
        TranslateRadialDragger::initClass();
	
  SO_NODE_INIT_CLASS(
      TranslateRadialManip, SoTransformManip, "TransformManip");		
}

//  Constructor. 
TranslateRadialManip::TranslateRadialManip()
{
   SO_NODE_CONSTRUCTOR(TranslateRadialManip);
   
   // Create a new dragger and call setDragger(), 
   // a method inherited from SoTransformManip.
   TranslateRadialDragger *myDrag = new TranslateRadialDragger;
   setDragger(myDrag);
}

TranslateRadialManip::~TranslateRadialManip()
{
}
