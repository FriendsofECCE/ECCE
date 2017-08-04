#include "wxviz/MotionData.H"


MotionData::MotionData(int button, float deltaX, float deltaY, float deltaZ,
                       bool shiftDown, bool ctrlDown, MOTION_STATE state)
{
   p_button = button;

   p_deltax = deltaX;
   p_deltay = deltaY;
   p_deltaz = deltaZ;

   p_shiftDown = shiftDown;
   p_controlDown = ctrlDown;

   p_state = state;
}



MotionData::~MotionData()
{
}



bool MotionData::isButton1() const
{
   return p_button == 1;
}



bool MotionData::isButton2() const
{
   return p_button == 2;
}



bool MotionData::isButton3() const
{
   return p_button == 3;
}



float MotionData::getDeltaX() const
{
   return p_deltax;
}



float MotionData::getDeltaY() const
{
   return p_deltay;
}



float MotionData::getDeltaZ() const
{
   return p_deltaz;
}


bool MotionData::wasShiftDown() const
{
   return p_shiftDown;
}



bool MotionData::wasControlDown() const
{
   return p_controlDown;
}



bool MotionData::wasStartMotion() const
{
   return (p_state == START);
}


bool MotionData::wasFinishMotion() const
{
   return (p_state == FINISH);
}


bool MotionData::wasContinueMotion() const
{
   return (p_state == CONTINUE);
}
