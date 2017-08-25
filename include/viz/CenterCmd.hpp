#ifndef CENTERCMD_HH
#define CENTERCMD_HH


#include "util/Command.H"

#include <vector>
  using std::vector;

class CenterCmd : public Command
{
   public:
      CenterCmd(const string& name, Receiver * receiver);
      ~CenterCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual string getUndoLabel();

   protected:
      void init();

   private:
      double p_x, p_y, p_z;
      vector<int> p_highLight;
      bool p_all;
      string p_label;
};
#endif
