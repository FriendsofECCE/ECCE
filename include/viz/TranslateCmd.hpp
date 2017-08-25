#ifndef TRANSLATECMD_HH
#define TRANSLATECMD_HH


#include "util/Command.H"

class TranslateCmd : public Command
{
   public:
      TranslateCmd(const string& name, Receiver * receiver);
      ~TranslateCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

   private:
      double p_x, p_y, p_z;
      vector<int> p_highlight;
      bool p_all;

};
#endif
