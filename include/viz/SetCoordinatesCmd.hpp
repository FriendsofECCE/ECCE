/**
 * @file
 *
 *
 */

#ifndef SETCOORDINATESCMD_HH
#define SETCOORDINATESCMD_HH

#include "util/Command.H"


class SetCoordinatesCmd : public Command
{
   public:
      SetCoordinatesCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetCoordinatesCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      double p_x;
      double p_y;
      double p_z;
};
#endif
