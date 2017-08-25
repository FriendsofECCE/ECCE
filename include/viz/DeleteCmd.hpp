/**
 * @file
 *
 *
 */
#ifndef  DELETECMD_H
#define  DELETECMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Command wrapper for deleting current selection.
 * This includes atoms and/or bonds.
 * The performance on this operation can be very poor if not done
 * properly.  Our rules are:
 *      . nubs must be deleted with their parents.
 *      . nubs cannot be deleted without their parents.
 *      . deleted bonds are replaced by nubs to keep the shape correct
 */
class DeleteCmd : public FragCloneCmd
{
   public:
      DeleteCmd(const string& name, Receiver * receiver);
      virtual ~DeleteCmd();

      virtual bool execute() throw(EcceException);
      //virtual string getUndoLabel();

   protected:
      virtual void init();

      string p_undoname;


};

#endif




