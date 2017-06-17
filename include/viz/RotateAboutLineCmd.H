#ifndef ROTATEABOUTLINECMD_HH
#define ROTATEABOUTLINECMD_HH


#include "util/Command.H"

class RotateAboutLineCmd : public Command
{
   public:
      RotateAboutLineCmd(const string& name, Receiver * receiver);
      ~RotateAboutLineCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

   private:
      double p_x1, p_y1, p_z1;
      double p_x2, p_y2, p_z2;
      double p_angle;
      vector<int> p_highlight;
      bool p_all;

};
#endif
