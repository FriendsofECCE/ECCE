#ifndef ALIGNCMD_HH
#define ALIGNCMD_HH


#include "util/Command.H"

class AlignCmd : public Command
{
   public:
      AlignCmd(const string& name, Receiver * receiver);
      ~AlignCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

   private:
      double p_x3, p_y3, p_z3;
      double p_angle;
      vector<int> p_highlight;
      bool p_all;

};
#endif
