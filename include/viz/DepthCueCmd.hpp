/**
 * @file
 *
 *
 */
#ifndef  DEPTHCUECMD_H
#define  DEPTHCUECMD_H

#include "util/Command.hpp"

class Fragment;

class DepthCueCmd : public Command
{
   public:
      DepthCueCmd(const string& name, Receiver * receiver);
      virtual ~DepthCueCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      virtual void init();


};

#endif




