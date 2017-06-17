/**
 * @file 
 *
 *
 */
#ifndef MOTIONLISTENER_HH
#define MOTIONLISTENER_HH

#include "wxviz/MotionData.H" 

/**
 * Abstract class that defines interface for objects that wich to
 * receive motion events from the 3D viewer.
 */
class MotionListener
{
public:
  /**
   * Process the motion event.
   */
  virtual void motionChanged(const MotionData& data) = 0;
};

#endif
