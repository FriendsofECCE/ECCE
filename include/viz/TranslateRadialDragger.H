#ifndef  _TRANSLATE_RADIAL_DRAGGER_
#define  _TRANSLATE_RADIAL_DRAGGER_

#include "inv/draggers/SoDragger.H"
#include "inv/fields/SoSFVec3f.H"
#include "inv/sensors/SoFieldSensor.H"

class SbLineProjector;

class TranslateRadialDragger : public SoDragger
{
   SO_KIT_HEADER(TranslateRadialDragger);

   // Catalog entries for new parts added by this class...
   SO_KIT_CATALOG_ENTRY_HEADER(translatorSwitch);
   SO_KIT_CATALOG_ENTRY_HEADER(translator);
   SO_KIT_CATALOG_ENTRY_HEADER(translatorActive);
   SO_KIT_CATALOG_ENTRY_HEADER(feedbackRotate);
   SO_KIT_CATALOG_ENTRY_HEADER(feedbackSwitch);
   SO_KIT_CATALOG_ENTRY_HEADER(feedback);
   SO_KIT_CATALOG_ENTRY_HEADER(feedbackActive);

  public:
  
   // Constructor
   TranslateRadialDragger();

   // Field that will always contain the dragger's position
   SoSFVec3f translation;

   // Initialize the class. This should be called once
   // after SoInteraction::init().
   static void initClass();

  protected:

   void orientFeedbackGeometry(const SbVec3f &localDir);

   // projector used for calculating motion along a line.
   SbLineProjector *lineProj;

   // Static callback functions invoked by SoDragger when the 
   // mouse button goes down over this dragger, when the
   // mouse drags, and when the button is released.
   static void startCB(void *, SoDragger *);
   static void motionCB(void *, SoDragger *);
   static void finishCB(void *, SoDragger *);

   // These functions, invoked by the static callback 
   // functions, do all the work of moving the dragger.
   void dragStart();
   void drag();
   void dragFinish();

   // This sensor watches for changes to the translation field
   SoFieldSensor *fieldSensor;
   static void fieldSensorCB(void *, SoSensor *);

   // This callback updates the translation field when 
   // the dragger is moved.
   static void valueChangedCB(void *, SoDragger *);

   // This will detach/attach the fieldSensor.
   // It is called at the end of the constructor (to attach).
   // and at the start/end of SoBaseKit::readInstance()
   // and on the new copy at the start/end of SoBaseKit::copy()
   // Returns the state of the node when this was called.
   virtual SbBool setUpConnections( SbBool onOff, 
				SbBool doItAlways = FALSE);

  private:

   static const char geomBuffer[];

   // Destructor.
   ~TranslateRadialDragger();
};    

#endif  /* _TRANSLATE_RADIAL_DRAGGER_ */
