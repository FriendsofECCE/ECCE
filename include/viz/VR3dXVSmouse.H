#ifndef  __VR_3D_XVS_MOUSE_H__
#define  __VR_3D_XVS_MOUSE_H__

//required free includes  //

#ifdef XVS_OK
#include "synclink/XvsSyncLink.h"
#endif

// required commercial includes  //

#include "inv/nodes/SoSubNode.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoBlinker.H"
#include "inv/nodes/SoSwitch.H"

class SoSensor;
class RTDraggerNode;

class  VR3dXVSmouse : public SoGroup {

  SO_NODE_HEADER(VR3dXVSmouse);


  public:

  VR3dXVSmouse(); 
  RTDraggerNode* getDragger();
  void doReposition();
  static void animationSensorCB(void *, SoSensor *);

  SoINTERNAL public:
    static void		initClass();

  protected:
  virtual  ~VR3dXVSmouse();
  void  constructor();

  private:

  RTDraggerNode * mickeyDragg;

#ifdef XVS_OK

  XvsSensor* sensor;          // Sensor object
  XvsPosition pval;           // Position data
  XvsEulerAngles aval;        // Euler angles data
  XvsToggles* tval;           // Toggle data (variable size)
  XvsDeviceData* dval;        // Device-specific data (variable size)
  int maxToggles;             // Number of toggles sensor reports
  int maxDeviceData;          // How much device data sensor reports
  char inputBuffer [80];      // User input from standard input
  char filename [80];         // Device file name
  int baudrate;               // Device baudrate
  int iterations;             // Number of times to test device
  int i;                      // Iterations counter
  int r;                      // Number of data elements read from sensor

#endif


};




#endif /* !__VR_3D_XVS_MOUSE_H__*/

