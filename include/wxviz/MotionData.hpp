/**
 * @file 
 *
 *
 */
#ifndef MOTIONDATA_HH
#define MOTIONDATA_HH

/**
 * Class containing data about motion events.
 * @see MotionListener
 */
class MotionData
{
   public:
      enum MOTION_STATE { START, CONTINUE, FINISH};

      MotionData(int button, float deltaX, float deltaY, float deltaZ,
                 bool shiftDown, bool controlDown, MOTION_STATE state);
      virtual ~MotionData();

      bool isButton1() const;
      bool isButton2() const;
      bool isButton3() const;

      float getDeltaX() const;
      float getDeltaY() const;
      float getDeltaZ() const;

      bool  wasShiftDown() const;
      bool  wasControlDown() const;

      bool wasStartMotion() const;
      bool wasFinishMotion() const;
      bool wasContinueMotion() const;


   protected:
      int p_button;

      bool p_shiftDown;

      bool p_controlDown;

      float p_deltax;

      float p_deltay;

      float p_deltaz;
      
      MOTION_STATE p_state;

};

#endif
